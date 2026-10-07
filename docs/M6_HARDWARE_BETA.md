# M6 hardware beta safety path

M6 introduces the guard rails required before a real FM-1 hardware experiment. It does **not** claim that the current firmware is ready to flash, and no physical-device test has been performed by this repository automation.

## Implemented

1. `tools/release_manifest.py` creates a deterministic manifest containing artifact size, SHA-256, FM-1 package identity, pinned Felucca upstream commit, local commit and the expected loader-mediated install protocol.
2. `tools/hardware_gate.py` is non-writing and rejects a package unless its SHA-256 matches, its embedded identity is FM-1 shaped, the connected identity is FM-1 shaped, `--experimental` is present and the exact confirmation text is supplied.
3. `tools/experimental_install.py` defaults to dry-run. Even after the first gate passes it cannot write unless `--execute` and a second exact risk acknowledgement are supplied.
4. The execution path delegates to the **pinned Felucca `fm1_install.py`** after `tools/bootstrap_felucca.sh`; it does not implement a new raw-flash writer.
5. Host tests intentionally cover hash mismatch, missing acknowledgement, wrong device identity and manifest tampering.

## Required before the first physical flash

- build an actual TheReFmB1rth `.fwsc` from the pinned Felucca foundation;
- pass all inherited Felucca host/update/loader/emulator tests;
- pass TheReFmB1rth host DSP/sequencer/mixer/project tests;
- verify linker map XIP, RAM, pool and stacks;
- run worst-case CPU/audio-underrun testing;
- generate and independently verify the release manifest;
- use `fm1_install.py --info` to read the device identity before installation;
- have the official V15 package available for return-to-stock;
- have the FM-1-transporter recovery procedure and required hardware available before experimenting;
- use a known-good USB data cable and stable power; do not disconnect during loader/write/reboot;
- perform the first hardware experiment on a device whose loss can be tolerated.

## Example dry run

```sh
tools/bootstrap_felucca.sh
python3 tools/release_manifest.py build/the-refmb1rth-dev.fwsc --commit "$(git rev-parse HEAD)" --output build/manifest.json
python3 tools/hardware_gate.py build/the-refmb1rth-dev.fwsc \
  --sha256 <sha-from-manifest> \
  --device-identity FM-1_15 \
  --experimental \
  --confirm "FLASH THE FM-1"
python3 tools/experimental_install.py build/the-refmb1rth-dev.fwsc \
  --sha256 <sha-from-manifest> \
  --device-identity FM-1_15 \
  --experimental \
  --confirm "FLASH THE FM-1"
```

The final command above remains a dry run. Actual execution additionally requires `--execute --execute-confirm "I ACCEPT THE EXPERIMENTAL FLASH RISK"`.

There is no zero-brick guarantee. The design goal is to minimize preventable mistakes and preserve Felucca's tested loader-mediated path.
