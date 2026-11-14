// early_delay_dsp.h - the audio engine of Early Delay (Flat Peach).
//
// Copyright (c) 2026 Flat Peach. All rights reserved. Original code; uses only the
// MIT-licensed MPC Plugin SDK dsp helpers (see docs/LICENSES.md).
//
// This header has no dependency on the plugin framework, so the same engine is compiled into
// the plugin and into the offline measurement / demo renderer (tests/offline/).
//
// ---------------------------------------------------------------------------------------------
// MODEL: an early-1980s style digital delay with a variable sample clock.
//
//   in -> drive + op-amp soft clip -> (+ feedback) -> pre-emphasis -> 2:1 compressor
//      -> 4-pole anti-alias low-pass (tracks the clock) -> S/H + ~12-bit ADC (+ hiss)
//      -> CELL MEMORY written at the clock rate fc
//      -> DAC (zero-order hold, blended with first-order hold when Grit is low)
//      -> 4-pole reconstruction low-pass (tracks the clock) -> 1:2 expander -> de-emphasis
//      -> output op-amp soft clip -> Tone low-pass -> WET
//   WET -> low cut -> * feedback (-0.99..+0.99) -> tanh saturation -> back to the loop input
//
// Variable clock: the memory has a fixed number of cells N (set by Bandwidth, 2048..32768).
// The clock is fc = N / T, capped at the host rate (44.1 kHz). Above the cap (short times)
// the clock stays at 44.1 kHz and the used cell count shrinks to T * fs instead; this is the
// range where the wet bandwidth is set by the ~8 kHz filters. Above the knee T = N / fs the
// clock drops as the time grows, so long delays get darker and alias more (the filters track
// the clock only partially, as on the hardware). Because the cells hold samples taken at the
// OLD clock rate, moving Time re-pitches everything already in memory (true variable-clock
// behaviour, not just Doppler while the knob moves).
//
// HOLD: captures the current cell span without writing it. A bounded (1/8 tap) interpolator
// reads that immutable span at N/T, including more than one cell per host sample. Ping-pong
// alternates frozen L/R banks each turn. New input and feedback do not alter held cells;
// signed feedback polarity is already captured in their contents. Audio memory is not saved.
// High-ratio playback is intentionally lo-fi; the small aperture is not an ideal antialias filter.
//
// Compander: the 2:1 compressor's gain is stored with every memory cell and the 1:2 expander
// applies its inverse when the cell is read, through a control path that lags with Grit
// (imperfect tracking). The tracking error is clamped to be a loss only. A detector-based
// expander was tried first and measured unstable: its transient overshoot regenerated in the
// feedback loop into a sustained limit cycle at high feedback.
//
// Bounded converters and limited signed feedback constrain levels. Decay/stability are
// measured under the regression suite, not a proof for every time-varying signal/control.
// Stereoize uses shared clocks and cross-feedback, never opposing time offsets or side EQ.
// ---------------------------------------------------------------------------------------------

#ifndef FLATPEACH_EARLY_DELAY_DSP_H
#define FLATPEACH_EARLY_DELAY_DSP_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "dsp/math.h"
#include "dsp/noise.h"

