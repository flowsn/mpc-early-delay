# Project 1 brief - Early Digital Delay (priority 1)

Create an authentic early-digital hardware delay emulation: distinctive bandwidth limits, modulation, feedback behaviour and gritty coloration. Not a clean modern delay with a basic low-pass.

## Essential features
- Delay range about 1.75-900 ms.
- Vintage variable-clock behaviour: pitch changes when the delay time is moved.
- Wet-signal bandwidth about 8 kHz.
- Adjustable feedback, including inverted feedback.
- Modulation speed and depth; tone control.
- Infinite repeat / HOLD; wet/dry balance.
- Period-correct companding and analog signal conditioning (approximation).
- Simple hardware-inspired touchscreen UI; MPC Q-Link assignments; built-in presets (no custom SAVE - the MPC's own preset function is used).

## Strategy
1. Build and run the official SDK StereoDelay example (done on the cloud server, see ../REPORT).
2. Implement delay timing, feedback, modulation.
3. Add vintage bandwidth and nonlinear colouration.
4. Add HOLD and inverted feedback.
5. UI (JSON skin, may use custom PNG artwork) and presets.
6. Compare with original hardware recordings where available.
7. Optimise and test on MPC Live III.

Priority: accurate sound and stability over extra modern features.

## Success criteria
Stable, characterful early-digital delay running natively on MPC Live III, simple interface, commercially distributable build.

## Constraints from the SDK (verified)
Effects receive no MIDI; fixed 44.1 kHz; block size 32-512; real-time audio thread (no allocation/locks/file I/O in process()); UI is JSON skins with optional PNG artwork, no arbitrary drawing from code; presets are built-in programs.
