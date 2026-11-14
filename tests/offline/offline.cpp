// offline.cpp - offline measurements, self-tests and demo renders for the Early Delay engine.
//
// Copyright (c) 2026 Flat Peach. All rights reserved.
//
// Compiles the exact engine header used by the plugin (plugin/src/early_delay_dsp.h).
//   offline measure            -> Markdown measurement report on stdout
//   offline selftest           -> pass/fail checks, exit code 0 = all passed
//   offline demos <dir>        -> renders demo WAVs (synthetic sources; nothing external)
// Build: tools/offline.py (g++ -O2 -std=c++17 -I sdk/framework -I plugin/src)

#include "early_delay_dsp.h"

#include <algorithm>
#include <chrono>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using edly::Engine;
using edly::EngineParams;

static constexpr float kSr = 44100.0f;
static int g_fail = 0;

// ------------------------------------------------------------------------------------------
// helpers
// ------------------------------------------------------------------------------------------
struct Stereo { std::vector<float> l, r; size_t size() const { return l.size(); } };

static Stereo makeStereo(size_t n) { Stereo s; s.l.assign(n, 0.0f); s.r.assign(n, 0.0f); return s; }

// Runs the engine over `in` in blocks; `cb(frame)` may change params before each block.
static Stereo run(Engine& e, const Stereo& in, EngineParams p, int block = 128,
                  const std::function<void(size_t, EngineParams&)>& cb = nullptr)
{
    Stereo out = in;   // processed in place, like the MPC
    e.setParams(p);
    for (size_t pos = 0; pos < in.size(); pos += (size_t)block)
    {
        const int n = (int)std::min((size_t)block, in.size() - pos);
        if (cb) { cb(pos, p); e.setParams(p); }
        e.process(&out.l[pos], &out.r[pos], &out.l[pos], &out.r[pos], n);
    }
    return out;
}

static Engine* freshEngine(const EngineParams& p)
{
    Engine* e = new Engine();
    e->setParams(p);
    e->prepare(kSr);
    return e;
}

static EngineParams wetOnly()
{
    EngineParams p;
    p.mix = 1.0f; p.feedback = 0.0f; p.modDepth = 0.0f; p.grit = 0.0f; p.tone = 1.0f; p.drive = 0.0f;
    return p;
}

static double rms(const std::vector<float>& x, size_t a, size_t b)
{
    double s = 0; b = std::min(b, x.size());
    for (size_t i = a; i < b; ++i) s += (double)x[i] * x[i];
    return b > a ? std::sqrt(s / (double)(b - a)) : 0.0;
}
static double db(double x) { return 20.0 * std::log10(std::max(x, 1e-12)); }

// Goertzel amplitude of frequency f over [a,b) (Hann window), returns sine amplitude.
static double toneAmp(const std::vector<float>& x, size_t a, size_t b, double f)
{
    const size_t n = b - a;
    const double w = 2.0 * M_PI * f / kSr;
    double re = 0, im = 0, wsum = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double h = 0.5 - 0.5 * std::cos(2.0 * M_PI * (double)i / (double)(n - 1));
        re += h * x[a + i] * std::cos(w * (double)i);
        im -= h * x[a + i] * std::sin(w * (double)i);
        wsum += h;
    }
    return 2.0 * std::sqrt(re * re + im * im) / wsum;
}

static void fft(std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * M_PI / (double)len;
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w(1);
            for (size_t j = 0; j < len / 2; ++j)
            {
                auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl;
            }
        }
    }
}

// Averaged power spectrum (Hann, 4096) over [a,b).
static std::vector<double> powerSpectrum(const std::vector<float>& x, size_t a, size_t b, size_t nfft = 4096)
{
    std::vector<double> ps(nfft / 2 + 1, 0.0);
    int frames = 0;
    for (size_t s = a; s + nfft <= b; s += nfft / 2)
    {
        std::vector<std::complex<double>> buf(nfft);
        for (size_t i = 0; i < nfft; ++i)
            buf[i] = x[s + i] * (0.5 - 0.5 * std::cos(2.0 * M_PI * (double)i / (double)(nfft - 1)));
        fft(buf);
        for (size_t k = 0; k <= nfft / 2; ++k) ps[k] += std::norm(buf[k]);
        ++frames;
    }
    if (frames) for (auto& v : ps) v /= frames;
    return ps;
}

static double bandPower(const std::vector<double>& ps, double f0, double f1, size_t nfft = 4096)
{
    double s = 0;
    for (size_t k = 0; k < ps.size(); ++k)
    {
        const double f = (double)k * kSr / (double)nfft;
        if (f >= f0 && f < f1) s += ps[k];
    }
    return s;
}

static std::vector<float> sine(size_t n, double f, double amp)
{
    std::vector<float> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = (float)(amp * std::sin(2.0 * M_PI * f * (double)i / kSr));
    return v;
}

static std::vector<float> pinkish(size_t n, uint32_t seed, double amp)
{
    mpc::dsp::PinkNoise pn(seed);
    std::vector<float> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = (float)(amp * pn.process());
    return v;
}

// zero-crossing frequency estimate over [a,b)
static double zcFreq(const std::vector<float>& x, size_t a, size_t b)
{
    int c = 0; double first = -1, last = -1;
    for (size_t i = a + 1; i < b; ++i)
        if (x[i - 1] < 0 && x[i] >= 0)
        {
            const double t = (double)(i - 1) + x[i - 1] / (x[i - 1] - x[i]);
            if (first < 0) first = t; else ++c;
            last = t;
        }
    return c > 0 ? (double)c * kSr / (last - first) : 0.0;
}

static double corr(const std::vector<float>& a, const std::vector<float>& b, size_t s, size_t e)
{
    double ab = 0, aa = 0, bb = 0;
    for (size_t i = s; i < e; ++i) { ab += (double)a[i] * b[i]; aa += (double)a[i] * a[i]; bb += (double)b[i] * b[i]; }
    return ab / std::sqrt(std::max(aa * bb, 1e-30));
}

static bool allFinite(const Stereo& s)
{
    for (size_t i = 0; i < s.size(); ++i) if (!std::isfinite(s.l[i]) || !std::isfinite(s.r[i])) return false;
    return true;
}
static double peakAbs(const Stereo& s, size_t a = 0)
{
    double p = 0;
    for (size_t i = a; i < s.size(); ++i) p = std::max(p, (double)std::max(std::fabs(s.l[i]), std::fabs(s.r[i])));
    return p;
}

