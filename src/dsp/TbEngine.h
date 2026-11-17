// TalkBox Reference — TalkboxEngine (orchestrator)
// Signal chain: Carrier -> DC -> Driver(ADAA+BP) -> Hose -> VocalTract
//               -> Lip/Mic -> Consonant Bridge blend -> Mix/Limiter
// Control: Pad (articulatory) OR Live Mic (LPC) OR MIDI/Automation.
// Quality: Eco (0-lat, ctrl 32) / Balanced (2x OS on driver, ctrl 16) /
//          Studio (4x OS, physical hose) / Creative (extended VTL).
// Zero alloc / zero locks on audio thread. All buffers preallocated.
#pragma once
#include "TbCore.h"
#include "TbSynthDriver.h"
#include "TbVocalTract.h"
#include "TbLpcMic.h"
#include <array>

namespace tb {

enum class QualityMode { Eco=0, Balanced=1, Studio=2, Creative=3 };
enum class SourceMode { Pad=0, Mic=1 };
enum class CarrierMode { External=0, Internal=1 };
enum class GateBehavior { CloseMouth=0, HoldVowel=1, ReturnToPad=2 };

struct EngineParams {
    // Carrier / driver
    float inputGain = 1.0f;
    float carrierDrive = 0.6f;      // driver drive
    float diaphragmAsym = 0.0f;
    float driverComp = 0.3f;
    float hornCut = 6500.0f;
    // Hose
    float hoseLength = 1.75f;       // m
    float hoseDamping = 0.3f;
    // Articulation (pad)
    float padX = 0.5f, padY = 0.45f;
    float jaw = 0.45f, tongueX = 0.5f, tongueHeight = 0.45f;
    float lipRounding = 0.0f, mouthOpening = 0.8f, nasal = 0.0f;
    float vtl = 1.0f, glide = 0.3f;
    // Macros that were previously declared in the UI but never reached the DSP
    float mouthSize = 1.0f;      // 0.75..1.35, talkbox aperture
    float articSmooth = 0.5f;    // 0..1, single-knob articulation inertia
    float tubeCharacter = 0.5f;  // 0..1, transistor..tube even harmonics
    float micProximity = 0.5f;   // 0..1, single-knob proximity effect
    // Mic
    float micPreGain = 1.0f, gateThresh = 0.008f, whisperBoost = 1.0f;
    float consonantCrisp = 0.7f, sibilDeess = 0.3f, voiceBleed = 0.0f;
    float micSmooth = 0.6f;
    GateBehavior gateBehavior = GateBehavior::CloseMouth;
    // Lip/mic
    int micModel = 0; float micDistance = 3.0f, proxBoost = 0.5f;
    float ambience = 0.0f; // 0 default
    // Mix
    float dryWet = 1.0f; // 1=wet only (talkbox), <1 blends dry carrier
    float outputGain = 1.0f;
    // Synth
    int synthWave = 0; float synthOctave = 0.0f, synthGlide = 0.06f, synthVibrato = 0.0f;
    // Modes
    QualityMode quality = QualityMode::Balanced;
    SourceMode source = SourceMode::Pad;
    CarrierMode carrier = CarrierMode::External;
    bool bypass = false;
};

struct TalkboxEngine {
    EngineParams p;
    float fs = 48000.0f;
    int blockSize = 128;

    DcBlocker dc;
    AdaaDriver driver;
    DriverBandpass driverBp;
    HoseWaveguide hose;
    VocalTract tract;
    LipMicStage lipMic;
    TalkboxSynth synth;
    LpcAnalyzer lpc;

    // Control-rate state
    int ctrlCounter = 0;
    int ctrlPeriod = 16;
    int lpcTickCounter = 0;
    int lpcTickPeriod = 12; // analyze every ~4 ms (hop), not every control tick
    Formants curFormants, padFormants, micFormants;
    bool micValid = false;
    float micGateEnv = 0.0f;
    float consonantEnv = 0.0f;
    float dryLpState = 0.0f;

    // Mic analysis ring (16k samples, preallocated)
    static constexpr int kMicRing = 1024;
    float micRing[kMicRing] = {}; int micHead = 0;
    float frame16k[LpcAnalyzer::kWin] = {};

