# Open303 r5 reference notes

Open303 r5 is used as a technical reference for the stock ACID voice. It is not copied wholesale. The ReFmB1rth implementation remains a compact, deterministic, allocation-free fixed-point engine for the FM-1.

Reference source:

- https://sourceforge.net/p/open303/code/HEAD/tree/Source/DSPCode/rosic_Open303.cpp
- https://sourceforge.net/p/open303/code/HEAD/tree/Source/DSPCode/rosic_TeeBeeFilter.cpp

Open303 is MIT-licensed; attribution and the retained license notice are recorded in `THIRD_PARTY_DSP.md`.

## Adopted behavior

### Measured cutoff / Env Mod law

r5 uses measured TB-303 constants instead of a generic linear filter-envelope amount. The important constants are:

- nominal cutoff low: `313.8152786059267 Hz`
- nominal cutoff high: `2394.411986817546 Hz`
- offset factor: `0.048292930943553`
- offset constant: `0.294391201442418`
- low-cutoff scaler factor: `3.773996325111173`
- low-cutoff scaler constant: `0.736965594166206`
- high-cutoff scaler factor: `4.194548788411135`
- high-cutoff scaler constant: `0.864344900642434`

The FM-1 implementation converts these to fixed-point constants and uses an exponential 33-point cutoff LUT. This avoids `log`/`pow` on pi32v2 while retaining the measured response shape.

### Envelope timing

r5 defaults / behavior used as calibration anchors:

- normal filter envelope decay: about `1000 ms`
- accented filter envelope decay: about `200 ms`
- amplitude envelope decay: about `1230 ms`
- normal amp release: about `1 ms`
- accented amp release: about `50 ms`
- filter-envelope RC smoothing stage: about `15 ms`

The DECAY knob still controls normal filter-envelope duration in ReFmB1rth; accent switches the active filter-envelope decay to the short ~200 ms path.

### Slide

Open303 exposes a nominal `60 ms` slide and feeds `0.2 * slideTime` into its pitch slew limiter. The fixed-point implementation therefore targets an approximately `12 ms` exponential-style slew constant, while preserving the user-facing/classic nominal slide behavior.

A slid note does not retrigger the main filter envelope, but changing into/out of an accented slide changes accent gain/decay/release state.

### Idle reset

Open303 resets oscillator/filter state only when a new phrase starts from idle. ReFmB1rth now follows that rule: normal retriggers inside an active phrase preserve oscillator/filter state, while a fresh note after the VCA has reached silence resets the analog-path state.

### Filter/output path

Already adopted or retained:

- four-stage TeeBee/diode-ladder-inspired topology;
- resonance feedback high-pass around `150 Hz`;
- 2x oscillator/filter processing;
- bounded feedback non-linearity;
- output AC-coupling/high-pass behavior calibrated near r5's `44.486 Hz` stage.

## Intentionally not copied yet

r5 also contains experimental/tweakable conditioning around the core model:

- a second high-pass around `24.167 Hz`;
- an all-pass around `14.008 Hz`;
- a very-low-frequency notch around `7.5164 Hz`;
- a 200 Hz 12 dB amplitude de-clicker;
- additional generic envelope/sequencer/VST scaffolding.

These are **not** automatically part of the FM-1 model. Each additional stage must demonstrate an audible/reference benefit and must pass the FM-1 CPU/headroom gate before adoption.

## Current validation policy

Every intentional DSP change must:

1. pass bounded-output and behavioral host tests;
2. produce a deterministic whole-groovebox render hash;
3. explicitly update the canonical hash only after the behavior tests pass;
4. pass WASM build/smoke tests;
5. pass the real JieLi/FM-1 compile, memory and packaging gates;
6. eventually pass physical CPU/underrun and listening/reference comparison before hardware-beta signoff.

The goal is not source-level similarity to Open303. The goal is a convincing TB-303 behavior under the FM-1's embedded constraints.