static void check(bool ok, const char* what)
{
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_fail;
}

// ------------------------------------------------------------------------------------------
// measurements
// ------------------------------------------------------------------------------------------

// Steady-state wet gain at frequency f (dB re input), Goertzel on the fundamental.
static double sineGain(float timeMs, float bw, double f, float grit = 0.0f, double amp = 0.25)
{
    EngineParams p = wetOnly(); p.timeMs = timeMs; p.bandwidth = bw; p.grit = grit;
    Engine* e = freshEngine(p);
    const size_t settle = (size_t)(timeMs * 0.001f * kSr) + 22050, len = 16384;
    Stereo in = makeStereo(settle + len);
    in.l = sine(in.size(), f, amp); in.r = in.l;
    Stereo out = run(*e, in, p);
    delete e;
    return db(toneAmp(out.l, settle, settle + len, f) / amp);
}

static double minus3dB(float timeMs, float bw)
{
    const double ref = sineGain(timeMs, bw, 500.0);
    double lo = 500, hi = 20000;
    for (int it = 0; it < 14; ++it)
    {
        const double mid = std::sqrt(lo * hi);
        if (sineGain(timeMs, bw, mid) > ref - 3.0) lo = mid; else hi = mid;
    }
    return std::sqrt(lo * hi);
}

// For a sine at f (wet only): returns the fundamental level and the level of everything else
// (aliases, images, distortion, noise), both in dB relative to the input sine.
struct AliasResult { double fund, rest; };
static AliasResult aliasRatio(float timeMs, float bw, double f, float grit)
{
    EngineParams p = wetOnly(); p.timeMs = timeMs; p.bandwidth = bw; p.grit = grit;
    Engine* e = freshEngine(p);
    const size_t settle = (size_t)(timeMs * 0.001f * kSr) + 22050, len = 4096 * 8;
    Stereo in = makeStereo(settle + len);
    in.l = sine(in.size(), f, 0.25); in.r = in.l;
    Stereo out = run(*e, in, p);
    delete e;
    auto ps = powerSpectrum(out.l, settle, settle + len);
    const double fund = bandPower(ps, f - 40, f + 40);
    const double total = bandPower(ps, 20, 20000);
    auto ref = powerSpectrum(in.l, settle, settle + len);
    const double inP = bandPower(ref, f - 40, f + 40);
    return { 10.0 * std::log10(std::max(fund, 1e-20) / inP), 10.0 * std::log10(std::max(total - fund, 1e-20) / inP) };
}

