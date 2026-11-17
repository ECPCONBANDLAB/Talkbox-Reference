// TalkBox Reference — PluginEditor.h
// 720 x 460 dark UI: source column, tract column, microphone column, meter.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>
#include <vector>

#include "PluginProcessor.h"

class TalkboxAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer {
public:
    explicit TalkboxAudioProcessorEditor(TalkboxAudioProcessor&);
    ~TalkboxAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct ContinuousControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct ChoiceControl {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void timerCallback() override;
    void buildControls();
    void addContinuous(const juce::String& paramId, const juce::String& caption, juce::Rectangle<int> area);
    void addChoice(const juce::String& paramId, const juce::String& caption, juce::Rectangle<int> area);
    void styleSlider(juce::Slider& s);

    TalkboxAudioProcessor& processor_;
    juce::ComboBox presetBox_;
    juce::Label presetLabel_;
    juce::TextButton plosiveButton_{"P"}, fricativeButton_{"S"}, nasalButton_{"N"};
    std::vector<std::unique_ptr<ContinuousControl>> continuous_;
    std::vector<std::unique_ptr<ChoiceControl>> choices_;
    float meter_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkboxAudioProcessorEditor)
};
