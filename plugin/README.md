# Early Delay - Flat Peach

A gritty early-digital delay for Akai MPC standalone hardware (built and tuned for the MPC Live III).
Effect, stereo in / stereo out. Version 0.1.0-alpha.

It models a free-running-clock digital delay of the early 1980s. It has a fixed block of memory,
and the Time control changes the clock speed:

- Turning **Time** re-pitches what is already in memory, so repeats swoop down when you lengthen the time and up
  when you shorten it. With **Hold** on, Time plays the frozen loop like a pitch control.
- Long times run a slower clock, so they sound darker and more aliased. Short times have about 8 kHz of bandwidth.
- The signal is stored as ~12-bit samples behind a 2:1 / 1:2 compander. The compander's tracking is imperfect, so
  onsets soften and the noise floor breathes with the music. There are op-amp soft clipping stages, and the
  feedback loop saturates.

## Controls

| Control | Range | What it does |
|---|---|---|
| Time | 1.75 ms - 900 ms | Clock period. Delay time, and pitch while it moves. The knob has about 70 ms of lag, like a real pot driving a clock VCO. |
| Feedback | -100 % ... +100 % | Repeats. Negative values invert the polarity of every repeat (hollow flanger at short times). Even at the extremes, the repeats always die away. |
| Mix | 0 - 100 % | Dry / wet. Both are at full level at 50 %. 100 % is wet only. |
| Drive | 0 - 100 % | Input op-amp drive, 0 to +24 dB, partly level compensated. |
| Tone | 0 - 100 % | Low-pass in the repeat path, 700 Hz to 17 kHz. Lower values make each repeat darker than the last. |
| Grit | 0 - 100 % | Converter bits (14 down to 8, about 12 at 35 %), sample-and-hold style, compander mistracking, hiss and clock jitter. |
| Bandwidth | 0 - 100 % | "Age". Converter filter frequency (4.5 to 11.9 kHz, 8 kHz at 60 %) and memory size. Lower values reach the dark, aliased long-delay sound at shorter times. |
| Width | 0 - 100 % | How far apart the two sides move when Stereoize is on. |
| Mod Speed | 0.05 - 10 Hz | Rate of the clock LFO. Its rate and depth drift slightly, like an analog oscillator. |
| Mod Depth | 0 - 100 % | LFO depth on the clock: chorus/flanger at short times, gentle wow at long times (pitch swing is limited to about +/-8 %). |
| Hold | Off / On | Infinite repeat. The memory loops on itself without loss, and the input is not recorded. |
| Stereoize | Off / On | Gives the two sides slightly different times, opposite LFO phases, different bandwidths and independent jitter. Bass stays centred, and the mono sum has no cancellation notches. With Stereoize off, a mono input stays exactly mono. |

## Built-in presets

Classic Echo, Slap, Tape-ish Short, Dirty Echo, Dub Throw, Inverted Flange, Long Hold Drone (press Hold to freeze),
Wide Chorus Delay, Broken Clock, Doubler, Metal Comb. Save your own versions with the MPC's own preset function.

## Install

1. Copy the folder `Flat Peach - VST - Early Delay` into a folder named `VST2 Plugins` at the root of a drive
   (USB stick, SD card or internal drive) on the MPC.
2. The MPC must run **MPC OS 3.10.1 or later**. Turn on **Preferences > Enhancements (Beta) > Enable Enhancements**
   and **External Plugin Support**, then restart the MPC.
3. Load "Early Delay" as an insert effect.

## Status and honest limits (0.1.0-alpha)

- Built and tested offline and in the SDK's host simulator. **Not yet tested on an MPC.** On-device CPU, the touch
  UI and the Q-Link mapping are still to be checked.
- CPU: about 1.2 % of one desktop x86 core per instance. On the Live III's ARM cores, expect several times that
  (estimated 4-8 % of one core; not measured).
- The sound is a model built from the published behaviour of this class of unit. It has not been compared with
  recordings of the original hardware yet.

Copyright (c) 2026 Flat Peach. All rights reserved. Built with the inMusic MPC Plugin SDK (MIT licence, notice in
`Content/LICENSES.txt`).
