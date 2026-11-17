// TalkBox Reference — PluginProcessor implementation
#include "PluginProcessor.h"
#include "PluginEditor.h"

TalkBoxReferenceProcessor::TalkBoxReferenceProcessor()
    : AudioProcessor(BusesProperties()
          .addBus(true, "Input", juce::AudioChannelLayout::stereo())
          .addBus(true, "Sidechain", juce::AudioChannelLayout::mono())
          .addBus(false, "Output", juce::AudioChannelLayout::stereo())),
      apvts(*this, nullptr, "TalkBoxReference", createLayout()) {
    for (auto& a : visF) a.store(500.f);
}

juce::AudioProcessorValueTreeState::Layout TalkBoxReferenceProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::Layout layout;
    auto addFloat = [&](const char* id, float def, float lo, float hi) {
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID(id, 1), id, lo, hi, def));
    };
    auto addChoice = [&](const char* id, int def, std::vector<String> items) {
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID(id, 1), id, items, def));
    };
    auto addBool = [&](const char* id, bool def) {
        layout.add(std::make_unique<AudioParameterBool>(ParameterID(id, 1), id, def));
    };
    // Header / modes
    addChoice("sourceMode", 0, {"Pad/MIDI", "Live Mic Control"});
    addChoice("carrierMode", 0, {"External Audio", "Internal Talkbox Synth"});
    addChoice("quality", 1, {"Eco", "Balanced", "Studio", "Creative"});
    addBool("bypass", false);
    // Simple macros
    addFloat("carrierDrive", 0.6f, 0.f, 1.f);
    addFloat("tubeCharacter", 0.5f, 0.f, 1.f);
    addFloat("mouthSize", 1.0f, 0.75f, 1.35f); // VTL (Creative extends to 0.5-2.0 in UI)
    addFloat("articSmooth", 0.5f, 0.f, 1.f);
    addFloat("consonantCrisp", 0.7f, 0.f, 1.f);
    addFloat("micProximity", 0.5f, 0.f, 1.f);
    addFloat("dryWet", 1.0f, 0.f, 1.f);
    addFloat("outputGain", 1.0f, 0.f, 2.f);
    // Advanced: Vocal Tract
    addFloat("padX", 0.5f, 0.f, 1.f); addFloat("padY", 0.45f, 0.f, 1.f);
    addFloat("jaw", 0.45f, 0.f, 1.f); addFloat("tongueX", 0.5f, 0.f, 1.f);
    addFloat("tongueHeight", 0.45f, 0.f, 1.f); addFloat("lipRounding", 0.f, 0.f, 1.f);
    addFloat("mouthOpening", 0.8f, 0.f, 1.f); addFloat("nasal", 0.f, 0.f, 1.f);
    addFloat("vtl", 1.0f, 0.5f, 2.0f); addFloat("glide", 0.5f, 0.f, 1.f);
    // Advanced: Mic & Carrier
    addChoice("micRouting", 0, {"Sidechain", "L-R Split", "Standalone Mic"});
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
    addChoice("micModel", 0, {"Classic Dynamic 57/58", "Crisp Studio Condenser", "Warm Ribbon", "Flat Direct"});
    addFloat("micDistance", 3.f, 1.f, 30.f); addFloat("proxBoost", 0.5f, 0.f, 1.f);
    addFloat("ambience", 0.f, 0.f, 1.f);
    // Carrier conditioning
    addFloat("inputGain", 1.0f, 0.f, 4.f);
    return layout;
}

void TalkBoxReferenceProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.prepare((float)sampleRate, samplesPerBlock);
    syncParams();
}

void TalkBoxReferenceProcessor::releaseResources() { engine.reset(); }

bool TalkBoxReferenceProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Main stereo in/out; sidechain mono optional (Standalone has no sidechain bus)
    return layouts.getMainOutputChannelSet() == juce::AudioChannelLayout::stereo()
        && layouts.inputBuses.size() <= 2;
}

