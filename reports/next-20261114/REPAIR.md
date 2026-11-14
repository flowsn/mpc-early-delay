# Early Delay 0.1.1 — next development candidate repair

Implementation follow-up to the independent 2026-11-14 review. **Not hardware-ready; no deployment or public release.**
The original review/snapshot at `/home/flow/projects/mpc-plugins/reviews/early-delay-20261114/` was left unchanged.
This report is an implementation report, not an independent sign-off; parent coordinator arranges separate QA.

## Identity and preserved work

Private branch `repair/stereo-pingpong-next-20261114` in `flowsn/mpc-early-delay`.
Starting reviewed HEAD `b0310e64bd2c5362ac99209dc8fc7ffd6cbaf132`; SDK submodule remains
`febd2ca5bd9eb16c4a412837196e145566bc981b`.

- `d7deded`: checkpoint of **all** pre-existing dirty/untracked offline tests, tooling, measurement notes and Content/README.
- `f624c7c`: WIP DSP repair with offline baseline passing, pushed privately.
- `a6474d4`: approved design integration, regression suite, accurate controls/architecture docs, pushed privately.
- Subsequent verification/package commit is recorded in Git history and PROGRESS.md.

UID `FpEd`, Flat Peach / Early Delay identity, stereo 2-in/2-out, 12 parameter IDs/order and all eleven original
program value arrays are unchanged. Index 11 / ID `width` now displays **Ping Width**. Version 0.1.1.
Stereoize-enabled presets intentionally acquire the corrected ping-pong behavior, not sonic backward compatibility
with the rejected widening design. Preset/state compatibility is control-data compatibility; held audio is not saved.

## Corrected routing

**OFF:** independent ordinary L/R delay and self-feedback; dry never collapses to mono.
**ON:** `(L+R)/2` enters the left wet bank. Shared delay clocks plus cross-feedback produce L at Time, R at
2×Time, L at 3×Time, etc. Both previous-frame feedback values are sampled before either channel processes,
avoiding channel-order bias. Signed feedback applies at each feedback traversal, so successive echoes invert
when negative; the first echo is not inverted by feedback. Zero feedback means only the first left echo.

Width changes post-loop pan separation only: 0% centred, 100% hard L/R. It does not skew clocks or construct an
antiphase/allpass/Haas side signal. Mono fold is independent of Width within floating-point tolerance.
ON intentionally mono-sums the **wet injection**: anti-phase stereo input can cancel there; OFF preserves stereo
wet content. Ordinary delay/feedback interference, especially signed feedback, still exists. No guarantee that
all natural cancellation disappears is made.

## Review findings addressed

| Prior finding | Repair and evidence |
|---|---|
| Severe Stereoize narrow-band cancellation | Removed opposing delay offsets, side high-pass and stereo coefficient offsets; genuine cross-feedback routing. Prior 62.5–1000Hz zero-feedback probes now give -3.010dB mono loss relative to mean channel power (expected single-sided echo), not the previous ~-13 to -27dB notches. |
| Reset history leakage | Reset control counter, clock ratio, derived initial state and force fresh control update. Fresh-vs-reset maximum delta **0** after both 44,103 and 44,160 historical frames. |
| HOLD host-clock pitch ceiling | Immutable captured ring with bounded interpolated read, not one-cell-per-frame clock ticks. 100→50ms: **2007.05Hz**; 300→150ms: **2002.18Hz**, expected ~2000Hz. 300→50ms: **6021.23Hz**, expected ~6000Hz. Small deviations include integer capture length, latency compensation and clock jitter. |
| NaN/Inf contamination | Wrapper validates normalized controls before conversion; engine separately clamps/sanitizes plain controls; audio sanitized before both dry/wet branches. Recursive guard repairs affected state. NaN/Inf input: **0** nonfinite outputs, recovered late RMS **-15.25dBFS**. All 12 control indices tested with NaN and ±Inf; finite output. Raw invalid host storage is not rewritten on the audio thread. |
| Per-sample pow | Memory-size mapping moved to the existing 16-frame control cadence. Parameter mappings/transcendentals are per block/control tick; per-sample compander sqrt remains and needs device profiling. |
| Unsupported stability/mono/CPU guarantees | Removed peak-gain proof, no-cancellation and unmeasured ARM CPU claims. Historical measurement notebook clearly marked superseded. |

HOLD stops writing captured cells, ignores new wet input/feedback, and in ping-pong alternates frozen channel
banks once per captured loop. Feedback polarity already in the captured signal remains. It uses one sample tap
at <=1 cell/frame and eight aperture taps above; no loop proportional to pitch ratio, allocation, locks or I/O.
The aperture is an approximate lo-fi filter, **not ideal antialiasing**: extreme pitch-up can alias/smear. Release
restores normal writes with span glide and a 2ms output crossfade. Capture/toggle boundaries can be audible.

## Tests and evidence

All logs are in [logs/](logs/). Original probes are copied here for reproducibility and run against current source,
not the historical snapshot. Assertions and exit-code handling live in `tools/next_check.py`.

