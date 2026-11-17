// TalkBox Reference - UI: vowel-space pad.
// Drag to place the articulation; in Mic mode it follows the tracked vowel.
// Draws the real F1/F2 plane (not an abstract square): the nine anchors are
// positioned from their published formants, so the picture matches the sound.
// Vector only, 30 Hz repaint, 0 % CPU when the editor is closed.
#pragma once
#include "TbLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class TalkBoxReferenceProcessor; // fwd (defined in src/plugin/PluginProcessor.h)

class VowelPad : public juce::Component {
public:
    explicit VowelPad(TalkBoxReferenceProcessor& p) : proc(p) { setOpaque(false); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override { mouseDrag(e); }
    void mouseDrag(const juce::MouseEvent& e) override;

    // Formant display is drawn by the same component so the two never drift.
    void setShowFormants(bool s) { showFormants = s; repaint(); }

private:
    TalkBoxReferenceProcessor& proc;
    bool showFormants = true;
    juce::Rectangle<float> plotArea() const;
    juce::Point<float> toScreen(float f1, float f2) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VowelPad)
};