static void measure()
{
    std::printf("<!-- generated by tools/offline.py measure; engine plugin/src/early_delay_dsp.h -->\n\n");

    // ---- 1. impulse response -------------------------------------------------------------
    std::printf("## 1. Impulse response (wet only, feedback 0, mod 0, Grit 0, Bandwidth 60 %%)\n\n");
    std::printf("| Time setting | first echo peak at | expected | peak level | echo -20 dB width |\n|---|---|---|---|---|\n");
    for (float t : {1.75f, 10.0f, 100.0f, 300.0f, 900.0f})
    {
        EngineParams p = wetOnly(); p.timeMs = t;
        Engine* e = freshEngine(p);
        const size_t n = (size_t)(t * 0.001f * kSr) + 8192;
        Stereo in = makeStereo(n); in.l[1000] = in.r[1000] = 0.5f;
        Stereo out = run(*e, in, p);
        delete e;
        size_t pk = 0; float pv = 0;
        for (size_t i = 1000; i < n; ++i) if (std::fabs(out.l[i]) > pv) { pv = std::fabs(out.l[i]); pk = i; }
        size_t a = pk, b = pk;
        while (a > 1000 && std::fabs(out.l[a]) > pv * 0.1f) --a;
        while (b + 1 < n && std::fabs(out.l[b]) > pv * 0.1f) ++b;
        std::printf("| %.2f ms | %.2f ms | %.2f ms | %.1f dBFS (in -6 dBFS) | %.2f ms |\n", t,
                    (double)(pk - 1000) * 1000.0 / kSr, t, db(pv), (double)(b - a) * 1000.0 / kSr);
    }
    std::printf("\nThe compander smears a lone impulse (the compressor gain at the instant of the click is "
                "still the silence gain, then the expander tracks it), so the echo is a short burst rather "
                "than a clean click; that is expected for this class of unit.\n\n");

    // ---- 2. frequency response at several times --------------------------------------------
    std::printf("## 2. Wet frequency response (stepped sine, -12 dBFS, Grit 0, Bandwidth 60 %%)\n\n");
    const double freqs[] = {100, 500, 1000, 2000, 4000, 6000, 7000, 8000, 9000, 10000, 12000, 15000};
    std::printf("| Time | ");
    for (double f : freqs) std::printf("%.0f Hz | ", f);
    std::printf("\n|---|");
    for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; ++i) std::printf("---|");
    std::printf("\n");
    for (float t : {5.0f, 50.0f, 200.0f, 400.0f, 900.0f})
    {
        std::printf("| %.0f ms | ", t);
        const double ref = sineGain(t, 0.6f, 1000.0);
        for (double f : freqs) std::printf("%.1f | ", sineGain(t, 0.6f, f) - ref);
        std::printf("\n");
    }
    std::printf("\nValues in dB relative to 1 kHz. The 1 kHz wet level itself is listed in section 3.\n\n");

    // ---- 3. bandwidth vs time ---------------------------------------------------------------
    std::printf("## 3. Bandwidth vs delay time (-3 dB point of the wet path, Grit 0)\n\n");
    std::printf("| Time | clock (BW 0 %%) | -3 dB (BW 0 %%) | clock (BW 60 %%) | -3 dB (BW 60 %%) | 1 kHz level (BW 60 %%) | clock (BW 100 %%) | -3 dB (BW 100 %%) |\n|---|---|---|---|---|---|---|---|\n");
    for (float t : {1.75f, 10.0f, 50.0f, 100.0f, 200.0f, 300.0f, 500.0f, 700.0f, 900.0f})
    {
        auto clk = [&](float bw) { return std::min(44100.0, edly::memoryCells(bw) / (t * 0.001)); };
        std::printf("| %.2f ms | %.1f kHz | %.0f Hz | %.1f kHz | %.0f Hz | %.2f dB | %.1f kHz | %.0f Hz |\n", t,
                    clk(0.0f) / 1000.0, minus3dB(t, 0.0f), clk(0.6f) / 1000.0, minus3dB(t, 0.6f),
                    sineGain(t, 0.6f, 1000.0), clk(1.0f) / 1000.0, minus3dB(t, 1.0f));
    }
    std::printf("\nClock = memory cells / time, capped at 44.1 kHz (cells: 2048 at BW 0 %%, ~10.8k at 60 %%, 32768 at 100 %%).\n\n");

    std::printf("### Aliasing / images: 3 kHz sine at -12 dBFS, wet only\n\n");
    std::printf("Levels relative to the input sine: the 3 kHz fundamental that comes out, and everything else "
                "(aliases folded around the clock, images, distortion, noise) in 20 Hz-20 kHz.\n\n");
    std::printf("| Time | BW 60 %% Grit 0: fund / rest | BW 60 %% Grit 35 %%: fund / rest | BW 0 %% Grit 35 %%: fund / rest |\n|---|---|---|---|\n");
    for (float t : {10.0f, 200.0f, 500.0f, 900.0f})
    {
        const AliasResult a = aliasRatio(t, 0.6f, 3000, 0.0f), b = aliasRatio(t, 0.6f, 3000, 0.35f), c = aliasRatio(t, 0.0f, 3000, 0.35f);
        std::printf("| %.0f ms | %.1f / %.1f dB | %.1f / %.1f dB | %.1f / %.1f dB |\n", t, a.fund, a.rest, b.fund, b.rest, c.fund, c.rest);
    }
    std::printf("\n");

    // ---- 4. grit: quantisation / compander ---------------------------------------------------
    std::printf("## 4. Grit: distortion + noise of the wet path (1 kHz sine, Time 100 ms, BW 60 %%)\n\n");
    std::printf("| Grit | bits | THD+N @ -12 dBFS | THD+N @ -36 dBFS | idle noise after signal stops (A-unweighted) |\n|---|---|---|---|---|\n");
    for (float g : {0.0f, 0.35f, 0.7f, 1.0f})
    {
        double thd[2];
        int k = 0;
        for (double amp : {0.25, 0.0158})
        {
            EngineParams p = wetOnly(); p.timeMs = 100; p.grit = g;
            Engine* e = freshEngine(p);
            const size_t settle = 30000, len = 4096 * 8;
            Stereo in = makeStereo(settle + len); in.l = sine(in.size(), 1000, amp); in.r = in.l;
            Stereo out = run(*e, in, p);
            delete e;
            auto ps = powerSpectrum(out.l, settle, settle + len);
            const double fund = bandPower(ps, 960, 1040), tot = bandPower(ps, 20, 20000);
            thd[k++] = 10.0 * std::log10(std::max(tot - fund, 1e-20) / fund);
        }
        // idle noise: 1 s of tone then silence; measure 0.5..1.0 s after the echo has passed
        EngineParams p = wetOnly(); p.timeMs = 100; p.grit = g;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100 * 3);
        auto s = sine(44100, 1000, 0.25);
        std::copy(s.begin(), s.end(), in.l.begin()); in.r = in.l;
        Stereo out = run(*e, in, p);
        delete e;
        std::printf("| %.0f %% | %.1f | %.1f dB | %.1f dB | %.1f dBFS |\n", g * 100, edly::gritBits(g), thd[0], thd[1],
                    db(rms(out.l, 44100 + 22050, 44100 * 2)));
    }
    std::printf("\nThe expander pulls the idle noise far below the converter floor (12 bits ~ -74 dBFS); "
                "with signal present the noise rises with it (breathing).\n\n");

    // breathing: noise level beside a quiet low tone vs silence
    {
        EngineParams p = wetOnly(); p.timeMs = 50; p.grit = 0.35f;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100 * 4);
        auto s = sine(44100 * 2, 200, 0.25);
        std::copy(s.begin(), s.end(), in.l.begin()); in.r = in.l;
        Stereo out = run(*e, in, p);
        delete e;
        auto ps1 = powerSpectrum(out.l, 22050, 44100 * 2);
        auto ps2 = powerSpectrum(out.l, 44100 * 3, 44100 * 4);
        const double n1 = bandPower(ps1, 3000, 15000), n2 = bandPower(ps2, 3000, 15000);
        std::printf("Breathing (Grit 35 %%, 200 Hz tone at -12 dBFS): 3-15 kHz noise with tone present %.1f dB vs "
                    "after it stops %.1f dB (difference %.1f dB).\n\n",
                    10 * std::log10(std::max(n1, 1e-20) / 4096.0), 10 * std::log10(std::max(n2, 1e-20) / 4096.0),
                    10 * std::log10(std::max(n1, 1e-20) / std::max(n2, 1e-20)));
    }

    // ---- 5. feedback stability ----------------------------------------------------------------
    std::printf("## 5. Feedback stability\n\n");
    std::printf("Noise burst (50 ms, -12 dBFS) into the delay at Time 200 ms, wet only, Grit 35 %%, mod 0. "
                "Level of successive repeats (RMS over each echo window):\n\n");
    std::printf("| Feedback | Tone | repeat 1 | repeat 2 | repeat 5 | repeat 10 | repeat 20 | mean change per repeat | polarity of repeat 2 vs 1 |\n|---|---|---|---|---|---|---|---|---|\n");
    struct FbCase { float fb, tone; };
    for (FbCase c : {FbCase{0.5f, 1.0f}, FbCase{0.99f, 1.0f}, FbCase{-0.99f, 1.0f}, FbCase{0.99f, 0.3f}, FbCase{-0.5f, 1.0f}})
    {
        EngineParams p = wetOnly(); p.timeMs = 200; p.feedback = c.fb; p.grit = 0.35f; p.tone = c.tone;
        Engine* e = freshEngine(p);
        const size_t T = (size_t)(0.2 * kSr);
        Stereo in = makeStereo(T * 22 + 2000);
        auto nz = pinkish((size_t)(0.05 * kSr), 99, 0.25 / 0.24);
        for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
        Stereo out = run(*e, in, p);
        delete e;
        double lv[21];
        for (int k = 1; k <= 20; ++k) lv[k] = db(rms(out.l, T * k, T * k + nz.size() + 400));
        const double c12 = corr(std::vector<float>(out.l.begin() + T, out.l.begin() + T + nz.size()),
                                std::vector<float>(out.l.begin() + 2 * T, out.l.begin() + 2 * T + nz.size()), 0, nz.size());
        std::printf("| %+.0f %% | %.0f %% | %.1f dB | %.1f dB | %.1f dB | %.1f dB | %.1f dB | %.2f dB | %s (r=%.2f) |\n",
                    c.fb * 100, c.tone * 100, lv[1], lv[2], lv[5], lv[10], lv[20], (lv[20] - lv[1]) / 19.0,
                    c12 < 0 ? "inverted" : "same", c12);
    }
    std::printf("\nRepeats always decay (loop gain < 1 at the maximum setting); inverted feedback flips the polarity of every repeat.\n\n");

    // worst-case abuse
    {
        EngineParams p; p.mix = 1.0f; p.timeMs = 30; p.feedback = 1.0f; p.drive = 1.0f; p.grit = 1.0f; p.tone = 1.0f;
        p.bandwidth = 1.0f; p.modDepth = 1.0f; p.modSpeedHz = 10.0f;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100 * 30);
        auto nz = pinkish(44100 * 5, 7, 4.0);   // +12 dBFS pink noise for 5 s, then 25 s silence
        for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
        Stereo out = run(*e, in, p);
        delete e;
        std::printf("Abuse test (pink noise at +12 dBFS for 5 s into Feedback +100 %%, Drive/Grit/Depth/Speed max, Time 30 ms): "
                    "peak output %.2f (%.1f dBFS), all samples finite: %s, level 20-30 s after input stops: %.1f dBFS.\n\n",
                    peakAbs(out), db(peakAbs(out)), allFinite(out) ? "yes" : "NO", db(rms(out.l, 44100 * 25, 44100 * 30)));
    }

    // ---- 6. HOLD ---------------------------------------------------------------------------
    std::printf("## 6. HOLD (infinite repeat)\n\n");
    {
        EngineParams p = wetOnly(); p.timeMs = 400; p.grit = 0.35f; p.feedback = 0.3f;
        Engine* e = freshEngine(p);
        const size_t n = 44100 * 62;
        Stereo in = makeStereo(n);
        auto nz = pinkish(44100, 3, 0.25 / 0.24);
        for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i] * (float)(0.5 + 0.5 * std::sin(i * 0.0007));
        Stereo out = run(*e, in, p, 128, [](size_t pos, EngineParams& q) { q.hold = pos >= 44100; });
        delete e;
        std::printf("Pink noise for 1 s (Time 400 ms, Feedback 30 %%, Grit 35 %%), HOLD on at 1.0 s, then 60 s of hold:\n\n");
        std::printf("| seconds after HOLD | 2 | 10 | 30 | 60 |\n|---|---|---|---|---|\n| wet RMS | ");
        for (int s : {2, 10, 30, 60}) std::printf("%.2f dBFS | ", db(rms(out.l, 44100 * (1 + s) - 22050, 44100 * (1 + s) + 22050)));
        std::printf("\n\n");
    }
    // leakage: identical run without the late input tone must give bit-identical output
    {
        auto render = [](bool withTone) {
            EngineParams p = wetOnly(); p.timeMs = 400; p.grit = 0.35f; p.feedback = 0.3f;
            Engine* e = freshEngine(p);
            const size_t n = 44100 * 10;
            Stereo in = makeStereo(n);
            auto nz = pinkish(44100, 3, 0.25 / 0.24);
            for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
            if (withTone) for (size_t i = 44100 + 22050; i < n; ++i) in.l[i] = in.r[i] = (float)(0.3 * std::sin(i * 0.05));
            Stereo out = run(*e, in, p, 128, [](size_t pos, EngineParams& q) { q.hold = pos >= 44100; });
            delete e;
            return out;
        };
        Stereo a = render(false), b = render(true);
        double d = 0;
        for (size_t i = 44100 * 2; i < a.size(); ++i) d = std::max(d, (double)std::fabs(a.l[i] - b.l[i]));
        std::printf("Input isolation: the same run with and without a loud 350 Hz tone at the input from 1.5 s "
                    "differs by at most %.3g in the wet output from 2 s on (0 = nothing leaks into the frozen loop).\n", d);
    }
    // periodicity: Grit 0 (minimal clock jitter), no modulation
    {
        EngineParams p = wetOnly(); p.timeMs = 100; p.grit = 0.0f;
        Engine* e = freshEngine(p);
        const size_t n = 44100 * 12;
        Stereo in = makeStereo(n);
        auto nz = pinkish(44100, 3, 0.25 / 0.24);
        for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
        Stereo out = run(*e, in, p, 128, [](size_t pos, EngineParams& q) { q.hold = pos >= 44100; });
        delete e;
        const size_t L = 4410;
        double best = -1;
        for (size_t lag = L - 3; lag <= L + 3; ++lag)
            best = std::max(best, corr(std::vector<float>(out.l.begin() + 44100 * 10, out.l.begin() + 44100 * 11),
                                       std::vector<float>(out.l.begin() + 44100 * 10 + lag, out.l.begin() + 44100 * 11 + lag), 0, 44100));
        std::printf("Loop periodicity (Time 100 ms, Grit 0, no modulation): correlation of the held output with itself "
                    "one loop (~%zu samples) later = %.5f. The memory itself recirculates bit-exactly (cell copy); "
                    "what remains is the free-running clock's jitter and the output-stage noise.\n\n", L, best);
    }
    // hold + time change re-pitches the loop
    {
        EngineParams p = wetOnly(); p.timeMs = 300; p.grit = 0.0f;
        Engine* e = freshEngine(p);
        const size_t n = 44100 * 6;
        Stereo in = makeStereo(n);
        auto s = sine(44100, 1000, 0.25);
        std::copy(s.begin(), s.end(), in.l.begin()); in.r = in.l;
        Stereo out = run(*e, in, p, 128, [](size_t pos, EngineParams& q) {
            q.hold = pos >= 44100 - 4410; q.timeMs = pos >= 44100 * 2 ? 600.0f : 300.0f; });
        delete e;
        std::printf("HOLD a 1 kHz tone at Time 300 ms, then turn Time to 600 ms: held loop pitch %.1f Hz before, %.1f Hz after "
                    "(clock halves -> one octave down; expected 500 Hz).\n\n",
                    zcFreq(out.l, 44100 + 4410, 44100 * 2 - 4410), zcFreq(out.l, 44100 * 4, 44100 * 5));
    }

    // ---- 7. variable clock (no hold) ---------------------------------------------------------
    std::printf("## 7. Variable-clock pitch behaviour (no HOLD)\n\n");
    {
        EngineParams p = wetOnly(); p.timeMs = 300; p.grit = 0.0f;
        Engine* e = freshEngine(p);
        const size_t n = 44100 * 4;
        Stereo in = makeStereo(n);
        in.l = sine(n, 1000, 0.25); in.r = in.l;
        Stereo out = run(*e, in, p, 128, [](size_t pos, EngineParams& q) { q.timeMs = pos >= 44100 ? 600.0f : 300.0f; });
        delete e;
        std::printf("Steady 1 kHz input, Time moved 300 -> 600 ms at 1.0 s (70 ms knob lag). Output pitch: "
                    "before %.0f Hz; 1.20-1.50 s %.0f Hz (memory written at the old clock is read at the new, slower one); "
                    "2.5-3.5 s %.0f Hz (settled).\n\n",
                    zcFreq(out.l, 22050, 44100), zcFreq(out.l, (size_t)(1.2 * kSr), (size_t)(1.5 * kSr)),
                    zcFreq(out.l, (size_t)(2.5 * kSr), (size_t)(3.5 * kSr)));
    }

    // ---- 8. stereoize -----------------------------------------------------------------------
    std::printf("## 8. Stereoize: decorrelation and mono compatibility\n\n");
    std::printf("Mono pink noise (-12 dBFS) in, wet only, Time 250 ms, Feedback 40 %%, Mod Depth 30 %%, Grit 35 %%. "
                "Mono loss = level of (L+R)/2 relative to the mean L/R level (0 dB = identical channels, "
                "-3 dB = uncorrelated, below -3 dB = cancellation). Worst octave = largest loss in any octave band 63 Hz-16 kHz.\n\n");
    std::printf("| Mode | Time | L/R correlation | correlation < 150 Hz | mono loss (broadband) | worst octave mono loss |\n|---|---|---|---|---|---|\n");
    for (float t : {250.0f, 20.0f, 3.0f})
    for (int mode = 0; mode < 3; ++mode)
    {
        EngineParams p = wetOnly(); p.timeMs = t; p.feedback = 0.4f; p.modDepth = 0.3f; p.grit = 0.35f; p.tone = 0.7f;
        p.stereoize = mode > 0; p.width = mode == 2 ? 1.0f : 0.5f;
        Engine* e = freshEngine(p);
        const size_t n = 44100 * 8;
        Stereo in = makeStereo(n);
        in.l = pinkish(n, 11, 0.25 / 0.24); in.r = in.l;
        Stereo out = run(*e, in, p);
        delete e;
        const size_t a = 44100 * 2;
        std::vector<float> mono(n), lowL(n), lowR(n);
        float zl = 0, zr = 0; const float c = 1.0f - std::exp(-2.0f * (float)M_PI * 150.0f / kSr);
        for (size_t i = 0; i < n; ++i)
        {
            mono[i] = 0.5f * (out.l[i] + out.r[i]);
            zl += c * (out.l[i] - zl); zr += c * (out.r[i] - zr); lowL[i] = zl; lowR[i] = zr;
        }
        const double lr = 0.5 * (rms(out.l, a, n) + rms(out.r, a, n));
        auto pl = powerSpectrum(out.l, a, n), pr = powerSpectrum(out.r, a, n), pm = powerSpectrum(mono, a, n);
        double worst = 0;
        for (double f0 = 63; f0 < 16000; f0 *= 2)
        {
            const double avg = 0.5 * (bandPower(pl, f0, f0 * 2) + bandPower(pr, f0, f0 * 2));
            worst = std::min(worst, 10 * std::log10(std::max(bandPower(pm, f0, f0 * 2), 1e-20) / std::max(avg, 1e-20)));
        }
        std::printf("| %s | %.0f ms | %.3f | %.3f | %.2f dB | %.2f dB |\n",
                    mode == 0 ? "Stereoize off" : (mode == 1 ? "Stereoize, Width 50 %" : "Stereoize, Width 100 %"), t,
                    corr(out.l, out.r, a, n), corr(lowL, lowR, a, n), db(rms(mono, a, n) / lr), worst);
    }
    std::printf("\n");

    // ---- 9. CPU ----------------------------------------------------------------------------
    std::printf("## 9. CPU per block (this build machine, x86-64, g++ -O2, one instance, stereo)\n\n");
    std::printf("| Block | settings | mean time per block | real-time share |\n|---|---|---|---|\n");
    for (int block : {32, 128, 512})
    for (int heavy = 0; heavy < 2; ++heavy)
    {
        EngineParams p;
        if (heavy) { p.timeMs = 700; p.stereoize = true; p.modDepth = 0.8f; p.modSpeedHz = 6; p.grit = 1; p.bandwidth = 0.2f; p.feedback = 0.9f; }
        Engine* e = freshEngine(p);
        const size_t n = 44100 * 20;
        Stereo in = makeStereo(n); in.l = pinkish(n, 5, 0.3); in.r = pinkish(n, 6, 0.3);
        const auto t0 = std::chrono::steady_clock::now();
        Stereo out = run(*e, in, p, block);
        const auto t1 = std::chrono::steady_clock::now();
        delete e;
        const double sec = std::chrono::duration<double>(t1 - t0).count();
        const double blocks = (double)n / block;
        std::printf("| %d | %s | %.2f us | %.2f %% |\n", block, heavy ? "Stereoize, max Grit/mod, 700 ms" : "defaults",
                    sec / blocks * 1e6, 100.0 * sec / (n / kSr));
        if (!allFinite(out)) std::printf("(non-finite output!)\n");
    }
    std::printf("\nThe MPC Live III (Cortex-A55) is several times slower per clock than this machine; see the notes in the report.\n");
}

