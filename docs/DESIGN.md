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
