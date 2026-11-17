// TalkBox Reference - Editor.
//
// Layout goals, after user feedback on v1.2.0:
//   * fewer knobs: six macros replace the wall of single-parameter dials, and
//     each macro drives a whole group. Every underlying parameter is still
//     adjustable in the ADVANCED tabs, so nothing is lost.
//   * not stiff: grouped panels with breathing room, no uniform grid.
//   * presets that actually differ: a generator with a dice, not static JSON.
//
// Structure borrowed from the supplied Arturia reference (centred preset
// browser with categories on the left), palette and knob style from Valhalla.
#pragma once
#include "PluginProcessor.h"
#include "../ui/TbLookAndFeel.h"
#include "../ui/TbKnob.h"
#include "../ui/TbMacroKnob.h"
#include "../ui/TbControls.h"
#include "../ui/VowelPad.h"
#include "../ui/TbDice.h"
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
    void switchTab(int newTab);
    void drawHeader(juce::Graphics& g, juce::Rectangle<int> r);
    void drawTabBar(juce::Graphics& g, juce::Rectangle<int> r);

    juce::AudioParameterChoice* choice(const char* id);
    juce::AudioParameterFloat*  flt(const char* id);
    juce::AudioParameterBool*   bll(const char* id);

    tbui::Knob& addKnob(const char* paramId, const juce::String& caption,
                        const juce::String& suffix, int decimals,
                        bool bipolar, int tab);
    tbui::MacroKnob& addMacro(const juce::String& caption, const juce::String& suffix,
                              int decimals,
                              std::vector<tbui::MacroKnob::Target> targets, int tab);

    TalkBoxReferenceProcessor& proc;

    VowelPad pad;
    std::vector<std::unique_ptr<tbui::Knob>> knobs;
    std::vector<int> knobTabs;
    std::vector<std::unique_ptr<tbui::MacroKnob>> macros;
    std::vector<int> macroTabs;
    std::vector<std::unique_ptr<tbui::Segmented>> segments;
    std::vector<std::unique_ptr<tbui::Toggle>> toggles;
    std::vector<std::pair<juce::Component*, int>> tabbed;

    juce::TextButton bypassBtn { "BYPASS" };
    juce::TextButton advBtn { "ADVANCED" };
    juce::TextButton diceBtn { "DICE" };
    juce::TextButton presetPrev { "<" }, presetNext { ">" };
    juce::Label presetName;

    int currentTab = 0;
    int visibleTabCount = 2;
    int diceRoll = 1;

    juce::Rectangle<int> headerArea, tabBarArea, padArea, macroArea, extraArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalkBoxReferenceEditor)
};
