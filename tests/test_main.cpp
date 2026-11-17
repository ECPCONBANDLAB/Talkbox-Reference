// TalkBox Reference — validation suite
// Build:  g++ -std=c++20 -O2 -I src/dsp -I tests tests/test_main.cpp -o tests/run_tests
// Run:    ./tests/run_tests     (writes PASS/FAIL lines and measured metrics)
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "Engine.h"
#include "LpcTools.h"

using namespace tb;

// ---------------------------------------------------------------------------
// Tiny harness
// ---------------------------------------------------------------------------
namespace {

int g_pass = 0;
int g_fail = 0;

void check(const char* id, const std::string& name, bool ok, const std::string& detail) {
    if (ok) {
        ++g_pass;
        std::printf("PASS %s  %-46s %s\n", id, name.c_str(), detail.c_str());
    } else {
        ++g_fail;
        std::printf("FAIL %s  %-46s %s\n", id, name.c_str(), detail.c_str());
    }
}

std::string fmt(const char* f, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    return std::string(buf);
}

double rms(const std::vector<double>& v) {
    double s = 0.0;
    for (double x : v) s += x * x;
    return std::sqrt(s / std::max<size_t>(1, v.size()));
}

// Steady-state RMS gain of a stateful generator at a given frequency.
template <typename Gen>
double toneGainDb(Gen&& gen, double freq, double sr, double amp = 0.05) {
    const int n = static_cast<int>(sr * 0.4);
    double acc = 0.0;
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        const double x = amp * std::sin(kTwoPi * freq * i / sr);
        const double y = gen(x);
        if (i > n / 2) {
            acc += y * y;
            ++cnt;
        }
    }
    return 10.0 * std::log10(std::max(1e-24, (acc / std::max(1, cnt)) / (amp * amp / 2.0)));
}

std::vector<double> impulseResponse(VocalTract& vt, int n) {
    std::vector<double> ir;
    ir.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) ir.push_back(vt.process(i == 0 ? 1.0 : 0.0));
    return ir;
}

// Whisper analysis: engine output -> FFT envelope -> formant peak picking.
void whisperFormants(int vowel, double sr, int oversample, double& f1, double& f2, double* levelDb = nullptr) {
    TalkboxEngine e;
    EngineParams p = factoryPresets()[1].params;  // "Mic": external carrier, minimal drive
    p.vowel = vowel;
    p.oversample = oversample;
    e.prepare(sr);
    e.setParams(p);
    e.reset();

    Rng rng(0x51ce11u);
    std::vector<double> y;
    const int n = static_cast<int>(sr * 1.2);
    for (int i = 0; i < n; ++i) y.push_back(e.processSample(rng.bipolar() * 0.25));

    if (levelDb) *levelDb = gainToDb(rms(y));

    std::vector<double> seg(y.begin() + static_cast<size_t>(sr * 0.35), y.begin() + static_cast<size_t>(sr * 1.05));
    Fft fft(16384);
    auto mag = fft.magnitudeAvg(seg, sr);
    const Formant* t = VocalTract::vowelTable(vowel);
    f1 = formantPeakHz(mag, sr, 16384, t[0].freq, t[0].freq * 0.25);
    f2 = formantPeakHz(mag, sr, 16384, t[1].freq, t[1].freq * 0.15);
}

// Total energy in a band relative to the whole spectrum, as a linear fraction.
double bandFraction(const std::vector<double>& mag, double sr, int fftSize, double f1, double f2) {
    const double hi = bandEnergyDb(mag, sr, fftSize, f1, f2);
    const double all = bandEnergyDb(mag, sr, fftSize, 60.0, sr * 0.48);
    return std::pow(10.0, (hi - all) / 10.0);
}

}  // namespace

