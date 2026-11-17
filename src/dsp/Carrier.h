// TalkBox Reference — Carrier.h
// Source stage: external carrier (guitar / synth) or internal PolyBLEP oscillator,
// with a noise blend and a click-free crossfade.
#pragma once

#include "Common.h"

namespace tb {

enum class CarrierMode : int {
    External = 0,
    Saw = 1,
    Square = 2,
    Pulse = 3,
    Noise = 4
};

class Carrier {
public:
    void prepare(double sr) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        phase_ = 0.0;
        click_.reset();
        level_.reset();
        // Gain ramp only: the smoother never sits in the audio path, otherwise
        // it would low-pass the carrier itself.
        level_.setRelease(0.005, sr_);
        rng_.seed(0x51ed270bu);
    }

    void setMode(CarrierMode m) { mode_ = m; }
    CarrierMode mode() const { return mode_; }
    void setFrequency(double hz) { freq_ = clampd(hz, 20.0, sr_ * 0.45); }
    double frequency() const { return freq_; }
    void setPulseWidth(double pw) { pw_ = clampd(pw, 0.05, 0.95); }
    void setNoiseMix(double m) { noiseMix_ = clampd(m, 0.0, 1.0); }
    double noiseMix() const { return noiseMix_; }
    void setMuted(bool m) { muted_ = m; }
    bool muted() const { return muted_; }

    // carrierIn may be nullptr; internal oscillator then runs standalone.
    double process(double carrierIn) {
        const double dt = freq_ / sr_;
        const double naiveSaw = 2.0 * phase_ - 1.0;
        const double blepSaw = naiveSaw - polyBlep(phase_, dt);

        const double p2 = wrap(phase_ + 0.5);
        const double blepSq = (phase_ < 0.5 ? 1.0 : -1.0) + polyBlep(phase_, dt) - polyBlep(p2, dt);

        const double pPw = wrap(phase_ + (1.0 - pw_));
        double pulse = (phase_ < pw_ ? 1.0 : -1.0) + polyBlep(phase_, dt) - polyBlep(pPw, dt);
        pulse -= (2.0 * pw_ - 1.0);  // remove DC so narrow pulses stay centred

        const double noise = rng_.bipolar();
        double osc = 0.0;
        switch (mode_) {
            case CarrierMode::External: osc = carrierIn; break;
            case CarrierMode::Saw: osc = blepSaw * 0.5; break;
            case CarrierMode::Square: osc = blepSq * 0.35; break;
            case CarrierMode::Pulse: osc = pulse * 0.3; break;
            case CarrierMode::Noise: osc = noise * 0.3; break;
        }
        if (mode_ == CarrierMode::External && noiseMix_ > 0.0) osc = osc * (1.0 - noiseMix_) + noise * noiseMix_;

        phase_ = wrap(phase_ + dt);
        if (muted_) return 0.0;
        const double gated = osc * level_.process(1.0);
        // Hard safety: the carrier stage must never hand a non-finite value downstream.
        return std::isfinite(gated) ? gated : 0.0;
    }

    // Reference implementation without PolyBLEP — used by the alias test.
    double processNaiveSaw(double carrierIn) {
        (void)carrierIn;
        const double dt = freq_ / sr_;
        (void)0;
        const double v = 2.0 * naivePhase_ - 1.0;
        naivePhase_ = wrap(naivePhase_ + dt);
        return v * 0.5;
    }

    void reset() {
        phase_ = 0.0;
        naivePhase_ = 0.0;
        click_.reset();
        level_.reset();
    }

private:
    struct Smoother {
        double a = 0.0, y = 0.0;
        void setAttack(double t, double sr) { a = std::exp(-1.0 / std::max(1.0, t * sr)); }
        void setRelease(double t, double sr) { a = std::exp(-1.0 / std::max(1.0, t * sr)); }
        double process(double x) {
            y = (1.0 - a) * x + a * y;
            return y;
        }
        void reset() { y = 0.0; }
    };

    static double wrap(double p) {
        p -= std::floor(p);
        return p;
    }
    // PolyBLEP residual around the discontinuity (band-limited step).
    static double polyBlep(double t, double dt) {
        if (dt <= 0.0) return 0.0;
        if (t < dt) {
            const double x = t / dt;
            return x + x - x * x - 1.0;
        }
        if (t > 1.0 - dt) {
            const double x = (t - 1.0) / dt;
            return x * x + x + x + 1.0;
        }
        return 0.0;
    }

    double sr_ = 48000.0;
    double phase_ = 0.0, naivePhase_ = 0.0;
    double freq_ = 110.0, pw_ = 0.5, noiseMix_ = 0.0;
    bool muted_ = false;
    CarrierMode mode_ = CarrierMode::External;
    Rng rng_;
    Smoother click_, level_;
};

}  // namespace tb