namespace edly {

using mpc::dsp::clamp;
using mpc::dsp::fastTanh;
using mpc::dsp::kPi;
using mpc::dsp::Xorshift32;

// ---------------------------------------------------------------------------------------------
// Small building blocks
// ---------------------------------------------------------------------------------------------

// Two-pole low-pass section (trapezoidal SVF) with explicit Q. Unity DC gain.
struct LpSection
{
    float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, s1 = 0.0f, s2 = 0.0f;
    void set(float hz, float q, float sr)
    {
        hz = clamp(hz, 20.0f, 0.45f * sr);
        const float g = std::tan(kPi * hz / sr);
        const float k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { s1 = s2 = 0.0f; }
    inline float process(float x)
    {
        const float v3 = x - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        return v2;
    }
};

// 4-pole Butterworth low-pass (two sections); frequency response is not a peak-gain proof.
struct Lp4
{
    LpSection a, b;
    float lastHz = -1.0f;
    void set(float hz, float sr)
    {
        if (std::fabs(hz - lastHz) < 0.004f * hz) return;   // skip tiny changes (saves tan())
        lastHz = hz;
        a.set(hz, 0.5412f, sr);
        b.set(hz, 1.3066f, sr);
    }
    void reset() { a.reset(); b.reset(); }
    inline float process(float x) { return b.process(a.process(x)); }
};

// First-order shelf from an analog zero/pole pair, unity gain at DC: H(s) = (wp/wz)(s+wz)/(s+wp).
// Pre-emphasis uses (wz < wp); de-emphasis swaps them, which is the exact inverse after the
// bilinear transform.
struct Shelf1
{
    float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    void set(float zeroHz, float poleHz, float sr)
    {
        const float c = 2.0f * sr;
        const float wz = 2.0f * kPi * zeroHz, wp = 2.0f * kPi * poleHz;
        const float g = wp / wz;
        const float a0 = c + wp;
        b0 = g * (c + wz) / a0;
        b1 = g * (wz - c) / a0;
        a1 = (wp - c) / a0;
    }
    void reset() { x1 = y1 = 0.0f; }
    inline float process(float x)
    {
        const float y = b0 * x + b1 * x1 - a1 * y1;
        x1 = x; y1 = y;
        return y;
    }
};

struct OnePole
{
    float c = 1.0f, z = 0.0f;
    void setHz(float hz, float sr) { c = 1.0f - std::exp(-2.0f * kPi * hz / sr); }
    void setMs(float ms, float sr) { c = 1.0f - std::exp(-1000.0f / (ms * sr)); }
    void reset(float v = 0.0f) { z = v; }
    inline float lp(float x) { z += c * (x - z); return z; }
    inline float hp(float x) { z += c * (x - z); return x - z; }
};

// Asymmetric op-amp style soft clip: slight bias gives a touch of even harmonics; slope 1 at 0.
inline float opampClip(float x, float rail)
{
    constexpr float kBias = 0.12f;
    constexpr float kOff = 0.1194273f;            // tanh(kBias)
    const float y = fastTanh(x / rail + kBias) - kOff;
    return y * rail / (1.0f - kOff * kOff);   // d/dx at 0 == 1 (tanh' = 1 - tanh^2)
}

// Analog-style LFO: rounded triangle whose rate and amplitude wander slowly and randomly.
struct DriftLfo
{
    float phase = 0.0f;
    float drift = 0.0f, driftTarget = 0.0f;
    float amp = 1.0f, ampTarget = 1.0f;
    int countdown = 0;
    Xorshift32 rng{0x1F2E3D4Cu};

    void reset(uint32_t seed, float startPhase)
    {
        rng.reseed(seed); phase = startPhase; drift = driftTarget = 0.0f; amp = ampTarget = 1.0f; countdown = 0;
    }
    // Advances one sample; returns -1..1. `rateHz` is the nominal rate.
    inline float next(float rateHz, float invSr, float wander)
    {
        if (--countdown <= 0)
        {   // new random targets roughly every 0.35 s
            countdown = 15000 + (int)(rng.next() & 0x1FFF);
            driftTarget = rng.nextBipolar() * 0.12f * wander;
            ampTarget = 1.0f + rng.nextBipolar() * 0.10f * wander;
        }
        drift += 0.00008f * (driftTarget - drift);
        amp += 0.00008f * (ampTarget - amp);
        phase += rateHz * (1.0f + drift) * invSr;
        if (phase >= 1.0f) phase -= 1.0f;
        return shape(phase) * amp;
    }
    static inline float shape(float p)
    {
        const float tri = 1.0f - 4.0f * std::fabs(p - 0.5f);   // -1..1 triangle
        return tri * (1.5f - 0.5f * tri * tri);                 // rounded (sine-like) triangle
    }
};

// ---------------------------------------------------------------------------------------------
// Parameters as the engine sees them (plain values)
// ---------------------------------------------------------------------------------------------
struct EngineParams
{
    float timeMs = 300.0f;     // 1.75 .. 900
    float feedback = 0.35f;    // -1 .. +1 (knob -100..100 %)
    float modSpeedHz = 0.6f;   // 0.05 .. 10
    float modDepth = 0.15f;    // 0 .. 1
    float tone = 0.6f;         // 0 .. 1
    float grit = 0.35f;        // 0 .. 1
    float bandwidth = 0.6f;    // 0 .. 1 (0 = aged / narrow, 1 = fresh / wide)
    float drive = 0.2f;        // 0 .. 1
    float mix = 0.35f;         // 0 .. 1
    bool  hold = false;
    bool  stereoize = false;
    float width = 0.7f;        // 0 .. 1 (ping-pong wet pan separation)
};

// Derived mapping helpers (shared with the measurement tools and README).
inline float bandwidthCutHz(float bw) { return 4500.0f * std::pow(2.0f, 1.4f * bw); }   // 4.5k..11.9k, 8k at 0.6
inline float memoryCells(float bw)    { return 2048.0f * std::pow(2.0f, 4.0f * bw); }   // 2048..32768
inline float gritBits(float grit)     { return 14.0f - 6.0f * grit; }                    // 14..8, ~12 at 0.35
inline float toneCutHz(float tone)    { return 700.0f * std::pow(2.0f, 4.6f * tone); }   // 700 Hz..17 kHz

// ---------------------------------------------------------------------------------------------
// One delay channel (memory + converters + compander + loop)
// ---------------------------------------------------------------------------------------------
class Channel
{
public:
    static constexpr int kCells = 65536;            // power of two >= 32768 + hold splice margin
    static constexpr int kMask = kCells - 1;

