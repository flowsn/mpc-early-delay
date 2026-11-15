# 80s Lofi Delay Box — v0.5 UI implementation

Refines the approved charcoal/amber hardware design (historical design worktree 97f239c).
Tab is 1280 × 628 at screen y=110; native-size 112/88 px knobs and 278 × 76 px latching
keys remain. Title: **80s LOFI DELAY BOX**, strip version **v0.5**. Current static previews:
`reports/v0.5/preview-delay.png` and `preview-delay-more1.png`; not device screenshots.

## Feedback from the device session

### One tap, one toggle

The old skin already used a Button v2 / `gestureBehaviour: Instant`, but its parent
handled Mouse Down as `Q-Link`. Name/state images and labels covered most of that button.
Thus touches reaching the parent merely selected a control; the exact firmware hit-test
sequence is not observable here. We removed that ambiguity rather than claiming the SDK
Button default was wrong.

All three bools use **one whole-slot Mouse Down → Toggle Switch action**; Enter Pressed
uses the same action. Their children are passive Label/Image/Indicator widgets. No child
Button competes for the event, no slider/drag gesture and no menu overlay is needed.
Indicator v1, indicatorId 1, numIndicatorsInGroup 2, follows the actual bool parameter.
SDK evidence: `sdk/docs/ui-reference.md` §4.2 (Mouse Down/Toggle Switch), §6.4 (Button Instant),
§6.18 (noninteractive Indicator). Indicator is described as less tested by the SDK; actual
single-tap and hardware Enter behavior remain a physical-device confirmation item.

### No yellow selection box

The old `Focus` child used `outlineColour=ffefb34e`, `outlineThickness=1`,
`backgroundColour=19ffffff`, and `WhenFocussed`. It directly explains the yellow control
box after touch. No `Focus` widget remains on either page. Host page selection and Q-Link
bank indicators may still be drawn by firmware; these are distinct from removed skin
focus rectangles. Lit lamp, illuminated lower bar, key brightness, and On/Off text indicate
state. The off/on key outline itself stays the same neutral gray.

## Layout and Q-Links

Interpreted “between NE(?) and Bandwidth” as **between Tone and Bandwidth**.

| Page / bank | Q-Links | Left → right |
|---|---|---|
| DELAY / top | 13–16 | Time, Feedback, Invert, Mix |
| DELAY / middle | 9–12 | Tone, Drive, Bandwidth, Grit |
| DELAY / bottom | 5–8 | Mod Speed, Mod Depth, Hold, Stereoize |
| MORE / top | 13–14 | Ping Width, Output |

Remaining screen slots are unassigned. Program Mode Q-Link 1..14 map to stable parameter
indices 0..13, exactly once each; 15/16 unassigned. Screen maps are resolved by real
parameter names, never guessed IDs. Both MORE controls use matching real knobs.

Feedback remains signed -100..+100%; its label says BASE LOOP SIGN. Invert reverses that
sign: effective feedback = signed Feedback × (Invert ? -1 : +1), then the existing 0.99
loop coefficient cap. Negative Feedback + Invert ON gives positive feedback. This retains
legacy automation, parameter range, midpoint and ID. Invert does not invert the first wet
echo, dry output, or already captured HOLD audio. The seven comb factory presets use
positive magnitudes and the Invert switch for their negative-polarity cases.

## Original aged materials

`design/render_assets.py` generates layered low-frequency oxidation, anisotropic brushed
metal, directional reflection, tiny paint pits, fine scratches, rubbed bare-metal edges,
dust and worn fasteners. Broad control fields stay quiet and legends protected. Latching
keys have recessed push-key artwork, no sliding thumb. SHA-256-derived seeded random
streams, bundled OFL fonts and Pillow produce deterministic assets. No external images,
photo pixels, logos or traced typography were used; private reference photos were not
opened or copied in this update. Their prior material observations were read in the design
handoff only. Installed skin remains below 600 KiB.

## Reproduce

```sh
python3 tools/build.py --target native --skip-test --no-ui --no-validate
python3 tools/make_skin.py --params build/params.json --out plugin/skins --style A --preview reports/v0.5/preview-delay.png
python3 tools/verify_skin.py plugin/skins --params build/params.json
python3 tools/build.py
```

Static composition was inspected for both pages. Text hit-testing, Q-Link bank feedback,
right sidebar overlap, physical contrast, and firmware-drawn highlights need hardware.

## v0.6 hardware-feedback corrections (supersedes v0.5 material notes above)

- Hardware v0.5 taps are confirmed by Flo. Keep the exact whole-slot Mouse Down/Enter
  Toggle Switch topology and passive Indicator. Only its artwork changes back to the
  design-ui vertical slide-switch thumb; no SDK draggable Switch/Slider was introduced.
- The firmware paints the orange Q at the upper-left of the active qlinkBoundsData row.
  SDK ui-reference section3.6 documents that row rectangle but does not expose badge
  dimensions or firmware painting code. In the private photo it intersects the old x34
  label anchor. Reserve a conservative x20..62 envelope in logical1280px coordinates;
  move all top label anchors to slot x+36 (first label x66), leaving4px clearance.
  Previews deliberately draw simulated badges on every occupied row as a worst-case check.
  Exact physical badge extent and post-fix hardware appearance still need confirmation.
- MORE used fixed Label(type=Text, handleName=Invalid), whereas page1 names used PNGs.
  Its label assets were absent by design, not misspelled. Source JSON order/mappings passed
  SDK validation; the photo also lacks other fixed Text captions/header lines. Firmware
  fixed-text rendering is the evidenced failure path, but its internal cause cannot be
  determined offline. Bake ALL fixed text to bound-correct PNG Image entries, preserving
  passive behavior. Live values stay Label(type=Value, handleName=Data). Both pages now
  use the same proven image path; generic fallback labels are generated from real names.
- Top strip now uses original generated photographic material, not procedural line art:
  design/textures/charcoal-original.png. The --texture option allows a replacement source.
  Trim solid padding; preserve chipped top/bottom edges; resample a single strip without
  tiling seams; grade charcoal-blue; glaze text area; quantize96colours. Title/version are
  separate protected text imagery. No reference-photo pixels are included.
- Product0.6.0 / integer600; uiversion remains0.1.0.0 (minimum host format, NOT product).
  Folder remains Flat Peach - VST - 80s Lofi Delay Box; SDK update replaces this folder.

Reproduce with reports/v0.6/preview-delay.png and tools/v06_verify.py (repro directory
  build/v06-repro); see CHANGES-v0.6.md and reports/v0.6/REPORT.md for all measured results.
