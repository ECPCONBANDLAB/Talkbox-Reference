// TalkBox Reference — Driver.h
// Driver stage: DC block -> drive -> ADAA cubic soft-clip (localised 1x/2x/4x
// oversampling) -> high-pass 90 Hz -> low-pass 6.5 kHz.
//
// The non-linearity is a cubic soft clip whose first antiderivative is known in
// closed form, so first-order ADAA suppresses aliasing without look-ahead.
#pragma once

#include "Common.h"

namespace tb {

// Localised polyphase oversampler: zero-stuffing + windowed-sinc FIR both ways.
class Oversampler {
public:
    void prepare(double sr, int factor) {
        sr_ = sr;
        factor_ = (factor == 4) ? 4 : (factor == 2 ? 2 : 1);
        if (factor_ == 1) {
            up_.design(1, 0.5, 1.0);  // identity-ish (unused)
            down_.design(1, 0.5, 1.0);
            return;
        }
        const int taps = 4 * factor_ + 1;
        const double cutoff = 0.5 / factor_ * 0.92;  // normalised to oversampled rate
        up_.design(taps, cutoff, factor_);
        down_.design(taps, cutoff, 1.0);
        // Each FIR contributes (taps-1)/2 at the oversampled rate: (taps-1)/factor total.
        latency_ = (taps - 1) / factor_;
    }

    int factor() const { return factor_; }
    int latencySamples() const { return latency_; }

    template <typename Nl>
    double process(double x, Nl&& nl) {
        if (factor_ == 1) return nl(x);
        double y = 0.0;
        for (int i = 0; i < factor_; ++i) {
            const double up = up_.process(i == 0 ? x : 0.0);
            const double shaped = nl(up);
            y = down_.process(shaped);
        }
        return y;
    }

    void reset() {
        up_.reset();
        down_.reset();
    }

private:
    double sr_ = 48000.0;
    int factor_ = 1;
    int latency_ = 0;
    FirLowpass up_, down_;
};

class Driver {
public:
    static constexpr double kHpHz = 90.0;
    static constexpr double kLpHz = 6500.0;
    static constexpr double kDcHz = 18.0;

    void prepare(double sr) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        dc_.setCutoff(sr_, kDcHz);
        hp_.setHighpass(sr_, kHpHz, 0.7071);
        lp_.setLowpass(sr_, kLpHz, 0.7071);
        setOversample(oversample_);
        reset();
    }

    void setOversample(int factor) {
        oversample_ = (factor >= 4) ? 4 : (factor == 2 ? 2 : 1);
        os_.prepare(sr_, oversample_);
        reset();
    }
    int oversample() const { return oversample_; }
    int latencySamples() const { return os_.latencySamples(); }

    void setDriveDb(double db) { driveDb_ = clampd(db, -24.0, 36.0); }
    double driveDb() const { return driveDb_; }
    void setOutputTrimDb(double db) { trimDb_ = clampd(db, -24.0, 12.0); }

    double process(double x) {
        if (!std::isfinite(x)) x = 0.0;
        const double dc = dc_.process(x);
        const double pre = dc * dbToGain(driveDb_);
        const double shaped = os_.process(pre, [this](double v) { return adaa(v); });
        double y = hp_.process(shaped);
        y = lp_.process(y);
        y *= dbToGain(trimDb_ + compDb());
        return std::isfinite(y) ? y : 0.0;
    }

    void reset() {
        dc_.reset();
        hp_.reset();
        lp_.reset();
        os_.reset();
        adaaXPrev_ = 0.0;
        adaaFPrev_ = anti(0.0);
    }

    // Cubic soft clip and its antiderivative (exposed for tests).
    static double clip(double x) {
        if (x >= 1.0) return 2.0 / 3.0;
        if (x <= -1.0) return -2.0 / 3.0;
        return x - x * x * x / 3.0;
    }
    // F is even (the antiderivative of an odd function) and continuous at |x|=1.
    static double anti(double x) {
        if (x >= 1.0) return 2.0 * x / 3.0 - 0.25;
        if (x <= -1.0) return -2.0 * x / 3.0 - 0.25;
        return 0.5 * x * x - x * x * x * x / 12.0;
    }

    // First-order ADAA around the cubic soft clip.
    double adaa(double x) {
        const double f = anti(x);
        const double dx = x - adaaXPrev_;
        double y;
        if (std::abs(dx) > 1e-6)
            y = (f - adaaFPrev_) / dx;
        else
            y = clip(0.5 * (x + adaaXPrev_));
        adaaXPrev_ = x;
        adaaFPrev_ = f;
        return y;
    }

    double compDb() const { return -driveDb_ * 0.6; }

private:
    double sr_ = 48000.0;
    double driveDb_ = 0.0, trimDb_ = 0.0;
    int oversample_ = 2;
    DcBlocker dc_;
    Biquad hp_, lp_;
    Oversampler os_;
    double adaaXPrev_ = 0.0, adaaFPrev_ = 0.0;
};

}  // namespace tb
