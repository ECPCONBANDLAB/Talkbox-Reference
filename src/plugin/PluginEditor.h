// TalkBox Reference — Editor (Simple/Advanced vector GUI)
// GPU-optional, DPI-scalable, near-zero CPU when idle, 0% when closed.
#pragma once
#include "PluginProcessor.h"
#include "../ui/VowelPad.h"

class TalkBoxReferenceEditor : public juce::AudioProcessorEditor,
                               public juce::Timer {
public:
    TalkBoxReferenceProcessor& proc;
    TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
private:
    VowelPad pad;
    juce::ComboBox sourceBox, carrierBox, qualityBox, presetBox;
    juce::TextButton bypassBtn, simpleAdvBtn, micSetupBtn;
    juce::TabbedComponent tabs;
    juce::Component simplePage, tractPage, micPage, hwPage, sysPage;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkBoxReferenceEditor)
};
