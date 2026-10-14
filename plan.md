# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

- **ACID A / ACID B** — two independent 303-inspired mono bass synths
- **808 / 909** — distinct drum-machine workflows
- A–H patterns + song/chain sequencing
- four-channel mixer + performance FX
- USB/TRS MIDI + USB audio
- original ReBirth-inspired 240×240 physical UI
- browser installer/editor + Virtual FM-1/WASM reference
- project persistence and safe recovery workflow

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — beta.7 calibration candidate, 2026-10-14

The next requested hardware-test prerelease is **`v0.1.0-beta.7`**. Existing beta tags are immutable and must not be overwritten or repointed.

The beta.7 batch is deliberately corrective. It combines the post-beta hardware input/mixer fixes with a reference-driven TB-303 calibration pass rather than adding unrelated features.

## Hardware input / mapping corrections

- [x] Physical musical keys are scanned directly from `fm1_in.notes` in the real audio/control path.
- [x] Key-down owns the exact note that key-up later releases.
- [x] Physical ACID step entry stores the actual chromatic hardware MIDI note.
- [x] Physical ACID step entry no longer reuses the Virtual/ReBirth 13-key C…C pitch remap.
- [x] On physical ACID STEP, `OCT- / OCT+` shift the keyboard register by a real octave.
- [x] `K1` remains the fine semitone editor for the selected stored ACID note.
- [x] Machine/page transitions panic held notes to prevent stuck notes.
- [x] Primary hardware controls remain immediate and page-first rather than hidden modifier/tap-hold layers.

## Mixer / audibility corrections

- [x] Physical MASTER (`song.master_q12`) is restored after the ReFm render.
- [x] Conservative final target output headroom is retained before DAC/UAC handoff.
- [x] Master distortion starts at zero.
- [x] Compressor starts effectively transparent.
- [x] 808/909 synthesized fallback voices are verified audible without sample assets.
- [x] Drums retain full mixer-level startup headroom.

## TB-303 calibration findings

Reference comparison was performed against Open303/TeeBee source behavior plus the original TB-303 service pitch calibration.

Confirmed already correct and therefore **not arbitrarily changed**:

- [x] equal temperament with A4=440 Hz;
- [x] measured nominal cutoff range ~313.8–2394.4 Hz;
- [x] measured Env Mod scaler/offset mapping;
- [x] Open303 resonance skew law;
- [x] TeeBee TB-303 `b0`, feedback `k` and output gain `g` coefficient law;
- [x] ~150 Hz feedback high-pass;
- [x] 4x oscillator/filter processing at 176.4 kHz;
- [x] ~1000 ms normal and ~200 ms accented main-envelope decay anchors;
- [x] raw main filter envelope plus ~15 ms accent RC contribution;
- [x] ~1230 ms amp contour;
- [x] fast normal / ~50 ms accent release behavior;
- [x] nominal 60 ms slide represented by Open303's ~12 ms slew time constant;
- [x] ~44.486 Hz pre-ladder and ~24.167 Hz post-ladder AC-coupling stages;
- [x] Open303-shaped SAW303/SQUARE303 target oscillator prototypes.

### beta.7 303 change

Open303 keeps floating-point range through the oscillator/filter/VCA, while the FM-1 ultimately must fit a Q15 signal path. Strong low-register resonance/accent could therefore be flattened numerically before later mixer attenuation helped.

- [x] Reserve **6 dB source headroom** in compact FM-1 303 oscillator tables.
- [x] Apply the same 6 dB policy to Virtual/HQ reference generation.
- [x] Preserve square:saw relative level.
- [x] Preserve pitch, filter, Env Mod, resonance, decay, accent and slide equations.
- [x] Add a production-wavetable aggressive low-register resonance/accent test that rejects sustained Q15 rail-clipping.
- [x] Add the original TB-303 low-A service calibration anchor (~110 Hz) to pitch tests, plus exact octave doubling.
- [x] Keep end-to-end rendered C2/C3/C4 frequency tests.

Do not add resonance-dependent distortion, accent-to-resonance modulation, arbitrary EQ, or generic soft clipping just to make the output sound “more acid”. Any further DSP change must be tied to a reference mismatch and pass target CPU/headroom gates.

---

# Simple physical control contract

## ACID

1. `ENV` → ACID A.
2. `LFO` → ACID B.
3. `SEQ` → selected ACID step editor.
4. `PRESET` → select step 1–16.
5. Musical key → write/audition that physical key's true chromatic note.
6. `OCT- / OCT+` → keyboard octave down/up on ACID STEP.
7. `K1` → stored-note semitone down/up.
8. `K2` → REST / NOTE / TIE.
9. `K3` → Accent/Slide state.
10. `K4` → probability.

## DRUM

1. `ARP` → drum programmer.
2. Contextual/repeated drum selection switches 808/909.
3. `PRESET` → select step 1–16.
4. `K1` or `OCT- / OCT+` → lane down/up.
5. `K2` → OFF / HIT / ACCENT.
6. Musical key → select/program/audition mapped drum lane.
7. 909 `K3` → flam toggle.
8. 909 `K4` → flam width.

## Other primary pages

- `HOME` → home/status.
- `GLO` → mixer.
- `FX` → effects.
- `SCL` → song/pattern.
- `SAVE` → project.
- `PLAY` → transport immediately.

---

# Display contract

## Virtual

- [x] real 16-step ACID/drum state;
- [x] selected step distinct from playback step;
- [x] direct screen editing;
- [x] 240×240 source + true 480×480 2× view;
- [x] ReBirth-style C…C pitch workflow remains Virtual-only convenience behavior.

## Physical RB-240 renderer

