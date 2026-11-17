// TalkBox Reference — Engine.h
// Full source-filter chain and the factory preset table.
//
//   Carrier -> DC block 18 Hz -> Driver (ADAA + HP 90 / LP 6.5 k, 1x/2x/4x)
//           -> Hose waveguide (1.5–2.0 m) -> Vocal tract (lattice + F1–F5
//           + nasal + lip) -> Consonant bridge -> Radiation + mic -> Limiter
#pragma once

#include "Carrier.h"
#include "Common.h"
#include "Driver.h"
#include "Hose.h"
#include "Radiation.h"
#include "VocalTract.h"

namespace tb {

struct EngineParams {
    double vtl = 1.0;
    double hoseLength = 1.75;
    double hoseFeedback = 0.88;
    double driveDb = 6.0;
    int oversample = 2;
    int vowel = 5;              // /a/
    double formantShift = 0.0;  // semitones
    double nasal = 0.12;
    double lipAperture = 1.0;
    MicModel mic = MicModel::Dynamic;
    double micDistance = 0.15;
    double saturation = 0.15;
    CarrierMode carrierMode = CarrierMode::External;
    double carrierFreq = 110.0;
    double carrierNoiseMix = 0.0;
    double outputDb = -3.0;
    double ceilingDb = -0.3;
    bool driverEnabled = true;
    bool hoseEnabled = true;
    bool radiationEnabled = true;
    bool carrierMute = false;
};

class TalkboxEngine {
public:
    void prepare(double sr, int maxBlock = 1024) {
        (void)maxBlock;
        sr_ = sr > 1000.0 ? sr : 48000.0;
        carrier_.prepare(sr_);
        driver_.prepare(sr_);
        hose_.prepare(sr_, Hose::kMaxLengthM);
        tract_.prepare(sr_);
        radiation_.prepare(sr_);
        bridge_.prepare(sr_);
        applyParams(params_);
        reset();
    }

    void reset() {
        carrier_.reset();
        driver_.reset();
        hose_.reset();
        tract_.reset();
        radiation_.reset();
        bridge_.reset();
    }

    double sampleRate() const { return sr_; }

    void setParams(const EngineParams& p) {
        params_ = p;
        applyParams(p);
    }
    const EngineParams& params() const { return params_; }

    void setVowel(int v) {
        params_.vowel = v;
        tract_.setVowel(v);
    }
    void setCarrierMode(CarrierMode m) {
        params_.carrierMode = m;
        carrier_.setMode(m);
    }
    void setDriveDb(double db) {
        params_.driveDb = db;
        driver_.setDriveDb(db);
    }
    void setHoseLength(double m) {
        params_.hoseLength = m;
        hose_.setLength(m);
    }
    void setMicModel(MicModel m) {
        params_.mic = m;
        radiation_.setMicModel(m);
    }
    void setOversample(int f) {
        params_.oversample = f;
        driver_.setOversample(f);
    }
    void triggerConsonant(Consonant c, double level = 1.0) { bridge_.trigger(c, level); }

    int latencySamples() const {
        return driver_.latencySamples() + radiation_.latencySamples();
    }
    double latencyMs() const { return latencySamples() / sr_ * 1000.0; }
    double hoseDelayMs() const { return hose_.delayMs(); }

    // ---- stage accessors (tests / UI metering) ---------------------------
    Carrier& carrier() { return carrier_; }
    Driver& driver() { return driver_; }
    Hose& hose() { return hose_; }
    VocalTract& tract() { return tract_; }
    Radiation& radiation() { return radiation_; }
    ConsonantBridge& bridge() { return bridge_; }

    double processSample(double carrierIn) {
        if (!std::isfinite(carrierIn)) carrierIn = 0.0;
        if (mute_) return 0.0;
        double x = carrier_.process(carrierIn);
        if (params_.driverEnabled) x = driver_.process(x);
        if (params_.hoseEnabled) x = hose_.process(x);
        x = tract_.process(x);
        if (bridge_.active()) x += bridge_.process();
        if (params_.radiationEnabled) x = radiation_.process(x);
        x *= dbToGain(params_.outputDb);
        if (!std::isfinite(x)) x = 0.0;
        return x;
    }

