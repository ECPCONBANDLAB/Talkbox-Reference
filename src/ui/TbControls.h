// TalkBox Reference - UI controls: selector (segmented), toggle, dropdown,
// and the formant display. All vector, all DPI independent.
#pragma once
#include "TbLookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

namespace tbui {

// Segmented selector bound to an AudioParameterChoice. Renders every option
// with the active one filled amber, so the current state is readable at a
// glance without opening a popup.
class Segmented : public juce::Component, private juce::Timer {
public:
    Segmented(juce::AudioParameterChoice& param) : p(param)
    {
        startTimerHz(15);
        setInterceptsMouseClicks(true, false);
    }

    void paint(juce::Graphics& g) override
    {
        const int n = p.choices.size();
        if (n <= 0) return;
        auto b = getLocalBounds().toFloat();
        g.setColour(colour::panelRaised);
        g.fillRoundedRectangle(b, 4.0f);

        const float segW = b.getWidth() / (float)n;
        const int active = p.getIndex();
        for (int i = 0; i < n; ++i) {
            auto seg = b.withWidth(segW).withX(b.getX() + segW * (float)i).reduced(2.0f);
            if (i == active) {
                g.setColour(colour::accentDim);
                g.fillRoundedRectangle(seg, 3.0f);
            }
            g.setColour(i == active ? colour::text : colour::textDim);
            g.setFont(labelFont(9.5f));
            g.drawText(p.choices[i].toUpperCase(), seg.toNearestInt(),
                       juce::Justification::centred, false);
        }
        g.setColour(colour::border);
        g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int n = p.choices.size();
        if (n <= 0) return;
        const float segW = (float)getWidth() / (float)n;
        const int i = juce::jlimit(0, n - 1, (int)((float)e.getPosition().x / segW));
        p.setValueNotifyingHost(p.convertTo0to1((float)i));
        repaint();
    }

private:
    void timerCallback() override
    {
        if (p.getIndex() != last) { last = p.getIndex(); repaint(); }
    }
    juce::AudioParameterChoice& p;
    int last = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Segmented)
};

// Simple toggle bound to an AudioParameterBool.
class Toggle : public juce::Component, private juce::Timer {
public:
    Toggle(const juce::String& cap, juce::AudioParameterBool& param, juce::Colour on = colour::accent)
        : caption(cap), p(param), onColour(on)
    {
        startTimerHz(15);
        setInterceptsMouseClicks(true, false);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto led = b.removeFromLeft(b.getHeight()).reduced(3.0f);
        g.setColour(p.get() ? onColour : colour::panelRaised);
        g.fillEllipse(led);
        g.setColour(p.get() ? onColour.brighter(0.4f) : colour::border);
        g.drawEllipse(led, 1.0f);
        g.setColour(p.get() ? colour::text : colour::textDim);
        g.setFont(labelFont(9.5f));
        g.drawText(caption.toUpperCase(), b.toNearestInt(),
                   juce::Justification::centredLeft, false);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        const bool nv = !p.get();
        p.setValueNotifyingHost(nv ? 1.0f : 0.0f);
        repaint();
    }

private:
    void timerCallback() override
    {
        if (p.get() != last) { last = p.get(); repaint(); }
    }
    juce::String caption;
    juce::AudioParameterBool& p;
    juce::Colour onColour;
    bool last = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Toggle)
};

// Flat dropdown with a consistent look, bound to a choice parameter.
class Dropdown : public juce::Component, private juce::Timer {
public:
    Dropdown(juce::AudioParameterChoice& param) : p(param)
    {
        box.addItemList(p.choices, 1);
        box.setSelectedItemIndex(p.getIndex(), juce::dontSendNotification);
        box.onChange = [this] {
            p.setValueNotifyingHost(p.convertTo0to1((float)box.getSelectedItemIndex()));
        };
        addAndMakeVisible(box);
        startTimerHz(10);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(colour::panelRaised);
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(colour::border);
        g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);
    }

    void resized() override { box.setBounds(getLocalBounds().reduced(6, 3)); }

private:
    void timerCallback() override
    {
        if (box.getSelectedItemIndex() != p.getIndex())
            box.setSelectedItemIndex(p.getIndex(), juce::dontSendNotification);
    }
    juce::AudioParameterChoice& p;
    juce::ComboBox box;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dropdown)
};

} // namespace tbui
