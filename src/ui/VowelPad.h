// TalkBox Reference — UI: 2D Vowel & Mouth Visualizer (vector, DPI-scalable)
// Drag in Pad Mode; animates live to tracked vowels in Mic Mode.
// GPU-optional, near-zero CPU (30 Hz timer driven by the editor), 0% when closed.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class TalkBoxReferenceProcessor; // fwd (defined in src/plugin/PluginProcessor.h)

class VowelPad : public juce::Component {
public:
    explicit VowelPad(TalkBoxReferenceProcessor& p) : proc(p) { setOpaque(true); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override { mouseDrag(e); }
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    TalkBoxReferenceProcessor& proc;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VowelPad)
};
