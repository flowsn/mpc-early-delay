# v0.6 implementation, verification and delivery report

**80s Lofi Delay Box / Flat Peach / metadata0.6.0 / display v0.6 / UID FpEd**.
Branch **v0.6-ui-presets**, from **475b9cb** on v0.5-hardware-feedback.
Single owner for code and skin; SDK submodule unchanged. No DSP engine code changed.
Milestones **4196e5a** (implementation) and **c87e291** (measured bank, UI and build evidence)
were committed and pushed to the private origin through **github-mpc-delay**.
Final package and delivery checkpoint hashes are in branch history / PROGRESS.md.

## Requested fixes and evidence

1. **Q badge:** inspected the private hardware photo without copying it into the repository.
   The orange firmware Q sits at the top-left of the active `qlinkBoundsData` row and obscured
   the old first-label anchor x34. SDK ui-reference §3.6 exposes the group rectangle, not the
   badge's dimensions or rendering code. Reserve x20..62 (42px); labels now start at x66
   (slot-relative x36, formerly x4) with4px minimum reserved-envelope clearance. This applies
   to every top label, including TIME/TONE/MOD SPEED/PING WIDTH. Previews show conservative
   simulated badges on every occupied row. `tools/v06_verify.py` checks name geometry.
   Exact firmware extent remains an inference from the photo, not a measured SDK constant.
2. **MORE labels:** traced the code difference: page1 names were Image/PNG; fallback MORE
   names were fixed Label(type=Text, handleName=Invalid), with no name PNGs generated.
   JSON ordering/assets for that strategy were SDK-valid. The hardware photo also omits
   static captions and header-right text, while values and PNG names render. The failure
   path is fixed host Text rendering; inaccessible firmware prevents identifying its internal
   cause. `bake_fixed_labels` now generates passive Image assets for ALL fixed text, including
   fallback names from the real parameter dump. No static Text widgets remain; live values
   are still bound Label(type=Value, handleName=Data). Both MORE names/assets/bounds/mappings
   and rendered previews pass. This avoids the failing path rather than guessing a text order.
3. **Switches:** restored the design-ui vertical ribbed slide-thumb artwork for INVERT,
   HOLD and STEREOIZE, including visibly different thumb positions and lamps. Retained
   v0.5's whole-slot Mouse Down/Enter → Toggle Switch and passive Indicator, the topology
   Flo confirmed works. No Slider/drag widget or competing inner Button was introduced.
   SDK refs §§4.2/6.18; static topology/state-image checks pass. On/off previews inspected.
4. **Strip:** selected the second supplied original AI texture, committed as
   `design/textures/charcoal-original.png`; the first supplied texture was not copied.
   `texture_strip` trims solid padding, separately resamples chipped edge bands and centre,
   grades charcoal-blue, softly protects title contrast and quantizes the material to96colours.
   One full-width image, no tiled seams. `--texture` permits a drop-in replacement.
   Title/version are separate protected image overlays. Both pages visually inspected.
   Provenance in `docs/LICENSES.md` and shipped Content/LICENSES.txt: **AI-generated texture,
   original, no reference photos used**. No private-photo pixels/logos/trademark names in artwork.
   Skin **47files /508,178bytes /496.27KiB**; full source texture is not installed in the skin.
5. **Presets:** retuned seven combs across2–12ms, positive/negative feedback, static/slow/fast
   clock motion, varied tone/BW/grit/drive and stereoize/width. First10 presets are unchanged
   (including chorus/flange favourites); added Slow Velvet Chorus, Fast Shimmer Chorus and
   Hollow Tape Flange for20total. All14 values/IDs remain stable. HOLD starts off everywhere
   because freezing empty memory is silent; Hold Drone Comb is play-then-capture with deep
   slow modulation/stereo, demonstrated in demo5. Parameter and measured peak/centroid table
   in CHANGES-v0.6.md. Actual-wrapper wet spectra: all7 dominant peaks differ; all21 pairs
   have normalized-power L1 distance≥0.46667 (0=identical,2=disjoint; test gate>0.1).
   Strongest measured resonance is not necessarily the fundamental; no listening verdict claimed.
6. **Identity/update:** manifest0.6.0, binary integer600, version.xml0.6.0.0;
   uiversion remains0.1.0.0 (minimum UI format). Product, manufacturer, UID and installed folder
   **Flat Peach - VST - 80s Lofi Delay Box** unchanged from v0.5. `LocalPlugin.folder` and
   SDK `update → replace=True` verified in sdk/tools/mpc_remote.py. Update replaces the same
   folder; no old-name removal or side-by-side migration is needed for v0.5 → v0.6.

## Gates (final runs)