    void allocate() { cells_.assign((size_t)kCells, 0.0f); gains_.assign((size_t)kCells, 1.0f); }

    void setSampleRate(float sr)
    {
        sr_ = sr;
        preEmph_.set(1500.0f, 4500.0f, sr);    // +9.5 dB shelf on the way in
        deEmph_.set(4500.0f, 1500.0f, sr);     // exact inverse on the way out
        fbLowCut_.setHz(55.0f, sr);
        compAtt_ = onePoleMs(4.0f); compRel_ = onePoleMs(40.0f);
        aa_.lastHz = rec_.lastHz = toneLastHz_ = -1.0f;
    }

    void reset()
    {
        if (!cells_.empty()) std::memset(cells_.data(), 0, sizeof(float) * cells_.size());
        for (float& g : gains_) g = 1.0f;
        w_ = 0; phase_ = 0.0f; dacPrev_ = dacCur_ = 0.0f; adcPrevIn_ = 0.0f;
        envC_ = 1e-4f; gRead_ = 1.0f; gExp_ = 1.0f;
        aa_.reset(); rec_.reset(); tone_.reset(); preEmph_.reset(); deEmph_.reset();
        fbLowCut_.reset();
        lastWet_ = 0.0f; lOffset_ = 0.0f; holdLen_ = 0; lCur_ = 0.0f;
        fcRatio_ = 1.0f; heldPos_ = 0.0f; holdStart_ = 0; holdAlternate_ = false;
        releaseMix_ = heldLast_ = 0.0f;
        aa_.lastHz = rec_.lastHz = toneLastHz_ = -1.0f;
    }

    // Per control-rate update (every kCtrl samples): filter coefficients that depend on the clock.
    void updateFilters(float fcHz, float bwCut, float bwScale, float toneHz)
    {
        // Filters track the clock only partially: 0.42*fc puts the edge a bit below Nyquist,
        // so at low clock rates the 4-pole skirts let some aliasing / imaging through.
        // 1.12: two cascaded 4-pole Butterworths are -3 dB at 0.896 x their cutoff, so the
        // nominal Bandwidth figure is the -3 dB point of the whole wet path.
        const float cut = std::fmin(1.12f * bwCut * bwScale, 0.42f * fcHz);
        aa_.set(cut, sr_);
        rec_.set(cut * 1.03f, sr_);
        if (std::fabs(toneHz - toneLastHz_) > 0.004f * toneHz)
        {
            toneLastHz_ = toneHz;
            tone_.set(toneHz, 0.62f, sr_);
        }
    }

