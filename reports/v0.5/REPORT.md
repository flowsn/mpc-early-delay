# v0.5 implementation and delivery report

**80s Lofi Delay Box / Flat Peach / 0.5.0 (display v0.5)**. Private development candidate.
Branch `v0.5-hardware-feedback`, from `repair/stereo-pingpong-next-20261114` at `c2a90e0`.
SDK unchanged at `febd2ca5bd9eb16c4a412837196e145566bc981b`.

Milestone commits: `2474fc2` (code/materials), `a3be4df` (skin, measured preset trims/tests).
Subsequent verification/package/delivery commits are in branch history and PROGRESS.md.
Every milestone was committed and pushed using the private origin via `github-mpc-delay`.
The design worktree was read, not modified. No private reference photos were opened, copied,
committed or uploaded; prior textual material observations guided original procedural artwork.

## Six requested changes

1. **Single-tap switches.** The previous Button was already v2 Instant, but overlaid name/state
   elements sat over it while the parent Mouse Down only selected Q-Link. That is a plausible
   cause of the observed interaction; hardware event tracing is unavailable. All three keys
   now have a single whole-slot `Mouse Down -> Toggle Switch` and `Enter Pressed -> Toggle Switch`.
   Passive Indicator/Label/Image children cannot compete as another Button, so taps on label,
   state or key use the same handler. No slider/drag/menu operation. SDK ui-reference §4.2
   explicitly documents this tap toggle; §6.18 documents noninteractive Indicator. SDK/static
   acceptance passes. Physical gesture execution remains unverified (Indicator is SDK “less tested”).
2. **Yellow box removed.** Found explicit `Focus`, `WhenFocussed`, outline `ffefb34e`, one-pixel
   thickness and translucent fill in the old skin generator. Removed every Focus from both pages.
   State is lamp, key brightness, lower lit bar and On/Off text; gray key border never changes.
   Firmware may draw its own Q-Link/page selection indication; the selected page border is meaningful.
3. **INVERT and layout.** Page 1 top: Time / Feedback / Invert / Mix. Middle: Tone / Drive /
   Bandwidth / Grit — interpreted “NE(?)” as Tone. Bottom: Mod Speed / Mod Depth / Hold / Stereoize.
   Feedback retains its original bipolar range/normalization, ID and index. Invert reverses its
   signed value (`negative + Invert = positive`); first echo/dry/captured HOLD are not inverted.
   Skin calls the Feedback readout BASE LOOP SIGN. Existing IDs 0..11 stay fixed; append Invert=12,
   Output=13. Fixed count14 from construction. All17 programs contain all14 values; screen and
   program Q-Link maps cover each parameter. MORE: Ping Width and -24..0 dB smoothed post-mix Output.
4. **Resonating presets and level safety.** 17 presets, including improved Metal Comb plus Inverted
   Comb, Bell Comb, Robot Voice, Ringing Pipe, Flange Comb, Hold Drone Comb. Times 2.4–8 ms, signed
   high regeneration, unchanged bounded loop coefficient (max ±0.99) and saturation model.
   Seven comb presets use -12/-15 dB Output. Initial stress exposed inherited presets peaking
   +4.5..+6.9 dBFS, so the original ten now add -9 dB trim without changing their first12 controls.
   Worst final sample peak **-2.080 dBFS**, comb bank **-4.698 dBFS or lower**. All decay below
   -50 dBFS within20s silence (worst long drone -59.36 dBFS). HOLD starts OFF to allow capture.
   Output is outside the loop and remains active while held. Default/manual settings and old fresh
   state loads keep0 dB for compatibility; this is not a limiter or an arbitrary-level loudness guarantee.
5. **Name/version.** Host manifest and binary name are `80s Lofi Delay Box`; manufacturer Flat Peach,
   stable unique ID `FpEd`. Metadata0.5.0, integer500, version.xml0.5.0.0; top strip v0.5.
   `uiversion` deliberately remains0.1.0.0: SDK documentation defines it as minimum host UI format,
   not product version (raising it would prevent loading). New folder: **Flat Peach - VST - 80s Lofi
   Delay Box**. Old folder **Flat Peach - VST - Early Delay** should be removed before installing the
   new one, because both share UID FpEd. `mpc_remote.py` names folders from manufacturer/name;
   later authorized command template: `python3 sdk/tools/mpc_remote.py <HOST> remove 'Flat Peach - VST - Early Delay'`
   with the correct drive selection. **Not executed.** SDK-required VST folder/format markers are
   packaging conventions, not product/artwork names. Binary filenames retain early-delay for continuity.
