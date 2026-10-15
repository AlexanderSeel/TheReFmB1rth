# TheReFmB1rth

Experimental replacement firmware for the M-VAVE FM-1: a compact four-machine acid groovebox inspired by the classic two-bassline/two-drum-machine workflow.

## Target

- ACID A — original 303-inspired mono synth
- ACID B — second independent acid synth
- 808 — synthesized drum machine
- 909 — distinct synthesized drum machine with real sequencer flam state
- A–H patterns + song chaining
- four-track mixer + performance FX
- USB/TRS MIDI + USB audio
- browser installer/editor + Virtual FM-1/WASM
- guarded FM-1 installer based on the pinned Felucca update path

See [`plan.md`](plan.md), [`docs/TB303_MODEL.md`](docs/TB303_MODEL.md), [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) and [`docs/PHYSICAL_BETA2_VALIDATION.md`](docs/PHYSICAL_BETA2_VALIDATION.md).

## Current status — beta.8 clean-audio candidate

The physical beta.7 listening pass exposed a real audio-path failure: the overall result sounded overloaded, the 303 square was thin/metallic, pitch felt unstable, and the synthesized drums were unusably harsh. Beta.8 treats that report as a correctness problem rather than adding more coloration.

The corrective batch includes:

- live ACID notes now preserve the exact incoming MIDI/physical-key pitch instead of clamping every note above MIDI 60/C4 down to C4;
- programmed ACID steps keep the extended C1..C6 editor range independently from live keyboard routing;
- the DDS has explicit upper-MIDI monotonicity tests and remains numerically static for a full second whenever slide and LFO are off;
- stock slide now uses the documented Open303 **60 ms** slew time constant instead of the previous ~12 ms implementation;
- all four mixer channel faders start at unity, with one explicit **-12 dB master summing headroom** stage before optional nonlinear processing;
- DIST starts at zero, COMP is transparent and PCF starts fully open/bypassed, so a fresh project has no hidden saturation, compression or low-pass coloration;
- the physical MASTER pot remains active, while the older duplicate fixed target attenuation is removed because summing headroom now has one clear owner;
- the synthetic 808 kick is moved from the erroneous ~390 Hz range to ~55 Hz and the 909 kick to ~60 Hz, with substantially longer analogue-style body decays;
- snare/tom fundamentals are moved into musically appropriate ranges;
- hats/cymbals use bipolar ring multiplication instead of signed bitwise XOR, which had been producing digital metallic artefacts;
- the drum bus reserves its own transient headroom so kick/snare/hat combinations do not rail-clip;
- samples remain optional: the corrected synthesized engines are the guaranteed firmware fallback.

This is software-validated but still requires a new physical FM-1 listening pass. Beta.8 must therefore be judged first on **clean pitch and clean gain structure**, before any further attempt to make the 303 more characterful.

## 303 reference points

The stock-style ACID core currently uses:

- equal temperament with A4=440 Hz at TUNE center;
- explicit TB-303 low-A service anchor near 110 Hz;
- stable DDS pitch with no hidden drift when slide/LFO are off;
- 4x oscillator/filter processing at 176.4 kHz;
- Open303-shaped SAW303/SQUARE303 source tables;
- Open303's measured nominal cutoff range around 313.8–2394.4 Hz;
- Open303's exponential Env Mod scaler/offset mapping;
- TeeBee/Open303 resonance skew and feedback coefficient law;
- ~150 Hz feedback-loop high-pass;
- ~1000 ms normal / ~200 ms accented main-envelope decay anchors;
- raw main filter envelope plus ~15 ms smoothed accent contribution;
- ~1230 ms amplitude decay and fast normal / ~50 ms accented release behavior;
- nominal **60 ms** slide time constant at the 44.1 kHz control/audio rate;
- ~44.486 Hz pre-ladder and ~24.167 Hz post-ladder AC-coupling stages.

The goal is a convincing hardware-safe TB-303 behavior, not source-level duplication of any commercial emulator. The beta.8 pass deliberately does not add random pitch drift, detune or always-on distortion.

## Virtual FM-1

Virtual FM-1 uses the same groovebox state model and is the primary no-hardware test surface.

### ACID A / ACID B

Use **SEQ** as a direct 303-style step editor:

1. select one of the 16 steps;
2. choose note, REST or TIE;
3. toggle ACCENT and SLIDE;
4. use the Virtual ReBirth-style C…C pitch keyboard and DOWN/NORMAL/UP register workflow;
5. audition through the shared groovebox runtime.

Virtual's compact C…C interaction is intentionally a UI convenience. It must not rewrite the pitch meaning of a physical FM-1 key.

### 808 / 909

Select a drum lane and click the 16-step grid. Each cell cycles:

