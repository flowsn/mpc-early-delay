# v0.5 hardware-feedback update — durable progress

Branch: v0.5-hardware-feedback, forked from c2a90e0. Private origin via github-mpc-delay only.
Single code/skin owner. No hardware deployment or external messaging permitted.

Milestone 1: implementation sources complete; native build + real parameter dump PASS.
- Product 80s Lofi Delay Box / Flat Peach, version 0.5.0, UID FpEd unchanged.
- Existing indices 0..11, IDs and signed Feedback normalization preserved. Appended Invert (12)
  flips signed feedback; Output (13) -24..0 dB post-mix trim, smoothed outside the loop.
- 17 presets (seven comb-focused), new comb presets have -12/-15 dB output trim.
- Page 1: Time / Feedback / Invert / Mix; Tone / Drive / Bandwidth / Grit;
  Mod Speed / Mod Depth / Hold / Stereoize. MORE: Ping Width and Output.
- SDK ui-reference §§4.2/6.4/6.18: whole-slot Mouse Down -> Toggle Switch; passive Indicator
  avoids overlaid labels stealing an inner Button tap and avoids double toggles. Enter toggles too.
  Removed explicit yellow Focus widgets. All state cues are key brightness/lamp/On-Off text.
- Original seeded brushed-metal patina renderer with oxidation, scratches, edge wear, dust;
  no private reference photographs read/copied/uploaded. Title and v0.5 on strip.

IN PROGRESS: generated skin, preview inspection, v0.5 regression/peak demos, full native/ARM/validator/
all-block hostsim/sanitizer gates, reproducibility, package + checksums + private rclone delivery.
Important: tools/build.py now selects manifest's exact native library, not a stale pre-rename .so.
Read reports/next-20261114/REPAIR.md for baseline evidence; current checks go under reports/v0.5/.

Milestone 2: initial generated skin validated (14/14 controls, two pages, ~451 KiB) and
both page previews visually inspected. Native/ARM32/ARM64 + SDK validator 0 errors/0 warnings;
all seven host simulator block sizes 20 passed/0 warnings/0 failures. Existing offline selftest
and tools/next_check.py --sanitize PASS (17 programs, all14 NaN/Inf controls).
New actual-wrapper stress test exposed +4.5..+6.9 dBFS peaks in inherited factory presets
at near-full-scale input. Fixed by appending -9 dB Output to first ten presets (old controls
unchanged); comb presets -12/-15 dB. Revised test/demos and reproducibility in progress.
No feedback loop code or legacy Feedback normalization changed. New docs/DESIGN.md records
actual SDK touch evidence and the exact focus-highlight cause; hardware tap remains unverified.