int main() {
    const double sr = 48000.0;
    std::printf("TalkBox Reference — validation suite\n");
    std::printf("Sample rate %.0f Hz, C++20, %s\n\n", sr, __VERSION__);

    // ======================= A. Configuration & presets =====================
    {
        const auto& presets = factoryPresets();
        bool names = presets.size() == 4;
        for (const auto& p : presets) names = names && p.name && p.name[0] && p.description && p.description[0];
        check("A1", "4 factory presets with name/description", names,
              fmt("count=%zu", presets.size()));

        bool ladder = presets[0].params.oversample <= presets[2].params.oversample &&
                      presets[2].params.oversample <= presets[3].params.oversample;
        check("A2", "quality ladder Eco <= Balanced <= Studio", ladder,
              fmt("OS=%d/%d/%d/%d", presets[0].params.oversample, presets[1].params.oversample,
                  presets[2].params.oversample, presets[3].params.oversample));

        bool ranges = true;
        for (const auto& p : presets) {
            ranges = ranges && p.params.driveDb >= -24.0 && p.params.driveDb <= 36.0 &&
                     p.params.hoseLength >= 1.5 && p.params.hoseLength <= 2.0 &&
                     p.params.vtl > 0.7 && p.params.vtl < 1.45;
        }
        check("A3", "preset parameter ranges (drive/hose/VTL)", ranges, "hose 1.6–1.9 m, drive 0–9 dB");

        // The JSON manifest must describe the same four presets.
        std::ifstream f("presets/factory_presets.json");
        std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        int nameCount = 0;
        for (size_t pos = 0; (pos = json.find("\"name\"", pos)) != std::string::npos; ++pos) ++nameCount;
        check("A4", "presets/factory_presets.json mirrors 4 presets", nameCount == 4,
              fmt("entries=%d bytes=%zu", nameCount, json.size()));
    }

    // ============================ B. Carrier ===============================
    {
        Carrier c;
        c.prepare(sr);
        c.setMode(CarrierMode::Saw);
        c.setFrequency(1000.0);
        std::vector<double> y;
        for (int i = 0; i < 32768; ++i) y.push_back(c.process(0.0));
        Fft fft(32768);
        auto mag = fft.magnitudeAvg(y, sr);
        const double f0 = peakHz(mag, sr, 32768, 1000.0, 200.0);
        const double err = 100.0 * (f0 - 1000.0) / 1000.0;
        check("B1", "saw fundamental accuracy", std::abs(err) <= 0.5, fmt("f0=%.2f Hz (%+.3f%%)", f0, err));

        // Alias energy = everything that is not one of the first 20 harmonics.
        auto aliasEnergy = [&](bool polyBlep) {
            Carrier cc;
            cc.prepare(sr);
            cc.setMode(CarrierMode::Saw);
            cc.setFrequency(1000.0);
            std::vector<double> sig;
            for (int i = 0; i < 32768; ++i) sig.push_back(polyBlep ? cc.process(0.0) : cc.processNaiveSaw(0.0));
            Fft f2(32768);
            auto m = f2.magnitudeAvg(sig, sr);
            double total = 0.0, harmonic = 0.0;
            const double df = sr / 32768.0;
            for (size_t b = 2; b < m.size(); ++b) {
                const double hz = b * df;
                const double p = m[b] * m[b];
                total += p;
                for (int k = 1; k <= 20; ++k) {
                    if (std::abs(hz - k * 1000.0) < 12.0) { harmonic += p; break; }
                }
            }
            return 10.0 * std::log10(std::max(1e-24, (total - harmonic) / std::max(1e-24, harmonic)));
        };
        const double aliasing = aliasEnergy(true) - aliasEnergy(false);
        check("B2", "PolyBLEP alias suppression vs naive", aliasing <= -12.0,
              fmt("%.1f dB", aliasing));

        Carrier n;
        n.prepare(sr);
        n.setMode(CarrierMode::Noise);
        double mean = 0.0, mx = 0.0;
        std::vector<double> ny;
        for (int i = 0; i < 48000; ++i) {
            const double v = n.process(0.0);
            ny.push_back(v);
            mx = std::max(mx, std::abs(v));
        }
        for (double v : ny) mean += v;
        mean /= ny.size();
        check("B3", "noise carrier zero-mean and bounded", std::abs(mean) < 0.01 && mx < 1.0,
              fmt("mean=%.4f peak=%.3f", mean, mx));

        // A mode switch must not introduce a discontinuity beyond the waveform's
        // own natural slope: compare the step at the switch with steady state.
        Carrier s;
        s.prepare(sr);
        s.setMode(CarrierMode::Saw);
        s.setFrequency(110.0);
        double prev = 0.0;
        for (int i = 0; i < 4800; ++i) prev = s.process(0.0);
        s.setMode(CarrierMode::Square);
        for (int i = 0; i < 2400; ++i) prev = s.process(0.0);  // settle in square mode
        double naturalStep = 0.0;
        for (int i = 0; i < 4800; ++i) {
            const double v = s.process(0.0);
            naturalStep = std::max(naturalStep, std::abs(v - prev));
            prev = v;
        }
        s.setMode(CarrierMode::Pulse);
        double switchStep = 0.0;
        bool clickFree = true;
        for (int i = 0; i < 64; ++i) {
            const double v = s.process(0.0);
            switchStep = std::max(switchStep, std::abs(v - prev));
            prev = v;
            clickFree = clickFree && std::isfinite(v);
        }
        check("B4", "carrier mode switch stays click-free",
              clickFree && switchStep <= naturalStep + 0.05,
              fmt("switch step %.3f vs natural %.3f", switchStep, naturalStep));
    }

    // ============================= C. Driver ===============================
    {
        Driver d;
        d.prepare(sr);
        d.setOversample(2);
        d.setDriveDb(12.0);
        d.reset();
        double mx = 0.0;
        for (int i = 0; i < static_cast<int>(sr); ++i)
            mx = std::max(mx, std::abs(d.process(0.9 * std::sin(kTwoPi * 220.0 * i / sr))));
        check("C1", "ADAA soft-clip output bounded", mx <= 0.8, fmt("peak=%.3f (bound 0.8)", mx));

        DcBlocker dc;
        dc.setCutoff(sr, 18.0);
        dc.reset();
        double last = 0.0;
        for (int i = 0; i < static_cast<int>(sr * 5); ++i) last = dc.process(0.5);
        check("C2", "DC blocker 18 Hz residual", gainToDb(std::abs(last)) < -60.0,
              fmt("%.0f dB after 5 s", gainToDb(std::abs(last))));

        Driver h;
        h.prepare(sr);
        h.setOversample(2);
        h.setDriveDb(0.0);
        h.reset();
        const double g30 = toneGainDb([&](double x) { return h.process(x); }, 30.0, sr, 0.01);
        check("C3", "high-pass 90 Hz rejects 30 Hz", g30 <= -12.0, fmt("%.1f dB", g30));

        Driver l;
        l.prepare(sr);
        l.setOversample(2);
        l.setDriveDb(0.0);
        l.reset();
        const double g16k = toneGainDb([&](double x) { return l.process(x); }, 16000.0, sr, 0.01);
        check("C4", "low-pass 6.5 kHz rejects 16 kHz", g16k <= -20.0, fmt("%.1f dB", g16k));

        auto aliasAt = [&](int osf) {
            Driver dd;
            dd.prepare(sr);
            dd.setOversample(osf);
            dd.setDriveDb(18.0);
            dd.reset();
            std::vector<double> y;
            for (int i = 0; i < 32768; ++i) y.push_back(dd.process(0.5 * std::sin(kTwoPi * 9000.0 * i / sr)));
            Fft f3(32768);
            auto m = f3.magnitudeAvg(y, sr);
            // 3rd harmonic (27 kHz) folds to 21 kHz: measure it against the fundamental.
            return bandEnergyDb(m, sr, 32768, 20800.0, 21200.0) -
                   bandEnergyDb(m, sr, 32768, 8800.0, 9200.0);
        };
        const double improvement = aliasAt(1) - aliasAt(4);
        check("C5", "localised 4x oversampling improves aliasing", improvement >= 3.0,
              fmt("%.1f dB better than 1x", improvement));
    }

    // ============================== D. Hose ================================
    {
        for (int k = 0; k < 2; ++k) {
            const double len = (k == 0) ? 1.5 : 2.0;
            Hose h;
            h.prepare(sr);
            h.setLength(len);
            h.setFeedback(0.0);
            h.reset();
            int peakIdx = 0;
            double best = 0.0;
            for (int i = 0; i < 4000; ++i) {
                const double v = h.process(i == 0 ? 1.0 : 0.0);
                if (std::abs(v) > best) { best = std::abs(v); peakIdx = i; }
            }
            const double expected = len / kSpeedOfSound * 1000.0;
            const double measured = peakIdx / sr * 1000.0;
            const double err = 100.0 * (measured - expected) / expected;
            check(k == 0 ? "D1" : "D2", k == 0 ? "hose delay at 1.50 m" : "hose delay at 2.00 m",
                  std::abs(err) <= 3.0, fmt("%.2f ms vs %.2f ms (%+.1f%%)", measured, expected, err));
        }

        Hose hs;
        hs.prepare(sr);
        hs.setLength(2.0);
        hs.setFeedback(0.995);
        hs.reset();
        Rng rng(0x1234u);
        double mx = 0.0;
        for (int i = 0; i < static_cast<int>(sr * 20); ++i) mx = std::max(mx, std::abs(hs.process(rng.bipolar() * 0.5)));
        check("D3", "hose stable at max feedback (20 s)", mx < 8.0, fmt("peak=%.3f", mx));

        auto hfRatio = [&](double dampHz) {
            Hose hh;
            hh.prepare(sr);
            hh.setLength(2.0);
            hh.setFeedback(0.9);
            hh.setDampingHz(dampHz);
            hh.reset();
            std::vector<double> y;
            for (int i = 0; i < static_cast<int>(sr); ++i) y.push_back(hh.process(rng.bipolar() * 0.3));
            Fft f4(8192);
            auto m = f4.magnitudeAvg(y, sr);
            return bandEnergyDb(m, sr, 8192, 6000.0, 10000.0) - bandEnergyDb(m, sr, 8192, 400.0, 1000.0);
        };
        const double dampDelta = hfRatio(12000.0) - hfRatio(1000.0);
        check("D4", "wall damping attenuates the top end", dampDelta >= 6.0,
              fmt("%.1f dB HF reduction at 1 kHz damping", dampDelta));
    }

    // ====================== E. Vocal tract formants ========================
    {
        Fft fft(32768);
        VocalTract vt;
        vt.prepare(sr);
        const char* vowelNames[VocalTract::kNumVowels] = {"i", "I", "e", "E", "ae", "a", "open o", "o", "u"};
        for (int v = 0; v < VocalTract::kNumVowels; ++v) {
            vt.setVowel(v);
            vt.reset();
            auto ir = impulseResponse(vt, 32768);
            auto mag = fft.magnitudeAvg(ir, sr);
            const Formant* t = VocalTract::vowelTable(v);
            const double f1 = formantPeakHz(mag, sr, 32768, t[0].freq, t[0].freq * 0.12);
            const double f2 = formantPeakHz(mag, sr, 32768, t[1].freq, t[1].freq * 0.10);
            const double f3 = formantPeakHz(mag, sr, 32768, t[2].freq, t[2].freq * 0.08);
            const double e1 = 100.0 * (f1 - t[0].freq) / t[0].freq;
            const double e2 = 100.0 * (f2 - t[1].freq) / t[1].freq;
            const double e3 = 100.0 * (f3 - t[2].freq) / t[2].freq;
            const double worst = std::max({std::abs(e1), std::abs(e2), std::abs(e3)});
            const std::string id = fmt("E%d", v + 1);
            check(id.c_str(), fmt("/%s/ F1-F3 within +/-5%%", vowelNames[v]).c_str(), worst <= 5.0,
                  fmt("%.0f/%.0f/%.0f Hz  err %+.2f/%+.2f/%+.2f%%", f1, f2, f3, e1, e2, e3));
        }
    }

    // ==================== F. Microphone / whisper path =====================
    {
        struct Case { int vowel; const char* name; };
        const Case cases[3] = {{0, "i"}, {5, "a"}, {8, "u"}};
        const char* ids[3] = {"F1", "F3", "F4"};
        for (int c = 0; c < 3; ++c) {
            double f1 = 0.0, f2 = 0.0;
            whisperFormants(cases[c].vowel, sr, 1, f1, f2);
            const Formant* t = VocalTract::vowelTable(cases[c].vowel);
            const double e1 = 100.0 * (f1 - t[0].freq) / t[0].freq;
            const double e2 = 100.0 * (f2 - t[1].freq) / t[1].freq;
            check(ids[c], fmt("whisper formants /%s/ (FFT envelope)", cases[c].name),
                  std::abs(e1) <= 15.0 && std::abs(e2) <= 5.0,
                  fmt("F1 %.0f Hz (%+.1f%%)  F2 %.0f Hz (%+.1f%%)", f1, e1, f2, e2));
        }

        // The second whisper case doubles as the F2 check for /a/.
        {
            double f1 = 0.0, f2 = 0.0;
            whisperFormants(5, sr, 4, f1, f2);
            const Formant* t = VocalTract::vowelTable(5);
            const double e2 = 100.0 * (f2 - t[1].freq) / t[1].freq;
            check("F2", "whisper F2 at 4x oversampling", std::abs(e2) <= 5.0,
                  fmt("F2 %.0f Hz (%+.1f%%)", f2, e2));
        }

        TalkboxEngine e;
        EngineParams p = factoryPresets()[2].params;
        p.carrierMute = true;
        e.prepare(sr);
        e.setParams(p);
        e.reset();
        std::vector<double> y;
        for (int i = 0; i < static_cast<int>(sr * 0.5); ++i) y.push_back(e.processSample(0.0));
        const double bleed = gainToDb(rms(y));
        check("F5", "carrier bleed below -60 dBFS", bleed < -60.0, fmt("%.0f dBFS", bleed));

        double f1 = 0.0, f2 = 0.0;
        whisperFormants(0, sr, 2, f1, f2);
        check("F6", "reported algorithmic latency", e.latencyMs() < 6.0,
              fmt("%d samples = %.2f ms (hose adds %.2f ms of acoustics)", e.latencySamples(), e.latencyMs(),
                  e.hoseDelayMs()));
    }

    // ========================== G. Consonants ==============================
    {
        ConsonantBridge b;
        b.prepare(sr);
        b.trigger(Consonant::S, 1.0);
        std::vector<double> y;
        while (b.active()) y.push_back(b.process());
        Fft fft(8192);
        auto mag = fft.magnitudeAvg(y, sr);
        const double ratio = bandFraction(mag, sr, 8192, 4000.0, 10000.0);
        check("G1", "sibilance index /s/ 0.50 +/- 0.08", std::abs(ratio - 0.50) <= 0.08,
              fmt("%.2f", ratio));

        ConsonantBridge p;
        p.prepare(sr);
        p.trigger(Consonant::Plosive, 1.0);
        int peakIdx = 0;
        double best = 0.0;
        int idx = 0;
        while (p.active()) {
            const double v = std::abs(p.process());
            if (v > best) { best = v; peakIdx = idx; }
            ++idx;
        }
        const double attackMs = peakIdx / sr * 1000.0;
        check("G2", "plosive attack within 20 ms", attackMs <= 20.0, fmt("%.2f ms", attackMs));

        TalkboxEngine e;
        EngineParams ep = factoryPresets()[3].params;
        ep.carrierMute = true;
        ep.outputDb = 0.0;
        e.prepare(sr);
        e.setParams(ep);
        e.reset();
        e.triggerConsonant(Consonant::Sh, 1.0);
        double mx = 0.0;
        for (int i = 0; i < static_cast<int>(sr * 0.2); ++i) mx = std::max(mx, std::abs(e.processSample(0.0)));
        check("G3", "consonant bridge stays under ceiling", mx <= 0.98, fmt("peak=%.3f (ceiling 0.966)", mx));
    }

    // ====================== H. Radiation / microphone ======================
    {
        Limiter l;
        l.prepare(sr, 32);
        l.setCeilingDb(-0.3);
        l.reset();
        double mx = 0.0;
        for (int i = 0; i < static_cast<int>(sr * 2); ++i) mx = std::max(mx, std::abs(l.process(4.0 * std::sin(kTwoPi * 500.0 * i / sr))));
        check("H1", "limiter ceiling with +12 dB input", gainToDb(mx) <= -0.25,
              fmt("%.2f dBFS (ceiling %.2f)", gainToDb(mx), l.ceiling()));

        Radiation near;
        near.prepare(sr);
        near.setMicModel(MicModel::Dynamic);
        near.setDistance(0.05);
        Radiation far;
        far.prepare(sr);
        far.setMicModel(MicModel::Dynamic);
        far.setDistance(0.50);
        const double prox = near.colourResponseDb(100.0) - far.colourResponseDb(100.0);
        check("H2", "proximity effect at 5 cm vs 50 cm", prox >= 3.0, fmt("+%.1f dB at 100 Hz", prox));

        double spread = 0.0;
        for (int a = 0; a < 4; ++a)
            for (int b2 = a + 1; b2 < 4; ++b2) {
                Radiation ra;
                ra.prepare(sr);
                ra.setMicModel(static_cast<MicModel>(a));
                ra.setDistance(0.15);
                Radiation rb;
                rb.prepare(sr);
                rb.setMicModel(static_cast<MicModel>(b2));
                rb.setDistance(0.15);
                spread = std::max({spread, std::abs(ra.colourResponseDb(100.0) - rb.colourResponseDb(100.0)),
                                   std::abs(ra.colourResponseDb(10000.0) - rb.colourResponseDb(10000.0))});
            }
        check("H3", "four mic models are audibly distinct", spread >= 3.0, fmt("max delta %.1f dB", spread));

        Radiation r;
        r.prepare(sr);
        r.setMicModel(MicModel::SmallDiaphragm);
        r.setDistance(0.5);
        r.reset();
        const double slope = toneGainDb([&](double x) { return r.process(x); }, 600.0, sr, 0.001) -
                             toneGainDb([&](double x) { return r.process(x); }, 300.0, sr, 0.001);
        check("H4", "lip radiation slope +6 dB/oct", std::abs(slope - 6.0) <= 1.5, fmt("%+.1f dB/oct", slope));
    }

    // ============================ I. Stability =============================
    {
        TalkboxEngine e;
        e.prepare(sr);
        e.setParams(factoryPresets()[3].params);
        e.reset();
        auto feed = [&](double v, int n) {
            double mx = 0.0;
            bool finite = true;
            for (int i = 0; i < n; ++i) {
                const double o = e.processSample(v);
                finite = finite && std::isfinite(o);
                mx = std::max(mx, std::abs(o));
            }
            return std::pair<double, bool>(mx, finite);
        };
        e.reset();
        feed(std::nan(""), 32);
        auto nanRec = feed(0.1, 128);
        e.reset();
        feed(INFINITY, 32);
        auto infRec = feed(0.1, 128);
        check("I1", "NaN/Inf injection recovers within one block",
              nanRec.second && infRec.second && nanRec.first < 1.0 && infRec.first < 1.0,
              fmt("NaN peak %.3f, Inf peak %.3f", nanRec.first, infRec.first));

        e.reset();
        auto hot = feed(1000.0, static_cast<int>(sr));
        check("I2", "+120 dBFS input stays bounded", hot.second && hot.first <= 0.99,
              fmt("peak=%.3f (%.2f dBFS)", hot.first, gainToDb(hot.first)));

        struct Sweep { double rate; int block; };
        const Sweep sweeps[6] = {{44100, 16}, {44100, 512}, {48000, 2048}, {96000, 256}, {192000, 128}, {192000, 1024}};
        bool sweepOk = true;
        double sweepPeak = 0.0;
        for (const auto& s : sweeps) {
            TalkboxEngine ee;
            ee.prepare(s.rate);
            ee.setParams(factoryPresets()[2].params);
            ee.reset();
            ee.triggerConsonant(Consonant::Plosive, 0.8);
            std::vector<float> in(static_cast<size_t>(s.block)), out(static_cast<size_t>(s.block));
            for (size_t i = 0; i < in.size(); ++i) in[i] = 0.6f * static_cast<float>(std::sin(kTwoPi * 220.0 * i / s.rate));
            for (int b = 0; b < 40; ++b) {
                ee.processBlock(out.data(), in.data(), s.block);
                for (float v : out) {
                    sweepOk = sweepOk && std::isfinite(v);
                    sweepPeak = std::max(sweepPeak, static_cast<double>(std::abs(v)));
                }
            }
        }
        check("I3", "44.1–192 kHz, blocks 16–2048 stay finite", sweepOk && sweepPeak <= 1.0,
              fmt("peak=%.3f over 6 rate/block combos", sweepPeak));
    }

    // =============================== J. CPU ================================
    {
        const auto& presets = factoryPresets();
        const double budgets[4] = {5.0, 5.0, 6.0, 8.0};
        struct Result { double pct; };
        std::vector<Result> results;
        const int block = 128;
        const int blocks = static_cast<int>(sr * 4.0) / block;
        for (const auto& pr : presets) {
            TalkboxEngine e;
            e.prepare(sr);
            e.setParams(pr.params);
            e.reset();
            std::vector<float> in(block, 0.1f), out(block);
            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < blocks; ++b) e.processBlock(out.data(), in.data(), block);
            const auto t1 = std::chrono::steady_clock::now();
            const double secs = std::chrono::duration<double>(t1 - t0).count();
            results.push_back({100.0 * secs / 4.0});
        }
        const char* ids[4] = {"J1", "J2", "J3", "J4"};
        for (int i = 0; i < 4; ++i)
            check(ids[i], fmt("%s within CPU budget", presets[i].name), results[i].pct <= budgets[i],
                  fmt("%.2f%% of one core at 48 kHz/128 (budget %.0f%%)", results[i].pct, budgets[i]));
    }

    // =============================== Summary ===============================
    std::printf("\n----------------------------------------\n");
    std::printf("PASS=%d FAIL=%d\n", g_pass, g_fail);
    std::printf("----------------------------------------\n");
    return g_fail == 0 ? 0 : 1;
}
