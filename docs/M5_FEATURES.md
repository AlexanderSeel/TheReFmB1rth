# M5 feature expansion

The host groovebox now includes A–H pattern-bank primitives and channel MIDI routing in addition to the unified four-machine runtime.

- Eight pattern slots store both acid tracks plus both 11-lane drum grids.
- Patterns can be cleared, copied, captured from the live groovebox and loaded back into the runtime.
- MIDI channels 1/2 address ACID 1/2; channels 3/4 can address the two drum machines; channel 10 targets the selected drum machine.
- Note on/off, program change and core CCs are decoded without dynamic allocation.
- Acid CC mapping includes cutoff (74), resonance (71), decay (73), env-mod (1), level (7) and pan (10).
- Drum routing uses a General-MIDI-style core map for kick, snare, clap, rim, hats, toms, crash and ride. Level/pan use CC 7/10.
- The channel-10 drum target can be toggled through CC 20 in the prototype routing layer.

The FM-1 USB/TRS adapters still need to feed these functions from the real Felucca MIDI receive path, and UI pattern switching must use quantized boundaries rather than calling the host helper directly.