    // Oversampling scratch for driver (Balanced 2x, Studio 4x) — preallocated
    static constexpr int kMaxOs = 4;
    float osBuf[kMaxOs] = {};

    // Visualizer taps (lock-free, GUI reads)
    float visF[5] = {500.f,1500.f,2500.f,3500.f,4500.f};
    float visMouth = 0.8f, visGate = 0.0f, visSib = 0.0f;

    void prepare(float sampleRate, int maxBlock) noexcept {
        fs = sampleRate; blockSize = maxBlock;
        dc.prepare(fs); driverBp.prepare(fs);
        hose.prepare(fs); tract.prepare(fs); lipMic.prepare(fs);
        synth.prepare(fs); lpc.prepare(fs);
        updateQuality();
        padFormants = padToFormants(p.padX, p.padY, p.vtl, p.lipRounding);
        curFormants = padFormants; micFormants = padFormants;
        tract.setTarget(curFormants, p.nasal, p.mouthOpening, p.glide);
        tract.updateCoeffs();
        reset();
    }
    void updateQuality() noexcept {
        switch (p.quality) {
            case QualityMode::Eco: ctrlPeriod=32; hose.physicalDelay=false; break;
            case QualityMode::Balanced: ctrlPeriod=16; hose.physicalDelay=false; break;
            case QualityMode::Studio: ctrlPeriod=16; hose.physicalDelay=true; break;
            case QualityMode::Creative: ctrlPeriod=16; hose.physicalDelay=true; break;
        }
        // LPC hop = 4 ms => hopSamp = Fs*0.004; ticks per hop = hopSamp/ctrlPeriod
        int hopSamp = (int)(fs*0.004f);
        lpcTickPeriod = std::max(1, hopSamp/std::max(1,ctrlPeriod));
        hose.lengthM = p.hoseLength; hose.damping = p.hoseDamping; hose.updateDelay();
        driverBp.hornCut = p.hornCut; driverBp.setCoeffs();
    }
    void reset() noexcept {
        dc.reset(); driver.reset(); driverBp.reset(); hose.reset();
        tract.reset(); lipMic.reset(); synth.reset(); lpc.reset();
        std::memset(micRing,0,sizeof(micRing)); micHead=0;
        ctrlCounter=0; micValid=false; consonantEnv=0.f;
    }

