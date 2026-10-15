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

## Current status — beta.13 303 target-DSP correction

The physical beta.12 pass produced an important improvement: the previous added/doubled/scratch-like component is now reported **gone** and the hardware output sounds clean. The remaining fault is much narrower:

- the 303 sounds too close to a clean sine/fundamental;
- Cutoff/Resonance/Env Mod changes have little or no audible effect on the physical unit;
- the physical/UI and MIDI control paths do update the actual `acid303_t` parameters, so the leading fault is inside the active 303 target DSP rather than the control routing.

beta.13 therefore does **not** widen or retune the measured TB-303 knob law. It keeps the Open303-derived oscillator tables and nominal ~314–2394 Hz cutoff mapping, but removes the remaining signed-64-bit arithmetic from the active 303 realtime path.

### beta.13 303 changes

- Open303/TeeBee filter coefficient formulas are represented by a compact 65-point target LUT spanning 200–20,000 Hz at the 176.4 kHz ladder rate.
- Coefficients are interpolated with bounded 32-bit arithmetic; the beta.12 coefficient law and conservative high-resonance bound are retained.
- The active ladder, feedback/pre/post high-pass filters, VCA, Env Mod path, wavetable interpolation/crossfade, slide and LFO path no longer require `int64_t`/`uint64_t`.
- A bounded decomposed Q15 multiply handles large ladder state × coefficient products without overflowing signed 32 bit.
- Filter coefficient control updates now land on the exact target every 32 samples instead of `(target-current)/32` integer stepping. The old stepping could truncate a small remaining delta to zero indefinitely and create control dead-zones.
- New host regression renders a real C2 SAW and requires Cutoff MIN→MAX to cause a material coefficient and audio change. It also verifies a material resonance response and that the open-cutoff SAW retains more edge/harmonic motion than the closed-filter case.
- The regression rejects reintroduction of 64-bit integer types in `acid303.c`.

The beta.9 12×1024 Open303-derived physical wavetable bank is intentionally retained. The physical feedback did not justify regenerating the source tables before fixing the target ladder/control arithmetic.

## beta.12 physical isolation retained

For this corrective pass the physical-only ten-second startup probe is retained so the same hardware can be compared against beta.12 under identical conditions:

1. **0–2 s — direct 1 kHz reference tone**
2. **2–4 s — direct 808 kick**
3. **4–6 s — mixed 808 kick**
4. **6–8 s — direct 303 C2 SAW**
5. **8–10 s — mixed 303 C2 SAW**

After ten seconds a fresh stopped groovebox is initialized and normal operation begins. Host and Virtual/WASM do not run the probe.

The known physical result before beta.13 is that the former added/doubled sound is gone. The next physical priority is normal 303 operation after the startup sequence: verify an obvious Cutoff sweep, Resonance response, Env Mod sweep and SAW/SQUARE distinction on C2–C4.

## 303 reference points

The ACID engine uses:

- equal temperament with A4=440 Hz;
- stable DDS pitch with no hidden drift when slide/LFO are off;
- 4× oscillator/filter processing at 176.4 kHz while active;
- shared 2048-sample Open303-style SAW303/SQUARE303 source model;
- 12 precomputed pitch bands and 1024 samples per physical-target table;
- phase-aligned two-semitone crossfades between adjacent mip bands;
- ~19.845 kHz harmonic guard;
- measured Open303/TeeBee-inspired Cutoff, Env Mod and Resonance mappings;
- ~150 Hz feedback-loop high-pass;
- ~1000 ms normal / ~200 ms accented main-envelope decay anchors;
- nominal 60 ms slide time constant;
- pre/post-ladder AC coupling;
- no random stock-mode pitch drift or always-on distortion.

## Physical controls

Primary pages are page-first:

- `ENV` → ACID A
- `LFO` → ACID B
- `SEQ` → ACID step editor
- `ARP` → 808/909 drum programmer
- `GLO` → mixer
- `FX` → effects
- `SCL` → song/pattern
- `SAVE` → project
- `PLAY` → transport

### ACID main page

- `K1` → Cutoff
- `K2` → Resonance
- `K3` → Env Mod
- `K4` → Decay

The browser/MIDI editor uses the same engine fields (`CC74`, `CC71`, `CC1`, `CC73` respectively).

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

Outside the physical diagnostic sequence:

- four track faders start at unity;
- one -12 dB shared master summing stage provides four-channel headroom;
- DIST=0;
- compressor startup is effectively transparent;
- PCF starts fully open/bypassed;
- physical MASTER is applied after the ReFm render;
- Felucca performs its original Q15→24-bit DAC conversion after the ReFm callback.

The clean beta.11 direct tone and the cleaner beta.12 physical result mean no additional guessed output shift, EQ or attenuation workaround is being added.

## Virtual FM-1

Virtual uses the same groovebox state model and 303 source spectrum as hardware. Its wavetable lookup uses 2048 samples rather than the physical target's 1024-sample tables, while source prototype, pitch-band policy, harmonic guard and SAW/SQUARE balance remain shared.

## Browser installer

The browser release loader reads the CORS-safe `firmware-channel/latest.json`, then verifies that exact tag through GitHub's `releases/tags/<tag>` endpoint, verifies the exact `.fwsc` asset name and SHA-256, and only then enables the package. It does not depend on GitHub's release-list ordering.

## Build and release safety

`.github/workflows/build-all.yml` validates the exact `main` commit for:

1. host DSP/unit/integration tests;
2. shared WASM runtime + browser smoke tests;
3. real JieLi/FM-1 app, loader and experimental `.fwsc`;
4. target XIP/RAM/pool budgets;
5. ROM-reference and manifest/checksum gates;
6. sample link-size experiment.

The FM-1 build uses pinned Felucca `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`. GitHub Actions never connects to or flashes a physical unit.

Published betas are immutable. `v0.1.0-beta.12` and all earlier tags remain untouched. The next corrective prerelease is **`v0.1.0-beta.13`** and is published only from the exact successful Build-all commit.

## beta.13 physical test priority

After the ten-second startup probe:

1. select ACID A and play C2, C3 and C4;
2. sweep **Cutoff K1** from minimum to maximum — it should now be unmistakable rather than nearly static;
3. with Cutoff around the middle/high range, sweep **Resonance K2**;
4. sweep **Env Mod K3** and listen for the envelope opening/closing the ladder;
5. compare SAW and SQUARE;
6. confirm the former added/doubled scratch component remains absent.

Do not compensate a failed filter sweep with mixer EQ/PCF. If the physical filter is still static after beta.13, the next diagnostic will expose raw wavetable, pre-ladder and post-ladder stages separately.

## Sample policy

Only auditable redistributable content may ship. The preferred implementation synthesizes 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names are historical references to instruments/software and desired workflow/sound characteristics.
