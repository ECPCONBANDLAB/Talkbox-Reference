// TalkBox Reference — PluginProcessor.cpp
#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace {

juce::StringArray vowelChoices() {
    return {"i", "I", "e", "E", "ae", "a", "open o", "o", "u"};
}

juce::StringArray carrierChoices() { return {"External", "Saw", "Square", "Pulse", "Noise"}; }

juce::StringArray micChoices() { return {"Dynamic", "Small diaphragm", "Condenser", "Ribbon"}; }

}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout TalkboxAudioProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&layout](const char* id, const char* name, NormalisableRange<float> range, float def,
                         StringArray choices = {}) {
        if (choices.isEmpty())
            layout.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name, range, def));
        else
            layout.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, choices,
                                                              static_cast<int>(def)));
    };

    add(tb::paramName(tb::ParamId::Vowel), "Vowel", {0.0f, 8.0f, 1.0f}, 5.0f, vowelChoices());
    add(tb::paramName(tb::ParamId::Vtl), "Vocal tract (VTL)", {0.70f, 1.45f, 0.001f}, 1.0f);
    add(tb::paramName(tb::ParamId::FormantShift), "Formant shift", {-12.0f, 12.0f, 0.01f}, 0.0f);
    add(tb::paramName(tb::ParamId::Nasal), "Nasal coupling", {0.0f, 1.0f, 0.001f}, 0.12f);
    add(tb::paramName(tb::ParamId::LipAperture), "Lip aperture", {0.0f, 1.0f, 0.001f}, 1.0f);
    add(tb::paramName(tb::ParamId::HoseLength), "Hose length", {1.5f, 2.0f, 0.001f}, 1.75f);
    add(tb::paramName(tb::ParamId::HoseFeedback), "Hose feedback", {0.0f, 0.995f, 0.001f}, 0.88f);
    add(tb::paramName(tb::ParamId::Drive), "Driver", {-24.0f, 36.0f, 0.01f}, 6.0f);
    add(tb::paramName(tb::ParamId::Oversample), "Oversampling", {1.0f, 4.0f, 1.0f}, 2.0f);
    add(tb::paramName(tb::ParamId::CarrierMode), "Carrier", {0.0f, 4.0f, 1.0f}, 0.0f, carrierChoices());
    add(tb::paramName(tb::ParamId::CarrierFreq), "Carrier pitch", {40.0f, 880.0f, 0.01f}, 110.0f);
    add(tb::paramName(tb::ParamId::CarrierNoise), "Carrier noise", {0.0f, 1.0f, 0.001f}, 0.0f);
    add(tb::paramName(tb::ParamId::MicModel), "Microphone", {0.0f, 3.0f, 1.0f}, 0.0f, micChoices());
    add(tb::paramName(tb::ParamId::MicDistance), "Mic distance", {0.02f, 1.5f, 0.001f}, 0.15f);
    add(tb::paramName(tb::ParamId::Saturation), "Saturation", {0.0f, 1.0f, 0.001f}, 0.18f);
    add(tb::paramName(tb::ParamId::Output), "Output", {-24.0f, 12.0f, 0.01f}, -3.0f);
    add(tb::paramName(tb::ParamId::Ceiling), "Ceiling", {-12.0f, 0.0f, 0.01f}, -0.3f);
    return layout;
}

TalkboxAudioProcessor::TalkboxAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Carrier", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "TALKBOX", createLayout()) {}

void TalkboxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    api_.prepare(sampleRate, juce::jmax(32, samplesPerBlock));
    carrier_.setSize(1, juce::jmax(32, samplesPerBlock));
    currentProgram_ = 2;
    api_.loadPreset(currentProgram_);
    setLatencySamples(api_.latencySamples());
}

bool TalkboxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    const bool inputOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    return inputOk && out == juce::AudioChannelSet::stereo();
}

void TalkboxAudioProcessor::syncParams() {
    for (int i = 0; i < static_cast<int>(tb::ParamId::Count); ++i) {
        const auto id = static_cast<tb::ParamId>(i);
        if (auto* p = apvts_.getParameter(tb::paramName(id))) api_.setParam(id, p->getValue());
    }
}

void TalkboxAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    if (n <= 0) return;

    syncParams();

    const int numIn = getTotalNumInputChannels();
    const auto* inL = numIn > 0 ? buffer.getReadPointer(0) : nullptr;
    const auto* inR = numIn > 1 ? buffer.getReadPointer(1) : nullptr;

    if (carrier_.getNumSamples() < n) carrier_.setSize(1, n, false, false, true);
    auto* carrier = carrier_.getWritePointer(0);
    for (int i = 0; i < n; ++i) {
        const float l = inL ? inL[i] : 0.0f;
        const float r = inR ? inR[i] : l;
        carrier[i] = 0.5f * (l + r);
    }

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;
    api_.process(carrier, outL, outR, n);

    float peak = 0.0f;
    for (int i = 0; i < n; ++i) peak = juce::jmax(peak, std::abs(outL[i]));
    peak_.store(peak, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* TalkboxAudioProcessor::createEditor() {
    return new TalkboxAudioProcessorEditor(*this);
}

void TalkboxAudioProcessor::setCurrentProgram(int index) {
    if (index < 0 || index >= api_.numPresets()) return;
    currentProgram_ = index;
    api_.loadPreset(index);
    for (int i = 0; i < static_cast<int>(tb::ParamId::Count); ++i) {
        const auto id = static_cast<tb::ParamId>(i);
        if (auto* p = apvts_.getParameter(tb::paramName(id)))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(api_.getParam(id))));
    }
}

const juce::String TalkboxAudioProcessor::getProgramName(int index) {
    return api_.presetName(index);
}

void TalkboxAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto xml = std::make_unique<juce::XmlElement>("TalkBoxState");
    xml->setAttribute("version", "1.0.0");
    xml->setAttribute("program", currentProgram_);
    xml->setAttribute("params", juce::String(api_.saveState()));
    copyXmlToBinary(*xml, destData);
}

void TalkboxAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml || !xml->hasTagName("TalkBoxState")) return;
    currentProgram_ = xml->getIntAttribute("program", 2);
    api_.restoreState(xml->getStringAttribute("params").toStdString());
    for (int i = 0; i < static_cast<int>(tb::ParamId::Count); ++i) {
        const auto id = static_cast<tb::ParamId>(i);
        if (auto* p = apvts_.getParameter(tb::paramName(id)))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(api_.getParam(id))));
    }
}

void TalkboxAudioProcessor::triggerConsonant(tb::Consonant c) { api_.trigger(c, 1.0); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TalkboxAudioProcessor(); }
