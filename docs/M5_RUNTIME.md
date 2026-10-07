# M5 unified runtime

The groovebox prototype now has an actual four-machine render path rather than disconnected experiments.

`groovebox.*` owns two ACID engines, two 16-step acid sequencers, an 808-inspired synthesized drum machine, a 909-inspired synthesized drum machine, two 11-lane drum grids, shared MIDI/internal transport, song state and the four-channel mixer/FX chain.

`drum_machine.*` is an original fixed-point/no-allocation drum synthesis prototype covering BD, SD, clap, rim, closed/open hats, low/mid/high toms, crash and ride. The 808 and 909 modes use different tuning/level characteristics; closed hat chokes open hat. It is intentionally an original approximation rather than copied ROM/sample data.

The runtime can be clocked internally or by MIDI realtime messages. A 16th-step event advances both acid sequencers and both drum grids. Pattern-boundary hooks advance song state. Audio from all four machines is then routed through the shared mixer/FX path.

Project save/load now serializes BPM, acid parameters, acid steps, drum grids, mixer/FX state and song chain into the versioned CRC-protected project envelope. Runtime-only audio states such as delay buffers, filter histories and oscillator phase are intentionally not persisted.

## Remaining M5 hardware integration

- replace prototype drum voicing with profiled/tuned production DSP;
- add real A-H pattern banks instead of the current active-pattern state/hook;
- integrate the runtime with Felucca's FM-1 audio callback and UI controls;
- bind save/load to Felucca flash sectors atomically;
- wire MIDI notes/CCs and USB/TRS routing, not only realtime sync;
- run WASM emulator and FM-1 CPU/linker-budget regressions.

This remains host firmware logic, not a release image.