// ------------------------------------------------------------------------------------------
// self-tests (exit code)
// ------------------------------------------------------------------------------------------
static void selftest()
{
    // stability at every extreme combination
    for (float fb : {-1.0f, 1.0f})
    for (float t : {1.75f, 900.0f})
    for (float bw : {0.0f, 1.0f})
    for (int st = 0; st < 2; ++st)
    {
        EngineParams p; p.mix = 1; p.feedback = fb; p.timeMs = t; p.bandwidth = bw; p.stereoize = st;
        p.drive = 1; p.grit = 1; p.modDepth = 1; p.modSpeedHz = 10; p.tone = 1;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100 * 12);
        auto nz = pinkish(44100 * 2, 1, 2.0);
        for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
        Stereo out = run(*e, in, p, 96);
        delete e;
        char what[160];
        const double tail = db(rms(out.l, 44100 * 11, 44100 * 12));
        std::snprintf(what, sizeof what, "stable fb %+.0f time %.2f bw %.0f stereo %d: peak %.2f, last-second %.1f dBFS",
                      fb * 100, t, bw * 100, st, peakAbs(out), tail);
        check(allFinite(out) && peakAbs(out) < 4.0 && (t > 100 || tail < -40.0), what);
    }
    // hold stays bounded and steady for 60 s
    {
        EngineParams p = wetOnly(); p.timeMs = 333; p.grit = 1.0f; p.feedback = -1.0f; p.stereoize = true;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100 * 62);
        auto nz = pinkish(44100, 2, 1.5);
        for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
        Stereo out = run(*e, in, p, 512, [](size_t pos, EngineParams& q) { q.hold = pos >= 40000; });
        delete e;
        const double a = db(rms(out.l, 44100 * 3, 44100 * 5)), b = db(rms(out.l, 44100 * 59, 44100 * 61));
        char what[160];
        std::snprintf(what, sizeof what, "HOLD steady for 60 s: %.2f dBFS at 3 s, %.2f dBFS at 60 s", a, b);
        check(allFinite(out) && std::fabs(a - b) < 1.0 && peakAbs(out) < 2.5, what);
    }
    // HOLD isolates the input completely
    {
        auto render = [](bool withTone) {
            EngineParams p = wetOnly(); p.timeMs = 250; p.grit = 0.6f; p.feedback = 0.5f; p.stereoize = true; p.modDepth = 0.4f;
            Engine* e = freshEngine(p);
            Stereo in = makeStereo(44100 * 6);
            auto nz = pinkish(30000, 12, 0.8);
            for (size_t i = 0; i < nz.size(); ++i) in.l[i] = in.r[i] = nz[i];
            if (withTone) for (size_t i = 50000; i < in.size(); ++i) { in.l[i] = 0.9f * std::sin(i * 0.031f); in.r[i] = -in.l[i]; }
            Stereo out = run(*e, in, p, 64, [](size_t pos, EngineParams& q) { q.hold = pos >= 44100 * 0.8; });
            delete e;
            return out;
        };
        Stereo a = render(false), b = render(true);
        double d = 0;
        for (size_t i = 44100; i < a.size(); ++i) d = std::max(d, (double)std::max(std::fabs(a.l[i] - b.l[i]), std::fabs(a.r[i] - b.r[i])));
        check(d == 0.0, "HOLD: nothing from the input reaches the frozen loop (bit-identical wet output)");
    }
    // stereoize off: mono in -> identical L/R out
    {
        EngineParams p; p.mix = 0.6f; p.modDepth = 0.5f; p.grit = 0.8f;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100 * 3); in.l = pinkish(in.size(), 4, 0.5); in.r = in.l;
        Stereo out = run(*e, in, p);
        delete e;
        double d = 0; for (size_t i = 0; i < out.size(); ++i) d = std::max(d, (double)std::fabs(out.l[i] - out.r[i]));
        check(d == 0.0, "Stereoize off keeps a mono input exactly mono");
    }
    // block-size independence (32 vs 512) apart from control-rate timing: outputs nearly equal
    {
        EngineParams p;
        Stereo in = makeStereo(44100 * 2); in.l = pinkish(in.size(), 8, 0.4); in.r = pinkish(in.size(), 9, 0.4);
        Engine* e1 = freshEngine(p); Stereo o1 = run(*e1, in, p, 32); delete e1;
        Engine* e2 = freshEngine(p); Stereo o2 = run(*e2, in, p, 512); delete e2;
        double d = 0; for (size_t i = 0; i < o1.size(); ++i) d = std::max(d, (double)std::fabs(o1.l[i] - o2.l[i]));
        check(d == 0.0, "output is independent of the block size (32 vs 512)");
    }
    // mix 0 = dry exactly
    {
        EngineParams p; p.mix = 0.0f;
        Engine* e = freshEngine(p);
        Stereo in = makeStereo(44100); in.l = pinkish(in.size(), 3, 0.4); in.r = pinkish(in.size(), 4, 0.4);
        Stereo out = run(*e, in, p);
        delete e;
        double d = 0; for (size_t i = 0; i < out.size(); ++i) d = std::max(d, (double)std::fabs(out.l[i] - in.l[i]));
        check(d == 0.0, "Mix 0 % passes the dry signal untouched");
    }
    // delay accuracy: echo of a click lands within 0.3 ms of the setting
    for (float t : {1.75f, 50.0f, 450.0f, 900.0f})
    {
        EngineParams p = wetOnly(); p.timeMs = t;
        Engine* e = freshEngine(p);
        const size_t n = (size_t)(t * 0.001f * kSr) + 6000;
        Stereo in = makeStereo(n); for (int k = 0; k < 40; ++k) in.l[1000 + k] = in.r[1000 + k] = 0.5f * std::sin(k * 0.157f);
        Stereo out = run(*e, in, p);
        delete e;
        // cross-correlation peak
        size_t best = 0; double bv = -1;
        for (size_t lag = 20; lag + 1040 < n; ++lag)
        {
            double s = 0; for (int k = 0; k < 40; ++k) s += in.l[1000 + k] * out.l[1000 + lag + k];
            if (s > bv) { bv = s; best = lag; }
        }
        char what[120];
        const double ms = best * 1000.0 / kSr;
        std::snprintf(what, sizeof what, "delay %.2f ms measured %.2f ms", t, ms);
        check(std::fabs(ms - t) < 0.3 + 0.002 * t, what);
    }
}

