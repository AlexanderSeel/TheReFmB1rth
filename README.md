# TheReFmB1rth

Experimental replacement firmware for the M-VAVE FM-1: a compact four-machine acid groovebox inspired by the classic two-bassline/two-drum-machine workflow.

## Target

- ACID A / ACID B — two independent 303-inspired mono synths
- 808 / 909 — distinct synthesized drum machines, including persisted 909 flam state
- A–H patterns + song chaining
- four-track mixer + PCF / Delay / Dist / Comp performance FX
- USB/TRS MIDI + USB audio
- 240×240 physical UI + matching Virtual FM-1/WASM
- guarded FM-1 package flow based on pinned Felucca 1.0.5

See [`plan.md`](plan.md), [`docs/TB303_MODEL.md`](docs/TB303_MODEL.md), [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) and [`docs/PHYSICAL_BETA2_VALIDATION.md`](docs/PHYSICAL_BETA2_VALIDATION.md).

## Current status — beta.10 physical realtime-audio correction

Physical beta.9 feedback showed a more important failure than oscillator character alone: both 303 and drums are dominated by a high, scratchy/digital component while the intended bass/drum sound remains faint underneath. Cutoff, resonance, Env Mod and decay therefore appear to affect only the quiet underlying signal.

Because the same artifact is present on independent synth and drum engines, beta.10 moves the investigation to the **shared FM-1 realtime path** instead of applying more oscillator EQ, detune or saturation.

### What was verified first

The pinned Felucca output path still performs its original final **Q15 → 24-bit `<< 7` conversion** after the ReFm render. Replacing Felucca's `mix_block()` did not remove the later UAC/DAC processing. The bug is therefore not explained by a missing final output shift.

### beta.10 realtime changes

- the shared mixer hot path is converted from repeated signed `int64_t` multiplies to bounded signed **32-bit fixed-point arithmetic**;
- all 32-bit products have explicit numerical bounds, including track level/pan, delay, reverb, PCF, Dist and the -12 dB master summing stage;
- the mixer clamps the pre-master accumulator before its Q15 multiply so the maximum legal product remains below `INT32_MAX`;
- synthetic drum oscillator/ring-modulation multiplications are likewise moved out of the per-sample signed-64-bit path;
- drum pitch, decay and timbre constants are unchanged by this batch;
- a fully released/idle 303 now returns zero immediately from the groovebox mixer input instead of continuously running its oscillator and four-times-oversampled ladder;
- this is especially important for drum-only playback: beta.9 still spent substantial CPU on **two silent 303 ladders** every sample;
- active 303 oscillator, ladder, cutoff, resonance, Env Mod, decay, accent and slide laws are unchanged;
- beta.9's 12×1024 Open303-derived SAW/SQUARE wavetable bank and smooth mip crossfades remain unchanged.

The purpose of beta.10 is to test whether the common high-frequency scratch is caused by target realtime/compiler load or arithmetic in the shared path. It deliberately avoids another subjective timbre redesign.

## 303 reference points

The ACID engine currently uses:

- equal temperament with A4=440 Hz and a low-A reference near 110 Hz;
- stable DDS pitch with no hidden drift when slide/LFO are off;
- 4× oscillator/filter processing at 176.4 kHz while the voice is active;
- one shared 2048-sample Open303-style SAW303/SQUARE303 source model;
- 12 precomputed pitch bands and 1024 samples per physical-target table;
- phase-aligned two-semitone crossfades between adjacent mip bands;
- ~19.845 kHz harmonic guard below final 44.1 kHz Nyquist;
- Open303/TeeBee-inspired cutoff, Env Mod and resonance mappings;
- ~150 Hz feedback-loop high-pass;
- ~1000 ms normal / ~200 ms accented main-envelope decay anchors;
- nominal 60 ms slide time constant;
- pre/post-ladder AC coupling;
- no random stock-mode pitch drift or always-on distortion.

