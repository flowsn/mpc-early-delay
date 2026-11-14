# Architecture — next stereo ping-pong candidate

## Signal and memory

Two 65,536-cell buffers plus gain side channels are allocated in prepare, never process. Each channel keeps
input conditioning, pre-emphasis, gain-tracked compression, clock-tracking anti-alias filters, 14–8 bit
quantization (~12 at default Grit), reconstruction filtering, loss-limited expansion, de-emphasis and Tone.
The stored gain metadata is a pragmatic compander model, not a validated physical circuit emulation.

Bandwidth sets 2,048–32,768 nominal cells. Normal playback/write clock is N/T, capped at the host rate;
short delays reduce the used span instead. Time smoothing is ~70 ms. Coefficients and memory mapping are
updated every 16 frames; no per-sample pow/exp/sin. Per-sample compander sqrt remains a profiling target.

## Stereo contract

Always two inputs and two outputs. OFF processes L and R independently with self-feedback and shared clocks.
ON linearly transitions the injection to `(L+R)/2 -> L`, zero new input to R, and switches feedback to the other
channel's previous-frame wet sample. Both feedback samples are captured before processing either channel,
so processing order does not introduce asymmetric timing. Echo 1 is L at T; echo 2 R at 2T; echo 3 L at 3T.
Feedback sign is applied on each traversal (first echo is not feedback-inverted). Zero feedback means only
one left echo. Width is solely a post-loop pan matrix: 0 centres both banks, 1 leaves them hard-panned.
Dry is outside that matrix. Modulation and jitter share timing: no deliberate opposing offsets/side high-pass.
Stereo wet sources are deliberately mono-summed in ON; use OFF to retain their separation. Natural delayed
signal interference remains possible, especially signed feedback; there is no blanket mono-cancellation claim.

## HOLD

Capture records write-head/start/span metadata and stops cell writes. Interpolated playback wraps the immutable
ring in constant work per output frame (one tap at <=1 cell/frame, eight aperture taps above). It does not
iterate once per skipped cell. Ping-pong alternates which frozen channel bank is read each loop; in ordinary
stereo each channel repeats its own bank. The clock can exceed the host rate (bounded to 1024 cells/frame,
above ratios reachable in the supported 44.1 kHz/Time range). This is a gritty approximate aperture filter,
not ideal anti-aliasing. New input and feedback have no effect on frozen contents. Feedback polarity already
inside the capture persists. Release restores writes, glides the memory span and uses a 2 ms output crossfade;
short-loop boundary/toggle artifacts are possible. Frozen audio is not serialized in presets/projects.

## Robustness and lifecycle

Reset clears memory and recursive state, resets clock ratio/control counter/RNG/LFO and forces a fresh snapshot.
The wrapper rejects non-finite normalized controls before logarithmic/integer conversion, using declared defaults;
engine setParams separately rejects non-finite/out-of-range plain controls. Non-finite input becomes zero before
dry/wet branching; finite input is limited to ±16. Final recursive guard resets affected filter/compander states
without buffer allocation or scanning. Boundedness and decay are empirical checks, not a proof for arbitrary
nonlinear time-varying feedback. Host's raw parameter storage is not modified by the audio callback.

## Identity and UI

UID FpEd, manufacturer Flat Peach, name Early Delay. Twelve permanent parameter IDs and indices, eleven original
program value arrays. Width keeps ID width/index 11 and is labelled Ping Width. Approved design 97f239c is
integrated as deterministic distressed charcoal/amber skin; all controls, including the MORE-page width knob,
are reachable. Program Q-Links enumerate all 12 controls. Static inspection is not device interaction testing.