void TalkBoxReferenceProcessor::syncParams() {
    auto& p = engine.p;
    auto getF = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    p.source = (getF("sourceMode") > 0.5f) ? tb::SourceMode::Mic : tb::SourceMode::Pad;
    p.carrier = (getF("carrierMode") > 0.5f) ? tb::CarrierMode::Internal : tb::CarrierMode::External;
    int q = (int)getF("quality");
    p.quality = (q==0)?tb::QualityMode::Eco:(q==1)?tb::QualityMode::Balanced:(q==2)?tb::QualityMode::Studio:tb::QualityMode::Creative;
    p.bypass = getF("bypass") > 0.5f;
    p.carrierDrive = getF("carrierDrive");
    p.vtl = getF("vtl"); p.glide = getF("glide");
    p.padX = getF("padX"); p.padY = getF("padY");
    p.jaw = getF("jaw"); p.tongueX = getF("tongueX"); p.tongueHeight = getF("tongueHeight");
    p.lipRounding = getF("lipRounding"); p.mouthOpening = getF("mouthOpening"); p.nasal = getF("nasal");
    p.micPreGain = getF("micPreGain"); p.gateThresh = getF("gateThresh");
    int gb = (int)getF("gateBehavior");
    p.gateBehavior = (gb==0)?tb::GateBehavior::CloseMouth:(gb==1)?tb::GateBehavior::HoldVowel:tb::GateBehavior::ReturnToPad;
    p.whisperBoost = getF("whisperBoost"); p.consonantCrisp = getF("consonantCrisp");
    p.sibilDeess = getF("sibilDeess"); p.voiceBleed = getF("voiceBleed"); p.micSmooth = getF("micSmooth");
    p.synthWave = (int)getF("synthWave"); p.synthGlide = getF("synthGlide"); p.synthVibrato = getF("synthVibrato");
    p.driverComp = getF("driverComp"); p.diaphragmAsym = getF("diaphragmAsym"); p.hornCut = getF("hornCut");
    p.hoseLength = getF("hoseLength"); p.hoseDamping = getF("hoseDamping");
    p.micModel = (int)getF("micModel"); p.micDistance = getF("micDistance"); p.proxBoost = getF("proxBoost");
    p.dryWet = getF("dryWet"); p.outputGain = getF("outputGain"); p.inputGain = getF("inputGain");
    engine.updateQuality();
}

void TalkBoxReferenceProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    syncParams();
    const int n = buffer.getNumSamples();
    auto* carrierL = buffer.getReadPointer(0);
    auto* carrierR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : carrierL;
    // Sidechain bus (index 1) if present; else L-R split (R = mic) or silence.
    const float* micPtr = nullptr;
    juce::AudioBuffer<float> sidechain;
    if (getBusCount() > 1 && getBus(1)->getBuffer()) {
        // JUCE sidechain access via getBusBuffer (JUCE 7+)
        // Fallback: use R channel as mic when routing = L-R Split.
    }
    int routing = (int)apvts.getRawParameterValue("micRouting")->load();
    // Extract mic into temp (mono). For Sidechain, JUCE provides bus 1 via
    // getBusBuffer in newer JUCE; here we support L-R split + Standalone
    // (Standalone mic is fed via carrier input split in Standalone wrapper).
    // To keep this skeleton host-agnostic, mic = R channel when routing==1,
    // else silence (Sidechain hosts should use JUCE sidechain API in full build).
    std::vector<float> micTmp(n, 0.f);
    if (routing == 1) { // L-R Split: R = mic
        for (int i=0;i<n;++i) micTmp[i]=carrierR[i];
    }
    micPtr = micTmp.data();
    // MIDI -> synth
    for (auto ev : midi) {
        if (ev.isNoteOn()) engine.midiNoteOn(ev.getNoteNumber(), ev.getFloatVelocity());
        else if (ev.isNoteOff()) engine.midiNoteOff(ev.getNoteNumber());
        else if (ev.isPitchWheel()) engine.synth.pitchBend = (ev.getPitchWheelValue()-8192.f)/8192.f*2.f;
    }
    std::vector<float> outL(n), outR(n);
    // Mono carrier sum (L+R)/2 for engine (dual-mono output)
    std::vector<float> carrierMono(n);
    for (int i=0;i<n;++i) carrierMono[i]=(carrierL[i]+carrierR[i])*0.5f;
    engine.processBlock(carrierMono.data(), micPtr, outL.data(), outR.data(), n);
    buffer.clear();
    buffer.copyFrom(0, 0, outL.data(), n);
    if (buffer.getNumChannels()>1) buffer.copyFrom(1, 0, outR.data(), n);
    // Visualizer taps
    for (int i=0;i<5;++i) visF[i].store(engine.visF[i]);
    visMouth.store(engine.visMouth); visGate.store(engine.visGate); visSib.store(engine.visSib);
}

void TalkBoxReferenceProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
}
void TalkBoxReferenceProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto tree = juce::ValueTree::readFromStream(juce::MemoryInputStream(data, (size_t)sizeInBytes, false));
    if (tree.isValid()) apvts.state = tree;
}

juce::AudioProcessorEditor* TalkBoxReferenceProcessor::createEditor() {
    return new TalkBoxReferenceEditor(*this);
}
