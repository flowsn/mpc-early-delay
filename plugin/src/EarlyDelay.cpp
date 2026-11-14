// EarlyDelay.cpp - Early Delay by Flat Peach: a gritty early-digital variable-clock delay.
//
// Copyright (c) 2026 Flat Peach. All rights reserved.
// Built on the inMusic MPC Plugin SDK framework (MIT, see docs/LICENSES.md).
//
// Effect, 2 in / 2 out, no MIDI. All audio code lives in early_delay_dsp.h (signal flow and
// model are documented there); this file declares parameters, presets and value text, and
// hands parameter snapshots to the engine once per block.
//
// Parameter order is permanent (ids are stored in projects). Parameter 0 is "Mix" because the
// host nudges parameter 0 to 0/1 and straight back on every prepare: Mix is smoothed and has no
// side effects, so the nudge is inaudible.
//
//  #  id         name        range                 purpose
//  0  mix        Mix         0..100 %              dry/wet (both at unity at 50 %)
//  1  time       Time        1.75..900 ms (log)    clock period: sets delay, pitch moves with it
//  2  feedback   Feedback    -100..+100 %          regeneration; negative = inverted polarity
//  3  modspeed   Mod Speed   0.05..10 Hz (log)     drifting analog-style LFO rate
//  4  moddepth   Mod Depth   0..100 %              clock modulation depth
//  5  tone       Tone        0..100 %              low-pass in the repeat path (dark..open)
//  6  grit       Grit        0..100 %              bit depth 14..8, S/H style, compander mistracking, hiss, jitter
//  7  bandwidth  Bandwidth   0..100 %              converter bandwidth + memory size (aged..fresh)
//  8  drive      Drive       0..100 %              input op-amp drive 0..+24 dB
//  9  hold       Hold        Off/On                infinite repeat (frozen memory loop)
// 10  stereoize  Stereoize   Off/On                alternating L/R cross-feedback repeats
// 11  width      Width       0..100 %              wet ping-pong pan separation

#include "mpc_plugin.h"
#include "early_delay_dsp.h"

#include <cstdio>