    // Per-sample settings computed by the engine.
    struct Tick
    {
        float teffSamples;   // effective delay in host samples (time + modulation + jitter)
        float cellsN;        // memory size in cells (Bandwidth)
        float fb;            // feedback gain incl. sign
        float bitsStep;      // quantiser step (2^(1-bits))
        float zoh;           // 1 = pure zero-order hold, 0 = first-order hold
        float hiss;          // noise before the ADC (linear)
        float noise;         // shared noise sample -1..1 for hiss
        float dither;        // shared noise sample -1..1 for converter dither
        float trackLoss;     // static expander gain error (< 1, never a gain)
        float expLag;        // expander smoothing coefficient (mistracking grows with Grit)
        bool  hold;
        float feedbackSample; // previous-frame self/cross feedback, sampled symmetrically
        const Channel* peer;
        bool pingpong;
    };

    // Processes one sample of input (already driven). Returns the wet signal.
    inline float process(float in, const Tick& t)
    {
        // ---------------- clock & cell count -------------------------------------------------
        float lTarget;
        if (t.hold)
        {
            if (holdLen_ == 0) engageHold();
            lTarget = (float)holdLen_;
        }
        else
        {
            if (holdLen_ != 0) releaseHold(t);
            lTarget = std::fmin(t.cellsN, t.teffSamples);
        }
        lOffset_ *= 0.9985f;                                   // ~15 ms glide after jumps
        if (std::fabs(lOffset_) < 1e-3f) lOffset_ = 0.0f;
        lCur_ = t.hold ? (float)holdLen_ : lTarget + lOffset_;
        float inc = lCur_ / t.teffSamples;                     // clock / host rate
        if (!t.hold && inc > 0.9999f) inc = 1.0f;
        inc = std::fmin(inc, 1024.0f); // bounded resampling ratio, not an unbounded tick loop
        if (inc < 0.002f) inc = 0.002f;
        fcRatio_ = inc;

        // ---------------- write side: loop sum -> emphasis -> compressor -> AA -------------
        const float fbSig = fastTanh(fbLowCut_.hp(t.feedbackSample) * t.fb);
        float s = in + fbSig;
        s = preEmph_.process(s);
        const float rect = std::fabs(s);
        envC_ += (rect > envC_ ? compAtt_ : compRel_) * (rect - envC_);
        const float gC = std::sqrt(kCompRef / std::fmax(envC_, kEnvFloor));
        s *= gC;
        s += t.hiss * t.noise;                                 // analog hiss ahead of the ADC
        const float aaOut = aa_.process(s);

        // ---------------- clock tick: sample, quantise, write, read --------------------------
        if (t.hold)
        {
            // Immutable captured ring. Constant-cost interpolation supports pitch-up even
            // above one cell/host sample. Eight aperture taps soften images at high ratios;
            // this is deliberately gritty, not an ideal bandlimited resampler.
            float value = 0.0f, gain = 0.0f;
            const int taps = inc > 1.0f ? 8 : 1;
            for (int tap = 0; tap < taps; ++tap)
            {
                float pos = heldPos_ + inc * (float)tap / (float)taps;
                const int turns = (int)(pos / (float)holdLen_);
                pos -= (float)(turns * holdLen_);
                const bool alternate = holdAlternate_ != ((turns & 1) != 0);
                const Channel* source = t.pingpong && alternate ? t.peer : this;
                // Both channels share the clock and capture on this frame. When processing
                // the left channel first the peer's current write head is still its start.
                const int len = source->holdLen_ ? source->holdLen_ : holdLen_;
                const int start = source->holdLen_ ? source->holdStart_ : (source->w_ - len) & kMask;
                const float scaled = pos * (float)len / (float)holdLen_;
                const int ix = (int)scaled;
                const float frac = scaled - (float)ix;
                const int i0 = (start + ix) & kMask;
                const int i1 = (start + (ix + 1) % len) & kMask;
                value += source->cells_[i0] + (source->cells_[i1] - source->cells_[i0]) * frac;
                gain += source->gains_[i0] + (source->gains_[i1] - source->gains_[i0]) * frac;
            }
            dacPrev_ = dacCur_;
            dacCur_ = value / (float)taps;
            gRead_ = gain / (float)taps;
            heldPos_ += inc;
            const int turns = (int)(heldPos_ / (float)holdLen_);
            heldPos_ -= (float)(turns * holdLen_);
            holdAlternate_ = holdAlternate_ != ((turns & 1) != 0);
        }
        else
        {
            phase_ += inc;
            if (phase_ >= 1.0f)
            {
                phase_ -= 1.0f;
                const float frac = phase_ / inc;
                const float interp = aaOut + (adcPrevIn_ - aaOut) * frac;
                const float sh = interp + (aaOut - interp) * t.zoh;
                const float q = quantise(sh, t.bitsStep, t.dither);
                const float rp = (float)w_ - lCur_;
                const float rpf = std::floor(rp);
                const int i0 = (int)rpf & kMask;
                const float fr = rp - rpf;
                const size_t i1 = (size_t)((i0 + 1) & kMask);
                const float readv = cells_[i0] + (cells_[i1] - cells_[i0]) * fr;
                const float readg = gains_[i0] + (gains_[i1] - gains_[i0]) * fr;
                cells_[w_] = q; gains_[w_] = gC;
                w_ = (w_ + 1) & kMask;
                dacPrev_ = dacCur_; dacCur_ = readv; gRead_ = readg;
            }
        }
        adcPrevIn_ = aaOut;

        // ---------------- read side: DAC hold -> recon -> expander -> de-emphasis ------------
        float y = dacCur_;
        if (!t.hold && inc < 1.0f)
        {   // between ticks the DAC holds (ZOH); low Grit blends in a first-order hold
            const float ph = std::fmin(1.0f, phase_ / inc);
            const float foh = dacPrev_ + (dacCur_ - dacPrev_) * ph;
            y = foh + (dacCur_ - foh) * t.zoh;
        }
        y = rec_.process(y);
        // Expander (1:2): the inverse of the gain the compressor used for these cells. Its
        // control voltage lags (more with Grit), which is the imperfect tracking: onsets come
        // out softened and the noise floor swells and breathes behind the signal. The error is
        // limited to a LOSS (ratio <= 1), so the compander can never add gain inside the loop.
        const float gTarget = 1.0f / gRead_;
        gExp_ += t.expLag * (gTarget - gExp_);
        const float ratio = clamp(gExp_ * gRead_, 0.5f, 1.0f);
        y *= ratio * gTarget * t.trackLoss;
        y = deEmph_.process(y);
        y = opampClip(y, 1.6f);
        y = tone_.process(y);
        if (!(std::fabs(y) < 8.0f))
        {   // Repair recursive state, not merely its visible output. No memory scan/clear.
            aa_.reset(); rec_.reset(); tone_.reset(); preEmph_.reset(); deEmph_.reset();
            fbLowCut_.reset(); envC_ = kEnvFloor; gExp_ = gRead_ = 1.0f;
            adcPrevIn_ = dacPrev_ = dacCur_ = 0.0f; y = 0.0f;
        }
        if (t.hold) heldLast_ = y;
        else if (releaseMix_ > 0.0f)
        {
            y += (heldLast_ - y) * releaseMix_;
            releaseMix_ = std::fmax(0.0f, releaseMix_ - 1.0f / (0.002f * sr_));
        }
        if (std::fabs(y) < 1.0e-12f) y = 0.0f;                // no endless sub-audible tails
        lastWet_ = y;
        return y;
    }

