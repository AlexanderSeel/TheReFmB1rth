# TheReFmB1rth v0.1.0-beta.2 — physical FM-1 validation

Target release: `v0.1.0-beta.2`

Exact firmware commit: `2b4c537cc1a1f8c42c43ee953e14e27d7740837b`

Firmware-channel SHA-256: `f2d2e13089a52cf279cee0bc43d9d4791ae913f0b3b889ec5b3de02b70c667b2`

This runbook is the second physical FM-1 validation pass. Do not treat a successful install as sufficient proof. The beta is accepted only when the instrument workflow, target runtime gates and recovery path all pass on real hardware.

## 0. Recovery preparation

Before flashing:

- keep the verified official V15 package and its known SHA available;
- keep the known-good Felucca/recovery path available;
- keep FM-1-transporter/XIAO-RP2040 recovery notes and wiring available as the last-resort path;
- use a direct USB connection, not an unreliable hub;
- do not start the test if the recovery path cannot be reproduced.

No custom firmware has a zero-brick guarantee.

## 1. Install beta.2

Use the browser installer and load the current firmware-channel package.

Expected identity:

- tag: `v0.1.0-beta.2`
- commit: `2b4c537cc1a1f8c42c43ee953e14e27d7740837b`
- asset: `TheReFmB1rth-experimental.fwsc`
- bytes: `609849`
- SHA-256: `f2d2e13089a52cf279cee0bc43d9d4791ae913f0b3b889ec5b3de02b70c667b2`

Pass conditions:

- package SHA is accepted before flashing;
- OTA loader handoff completes;
- device reboots into TheReFmB1rth;
- browser reconnect/identity check succeeds;
- no recovery intervention is required.

## 2. Basic boot + LCD

Immediately after boot:

1. Verify HOME appears as a ReFm screen, not a Felucca synth page.
2. Check machine, pattern, BPM and transport state are visible.
3. Visit HOME / ACID / STEP / DRUM / MIXER / FX / SONG / PROJECT.
4. Check every page for clipping, stale text, redraw corruption and unreadable footer labels.
5. Rotate K1–K4, PRESET, ALGO and SELECT and compare behavior with the LCD footer.

Fail immediately if the old Felucca workflow owns normal operation again.

## 3. Live keyboard and ACID A/B

For ACID A and then ACID B:

- press/release every physical key several times;
- hold a note while moving Cutoff, Resonance, Env Mod and Decay;
- switch SAW/SQUARE;
- switch A -> B -> A while notes are being played;
- enter/leave STEP while a note is held;
- verify release/panic prevents stuck notes.

Pass conditions:

- immediate audible attack;
- no long silence/lag before notes;
- release stops notes reliably;
- no stuck note after page/machine changes;
- controls audibly correspond to their labels.

## 4. Factory Pattern A + transport

1. Return to Pattern A.
2. Press physical PLAY.
3. Confirm the factory demonstration is audible immediately.
4. Press STOP and verify audio/sequencer stop cleanly.
5. Repeat START/STOP at least 20 times.
6. Repeat with browser START / CONTINUE / STOP.

Pass conditions:

- physical PLAY owns the ReFm transport;
- browser and physical transport remain coherent;
- no Felucca legacy sequencer becomes audible;
- no stuck voices after STOP.

## 5. STEP editor

On ACID A and B:

- select all 16 steps with PRESET;
- create NOTE, REST and TIE steps;
- toggle Accent and Slide;
- change probability;
- alter notes and play the pattern after each edit.

Check that the selected step on the LCD matches the audible/edit result.

## 6. DRUM editor

Test both 808 and 909:

- select every lane;
- program OFF / HIT / ACCENT states;
- create a dense 16-step pattern;
- audition drum voices from the browser pads;
- switch 808 <-> 909 while stopped and while playing.

Pass conditions:

- edits affect the intended lane/step only;
- 808 and 909 state remains independent;
- no transport glitch or stuck drum activity.

## 7. MIXER + FX under load

While all four machines play:

- move each channel Level and Pan;
- exercise Delay/Reverb sends;
- toggle Mute/Solo repeatedly;
- move master Drive, Delay feedback, Reverb mix and Filter;
- change pages and edit steps while the pattern continues.

Pass conditions:

- UI remains responsive;
- no obvious audio starvation/click bursts caused by UI activity;
- mute/solo state is coherent between hardware and browser sync.

## 8. Browser state synchronization

Use the device-state sync/readback feature after making changes only on the FM-1.

Verify the browser reflects:

- machine;
- pattern;
- selected step/lane;
- transport state;
- ACID controls/step state;
- drum state;
- mixer mute/solo/current parameters.

Do not count a browser-local value as proof; change the hardware first, then request readback.

## 9. Project backup / restore

1. Build a recognizable A–H project containing ACID and drum edits.
2. Stop transport.
3. Back up the full project in the browser.
4. Deliberately change multiple patterns and mixer settings.
5. Restore the browser backup.
6. Verify A–H content and mixer/FX/song state return.
7. Power-cycle before saving and verify the restored live project was not silently persisted.
8. Restore again, explicitly save to a physical PROJECT slot, power-cycle and reload the slot.

Pass conditions:

- transfer refuses while transport is running;
- restore reconstructs the complete project;
- restore itself does not write flash;
- explicit PROJECT SAVE persists correctly.

## 10. Runtime diagnostic capture — factory pattern

With Pattern A playing normally for at least several minutes:

1. use **CAPTURE RUNTIME** in the browser;
2. download the diagnostic JSON;
3. record the browser PASS/FAIL result;
4. optionally verify on a PC:

```sh
python3 tools/target_runtime_report.py fm1-runtime.json --half-frames 128
```

Required gate:

- `halves > 0`
- `late == 0`
- sustained CPU <= 75%
- peak render <= 82%
- user-stack free >= 4096 B
- supervisor/ISR-stack free >= 1024 B

Keep the JSON as hardware evidence.

## 11. Worst-case four-machine stress capture

Build the heaviest practical pattern:

- both ACID engines running with high resonance/env-mod, accents and slides;
- dense 808 + dense 909 activity;
- all four mixer channels enabled;
- strong delay/reverb sends;
- master drive/filter/compression/delay active;
- continuously edit pages/steps/knobs;
- send MIDI while playing.

Run for several minutes, then capture/download runtime diagnostics again.

The same CPU/late/stack gate must pass. Do not relax thresholds to make the firmware pass.

## 12. USB MIDI

Verify:

- note on/off;
- CC edits;
- external MIDI clock/start/continue/stop;
- browser editor traffic while audio is running;
- state readback;
- project transfer;
- runtime diagnostic command.

No stuck notes, transport divergence or dropped editor state is acceptable.

## 13. TRS MIDI

Repeat basic note/clock/transport tests through TRS MIDI.

Verify USB editor traffic can coexist with TRS performance MIDI without corrupting ReFm state.

## 14. USB audio

With the full groovebox playing:

- select FM-1 USB audio input on the host;
- record several minutes;
- edit UI/MIDI during capture;
- listen for gaps, repeats, corrupted blocks or large timing discontinuities.

## 15. Persistence / corruption behavior

For every physical project slot:

- save a distinct project;
- reboot;
- load each slot;
- verify expected content.

Then test the documented corrupted/invalid-project path without modifying the recovery firmware area. The expected outcome is a safe project-load rejection/fallback, never arbitrary execution or unrelated flash damage.

## 16. Recovery / rollback

After the beta tests:

1. exercise the normal update/boot recovery route;
2. return to known-good official V15 or Felucca;
3. verify the restored firmware boots and basic audio/USB work;
4. reinstall beta.2 through the normal loader-mediated path;
5. repeat the rollback/update cycle more than once.

A beta cannot be called hardware-proven until rollback has been demonstrated on the physical test unit.

## Evidence to keep

For the beta.2 validation record keep:

- installation result and exact beta tag/commit/SHA;
- photos/screenshots of all eight ReFm pages;
- factory-pattern runtime JSON;
- worst-case runtime JSON;
- USB MIDI result;
- TRS MIDI result;
- USB audio result;
- project backup/restore + power-cycle result;
- physical slot save/load result;
- rollback/recovery result;
- any defect with exact page/control/action needed to reproduce it.

## Decision

- **PASS / hardware-proven beta:** every mandatory runtime, control, persistence and rollback gate passes.
- **FAIL / corrective build required:** any DMA late event, stack gate failure, unusable physical workflow, persistence corruption, USB/TRS regression or inability to recover/rollback.