- [x] no legacy Felucca graph strip under normal pages;
- [x] no persistent help footer;
- [x] compact page-specific layout;
- [x] exact ACID NOTE / REST / TIE / ACCENT / SLIDE readback;
- [x] exact DRUM OFF / HIT / ACCENT / 909 FLAM readback;
- [x] selected step and playhead are independent;
- [x] dirty card/cell redraw for same-page changes;
- [x] full-screen clear only on page/machine transition;
- [ ] verify zero visible full-screen flash during continuous knob rotation on real LCD;
- [ ] verify no clipped/overlapping labels on real LCD;
- [ ] verify no stale pixels remain when values shrink/change.

---

# Engine/runtime features

## ACID A / ACID B

- [x] saw + 303-square;
- [x] tune/pitch;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] slide + tie/rest/note sequencing;
- [x] independent dual instances;
- [x] optional per-ACID drive;
- [x] fixed-point target engine;
- [x] Virtual/WASM reference path;
- [x] Open303/TeeBee coefficient/reference audit for beta.7;
- [x] service-manual pitch anchor regression test;
- [x] fixed-point source-headroom regression test;
- [ ] physical cutoff/resonance/env-mod listening calibration;
- [ ] physical listening approval.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] independent 808/909 lane maps;
- [x] synthesized baseline voices;
- [x] open-hat choke;
- [x] real 909 flam state + width;
- [x] direct runtime drum audition;
- [x] synthesized fallback audibility test with no samples;
- [ ] physical listening approval;
- [ ] physical lane/edit workflow approval.

## Sequencer / song / mixer / FX

- [x] 16-step patterns + A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized live pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] delay/reverb returns;
- [x] Dist → Comp → PCF master performance rack;
- [x] versioned project save/load;
- [ ] defer longer patterns/ratchets/automation until physical basics are approved.

---

# Validation gates for beta.7

The exact candidate commit must pass all of the following before publication:

1. [ ] full host DSP/integration test suite;
2. [ ] generated compact 303 table freshness check;
3. [ ] A4=440 + low-A ~110 Hz + octave-ratio pitch tests;
4. [ ] end-to-end rendered-pitch tests;
5. [ ] production wavetable aggressive-patch rail/headroom test;
6. [ ] deterministic whole-groovebox render hash updated only after intentional DSP change is validated;
7. [ ] WASM/HQ build and browser smoke tests;
8. [ ] pinned Felucca overlay verification;
9. [ ] real JieLi FM-1 compile/link/package build;
10. [ ] target ROM-reference safety gate;
11. [ ] target memory budget gate;
12. [ ] release manifest/checksum verification.

Only after the exact `main` commit is green should `.release-beta` request `v0.1.0-beta.7`. The publisher is idempotent and must refuse to overwrite an existing tag.

---

# Physical beta.7 test order

Test at low external/headphone volume first.

1. [ ] STOP is silent.
2. [ ] MASTER minimum is effectively silent and scales smoothly upward.
3. [ ] On ACID STEP, press several physical keys and confirm displayed/stored note names match the actual keyboard.
4. [ ] Specifically verify G is G, not D# or another remapped note.
5. [ ] Verify `OCT- / OCT+` move the physical keyboard by exact octaves.
6. [ ] Verify `K1` changes the selected stored note by semitone without changing keyboard mapping.
7. [ ] Compare low ACID notes against beta.6: no sustained harsh/broken fixed-point distortion.
8. [ ] Sweep cutoff/resonance/env-mod at low and middle registers.
9. [ ] Test accented low notes and repeated accents.
10. [ ] Test slide between octave-spaced notes.
11. [ ] Audition 808 and 909 single hits with no dependence on external samples.
12. [ ] PLAY a seeded mixed pattern and verify drums are clearly audible against ACID.
13. [ ] STOP ends audio immediately.
14. [ ] Check no stuck note survives page/machine/octave changes.
15. [ ] Capture runtime diagnostics if any audio/input step fails.

---

# Safety / recovery

- [x] pinned Felucca foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and `.fwsc` generation;
- [x] package identity/SHA/memory gates;
- [x] strict ROM-reference safety gate;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] guarded one-shot beta publisher;
- [x] existing beta tags remain immutable.

Still required physically:

- [ ] factory-pattern runtime capture;
- [ ] worst-case four-machine stress capture;
- [ ] `late == 0`;
- [ ] sustained audio CPU <= 75%;
- [ ] peak render <= 82% of DMA-half deadline;
- [ ] user-stack free >= 4096 bytes;
- [ ] supervisor/ISR-stack free >= 1024 bytes;
- [ ] USB MIDI, TRS MIDI and USB audio under load;
- [ ] save/load + power-cycle behavior;
- [ ] recovery to official/Felucca firmware;
- [ ] repeated update/rollback cycle.

No custom firmware has a zero-brick guarantee.

---

# Release policy

- [x] `main` builds host/WASM/FM-1 artifacts;
- [x] verified one-shot prerelease publisher exists;
- [x] publisher requires the exact successful `main` Build-all commit;
- [x] publication is skipped when the requested tag already exists;
- [x] existing betas are immutable;
- [ ] beta.7 publish only after exact candidate commit is green;
- [ ] physical validation after beta.7 remains required before treating it as stable.

---

# Immediate next tasks after beta.7

1. Perform the physical beta.7 test order above.
2. Compare low-register ACID tone and resonance against a trustworthy TB-303/Open303 reference recording at matched pitch/level.
3. If a mismatch remains, capture which control region fails before altering DSP again.
4. Profile the exact four-machine worst case on hardware.
5. Only after basic physical approval, resume larger workflow/features work.
