// TalkBox Reference — PluginProcessor.h
// JUCE 7+ AudioProcessor shell around tb::TalkboxApi.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <memory>

#include "TalkboxApi.h"

class TalkboxAudioProcessor : public juce::AudioProcessor {
public:
    TalkboxAudioProcessor();
    ~TalkboxAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return tb::TalkboxApi{}.numPresets(); }
    int getCurrentProgram() override { return currentProgram_; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Editor-facing helpers
    void triggerConsonant(tb::Consonant c);
    tb::TalkboxApi& api() { return api_; }
    juce::AudioProcessorValueTreeState& state() { return apvts_; }
    double currentLatencyMs() const { return api_.latencyMs(); }
    double currentHoseMs() const { return api_.hoseDelayMs(); }
    float outputPeak() const { return peak_.load(std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static constexpr int kCarrierInputBus = 0;

private:
    void syncParams();
    static tb::ParamId toParamId(const juce::String& paramId);

    tb::TalkboxApi api_;
    juce::AudioProcessorValueTreeState apvts_;
    juce::AudioBuffer<float> carrier_;
    std::atomic<float> peak_{0.0f};
    int currentProgram_ = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkboxAudioProcessor)
};
