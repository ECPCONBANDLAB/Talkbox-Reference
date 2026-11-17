// TalkBox Reference — Mic LPC Tracker (Stage E) + Lip/Mic Stage (Stage F)
// Stage E: 16 kHz-decimated autocorrelation + Levinson-Durbin, zero-alloc.
// Whisper-compatible (spectral envelope, no pitch detection required).
#pragma once
#include "TbCore.h"
#include "TbVocalTract.h"
#include <array>

namespace tb {

// ---------------------------------------------------------------------------
// LPC Analyzer: decimate to ~16 kHz, Hann window, autocorrelation, Levinson.
// Frame: 20 ms window (320 @16k), 4 ms hop (64 @16k). Order P=12.
// Outputs reflection coefficients k[1..P] + gain + voiced/unvoiced + sibilance.
// ---------------------------------------------------------------------------
struct LpcAnalyzer {
    static constexpr int kOrder = 16;
    static constexpr int kWin = 400;   // 25 ms @16k (better low-formant resolution)
    static constexpr int kHop = 64;    // 4 ms @16k
    static constexpr float kAnalysisRate = 16000.0f;

    float winBuf[kWin] = {};
    float preEmphPrev = 0.0f;   // prev lowpassed sample @16k (legacy, unused with shelf)
    float lp800 = 0.0f;         // 800 Hz LP @16k for shelf pre-emphasis
    float lpAa = 0.0f;          // anti-alias lowpass state @FsIn
    float aaCoeff = 0.6f;       // 1-exp(-2pi*7000/Fs)
    float decBuf[64] = {}; int decIdx = 0; int decCount = 0; // decimator state
    float fsIn = 48000.0f;
    float decRatio = 3.0f; // Fs/16k (e.g. 48k/16k=3)

    // Levinson scratch (preallocated)
    float R[kOrder+1] = {};
    float a[kOrder+1] = {};   // LPC coeffs a[0]=1
    float k[kOrder+1] = {};   // reflection coeffs k[1..P]
    float tmp[kOrder+1] = {};

    float outK[kOrder+1] = {}; // smoothed LAR-domain output
    float outLar[kOrder+1] = {};
    bool initialized = false;
    float gateEnv = 0.0f;
    float gateThresh = 0.008f; // RMS threshold (calibratable)
    float whisperBoost = 1.0f;
    float smooth = 0.6f;       // ballistic smoothing 0..1

    // Sibilance / V-U detector state
    float lp1k = 0.0f, lp32 = 0.0f; // one-pole states @16k
    float hfEnv = 0.0f, lfEnv = 0.0f;
    float zcr = 0.0f;
    float sibilance = 0.0f; // 0..1
    float unvoiced = 0.0f;  // 0=voiced, 1=unvoiced
    float frameEnergy = 0.0f;

    void prepare(float sampleRate) noexcept {
        fsIn = sampleRate;
        decRatio = sampleRate / kAnalysisRate;
        aaCoeff = 1.0f - std::exp(-kTwoPi*7000.0f/sampleRate);
        decIdx = 0; decCount = 0; preEmphPrev = 0.0f; lpAa = 0.0f; decAcc = 0.0f; lp800 = 0.0f;
        std::memset(winBuf,0,sizeof(winBuf));
        std::memset(outK,0,sizeof(outK));
        initialized = false;
    }

    // Push one input sample (at fsIn). Returns true when a new 16k sample ready.
    // Anti-alias lowpass @FsIn -> fractional decimate -> shelf pre-emphasis @16k.
    // Shelf: y = x + (x - LP800(x)) => flat below 800 Hz (preserves F1),
    // +6 dB above (whitens for LPC). Matches spec "+6 dB/oct above ~800 Hz"
    // without suppressing F1 like differencing pre-emphasis does.
    inline bool push(float x, float& out16k) noexcept {
        lpAa += (x - lpAa)*aaCoeff;
        decAcc += 1.0f;
        if (decAcc >= decRatio) {
            decAcc -= decRatio;
            float dec = lpAa;
            lp800 += (dec - lp800)*0.27f; // 800 Hz LP @16k
            float y = dec + (dec - lp800)*1.0f;
            out16k = y * whisperBoost;
            return true;
        }
        return false;
    }
    float decAcc = 0.0f;