class EarlyDelay : public mpc::Plugin
{
public:
    enum Params { kMix, kTime, kFeedback, kModSpeed, kModDepth, kTone, kGrit, kBandwidth,
                  kDrive, kHold, kStereoize, kWidth, kNumParams };

    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo i;
        i.name = "Early Delay";
        i.vendor = "Flat Peach";
        i.uniqueId = MPC_FOURCC('F', 'p', 'E', 'd');   // == "uid" in plugin.json
        i.version = 110;                                // 0.1.1 development candidate
        i.isInstrument = false;
        i.numInputs = 2;
        i.numOutputs = 2;
        i.tailSamples = 0;                              // unknown (feedback / hold)
        return i;
    }

    EarlyDelay() : mpc::Plugin(makeInfo())
    {
        using namespace mpc;
        addParam(floatParam("mix",       "Mix",       0.0f, 100.0f, 35.0f, "%", 0));
        addParam(logParam  ("time",      "Time",      1.75f, 900.0f, 300.0f, "ms", 1));
        addParam(floatParam("feedback",  "Feedback",  -100.0f, 100.0f, 35.0f, "%", 0));
        addParam(logParam  ("modspeed",  "Mod Speed", 0.05f, 10.0f, 0.6f, "Hz", 2));
        addParam(floatParam("moddepth",  "Mod Depth", 0.0f, 100.0f, 15.0f, "%", 0));
        addParam(floatParam("tone",      "Tone",      0.0f, 100.0f, 60.0f, "%", 0));
        addParam(floatParam("grit",      "Grit",      0.0f, 100.0f, 35.0f, "%", 0));
        addParam(floatParam("bandwidth", "Bandwidth", 0.0f, 100.0f, 60.0f, "%", 0));
        addParam(floatParam("drive",     "Drive",     0.0f, 100.0f, 20.0f, "%", 0));
        addParam(boolParam ("hold",      "Hold",      false));
        addParam(boolParam ("stereoize", "Stereoize", false));
        addParam(floatParam("width",     "Width",     0.0f, 100.0f, 70.0f, "%", 0));
        if (numParams() != kNumParams) mpc::log("EarlyDelay: parameter count mismatch");

        //                                Mix  Time   Fb  MSpd MDep Tone Grit  BW  Drv Hold St  Wid
        addProgram("Classic Echo",      { 35, 300,   35, 0.6f, 15,  60,  35,  60,  20, 0, 0, 70 });
        addProgram("Slap",              { 35,  95,   10, 0.5f,  5,  70,  35,  65,  25, 0, 0, 60 });
        addProgram("Tape-ish Short",    { 40, 140,   30, 0.9f, 30,  50,  30,  55,  35, 0, 0, 60 });
        addProgram("Dirty Echo",        { 40, 380,   55, 0.4f, 12,  45,  75,  35,  55, 0, 0, 60 });
        addProgram("Dub Throw",         { 50, 600,   82, 0.25f,18,  35,  50,  40,  45, 0, 1, 70 });
        addProgram("Inverted Flange",   { 50, 2.2f, -85, 0.18f,70,  85,  25,  80,  20, 0, 0, 60 });
        addProgram("Long Hold Drone",   { 60, 850,   90, 0.12f,25,  40,  55,  25,  30, 0, 1, 80 });
        addProgram("Wide Chorus Delay", { 45,  22,   15, 0.7f, 45,  70,  30,  70,  20, 0, 1, 90 });
        addProgram("Broken Clock",      { 50, 450,   60, 6.5f, 55,  50,  95,  10,  50, 0, 1, 80 });
        addProgram("Doubler",           { 45,  28,    0, 1.2f, 20,  75,  20,  75,  15, 0, 1, 100 });
        addProgram("Metal Comb",        { 55,   6,   80, 0.08f,10,  80,  45,  60,  30, 0, 0, 60 });
    }

    void prepare(double sampleRate, int /*maxBlockSize*/) override
    {
        engine_.prepare((float)sampleRate);   // the only allocation (2 x 64k cells)
    }

    void reset() override { engine_.reset(); }

    void process(mpc::ProcessContext& ctx) override
    {
        edly::EngineParams p;
        p.mix = safeGet(kMix) * 0.01f;
        p.timeMs = safeGet(kTime);
        p.feedback = safeGet(kFeedback) * 0.01f;
        p.modSpeedHz = safeGet(kModSpeed);
        p.modDepth = safeGet(kModDepth) * 0.01f;
        p.tone = safeGet(kTone) * 0.01f;
        p.grit = safeGet(kGrit) * 0.01f;
        p.bandwidth = safeGet(kBandwidth) * 0.01f;
        p.drive = safeGet(kDrive) * 0.01f;
        p.hold = safeGet(kHold) >= 0.5f;
        p.stereoize = safeGet(kStereoize) >= 0.5f;
        p.width = safeGet(kWidth) * 0.01f;
        engine_.setParams(p);

        const float* inL = ctx.inputs[0];
        const float* inR = ctx.numInputs > 1 ? ctx.inputs[1] : ctx.inputs[0];
        float* outL = ctx.outputs[0];
        float* outR = ctx.numOutputs > 1 ? ctx.outputs[1] : ctx.outputs[0];
        if (outR == outL)
        {   // mono output (not used by the MPC, which always gives effects 2 channels): process
            // into the left buffer only, with a scratch-free path: duplicate input pointer.
            engine_.process(inL, inL, outL, outL, ctx.numFrames);
            return;
        }
        engine_.process(inL, inR, outL, outR, ctx.numFrames);
    }

    // Value text includes the unit: the MPC shows the display text but not the unit label.
    bool formatValue(int index, float v, std::string& out) override
    {
        char buf[32];
        switch (index)
        {
            case kTime:
                if (v < 10.0f) std::snprintf(buf, sizeof buf, "%.2f ms", (double)v);
                else if (v < 100.0f) std::snprintf(buf, sizeof buf, "%.1f ms", (double)v);
                else std::snprintf(buf, sizeof buf, "%.0f ms", (double)v);
                break;
            case kFeedback:
                if (std::fabs(v) < 0.5f) std::snprintf(buf, sizeof buf, "0%%");
                else std::snprintf(buf, sizeof buf, "%+.0f%%", (double)v);
                break;
            case kModSpeed:
                if (v < 1.0f) std::snprintf(buf, sizeof buf, "%.2f Hz", (double)v);
                else std::snprintf(buf, sizeof buf, "%.1f Hz", (double)v);
                break;
            case kHold:
            case kStereoize:
                return false;   // "On" / "Off"
            default:
                std::snprintf(buf, sizeof buf, "%.0f%%", (double)v);
                break;
        }
        out = buf;
        return true;
    }

private:
    float safeGet(int i) const
    {
        const float norm = getNormalised(i);
        return std::isfinite(norm) ? param(i).toPlain(mpc::clamp(norm, 0.0f, 1.0f)) : param(i).def;
    }
    edly::Engine engine_;
};

MPC_PLUGIN_ENTRY(EarlyDelay)
