# Sample sources and redistribution policy

TheReFmB1rth must only ship audio assets whose redistribution rights are clear and auditable.

## Policy

For every bundled sample keep:

- source project/repository;
- exact source URL and commit/tag where possible;
- original filename;
- declared license;
- original SHA-256;
- processing steps;
- processed SHA-256;
- attribution text when required.

A sample without provenance does not ship.

`free`, `royalty-free`, or `free for music production` is **not** enough for firmware redistribution. Many sample packs allow use in songs but forbid repackaging/redistribution of the raw samples.

## 808 candidates

### Michael Fischer / Edward Loveall TR-808 sample lineage

Open-source music projects distribute the Fischer/Loveall TR-808 recordings as public-domain/CC0 material. Candidate upstreams include the TidalCycles ecosystem and mirrors that explicitly retain the public-domain/CC0 provenance.

Status: **candidate for bundling after exact upstream files + license provenance are pinned and hashes recorded.**

Use cases if accepted:

- cymbal / hi-hat reference or optional sample-backed voices;
- A/B validation against synthesized 808-style voices.

Preferred production direction remains original synthesis for kick, snare, clap, toms and cowbell.

## 909 candidates

### Oramics sampled — TR-909 Detroit

The Oramics `sampled` project is a collection of sampled instruments with open/public-domain-style licenses and includes a `TR-909 Detroit` collection.

Status: **preferred 909 sample research candidate.** Before importing, pin the exact collection metadata/license and hashes.

Most useful for:

- ride;
- crash;
- potentially hats where a sample-backed implementation gives materially better authenticity/CPU usage.

Kick/snare/toms/clap should first be attempted as original synthesis.

## CC0 synthesis reference

### octalmusic/octal-samples

This project states that its shipped house-core samples are either synthesized from scratch or derived from verified public-domain material and are released under CC0-1.0. Its classic-machine-inspired drum synthesis recipes are valuable references for creating our own redistributable assets.

Status: **strong reference candidate**, subject to respecting its source-code license separately from the generated audio asset license.

## Sources not suitable for bundling by default

SampleRadar/MusicRadar and similar packs may be royalty-free for musical use while explicitly prohibiting redistribution. Do not put such raw samples in firmware, release ZIPs, Git history, or automated fetch scripts unless the license expressly permits redistribution.

Users may optionally import their own legally obtained samples in the future, but those files are not part of TheReFmB1rth.

## Preferred strategy

For v1:

1. synthesize 808-style kick, snare, clap, toms, cowbell and metallic voices in firmware;
2. synthesize 909-style kick, snare, toms and clap;
3. benchmark synthesized hats/cymbals against small openly licensed sample assets;
4. use samples only where they improve sound enough to justify flash storage;
5. preprocess all assets to the firmware's actual requirements rather than storing oversized WAV files.

## Planned tooling

`tools/fetch_samples.py` will use a manifest rather than arbitrary URLs. Each entry should contain fields similar to:

```json
{
  "id": "909-ride",
  "source": "...",
  "commit": "...",
  "license": "Public-Domain",
  "sha256": "...",
  "target_rate": 44100,
  "channels": 1,
  "trim": true
}
```

The fetch step must fail on checksum mismatch.

Release builds must fail if a bundled audio asset is not represented in the provenance manifest.