    float wet() const { return lastWet_; }

    float clockRatio() const { return fcRatio_; }
    float lCur() const { return lCur_; }
    float onePoleMs(float ms) const { return 1.0f - std::exp(-1000.0f / (ms * sr_)); }

private:
    static constexpr float kCompRef = 0.25f;     // compander reference level (rectified average)
    static constexpr float kEnvFloor = 1.0e-4f;  // max compressor gain sqrt(0.25/1e-4) = +34 dB

    static inline float quantise(float x, float step, float dither)
    {
        x += dither * step * 0.35f;                            // a little converter noise, not full TPDF
        x = clamp(x, -1.0f, 1.0f - step);                      // ADC range: hard clip
        return std::floor(x / step + 0.5f) * step;
    }

    void engageHold()
    {
        int l = (int)std::lround(lCur_);
        if (l < 16) l = 16;
        if (l > kCells - 512) l = kCells - 512;
        holdLen_ = l;
        lOffset_ = 0.0f;
        holdStart_ = (w_ - holdLen_) & kMask;
        heldPos_ = 0.0f; holdAlternate_ = false;

    }
    void releaseHold(const Tick& t)
    {
        // continue from the held loop length and glide to the requested one
        const float target = std::fmin(t.cellsN, t.teffSamples);
        lOffset_ = (float)holdLen_ - target;
        holdLen_ = 0; phase_ = 0.0f; releaseMix_ = 1.0f;
    }

