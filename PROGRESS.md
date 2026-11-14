# Next candidate repair — durable checkpoint

Branch: repair/stereo-pingpong-next-20261114. Starting HEAD b0310e6.
Inherited dirty/untracked work preserved in d7deded before any edits.

Implemented (working tree, awaiting focused regression):
- Stereoize OFF ordinary stereo; ON mono wet injection L with cross-feedback, shared timing, Width wet pan separation.
- Constant-cost immutable HOLD ring resampling (up to eight aperture taps), alternating banks for ping-pong.
- Reset control/clock state, safe normalized wrapper reads and engine inputs/parameters, recursive recovery, control-rate memory pow.
- Version 0.1.1 and build script all seven block sizes.

Verified: existing offline selftest PASS (25 checks incl. 60s HOLD). Log reports/next-20261114/logs/selftest.log.
Running: native build and copied original review probes, logs in same directory.
Next: inspect logs, add assertion-based pingpong timing/dry stereo/reset/nonfinite/HOLD regressions; integrate clean approved design from early-delay-design 97f239c; full ARM/native build; sanitizers; docs, demos and dev zip; independent parent QA.
No hardware deployment, no public release. Original review untouched. SDK checkout is ignored/untracked upstream tooling and is not being changed.
