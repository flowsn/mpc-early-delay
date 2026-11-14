# Early Delay — touchscreen design handoff

## Recommendation and options

**Recommend A: Charcoal / Amber.** A muted charcoal faceplate, warm legends, amber value windows and a clearly lit STEREOIZE key feel like an original early-digital instrument without copying a commercial product. The restrained colour hierarchy leaves the live values and two latching keys prominent.

| Option | Character | Mock-up |
| --- | --- | --- |
| **A — Charcoal / Amber** | Dark olive-charcoal, warm amber indicators, red HOLD lamp | `option-A-charcoal-amber.png` |
| B — Parchment / Oxide | Cream faceplate, brown fluted knobs, red lamps, ivory button lettering | `option-B-parchment-oxide.png` |
| C — Black / Fluorescent | Black-blue rack panel, blue-white value windows and STEREOIZE lamp, red HOLD lamp | `option-C-black-fluorescent.png` |

All three share exactly the same layout and control mapping. Full-resolution images live in `design/previews/`. They are explicitly **mock-ups, not hardware screenshots**. The renderer walks the generated `TUI.json`, composites its actual PNGs, selects film-strip frames from the testbed's normalised defaults, and uses its value strings. Neutral host header/footer placeholders are outside the plugin's content area. No mock-up labels or option names are included in the installed skin.

Dropbox delivery: `klawsapp:MPC-plugins/early-delay/design/` contains the three PNGs and this document. In that flat Dropbox folder, the filenames in the table open the corresponding mock-ups.

## Geometry and typography

SDK reference: `sdk/docs/ui-reference.md` §§1.4–1.5, 3.6, 6, 7.4, 8. The submodule is pinned at `febd2ca5bd9eb16c4a412837196e145566bc981b`.

- Physical screen: **1280 × 800**.
- Plugin tab: **1280 × 628**, physical origin **(0, 110)**; the bottom host function-key area begins at physical y=738.
- All TUI coordinates are relative to the tab, not the full screen.
- Three horizontal panels; control origins x=30,342,654,966 and y=112,288,464. Each focus slot is 284 × 148. The 28 px gaps between slots and the panel dividers separate controls.
- Primary knobs: 112 × 112, secondary knobs: 88 × 88. These film strips are drawn at native size, never scaled by the Knob widget. Both have 127 frames (frame 63 is the exact bipolar centre).
- HOLD and STEREOIZE: **278 × 76** button hit targets, on/off images with different outlines, LED fill and a lower illuminated bar. Live On/Off text adds a non-colour state cue. These are **latching**, not press-and-hold gestures.
- Value windows: 150 × 48, live values at 28 px; control names 22 px; button names 25 px. Small secondary legends (14–18 px) are optional explanatory text, never the only way to identify or operate a control.
- Live labels use the supported **Titillium Web** family, Regular/SemiBold, not an unsupported bundled font name. The PNG title and mock-ups are rendered from bundled, OFL-licensed Titillium Web files. Firmware text shaping/metrics and value-unit joining can differ slightly from PIL; test those on hardware.
- Rightmost controls extend beyond x≈1025. Explicit row `qlinkBoundsData` is provided, per the SDK. The host's optional right sidebar may still overlay part of the last column: verify sidebar-open behaviour on the target hardware. The normal full-page mock-ups do not imply that the sidebar was tested.

## Parameter and Q-Link map

Names, not provisional indices, are the source of binding. The indices below describe **only the compiled UI testbed**. `tools/make_skin.py` resolves the current dump, then emits fresh `Parameter N` handles, every discrete parameter's `replacementStrings`, and fresh Q-Link maps together.