// ------------------------------------------------------------------------------------------
// demo sources and renders
// ------------------------------------------------------------------------------------------
// Writes 24-bit stereo. Renders whose peak exceeds -1 dBFS are scaled down to -1 dBFS (the
// plugin itself runs in floating point with headroom; the demo files must not clip).
static void writeWav(const std::string& path, const Stereo& in)
{
    Stereo s = in;
    const double pk = peakAbs(s);
    if (pk > 0.891)
    {
        const float g = (float)(0.891 / pk);
        for (size_t i = 0; i < s.size(); ++i) { s.l[i] *= g; s.r[i] *= g; }
        std::printf("  (%s: peak %.2f dBFS, scaled to -1 dBFS)\n", path.c_str(), db(pk));
    }
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", path.c_str()); g_fail++; return; }
    const uint32_t n = (uint32_t)s.size(), dataBytes = n * 2 * 3;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(2); u32(44100); u32(44100 * 6); u16(6); u16(24);
    std::fwrite("data", 1, 4, f); u32(dataBytes);
    for (uint32_t i = 0; i < n; ++i)
        for (float v : {s.l[i], s.r[i]})
        {
            const int32_t q = (int32_t)std::lround(std::max(-1.0f, std::min(1.0f, v)) * 8388607.0f);
            const uint8_t b[3] = {(uint8_t)(q & 255), (uint8_t)((q >> 8) & 255), (uint8_t)((q >> 16) & 255)};
            std::fwrite(b, 1, 3, f);
        }
    std::fclose(f);
}

