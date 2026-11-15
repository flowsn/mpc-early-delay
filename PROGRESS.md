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

Milestone 3 COMPLETE verification/package (2026-11-15 02:14UTC): final integrated
next_check --sanitize includes v06_presets, all5sanitized harnesses PASS; all20state
roundtrips/all14invalid controls/zero callback allocations. Strict validator0errors/0warnings;
all47skin files and both previews byte-identical. Three previews visually inspected.
Fixed verifier's stale0.5.0 expectation; final UI check PASS without relaxing criteria.
ZIP:plugin/dist/80s-lofi-delay-box-0.6-mpc.zip (520478bytes), SHA256
a7fac3228feb7d390bb57fca9a1b219508d0a95281eb02b7932bdbd3e9b665b2.
5WAVs/ZIPCRC/ARMELF/metadata/unchanged installer folder/skin/content verified.
Report:reports/v0.6/REPORT.md. Milestone2:c87e291 pushed.
REMAINING: exact11file private rclone upload+download-check, then final checkpoint.

Milestone4 COMPLETE private delivery (2026-11-15 02:15:55UTC).
Verified package commit:a7d082f pushed; earlier4196e5a/c87e291 pushed.
Uploaded exactly11deliverables to klawsapp:MPC-plugins/early-delay/v0.6/.
rclone check --one-way --download:11matching files,0differences. Evidence:
reports/v0.6/logs/upload.log and upload-check.log. No share links.
TASK COMPLETE. No device access/deploy/remove/restart or external messages occurred.
Only physical v0.6 visuals/Q-Link/contrast/performance/reload/audition remain unverified.
All files committed; final checkpoint pushed to own branch only.