    // Control-rate tick (every ctrlPeriod samples): update formant targets.
    void controlTick() noexcept {
        // Pad formants from articulatory controls
        // Jaw/TongueHeight -> Y, TongueX -> X (blend with pad for automation)
        // The pad position IS the articulation. The tongue/jaw controls were
        // previously averaged in at a fixed 50 %, which pinned every vowel
        // halfway toward them and made the pad feel dead. They are now offsets
        // around their neutral 0.5, so moving the pad alone gives the pad vowel.
        float px = clampf(p.padX + (p.tongueX - 0.5f)*0.5f, 0.f, 1.f);
        float py = clampf(p.padY + ((0.5f*(p.jaw + p.tongueHeight)) - 0.5f)*0.5f, 0.f, 1.f);
        // mouthSize widens/narrows the talkbox aperture. It scales the effective
        // tract length, so it shifts all formants together -- distinct from vtl,
        // which models speaker size. 1.0 = neutral.
        float vtlEff = clampf(p.vtl, 0.5f, 2.0f) / clampf(p.mouthSize, 0.75f, 1.35f);
        padFormants = padToFormants(px, py, clampf(vtlEff,0.5f,2.0f), p.lipRounding);

        if (p.source == SourceMode::Mic) {
            // LPC analysis at hop rate (~4 ms), not every control tick (CPU).
            if (++lpcTickCounter >= lpcTickPeriod) {
                lpcTickCounter = 0;
                // Collect last kWin samples from ring (oldest->newest)
                for (int i=0;i<LpcAnalyzer::kWin;++i) {
                    int idx = (micHead - LpcAnalyzer::kWin + i + kMicRing*2) % kMicRing;
                    frame16k[i]=micRing[idx];
                }
                bool open = lpc.analyze(frame16k, LpcAnalyzer::kWin);
                micGateEnv += ((open?1.0f:0.0f)-micGateEnv)*0.3f;
                if (open) {
                    Formants mf;
                    if (lpc.kToFormants(lpc.outK, LpcAnalyzer::kAnalysisRate, mf)) {
                        // VTL warping in mic mode: same effective scale as the pad
                        float s = 1.0f/clampf(p.vtl/clampf(p.mouthSize,0.75f,1.35f),0.5f,2.0f);
                        for (int i=0;i<5;++i){ mf.f[i]*=s; mf.b[i]=clampf(mf.b[i],30.f,500.f); }
                        micFormants = mf;
                        micValid = true;
                    }
                } else {
                    // Gate closed: behavior
                    if (p.gateBehavior==GateBehavior::CloseMouth) {
                        // mouth closes: damp tract (handled via mouthOpening blend below)
                    } else if (p.gateBehavior==GateBehavior::HoldVowel) {
                        micValid = true; // keep last micFormants
                    } else { // ReturnToPad
                        micValid = false;
                    }
                }
                // Sibilance -> consonant env
                float target = lpc.sibilance * p.consonantCrisp;
                consonantEnv += (target-consonantEnv)*0.4f;
                visSib = lpc.sibilance; visGate = micGateEnv;
            }
        } else {
            visGate = 0.f; visSib = 0.f;
        }

        // Select active formants
        Formants active;
        float activeMouth = p.mouthOpening;
        float activeNasal = p.nasal;
        if (p.source==SourceMode::Mic && micValid) {
            active = micFormants;
            // Mouth opening from gate + LPC energy (closed when silent)
            float gateMouth = clampf(micGateEnv*1.2f, 0.05f, 1.0f);
            if (p.gateBehavior==GateBehavior::CloseMouth) activeMouth = gateMouth;
            else activeMouth = p.mouthOpening;
            activeNasal = p.nasal;
        } else if (p.source==SourceMode::Mic && !micValid && p.gateBehavior==GateBehavior::CloseMouth) {
            active = curFormants; // hold last, but mouth closed damps
            activeMouth = 0.05f;
        } else {
            active = padFormants;
        }
        curFormants = active;
        // articSmooth and glide both control articulation inertia: glide is the
        // fine control in the Vocal Tract tab, articSmooth the single macro.
        // Average them so both are always audible; either one alone still moves
        // the result by half its range.
        const float glideEff = clampf(0.5f*p.glide + 0.5f*p.articSmooth, 0.f, 1.f);
        tract.setTarget(active, activeNasal, activeMouth, glideEff);
        tract.updateCoeffs();
        lipMic.mouthOpening = activeMouth;
        lipMic.update();
        visMouth = activeMouth;
        for (int i=0;i<5;++i) visF[i]=active.f[i];
    }

