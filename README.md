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

## Current status — beta.11 direct physical audio-path probe

Physical beta.9/10 feedback points to a shared high-frequency scratch/digital component that can mask the intended 303 and drum sound. Because the same artifact is heard on independent engines, beta.11 is a deliberately narrow diagnostic prerelease.

On the **physical FM-1 target only**, the first three seconds after ReFm initialization output a direct ~1 kHz, quarter-scale (~-12 dBFS) test tone. This tone is generated at the target output boundary and bypasses:

- ACID A / ACID B;
- 808 / 909;
- groovebox transport/rendering;
- mixer, PCF, Delay, Dist, Comp and sends.

After exactly three seconds the probe ends automatically and beta.10 normal operation resumes. Host and Virtual/WASM builds do **not** enable this startup probe.

The purpose is binary diagnosis:

- **boot tone is already scratchy / has the same high overlay** → investigate Felucca post-render / I²S / DAC / DMA / output formatting before touching synthesis again;
- **boot tone is clean, normal 303/drums are scratchy** → the corruption is inside the ReFm DSP/render path and the next build should isolate direct drum and 303 pre/post-ladder stems.

Use low external/headphone volume before powering the unit because the diagnostic tone starts automatically.

## beta.10 realtime correction retained

beta.11 keeps all beta.10 target-realtime changes:

- shared mixer hot path uses bounded signed 32-bit fixed-point arithmetic instead of repeated signed `int64_t` multiplies;
- track level/pan, delay/reverb sends, PCF, Dist and -12 dB master summing remain bounded and regression-tested;
- synthetic drum oscillator/ring paths avoid unnecessary signed-64 hot-path work;
- a fully released/idle 303 returns exact zero instead of continuously running oscillator + 4× ladder;
- active 303 oscillator/filter laws are unchanged;
- beta.9's 12×1024 Open303-derived target wavetable bank and smooth mip crossfades remain unchanged.

The pinned Felucca output path still performs its original final Q15 → 24-bit `<< 7` conversion after the ReFm render. beta.11 does not add a second guessed output shift or another global attenuation stage.

## 303 reference points

The ACID engine currently uses:

- equal temperament with A4=440 Hz;
- stable DDS pitch with no hidden drift when slide/LFO are off;
- 4× oscillator/filter processing at 176.4 kHz while active;
- shared 2048-sample Open303-style SAW303/SQUARE303 source model;
- 12 precomputed pitch bands and 1024 samples per physical-target table;
- phase-aligned two-semitone crossfades between adjacent mip bands;
- ~19.845 kHz harmonic guard;
- Open303/TeeBee-inspired cutoff, Env Mod and resonance mappings;
- ~150 Hz feedback-loop high-pass;
- ~1000 ms normal / ~200 ms accented main-envelope decay anchors;
- nominal 60 ms slide time constant;
- pre/post-ladder AC coupling;
- no random stock-mode pitch drift or always-on distortion.

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

Outside the beta.11 three-second startup probe, startup mixer behavior remains neutral:

- four track faders at unity;
- one -12 dB shared master summing stage for four-channel headroom;
- DIST=0;
- compressor effectively transparent;
- PCF fully open/bypassed;
- physical MASTER applied after the ReFm render;
- target fixed attenuation remains unity because the shared mixer owns summing headroom;
- Felucca's original Q15→24-bit DAC conversion remains after the ReFm callback.

## Virtual FM-1

Virtual uses the same groovebox state model and audible 303 source spectrum as hardware. Its wavetable lookup uses 2048 samples rather than 1024, but source prototype, band boundaries, harmonic budgets, headroom and SAW/SQUARE balance are shared.

The beta.11 boot probe is intentionally physical-target-only because it exists to isolate the FM-1 output chain. Virtual/WASM starts normally.

## Build and release safety

`.github/workflows/build-all.yml` builds the exact commit for:

1. Host DSP/integration tests;
2. shared WASM runtime + browser smoke tests;
3. real JieLi/FM-1 app, loader and experimental `.fwsc` package;
4. target ROM/memory gates, manifest and SHA checks;
5. a non-release sample link-size experiment.

The FM-1 build uses pinned Felucca `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`. GitHub Actions never connects to or flashes a physical unit.

Published betas are immutable. A new beta is published only from the exact successful `main` Build-all commit; older tags are never moved or overwritten.

The next physical diagnostic prerelease is **`v0.1.0-beta.11`**. beta.10 and all earlier releases remain untouched.

## beta.11 physical test priority

Start with low headphone/external volume.

1. Power-cycle / boot beta.11 and listen only to the first ~3 seconds.
2. Classify the direct 1 kHz probe as either **clean** or carrying the **same high scratch/overlay**.
3. After the probe ends, leave transport stopped: output should return to normal silence.
4. Press `PLAY`, then test a single 808/909 kick or snare with ACID silent.
5. Test one sustained ACID SAW note with drums stopped, then SQUARE.
6. Sweep cutoff through the full range, then resonance, Env Mod and decay separately.
7. Finally test ACID + drums under load.

The first result is the key one. If the direct boot probe is contaminated, do not retune 303/drum synthesis next. If the probe is clean but engine audio is contaminated, the next diagnostic build will expose isolated drum and 303 pre/post-ladder paths.

## Sample policy

Only auditable redistributable content may ship. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names are historical references to instruments/software and desired workflow/sound characteristics.
