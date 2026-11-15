# v0.6.1 — preset GUI refresh hotfix

**80s Lofi Delay Box / Flat Peach / 0.6.1 / binary 601 / UID FpEd**

Branch `v0.6.1-gui-refresh`, based on `v0.6-ui-presets` at `4f3b4ab`.

## Root cause and repair

The GUI is host-owned JSON, not an editor implemented by this plugin. All 14
controls already bind correctly: `plugin/skins/TUI.json` remaps each component's
`Data` to `Parameter N`; both Knob/Indicator visuals and Value labels read that
handle. MORE uses the same mechanism for Ping Width and Output. There are no
plugin-side widget listeners or redraw methods to patch.

`sdk/framework/mpc_plugin.h:554` (`setProgram`) and `:513` (`internalLoadState`)
write atomic parameter values and schedule an audio-state reset, but neither
calls `requestDisplayUpdate`. The plugin used the bare SDK entry/dispatcher, so
no layer invalidated the host's cached GUI after a bulk load. This explains why
the DSP gets new values while knobs/switches retain their displayed values.

The SDK explicitly documents `requestDisplayUpdate` as re-reading parameters/text
and rebuilding the UI page (`sdk/docs/host-contract.md:58`); the helper emits
`audioMasterUpdateDisplay` (`sdk/framework/mpc_plugin.h:395`).

`plugin/src/EarlyDelay.cpp:188` now wraps the SDK dispatcher locally and emits
exactly one display update **after** a valid program selection or successful
chunk load. Re-selecting the same program works. Both bank/project and preset
chunks are covered. It delegates every opcode/return convention to the SDK;
no UI callbacks are emitted from ordinary `setParameter` or audio processing,
and no fake `audioMasterAutomate` gestures are recorded. The SDK remains pinned
and unmodified. No DSP, factory values or skin code needed changing.

## Verification

| Check | Result / evidence in `logs/` |
|---|---|
| Reproduce on original v0.6 native binary | Expected FAIL: missing display update; `gui-refresh-before.log` |
| Native / ARM32 / ARM64 builds | PASS; `build-all.log` |
| Strict validator | **0 errors / 0 warnings**; `validate-strict.log` |
| Host-sim 32/64/96/128/192/256/512 frames | **20 passed / 0 warnings / 0 failures each**; `build-all.log` |
| Baseline selftest | PASS, 0 failures; `offline-selftest.log` |
| Existing regression suite | PASS; `next-check.log`, `next/regression-summary.txt` |
| Existing ASan/UBSan/float-cast-overflow harnesses | All five PASS; `next/*sanitized.log` |
| Native GUI callback/cache regression | PASS; `gui-refresh.log`, `v061_gui_refresh.log` |
| GUI harness + plugin sanitizers | PASS; `gui-refresh-sanitized.log`, `v061_gui_refresh_sanitized.log` |
| Both pages, assets, Q-Link reachability | All 14 parameters PASS; `ui-verify.log` |
| Preserved v0.6 behavior | Factory declarations, DSP and all 47 skin files byte-identical; `gui-refresh.log` |
| Package | ZIP CRC, exact contents, versions, ARM ELF machine types and in-place folder PASS; `package-verify.log` |

New test: `tests/offline/v061_gui_refresh.cpp`, run by `tools/v061_verify.py`.
It dynamically loads the actual built native library and simulates the documented
host cache, refreshing it only upon the display callback. Before every load,
all controls/cache values are deliberately wrong (including HOLD on before
factory loads). At the callback it verifies all 14 final normalized values and
formatted labels; early/partial and missing/duplicate callbacks fail. Tests
include repeat selection, all programs as bank and preset chunks, initial/manual
state, a saved all-switches-on state, invalid requests, audio processing and a
null-callback host. No automation events are accepted.

**Count correction:** this base declares **20 factory programs**, not 21. The
hotfix preserves those 20 exactly. Testing additionally includes the initial
manual state, yielding 21 distinct base states; a custom switch-state fixture is
also tested. In total, 82 load cases and **2,296 widget-model checks** verify
Knob/Indicator and Value bindings on DELAY/MORE, including both states of all
three switches. The model resolves the real skin bindings; it does not execute
proprietary firmware rendering or touch code.

Existing regressions retain state roundtrips, legacy 12-ID state compatibility,
all 14 NaN/Inf controls, feedback polarity/trim, HOLD/reset/stereo/automation,
factory stress/tails and comb spectral distinction. The compiler reports the
SDK's pre-existing pin short-label `snprintf` truncation warning in some standalone
GCC test builds; this is not a validator/host-sim warning or sanitizer diagnostic.

## Artifact and delivery

- ZIP: `plugin/dist/80s-lofi-delay-box-0.6.1-mpc.zip` — **518,991 bytes**, 56 entries.
- SHA256: `1f741d930bb99c0b4162a3c8dd3f1a0ba20109d5fdd80e648291bdc5e9243ba7`
- Checksums/manifest: `reports/v0.6.1/SHA256SUMS.txt`, `artifacts.json`.
- Changes: `CHANGES-v0.6.1.md`.
- Delivery: `klawsapp:MPC-plugins/early-delay/v0.6.1/` only. Exact ZIP, checksum,
  changes and this report; upload/download comparison evidence in `logs/upload.log`
  and `logs/upload-check.log` (delivery outcome recorded in PROGRESS.md).

Metadata is 0.6.1, binary 601, generated version.xml 0.6.1.0. UID, name and
manufacturer are unchanged. The installer/updater targets the existing
`Flat Peach - VST - 80s Lofi Delay Box` folder. Replace in place under
`VST2 Plugins`; MPC OS 3.10.1+ with Enhancements/External Plugin Support enabled.
The untouched artwork deliberately retains the v0.6 series badge.

## Limits

The missing callback is reproduced and the documented notification path now
passes native tests. Physical MPC redraw, page selection/focus retention,
touch/Q-Link behavior, audio audition and device CPU remain unverified. No
hardware was accessed, deployed to or restarted. No private photos, external
messages or share links were used. This is implementation/test evidence, not an
independent physical-device or reviewer sign-off.