    // Analyze one 20 ms frame (winBuf filled with 16k samples, Hann applied inside).
    // Fills outK[1..P], sibilance, unvoiced, frameEnergy. Returns true if gate open.
    bool analyze(const float* frame16k, int n) noexcept {
        int N = std::min(n, kWin);
        // Hann window + copy
        float sumSq = 0.0f;
        int zc = 0; float prevS = 0.0f;
        for (int i=0;i<N;++i) {
            float w = 0.5f*(1.0f - std::cos(kTwoPi*(float)i/(float)(kWin-1)));
            float s = frame16k[i]*w;
            winBuf[i]=s;
            sumSq += s*s;
            if (i>0 && ((prevS>=0&&s<0)||(prevS<0&&s>=0))) ++zc;
            prevS=s;
        }
        for (int i=N;i<kWin;++i) winBuf[i]=0.0f;
        frameEnergy = std::sqrt(sumSq/(float)kWin + 1e-12f);
        zcr = (float)zc/(float)N;

        // HF/LF energy for sibilance: HF via first-difference (sharp HP,
        // peak at Nyquist), LF via 1 kHz one-pole. Frame-local for stability.
        float hf = 0.0f, lf = 0.0f;
        float l1 = 0.0f, prevS2 = 0.0f;
        for (int i=0;i<N;++i) {
            float s = frame16k[i];
            l1 += (s - l1)*0.35f;    // ~1 kHz LP @16k
            float h = (i>0) ? (s - prevS2) : 0.0f;
            prevS2 = s;
            hf += h*h; lf += l1*l1;
        }
        float hfRms = std::sqrt(hf/(float)N+1e-12f);
        float lfRms = std::sqrt(lf/(float)N+1e-12f);
        hfEnv += (hfRms-hfEnv)*0.5f;
        lfEnv += (lfRms-lfEnv)*0.5f;
        float ratio = hfEnv/(lfEnv+1e-6f);
        // Sibilance: noise ratio ~3.0, voiced ~0.06. Threshold 0.8.
        float sib = clampf((ratio-0.8f)/1.5f, 0.0f, 1.0f) * clampf((zcr-0.12f)/0.25f, 0.0f, 1.0f);
        sibilance += (sib - sibilance)*0.5f;
        unvoiced = clampf(sibilance*1.2f + (zcr>0.35f?0.3f:0.0f), 0.0f, 1.0f);

        // Gate envelope
        gateEnv += (frameEnergy - gateEnv)*0.4f;
        bool gateOpen = gateEnv > gateThresh;

        if (!gateOpen) return false;

        // Autocorrelation R[0..P]
        for (int lag=0; lag<=kOrder; ++lag) {
            float s=0.0f;
            for (int i=0;i<kWin-lag;++i) s += winBuf[i]*winBuf[i+lag];
            R[lag]=s;
        }
        // Regularization (white noise floor) for stability
        R[0] += 1e-6f * (R[0]+1.0f);
        // Levinson-Durbin
        a[0]=1.0f;
        float E = R[0];
        for (int i=1;i<=kOrder;++i) {
            float acc=0.0f;
            for (int j=1;j<i;++j) acc += a[j]*R[i-j];
            float ki = -(R[i]+acc)/(E+1e-12f);
            ki = clampf(ki, -kMaxK, kMaxK);
            k[i]=ki;
            tmp[0]=1.0f;
            for (int j=1;j<i;++j) tmp[j]=a[j]+ki*a[i-j];
            tmp[i]=ki;
            for (int j=0;j<=i;++j) a[j]=tmp[j];
            E *= (1.0f - ki*ki);
            if (E < 1e-12f) E = 1e-12f;
        }
        // LAR smoothing (ballistic): convert to LAR, smooth, back
        float sm = clampf(smooth, 0.0f, 0.95f);
        for (int i=1;i<=kOrder;++i) {
            float lar = kToLar(k[i]);
            if (!initialized) { outLar[i]=lar; outK[i]=k[i]; }
            else {
                outLar[i] += (lar - outLar[i])*(1.0f-sm*0.7f);
                outK[i] = larToK(outLar[i]);
            }
        }
        initialized = true;
        return true;
    }

