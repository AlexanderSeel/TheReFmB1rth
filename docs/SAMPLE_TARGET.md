# Drum sample target policy

TheReFmB1rth keeps synthesis as the default drum implementation. Redistributable CC0/Public-Domain samples are an optional hybrid layer, not a requirement for a working groovebox.

## Browser audition set

The GitHub WASM artifact fetches the complete approved `samples/sources.json` allow-list, verifies each pinned Git blob, preprocesses the files to mono PCM16 at 22.05 kHz, and embeds them into the browser audition engine.

The first reproducible sample-backed build processed 13 assets into **307,996 bytes** of PCM. That is intentionally too large to treat as an automatic FM-1 firmware addition.

The full browser set exists so synth/sample A/B decisions can be made by ear before target flash is spent.

## FM-1 candidate set

`samples/target-candidates.json` is a separate, deliberately conservative allow-list. The initial target candidates are:

- 909-style closed hat
- 909-style open hat
- 909-style crash
- 909-style ride

`tools/check_sample_budget.py` calculates their processed PCM size from the generated sample manifest and refuses the candidate set above **96 KiB**.

Passing this sample budget does **not** embed the samples into the FM-1 image. It only proves the selected set is small enough to proceed to an explicit target-build experiment.

Before target embedding, the candidate set must also pass:

1. final XIP headroom after linking;
2. worst-case four-machine CPU/audio-underrun profiling;
3. deterministic audio regression tests;
4. physical A/B listening against the synthesized fallback;
5. recovery/return-to-stock preparation.

## Hybrid behavior

Each drum lane can independently have a sample configured. When sample playback is disabled or a lane has no sample, the existing synthesized 808/909-inspired voice remains active. Closed-hat choking applies to both synthesized and sample-backed open hats.

This keeps sample support optional and reversible and avoids turning TheReFmB1rth into a fixed sample ROM.
