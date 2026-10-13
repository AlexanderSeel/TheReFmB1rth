# Virtual FM-1 — combined installer/editor/WASM workflow

The browser build now combines four workflows in one application:

1. **INSTALLER** — verified loader-mediated firmware installation on a physical FM-1.
2. **LIVE EDITOR** — edits either a connected physical FM-1 or the virtual WASM device.
3. **VIRTUAL FM-1** — a full FM-1-shaped front panel around a 240×240 display.
4. **VALIDATION** — physical beta acceptance evidence after flashing hardware.

## Build and run

Build the browser/WASM package:

```sh
web/emu/build_wasm.sh
```

The combined application is emitted at:

```text
build/wasm/app/index.html
```

Serve `build/wasm` from a local HTTP server and open `/app/`. Do not open the files directly with `file://`; ES modules and WASM loading require HTTP/HTTPS.

## Virtual hardware model

The virtual panel follows the FM-1 geometry used by the GPL-3.0 `charlesvestal/fm1-x0x` browser emulator as a hardware-layout reference:

- 1000×640 panel coordinate space;
- physical 240×240 display position and scale;
- MASTER, SELECT, PRESETS, ALGORITHM and KNOB1–KNOB4 placement;
- FX / SEL / ENV / LFO / EDIT / GLO / HOME / SAVE / ARP / SEQ / PLAY / REC buttons;
- OCT− / OCT+ controls;
- 16 white + 11 black physical key positions using the FM-1 27-key index ordering.

No X0X synthesis/sequencer engine is used. The virtual instrument runs TheReFmB1rth's own WASM build.

## Shared firmware UI state

The WASM bridge includes `ui240.c` and exposes panel input functions whose IDs intentionally mirror the physical Felucca bridge:

- encoder 0–3: K1–K4;
- encoder 4: PRESETS;
- encoder 5: ALGORITHM;
- encoder 6: SELECT;
- actions for HOME, ACID A/B, STEP, DRUM, MIXER, FX, SONG, PROJECT, PLAY and contextual EDIT/REC.

The virtual display reads `ui240_snapshot()` from the same groovebox state and follows the dedicated target beta renderer's page/card/step layout. The browser also boots with the same beta factory Pattern A used to make the physical PLAY test immediately audible.

## Panel interaction

- Drag an encoder vertically or use the mouse wheel.
- Click/touch panel buttons and keys.
- `Space`: PLAY/STOP.
- `Enter`: REC/context action.
- `Esc`: HOME.
- `Z` / `X`: OCT− / OCT+.
- Arrow up/down: SELECT encoder.
- White keys 1–8: `Q`–`I`; white keys 9–16: `A`–`K`.
- Black keys: `1`–`0` and `-`.

On the PROJECT page, OCT− loads the virtual browser project and OCT+ saves it, matching the physical beta workflow. Outside PROJECT those buttons cycle the selected machine/track in the same direction as the hardware beta input path.

## Live Editor → virtual routing

Open **LIVE EDITOR** and choose **USE VIRTUAL**. The virtual device starts if necessary. The editor then mirrors its normal MIDI/editor messages into the WASM groovebox:

- ACID controls;
- pattern selection;
- 16-step ACID edits;
- 808/909 step edits;
- mixer controls;
- keyboard/drum audition;
- START / CONTINUE / STOP.

Transport has a single owner (`transport.js`) so a virtual START/STOP is not injected twice.

For the cleanest virtual-only test, do not keep a physical FM-1 editor connection active at the same time. Virtual routing is intended as the pre-flash rehearsal path; physical Web MIDI remains the post-flash path.

## What virtual testing proves

The virtual device is useful for validating:

- page flow and control mapping;
- pattern/step/drum editing semantics;
- project workflow;
- editor integration;
- audio behavior of the browser WASM reference;
- the expected 240×240 beta UI state and layout.

It **does not replace physical FM-1 validation**. Browser CPU load is not JieLi CPU load, browser audio scheduling is not the FM-1 DMA path, and it cannot prove target stack headroom, USB/TRS behavior, LCD electrical behavior, flash persistence or rollback/recovery. The physical VALIDATION workflow remains authoritative for those gates.

## Attribution

The full-device browser layout approach and FM-1 panel/key geometry were studied from `charlesvestal/fm1-x0x`, which is GPL-3.0 licensed. TheReFmB1rth keeps its own synthesis, sequencing, UI state, project format and WASM implementation; no X0X engine is embedded.
