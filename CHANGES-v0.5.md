# 80s Lofi Delay Box — v0.5

By Flat Peach · metadata 0.5.0 · stable UID `FpEd`

- **Single-tap keys:** HOLD, STEREOIZE and new INVERT toggle from the whole touch target.
  Lit key/lamp and On/Off text show state; the yellow control-focus box is removed.
- **INVERT beside Feedback:** reverses signed Feedback; negative + INVERT gives positive.
  Signed range/old IDs stay compatible. INVERT and Output append as indices 12/13.
- **Tone → Drive → Bandwidth** on page 1. MORE has Ping Width and new -24..0 dB Output trim.
- **17 presets**, including Metal Comb, Inverted Comb, Bell Comb, Robot Voice, Ringing Pipe,
  Flange Comb and Hold Drone Comb. Play into Hold Drone Comb, then tap HOLD to capture.
  Factory output trims are -9 dB (original ten) or -12/-15 dB (comb bank).
- **Original aged brushed-metal artwork:** fine scratches, edge wear, dust and oxidation,
  protected legends, push-key artwork, title and v0.5 on the top strip. No photo textures copied.
- Native/ARM32/ARM64 builds, strict validator, seven host-sim block sizes, stereo/HOLD/reset/NaN,
  sanitizers, state roundtrips, zero callback allocations and reproducible skin checks.
  Worst factory stress sample peak **-2.080 dBFS**; seven combs **-4.698 dBFS or lower**.

## Replacing the old version

New installed folder: **`Flat Peach - VST - 80s Lofi Delay Box`**.
Remove the old **`Flat Peach - VST - Early Delay`** folder with `mpc_remote.py remove`
before installing the new one; the stable UID makes side-by-side discovery ambiguous.
Old 12-control states retain their values in a fresh instance, with Invert Off / Output 0 dB.
When importing into an edited instance, set those two controls neutral first.

No device deployment/restart was performed. The prior version ran on MPC Live III; v0.5
single-tap behavior, firmware highlights, Q-Links and device performance still need confirmation.
Previews are mock-ups, not hardware screenshots. Five synthetic 8-second demos are included
separately; HOLD demo captures a drone and changes Time to repitch it.