    // Convert LPC k's to formants via pole extraction (Durand-Kerner, double
    // precision, safeguarded) with spectral-peak fallback. Poles are robust
    // to pre-emphasis tilt (unlike raw spectrum peaks). Fills f[0..4], b[0..4].
    bool kToFormants(const float* kk, float fsAnalysis, Formants& out) noexcept {
        // Build A(z) = 1 + sum a_i z^-i from k's via Levinson reconstruction
        double aa[kOrder+1]={}; aa[0]=1.0;
        for (int i=1;i<=kOrder;++i) {
            double t[kOrder+1]={};
            t[0]=1.0;
            for (int j=1;j<i;++j) t[j]=aa[j]+(double)kk[i]*aa[i-j];
            t[i]=(double)kk[i];
            for (int j=0;j<=i;++j) aa[j]=t[j];
        }
        const int P = kOrder;
        double re[P], im[P], nre[P], nim[P];
        for (int i=0;i<P;++i) {
            double ang = 2.0*3.141592653589793*(double)i/(double)P + 0.4;
            re[i]=std::cos(ang)*0.95; im[i]=std::sin(ang)*0.95;
        }
        bool diverged=false;
        for (int iter=0; iter<80 && !diverged; ++iter) {
            double maxDelta=0.0;
            for (int i=0;i<P;++i) {
                // p(z_i) via Horner (double)
                double pr=1.0, pi=0.0;
                for (int j=1;j<=P;++j) {
                    double npr = pr*re[i] - pi*im[i] + aa[j];
                    double npi = pr*im[i] + pi*re[i];
                    pr=npr; pi=npi;
                }
                if (!(pr==pr && pi==pi) || std::fabs(pr)>1e6 || std::fabs(pi)>1e6) { diverged=true; break; }
                double dr=1.0, di=0.0;
                for (int j=0;j<P;++j) if (j!=i) {
                    double ddr=re[i]-re[j], ddi=im[i]-im[j];
                    double ndr=dr*ddr-di*ddi, ndi=dr*ddi+di*ddr;
                    dr=ndr; di=ndi;
                }
                double denom = dr*dr+di*di;
                if (denom<1e-18) { diverged=true; break; }
                double ddr=(pr*dr+pi*di)/denom, ddi=(pi*dr-pr*di)/denom;
                nre[i]=re[i]-ddr; nim[i]=im[i]-ddi;
                double d=std::sqrt(ddr*ddr+ddi*ddi);
                if (d>maxDelta) maxDelta=d;
                if (!(nre[i]==nre[i] && nim[i]==nim[i])) { diverged=true; break; }
            }
            if (diverged) break;
            for (int i=0;i<P;++i){re[i]=nre[i]; im[i]=nim[i];}
            if (maxDelta<1e-9) break;
        }
        if (!diverged) {
            struct Pole{float f,b;};
            Pole poles[6]; int np=0;
            for (int i=0;i<P;++i) {
                if (im[i]<=0.005) continue;
                double r=std::sqrt(re[i]*re[i]+im[i]*im[i]);
                if (!(r==r) || r>=1.0 || r<0.5) continue;
                double theta=std::atan2(im[i],re[i]);
                double f=theta/(2.0*3.141592653589793)*(double)fsAnalysis;
                double b=-std::log(r)/(3.141592653589793)*(double)fsAnalysis;
                if (f<80.0||f>7000.0) continue;
                if (np<6){poles[np].f=(float)f; poles[np].b=clampf((float)b,30.f,500.f); ++np;}
            }
            if (np>=2) {
                for (int i=0;i<np;++i) for (int j=i+1;j<np;++j) if (poles[j].f<poles[i].f){Pole t=poles[i];poles[i]=poles[j];poles[j]=t;}
                for (int i=0;i<5;++i) {
                    if (i<np){out.f[i]=poles[i].f; out.b[i]=poles[i].b;}
                    else {out.f[i]=500.f+(float)i*1000.f; out.b[i]=100.f+(float)i*50.f;}
                }
                for (int i=1;i<5;++i) if (out.f[i]<=out.f[i-1]) out.f[i]=out.f[i-1]+200.f;
                return true;
            }
        }
        // Fallback: spectral peak picking on |1/A(e^jw)|
        {
            const int NF = 200;
            float mag[NF]; float freqs[NF];
            for (int bi=0; bi<NF; ++bi) {
                float f = 80.0f + (7000.0f-80.0f)*(float)bi/(float)(NF-1);
                freqs[bi]=f;
                float w = kTwoPi*f/fsAnalysis;
                float cw = std::cos(w), sw = std::sin(w);
                float pr=1.0f, pi=0.0f;
                for (int i=1;i<=kOrder;++i) {
                    float npr = pr*cw + pi*sw + (float)aa[i];
                    float npi = -pr*sw + pi*cw;
                    pr=npr; pi=npi;
                }
                float m = std::sqrt(pr*pr+pi*pi)+1e-12f;
                mag[bi]=1.0f/m;
            }
            struct Cand{float f,m;};
            Cand cands[NF]; int nc=0;
            for (int i=1;i<NF-1;++i) if (mag[i]>mag[i-1]&&mag[i]>mag[i+1]) { cands[nc].f=freqs[i]; cands[nc].m=mag[i]; ++nc; }
            if (nc<2) return false;
            for (int i=0;i<nc;++i) for (int j=i+1;j<nc;++j) if (cands[j].m>cands[i].m){Cand t=cands[i];cands[i]=cands[j];cands[j]=t;}
            float picked[5]={0,0,0,0,0}; int np=0;
            for (int i=0;i<nc && np<5; ++i) {
                bool far=true;
                for (int j=0;j<np;++j) if (std::fabs(cands[i].f-picked[j])<150.0f) far=false;
                if (far) picked[np++]=cands[i].f;
            }
            if (np<2) return false;
            for (int i=0;i<np;++i) for (int j=i+1;j<np;++j) if (picked[j]<picked[i]){float t=picked[i];picked[i]=picked[j];picked[j]=t;}
            for (int i=0;i<5;++i) {
                if (i<np){ out.f[i]=picked[i]; out.b[i]=70.0f+(float)i*40.0f; }
                else { out.f[i]=500.0f+(float)i*1000.0f; out.b[i]=100.0f+(float)i*50.0f; }
            }
            for (int i=1;i<5;++i) if (out.f[i]<=out.f[i-1]) out.f[i]=out.f[i-1]+200.f;
            return true;
        }
    }