    // Process one block. carrierL/R: external carrier (mono sum if stereo).
    // mic: sidechain/mic input (mono). Outputs stereo (dual-mono) via outL/outR.
    // All loops fixed-size, no alloc.
    void processBlock(const float* carrier, const float* mic, float* outL, float* outR, int n) noexcept {
        ScopedNoDenormals noDen;
        if (p.bypass) {
            for (int i=0;i<n;++i){ float c = carrier?carrier[i]:0.f; outL[i]=sanitize(c); outR[i]=sanitize(c); }
            return;
        }
        // Sync params to sub-blocks (cheap, control-rate-ish; called per block)
        driver.drive = 0.5f + p.carrierDrive*2.5f;
        // tubeCharacter blends the driver between symmetric (transistor, odd
        // harmonics) and asymmetric (tube, even harmonics). Added to, rather
        // than replacing, the explicit diaphragmAsym trim.
        driver.asymmetry = clampf(p.diaphragmAsym + (p.tubeCharacter-0.5f)*0.6f, -0.5f, 0.5f);
        driver.comp = p.driverComp;
        hose.mouthReflect = 0.05f + (1.0f-p.mouthOpening)*0.25f;
        lipMic.proxBoost = p.proxBoost; lipMic.micDistanceCm = p.micDistance;
        lipMic.micModel = p.micModel; lipMic.outGain = p.outputGain;
        lipMic.ambience = p.ambience;
        // micProximity is the single-knob form of the proximity effect: it
        // scales proxBoost and pushes the virtual mic closer as it rises.
        lipMic.proxBoost = clampf(0.5f*p.proxBoost + 0.5f*p.micProximity, 0.f, 1.f);
        lipMic.micDistanceCm = clampf(p.micDistance * (1.0f - 0.6f*p.micProximity), 1.f, 30.f);
        synth.glideTime = p.synthGlide; synth.vibratoDepth = p.synthVibrato;
        synth.octave = clampf(p.synthOctave, -2.f, 2.f);
        synth.setWaveform(p.synthWave);
        lpc.gateThresh = p.gateThresh; lpc.whisperBoost = p.whisperBoost; lpc.smooth = p.micSmooth;

        int osFactor = (p.quality==QualityMode::Studio||p.quality==QualityMode::Creative) ? 4 : (p.quality==QualityMode::Balanced ? 2 : 1);

        for (int i=0;i<n;++i) {
            float cIn = carrier ? carrier[i] : 0.0f;
            float mIn = mic ? mic[i] : 0.0f;

            // Mic decimation push (always, cheap)
            {
                float o16;
                if (lpc.push(mIn * p.micPreGain, o16)) {
                    micRing[micHead]=o16; micHead=(micHead+1)%kMicRing;
                }
            }

            // Carrier select
            float carrierSamp;
            if (p.carrier==CarrierMode::Internal) carrierSamp = synth.process();
            else carrierSamp = cIn * p.inputGain;
            carrierSamp = dc.process(carrierSamp);

            // Driver with localized oversampling (Balanced/Studio only)
            float drv;
            if (osFactor==1) {
                drv = driver.process(carrierSamp);
            } else {
                // Simple 2x/4x linear-interp oversample around ADAA only.
                // (Polyphase IIR in production; linear is sufficient for test
                //  and keeps Eco path zero-latency. Documented in DSP spec.)
                float prev = driver.xPrev; // approx previous input (pre-drive)
                // Upsample: interpolate between prev and current
                float acc=0.0f;
                for (int k=0;k<osFactor;++k) {
                    float frac = (float)(k+1)/(float)osFactor;
                    float xi = prev + (carrierSamp-prev)*frac;
                    // ADAA state advances per sub-sample (correct for OS)
                    acc += driver.process(xi);
                }
                drv = acc/(float)osFactor;
            }
            drv = driverBp.process(drv);

            // Hose
            float hosed = hose.process(drv, p.mouthOpening);

            // Vocal tract
            float tractOut = tract.process(hosed);

            // Lip/mic
            float wet = lipMic.process(tractOut);

            // Consonant bridge: HP mic transients blended when sibilant
            if (p.source==SourceMode::Mic && consonantEnv>0.001f) {
                // High-pass mic at ~3 kHz via one-pole (reuse lipMic radLp state? use simple diff)
                // De-harooed: remove low harmonics by HP; de-ess reduces harshness
                float hp = mIn - dryLpState;
                dryLpState += (mIn - dryLpState)*0.25f; // ~3kHz LP approx @48k
                float deess = 1.0f - p.sibilDeess*0.5f;
                wet += hp * consonantEnv * p.consonantCrisp * deess * 0.8f;
            } else {
                dryLpState += (mIn - dryLpState)*0.25f;
            }
            // Voice bleed (default 0%): blend raw mic (experimental Creative)
            if (p.voiceBleed>0.001f) wet += mIn * p.voiceBleed * 0.5f;

            // Dry/wet mix (dry = conditioned carrier, pre-tract)
            float dry = carrierSamp * 0.7f;
            float y = wet*p.dryWet + dry*(1.0f-p.dryWet);
            y = sanitize(y);
            outL[i]=y; outR[i]=y;

            // Control tick
            if (++ctrlCounter >= ctrlPeriod) { ctrlCounter=0; controlTick(); }
        }
    }

    // MIDI helpers (called from message thread, lock-free voice steal)
    void midiNoteOn(int note, float vel) noexcept { synth.noteOn((float)note, vel); }
    void midiNoteOff(int note) noexcept { synth.noteOff((float)note); }
};

} // namespace tb
