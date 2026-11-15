# 80s Lofi Delay Box — Flat Peach

A gritty early-digital stereo delay. Version **0.6.1**, retaining the **v0.6** series artwork.
Flo confirmed v0.5 single-tap interaction on the MPC Live III. This v0.6 update retains
that event topology, with badge-safe labels, restored slide-switch artwork and new presets.
Final v0.6 visual/device performance checks remain pending; no deployment was performed.

It models a free-running-clock digital delay of the early 1980s. It has a fixed block of memory,
and the Time control changes the clock speed:

- Turning **Time** re-pitches what is already in memory, so repeats swoop down when you lengthen the time and up
  when you shorten it. With **Hold** on, Time plays the frozen loop like a pitch control.
- Long times run a slower clock, so they sound darker and more aliased. Short times have about 8 kHz of bandwidth.
- The signal is stored as ~12-bit samples behind a 2:1 / 1:2 compander. The compander's tracking is imperfect, so
  onsets soften and the noise floor breathes with the music. There are op-amp soft clipping stages, and the
  feedback loop saturates.

## v0.6.1 hotfix

Program and saved-state loads now request a host GUI refresh after all parameters
are restored, updating knobs, live values and switches on DELAY and MORE.
DSP, all 20 factory programs, single-tap toggles and all skin files are unchanged.
The native notification/cache regression passes; physical MPC redraw is unverified.

## Controls

| Control | Range | What it does |
|---|---|---|
| Time | 1.75 ms - 900 ms | Clock period. Delay time, and pitch while it moves. The knob has about 70 ms of lag, like a real pot driving a clock VCO. |
| Feedback | -100 % ... +100 % | Repeats. Signed base loop amount; negative reverses each feedback traversal (not the first echo). Invert reverses this sign again. Feedback is capped at ±0.99; the tested bursts decay at both extremes. This is not an unconditional stability guarantee. |
| Invert | Off / On | Flips the signed Feedback amount. +80% with Invert On is -80%; -80% with Invert On is +80%. Does not rewrite frozen HOLD contents. |
| Output (MORE) | -24 to 0 dB | Smoothed post-mix output trim, outside the feedback loop; works during HOLD. No boost or limiter. |
| Mix | 0 - 100 % | Dry / wet. Both are at full level at 50 %. 100 % is wet only. |
| Drive | 0 - 100 % | Input op-amp drive, 0 to +24 dB, partly level compensated. |
| Tone | 0 - 100 % | Low-pass in the repeat path, 700 Hz to 17 kHz. Lower values make each repeat darker than the last. |
| Grit | 0 - 100 % | Converter bits (14 down to 8, about 12 at 35 %), sample-and-hold style, compander mistracking, hiss and clock jitter. |
| Bandwidth | 0 - 100 % | "Age". Converter filter frequency (4.5 to 11.9 kHz, 8 kHz at 60 %) and memory size. Lower values reach the dark, aliased long-delay sound at shorter times. |
| Width | 0 - 100 % | Wet pan separation when Stereoize is on: 0% centred, 100% hard alternating L/R. Displayed as Ping Width; parameter ID/index unchanged. |
| Mod Speed | 0.05 - 10 Hz | Rate of the clock LFO. Its rate and depth drift slightly, like an analog oscillator. |
| Mod Depth | 0 - 100 % | LFO depth on the clock: chorus/flanger at short times, gentle wow at long times (pitch swing is limited to about +/-8 %). |
| Hold | Off / On | Freezes the captured memory. Input and feedback changes do not rewrite it; Time pitches it up/down using bounded resampling. Ping-pong swaps frozen channel banks each loop. |
| Stereoize | Off / On | OFF: ordinary stereo delay, preserving independent L/R input and feedback. ON: (L+R)/2 feeds the left wet bank; cross-feedback produces left at Time, right at 2×Time, then alternating repeats. Shared clocks, no Haas offsets/allpass/antiphase widening. Dry always preserves stereo. |

## Built-in presets

Classic Echo, Slap, Tape-ish Short, Dirty Echo, Dub Throw, Inverted Flange, Long Hold Drone (press Hold to freeze),
Wide Chorus Delay, Broken Clock, Doubler, Metal Comb, Inverted Comb, Bell Comb, Robot Voice,
Ringing Pipe, Flange Comb, Hold Drone Comb, Slow Velvet Chorus, Fast Shimmer Chorus,
Hollow Tape Flange — **20 total**. Save your own versions with the host preset function.

