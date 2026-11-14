# Architecture notes (draft)

- Base: `mpc::Plugin` from the SDK framework + `framework/dsp` helpers (delay_line, lfo, svf/onepole/biquad, smoother). Start from `examples/StereoDelay`.
- Core: delay buffer written at a fixed rate; read position driven by a clock ratio so that changing delay time sweeps pitch (variable-clock behaviour). Buffer for 900 ms at 44.1 kHz is about 40k samples.
- Colour chain: pre-emphasis/compressor -> bandlimited write (anti-alias low-pass near 8 kHz, steep) -> low-bit-depth quantisation + noise -> delay -> reconstruction low-pass -> expander -> tone -> feedback path (polarity switch, soft clip).
- HOLD: feedback to 100 percent with input muted.
- Modulation: LFO (speed/depth) on the delay clock.
- Parameters (about 10): time, feedback, polarity, mod speed, mod depth, tone, hold, mix, colour/age. One TUI page + Q-Links map. Built-in programs for presets.
- Testing: SDK host simulator (`tools/host-sim`), null/impulse/sweep tests, CPU budget check, then on-device.
