# v0.6 UI/presets — durable progress

Branch: v0.6-ui-presets, from v0.5-hardware-feedback at 475b9cb.
Single code/skin owner. Private origin via github-mpc-delay, this branch only.
No device deployment/restart, external messages, or public sharing authorized.

Milestone 1: sources implemented; native build + real parameter dump PASS (20 programs).
- Metadata 0.6.0 / integer600, UID FpEd and display/manufacturer/install folder unchanged.
- Name anchors x36 (was4) leave a conservative42px badge area at each row start.
- MORE labels previously used fixed Text with Invalid handle; page1 names were images.
  Hardware photo also lacks fixed captions/header text. Exact firmware cause unavailable;
  bake all fixed text as passive images; live Value labels remain host-bound.
- Restored design-ui sliding thumb artwork for Invert/Hold/Stereoize, retaining v0.5
  whole-slot Mouse Down/Enter Toggle Switch and passive Indicator, no drag widget.
- 7 differentiated comb presets; first10 unchanged; 3 appended chorus/flange variants.
  HOLD stays initially OFF so a fresh preset can accept sound before capture.
- Original AI-generated texture received and copied to design/textures/charcoal-original.png.
  Edge-preserving resize/padding trim/charcoal-blue grade/96-colour compression pipeline.
  Private hardware photo viewed for diagnosis only, never copied/committed/uploaded.

IN PROGRESS: skin generation, preview review, spectrum/preset peak measurements and demos;
full native/ARM32/ARM64/strict validator/all7 hostsim/offline/sanitizer/repro gates, package,
CHANGES-v0.6.md, private rclone delivery/check. Logs will be reports/v0.6/logs/.

Milestone 2: texture integrated, both previews visually inspected, 47 skin files/508178bytes
(496.27KiB). All20 factory peak tests PASS; worst-2.080dBFS, combs<=-4.777dBFS.
New wet-spectrum measurement: all7 peaks distinct, all21 normalized spectrum pairs L1>0.4666;
5 new demos rendered (all below-12dBFS). Native/ARM32/ARM64 build and all7 hostsim sizes PASS
(20pass/0warn/0fail each); baseline offline PASS; initial next_check --sanitize PASS.
New v0.6 spectrum/demo harness is being added to the permanent runner and sanitized separately.
Source milestone1:4196e5a pushed. Reproduction and final package/delivery still pending.
