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

## Current status — beta.9 303 oscillator refinement

Beta.8 fixed the global audio-path defects found on the physical FM-1: live-note clamping, false slide timing, mixer clipping, duplicate target attenuation and the unusable fallback drum synthesis. Beta.9 keeps those corrections and works specifically on the remaining 303 SAW/SQUARE source quality.

The beta.9 oscillator batch changes the precomputed wavetable path instead of masking it with EQ or distortion:

- both FM-1 and Virtual now derive SAW303 and SQUARE303 from one shared **2048-sample Open303-style source prototype**;
- the physical target tables are doubled from **512 to 1024 samples** per waveform/band;
- the target expands from 10 to **12 pitch bands** covering the full supported MIDI range through note 127;
- SQUARE303 is derived from the SAW303 prototype using the Open303-style tanh shaping values (36.9 dB, offset +4.37), polarity inversion and 180° phase shift instead of using a separate ideal-square model in Virtual;
- square remains at half the raw saw-table amplitude, preserving the Open303 BlendOscillator convention while keeping the rendered SAW/SQUARE fundamentals closely level matched;
- harmonic budgets are calculated per pitch band with a small **19.845 kHz final-Nyquist guard** to reduce interpolation/decimation products folding back into the audible range;
- adjacent mip levels are phase aligned and crossfaded over the final **two semitones** before each pitch threshold instead of hard-switching spectra during a note or slide;
- target and Virtual use the same pitch-band boundaries and audible harmonic budgets, so browser testing no longer hides a different oscillator model;
- the beta.7 fixed-point source headroom remains intact;
- pitch, filter, Env Mod, resonance, envelope, accent, 60 ms slide and clean mixer laws are otherwise unchanged.

Software reference probes show essentially unchanged source level while changing the intended spectrum: the new open SAW render is peak 8673 / mean-abs 2865 and SQUARE is peak 5589 / mean-abs 2835. End-to-end rendered pitch remains C2/C3/C4 = 33/66/131 crossings per 0.5 seconds.

## 303 reference points

The stock-style ACID core currently uses:

- equal temperament with A4=440 Hz at TUNE center;
- explicit TB-303 low-A service anchor near 110 Hz;
- stable DDS pitch with no hidden drift when slide/LFO are off;
- 4x oscillator/filter processing at 176.4 kHz;
- one shared 2048-sample Open303-style SAW303/SQUARE303 source model;
- 12 precomputed pitch bands, 1024 samples per target table;
- two-semitone phase-aligned crossfades between adjacent mip bands;
- ~19.845 kHz harmonic guard below final 44.1 kHz Nyquist;
- Open303's measured nominal cutoff range around 313.8–2394.4 Hz;
- Open303's exponential Env Mod scaler/offset mapping;
- TeeBee/Open303 resonance skew and feedback coefficient law;
- ~150 Hz feedback-loop high-pass;
- ~1000 ms normal / ~200 ms accented main-envelope decay anchors;
- raw main filter envelope plus ~15 ms smoothed accent contribution;
- ~1230 ms amplitude decay and fast normal / ~50 ms accented release behavior;
- nominal **60 ms** slide time constant at the 44.1 kHz control/audio rate;
- ~44.486 Hz pre-ladder and ~24.167 Hz post-ladder AC-coupling stages.

The goal is a convincing hardware-safe TB-303 behavior, not source-level duplication of any commercial emulator. Stock mode deliberately has no random pitch drift, detune or always-on distortion.

## Virtual FM-1

Virtual FM-1 uses the same groovebox state model and the same audible 303 oscillator spectrum as the physical target. Its table lookup is 2048 samples rather than 1024, but the source prototype, band boundaries, harmonic budgets, headroom and SAW/SQUARE balance are shared.

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

The hardware key scanner records note ownership at key-down, so octave/page changes cannot transform a later key-up into the wrong note. Machine/page changes remain panic-guarded against stuck notes. Live notes are not constrained by the step-editor pitch clamp.

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

The clean startup contract retained from beta.8 is explicit:

- all four track faders start at unity;
- a single -12 dB master summing stage provides four-track headroom before nonlinear processing;
- master DIST starts at zero;
- compressor threshold starts effectively transparent;
- PCF starts fully open and bypasses its one-pole coloration at the top setting;
- the physical MASTER pot is applied after the ReFm render;
- target output scaling is unity after the physical master pot because the shared mixer already owns fixed summing headroom;
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

For this oscillator-refinement batch, the expected next tag is **`v0.1.0-beta.9`**. `v0.1.0-beta.8` and every earlier beta remain untouched.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are historical references to instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from Felucca-derived source and all included third-party components.
