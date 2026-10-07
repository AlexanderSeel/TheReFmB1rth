# TheReFmB1rth

Experimental replacement firmware project for the M-VAVE FM-1: a compact four-machine acid groovebox inspired by the classic two-bassline/two-drum-machine workflow.

## Target

- ACID 1 — original 303-inspired mono synth
- ACID 2 — second independent acid synth
- DRUM A — original 808-inspired synthesized drum machine
- DRUM B — original 909-inspired synthesized drum machine
- A–H patterns + song chaining
- mixer + performance FX
- USB/TRS MIDI and USB audio
- browser/WASM emulator
- guarded FM-1 installer based on the proven Felucca update path

See [`plan.md`](plan.md) and [`docs/MILESTONES.md`](docs/MILESTONES.md).

## Current status

**M5 groovebox core and M6 safety tooling are actively implemented and host-tested. This is still not a hardware release: do not flash a generated artifact until the Felucca target build/emulator/linker/CPU gates have been completed.**

Implemented host-side foundations now include:

- two acid engines and two synthesized 11-lane drum engines in one four-machine runtime;
- 16-step sequencing, accent/slide/tie/probability primitives;
- A–H pattern banks and song chaining;
- level/pan/mute/solo mixer, delay sends, drive, compressor, delay, performance low-pass and bounded master output;
- internal clock plus MIDI START/CONTINUE/STOP/CLOCK;
- MIDI note/CC/program routing for acid and drum machines;
- versioned CRC-protected project serialization;
- LFO/filter/ADSR graph models;
- release manifests, SHA/product/device checks and a double-confirmed experimental installer wrapper that delegates to the pinned Felucca loader-mediated installer;
- CI host tests for DSP, sequencer, runtime, pattern/MIDI, project and hardware-gate logic.

The next release-critical work is integration into the real pinned Felucca FM-1 target: audio callback, physical UI, flash persistence, USB/TRS adapters, WASM emulator, linker-map/CPU-budget checks and only then a controlled hardware beta.

Read [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) and [`docs/M6_HARDWARE_BETA.md`](docs/M6_HARDWARE_BETA.md) before any future hardware experiment.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes most 808/909-style voices and uses CC0/Public Domain samples only where useful, especially cymbals/ride/crash. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are references to historical instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from any Felucca-derived source and from all included third-party components.