A fully released voice is now skipped before entering this expensive path.

## Physical controls

Primary pages remain page-first:

- `ENV` → ACID A
- `LFO` → ACID B
- `SEQ` → ACID step editor
- `ARP` → 808/909 drum programmer
- `GLO` → mixer
- `FX` → effects
- `SCL` → song/pattern
- `SAVE` → project
- `PLAY` → transport

### ACID step entry

On `SEQ`:

- `PRESET` selects step 1–16;
- a musical key writes/auditions its true chromatic pitch;
- `OCT-` / `OCT+` change the physical keyboard register;
- `K1` edits stored pitch by semitone;
- `K2` selects REST / NOTE / TIE;
- `K3` controls Accent/Slide state;
- `K4` controls probability.

Live physical notes are not clamped to the editor's old note range.

### 808 / 909

On `ARP` / DRUM:

- contextual machine selection switches 808/909;
- `PRESET` selects step 1–16;
- `K1` or `OCT-` / `OCT+` selects the lane;
- `K2` cycles OFF / HIT / ACCENT;
- musical keys select/program/audition mapped voices;
- 909 `K3` toggles FLAM;
- 909 `K4` sets flam width.

## Clean audio contract

The startup mixer remains intentionally neutral:

- four track faders at unity;
- one -12 dB shared master summing stage for four-channel headroom;
- DIST=0;
- compressor effectively transparent;
- PCF fully open/bypassed;
- physical MASTER applied after the ReFm render;
- final target fixed attenuation remains unity because shared mixer headroom already owns the fixed gain reduction;
- Felucca's original Q15→24-bit DAC conversion remains after the ReFm callback.

beta.10 changes the **implementation width/realtime cost**, not this gain structure.

## Virtual FM-1

Virtual uses the same groovebox state model and audible 303 source spectrum as hardware. Its wavetable lookup uses 2048 samples rather than 1024, but source prototype, band boundaries, harmonic budgets, headroom and SAW/SQUARE balance are shared.

The logical display remains 240×240, with an optional true 480×480 DISPLAY 2× view.

## Build and release safety

`.github/workflows/build-all.yml` builds the exact commit for:

1. Host DSP/integration tests;
2. shared WASM runtime + browser smoke tests;
3. the real JieLi/FM-1 app, loader and experimental `.fwsc` package;
4. target ROM/memory gates, manifest and SHA checks;
5. a non-release sample link-size experiment.

The FM-1 build uses the pinned Felucca foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`. GitHub Actions never connects to or flashes a physical unit.

Published betas are immutable. A new beta is published only from the exact successful `main` Build-all commit; older tags are never moved or overwritten.

The next physical diagnostic prerelease is **`v0.1.0-beta.10`**. beta.9 and all earlier releases remain untouched.

## beta.10 physical test priority

Start at low external/headphone volume.

1. Test a single 808/909 kick or snare with both ACID voices silent. The previous common high scratch should be gone or substantially reduced if idle-303/realtime load was the cause.
2. Test one sustained ACID note with drums stopped. Listen for the same high scratch independently of oscillator pitch.
3. Compare SAW and SQUARE with low resonance/Env Mod.
4. Sweep cutoff through its full range; the audible body should now follow it clearly if the correct 303 signal is no longer masked.
5. Raise resonance and Env Mod separately and confirm clearly audible control depth.
6. Test mixed ACID + drums under load.
7. If the scratch remains almost unchanged on every engine, stop retuning synthesis. The next diagnostic build should bypass the groovebox/mixer with a direct Q15 sine/test tone, then direct drum and pre/post-ladder 303 paths, to isolate the Felucca/I²S/DAC postpath from DSP.

Existing runtime diagnostics should also be checked for late audio blocks / excessive render time during the mixed-load test.

## Sample policy

Only auditable redistributable content may ship. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names are historical references to instruments/software and desired workflow/sound characteristics.
