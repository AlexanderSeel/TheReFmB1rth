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

## Current status — beta.12 staged DSP isolation

Physical beta.11 produced the decisive first result: its direct ~1 kHz startup tone was **clean**, while normal 303 and 808/909 audio still contained the strange added/doubled high component. That makes Felucca's final Q15→24-bit conversion, I²S/DAC formatting and the basic physical output chain unlikely to be the source of the artifact.

beta.12 therefore does not retune the 303 or drums. On the **physical FM-1 target only**, the first ten seconds after ReFm initialization run five two-second diagnostic stages:

1. **0–2 s — direct 1 kHz reference tone**: bypasses all ReFm engines and mixer.
2. **2–4 s — direct 808 kick**: synthetic 808 BD directly from `drum_machine_process()`, mixer/FX bypassed; four hits at 0.5 s spacing.
3. **4–6 s — mixed 808 kick**: the same BD engine routed through the normal ReFm mixer.
4. **6–8 s — direct 303 C2 SAW**: complete 303 oscillator + ladder output, mixer/FX bypassed; C2 is retriggered once per second.
5. **8–10 s — mixed 303 C2 SAW**: the same 303 setup routed through the normal ReFm mixer.

After ten seconds the diagnostic state is discarded, a fresh stopped groovebox is initialized, and normal operation begins. Host and Virtual/WASM builds do **not** run the startup sequence.

This provides a short decision tree:

- **stage 2 clean, stage 3 contaminated** → shared mixer/FX path is the source for the drum artifact;
- **stage 2 already contaminated** → the synthetic drum engine itself is producing the unwanted component;
- **stage 4 clean, stage 5 contaminated** → shared mixer/FX path is the source for the 303 artifact;
- **stage 4 already contaminated** → the fault is inside the active 303 oscillator/ladder/fixed-point path;
- **stages 2 and 4 both clean but normal playback later contaminates** → investigate groovebox event/routing/load behavior rather than the individual engines or mixer.

Use low external/headphone volume before power-on because the diagnostic sequence begins automatically.

## beta.11 finding retained

beta.11's clean direct startup tone is now a known physical fact for this hardware test path. The pinned Felucca output still performs its original final Q15 → 24-bit `<< 7` conversion after the ReFm render. beta.12 does not add another output shift, EQ, limiter or guessed attenuation workaround.

The pinned Felucca source was also rechecked: its native `events_block()` and original synth rendering are called from `mix_block()`, and ReFm replaces that `mix_block()` call on the physical audio path. That makes a simultaneously rendered native Felucca synth less likely as the source of the physical-key artifact.

## beta.10 realtime correction retained

beta.12 keeps all beta.10 target-realtime changes:

- shared mixer hot path uses bounded signed 32-bit fixed-point arithmetic instead of repeated signed `int64_t` multiplies;
- track level/pan, delay/reverb sends, PCF, Dist and -12 dB master summing remain bounded and regression-tested;
- synthetic drum oscillator/ring paths avoid unnecessary signed-64 hot-path work;
- a fully released/idle 303 returns exact zero instead of continuously running oscillator + 4× ladder;
- active 303 oscillator/filter laws are unchanged;
- beta.9's 12×1024 Open303-derived target wavetable bank and smooth mip crossfades remain unchanged.

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

Outside beta.12's ten-second startup diagnostics, startup mixer behavior remains neutral:

- four track faders at unity;
- one -12 dB shared master summing stage for four-channel headroom;
- DIST=0;
- compressor effectively transparent;
- PCF fully open/bypassed;
- physical MASTER applied after the ReFm render;
- Felucca's original Q15→24-bit DAC conversion remains after the ReFm callback.

## Virtual FM-1

Virtual uses the same groovebox state model and audible 303 source spectrum as hardware. Its wavetable lookup uses 2048 samples rather than 1024, but source prototype, band boundaries, harmonic budgets, headroom and SAW/SQUARE balance are shared.

The beta.12 staged startup probe is intentionally physical-target-only because it exists to isolate target DSP sections. Virtual/WASM starts normally.

## Build and release safety

`.github/workflows/build-all.yml` builds the exact commit for:

1. Host DSP/integration tests;
2. shared WASM runtime + browser smoke tests;
3. real JieLi/FM-1 app, loader and experimental `.fwsc` package;
4. target ROM/memory gates, manifest and SHA checks;
5. a non-release sample link-size experiment.

The FM-1 build uses pinned Felucca `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`. GitHub Actions never connects to or flashes a physical unit.

Published betas are immutable. A new beta is published only from the exact successful `main` Build-all commit; older tags are never moved or overwritten.

The next physical diagnostic prerelease is **`v0.1.0-beta.12`**. beta.11 and all earlier releases remain untouched.

## beta.12 physical test priority

Start with low headphone/external volume and simply classify the five startup stages:

1. **0–2 s reference tone** — expected clean.
2. **2–4 s direct 808 BD** — does the strange extra/high component already exist?
3. **4–6 s mixed 808 BD** — is it materially different/worse than stage 2?
4. **6–8 s direct 303 C2 SAW** — does the doubled/high component already exist?
5. **8–10 s mixed 303 C2 SAW** — is it materially different/worse than stage 4?

The most useful report is simply `1 clean/dirty, 2 clean/dirty, 3 clean/dirty, 4 clean/dirty, 5 clean/dirty`, plus whether 2→3 or 4→5 introduces the unwanted sound.

## Sample policy

Only auditable redistributable content may ship. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names are historical references to instruments/software and desired workflow/sound characteristics.
