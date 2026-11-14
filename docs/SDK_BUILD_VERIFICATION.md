# MPC plugin SDK – build verification (14 Nov 2026)

SDK: https://github.com/inmusic-software/mpc-plugin-builder-sdk (cloned to /home/flow/projects/mpc-plugins/sdk, public, no credentials needed)

## Verdict
- **Build works on this cloud server (Linux x86_64, 4 cores, 7 GB).** `python3 tools/build.py examples/StereoDelay` finished in ~2m50s (first run downloads the Zig cross compiler, ~50 MB, via pip) with **0 errors, 0 warnings**.
- Outputs: stereo-delay-arm32.so (187 KB), stereo-delay-arm64.so (240 KB, = MPC Live III), native test build, TUI.json / Q-Links.json skins, plugin.json, version.xml. Validator: AArch64 ELF OK, only libc/libm/libpthread/libdl needed, max GLIBC 2.27 (limit 2.39), exports VSTPluginMain.
- Host simulator test on the native build: **PASS, 20 passed / 0 warnings / 0 failures** (state round-trip, in-place processing, silence, NaN/denormals, timing 0.22 % CPU per 128-frame block on desktop). Rendered a 3 s test WAV.

## Platform facts (from the SDK docs)
- MPC Live III is supported (64-bit ARM, Cortex-A55 target). Needs **MPC OS 3.10.1+** with Preferences > Enhancements > Enable Enhancements + External Plugin Support.
- Plugin = Linux .so following the VST 2.4 binary interface (SDK has its own clean-room header, no Steinberg code). Do not use the term "VST" in product names (Steinberg trademark); "MPC"/"Akai Professional" are inMusic trademarks.
- **Licence: MIT** (framework, tools, examples). Plugins built with it are the author's own; no copyleft. Commercial distribution is allowed; inMusic does not review/sign/distribute third-party plugins.
- Fixed **44.1 kHz**, block 32-512 frames (treat announced size as a maximum). Real-time audio thread: no alloc/locks/file I/O in process(). Audio threads may have a 1 MB stack. Big buffers on the heap in prepare().
- **UI is data (JSON skins), no custom graphics:** knobs, faders, switches, labels, images, overlays, envelope displays, XY pads; 16 hardware Q-Links mapped via Q-Links.json. Parameter names ~12 chars, value text ~8.
- **Effects get no MIDI; plugin MIDI output is discarded.** Instruments receive MIDI. A built-in preview sequencer therefore belongs in the instrument (Project 2), which is fine.
- Presets: built-in programs declared in code (addProgram), shown in the MPC's preset selector. User presets are saved by the MPC itself (.xpl, tied to the device path). A custom SAVE button inside the plugin would need its own storage design (state chunks up to 128 MB are accepted by the host).
- State: opaque chunk saved in the project; parameter count is read once at load and must never change at run time.

## Steps that need local hardware / access
1. Copying to the MPC: USB stick or SD card, folder "VST2 Plugins" at the drive root (physical).
2. Or network install with tools/mpc_remote.py: MPC and computer on the same LAN, **developer mode enabled on the MPC** (SSH). This server is in Helsinki and cannot reach the MPC; run it from a computer on the same network (e.g. the WSL Ubuntu), or copy by USB.
3. Enabling External Plugin Support on the MPC and testing audio/UI/CPU on the real device (MPC CPU meters).
Everything else (writing, building ARM binaries, host-sim tests, packaging) works in the cloud.

## Recommended architecture – Project 1 (early digital delay)
- Own repo/folder, built on mpc::Plugin + framework/dsp (delay_line.h, lfo.h, svf.h/onepole.h/biquad.h, smoother.h). Start from examples/StereoDelay.
- Mono-style delay core with a variable-clock model: write at fixed rate, read position driven by a clock ratio so changing delay time sweeps pitch; bandwidth-limited write/read (anti-alias + reconstruction low-pass around 8 kHz, steep, with slight resonance), 12-bit-class quantisation/noise, soft companding (compress before / expand after the delay) to get the grit; feedback with polarity switch; LFO for clock modulation (speed/depth); tone control in the feedback loop; HOLD = feedback 100 % with input muted.
- Delay 1.75-900 ms at 44.1 kHz = ~40 k samples (tiny); CPU budget is not a concern on Live III.
- Parameters (~10): time, feedback, polarity, mod speed, mod depth, tone, hold, mix, plus colour/age (bandwidth+noise). One TUI page + Q-Link map. Presets via addProgram.
- Use original code only (SDK is MIT; no DSP library needed). Compare against DM1000 recordings if the user can supply some.

## Project 2 notes (not started)
- Instrument with an internal preview/step sequencer is feasible (instruments get tempo/transport and MIDI).
- SID filter: do NOT assume reSID/libsidplayfp (GPL) can be linked into a proprietary product; plan an original filter model (measured 6581 response curves) or a permissively licensed reference, decided before implementation.
- Wavetable tables as step-based parameter sequencers fit the parameter/state model; state chunk can hold table data.

## Housekeeping
- SDK clone: /home/flow/projects/mpc-plugins/sdk (unmodified). Build outputs: sdk/examples/StereoDelay/build/.
- Next, once approved: separate private repos (delay, sid-synth) outside the SDK folder; a GitHub account/deploy key is needed only to push them (GitHub access for flowsn is currently limited to the sidstation-bank-studio deploy key).
