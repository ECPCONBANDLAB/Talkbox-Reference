// TalkBox Reference — Radiation.h
// Lip radiation, four microphone models, proximity effect, look-ahead limiter
// and the consonant bridge (fricative / plosive / nasal transients).
#pragma once

#include "Common.h"

namespace tb {

enum class MicModel : int {
    Dynamic = 0,
    SmallDiaphragm = 1,
    Condenser = 2,
    Ribbon = 3
};

// Feed-forward look-ahead limiter (instant attack, 80 ms release).
class Limiter {
public:
    void prepare(double sr, int lookaheadSamples = 32) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        look_ = std::max(1, lookaheadSamples);
        buf_.assign(static_cast<size_t>(look_ + 1), 0.0);
        idx_ = 0;
        env_ = 0.0;
        relA_ = std::exp(-1.0 / (0.080 * sr_));
        reset();
    }
    void setCeilingDb(double db) { ceiling_ = dbToGain(clampd(db, -12.0, 0.0)); }
    double ceiling() const { return ceiling_; }
    int latencySamples() const { return look_; }

    double process(double x) {
        if (!std::isfinite(x)) x = 0.0;
        buf_[static_cast<size_t>(idx_)] = x;
        const int readIdx = (idx_ - look_ + static_cast<int>(buf_.size())) % static_cast<int>(buf_.size());
        const double xd = buf_[static_cast<size_t>(readIdx)];
        idx_ = (idx_ + 1) % static_cast<int>(buf_.size());

        const double mag = std::abs(x);
        env_ = (mag > env_) ? mag : (relA_ * env_ + (1.0 - relA_) * mag);
        if (!std::isfinite(env_)) env_ = 0.0;
        const double g = (env_ > ceiling_ && env_ > 1e-9) ? ceiling_ / env_ : 1.0;
        double y = xd * g;
        // Safety net: never let a sample through above the ceiling.
        y = clampd(y, -ceiling_, ceiling_);
        return y;
    }

    void reset() {
        std::fill(buf_.begin(), buf_.end(), 0.0);
        idx_ = 0;
        env_ = 0.0;
    }

private:
    double sr_ = 48000.0, ceiling_ = 0.9772, env_ = 0.0, relA_ = 0.999;
    int look_ = 32, idx_ = 0;
    std::vector<double> buf_;
};

class Radiation {
public:
    void prepare(double sr) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        limiter_.prepare(sr_, 32);
        radLp_.setCutoff(sr_, 9000.0);
        setMicModel(model_);
        setDistance(distance_);
        reset();
    }

    void setMicModel(MicModel m) {
        model_ = m;
        configure();
    }
    MicModel micModel() const { return model_; }

    void setDistance(double metres) {
        distance_ = clampd(metres, 0.02, 1.5);
        const double prox = 6.0 * clampd(0.15 / distance_, 0.0, 4.0) * proxScale();
        prox_.setLowShelf(sr_, 130.0, 0.8, prox);
    }
    double distance() const { return distance_; }

    void setCeilingDb(double db) { limiter_.setCeilingDb(db); }
    double ceilingDb() const { return gainToDb(limiter_.ceiling()); }
    int latencySamples() const { return limiter_.latencySamples(); }

    void setLipModel(bool enabled) { lipOn_ = enabled; }
    // The physical chain is inherently low-gain (a 220 Hz carrier sits far below
    // F1 and the lip differentiator is -13 dB there), so a fixed makeup stage
    // restores a nominal working level before the limiter.
    void setMakeupDb(double db) { makeupDb_ = clampd(db, 0.0, 36.0); }
    double makeupDb() const { return makeupDb_; }
    void setSaturation(double amount) { sat_ = clampd(amount, 0.0, 1.0); }

    double process(double x) {
        if (!std::isfinite(x)) x = 0.0;
        // Lip radiation: true first-order difference (+6 dB/oct) with a gentle
        // one-pole roll-off so the top octave does not turn to fizz.
        double y = lipOn_ ? 8.0 * (x - xPrev_) : x;
        xPrev_ = x;
        if (lipOn_) y = radLp_.process(y);
        y = hp_.process(y);
        y = prox_.process(y);
        y = peak_.process(y);
        y = shelf_.process(y);
        if (sat_ > 0.0) {
            const double g = 1.0 + 2.0 * sat_;
            y = std::tanh(y * g) / std::tanh(g);
        }
        y *= dbToGain(makeupDb_);
        y = limiter_.process(y);
        return std::isfinite(y) ? y : 0.0;
    }

    // Impulse/sine magnitude response of the mic colouration only (no limiter).
    double colourResponseDb(double hz) const {
        // Direct evaluation of the biquad cascade at a single frequency.
        auto resp = [](const Biquad& b, double w) {
            const std::complex<double> z1 = std::polar(1.0, -w);
            const std::complex<double> num = b.b0 + b.b1 * z1 + b.b2 * z1 * z1;
            const std::complex<double> den = 1.0 + b.a1 * z1 + b.a2 * z1 * z1;
            return num / den;
        };
        const double w = kTwoPi * hz / sr_;
        std::complex<double> h = resp(hp_, w) * resp(prox_, w) * resp(peak_, w) * resp(shelf_, w);
        return 20.0 * std::log10(std::max(std::abs(h), 1e-12));
    }

    void reset() {
        xPrev_ = 0.0;
        radLp_.reset();
        hp_.reset();
        prox_.reset();
        peak_.reset();
        shelf_.reset();
        limiter_.reset();
    }

