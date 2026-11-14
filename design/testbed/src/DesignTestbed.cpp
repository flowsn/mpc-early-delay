// UI-only testbed using the public SDK Plugin API, as in examples/StereoDelay.
// This is a pass-through effect, NOT the delay product or a DSP simulation.
// SPDX-License-Identifier: MIT
#include "mpc_plugin.h"
class DesignTestbed : public mpc::Plugin {
    static mpc::PluginInfo info() {
        mpc::PluginInfo i;
        i.name="Design Testbed"; i.vendor="Early Delay Lab";
        i.uniqueId=MPC_FOURCC('E','d','U','i'); i.version=100;
        i.isInstrument=false; i.numInputs=2; i.numOutputs=2;
        return i;
    }
public:
    DesignTestbed():mpc::Plugin(info()) {
        using namespace mpc;
        addParam(logParam("time","Time",1.f,2000.f,384.f,"ms",0));
        addParam(floatParam("fb","Feedback",-100.f,100.f,-35.f,"%",0));
        addParam(logParam("rate","Mod Speed",.05f,20.f,.35f,"Hz",2));
        addParam(floatParam("depth","Mod Depth",0.f,100.f,18.f,"%",0));
        addParam(floatParam("tone","Tone",0.f,100.f,62.f,"%",0));
        addParam(floatParam("grit","Grit",0.f,100.f,26.f,"%",0));
        addParam(floatParam("age","Bandwidth/Age",0.f,100.f,40.f,"%",0));
        addParam(floatParam("drive","Drive",0.f,24.f,3.f,"dB",1));
        addParam(floatParam("mix","Mix",0.f,100.f,35.f,"%",0));
        addParam(floatParam("width","Width",0.f,200.f,120.f,"%",0));
        addParam(boolParam("hold","Hold",false));
        addParam(boolParam("stereoize","Stereoize",true));
    }
    void process(mpc::ProcessContext& c) override {
        for(int ch=0;ch<c.numOutputs;++ch)
            for(int n=0;n<c.numFrames;++n) c.outputs[ch][n]=c.inputs[ch%c.numInputs][n];
    }
};
MPC_PLUGIN_ENTRY(DesignTestbed)
