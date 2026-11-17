// TalkBox Reference — UI: 2D Vowel & Mouth Visualizer (vector, DPI-scalable)
// Drag in Pad Mode; animates live to tracked vowels/mouth in Mic Mode.
// GPU-optional, near-zero CPU (30 Hz timer), 0% when editor closed.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class TalkBoxReferenceProcessor; // fwd (defined in src/plugin/PluginProcessor.h)

class VowelPad : public juce::Component {
public:
    TalkBoxReferenceProcessor& proc;
    VowelPad(TalkBoxReferenceProcessor& p) : proc(p) {}
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseInput& mi) override;
    void mouseDrag(const juce::MouseInput& mi) override;
    void timerCallback() override { repaint(); }
};
