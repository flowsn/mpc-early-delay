# 80s Lofi Delay Box — v0.6

Flat Peach · metadata **0.6.0** · unchanged UID **FpEd**

## Screen fixes

- **Q badge clearance:** every top label starts32 logical pixels farther right. A conservative
  42px host-badge envelope no longer intersects TIME, TONE, MOD SPEED or PING WIDTH.
  Both preview pages include simulated Q badges to expose that clearance.
- **MORE names restored:** PING WIDTH and OUTPUT previously used fixed host Text labels,
  unlike page1's PNG names. The hardware also omitted other fixed captions. All fixed text
  now uses actual PNG Image assets; live values remain parameter-bound host labels. Both
  pages pass asset/mapping/bounds checks. Firmware's internal fixed-Text failure is not accessible.
- **Slide-switch appearance restored** from design-ui for INVERT/HOLD/STEREOIZE: the thumb
  visibly moves and the lamp changes. The v0.5 hardware-confirmed whole-slot single-tap/Enter
  Toggle Switch handlers are unchanged; a passive Indicator shows the artwork, not a drag slider.
- **Realistic charcoal-blue strip:** original AI-generated painted-metal texture, chipped silver
  edges, scratches, dust and grime. Edge-preserving resize, padding removal, subtle blue grade,
  protected title **80s LOFI DELAY BOX / v0.6**. No private-photo pixels, logos or brand names.
  Installed skin: **508,178 bytes /496.27 KiB**. Deterministic generator; replacement via `--texture`.

## Twenty presets: seven differentiated combs, three new chorus/flange variants

The first10 presets (including Inverted Flange, Wide Chorus Delay and Doubler) are unchanged.
Seven combs now cover static resonance, slow sweeps, fast flutter, dark pipes and stereo capture.
All14 parameter IDs/order and signed-feedback normalization stay compatible.

| Comb preset | Time ms | Feedback / Invert | Mod Hz / depth% | Tone / BW / grit / drive % | Stereo / width% | Output dB | Measured peak Hz / centroid Hz |
|---|---:|---|---|---|---|---:|---|
| Metal Comb |4|+88 / off|0.05 /0 (static)|95 /95 /12 /8|off /60|-12|772.5 /3578.6|
| Inverted Comb |6|-82 /off|0.12 /38 (slow)|72 /78 /20 /15|on /85|-12|635.2 /2771.6|
| Bell Comb |2|+96 /off|0.05 /0 (static)|100 /100 /4 /0|off /40|-15|1063.2 /3627.7|
| Robot Voice |12|-72 /off|7.5 /60 (flutter)|58 /45 /70 /32|off /50|-15|134.6 /2143.1|
| Ringing Pipe |9|+93 /off|0.35 /12|38 /62 /18 /10|off /60|-15|667.5 /1096.1|
| Flange Comb |2.8|+80 /on (effective -80)|0.8 /90|88 /90 /32 /18|on /100|-12|554.5 /3508.9|
| Hold Drone Comb |11|+97 /off|0.09 /75|65 /72 /48 /22|on /75|-15|234.2 /2096.9|

HOLD deliberately starts **off in every preset**: loading an already-held empty memory would
be silent. Hold Drone Comb is the capture variant: play into it, then single-tap HOLD; the demo
captures at2s and repitches at4s/6s. Stereoize, time, tone and modulation differ before capture.

Spectral proof: actual wrapper, identical deterministic broadband input, Mix temporarily100%
for isolated wet measurement; other parameters remain factory values. 44.1kHz,2s warmup,
8 nonoverlapping Hann16384 windows, summed L/R power. Peak search70–4000Hz; centroid70–8000Hz.
The peak is the strongest measured resonance/harmonic, **not** an assumed `1/time` fundamental.
All21 pairs differ in normalized spectral shape: minimum L1 distance **0.46667** (0=identical,
2=disjoint; test requires>0.1). This is objective differentiation, not a physical listening verdict.
Evidence: `reports/v0.6/logs/v06-presets.log`, `tests/offline/v06_presets.cpp`.

New additions:

| Preset | Time ms | Feedback% | Mod Hz / depth% | Character | Output dB |
|---|---:|---:|---|---|---:|
| Slow Velvet Chorus |32|+10|0.18 /65|dark, wide and slow|-9|
| Fast Shimmer Chorus |18|+22|3.2 /32|bright, fast, cleaner converters|-9|
| Hollow Tape Flange |4.8|-62|0.09 /95|deep slow negative-feedback sweep|-12|

Every factory program is stress-tested at near-full-scale input with noise/sweep/square/multitone
and20s silence. Worst sample peak **-2.080 dBFS**, revised combs **-4.777 dBFS or lower**;
all tails below-50dBFS after20s. 30s held comb stays finite/sustained; Output still works held.
These are sample-peak tests, not a limiter or oversampled true-peak guarantee.

## Five demos

Stereo44.1kHz/16-bit PCM, actual factory trims, no loudness normalization. Only10ms boundary fades.

| File | Duration | Content / preset order | Sample peak dBFS |
|---|---:|---|---:|
|01-seven-combs-comparison.wav|21s|3s each: Metal, Inverted, Bell, Robot, Pipe, Flange, Hold Drone; identical plucks|-13.71|
|02-chorus-flange-tour.wav|20s|4s each: Wide Chorus, Inverted Flange, Slow Velvet, Fast Shimmer, Hollow Tape|-12.16|
|03-robot-voice-flutter.wav|8s|synthetic voiced harmonics into fast flutter|-19.56|
|04-stereo-flange-plucks.wav|8s|wide stereo negative-feedback sweep|-13.80|
|05-hold-drone-capture.wav|8s|capture at2s, Time11→5.5→16.5ms at4s/6s|-15.24|

## Update installation

Folder stays **`Flat Peach - VST - 80s Lofi Delay Box`**. **Update/replace v0.5 in that same
folder**, not a new side-by-side install. SDK `mpc_remote.py update` computes this exact name
from the unchanged manufacturer/product and replaces the existing folder (`replace=True`).
No old-folder removal is needed when updating v0.5. No install, removal, device access or restart
was performed. UID remains FpEd; project parameter IDs and normalization are unchanged.

All requested build, validator, host-sim, regression/sanitizer and byte-reproduction results are
recorded in `reports/v0.6/REPORT.md`. Hardware still needs final visual/Q-Link/contrast checks;
previews are static mock-ups, not device screenshots. No physical audio audition was claimed.
