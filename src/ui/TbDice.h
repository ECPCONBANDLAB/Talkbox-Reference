// TalkBox Reference - random patch generator ("dice").
//
// The old factory presets were static JSON the plugin never even read, so they
// all sounded alike. This replaces them with a generator that produces a
// genuinely different, musically valid patch every time it is pressed.
//
// Design rules, from what makes a talkbox usable:
//   * the vowel starts somewhere pronounceable, not at a random corner
//   * motion (glide / arp) stays in the range a mouth and hand can follow
//   * drive and mouth size move together: a heavily driven carrier needs a
//     more open aperture or it just sounds like fizz
//   * extremes are avoided: a random patch must be playable, not an experiment
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <random>

namespace tb {

class Dice {
public:
    static void roll(juce::AudioProcessorValueTreeState& apvts)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        auto uni = [&gen](float lo, float hi) {
            return std::uniform_real_distribution<float>(lo, hi)(gen);
        };

        // Vowel position: pick one of the nine real vowels and land near it
        // with jitter, rather than sampling the pad uniformly. Uniform sampling
        // clusters in the crowded front region and produces eight patches that
        // sound like the same vowel - exactly the complaint about the old
        // static presets. Anchoring on a vowel guarantees each roll is a
        // pronounceable, distinct sound.
        const int vowelIdx = std::uniform_int_distribution<int>(0, 8)(gen);
        auto vowelXY = [vowelIdx](int axis) {
            static const float table[9][2] = {
                { 0.15f, 0.88f },  // i  (beat)
                { 0.30f, 0.62f },  // e  (bet)
                { 0.72f, 0.72f },  // ae (bat)
                { 0.36f, 0.40f },  // schwa
                { 0.28f, 0.22f },  // a  (father)
                { 0.55f, 0.26f },  // open o
                { 0.86f, 0.35f },  // o  (boat)
                { 0.82f, 0.22f },  // uh (book)
                { 0.52f, 0.86f }   // u  (boot)
            };
            return table[vowelIdx][axis];
        };
        // Half the rolls jitter the vowel slightly; half blend towards another
        // vowel, which gives the in-between sounds a talkbox is good at.
        const bool blend = std::uniform_int_distribution<int>(0, 1)(gen) == 1;
        float bx = vowelXY(0), by = vowelXY(1);
        if (blend) {
            static const float table2[9][2] = {
                { 0.15f, 0.88f }, { 0.30f, 0.62f }, { 0.72f, 0.72f }, { 0.36f, 0.40f },
                { 0.28f, 0.22f }, { 0.55f, 0.26f }, { 0.86f, 0.35f }, { 0.82f, 0.22f },
                { 0.52f, 0.86f }
            };
            const int other = std::uniform_int_distribution<int>(0, 8)(gen);
            const float t = uni(0.25f, 0.6f);
            bx = bx * (1.0f - t) + table2[other][0] * t;
            by = by * (1.0f - t) + table2[other][1] * t;
        }
        const float padX = juce::jlimit(0.06f, 0.94f, bx + uni(-0.06f, 0.06f));
        const float padY = juce::jlimit(0.08f, 0.92f, by + uni(-0.06f, 0.06f));
        set(apvts, "padX", padX * 100.0f);
        set(apvts, "padY", padY * 100.0f);

        // Body / jaw follow the pad so the patch is internally consistent.
        set(apvts, "tongueX",      juce::jlimit(0.0f, 100.0f, padX * 100.0f + uni(-12.0f, 12.0f)));
        set(apvts, "tongueHeight", juce::jlimit(0.0f, 100.0f, padY * 100.0f + uni(-12.0f, 12.0f)));
        set(apvts, "jaw",          juce::jlimit(0.0f, 100.0f, padY * 100.0f + uni(-20.0f, 20.0f)));

        // Aperture and rounding: rounding rises as the vowel moves back.
        set(apvts, "lipRounding", juce::jlimit(0.0f, 100.0f, (1.0f - padX) * 55.0f + uni(-15.0f, 15.0f)));
        set(apvts, "mouthOpening", uni(55.0f, 92.0f));

        // Carrier shaping. Drive and mouth size are coupled so the patch stays
        // balanced instead of turning to fizz at high drive.
        const float drive = uni(30.0f, 80.0f);
        set(apvts, "carrierDrive", drive);
        set(apvts, "tubeCharacter", uni(15.0f, 90.0f));
        set(apvts, "mouthSize",     juce::jlimit(0.82f, 1.22f,
            juce::jmap(drive, 30.0f, 80.0f, 0.92f, 1.18f) + uni(-0.06f, 0.06f)));

        // Motion: always some, never so much that the vowel arrives after the
        // note has ended.
        set(apvts, "glide",       uni(8.0f, 45.0f));
        set(apvts, "articSmooth", uni(5.0f, 40.0f));

        // Colour block.
        set(apvts, "driverComp",   uni(10.0f, 55.0f));
        set(apvts, "diaphragmAsym", uni(-25.0f, 25.0f));
        set(apvts, "hornCut",      uni(5000.0f, 8500.0f));
        set(apvts, "proxBoost",    uni(0.0f, 45.0f));
        set(apvts, "hoseLength",   uni(1.20f, 2.10f));
        set(apvts, "hoseDamping",  uni(15.0f, 60.0f));
        set(apvts, "ambience",     uni(0.0f, 35.0f));
        set(apvts, "nasal",        uni(0.0f, 30.0f));

        // Mic side: proximity in a musical range, never muddy.
        set(apvts, "micProximity", uni(20.0f, 70.0f));
        set(apvts, "micDistance",  uni(2.0f, 12.0f));
        set(apvts, "micModel",     (float)std::uniform_int_distribution<int>(0, 2)(gen));

        // Synth side (audible in Internal mode, always kept valid).
        set(apvts, "synthWave",   (float)std::uniform_int_distribution<int>(0, 1)(gen));
        set(apvts, "synthOctave", (float)std::uniform_int_distribution<int>(-1, 1)(gen));
        set(apvts, "synthGlide",  uni(0.02f, 0.20f));
        set(apvts, "synthVibrato", uni(0.0f, 0.45f));
        set(apvts, "arpRate",      uni(4.0f, 14.0f));

        // Output stays sane: no patch should clip on its own.
        set(apvts, "outputGain", uni(-6.0f, 2.0f));
        set(apvts, "dryWet", 100.0f);
        // Never leave the user in Bypass after rolling.
        set(apvts, "bypass", 0.0f);
    }

private:
    static void set(juce::AudioProcessorValueTreeState& apvts, const char* id, float v)
    {
        if (auto* p = apvts.getParameter(id)) {
            const auto& r = p->getNormalisableRange();
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f,
                r.convertTo0to1(juce::jlimit(r.start, r.end, v))));
        }
    }
};

} // namespace tb