    std::vector<float> cells_;
    std::vector<float> gains_;     // compressor gain stored with every cell (expander control)
    int w_ = 0;
    float sr_ = 44100.0f;
    float phase_ = 0.0f, fcRatio_ = 1.0f;
    float dacPrev_ = 0.0f, dacCur_ = 0.0f, adcPrevIn_ = 0.0f;
    float envC_ = 1e-4f, compAtt_ = 0.0f, compRel_ = 0.0f;
    float gRead_ = 1.0f, gExp_ = 1.0f;
    float lastWet_ = 0.0f;
    float lCur_ = 0.0f, lOffset_ = 0.0f;
    float heldPos_ = 0.0f, releaseMix_ = 0.0f, heldLast_ = 0.0f;
    int holdStart_ = 0;
    bool holdAlternate_ = false;
    int holdLen_ = 0;
    Lp4 aa_, rec_;
    LpSection tone_;
    float toneLastHz_ = -1.0f;
    Shelf1 preEmph_, deEmph_;
    OnePole fbLowCut_;
};

// ---------------------------------------------------------------------------------------------
// The stereo engine
// ---------------------------------------------------------------------------------------------
class Engine
{
public:
    static constexpr int kCtrl = 16;   // control-rate interval (samples)
    static constexpr float kLatency = 8.0f;

    void prepare(float sr)
    {
        sr_ = sr; invSr_ = 1.0f / sr;
        ch_[0].allocate(); ch_[1].allocate();
        ch_[0].setSampleRate(sr); ch_[1].setSampleRate(sr);
        timeGlide_ = 1.0f - std::exp(-1000.0f / (70.0f * sr));    // the Time "pot" lag, ~70 ms
        paramGlide_ = 1.0f - std::exp(-1000.0f / (25.0f * sr));
        jitterLp_.setHz(7.0f, sr);
        jitterLpR_.setHz(7.0f, sr);
        // white noise in [-1,1) has rms 0.577; a one-pole with coefficient c keeps c/(2-c) of
        // its power. Scale so the low-passed jitter has unit rms.
        jitterNorm_ = 1.0f / (0.57735f * std::sqrt(jitterLp_.c / (2.0f - jitterLp_.c)));
        prepared_ = true;
        reset();
    }

    void reset()
    {
        ch_[0].reset(); ch_[1].reset();
        lfoL_.reset(0xA5A5F00Du, 0.0f);
        lfoR_.reset(0x5EED1234u, 0.5f);
        rng_.reseed(0x13572468u); rngR_.reseed(0x24681357u);
        jitterLp_.reset(); jitterLpR_.reset();
        ctrl_ = 0; driveG_ = driveComp_ = 1.0f; cellsCtrl_ = memoryCells(0.6f);
        snap_ = true;
    }

    void setParams(const EngineParams& p)
    {
        const EngineParams def;
        auto clean = [](float x, float lo, float hi, float fallback) {
            return std::isfinite(x) ? clamp(x, lo, hi) : fallback;
        };
        p_ = p;
        p_.timeMs = clean(p.timeMs, 1.75f, 900.0f, def.timeMs);
        p_.feedback = clean(p.feedback, -1.0f, 1.0f, def.feedback);
        p_.modSpeedHz = clean(p.modSpeedHz, 0.05f, 10.0f, def.modSpeedHz);
#define EDLY_CLEAN(field) p_.field = clean(p.field, 0.0f, 1.0f, def.field)
        EDLY_CLEAN(modDepth); EDLY_CLEAN(tone); EDLY_CLEAN(grit); EDLY_CLEAN(bandwidth);
        EDLY_CLEAN(drive); EDLY_CLEAN(mix); EDLY_CLEAN(width);
#undef EDLY_CLEAN
    }

