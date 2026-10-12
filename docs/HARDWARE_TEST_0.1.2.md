# Hardware test — hardware-test-0.1.2

Date: 2026-10-12

This was the first successful physical installation of TheReFmB1rth on an M-VAVE FM-1.

## Findings

- The dedicated 240x240 ReFm screens shown in design previews were not present. The build only contained a Felucca graph/step-strip overlay.
- Runtime/UI interaction felt very laggy and audio was mostly absent/intermittent.
- Physical knob/button mapping was confusing and did not correspond to a coherent 303/groovebox workflow.
- Live playing from the FM-1 keyboard did not drive the audible ReFm engine.
- Physical PLAY did not start the audible ReFm pattern sequencer.
- On-device editing was not usable as a simple step workflow.

## Root causes identified

1. Felucca remained the owner of physical keyboard/transport state while the audio callback had been replaced by the ReFm renderer. The panel and audible engine were therefore not fully connected.
2. The first UI integration reused Felucca ENV/LFO/EDIT pages and painted ReFm graphs/mappings over them rather than implementing dedicated ReFm pages.
3. Silent ACID voices still evaluated the expensive oversampled ladder path every sample on the FM-1 target, wasting CPU before real target profiling was available.

## Fixes for the next hardware build

- Mirror FM-1 physical PLAY state into the ReFm internal transport.
- Mirror physical keyboard note edges into ACID A/B.
- Remove stale ADSR/LFO ReFm panel mappings.
- Keep only a temporary predictable four-control ACID bridge until dedicated pages exist.
- Add an FM-1-only idle fast path for completely silent ACID voices.
- Treat the existing Felucca-derived UI as temporary and implement real Home/ACID/STEP/DRUM/MIX/FX pages before calling the hardware UI complete.

## Acceptance criteria for the second test

The next physical build is useful only if all of these pass:

1. Pressing a normal piano key on ACID A produces sound immediately.
2. Switching to ACID B and pressing a key produces ACID B sound.
3. Releasing a key reliably stops/releases the note; no stuck notes.
4. PLAY starts the ReFm internal sequencer and PLAY again stops it.
5. UI remains responsive while one ACID voice is sounding.
6. UI remains responsive while a pattern is running.
7. No Felucca ADSR/LFO page changes ReFm sound parameters.
8. The temporary EDIT four-knob bridge behaves consistently as cutoff / resonance / Env Mod / decay.
9. CPU/late-buffer diagnostics are captured before enabling heavier target FX or samples.

The dedicated 240x240 UI remains a blocker for a usable hardware beta even if the items above pass.