**OFF → HIT → ACCENT → OFF**

909 also retains persisted flam state and width. The synthesized fallback engines remain active when no bundled sample exists.

### Display

The logical display remains **240×240**, matching the physical FM-1. Virtual FM-1 also provides a true **480×480 DISPLAY 2×** view while retaining the same underlying UI state.

## Physical FM-1 controls

The hardware remains page-first: ordinary operation must not require hidden modifier chords.

Primary pages:

- `ENV` → ACID A
- `LFO` → ACID B
- `SEQ` → ACID step editor
- `ARP` → 808/909 drum programmer
- `GLO` → mixer
- `FX` → effects
- `SCL` → song/pattern
- `SAVE` → project
- `PLAY` → transport immediately

### Physical ACID step entry

On **SEQ**:

- `PRESET` → select step 1–16;
- musical key → write that key's true chromatic pitch into the selected step and make it a NOTE;
- `OCT-` / `OCT+` → shift the physical keyboard register by one octave;
- `K1` → fine note edit by semitone;
- `K2` → REST / NOTE / TIE;
- `K3` → Accent/Slide combinations;
- `K4` → probability.

The hardware key scanner records note ownership at key-down, so octave/page changes cannot transform a later key-up into the wrong note. Machine/page changes remain panic-guarded against stuck notes. Live notes are no longer constrained by the step-editor pitch clamp.

### Physical 808 / 909 programming

On **DRUM**:

- repeated/contextual DRUM selection switches 808/909;
- `PRESET` → select step 1–16;
- `K1` or `OCT-` / `OCT+` → lane down/up;
- `K2` → OFF / HIT / ACCENT;
- musical keys → select/program/audition mapped drum voices;
- 909 `K3` → FLAM;
- 909 `K4` → flam width.

808 and 909 use different model-specific oscillator tunings/workflows; they are not two labels over one identical drum map.

## Audio and mixer behavior

Four machine channels feed level/pan/mute/solo plus delay/reverb sends. Master performance processing is Dist → Comp → PCF.

The clean startup contract for beta.8 is explicit:

- all four track faders start at unity;
- a single -12 dB master summing stage provides four-track headroom before nonlinear processing;
- master DIST starts at zero;
- compressor threshold starts effectively transparent;
- PCF starts fully open and bypasses its one-pole coloration at the top setting;
- the physical MASTER pot is applied after the ReFm render;
- no second fixed target attenuation is stacked after that pot;
- individual drum synthesis and the combined drum bus both retain headroom rather than relying on final hard clipping.

Initial hardware listening tests should still start at low external/headphone volume.

## 240×240 hardware UI

Current `main` uses the dedicated compact RB-240 renderer rather than stacking legacy Felucca graph layers underneath it.

Implemented target behavior includes:

- page-specific compact layout;
- exact 16-step ACID and drum readback;
- independent selected-step and playhead indication;
- real NOTE / REST / TIE / ACCENT / SLIDE state;
- real drum OFF / HIT / ACCENT and 909 FLAM state;
- dirty card/cell redraw for same-page changes;
- full-screen clear only on page/machine transitions.

Physical confirmation is still required for zero visible flash, label legibility and stale-pixel cleanup on the actual LCD.

## Build everything in GitHub

`.github/workflows/build-all.yml` builds the exact commit for Host, WASM and the real JieLi/FM-1 package.

Artifacts include:

1. **`refm-wasm-<commit>`**
   - `refm.js`
   - `refm.wasm`
   - combined browser/Virtual app
   - checksums

2. **`refm-fm1-experimental-<commit>`**
   - `TheReFmB1rth-experimental.fwsc`
   - application binary
   - Felucca `ota.bin`
   - manifest/provenance/memory reports
   - checksums

The FM-1 job prepares the exact pinned Felucca foundation, applies the deterministic overlay, downloads the JieLi pi32v2 toolchain, fetches the pinned AC79 SDK, performs the target compile/link/package build and enforces memory/ROM-reference gates. GitHub Actions never connects to or flashes a physical unit.

Useful local/CI-equivalent checks:

```sh
tests/run_host_tests.sh
sh web/emu/build_wasm.sh
# Real FM-1 package: Build all artifacts in GitHub Actions.
```

## Release policy

Published betas are immutable. The requested beta is cut only from an exact `main` commit after Host, WASM and FM-1 target-package jobs are green for that commit. The release marker is versioned; an existing tag is never overwritten.

For this corrective batch, the expected next tag is **`v0.1.0-beta.8`**. `v0.1.0-beta.7`, `v0.1.0-beta.2` and every other existing beta remain untouched.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are historical references to instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from Felucca-derived source and all included third-party components.