    // In-place safe: reads frame i of both inputs before writing frame i of the outputs.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n)
    {
        if (!prepared_) return;
        const EngineParams& p = p_;
        if (snap_)
        {
            tSm_ = p.timeMs; mixSm_ = p.mix; fbSm_ = feedbackGain(p.feedback); bwSm_ = p.bandwidth;
            toneSm_ = p.tone; driveSm_ = p.drive; widthSm_ = p.width; stereoSm_ = p.stereoize ? 1.0f : 0.0f;
            snap_ = false;
        }
        const float fbT = feedbackGain(p.feedback);
        const float widthT = p.width;
        const bool stereo = p.stereoize;

        // grit-dependent constants (per block)
        const float bits = gritBits(p.grit);
        const float step = std::pow(2.0f, 1.0f - bits);
        const float zoh = clamp(0.6f + 0.8f * p.grit, 0.0f, 1.0f);
        const float hiss = 0.00012f + 0.0009f * p.grit * p.grit;
        const float outHiss = 0.00002f + 0.00012f * p.grit;           // ~ -94 .. -82 dBFS rms
        const float trackLoss = 1.0f / (1.01f + 0.03f * p.grit);
        const float expLag = ch_[0].onePoleMs(0.4f + 4.0f * p.grit);
        // clock jitter / drift of the free-running clock oscillator: relative to the period,
        // so long delays wobble more (0.001 %..0.011 % rms, ~7 Hz wide)
        const float jitterRel = (0.1f + 1.0f * p.grit) * 1.0e-4f;

        Channel::Tick tk[2];
        for (int c = 0; c < 2; ++c)
        {
            tk[c].bitsStep = step; tk[c].zoh = zoh; tk[c].hiss = hiss; tk[c].trackLoss = trackLoss;
            tk[c].expLag = expLag; tk[c].hold = p.hold;
            tk[c].peer = &ch_[1-c]; tk[c].pingpong = stereo;
        }

        for (int i = 0; i < n; ++i)
        {
            const float dryL = std::isfinite(inL[i]) ? clamp(inL[i], -16.0f, 16.0f) : 0.0f;
            const float dryR = std::isfinite(inR[i]) ? clamp(inR[i], -16.0f, 16.0f) : 0.0f;

            // ---- smoothing ------------------------------------------------------------------
            tSm_ += timeGlide_ * (p.timeMs - tSm_);
            mixSm_ += paramGlide_ * (p.mix - mixSm_);
            fbSm_ += paramGlide_ * (fbT - fbSm_);
            bwSm_ += paramGlide_ * 0.3f * (p.bandwidth - bwSm_);
            toneSm_ += paramGlide_ * (p.tone - toneSm_);
            driveSm_ += paramGlide_ * (p.drive - driveSm_);
            widthSm_ += paramGlide_ * (widthT - widthSm_);
            stereoSm_ += paramGlide_ * ((stereo ? 1.0f : 0.0f) - stereoSm_);

            if ((ctrl_++ & (kCtrl - 1)) == 0) controlUpdate();

            // ---- modulation: drifting LFO + clock jitter --------------------------------------
            const float tSec = tSm_ * 0.001f;
            const float lfoL = lfoL_.next(p.modSpeedHz, invSr_, 1.0f);
            const float depthRel = modDepthRel(p.modDepth, p.modSpeedHz, tSec);
            const float jL = jitterLp_.lp(rng_.nextBipolar()) * jitterNorm_;
            float teffL = tSec * (1.0f + depthRel * lfoL + jitterRel * jL);
            float teffR = teffL; // identical clocks: no Haas offsets / opposite-phase modulation
            // minus ~0.18 ms: the converters' own latency (filter group delay, hold), so the
            // first echo lands on the Time setting
            teffL = clamp(teffL, 0.0012f, 1.25f) * sr_ - kLatency;
            teffR = clamp(teffR, 0.0012f, 1.25f) * sr_ - kLatency;

            // ---- shared noise (identical on both sides unless Stereoize) ----------------------
            const float nzL = rng_.nextBipolar(), dzL = rng_.nextBipolar();
            const float nzR = stereo ? rngR_.nextBipolar() : nzL;
            const float dzR = stereo ? rngR_.nextBipolar() : dzL;
            const float ozL = rng_.nextBipolar();
            const float ozR = stereo ? rngR_.nextBipolar() : ozL;

            // ---- input drive (op-amp stage) ---------------------------------------------------
            const float xL = opampClip(dryL * driveG_, 1.4f) * driveComp_;
            const float xR = opampClip(dryR * driveG_, 1.4f) * driveComp_;

            const float cells = cellsCtrl_;
            tk[0].teffSamples = teffL; tk[0].cellsN = cells; tk[0].fb = fbSm_; tk[0].noise = nzL; tk[0].dither = dzL;
            tk[1].teffSamples = teffR; tk[1].cellsN = cells; tk[1].fb = fbSm_; tk[1].noise = nzR; tk[1].dither = dzR;

            const float prevL = ch_[0].wet(), prevR = ch_[1].wet();
            tk[0].feedbackSample = prevL + stereoSm_ * (prevR - prevL);
            tk[1].feedbackSample = prevR + stereoSm_ * (prevL - prevR);
            // Wet mono-sum injection in ping-pong; OFF preserves independent L/R inputs.
            // Dry never enters this matrix. Anti-phase stereo can cancel in this wet mono sum.
            const float mono = 0.5f * (xL + xR);
            float wL = ch_[0].process(xL + stereoSm_ * (mono - xL), tk[0]);
            float wR = ch_[1].process(xR * (1.0f - stereoSm_), tk[1]);
            const float pan = 1.0f - stereoSm_ * (1.0f - widthSm_);
            const float mid = 0.5f * (wL + wR), side = 0.5f * (wL - wR) * pan;
            wL = mid + side; wR = mid - side;

            wL += outHiss * ozL;   // output stage noise (wet only, outside the loop)
            wR += outHiss * ozR;

            const float dryG = std::fmin(1.0f, 2.0f * (1.0f - mixSm_));
            const float wetG = std::fmin(1.0f, 2.0f * mixSm_);
            outL[i] = dryL * dryG + wL * wetG;
            outR[i] = dryR * dryG + wR * wetG;
        }
    }

