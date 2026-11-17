// TalkBox Reference — PluginProcessor implementation
#include "PluginProcessor.h"
#include "PluginEditor.h"

TalkBoxReferenceProcessor::TalkBoxReferenceProcessor()
    : AudioProcessor(BusesProperties()
          // Bus 0: stereo carrier in. Bus 1: mono sidechain = talkbox mic.
          // Bus 0: main stereo insert (carrier audio in).
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          // Bus 1: OPTIONAL mic/sidechain. Marked non-required: a host that
          // cannot route a second input (FL Studio's default) then simply
          // leaves it unconnected instead of silencing the effect.
          .withInput("Mic / Sidechain", juce::AudioChannelSet::mono(), false)
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
    auto addPct = [&params](const char* id, float defPct) {
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID(id, 1), id, NormalisableRange<float>(0.f, 100.f), defPct));
    };
    auto addDb = [&params](const char* id, float defDb, float lo, float hi) {
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID(id, 1), id, NormalisableRange<float>(lo, hi), defDb));
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
    addChoice("carrierMode", 0, {"External Audio", "Internal Talkbox Synth"});  // 0 = audio in
    addChoice("quality", 1, {"Eco", "Balanced", "Studio", "Creative"});
    addBool("bypass", false);
    // Simple macros
    addPct("carrierDrive", 60.f);
    addPct("tubeCharacter", 50.f);
    addFloat("mouthSize", 1.0f, 0.75f, 1.35f);
    addPct("articSmooth", 30.f);
    addPct("consonantCrisp", 70.f);
    addPct("micProximity", 50.f);
    addPct("dryWet", 100.f);
    addDb("outputGain", 0.f, -24.f, 12.f);
    // Advanced: Vocal Tract
    addPct("padX", 50.f); addPct("padY", 45.f);
    addPct("jaw", 45.f); addPct("tongueX", 50.f);
    addPct("tongueHeight", 45.f); addPct("lipRounding", 0.f);
    addPct("mouthOpening", 80.f); addPct("nasal", 0.f);
    addFloat("vtl", 1.0f, 0.5f, 2.0f); addPct("glide", 30.f);
    // Advanced: Mic & Carrier
    addChoice("micRouting", 0, {"Sidechain", "L-R Split"});
    addDb("micPreGain", 0.f, -12.f, 24.f); addDb("gateThresh", -42.f, -66.f, -26.f);
    addChoice("gateBehavior", 0, {"Close Mouth Naturally", "Hold Last Vowel", "Return to Pad"});
    addFloat("whisperBoost", 1.0f, 0.5f, 3.f); addPct("sibilDeess", 30.f);
    addPct("voiceBleed", 0.f); addPct("micSmooth", 60.f);
    addChoice("synthWave", 0, {"Saw", "Pulse"});
    addFloat("synthOctave", 0.f, -2.f, 2.f); addFloat("synthGlide", 0.06f, 0.f, 0.5f);
    addFloat("synthVibrato", 0.f, 0.f, 1.f);
    addFloat("arpRate", 8.f, 0.5f, 20.f);
    // Advanced: Hardware Chain
    addPct("driverComp", 30.f); addFloat("diaphragmAsym", 0.f, -50.f, 50.f);
    addFloat("hornCut", 6500.f, 4000.f, 9000.f);
    addFloat("hoseLength", 1.75f, 1.0f, 2.5f); addPct("hoseDamping", 30.f);
    addChoice("micModel", 0, {"Classic Dynamic 57/58", "Crisp Studio Condenser",
                              "Warm Ribbon", "Flat Direct"});
    addFloat("micDistance", 3.f, 1.f, 30.f); addPct("proxBoost", 50.f);
    addPct("ambience", 0.f);
    // Carrier conditioning
    addDb("inputGain", 0.f, -24.f, 12.f);

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
    // Parameters carry real units so the UI readouts are honest (a knob
    // labelled "dB" shows decibels). The engine works in linear gains and
    // normalised 0..1, so every conversion happens here, once per block.
    auto pct   = [&getF](const char* id) { return getF(id) * 0.01f; };
    auto dbLin = [&getF](const char* id) { return std::pow(10.0f, getF(id) / 20.0f); };
    p.source = (getF("sourceMode") > 0.5f) ? tb::SourceMode::Mic : tb::SourceMode::Pad;
    p.carrier = (getF("carrierMode") > 0.5f) ? tb::CarrierMode::Internal : tb::CarrierMode::External;
    int q = (int)getF("quality");
    p.quality = (q == 0) ? tb::QualityMode::Eco
              : (q == 1) ? tb::QualityMode::Balanced
              : (q == 2) ? tb::QualityMode::Studio : tb::QualityMode::Creative;
    p.bypass = getF("bypass") > 0.5f;
    p.carrierDrive = pct("carrierDrive");
    p.tubeCharacter = pct("tubeCharacter");
    p.mouthSize = getF("mouthSize");
    p.articSmooth = pct("articSmooth");
    p.micProximity = pct("micProximity");
    p.ambience = pct("ambience");
    p.synthOctave = getF("synthOctave");
    p.vtl = getF("vtl"); p.glide = pct("glide");
    p.padX = pct("padX"); p.padY = pct("padY");
    p.jaw = pct("jaw"); p.tongueX = pct("tongueX"); p.tongueHeight = pct("tongueHeight");
    p.lipRounding = pct("lipRounding"); p.mouthOpening = pct("mouthOpening");
    p.nasal = pct("nasal");
    p.micPreGain = dbLin("micPreGain");
    // Gate threshold is in dBFS; the gate wants a linear amplitude.
    p.gateThresh = std::pow(10.0f, getF("gateThresh") / 20.0f);
    int gb = (int)getF("gateBehavior");
    p.gateBehavior = (gb == 0) ? tb::GateBehavior::CloseMouth
                   : (gb == 1) ? tb::GateBehavior::HoldVowel
                               : tb::GateBehavior::ReturnToPad;
    p.whisperBoost = getF("whisperBoost");
    p.consonantCrisp = pct("consonantCrisp");
    p.sibilDeess = pct("sibilDeess"); p.voiceBleed = pct("voiceBleed");
    p.micSmooth = pct("micSmooth");
    p.synthWave = (int)getF("synthWave"); p.synthGlide = getF("synthGlide");
    p.synthVibrato = getF("synthVibrato");
    p.arpRate = getF("arpRate");
    p.driverComp = pct("driverComp"); p.diaphragmAsym = getF("diaphragmAsym") * 0.01f;
    p.hornCut = getF("hornCut");
    p.hoseLength = getF("hoseLength"); p.hoseDamping = pct("hoseDamping");
    p.micModel = (int)getF("micModel"); p.micDistance = getF("micDistance");
    p.proxBoost = pct("proxBoost");
    p.dryWet = pct("dryWet"); p.outputGain = dbLin("outputGain");
    p.inputGain = dbLin("inputGain");
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

    // Did any MIDI ever arrive? An effect insert in most DAWs never sends any,
    // so Internal-synth mode would be silent with no explanation. The editor
    // reads this to explain it instead of leaving the user guessing.
    if (!midi.isEmpty()) visGotMidi.store(true);

    // Output RMS for the signal meter (cheap, once per block).
    {
        double acc = 0.0;
        for (int i = 0; i < n; ++i) acc += (double)outL[(size_t)i] * outL[(size_t)i];
        visRms.store((float)std::sqrt(acc / (double)n));
    }

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