    void processBlock(double* out, const double* carrierIn, int n) {
        for (int i = 0; i < n; ++i) out[i] = processSample(carrierIn ? carrierIn[i] : 0.0);
    }
    void processBlock(float* out, const float* carrierIn, int n) {
        for (int i = 0; i < n; ++i)
            out[i] = static_cast<float>(processSample(carrierIn ? carrierIn[i] : 0.0));
    }
    void processBlockStereo(float* l, float* r, const float* carrierIn, int n) {
        for (int i = 0; i < n; ++i) {
            const float v = static_cast<float>(processSample(carrierIn ? carrierIn[i] : 0.0));
            l[i] = v;
            r[i] = v;
        }
    }

    void setMute(bool m) { mute_ = m; }
    bool muted() const { return mute_; }
    void setCarrierMute(bool m) {
        params_.carrierMute = m;
        carrierMute_ = m;
        carrier_.setMuted(m);
    }

private:
    void applyParams(const EngineParams& p) {
        carrier_.setMode(p.carrierMode);
        carrier_.setFrequency(p.carrierFreq);
        carrier_.setNoiseMix(p.carrierNoiseMix);
        carrierMute_ = p.carrierMute;
        carrier_.setMuted(p.carrierMute);

        driver_.setDriveDb(p.driveDb);
        if (p.oversample != driver_.oversample()) driver_.setOversample(p.oversample);

        hose_.setLength(p.hoseLength);
        hose_.setFeedback(p.hoseFeedback);
        hose_.setDampingHz(5200.0);

        tract_.setVowel(p.vowel);
        tract_.setVTL(p.vtl);
        tract_.setNasalAmount(p.nasal);
        tract_.setLipAperture(p.lipAperture);
        tract_.setFormantShift(p.formantShift);

        radiation_.setMicModel(p.mic);
        radiation_.setDistance(p.micDistance);
        radiation_.setSaturation(p.saturation);
        radiation_.setCeilingDb(p.ceilingDb);
    }

    double sr_ = 48000.0;
    bool mute_ = false, carrierMute_ = false;
    EngineParams params_{};
    Carrier carrier_;
    Driver driver_;
    Hose hose_;
    VocalTract tract_;
    Radiation radiation_;
    ConsonantBridge bridge_;
};

// ---------------------------------------------------------------------------
// Factory presets — mirrored in presets/factory_presets.json
// ---------------------------------------------------------------------------
struct Preset {
    const char* name;
    const char* description;
    EngineParams params;
};

inline EngineParams makePresetBase() {
    EngineParams p;
    p.vtl = 1.0;
    p.carrierMode = CarrierMode::External;
    p.oversample = 2;
    p.mic = MicModel::Dynamic;
    p.outputDb = -3.0;
    return p;
}

inline const std::vector<Preset>& factoryPresets() {
    static const std::vector<Preset> presets = [] {
        std::vector<Preset> v;

        EngineParams eco = makePresetBase();
        eco.hoseLength = 1.60;
        eco.driveDb = 3.0;
        eco.oversample = 1;
        eco.mic = MicModel::SmallDiaphragm;
        eco.carrierMode = CarrierMode::Saw;
        eco.carrierFreq = 110.0;
        eco.nasal = 0.20;
        eco.saturation = 0.10;
        eco.outputDb = -4.0;
        v.push_back({"Eco Pad", "Lowest CPU: 1x oversampling, internal saw carrier", eco});

        EngineParams mic = makePresetBase();
        mic.hoseLength = 1.75;
        mic.driveDb = 0.0;
        mic.oversample = 1;
        mic.mic = MicModel::Dynamic;
        mic.micDistance = 0.10;
        mic.saturation = 0.05;
        mic.outputDb = -2.0;
        v.push_back({"Mic", "Microphone-first: external carrier, minimal drive", mic});

        EngineParams bal = makePresetBase();
        bal.hoseLength = 1.75;
        bal.driveDb = 6.0;
        bal.oversample = 2;
        bal.mic = MicModel::Dynamic;
        bal.micDistance = 0.15;
        bal.saturation = 0.18;
        bal.outputDb = -3.0;
        v.push_back({"Balanced", "Default all-round voice: 2x oversampling, 6 dB drive", bal});

        EngineParams studio = makePresetBase();
        studio.hoseLength = 1.90;
        studio.driveDb = 9.0;
        studio.oversample = 4;
        studio.mic = MicModel::Condenser;
        studio.micDistance = 0.22;
        studio.nasal = 0.08;
        studio.saturation = 0.25;
        studio.outputDb = -4.0;
        v.push_back({"Studio", "Maximum detail: 4x oversampling, condenser, long hose", studio});

        return v;
    }();
    return presets;
}

}  // namespace tb