| Section / row | Design control | Provisional actual name | Stub index | Physical Q-Link | Widget |
| --- | --- | --- | ---: | ---: | --- |
| Performance | TIME | Time | 0 | 13 | 112 px unipolar film-strip Knob |
| Performance | FEEDBACK | Feedback | 1 | 14 | 112 px bipolar film-strip Knob |
| Performance | MIX | Mix | 8 | 15 | 112 px unipolar film-strip Knob |
| Performance | INPUT DRIVE | Drive | 7 | 16 | 112 px unipolar film-strip Knob |
| Tone / grit | TONE | Tone | 4 | 9 | 88 px Knob |
| Tone / grit | GRIT | Grit | 5 | 10 | 88 px Knob |
| Tone / grit | BANDWIDTH / AGE | Bandwidth/Age | 6 | 11 | 88 px Knob |
| Tone / grit | WIDTH | Width (optional) | 9 | 12 | 88 px Knob |
| Mod / space | MOD SPEED | Mod Speed | 2 | 5 | 88 px Knob |
| Mod / space | MOD DEPTH | Mod Depth | 3 | 6 | 88 px Knob |
| Mod / space | HOLD | Hold | 10 | 7 | 278 × 76 Button, red LED |
| Mod / space | STEREOIZE | Stereoize | 11 | 8 | 278 × 76 Button, theme LED |
| Unused fourth bank | — | — | — | 1–4 | Unassigned (-1) |

`Bank Direction` is `RowTopDown`: the first bank is Time / Feedback / Mix / Input Drive; the second is Tone / Grit / Bandwidth-Age / Width; the third is Mod Speed / Mod Depth / Hold / Stereoize. Program Mode uses the same physical map as the main page, so the main controls remain consistent. Each row has a single highlight rectangle enclosing all four slots. No parameter is assigned to more than one control.

### Name changes, extra parameters and missing controls

The authoritative layout, aliases and canonical keys are in `design/layout.json`. Matching ignores case, spaces and punctuation, but **does not fuzzy-match arbitrary names**. If two real names match the same slot, generation fails with an explanation rather than silently binding the wrong control. Add an explicit mapping file if needed:

```json
{"time": "Clock Time", "age": "Converter Age", "drive": "Input Saturation"}
```

Pass it with `--mapping design/final-mapping.json`. Values must be exact real names from the dump. Missing main-page controls are omitted, including their Q-Link assignments; optional Width therefore does not leave a fake knob. Blank space is intentional and can be reused after the final parameter list is agreed.

Every unmatched parameter is placed on **MORE**, **MORE2**, etc., in native 4 × 4 slider pages with name/value text and row-wise Q-Link groups. Discrete controls get a MenuOverlay and choice strings; continuous controls get a KnobOverlay. All fallback pages receive mock-ups when `--preview` is supplied. The provisional set fits on one page, so no second-page mock-up is needed for these three options.

HOLD/STEREOIZE become large toggle Buttons only when the dump reports exactly two discrete states. If the final implementation makes either continuous or multi-choice, it becomes a knob with the appropriate overlay instead of silently quantising it to On/Off. Confirm semantic changes with the DSP owner.

### Negative feedback / polarity

The feedback knob has a centre-zero arc, visible − / 0 / + legend and **the plugin's signed host value**. The stub's illustrative −35% shows the inverted-feedback case. This assumes the final feedback parameter has a symmetric bipolar normalisation with zero at 0.5.

No guessed dynamic polarity lamp is shipped. The dump does not expose a reliable arbitrary zero-crossing threshold, and the SDK conditional-visibility recipes are for known discrete states, not an exact signed numeric comparison. If the DSP eventually exposes a separate polarity bool/choice, an `IndexedEnabling` label can be added against that real parameter. Until then the signed value is the accurate indication. There is no `Indicator` widget in the delivered skin.

## Reproduce / apply to the final plugin

Commands run from the repository root, after this branch has been integrated by the parent. Nothing in this design branch changes `plugin/` or the audio implementation.

### Dependencies (once)

```sh
git submodule update --init
python3 -m venv design/.venv
design/.venv/bin/pip install -r design/requirements.txt
```