The first ten factory presets use -9 dB Output; the seven resonating comb presets use -12/-15 dB.
Metal/Bell/Pipe/Robot/Flange presets explore 2–12 ms, static/slow/fast modulation,
wide tone/grit/drive differences and signed feedback. Three added chorus/flange variants
use -9/-12 dB output trim. For Hold Drone Comb,
play a sustained sound then tap HOLD; change Time to repitch the frozen loop. HOLD starts OFF so
selecting a preset never freezes empty memory. Output remains accessible on MORE.

Factory stress tests use near-full-scale noise, a swept sine, square wave and multitone followed
by silence. See the v0.6 report for measured sample peaks; these tests are not a guarantee against
arbitrary input levels or high playback volume. Default/manual controls and imported old states
retain neutral 0 dB Output to preserve legacy signal levels; lower Output if needed.

## Install

1. Copy the folder `Flat Peach - VST - 80s Lofi Delay Box` into a folder named `VST2 Plugins` at the root of a drive
   (USB stick, SD card or internal drive) on the MPC.
2. The MPC must run **MPC OS 3.10.1 or later**. Turn on **Preferences > Enhancements (Beta) > Enable Enhancements**
   and **External Plugin Support**, then restart the MPC.
3. Load "80s Lofi Delay Box" by Flat Peach as an insert effect.

For v0.5/v0.6 → v0.6.1, **update/replace the same existing folder**; do not install side by side.
SDK `mpc_remote.py update` computes **Flat Peach - VST - 80s Lofi Delay Box** from the
unchanged manufacturer/name and replaces that folder. No old-folder removal is needed.
UID `FpEd` and parameter IDs/normalization stay unchanged. No device access/update/restart
was performed. For a much older pre-rename Early Delay installation, remove the conflicting
old-name copy as part of a separately authorized migration, not this v0.5 update.

## Status and limits (v0.6)

- v0.6 built and tested offline and in the SDK host simulator. v0.5 taps worked on hardware;
  v0.6 label clearance/contrast, Q-Link interaction, CPU and project reloading need a physical check.
- Desktop tests do not establish device CPU or latency. No numerical ARM CPU guarantee is made.
- Feedback zero gives only the first left echo in ping-pong mode; use nonzero Feedback for travel.
- Ping-pong intentionally mono-sums the wet input; anti-phase stereo material can cancel there. OFF is the choice
  for preserving stereo wet content. Ordinary delay/feedback interference and signed-feedback notches remain possible.
- HOLD uses at most eight aperture taps, not an ideal bandlimited resampler; extreme pitch-up can alias and smear.
  It captures roughly one Time span per channel and alternates the frozen banks in ping-pong. Toggle transitions
  and capture boundaries can be audible. Input is ignored only by the frozen wet memory, not the dry path.
- State/presets persist all 14 controls, not held audio. Existing indices/IDs 0–11 and signed Feedback
  range remain intact; Invert and Output append at 12/13. A 12-ID old state loads exactly into a fresh
  instance with new controls neutral. If importing an old state into an already edited instance, set
  Invert Off / Output 0 dB first: SDK missing-ID loading retains the current values. Seven comb presets are differentiated;
  the first ten presets are unchanged from v0.5 including their -9 dB output trim.
- Invalid audio samples become zero; finite audio is bounded to ±16 before processing. Invalid normalized controls
  use their declared defaults in the audio wrapper. Host storage may still contain the invalid raw value until changed.
- Whole-slot tap toggles, no yellow control focus rectangles, and original AI-generated worn charcoal-blue metal texture
  and restored slide-switch artwork are integrated and statically verified. The two-page skin has all14 touchscreen and Q-Link controls.
  All fixed legends are PNG Image assets; live values remain host-bound labels.
  See CHANGES-v0.6.md and reports/v0.6/REPORT.md for measurements and five demo descriptions.
- The sound is a model built from the published behaviour of this class of unit. It has not been compared with
  recordings of the original hardware yet.

Copyright (c) 2026 Flat Peach. All rights reserved. Built with the inMusic MPC Plugin SDK (MIT licence, notice in
`Content/LICENSES.txt`).
