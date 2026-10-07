# TheReFmB1rth

Experimental replacement firmware project for the M-VAVE FM-1: a compact four-machine acid groovebox inspired by the classic two-bassline/two-drum-machine workflow.

## Target

- ACID 1 — original 303-inspired mono synth
- ACID 2 — second independent acid synth
- DRUM A — 808-inspired drum machine
- DRUM B — 909-inspired drum machine
- pattern/song sequencer
- mixer + performance FX
- USB/TRS MIDI and USB audio
- browser/WASM emulator
- guarded FM-1 installer based on the proven Felucca update path

See [`plan.md`](plan.md) for the implementation roadmap.

## Current status

**Bootstrap / research phase. Do not flash anything from this repository to an FM-1 yet.**

The repository is being prepared around the current Felucca firmware architecture. Hardware flashing will remain disabled/unrecommended until the upstream base is imported, builds/tests/emulator are green, package checks are implemented, and a recovery procedure has been validated.

Read [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) before any future hardware experiment.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes most 808/909-style voices and uses CC0/Public Domain samples only where useful, especially cymbals/ride/crash. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are references to historical instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from any Felucca-derived source and from all included third-party components.
