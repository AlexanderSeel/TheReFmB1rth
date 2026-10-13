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

**M5 groovebox functionality, the Felucca integration overlay, the RB-240 ReBirth-inspired 240×240 hardware/virtual UI and the keyboard-first performance interaction layer are implemented software-side. A real experimental FM-1 `.fwsc` builds in GitHub Actions. `v0.1.0-beta.2` remains the published hardware beta and is intentionally not overwritten by the current interaction work.**

Implemented foundations include:

- two acid engines and two synthesized 11-lane drum engines in one four-machine runtime;
- 16-step sequencing with accent/slide/tie/probability primitives;
- complete A–H pattern persistence and quantized song-chain pattern changes;
- level/pan/mute/solo mixer, delay/reverb sends, drive, compressor, delay, performance low-pass and bounded master output;
- separate RB-240 PCF / Delay / Dist / Comp FX panels over the shared master FX rack;
- internal clock plus MIDI START/CONTINUE/STOP/CLOCK;
- MIDI note/CC/program routing for acid and drum machines;
- versioned CRC-protected project serialization and Felucca A/B atomic flash integration;
- shared target/WASM runtime and shared `ui240` interaction model;
- Felucca Q15 audio callback integration plus shared USB/TRS MIDI ingress;
- release manifests, SHA/product/device checks and a double-confirmed experimental installer wrapper that delegates to the pinned Felucca loader-mediated installer;
- CI tests for DSP, sequencer, runtime, pattern/MIDI, projects, keyboard interaction and hardware-gate logic.

## Keyboard-first live workflow

The physical FM-1 and the built Virtual FM-1 now use the same `ui240_performance` interaction layer. The 27-key keyboard is contextual instead of being only an ACID note keyboard. A normal key press on ACID still plays the selected synth; the held layers below temporarily reinterpret the keyboard for editing and performance.

| Hold | Keyboard layer |
| --- | --- |
| `SEQ` | write the played note directly into the selected ACID step |
| `ARP/DRUM` | 808/909 drum entry and immediate audition |
| `GLO/MIXER` | track mute/solo and machine selection |
| `FX` | direct PCF / Delay / Dist / Comp selection plus performance throws |
| `SAVE` | direct Pattern A–H selection |
| `EDIT` | Accent / Slide / Tie / Rest / probability shortcuts; high/accent drum entry |
| `REC` | live capture into the currently running sequencer step |
| `SCL/SEL` | enter the quick FOCUS navigation layer |

`HOME` remains the deterministic escape/panic action. Page and machine changes keep the existing stuck-note safety behavior.

### 808 vs 909 keyboard workflow

The two drum machines deliberately do not share one anonymous pad order.

- **808:** the first 11-key bank maps `BD, SD, RS, CP, CH, OH, LT, MT, HT, CR, RD`; the second 11-key bank writes the same lanes as high/accent hits. The ordering keeps the characteristic rim/clap percussion close to kick/snare.
- **909:** the first 11-key bank maps `BD, SD, LT, MT, HT, RS, CP, CH, OH, CR, RD`; the second 11-key bank is the HIGH/accent bank. The trailing performance keys expose the selected-lane flam toggle and flam-width down/up controls.
- Drum key presses also audition the selected voice immediately, so programming does not require starting transport first.
- With the `REC` layer while transport runs, drum hits are written at the current running step.

The 909 flam state is the real `drum_909_flams` sequencer state, not a cosmetic UI flag.

### FOCUS mode for the 240×240 display

`SCL/SEL` enters a full-panel FOCUS mode for dense RB-240 pages. It is a navigation zoom, not bitmap magnification: the selected logical panel occupies the normal 240×240 editing surface and therefore keeps the hardware and Virtual FM-1 layout identical.

- Keyboard selectors 1–8 jump directly to **ACID**, **STEP**, **DRUM**, **MIXER**, **PCF**, **DELAY**, **DIST**, or **COMP**.
- PRESET / ALGORITHM move through the focus regions while FOCUS is active.
- K1–K4 continue to edit the current focused page using the same page-specific parameter contract.
- `HOME` exits FOCUS immediately and provides the normal panic-safe escape.

The focus state and modifier state are exposed by both the Felucca bridge and the WASM panel ABI, so browser tests exercise the same state machine as the hardware overlay.

## FX and mixer routing

The mixer remains intentionally small enough for the FM-1 target budget. Each of the four machines has level, pan, mute, solo, delay send and reverb send. The summed dry/return signal then passes through the master performance rack: **Dist → Comp → PCF**, while the delay and reverb are send/return effects mixed before those master inserts. The 240×240 FX page exposes separate **PCF**, **Delay**, **Dist** and **Comp** sections; holding `FX` gives keyboard access to those four sections and to fast cutoff/delay/drive/compressor throws.

The WASM build uses larger delay/reverb buffers where browser memory permits, while the FM-1 keeps conservative buffers until physical profiling proves more target pool can safely be spent.

## Virtual/hardware interaction parity

The official Virtual FM-1 artifact is produced by `web/emu/build_wasm.sh`. The build exports the same keyboard-performance ABI used by the hardware bridge and applies the virtual panel mapping after copying the app into `build/wasm/app/`. Physical key index, note value and press/release state are all passed into the shared interaction model, so 808/909 mapping, Pattern A–H switching, mutes/solos, FX actions and FOCUS navigation can be rehearsed before a physical flash.

## Build everything in GitHub

`.github/workflows/build-all.yml` builds the project on every push to `main` and can also be started manually with **Actions → Build all artifacts → Run workflow**.

It produces two downloadable Actions artifacts:

1. **`refm-wasm-<commit>`**
   - `refm.js`
   - `refm.wasm`
   - browser app including Virtual FM-1
   - `SHA256SUMS`

2. **`refm-fm1-experimental-<commit>`**
   - `TheReFmB1rth-experimental.fwsc`
   - `TheReFmB1rth-app.bin`
   - Felucca `ota.bin`
   - `manifest.json`
   - overlay provenance stamp
   - `SHA256SUMS`

The FM-1 job reproducibly prepares the exact pinned Felucca 1.0.5 checkout, applies the deterministic ReFmB1rth overlay, downloads the official JieLi pi32v2 toolchain, fetches the required AC79 SDK, performs the actual target compile/link/package build, verifies the generated firmware manifest, and only then uploads the artifact. **The workflow never connects to or flashes a physical FM-1.**

Useful local/CI-equivalent checks are:

```sh
tests/run_host_tests.sh
sh web/emu/build_wasm.sh
# target package: run the Build all artifacts workflow, which bootstraps the pinned
# Felucca/JieLi/AC79 environment and performs the real FM-1 package build.
```

## Release policy

`v0.1.0-beta.2` is immutable. The keyboard/interaction/UI work on `main` is an unreleased next-beta candidate. Do not repoint the beta.2 firmware channel or publish a replacement package until the interaction layer, WASM artifact and real FM-1 package build are green and the new workflow has been exercised on hardware.

Read [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) and [`docs/M6_HARDWARE_BETA.md`](docs/M6_HARDWARE_BETA.md) before any future hardware experiment.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes most 808/909-style voices and uses CC0/Public Domain samples only where useful, especially cymbals/ride/crash. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are references to historical instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from any Felucca-derived source and from all included third-party components.