    void reset() noexcept {
        std::memset(winBuf,0,sizeof(winBuf));
        std::memset(outK,0,sizeof(outK));
        initialized=false; gateEnv=0.f; sibilance=0.f; unvoiced=0.f;
        preEmphPrev=0.f; decAcc=0.f; lp1k=lp32=0.f; hfEnv=lfEnv=0.f; lp800=0.f; lpAa=0.f;
    }
};

// ---------------------------------------------------------------------------
// Lip radiation + close-talk mic (Stage F)
// Radiation: +6 dB/oct shelf, cutoff scales with mouth opening.
// Mic models: 57/58 dynamic, studio condenser, ribbon, flat.
// Proximity boost, presence, saturation, ceiling limiter.
// ---------------------------------------------------------------------------
struct LipMicStage {
    TptSvFilter::OnePole radLp; // for radiation shelf
    TptSvFilter::OnePole proxLp;
    TptSvFilter presPeak;
    float fs=48000.f;
    float mouthOpening=0.8f;
    float proxBoost=0.5f; // 0..1
    float micDistanceCm=3.0f; // 1..30
    int micModel=0; // 0=57/58,1=condenser,2=ribbon,3=flat
    float outGain=1.0f;
    float ceiling=0.89f; // -1 dBFS

    void prepare(float sampleRate) noexcept { fs=sampleRate; update(); }
    void update() noexcept {
        float radFc = 400.0f + mouthOpening*2000.0f;
        radLp.setCutoff(clampf(radFc,200.f,0.45f*fs), fs);
        proxLp.setCutoff(180.0f, fs);
        float presF = (micModel==1)?4200.f:(micModel==2?2800.f:3800.f);
        float presQ = (micModel==3)?0.7f:1.2f;
        presPeak.setFreq(clampf(presF,500.f,0.45f*fs), presQ, fs);
    }
    inline float process(float x) noexcept {
        // Lip radiation: y = x + g*(x - LP(x))  (+6dB/oct tilt)
        float lp = radLp.lowpass(x);
        float radGain = 0.4f + mouthOpening*0.6f;
        float y = x + (x-lp)*radGain*0.7f;
        // Proximity effect: low-mid warmth boost, stronger when close
        float proxAmt = proxBoost * clampf(5.0f/(micDistanceCm+1.0f),0.0f,1.0f);
        float plp = proxLp.lowpass(y);
        y += plp * proxAmt * 0.8f;
        // Presence peak (bandpass add)
        if (micModel!=3) {
            float bp = presPeak.bandpass(y);
            y += bp * 0.25f;
        }
        // Mic character EQ (simple shelves via one-poles)
        if (micModel==2) { // ribbon: dark
            y = y*0.7f + proxLp.lowpass(y)*0.3f;
        } else if (micModel==1) { // condenser: bright
            y += (y - radLp.lowpass(y))*0.15f;
        }
        // Preamp saturation (soft)
        y = std::tanh(y*1.2f)/1.2f;
        y *= outGain;
        // Ceiling limiter (soft clip to ceiling)
        if (y>ceiling) y = ceiling + (y-ceiling)*0.1f;
        if (y<-ceiling) y = -ceiling + (y+ceiling)*0.1f;
        return sanitize(y);
    }
    void reset() noexcept { radLp.reset(); proxLp.reset(); presPeak.reset(); }
};

} // namespace tb