// A small synthetic drum loop at 100 bpm (kick, snare, hats), 2 bars, then silence.
static Stereo drums(double seconds)
{
    const size_t n = (size_t)(seconds * kSr);
    Stereo s = makeStereo(n);
    mpc::dsp::Xorshift32 r(77);
    const double beat = 0.6;
    auto add = [&](double t, int kind) {
        const size_t st = (size_t)(t * kSr);
        double ph = 0;
        for (size_t i = 0; i < (size_t)(0.5 * kSr) && st + i < n; ++i)
        {
            const double tt = i / (double)kSr;
            double v = 0;
            if (kind == 0) { const double f = 45 + 110 * std::exp(-tt * 30); ph += 2 * M_PI * f / kSr; v = 0.8 * std::sin(ph) * std::exp(-tt * 7); }
            if (kind == 1) { ph += 2 * M_PI * 190 / kSr; v = (0.35 * std::sin(ph) * std::exp(-tt * 25) + 0.45 * r.nextBipolar() * std::exp(-tt * 18)); }
            if (kind == 2) v = 0.18 * r.nextBipolar() * std::exp(-tt * (i % 2 ? 90 : 90));
            s.l[st + i] += (float)v; s.r[st + i] += (float)(kind == 2 ? v * 0.7 : v);
        }
    };
    for (int bar = 0; bar < 2; ++bar)
    {
        const double b0 = bar * 4 * beat;
        add(b0, 0); add(b0 + 1.5 * beat, 0); add(b0 + 2.5 * beat, 0);
        add(b0 + beat, 1); add(b0 + 3 * beat, 1);
        for (int h = 0; h < 8; ++h) add(b0 + h * beat / 2, 2);
    }
    // hi-pass the hats a little via the noise itself; fine for a demo source
    return s;
}

