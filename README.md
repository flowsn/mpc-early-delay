# 80s Lofi Delay Box — Flat Peach

Original proprietary stereo early-digital delay, built on the MIT MPC Plugin SDK.
**v0.5 / 0.5.0 — private hardware-feedback candidate.** Stable internal UID `FpEd`.

Variable-clock companded memory, signed feedback/INVERT, repitchable HOLD, genuine ping-pong,
17 factory presets including seven resonating combs, and original aged-metal charcoal/amber skin.

- Player controls and install: [plugin/README.md](plugin/README.md)
- Changes: [CHANGES-v0.5.md](CHANGES-v0.5.md)
- Verification and handoff: [reports/v0.5/REPORT.md](reports/v0.5/REPORT.md)
- UI implementation/evidence: [docs/DESIGN.md](docs/DESIGN.md)
- Interrupted-work recovery: [PROGRESS.md](PROGRESS.md)
- Build: `python3 tools/build.py` (native/ARM32/ARM64, validator, seven host-sim block sizes)
- Regressions/demos: `python3 tools/next_check.py --sanitize`
- Baseline tests: `python3 tools/offline.py selftest`

Prior version ran on the MPC Live III. v0.5 is checked offline; no deployment, restart,
public publication or external messaging was performed. Physical tap/Q-Link behavior remains
unverified. Historical reports in `reports/next-20261114` describe the prior candidate.
