# Early Delay — Flat Peach

Original proprietary stereo early-digital delay, built on the MIT MPC Plugin SDK.
**0.1.1 next development candidate — not hardware-ready or publicly released.**

Stereo in/stereo out. Stereoize OFF is ordinary stereo delay; ON uses real cross-feedback ping-pong repeats.
The original companded, variable-clock, roughly 12-bit character remains. HOLD now supports pitch-up above
one cell per host frame through bounded immutable-buffer resampling.

- Player controls/install/limitations: [plugin/README.md](plugin/README.md)
- Current repair findings and reproducible evidence: [reports/next-20261114/REPAIR.md](reports/next-20261114/REPAIR.md)
- Interrupted-work recovery: [PROGRESS.md](PROGRESS.md)
- Build: `python3 tools/build.py` (native, ARM32, ARM64 and all seven simulator block sizes)
- Focused regression: `python3 tools/next_check.py`

Private repository. No device deployment or public release performed. Hardware use requires MPC OS 3.10.1+
and enabled External Plugin Support; actual ARM CPU/touchscreen/project persistence remain unverified.
