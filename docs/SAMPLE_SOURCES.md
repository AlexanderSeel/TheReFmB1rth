# Sample sources and redistribution policy

TheReFmB1rth only ships audio assets whose redistribution rights are clear and auditable.

## Policy

For every bundled sample keep the source repository, exact commit, original path, declared license, Git blob identity, downloaded SHA-256, processing steps and processed SHA-256. A sample without provenance does not ship.

`free`, `royalty-free`, or `free for music production` is not enough for firmware redistribution. User permission is useful for user-owned material, but it cannot grant rights that belong to an unrelated third party. User-supplied samples may be imported locally when the user has the required rights; they are not automatically committed or redistributed by this project.

## Approved source manifest

`samples/sources.json` is now the allow-list used by `tools/fetch_samples.py`. The fetcher accepts no arbitrary URL, requires a redistribution-compatible license, pins source commits, verifies the exact Git blob bytes, and writes a `build/samples.lock.json` containing SHA-256 hashes for the downloaded files.

### 808 — Fischer/Loveall CC0 set

Pinned upstream: `tidalcycles/sounds-tr808-fischer@85fbecf1bec32553395625ea659e2a56dfd7c0e1`.

The upstream repository carries CC0-1.0 and preserves the Michael Fischer TR-808 sample lineage. The initial approved subset includes kick, snare, clap, closed/open hat and rim. More tone/decay variants can be added to the manifest when they are useful enough to justify firmware space.

### 909-style — Octal house-core CC0 set

Pinned upstream: `octalmusic/octal-samples@22d222ce36e290ba3792c776811aa9c69dcc8776`.

The pack is CC0-1.0. Most selected voices are deterministically synthesized from classic 808/909-style recipes; crash and ride are deterministic derivatives of public-domain/Unlicense cymbal recordings with provenance retained by the upstream project. The initial subset includes kick, snare, clap, closed/open hat, crash and ride.

These are independent redistributable assets. They are not Roland factory samples and must not be presented as such.

## Oramics TR-909 Detroit

The Oramics collection is still useful for A/B listening research, but the repository describes collections as having individual licenses and the Detroit collection metadata points to an external source without an explicit collection-level redistribution license in its README. It therefore remains **research-only** until an exact asset license is pinned. It is not part of `samples/sources.json`.

## Fetching approved samples

```bash
python3 tools/fetch_samples.py --validate-only
python3 tools/fetch_samples.py --kit 808
python3 tools/fetch_samples.py --kit 909
```

Downloaded files live under `build/samples/` and the resolved SHA-256 provenance is written to `build/samples.lock.json`.

## Firmware strategy

The synthesized 808/909 engines remain available as the zero-asset fallback. Sample-backed voices will be introduced selectively, beginning with hats/cymbals and A/B comparison in the browser. This avoids spending scarce FM-1 flash/RAM on samples that do not materially improve sound quality.

Before any sample is embedded into the `.fwsc` target build, the release gate must verify that every bundled asset appears in the approved manifest and resolved lock and that its processed payload fits the target memory/CPU budget.