| Gate | Result / relative evidence |
|---|---|
| Native / ARM32 / ARM64 | PASS; logs/build-all.log |
| SDK strict validator | **0errors /0warnings**; logs/validate-strict.log |
| Host simulator32/64/96/128/192/256/512 frames | **20passed /0warnings /0failures each**; logs/build-all.log |
| Baseline engine selftest | **0failures**; logs/offline-selftest.log |
| `tools/next_check.py --sanitize` | PASS including integrated v0.6 spectra; logs/next-check.log, logs/next/regression-summary.txt |
| State/invalid controls | 20program roundtrips,14IDs stable, all14 NaN/+Inf/-Inf tested; logs/next/framework_probe.log, parameter-nan.log |
| Old12-ID states / signed Invert / Output | PASS; logs/next/v05_regression.log |
| Stereo/reset/HOLD/mono/automation regressions | PASS; logs/next/next_regression.log, probe.log, lifecycle_probe.log |
| All20 factory stress tests | finite, sample peak<0dBFS, decay<-50dBFS after20s; logs/next/v05_regression.log |
| Worst factory / revised comb peak | **-2.080dBFS /-4.777dBFS** respectively |
| 30s held comb | finite and sustained; peak-19.207dBFS, RMS-31.611→-31.684dBFS; Output works frozen |
| ASan/UBSan/float-cast-overflow | all5 harnesses PASS, no runtime diagnostics; logs/next/*sanitized.log |
| Audio-thread allocation | **0instrumented C++ allocations**, actual wrapper and engine; DSP/process path unchanged |
| Spectrum distinction | all21pair distances>0.1; minimum0.46667; logs/next/v06_presets.log |
| UI/Q-Link | all14 mapped/reachable, images/bounds/overlays valid; logs/ui-verify.log |
| Skin byte reproduction | all47files and both main previews byte-identical; logs/repro-ui-acceptance.log |
| UI static behavior | one touch owner, passive children, no Focus, sliding-thumb state change; same acceptance log |
| ZIP/WAV integrity | CRC, exact skin/content/versions, both ARM ELF machines, five WAVs; logs/package-verify.log |

The first v0.6 UI-verifier run had a stale0.5.0 expected-version literal copied from v0.5;
corrected to0.6.0 and rerun successfully. No acceptance criterion was weakened.
One standalone framework-probe build retains the SDK's pre-existing snprintf truncation
warning. This is separate from the zero-warning strict plugin validator/host-sim results.
Callback instrumentation counts C++ new, not exhaustive C allocator interposition. Tests
measure sample peaks, not oversampled true peaks; factory trims are not a general limiter.

## Deliverables

- `plugin/dist/80s-lofi-delay-box-0.6-mpc.zip` (**520,478bytes**)
- ZIP SHA256: **a7fac3228feb7d390bb57fca9a1b219508d0a95281eb02b7932bdbd3e9b665b2**
- `reports/v0.6/preview-delay.png`, `preview-delay-more1.png`, `preview-keys-on.png`
- `CHANGES-v0.6.md`; `reports/v0.6/SHA256SUMS.txt`, `artifacts.json`
- Five generated WAVs in `demos/v0.6/` (gitignored, delivered privately):

| File | Duration | PCM peak dBFS |
|---|---:|---:|
|01-seven-combs-comparison.wav|21s|-13.707|
|02-chorus-flange-tour.wav|20s|-12.158|
|03-robot-voice-flutter.wav|8s|-19.563|
|04-stereo-flange-plucks.wav|8s|-13.804|
|05-hold-drone-capture.wav|8s|-15.240|

All stereo44.1kHz/16-bit PCM, no loudness normalization,10ms boundary fades. First demo
compares all7combs at3s each with identical excitation; second demonstrates2retained and3new
chorus/flange patches at4s each. Full sequence/capture times in CHANGES-v0.6.md.

Private delivery target: **klawsapp:MPC-plugins/early-delay/v0.6/**. Exact11files only
(ZIP,5WAVs,3PNGs,CHANGES,SHA256SUMS); no share links. Delivery/check status is recorded in
PROGRESS.md and logs/upload.log / logs/upload-check.log after transfer.

## Remaining uncertainty

Firmware internal cause of fixed-Text omission and exact orange-Q dimensions cannot be
proved without firmware tracing; PNG labels and generous clearance address observed behavior.
Final physical screen contrast/badge/Q-Link interaction, device CPU/project reload and actual
audio audition remain hardware-only. Flo confirmed v0.5 taps, not this new build's visuals.
Legacy12-ID states load neutral appended controls only into a fresh instance; SDK missing IDs
retain current values in an edited instance (set Invert off/Output0dB first). Captured audio is
not serialized. No MPC deploy/access/remove/restart, external messages or public sharing occurred.
This is implementation evidence, not independent reviewer sign-off.
