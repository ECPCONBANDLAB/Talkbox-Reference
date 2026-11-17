// TalkBox Reference — PluginProcessor implementation
#include "PluginProcessor.h"
#include "PluginEditor.h"

TalkBoxReferenceProcessor::TalkBoxReferenceProcessor()
    : AudioProcessor(BusesProperties()
          // Bus 0: stereo carrier in. Bus 1: mono sidechain = talkbox mic.
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::mono(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("TalkBoxReference"), createLayout()) {
    for (auto& a : visF) a.store(500.f);
    ensureScratch(4096);
}

juce::AudioProcessorValueTreeState::ParameterLayout
TalkBoxReferenceProcessor::createLayout() {
    using juce::ParameterID;
    using juce::AudioParameterFloat;
    using juce::AudioParameterChoice;
    using juce::AudioParameterBool;
    using juce::NormalisableRange;
    using juce::StringArray;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.reserve(48);

    auto addFloat = [&params](const char* id, float def, float lo, float hi) {
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID(id, 1), id, NormalisableRange<float>(lo, hi), def));
    };
    auto addChoice = [&params](const char* id, int def, std::initializer_list<const char*> items) {
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID(id, 1), id, StringArray(items), def));
    };
    auto addBool = [&params](const char* id, bool def) {
        params.push_back(std::make_unique<AudioParameterBool>(ParameterID(id, 1), id, def));
    };

    // Header / modes
    addChoice("sourceMode", 0, {"Pad/MIDI", "Live Mic Control"});
    addChoice("carrierMode", 0, {"External Audio", "Internal Talkbox Synth"});
    addChoice("quality", 1, {"Eco", "Balanced", "Studio", "Creative"});
    addBool("bypass", false);
    // Simple macros
    addFloat("carrierDrive", 0.6f, 0.f, 1.f);
    addFloat("tubeCharacter", 0.5f, 0.f, 1.f);
    addFloat("mouthSize", 1.0f, 0.75f, 1.35f);
    addFloat("articSmooth", 0.3f, 0.f, 1.f);
    addFloat("consonantCrisp", 0.7f, 0.f, 1.f);
    addFloat("micProximity", 0.5f, 0.f, 1.f);
    addFloat("dryWet", 1.0f, 0.f, 1.f);
    addFloat("outputGain", 1.0f, 0.f, 2.f);
    // Advanced: Vocal Tract
    addFloat("padX", 0.5f, 0.f, 1.f); addFloat("padY", 0.45f, 0.f, 1.f);
    addFloat("jaw", 0.45f, 0.f, 1.f); addFloat("tongueX", 0.5f, 0.f, 1.f);
    addFloat("tongueHeight", 0.45f, 0.f, 1.f); addFloat("lipRounding", 0.f, 0.f, 1.f);
    addFloat("mouthOpening", 0.8f, 0.f, 1.f); addFloat("nasal", 0.f, 0.f, 1.f);
    addFloat("vtl", 1.0f, 0.5f, 2.0f); addFloat("glide", 0.3f, 0.f, 1.f);
    // Advanced: Mic & Carrier
    addChoice("micRouting", 0, {"Sidechain", "L-R Split"});
    addFloat("micPreGain", 1.0f, 0.f, 4.f); addFloat("gateThresh", 0.008f, 0.0005f, 0.05f);
    addChoice("gateBehavior", 0, {"Close Mouth Naturally", "Hold Last Vowel", "Return to Pad"});
    addFloat("whisperBoost", 1.0f, 0.5f, 3.f); addFloat("sibilDeess", 0.3f, 0.f, 1.f);
    addFloat("voiceBleed", 0.0f, 0.f, 1.f); addFloat("micSmooth", 0.6f, 0.f, 1.f);
    addChoice("synthWave", 0, {"Saw", "Pulse"});
    addFloat("synthOctave", 0.f, -2.f, 2.f); addFloat("synthGlide", 0.06f, 0.f, 0.5f);
    addFloat("synthVibrato", 0.f, 0.f, 1.f);
    // Advanced: Hardware Chain
    addFloat("driverComp", 0.3f, 0.f, 1.f); addFloat("diaphragmAsym", 0.f, -0.5f, 0.5f);
    addFloat("hornCut", 6500.f, 4000.f, 9000.f);
    addFloat("hoseLength", 1.75f, 1.0f, 2.5f); addFloat("hoseDamping", 0.3f, 0.f, 1.f);
    addChoice("micModel", 0, {"Classic Dynamic 57/58", "Crisp Studio Condenser",
                              "Warm Ribbon", "Flat Direct"});
    addFloat("micDistance", 3.f, 1.f, 30.f); addFloat("proxBoost", 0.5f, 0.f, 1.f);
    addFloat("ambience", 0.f, 0.f, 1.f);
    // Carrier conditioning
    addFloat("inputGain", 1.0f, 0.f, 4.f);

    return {params.begin(), params.end()};
}

