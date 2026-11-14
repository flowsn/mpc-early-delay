# UI-only testbed

This tiny pass-through effect exercises the SDK Plugin API, as in `sdk/examples/StereoDelay`, with the provisional Early Delay controls. It contains no delay DSP and must not be delivered as the audio product.

Run `python3 design/check_design.py` from the repository root (Pillow required). The native build creates `build/params.json` with host_sim; `params.json` here is the checked-in snapshot of that actual dump, for render-only review. Generated binaries and local `skins/` are ignored. The full harness builds both ARM targets and validates each style separately.