// Plucked synth arpeggio (detuned saw-ish plucks), mono-ish.
static Stereo plucks(double seconds)
{
    const size_t n = (size_t)(seconds * kSr);
    Stereo s = makeStereo(n);
    const double notes[] = {57, 60, 64, 67, 69, 67, 64, 60};
    for (int k = 0; k < 8; ++k)
    {
        const double f = 440.0 * std::pow(2.0, (notes[k] - 69) / 12.0);
        const size_t st = (size_t)(k * 0.3 * kSr);
        double ph1 = 0, ph2 = 0, lp = 0;
        for (size_t i = 0; i < (size_t)(0.6 * kSr) && st + i < n; ++i)
        {
            const double tt = i / (double)kSr;
            ph1 += f / kSr; ph2 += f * 1.006 / kSr;
            const double saw = (ph1 - std::floor(ph1)) * 2 - 1 + (ph2 - std::floor(ph2)) * 2 - 1;
            const double cut = 0.05 + 0.5 * std::exp(-tt * 12);
            lp += cut * (saw - lp);
            const float v = (float)(0.3 * lp * std::exp(-tt * 5));
            s.l[st + i] += v; s.r[st + i] += v;
        }
    }
    return s;
}

// Short spoken-like formant bursts ("vocal-ish" source): buzz through two resonances.
static Stereo vox(double seconds)
{
    const size_t n = (size_t)(seconds * kSr);
    Stereo s = makeStereo(n);
    double ph = 0, b1 = 0, b2 = 0, c1 = 0, c2 = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double t = i / (double)kSr;
        const double env = (t < 1.6) ? std::max(0.0, std::sin(M_PI * std::fmod(t, 0.4) / 0.4)) : 0.0;
        const double f0 = 140 + 30 * std::sin(t * 3);
        ph += f0 / kSr;
        const double src = (ph - std::floor(ph)) < 0.1 ? 1.0 : -0.11;
        const double fA = 700 + 400 * std::sin(t * 5), fB = 1800 + 600 * std::cos(t * 4);
        const double gA = 2 * std::sin(M_PI * fA / kSr), gB = 2 * std::sin(M_PI * fB / kSr);
        b1 += gA * c1; c1 -= gA * (b1 + 0.08 * c1) - gA * src * 0.3; // crude resonators
        b2 += gB * c2; c2 -= gB * (b2 + 0.1 * c2) - gB * src * 0.2;
        const float v = (float)(env * 0.25 * (b1 + 0.6 * b2));
        s.l[i] = v; s.r[i] = v;
    }
    return s;
}

static Stereo concat(const Stereo& a, double tailSeconds)
{
    Stereo s = a;
    s.l.resize(a.size() + (size_t)(tailSeconds * kSr), 0.0f);
    s.r.resize(s.l.size(), 0.0f);
    return s;
}

