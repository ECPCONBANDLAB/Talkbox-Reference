// TalkBox Reference — VocalTract.h
// Source-filter vocal tract: a normalised reflection-coefficient lattice
// (|k| <= 0.995) feeding a five-formant TPT resonator bank (F1–F5), plus a
// nasal branch and a lip-aperture stage.
#pragma once

#include "Common.h"

namespace tb {

struct Formant {
    double freq = 500.0;
    double bw = 80.0;
    double gain = 1.0;
};

class VocalTract {
public:
    static constexpr int kNumFormants = 5;
    static constexpr int kNumVowels = 9;

    void prepare(double sr) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        for (auto& r : res_) r.reset();
        nasalLow_.reset();
        nasalHigh_.reset();
        lipLp_.reset();
        std::fill(std::begin(yHist_), std::end(yHist_), 0.0);
        xPrev_ = 0.0;
        update();
    }

    // --- Vowel table (Hz / Hz bandwidth) -----------------------------------
    static const Formant* vowelTable(int vowel) {
        static const Formant table[kNumVowels][kNumFormants] = {
            {{270, 60, 1.00}, {2290, 90, 0.63}, {3010, 120, 0.40}, {3500, 140, 0.25}, {4500, 180, 0.16}},  // i
            {{390, 70, 1.00}, {1990, 100, 0.60}, {2550, 130, 0.38}, {3300, 150, 0.24}, {4400, 180, 0.15}}, // I
            {{530, 70, 1.00}, {1840, 100, 0.58}, {2480, 130, 0.36}, {3300, 150, 0.23}, {4400, 180, 0.15}}, // e
            {{660, 80, 1.00}, {1720, 110, 0.57}, {2410, 130, 0.35}, {3300, 150, 0.22}, {4400, 180, 0.14}}, // E
            {{730, 90, 1.00}, {1090, 110, 0.55}, {2440, 140, 0.34}, {3350, 160, 0.22}, {4400, 180, 0.14}}, // ae
            {{700, 80, 1.00}, {1220, 100, 0.56}, {2600, 130, 0.35}, {3400, 150, 0.22}, {4500, 180, 0.14}}, // a
            {{570, 70, 1.00}, {840, 90, 0.54}, {2410, 130, 0.34}, {3300, 150, 0.21}, {4400, 180, 0.14}},   // open o
            {{430, 60, 1.00}, {1020, 90, 0.55}, {2340, 130, 0.34}, {3300, 150, 0.21}, {4400, 180, 0.14}},  // o
            {{300, 60, 1.00}, {870, 80, 0.52}, {2240, 120, 0.33}, {3300, 150, 0.20}, {4400, 180, 0.13}}    // u
        };
        const int v = std::max(0, std::min(vowel, kNumVowels - 1));
        return table[v];
    }

    void setVowel(int vowel) {
        vowel_ = std::max(0, std::min(vowel, kNumVowels - 1));
        const Formant* f = vowelTable(vowel_);
        for (int i = 0; i < kNumFormants; ++i) formants_[i] = f[i];
        // Lattice coefficients: progressively lighter, always |k| <= 0.995.
        for (int m = 0; m < kLatticeSections; ++m) {
            const double k = 0.42 - 0.055 * m;
            k_[m] = clampd(k * latticeAmount_, -0.995, 0.995);
        }
        update();
    }

    void setFormants(const Formant* f) {
        for (int i = 0; i < kNumFormants; ++i) formants_[i] = f[i];
        update();
    }

    void setVTL(double vtl) {
        vtl_ = clampd(vtl, 0.7, 1.45);
        update();
    }
    double vtl() const { return vtl_; }

    void setLipAperture(double a) {
        aperture_ = clampd(a, 0.0, 1.0);
        updateLip();
    }
    double lipAperture() const { return aperture_; }

    void setNasalAmount(double n) { nasal_ = clampd(n, 0.0, 1.0); }
    double nasalAmount() const { return nasal_; }

    void setLatticeAmount(double a) {
        latticeAmount_ = clampd(a, 0.0, 1.0);
        const int keep = vowel_;
        setVowel(keep);
    }
    double latticeAmount() const { return latticeAmount_; }

