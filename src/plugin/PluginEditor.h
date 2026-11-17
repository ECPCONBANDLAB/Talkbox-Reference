// TalkBox Reference - Editor.
//
// Layout follows the Arturia reference (grouped, framed sections with a header
// bar), the controls and palette follow the Valhalla reference (large round
// knobs, value printed under every knob, one amber accent on a dark ground).
//
// Every control is bound to a real APVTS parameter, and every parameter the
// plugin declares is reachable from here. Nothing is decorative.
//
// Cost: vector-only drawing, a single 20 Hz timer for the pad, zero allocation
// per paint. Closing the editor stops all repaints.
#pragma once
#include "PluginProcessor.h"
#include "../ui/TbLookAndFeel.h"
#include "../ui/TbKnob.h"
#include "../ui/TbControls.h"
#include "../ui/VowelPad.h"
#include <array>
#include <memory>
#include <vector>

class TalkBoxReferenceEditor : public juce::AudioProcessorEditor,
                               private juce::Timer {
public:
    explicit TalkBoxReferenceEditor(TalkBoxReferenceProcessor& p);
    ~TalkBoxReferenceEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    void buildControls();
    void layoutTab(int tabIndex, juce::Rectangle<int> area);
    void drawHeader(juce::Graphics& g, juce::Rectangle<int> r);
    void drawTabBar(juce::Graphics& g, juce::Rectangle<int> r);
    void switchTab(int newTab);

    // A knob plus a layout slot, so tabs can be laid out declaratively.
    struct KnobSlot { std::unique_ptr<tbui::Knob> knob; int cx, cy, cw, ch; };

    TalkBoxReferenceProcessor& proc;

    juce::AudioParameterChoice* choice(const char* id);
    juce::AudioParameterFloat*  flt(const char* id);
    juce::AudioParameterBool*   bll(const char* id);

    tbui::Knob& addKnob(const char* paramId, const juce::String& caption,
                        const juce::String& suffix, int decimals,
                        bool bipolar, int tab);

    VowelPad pad;
    std::vector<KnobSlot> knobs;
    std::vector<std::unique_ptr<tbui::Segmented>> segments;
    std::vector<std::unique_ptr<tbui::Toggle>> toggles;
    std::vector<std::unique_ptr<tbui::Dropdown>> drops;
    std::vector<std::pair<juce::Component*, int>> tabbed; // component -> tab index

    juce::TextButton bypassBtn { "BYPASS" }, advBtn { "ADVANCED" };
    int currentTab = 0;
    int visibleTabCount = 2;

    juce::Rectangle<int> headerArea, padArea, rightArea, tabBarArea, bodyArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkBoxReferenceEditor)
};
