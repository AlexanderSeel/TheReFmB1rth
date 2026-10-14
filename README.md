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

## Current status — beta.7 calibration candidate

`main` is being prepared for **`v0.1.0-beta.7`** as the next immutable hardware-test prerelease. Earlier betas are not overwritten or repointed.

The beta.7 batch combines the latest physical FM-1 corrections with a focused TB-303 calibration pass:

- physical FM-1 musical keys now preserve their real chromatic pitch when writing ACID steps;
- the physical keyboard no longer inherits the Virtual/ReBirth C…C step-key remapping;
- `OCT-` / `OCT+` shift the physical keyboard register on ACID STEP, while `K1` remains the fine semitone editor for the stored note;
- the default master path starts transparent instead of applying always-on distortion/compression coloration;
- 808/909 have synthesized fallback voices and therefore do not depend on external sample files to be audible;
- the 303 pitch model is explicitly checked against A4=440 and the original TB-303 service-calibration low-A anchor near 110 Hz;
- Open303/TeeBee cutoff, resonance, envelope, slide and accent equations were rechecked rather than arbitrarily retuned;
- the fixed-point oscillator banks reserve **6 dB source headroom** before the ladder/VCA, preventing strong low-register resonance/accent peaks from living on the Q15 rails;
- target and Virtual reference generators use the same headroom policy;
- an aggressive resonant/accented low-register render is now a regression test for sustained hard clipping.

The 6 dB oscillator headroom is a numerical implementation detail, not a pitch or filter-law change. Open303 can temporarily exceed normalized ±1 internally because its signal path is floating point; the FM-1 path ultimately must fit a bounded fixed-point mixer. See [`docs/TB303_MODEL.md`](docs/TB303_MODEL.md) for the calibration details.

## 303 reference points

The stock-style ACID core currently uses:

- equal temperament with A4=440 Hz at TUNE center;
- 4x oscillator/filter processing at 176.4 kHz;
- Open303-shaped SAW303/SQUARE303 source tables;
- Open303's measured nominal cutoff range around 313.8–2394.4 Hz;
- Open303's exponential Env Mod scaler/offset mapping;
- TeeBee/Open303 resonance skew and feedback coefficient law;
- ~150 Hz feedback-loop high-pass;
- ~1000 ms normal / ~200 ms accented main-envelope decay anchors;
- raw main filter envelope plus ~15 ms smoothed accent contribution;
- ~1230 ms amplitude decay and fast normal / ~50 ms accented release behavior;
- nominal 60 ms slide represented by the same ~12 ms slew time constant used by Open303;
- ~44.486 Hz pre-ladder and ~24.167 Hz post-ladder AC-coupling stages.

The goal is a convincing hardware-safe TB-303 behavior, not source-level duplication of any commercial emulator.

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

The hardware key scanner records note ownership at key-down, so octave/page changes cannot transform a later key-up into the wrong note. Machine/page changes remain panic-guarded against stuck notes.

### Physical 808 / 909 programming

On **DRUM**:

- repeated/contextual DRUM selection switches 808/909;
- `PRESET` → select step 1–16;
- `K1` or `OCT-` / `OCT+` → lane down/up;
- `K2` → OFF / HIT / ACCENT;
- musical keys → select/program/audition mapped drum voices;
- 909 `K3` → FLAM;
- 909 `K4` → flam width.

808 and 909 use different lane maps/workflows; they are not two labels over one identical drum map.

## Audio and mixer behavior

Four machine channels feed level/pan/mute/solo plus delay/reverb sends. Master performance processing is Dist → Comp → PCF.

For hardware safety and useful listening tests:

- master DIST starts at zero;
- compressor threshold starts effectively transparent;
- the physical MASTER pot is applied after the ReFm render;
- the target output retains conservative final headroom before DAC/UAC handoff;
- ACID oscillator source headroom prevents clipping **before** later mixer attenuation can help;
- drums remain audible without sample assets because synthesized fallback voices are part of the firmware.

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

For beta.7 specifically, the expected tag is **`v0.1.0-beta.7`**. `v0.1.0-beta.2` and every other existing beta remain untouched.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are historical references to instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from Felucca-derived source and all included third-party components.
