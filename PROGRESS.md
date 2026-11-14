# Next candidate repair — durable checkpoint

Branch: repair/stereo-pingpong-next-20261114, private GitHub origin.
Starting HEAD b0310e6. d7deded preserved ALL inherited dirty/untracked work.
f624c7c implemented initial repair and was pushed successfully.

Implemented:
- Ordinary stereo OFF; ON mono wet injection L with signed cross-feedback, shared timing, Ping Width output panning.
- Constant-cost immutable HOLD ring resampling; frozen banks alternate in ping-pong.
- Deterministic reset/control/clock; nonfinite wrapper/audio/engine containment; recursive recovery; control-rate pow.
- Version 0.1.1, unchanged 12 IDs/indices and 11 presets.
- Approved design 97f239c integrated (distressed charcoal/amber), updated PING-PONG caption, Ping Width MORE knob,
  all 12 program Q-Link slots; font OFL included. Preview visually inspected; static verification passed.
- Updated README/brief/architecture/player notes; marked old measurements historical.

Verified: original offline selftest PASS; all 7 native host simulator sizes PASS (20 checks each, zero warnings).
Old-fault probes: reset deltas exactly zero; NaN/Inf audio finite and recovered; all 12 NaN controls finite;
HOLD 100→50ms ~2007Hz and 300→150ms ~2002Hz; 2.56M automation frames finite; callback allocations zero.
New assertions: impulses alternate L/R at T/2T/3T/4T with ±feedback, mono/left/right input; ordinary stereo no
cross-leakage above noise; dry stereo bit-exact; Width mono sum unchanged; extreme HOLD pitchup/release finite.
Demos: demos/next-20261114/*.wav. Logs: reports/next-20261114/logs/.

COMPLETE implementation/build/package checks as of final verification commit (see git log):
- native/ARM32/ARM64 PASS with final custom skin; all7 hostsim sizes 20pass/0warn/0fail.
- python3 tools/next_check.py --sanitize PASS, including all12 NaN/±Inf controls and signed burst polarity.
- UI screen/program full12 mapping verifier PASS.
- ZIP plugin/dist/early-delay-0.1.1-mpc.zip produced, integrity/content checked.
- reports/next-20261114/artifacts.json contains exact binary/ZIP/demo paths and SHA256.
- reports/next-20261114/REPAIR.md is the detailed repair handoff.

Next step: parent independent QA; fix concrete findings if any, otherwise report this as a next development
candidate, NOT hardware-ready. No device deployment, public release, or external messages. Original review untouched.
Reproduction: python3 tools/build.py ; python3 tools/next_check.py --sanitize ; python3 tools/offline.py selftest.
