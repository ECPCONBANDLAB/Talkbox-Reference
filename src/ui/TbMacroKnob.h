// TalkBox Reference - macro knob: one control, several parameters.
//
// The plugin exposed 44 separate parameters. That is a spec sheet, not an
// instrument: no one wants 44 knobs, and most of them only make sense moved
// together. A MacroKnob drives a weighted set of parameters from one dial, so
// the surface gets smaller while every underlying parameter is still there and
// still reachable individually in the advanced tabs.
//
// The weights are re-based on every drag so that picking up the macro never
// jumps the sound: it moves the group by a delta from wherever the group
// currently sits, which keeps it usable after presets and automation.
#pragma once
#include "TbLookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace tbui {

class MacroKnob : public juce::Component, private juce::Timer {
public:
    struct Target {
        juce::RangedAudioParameter* param = nullptr;
        float weight = 1.0f;   // relative amount this macro moves it
        bool bipolar = false;  // true: 0.5 is neutral, moves up and down
    };

    MacroKnob(const juce::String& caption,
              const juce::String& suffix,
              int decimals,
              std::vector<Target> targets)
        : cap(caption), suf(suffix), dec(decimals), targets(std::move(targets))
    {
        setInterceptsMouseClicks(true, false);
        position = readGroup();     // start where the group already sits
        startTimerHz(15);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float readH = 13.0f, capH = 11.0f;
        auto dialArea = b.withTrimmedBottom(readH + capH + 1.0f);
        const float d = juce::jmin(dialArea.getWidth(), dialArea.getHeight());
        auto dial = juce::Rectangle<float>(d, d).withCentre(dialArea.getCentre());

        const float start = juce::MathConstants<float>::pi * 1.25f;
        const float end   = juce::MathConstants<float>::pi * 2.75f;
        const float ang   = start + position * (end - start);
        const float radius = d * 0.5f - 3.0f;
        const float thick  = juce::jmax(3.0f, d * 0.085f);
        auto ring = dial.reduced(thick * 0.5f + 2.0f);

        juce::Path track;
        track.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f,
                            ring.getHeight() * 0.5f, 0.0f, start, end, true);
        g.setColour(colour::panelRaised);
        g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // A macro shows a brighter, wider arc than a plain knob: visually it is
        // "this moves a whole group", not a single parameter.
        if (std::abs(ang - start) > 0.02f) {
            juce::Path arc;
            arc.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f,
                              ring.getHeight() * 0.5f, 0.0f, start, ang, true);
            g.setColour(isMouseOverOrDragging() ? colour::accentHot : colour::accent);
            g.strokePath(arc, juce::PathStrokeType(thick * 1.35f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }

        auto body = dial.reduced(d * 0.20f);
        g.setGradientFill(juce::ColourGradient(colour::panelRaised.brighter(0.12f),
                                               body.getTopLeft(),
                                               colour::panel.darker(0.25f),
                                               body.getBottomRight(), false));
        g.fillEllipse(body);
        g.setColour(colour::border);
        g.drawEllipse(body, 1.0f);

        // Marks around the body: one tick per driven parameter, so the user can
        // see at a glance that this macro is not a single control.
        const int n = (int)targets.size();
        for (int i = 0; i < n; ++i) {
            const float a = juce::MathConstants<float>::twoPi * (float)i / (float)juce::jmax(1, n);
            const float rr = body.getWidth() * 0.34f;
            g.setColour(colour::textFaint.withAlpha(0.7f));
            g.fillEllipse(body.getCentreX() + std::sin(a) * rr - 1.0f,
                          body.getCentreY() - std::cos(a) * rr - 1.0f, 2.0f, 2.0f);
        }

        const float pr = body.getWidth() * 0.5f;
        juce::Path ptr;
        ptr.startNewSubPath(body.getCentreX() + std::sin(ang) * pr * 0.30f,
                            body.getCentreY() - std::cos(ang) * pr * 0.30f);
        ptr.lineTo(body.getCentreX() + std::sin(ang) * pr * 0.72f,
                   body.getCentreY() - std::cos(ang) * pr * 0.72f);
        g.setColour(colour::text);
        g.strokePath(ptr, juce::PathStrokeType(juce::jmax(1.6f, d * 0.035f),
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

        drawValue(g, b.removeFromBottom(readH).toNearestInt(),
                  juce::String(position * 100.0f, dec) + suf);
        drawCaption(g, b.removeFromBottom(capH).toNearestInt(), cap);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStart = position;
        dragY = e.getPosition().y;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const float sensitivity = e.mods.isShiftDown() ? 0.0016f : 0.0060f;
        const float delta = (float)(dragY - e.getPosition().y) * sensitivity;
        apply(juce::jlimit(0.0f, 1.0f, dragStart + delta));
    }

    void mouseDoubleClick(const juce::MouseEvent&) override { apply(0.5f); }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        apply(juce::jlimit(0.0f, 1.0f, position + w.deltaY * 0.5f));
    }

private:
    // Current group position, derived from the parameters themselves so the
    // macro follows automation and preset loads.
    float readGroup() const
    {
        float sum = 0.0f, wsum = 0.0f;
        for (const auto& t : targets) {
            if (!t.param) continue;
            sum += t.param->getValue() * t.weight;
            wsum += t.weight;
        }
        return wsum > 0.0f ? juce::jlimit(0.0f, 1.0f, sum / wsum) : 0.5f;
    }

    void apply(float newPos)
    {
        newPos = juce::jlimit(0.0f, 1.0f, newPos);
        // Move each target by the macro's delta, scaled by its weight. Clamping
        // happens per parameter, so a group member that hits its end does not
        // stop the rest of the group.
        for (const auto& t : targets) {
            if (!t.param) continue;
            const float cur = t.param->getValue();
            float want = cur + (newPos - position) * t.weight;
            if (t.bipolar) {
                // Bipolar members travel at half rate around their centre.
                want = cur + (newPos - position) * t.weight * 0.5f;
            }
            t.param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, want));
        }
        position = newPos;
        repaint();
    }

    void timerCallback() override
    {
        if (isMouseButtonDown()) return;   // never fight the user's drag
        const float now = readGroup();
        if (std::abs(now - position) > 0.001f) { position = now; repaint(); }
    }

    juce::String cap, suf;
    int dec;
    std::vector<Target> targets;
    float position = 0.5f;
    float dragStart = 0.5f;
    int dragY = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroKnob)
};

} // namespace tbui