Pillow is pinned to 12.1.1. There is no NumPy dependency. Fonts are included, so regeneration does not fetch images or fonts. The SDK may install its Zig compiler the first time a build runs.

### Final plugin: dump real parameters, generate, build

```sh
python3 sdk/tools/build.py plugin --target native --no-ui --no-validate
design/.venv/bin/python tools/make_skin.py --params plugin/build/params.json --out plugin/skins --style A
python3 sdk/tools/build.py plugin
design/.venv/bin/python tools/verify_skin.py plugin/skins --params plugin/build/params.json
python3 sdk/tools/validate.py plugin --strict
```

The second line is the **one-command regeneration** once `plugin/build/params.json` exists. Select `--style B` or `--style C` for the other option. The build copies `plugin/skins/` verbatim to the final `Plugin Skins/` directory and writes `uiversion` 0.1.0.0. Regenerate after any parameter list/name/order changes; do not copy the testbed's hardcoded output into a different plugin without regeneration.

To write directly to a complete skin directory and make a current preview:

```sh
design/.venv/bin/python tools/make_skin.py --params plugin/build/params.json --out 'build/final/Plugin Skins' --style A --preview build/final-preview.png
```

The generator validates bindings, image sizes, bounds, overlay references, duplicate overlay version keys and complete touchscreen/Q-Link coverage before returning. `skin-map.json` beside the skin provides an auditable resolved-name/index map and lists omitted slots and fallback controls.

### Swap the product title

```sh
design/.venv/bin/python tools/make_skin.py --params plugin/build/params.json --out plugin/skins --style A --title 'YOUR FINAL NAME'
```

Only the **single `logo_plate.png` asset** carries the product title in the skin. It is referenced by one Image entry at `30 10 500 64`; it can also be replaced by a PNG with those exact dimensions. This operation does **not** change plugin vendor/name/UID, binary filenames or existing project identity. Change the default title in `design/layout.json` after naming approval. The subtitle is in the same logo asset.

### Reproduce the full test harness and all mock-ups

```sh
design/.venv/bin/python design/check_design.py
```

This compiles a tiny SDK pass-through effect in `design/testbed/`, dumps its actual parameter list, renders all assets/options/previews, builds ARM32 and ARM64, runs SDK strict validation separately for each skin, runs mapping regression tests, and runs the native host simulator. It leaves A applied to the local testbed output. The testbed is **not a delay and is not the final audio plugin**.

For a render-only review without compiling, `design/testbed/params.json` is a checked-in snapshot produced by the compiled stub's host_sim dump:

```sh
design/.venv/bin/python tools/make_skin.py --params design/testbed/params.json --out 'build/review/Plugin Skins' --assets-out build/review-assets --style A --preview build/review-A.png
```

## Asset and source inventory

| Source / generated asset | Purpose / dimensions |
| --- | --- |
| `design/layout.json` | Canonical controls, aliases, captions, default title |
| `design/render_assets.py` | Deterministic original PIL artwork and colour palettes |
| `design/render_preview.py` | Composites the generated TUI, not a second hand-maintained layout |
| `tools/make_skin.py` | Name-based resolver, complete TUI and 16-Q-Link generator |
| `tools/verify_skin.py` | Binding, geometry, assets, overlays, coverage checks |
| `design/test_skin.py`, `design/check_design.py` | Regression tests and full build/validate harness |
| `design/testbed/src/DesignTestbed.cpp` | Stand-in effect, SDK API, 12 provisional parameters |
| `design/assets/{A,B,C}/panel.png` | 1280 × 628 faceplate |
| `logo_plate.png` | 500 × 64, single title asset |
| `value_plate.png` | 150 × 48 recessed display background |
| `knob_112_uni.png`, `knob_112_bi.png` | 112 × 14224 strips, 127 native-size frames |
| `knob_88_uni.png`, `knob_88_bi.png` | 88 × 11176 strips, 127 frames; small bipolar is available for future controls |
| `hold_on.png`, `hold_off.png` | 278 × 76, red-lamp latching key states |
| `stereoize_on.png`, `stereoize_off.png` | 278 × 76, theme-lamp latching key states |
| `led_hold_on/off.png`, `led_lamp_on/off.png` | 24 × 24 standalone lamps, generated for reuse; not needed separately by this TUI |
| `design/skins/{A,B,C}/Plugin Skins/` | Complete generated skin and only its referenced PNGs |
| `design/previews/option-*.png` | 1280 × 800 mock-ups, generated from each TUI |
| `design/validation/` | Actual SDK, binding, host-sim and regression reports |
| `design/fonts/` | Titillium Web Regular/SemiBold, bundled OFL license |

