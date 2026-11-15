# 80s Lofi Delay Box — Flat Peach

Original proprietary stereo early-digital delay, built on the MIT MPC Plugin SDK.
**v0.6 / 0.6.0 — private hardware-feedback candidate.** Stable internal UID `FpEd`.

Variable-clock companded memory, signed feedback/INVERT, repitchable HOLD, genuine ping-pong,
20 factory presets including seven resonating combs, and original photographic painted-metal charcoal/amber skin.

- Player controls and install: [plugin/README.md](plugin/README.md)
- Changes: [CHANGES-v0.6.md](CHANGES-v0.6.md)
- Verification and handoff: [reports/v0.6/REPORT.md](reports/v0.6/REPORT.md)
- UI implementation/evidence: [docs/DESIGN.md](docs/DESIGN.md)
- Interrupted-work recovery: [PROGRESS.md](PROGRESS.md)
- Build: `python3 tools/build.py` (native/ARM32/ARM64, validator, seven host-sim block sizes)
- Regressions/demos: `python3 tools/next_check.py --sanitize`
- Baseline tests: `python3 tools/offline.py selftest`

Flo confirmed v0.5 single-tap behavior on the MPC Live III. v0.6 keeps those handlers and
restores slide-switch artwork; both pages use badge-safe image legends. Update the same
**Flat Peach - VST - 80s Lofi Delay Box** folder, not a side-by-side install.
v0.6 is checked offline; no deployment, restart, public publication or external messaging
was performed. Final v0.6 physical visual/Q-Link checks remain pending. Historical reports in `reports/next-20261114` describe the prior candidate.