6. **Aged faceplate.** Original layered procedural brushed-metal strip: low-frequency oxidation,
   directional reflection, anisotropic grain, pits, fine scratches, exposed edge wear, dust, fasteners.
   Protected cream legends/amber readouts, recessed push keys. No source-photo pixels/logo/text.
   Installed skin: **461,176 bytes /450.37 KiB**, under600 KiB. Byte-identical regeneration of all27
   skin files and both full-page previews. Pillow12.1.1 and bundled OFL fonts. Both pages plus lit
   key-state preview were visually inspected; these are static compositions, not device screenshots.

## Quality gates and evidence

All paths relative to repository root.

| Check | Result / evidence |
|---|---|
| Native, ARM32, ARM64 | PASS; `reports/v0.5/logs/build-all.log` |
| SDK validator, explicit strict rerun | **0 errors /0 warnings**; `logs/validate-strict.log` |
| Host-sim 32/64/96/128/192/256/512 frames | **20 passed /0 warnings /0 failures each**; correct renamed native binary selected, not stale old library |
| Existing offline selftest | **0 failures**; `logs/offline-selftest.log` |
| `tools/next_check.py --sanitize` | PASS; reset exact, NaN/Inf audio/control containment, stereo routing/signed polarity/mono sum, HOLD pitch/release; `logs/next-check.log` and `logs/next/` |
| Param/state/presets | 17×14 roundtrips; all14 controls NaN/±Inf tested; stable first12 IDs, old12-ID state exact into fresh instance; `logs/next/framework_probe.log`, `parameter-nan.log`, `v05_regression.log` |
| v0.5 Invert and Output | ±85/0 feedback inverted exactly equals opposite signed base; Output exact post-mix gain; `logs/next/v05_regression.log` |
| Factory stress | Every program: 8s near-full-scale noise/sweep/square/multitone (alternating stereo phase),20s silence; finite, all peak<0 dBFS, all decay; same log |
| HOLD comb |30s capture, peak -18.111 dBFS, RMS -26.424→-26.425 dBFS; Output attenuation works during freeze |
| ASan/UBSan/float-cast-overflow | all four harnesses PASS, no runtime diagnostics; `logs/next/*sanitized.log` |
| Callback allocation | **0** instrumented C++ allocations in engine and actual plugin-wrapper callbacks; process-path review no added locks/files/I/O |
| UI/Q-Links |14/14 touchscreen and screen/program bindings; bounds/images/overlays valid; `logs/ui-verify.log` |
| Tap topology / focus / reproduction | PASS; `tools/v05_verify.py`, `logs/repro-ui-acceptance.log` |
| ZIP / WAVs | ZIP CRC/manifest/versions/skin-byte match; five stereo16-bit44.1kHz WAVs,8s each, measured PCM peaks below -1 dBFS; `artifacts.json` |

Standalone framework compilation retains the SDK's pre-existing `snprintf` truncation warning
in a probe build; strict plugin validation, target builds and host-sim warning counts above are
separate. No extra engine allocation was introduced; callback instrumentation counts C++ new,
not an exhaustive allocator interposition. Peak tests are sample peaks, not oversampled true peaks.

## Deliverables

- `plugin/dist/80s-lofi-delay-box-0.5-mpc.zip`
- ZIP SHA256: **aefa2078262bb1d639ee365d7cc43f10ad93e0ad2ac8262df136ea8202363977**
- `reports/v0.5/preview-delay.png`, `preview-delay-more1.png`, `preview-keys-on.png`
- `CHANGES-v0.5.md`, `reports/v0.5/SHA256SUMS.txt`, `artifacts.json`
- Five synthetic demos in `demos/v0.5/` (generated, not externally sourced):

| WAV | PCM sample peak dBFS (approximately) |
|---|---:|
|01-metal-comb-plucks.wav|-14.44|
|02-inverted-comb-drums.wav|-13.31|
|03-bell-comb.wav|-21.44|
|04-robot-voice-synthetic.wav|-14.47|
|05-hold-drone-comb.wav|-20.80|

No post-normalization; they reflect actual factory trims. HOLD demo plays/captures a sustained
excitation then repitches with Time. WAVs structurally/numerically verified, not physically auditioned.
Delivery target: **`klawsapp:MPC-plugins/early-delay/v0.5/`**, exact files only. Final upload/check
result is recorded in `logs/upload.log`, `logs/upload-check.log` and PROGRESS.md.

## Remaining hardware-only checks

Actual single-tap/Enter event behavior, host-drawn focus/bank selection, Q-Link bank interaction,
right sidebar overlap, physical contrast, device CPU/latency/multiple-instance budget, and project
reload/discovery after renaming. Legacy IDs/normalized states remain compatible, but the SDK retains
current values for missing IDs: importing old12-ID states into an already edited instance requires
Invert Off/Output0 dB first (fresh-instance load is tested). No held audio is serialized.

No MPC deployment, removal, restart, public publication/share links or messages to anyone occurred.
This is implementation evidence, not an independent QA sign-off.