Artwork is original procedural drawing, with no proprietary firmware assets or commercial marks. The UI generator imports the pinned SDK's public schema/serialization helpers under its MIT license; this preserves the MPC-required **duplicate `version` keys in overlay data**. Do not round-trip TUI through ordinary `json.dump`, which would lose those duplicate keys. The preview renderer can read with normal JSON because it does not rewrite the skin.

Font upstream: <https://github.com/google/fonts/tree/main/ofl/titilliumweb>. Font license: `design/fonts/OFL.txt`. Runtime label fonts are supplied by the host; font files are build-time inputs, **not copied to the device**.

Film strips use a deterministic 192-colour indexed PNG palette with alpha. This keeps the complete delivered option to approximately **287–291 KiB**, including its JSON and ten PNGs; all four knob strips and reusable LED assets are generated separately in `design/assets/`. No preview image or source font is needed on the device.

## Verification and remaining hardware checks

Verified locally on 2026-11-14:

- Native, ARM32 and ARM64 testbed builds succeeded; both ARM binaries export `VSTPluginMain` and require no GLIBC above 2.27.
- **All three complete options: SDK strict validator 0 errors, 0 warnings.** See `design/validation/SDK-A.txt`, `SDK-B.txt`, `SDK-C.txt`.
- All 12 parameters have touchscreen controls and Q-Links; all handles, component definitions, referenced images and overlays resolve; dimensions and row-highlight bounds checked.
- Six regression tests pass: reorder independence, aliases/optional Width, 21 additional parameters across fallback pages, nonboolean HOLD safety, ambiguity handling, and missing-asset rejection.
- Native pass-through stub host simulator: **20 passed, 0 warnings, 0 failures**. This is a build/interface proof, not a sound-quality test of the delay.
- Main mock-ups visually inspected at screen scale; B's brown button/pointer contrast corrected. No hardware interaction has been verified.

Delivered runtime widgets — **Image, Knob, Label, Button, Focus, Slider, KnobOverlay, MenuOverlay** — are all marked *well-trodden* in the SDK reference. No *less-tested* widget is required. In particular, Indicator/Path/Plotter/XYPad and horizontal film-strip faders are not used. First hardware check should cover:

1. HOLD/STEREOIZE toggle by touch, Q-Link and Enter menu. `gestureBehaviour: Instant` intentionally does not take data-wheel edits while held; SDK ui-kit notes flag that behaviour for first-hardware verification.
2. RowTopDown banking, row focus rectangles, and last-column visibility with the optional host Q-Link sidebar open.
3. Value text/units and longest final plugin strings; the 28 px readouts do not auto-shrink. Firmware uses its own font rendering.
4. Indexed PNG transparency and frame selection at minimum/maximum/zero, including negative feedback. Confirm zero=0.5 and the DSP's negative=inverted convention.
5. Real STEREOIZE widening and HOLD behaviour are implemented by the DSP, not the skin. Validate with the finished audio plugin on the device.

Client decisions still open: **A/B/C**, **final product name**, and whether **Width** remains a separate control. Final parameter naming/ranges come from the DSP owner; regenerate against that dump before integration.