    // ---- mappings (public for the measurement tools) -------------------------------------
    static float feedbackGain(float fb) { return clamp(fb, -1.0f, 1.0f) * 0.99f; }
    static float driveGain(float d) { return std::pow(10.0f, (24.0f * d) / 20.0f); }   // 0..+24 dB
    // Relative clock modulation: up to +-45 % (flanger range) but the resulting pitch swing is
    // limited to about +-8 % so long delays wobble musically instead of reversing.
    static float modDepthRel(float depth, float rateHz, float tSec)
    {
        const float cap = 0.08f / (2.0f * kPi * std::fmax(rateHz, 0.01f) * std::fmax(tSec, 1e-4f));
        return depth * depth * std::fmin(0.45f, cap);
    }

    float clockRatio(int c) const { return ch_[c].clockRatio(); }

private:
    static float wrap01(float x) { return x >= 1.0f ? x - 1.0f : x; }

    void controlUpdate()
    {
        driveG_ = driveGain(driveSm_);
        driveComp_ = 1.0f / std::pow(driveG_, 0.6f);   // partial make-up so Drive mostly adds colour
        cellsCtrl_ = memoryCells(bwSm_);
        const float bwCut = bandwidthCutHz(bwSm_);
        const float toneHz = toneCutHz(toneSm_);
        ch_[0].updateFilters(std::fmin(1.0f, ch_[0].clockRatio()) * sr_, bwCut, 1.0f, toneHz);
        ch_[1].updateFilters(std::fmin(1.0f, ch_[1].clockRatio()) * sr_, bwCut, 1.0f, toneHz);
    }

    EngineParams p_;
    Channel ch_[2];
    DriftLfo lfoL_, lfoR_;
    Xorshift32 rng_{1}, rngR_{2};
    OnePole jitterLp_, jitterLpR_;
    float sr_ = 44100.0f, invSr_ = 1.0f / 44100.0f;
    float timeGlide_ = 0.0f, paramGlide_ = 0.0f;
    float tSm_ = 300.0f, mixSm_ = 0.35f, fbSm_ = 0.0f, bwSm_ = 0.6f, toneSm_ = 0.6f, driveSm_ = 0.2f, widthSm_ = 0.0f;
    float jitterNorm_ = 1.0f, driveG_ = 1.0f, driveComp_ = 1.0f;
    float cellsCtrl_ = 8192.0f, stereoSm_ = 0.0f;
    unsigned ctrl_ = 0;
    bool snap_ = true;
    bool prepared_ = false;
};

} // namespace edly

#endif
