# M5 groovebox core

M5 now has host-testable building blocks for the parts that turn the individual machines into a groovebox rather than isolated DSP experiments.

Implemented prototype modules:

- `song.*`: 64-slot pattern chain with repeat counts, looping and deterministic position state;
- `midi_transport.*`: MIDI realtime START/CONTINUE/STOP/CLOCK plus 24 PPQN to 16th-step conversion and an integer internal clock;
- `mixer_fx.*`: four-channel level/pan/mute/solo mixer, per-track delay send, master drive, compressor, delay, low-pass performance filter and limiter/bounding;
- `project_store.*`: versioned `RFM1` project envelope with payload length and CRC32 validation.

These modules are deliberately independent of the FM-1 HAL. That keeps their musical state, serialization and DSP behavior testable on the host before they are wired into the Felucca-derived audio callback, UI and flash sectors.

## Still required for M5 completion

- integrate ACID1, ACID2, 808 and 909 engines into the shared mixer;
- map project payload contents to the real machine/pattern/song/mixer structures;
- wire project envelopes to Felucca's proven flash persistence layer using atomic/versioned writes;
- wire USB/TRS MIDI to `midi_transport` and per-machine note/CC routing;
- connect song-mode changes to pattern banks on quantized boundaries;
- render mixer/song/transport state in the FM-1 UI and WASM emulator;
- add CPU-budget regression measurements around the full four-machine render path.

The host modules do not make this repository flashable. Hardware integration remains gated by M6.
