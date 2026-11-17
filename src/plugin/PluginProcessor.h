// TalkBox Reference — JUCE AudioProcessor (VST3/AU/Standalone)
// Wraps dependency-free tb::TalkboxEngine. Zero alloc on audio thread.
// Routing: Sidechain (bus 1 = mic) OR L-R Split (R = mic).
// All DSP in src/dsp/ (headless-testable); this file is thin glue + APVTS.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "../dsp/TbEngine.h"

class TalkBoxReferenceProcessor : public juce::AudioProcessor {
public:
    TalkBoxReferenceProcessor();
    ~TalkBoxReferenceProcessor() override = default;

    const juce::String getName() const override { return "TalkBox Reference"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    tb::TalkboxEngine engine;
    // Lock-free visualizer taps (GUI reads; audio writes)
    std::atomic<float> visF[5];
    std::atomic<float> visMouth{0.8f}, visGate{0.f}, visSib{0.f};

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void syncParams();
    void ensureScratch(int numSamples);

    // Preallocated audio-thread scratch (no allocation in processBlock)
    std::vector<float> micScratch, carrierMono, outL, outR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkBoxReferenceProcessor)
};
