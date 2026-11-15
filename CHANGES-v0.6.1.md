# 80s Lofi Delay Box — v0.6.1

Flat Peach · metadata **0.6.1** · binary version **601** · unchanged UID **FpEd**

## Preset/program GUI refresh

- Fix stale knobs, live value labels and INVERT/HOLD/STEREOIZE indicators after
  selecting a built-in program or loading a saved preset/project chunk.
- The SDK wrote all new parameter values to the DSP but did not notify the
  host-owned GUI. The plugin now requests one `audioMasterUpdateDisplay` after a
  successful complete load. The SDK documents this callback as re-reading values
  and text and rebuilding the MPC UI page. It covers both DELAY and MORE.
- Re-selecting the same program also refreshes; both bank and preset chunks work.
  Ordinary parameter/automation writes and audio processing do not rebuild the
  GUI. Loading a preset does not generate fake automation events.
- No changes to DSP, parameter order/IDs, factory values, single-tap/Enter
  toggles, labels, Q-Link mappings, skin or product name. The artwork intentionally
  retains its v0.6 series badge. HOLD remains off in all factory programs.

## Verification and count clarification

The supplied v0.6 base contains **20 factory programs**, not 21. No extra program
was invented for this hotfix: tests cover every factory program plus the initial
manual state (21 distinct base states), and a custom all-switches-on state.
The native VST regression reproduces the missing callback on v0.6, then checks
all 14 values/texts at the callback on v0.6.1: 82 program/repeat/chunk loads and
2,296 parameter-bound widget-model checks across DELAY/MORE. These are host-sim
and binding-model checks, not a physical MPC screen test.

See `reports/v0.6.1/REPORT.md` for builds, strict validator, existing regressions,
sanitizers, artifact checks and remaining hardware uncertainty.

## In-place update

Replace **`Flat Peach - VST - 80s Lofi Delay Box`** inside **`VST2 Plugins`** with
this package's folder; do not install side by side. Manufacturer/name/UID are
unchanged, and the SDK updater computes the same destination and replaces it.
MPC OS **3.10.1 or later**, Enhancements (Beta) and External Plugin Support are
required. No device access, deployment or restart was performed by this task.