static void demos(const std::string& dir)
{
    struct Preset { const char* file; float v[12]; };
    // same values as the plugin's addProgram() list: Mix Time Fb MSpd MDep Tone Grit BW Drv Hold St Wid
    const Preset presets[] = {
        {"classic-echo",      {35, 300, 35, 0.6f, 15, 60, 35, 60, 20, 0, 0, 70}},
        {"slap",              {35, 95, 10, 0.5f, 5, 70, 35, 65, 25, 0, 0, 60}},
        {"tape-ish-short",    {40, 140, 30, 0.9f, 30, 50, 30, 55, 35, 0, 0, 60}},
        {"dirty-echo",        {40, 380, 55, 0.4f, 12, 45, 75, 35, 55, 0, 0, 60}},
        {"dub-throw",         {50, 600, 82, 0.25f, 18, 35, 50, 40, 45, 0, 1, 70}},
        {"inverted-flange",   {50, 2.2f, -85, 0.18f, 70, 85, 25, 80, 20, 0, 0, 60}},
        {"long-hold-drone",   {60, 850, 90, 0.12f, 25, 40, 55, 25, 30, 0, 1, 80}},
        {"wide-chorus-delay", {45, 22, 15, 0.7f, 45, 70, 30, 70, 20, 0, 1, 90}},
        {"broken-clock",      {50, 450, 60, 6.5f, 55, 50, 95, 10, 50, 0, 1, 80}},
        {"doubler",           {45, 28, 0, 1.2f, 20, 75, 20, 75, 15, 0, 1, 100}},
        {"metal-comb",        {55, 6, 80, 0.08f, 10, 80, 45, 60, 30, 0, 0, 60}},
    };
    auto toParams = [](const float* v) {
        EngineParams p;
        p.mix = v[0] * 0.01f; p.timeMs = v[1]; p.feedback = v[2] * 0.01f; p.modSpeedHz = v[3]; p.modDepth = v[4] * 0.01f;
        p.tone = v[5] * 0.01f; p.grit = v[6] * 0.01f; p.bandwidth = v[7] * 0.01f; p.drive = v[8] * 0.01f;
        p.hold = v[9] > 0.5f; p.stereoize = v[10] > 0.5f; p.width = v[11] * 0.01f;
        return p;
    };
    const Stereo src[3] = {concat(drums(4.8), 5.0), concat(plucks(2.6), 6.0), concat(vox(2.0), 6.0)};
    const char* srcName[3] = {"drums", "plucks", "vox"};
    writeWav(dir + "/source-drums.wav", src[0]);
    writeWav(dir + "/source-plucks.wav", src[1]);
    writeWav(dir + "/source-vox.wav", src[2]);
    int k = 0;
    for (const Preset& pr : presets)
    {
        const int si = (k == 5 || k == 7 || k == 9) ? 1 : (k == 1 || k == 2 || k == 10) ? 2 : (k % 2 ? 1 : 0);
        EngineParams p = toParams(pr.v);
        Engine* e = freshEngine(p);
        Stereo out = run(*e, src[si], p);
        delete e;
        char name[256];
        std::snprintf(name, sizeof name, "%s/%02d-%s-%s.wav", dir.c_str(), k + 1, pr.file, srcName[si]);
        writeWav(name, out);
        std::printf("wrote %s\n", name);
        ++k;
    }
    // performance demos
    {   // time sweep while plucks echo: variable-clock pitch bends
        EngineParams p = toParams(presets[0].v); p.feedback = 0.6f; p.mix = 0.5f;
        Engine* e = freshEngine(p);
        Stereo out = run(*e, concat(plucks(2.6), 7.0), p, 128, [](size_t pos, EngineParams& q) {
            const double t = pos / 44100.0;
            q.timeMs = t < 3.0 ? 300.0f : (t < 5.0 ? (float)(300.0 * std::pow(3.0, (t - 3.0) / 2.0)) : 900.0f - (float)std::min(1.0, (t - 5.0)) * 750.0f); });
        delete e;
        writeWav(dir + "/perf-time-sweep-plucks.wav", out);
    }
    {   // HOLD a drum bar, then play the time knob over the frozen loop
        EngineParams p = toParams(presets[0].v); p.timeMs = 600; p.feedback = 0.2f; p.mix = 0.5f;
        Engine* e = freshEngine(p);
        Stereo out = run(*e, concat(drums(4.8), 8.0), p, 128, [](size_t pos, EngineParams& q) {
            const double t = pos / 44100.0;
            q.hold = t > 2.4 && t < 11.0;
            q.timeMs = t < 5.0 ? 600.0f : (t < 8.0 ? (float)(600.0 + 300.0 * std::sin((t - 5.0) * 1.3)) : 600.0f + (float)(t - 8.0) * 100.0f);
            q.mix = t > 2.4 ? 0.7f : 0.5f; });
        delete e;
        writeWav(dir + "/perf-hold-and-pitch-drums.wav", out);
    }
    {   // same vox with Stereoize off / on (for an A/B)
        for (int st = 0; st < 2; ++st)
        {
            EngineParams p = toParams(presets[7].v); p.stereoize = st; p.timeMs = 180; p.feedback = 0.45f;
            Engine* e = freshEngine(p);
            Stereo out = run(*e, src[2], p);
            delete e;
            writeWav(dir + (st ? "/ab-stereoize-on-vox.wav" : "/ab-stereoize-off-vox.wav"), out);
        }
    }
    {   // Grit 0 vs 100 on plucks (A/B)
        for (int g = 0; g < 2; ++g)
        {
            EngineParams p = toParams(presets[3].v); p.grit = (float)g; p.bandwidth = g ? 0.1f : 1.0f;
            Engine* e = freshEngine(p);
            Stereo out = run(*e, src[1], p);
            delete e;
            writeWav(dir + (g ? "/ab-grit-max-bw-min-plucks.wav" : "/ab-grit-zero-bw-max-plucks.wav"), out);
        }
    }
    std::printf("demos written to %s\n", dir.c_str());
}

int main_real(int argc, char** argv);
#ifndef PROBE
int main(int argc, char** argv) { return main_real(argc, argv); }
#endif
int main_real(int argc, char** argv)
{
    const std::string cmd = argc > 1 ? argv[1] : "";
    if (cmd == "measure") measure();
    else if (cmd == "selftest") selftest();
    else if (cmd == "demos" && argc > 2) demos(argv[2]);
    else { std::fprintf(stderr, "usage: offline measure | selftest | demos <dir>\n"); return 2; }
    if (cmd == "selftest") std::printf("%s (%d failures)\n", g_fail ? "SELFTEST FAILED" : "SELFTEST PASSED", g_fail);
    return g_fail ? 1 : 0;
}