private:
    double proxScale() const {
        switch (model_) {
            case MicModel::Ribbon: return 1.5;
            case MicModel::Dynamic: return 1.0;
            case MicModel::Condenser: return 0.8;
            default: return 0.6;
        }
    }

    void configure() {
        switch (model_) {
            case MicModel::Dynamic:
                hp_.setHighpass(sr_, 55.0, 0.7071);
                peak_.setPeaking(sr_, 5500.0, 1.2, 3.0);
                shelf_.setHighShelf(sr_, 11000.0, 0.7071, -4.0);
                break;
            case MicModel::SmallDiaphragm:
                hp_.setHighpass(sr_, 30.0, 0.7071);
                peak_.setPeaking(sr_, 8000.0, 1.0, 1.5);
                shelf_.setHighShelf(sr_, 12000.0, 0.7071, 1.0);
                break;
            case MicModel::Condenser:
                hp_.setHighpass(sr_, 35.0, 0.7071);
                peak_.setPeaking(sr_, 10000.0, 1.0, 2.0);
                shelf_.setLowShelf(sr_, 220.0, 0.8, 2.5);
                break;
            case MicModel::Ribbon:
                hp_.setHighpass(sr_, 45.0, 0.7071);
                peak_.setPeaking(sr_, 3000.0, 0.9, 1.5);
                shelf_.setHighShelf(sr_, 7000.0, 0.7071, -7.0);
                break;
        }
    }

    double sr_ = 48000.0;
    double distance_ = 0.15, xPrev_ = 0.0, sat_ = 0.0, makeupDb_ = 26.0;
    bool lipOn_ = true;
    MicModel model_ = MicModel::Dynamic;
    Biquad hp_, prox_, peak_, shelf_;
    OnePoleLP radLp_;
    Limiter limiter_;
};

enum class Consonant : int {
    None = 0,
    S = 1,
    Sh = 2,
    Plosive = 3,
    Nasal = 4
};

// The bridge injects consonant transients into the tract output so the plugin
// can articulate without a separate voice model.
class ConsonantBridge {
public:
    void prepare(double sr) {
        sr_ = sr > 1000.0 ? sr : 48000.0;
        hpNoise_.setHighpass(sr_, 3800.0, 0.7071);
        hpNoise2_.setHighpass(sr_, 3800.0, 0.7071);
        band_.setPeaking(sr_, 6500.0, 1.2, 6.0);
        lpNoise_.setLowpass(sr_, 2600.0, 0.7071);
        burstLp_.setLowpass(sr_, 1800.0, 0.7071);
        nasalRes_.set(sr_, 260.0, 3.0, 1.0);
        rng_.seed(0x2f1b3c5du);
        reset();
    }

    void setSibilanceMix(double hf, double lf) { mixHf_ = hf; mixLf_ = lf; }

    void trigger(Consonant c, double level = 1.0) {
        type_ = c;
        level_ = clampd(level, 0.0, 1.0);
        phase_ = 0;
        env_ = 0.0;
        switch (c) {
            case Consonant::S:
            case Consonant::Sh: len_ = static_cast<int>(0.16 * sr_); break;
            case Consonant::Plosive: len_ = static_cast<int>(0.09 * sr_); break;
            case Consonant::Nasal: len_ = static_cast<int>(0.18 * sr_); break;
            default: len_ = 0; break;
        }
    }

    bool active() const { return type_ != Consonant::None && phase_ < len_; }

    // Returns the consonant contribution only; the host adds it to the tract.
    double process() {
        if (!active()) {
            if (type_ != Consonant::None && phase_ >= len_) type_ = Consonant::None;
            return 0.0;
        }
        const double t = static_cast<double>(phase_) / sr_;
        const double noise = rng_.bipolar();
        double y = 0.0;
        switch (type_) {
            case Consonant::S: {
                const double e = std::exp(-t / 0.045) * (1.0 - std::exp(-t / 0.004));
                const double n = band_.process(hpNoise_.process(noise));
                y = 0.55 * e * (n * mixHf_ + mixLf_ * lpNoise_.process(noise));
                break;
            }
            case Consonant::Sh: {
                const double e = std::exp(-t / 0.055) * (1.0 - std::exp(-t / 0.005));
                const double n = hpNoise2_.process(noise);
                y = 0.42 * e * n;
                break;
            }
            case Consonant::Plosive: {
                const double e = std::exp(-t / 0.012) * (1.0 - std::exp(-t / 0.0015));
                y = 0.75 * e * burstLp_.process(noise);
                break;
            }
            case Consonant::Nasal: {
                const double e = std::exp(-t / 0.07) * (1.0 - std::exp(-t / 0.01));
                y = 0.30 * e * nasalRes_.process(noise);
                break;
            }
            default: break;
        }
        env_ = y;
        ++phase_;
        return std::isfinite(y) ? y * level_ : 0.0;
    }

    void reset() {
        type_ = Consonant::None;
        phase_ = 0;
        len_ = 0;
        env_ = 0.0;
        hpNoise_.reset();
        hpNoise2_.reset();
        band_.reset();
        lpNoise_.reset();
        burstLp_.reset();
        nasalRes_.reset();
    }

private:
    double mixHf_ = 1.05, mixLf_ = 0.42;
    double sr_ = 48000.0, level_ = 0.0, env_ = 0.0;
    Consonant type_ = Consonant::None;
    int phase_ = 0, len_ = 0;
    Biquad hpNoise_, hpNoise2_, band_, lpNoise_, burstLp_;
    TptBandpass nasalRes_;
    Rng rng_;
};

}  // namespace tb
