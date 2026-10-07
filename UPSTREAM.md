# Upstream foundation

TheReFmB1rth is intended to use the FM-1 hardware support, updater, emulator, test infrastructure and selected architecture from:

- `hugelton/Felucca`
- upstream branch: `main`
- license: GPL-3.0-only

## Import policy

Before importing source:

1. record the exact Felucca commit SHA;
2. preserve per-file copyright/SPDX headers;
3. preserve `LICENSE`, `LICENSING.md`, and required `LICENSES/` texts;
4. preserve third-party attributions for fonts, DSP ports, SDK components and samples;
5. mark files that are substantially modified for TheReFmB1rth;
6. keep hardware/updater changes minimal until a stable baseline has been reproduced.

## Update strategy

Felucca should be treated as an upstream platform rather than copied piecemeal without provenance. Hardware fixes, USB fixes, loader fixes and FM-1 recovery improvements should be evaluated regularly and ported deliberately.

The first imported baseline must pass Felucca's existing build/tests/emulator unchanged before TheReFmB1rth-specific DSP work begins.