| Verification | Observed result |
|---|---|
| Native / ARM32 / ARM64 | Fresh Zig 0.17.1 builds PASS; identity/ELF/export/GLIBC validation PASS. [build-all.log](logs/build-all.log) |
| Host simulator | **20 passed, 0 warnings, 0 failures at each of 32/64/96/128/192/256/512 frames**. [build-all.log](logs/build-all.log) |
| Existing offline suite | PASS, 0 failures: extrema, 60s HOLD/input rejection, mono OFF, block-size independence, dry and delay timing. [selftest.log](logs/selftest.log) |
| Old-fault regression | Reset exact, in-place exact, pitch ratios within 2%, narrow-tone mono-fold checks; all pass. [probe.log](logs/probe.log) |
| New stereo tests | Mono/left-only/right-only impulses, ±0.8 feedback, first four echoes alternate at 100/200/300/400ms within 3ms and >14.7dB channel separation; signed burst polarity; OFF channel isolation; dry stereo bit-exact; Width preserves mono sum; extreme HOLD 900→1.75ms/release finite. [next_regression.log](logs/next_regression.log) |
| Legal automation | 2,560,000 frames, randomized HOLD/stereo/time/signed feedback/other controls: 0 nonfinite output; peak **1.440885** (floating point). [probe.log](logs/probe.log) |
| Long HOLD/release | 2/10/30/60s: **-3.4212/-3.4189/-3.4191/-3.4185dBFS**. Released silence decays to **-42.56dBFS** final window. [lifecycle_probe.log](logs/lifecycle_probe.log) |
| Timing/bandwidth/color | At 10ms, 8kHz **-2.288dB**, 12kHz **-29.520dB**; at 900ms, 8kHz **-51.311dB**. First echo 1.678–900.045ms across nominal 1.75–900ms. ±99% feedback bursts decay through repeat 40. [spectral_probe.log](logs/spectral_probe.log) |
| State/presets | **11 × 12** exact normalized-value round trips, 195-byte chunks. [framework_probe.log](logs/framework_probe.log) |
| Memory safety / RT | ASan, UBSan, float-cast-overflow runs, no diagnostics; instrumented callback operator-new count **0**. Static process-path review: no locks/logging/files. The SDK may lazily allocate on an abnormal unprepared host lifecycle; not claimed safe for such misuse. [regression-summary.txt](logs/regression-summary.txt) |
| UI | Integrated approved `early-delay-design` commit `97f239c8640dc9d3d0c9c8a0d37bf26549379341`, deterministic distressed charcoal/amber; all 12 named controls reachable in screen/program maps, bounds/assets/overlays valid. Added real MORE-page Ping Width knob, updated caption. [ui-verify.log](logs/ui-verify.log), [preview](ui-preview.png), [MORE preview](ui-preview-more1.png) |

The standalone GCC framework probe emits an understood pre-existing SDK `snprintf` truncation warning in
program-name formatting; normal Zig plugin build and runtime sanitizers are separate evidence. Static UI preview
was inspected in this environment; it is not an MPC screenshot or a touch-interaction test.

## Reproduction

Run from `/home/flow/projects/mpc-plugins/early-delay`:

```sh
python3 tools/build.py
python3 tools/offline.py selftest
python3 tools/next_check.py --sanitize
python3 tools/verify_skin.py plugin/skins --params build/params.json
python3 sdk/tools/package.py plugin
```

Rebuild custom UI after changing parameters/design:

```sh
python3 tools/build.py --target native --skip-test
python3 tools/make_skin.py --params build/params.json --out plugin/skins --style A --preview reports/next-20261114/ui-preview.png
python3 tools/build.py
```

## Listening artifacts

Three 5-second stereo 44.1kHz/16-bit WAVs under `demos/next-20261114/` (generated, gitignored):

- `ordinary-stereo-mono-plucks.wav`: OFF comparison.
- `pingpong-mono-plucks.wav`: same mono plucks, ON, 250ms, positive feedback, full pan separation.
- `pingpong-stereo-negative-feedback.wav`: separated input plucks, ON wet mono injection, 180ms, negative feedback.

These synthetic demos contain no external recordings. They were rendered and structurally checked, not auditioned
against historical hardware. Exact checksums and artifact paths are in [artifacts.json](artifacts.json). Development ZIP: `/home/flow/projects/mpc-plugins/early-delay/plugin/dist/early-delay-0.1.1-mpc.zip` (ZIP integrity and required contents verified). The installed folder is `plugin/build/Flat Peach - VST - Early Delay/`.

## Remaining limitations

No physical MPC/ARM execution, CPU/latency/multiple-instance budget, touch/Q-Link interaction, device project reload,
reference-hardware authenticity listening, or full commercial legal/provenance audit. Compander transient attenuation
and extreme held-clock aliasing remain audible model choices to audition. Supported target is SDK 44.1kHz;
48/96kHz finite/timing probes do not establish arbitrary-rate sonic equivalence.

For later hardware testing: **MPC OS 3.10.1+** and **Preferences → Enhancements (Beta) → Enable Enhancements +
External Plugin Support**. No deployment was performed. Private development packaging is not a public release.