    void setFormantShift(double semitones) {
        shift_ = std::pow(2.0, clampd(semitones, -12.0, 12.0) / 12.0);
        update();
    }

    double formantHz(int i) const { return applied_[i].freq; }

    double process(double x) {
        if (!std::isfinite(x)) x = 0.0;

        // 1. All-pole lattice: reflection coefficients k (|k| <= 0.995) are
        //    converted to direct-form coefficients by the step-up recursion,
        //    which is stable for every |k| < 1.
        double lattice = x;
        for (int i = 1; i <= kLatticeSections; ++i) lattice -= a_[i] * yHist_[i - 1];
        for (int i = kLatticeSections - 1; i > 0; --i) yHist_[i] = yHist_[i - 1];
        yHist_[0] = std::isfinite(lattice) ? lattice : 0.0;
        const double f = lattice;
        xPrev_ = x;

        // 2. Parallel formant bank.
        double y = 0.0;
        for (int i = 0; i < kNumFormants; ++i) y += res_[i].process(f);
        (void)0;

        // 3. Nasal branch: low pole with a broad anti-resonance above it.
        if (nasal_ > 0.0) {
            const double nl = nasalLow_.process(f);
            const double nh = nasalHigh_.process(f);
            y += nasal_ * (0.45 * nl - 0.28 * nh);
        }

        // 4. Lip aperture: closure attenuates and dulls the output.
        y = lipLp_.process(y) * lipGain_;
        return std::isfinite(y) ? y : 0.0;
    }

    void reset() {
        for (auto& r : res_) r.reset();
        nasalLow_.reset();
        nasalHigh_.reset();
        lipLp_.reset();
        std::fill(std::begin(yHist_), std::end(yHist_), 0.0);
        xPrev_ = 0.0;
    }

    const Formant* appliedFormants() const { return applied_; }

private:
    static constexpr int kLatticeSections = 6;

    void update() {
        for (int i = 0; i < kNumFormants; ++i) {
            applied_[i] = formants_[i];
            applied_[i].freq = formants_[i].freq * shift_ / vtl_;
        }
        for (int i = 0; i < kNumFormants; ++i) {
            const double fc = clampd(applied_[i].freq, 40.0, sr_ * 0.45);
            const double q = clampd(applied_[i].freq / std::max(applied_[i].bw, 20.0), 0.5, 45.0);
            res_[i].set(sr_, fc, q, applied_[i].gain);
        }
        nasalLow_.set(sr_, 280.0 / vtl_, 3.0, 1.0);
        nasalHigh_.set(sr_, 900.0 / vtl_, 5.0, 1.0);
        updateLattice();
        updateLip();
    }

    // Step-up recursion: k_1..k_N  ->  A(z) = 1 + a_1 z^-1 + ... + a_N z^-N.
    void updateLattice() {
        double a[kLatticeSections + 1]{};
        a[0] = 1.0;
        for (int m = 1; m <= kLatticeSections; ++m) {
            const double km = k_[m - 1];
            const double prev[kLatticeSections + 1] = {a[0], a[1], a[2], a[3], a[4], a[5], a[6]};
            for (int i = 1; i <= m; ++i) a[i] = prev[i] + km * prev[m - i];
        }
        for (int i = 0; i <= kLatticeSections; ++i) a_[i] = a[i];
    }

    void updateLip() {
        lipGain_ = 0.12 + 0.88 * aperture_;
        // A closed lip removes the high-frequency radiation path.
        const double cutoff = 900.0 + 11000.0 * aperture_;
        lipLp_.setLowpass(sr_, cutoff, 0.7071);
    }

    double sr_ = 48000.0;
    double vtl_ = 1.0, shift_ = 1.0, aperture_ = 1.0, nasal_ = 0.0, latticeAmount_ = 0.5;
    int vowel_ = 5;  // /a/
    Formant formants_[kNumFormants]{};
    Formant applied_[kNumFormants]{};
    TptBandpass res_[kNumFormants];
    TptBandpass nasalLow_, nasalHigh_;
    Biquad lipLp_;
    double lipGain_ = 1.0;
    double k_[kLatticeSections]{};
    double a_[kLatticeSections + 1]{};
    double yHist_[kLatticeSections]{};
    double xPrev_ = 0.0;
};

}  // namespace tb