void TalkBoxReferenceProcessor::ensureScratch(int numSamples) {
    const auto need = (size_t)numSamples;
    if (micScratch.size() < need) {
        micScratch.assign(need, 0.f);
        carrierMono.assign(need, 0.f);
        outL.assign(need, 0.f);
        outR.assign(need, 0.f);
    }
}

void TalkBoxReferenceProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.prepare((float)sampleRate, samplesPerBlock);
    ensureScratch(juce::jmax(samplesPerBlock, 128));
    syncParams();
}

void TalkBoxReferenceProcessor::releaseResources() { engine.reset(); }

bool TalkBoxReferenceProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Main out must be stereo; main in stereo; optional mono sidechain.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    // Input bus 1 (sidechain) may be absent or mono.
    if (layouts.inputBuses.size() > 1) {
        const auto sc = layouts.inputBuses[1];
        if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono())
            return false;
    }
    return layouts.inputBuses.size() <= 2;
}

void TalkBoxReferenceProcessor::syncParams() {
    auto& p = engine.p;
    auto getF = [this](const char* id) {
        if (auto* v = apvts.getRawParameterValue(id)) return v->load();
        return 0.0f;
    };
    p.source = (getF("sourceMode") > 0.5f) ? tb::SourceMode::Mic : tb::SourceMode::Pad;
    p.carrier = (getF("carrierMode") > 0.5f) ? tb::CarrierMode::Internal : tb::CarrierMode::External;
    int q = (int)getF("quality");
    p.quality = (q == 0) ? tb::QualityMode::Eco
              : (q == 1) ? tb::QualityMode::Balanced
              : (q == 2) ? tb::QualityMode::Studio : tb::QualityMode::Creative;
    p.bypass = getF("bypass") > 0.5f;
    p.carrierDrive = getF("carrierDrive");
    p.tubeCharacter = getF("tubeCharacter");
    p.mouthSize = getF("mouthSize");
    p.articSmooth = getF("articSmooth");
    p.micProximity = getF("micProximity");
    p.ambience = getF("ambience");
    p.synthOctave = getF("synthOctave");
    p.vtl = getF("vtl"); p.glide = getF("glide");
    p.padX = getF("padX"); p.padY = getF("padY");
    p.jaw = getF("jaw"); p.tongueX = getF("tongueX"); p.tongueHeight = getF("tongueHeight");
    p.lipRounding = getF("lipRounding"); p.mouthOpening = getF("mouthOpening");
    p.nasal = getF("nasal");
    p.micPreGain = getF("micPreGain"); p.gateThresh = getF("gateThresh");
    int gb = (int)getF("gateBehavior");
    p.gateBehavior = (gb == 0) ? tb::GateBehavior::CloseMouth
                   : (gb == 1) ? tb::GateBehavior::HoldVowel
                               : tb::GateBehavior::ReturnToPad;
    p.whisperBoost = getF("whisperBoost"); p.consonantCrisp = getF("consonantCrisp");
    p.sibilDeess = getF("sibilDeess"); p.voiceBleed = getF("voiceBleed");
    p.micSmooth = getF("micSmooth");
    p.synthWave = (int)getF("synthWave"); p.synthGlide = getF("synthGlide");
    p.synthVibrato = getF("synthVibrato");
    p.driverComp = getF("driverComp"); p.diaphragmAsym = getF("diaphragmAsym");
    p.hornCut = getF("hornCut");
    p.hoseLength = getF("hoseLength"); p.hoseDamping = getF("hoseDamping");
    p.micModel = (int)getF("micModel"); p.micDistance = getF("micDistance");
    p.proxBoost = getF("proxBoost");
    p.dryWet = getF("dryWet"); p.outputGain = getF("outputGain");
    p.inputGain = getF("inputGain");
    engine.updateQuality();
}

void TalkBoxReferenceProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n <= 0) return;

    ensureScratch(n);
    syncParams();

    const int numIn = getTotalNumInputChannels();
    const int numOut = buffer.getNumChannels();

    // Clear any output channels we do not write (keep hosts happy).
    for (int ch = 2; ch < numOut; ++ch) buffer.clear(ch, 0, n);

    auto* carrierL = numIn > 0 ? buffer.getReadPointer(0) : nullptr;
    auto* carrierR = numIn > 1 ? buffer.getReadPointer(1) : carrierL;

    // Mic source: "Sidechain" bus (input bus 1) or L-R split.
    const int routing = (int)(apvts.getRawParameterValue("micRouting")->load());
    const float* micPtr = nullptr;
    if (routing == 0 && getBusCount(true) > 1 && numIn > 2) {
        // Sidechain bus channels follow the main input channels in the buffer.
        micPtr = buffer.getReadPointer(2);
    } else if (routing == 1 && carrierR) {
        // L-R Split: right channel carries the mic.
        for (int i = 0; i < n; ++i) micScratch[(size_t)i] = carrierR[i];
        micPtr = micScratch.data();
    } else {
        std::fill(micScratch.begin(), micScratch.begin() + n, 0.f);
        micPtr = micScratch.data();
    }

    // MIDI -> internal synth
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isNoteOn())       engine.midiNoteOn(m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) engine.midiNoteOff(m.getNoteNumber());
        else if (m.isPitchWheel())
            engine.synth.pitchBend = (m.getPitchWheelValue() - 8192.f) / 8192.f * 2.f;
    }

    // Mono carrier sum for the engine (dual-mono output downstream).
    for (int i = 0; i < n; ++i) {
        const float l = carrierL ? carrierL[i] : 0.f;
        const float r = carrierR ? carrierR[i] : l;
        carrierMono[(size_t)i] = (l + r) * 0.5f;
    }

    engine.processBlock(carrierMono.data(), micPtr, outL.data(), outR.data(), n);

    buffer.clear();
    buffer.copyFrom(0, 0, outL.data(), n);
    if (numOut > 1) buffer.copyFrom(1, 0, outR.data(), n);

    // Visualizer taps
    for (int i = 0; i < 5; ++i) visF[i].store(engine.visF[i]);
    visMouth.store(engine.visMouth);
    visGate.store(engine.visGate);
    visSib.store(engine.visSib);
}

void TalkBoxReferenceProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
}

void TalkBoxReferenceProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // readFromStream takes an lvalue reference, so the stream needs a name.
    juce::MemoryInputStream in(data, (size_t)sizeInBytes, false);
    auto tree = juce::ValueTree::readFromStream(in);
    if (tree.isValid()) apvts.state = tree;
}

juce::AudioProcessorEditor* TalkBoxReferenceProcessor::createEditor() {
    return new TalkBoxReferenceEditor(*this);
}

// ---------------------------------------------------------------------------
// Entry point required by every JUCE plugin wrapper (VST3, AU, Standalone).
// Without this symbol the wrappers link against an undefined reference to
// createPluginFilter() and every target fails at link time.
// ---------------------------------------------------------------------------
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TalkBoxReferenceProcessor();
}
