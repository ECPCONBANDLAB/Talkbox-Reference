// TalkBox Reference - UI look & feel: palette, fonts, shared drawing helpers.
// Vector only (no bitmaps): scales to any DPI and costs nothing when closed.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace tbui {

// Palette: dark warm plum ground, cream text, single amber accent -- the
// Valhalla reading of an instrument panel, with the grouped-section layout
// borrowed from Arturia. One accent colour means "interactive" is unambiguous.
namespace colour {
    inline const juce::Colour ground      { 0xff1c1418 }; // window
    inline const juce::Colour panel       { 0xff241a20 }; // section body
    inline const juce::Colour panelRaised { 0xff2e2129 }; // control well
    inline const juce::Colour border      { 0xff3d2c36 }; // hairlines
    inline const juce::Colour accent      { 0xffe8944a }; // amber: the one accent
    inline const juce::Colour accentDim   { 0xff8a5730 };
    inline const juce::Colour accentHot   { 0xfff7b871 };
    inline const juce::Colour text        { 0xfff2e6dc }; // cream
    inline const juce::Colour textDim     { 0xff9c8a92 };
    inline const juce::Colour textFaint   { 0xff6b5a63 };
    inline const juce::Colour meter       { 0xff7fd4a8 }; // green, signal only
    inline const juce::Colour warn        { 0xffe0574a };
}

// JUCE 7.0.12 API (FontOptions only exists in JUCE 8, which this project
// deliberately does not use: JUCE 8 is AGPLv3-only).
inline juce::Font labelFont(float h)   { return juce::Font(h, juce::Font::bold); }
inline juce::Font valueFont(float h)   { return juce::Font(h, juce::Font::plain); }
inline juce::Font titleFont(float h)   { return juce::Font(h, juce::Font::bold); }
inline juce::Font readoutFont(float h) {
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain);
}

// A section: rounded body, a small caps title on the left, optional rule.
inline void drawSection(juce::Graphics& g, juce::Rectangle<int> r,
                        const juce::String& title, bool highlighted = false)
{
    auto f = r.toFloat();
    g.setColour(colour::panel);
    g.fillRoundedRectangle(f, 6.0f);
    g.setColour(highlighted ? colour::accentDim : colour::border);
    g.drawRoundedRectangle(f.reduced(0.5f), 6.0f, 1.0f);

    if (title.isNotEmpty()) {
        g.setColour(highlighted ? colour::accent : colour::textDim);
        g.setFont(labelFont(10.0f));
        g.drawText(title.toUpperCase(),
                   r.reduced(9, 0).removeFromTop(16),
                   juce::Justification::centredLeft, false);
    }
}

// Small caps caption, used under knobs.
inline void drawCaption(juce::Graphics& g, juce::Rectangle<int> r,
                        const juce::String& s, juce::Colour c = colour::textDim,
                        juce::Justification j = juce::Justification::centred)
{
    g.setColour(c);
    g.setFont(labelFont(9.5f));
    g.drawText(s.toUpperCase(), r, j, false);
}

inline void drawValue(juce::Graphics& g, juce::Rectangle<int> r,
                      const juce::String& s, juce::Colour c = colour::text)
{
    g.setColour(c);
    g.setFont(readoutFont(11.0f));
    g.drawText(s, r, juce::Justification::centred, false);
}

// Amber focus ring used to show "this control is live / automated".
inline void drawLiveRing(juce::Graphics& g, juce::Rectangle<float> r, float amount)
{
    if (amount <= 0.001f) return;
    g.setColour(colour::accent.withAlpha(juce::jlimit(0.0f, 1.0f, amount) * 0.55f));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
}

} // namespace tbui
