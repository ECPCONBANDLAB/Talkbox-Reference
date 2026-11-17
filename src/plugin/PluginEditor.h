// TalkBox Reference — Editor (Simple/Advanced vector GUI)
// GPU-optional, DPI-scalable, near-zero CPU when idle, 0% when closed.
#pragma once
#include "PluginProcessor.h"
#include "../ui/VowelPad.h"

class TalkBoxReferenceEditor : public juce::AudioProcessorEditor,
                               private juce::Timer {
public:
    explicit TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p);
    ~TalkBoxReferenceEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    TalkBoxReferenceProcessor& proc;
    VowelPad pad;
    juce::ComboBox sourceBox, carrierBox, qualityBox, presetBox;
    juce::TextButton bypassBtn, simpleAdvBtn, micSetupBtn;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Component simplePage, tractPage, micPage, hwPage, sysPage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkBoxReferenceEditor)
};
