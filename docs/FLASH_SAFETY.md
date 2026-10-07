# FM-1 Flashing Safety

This project writes replacement firmware to the M-VAVE FM-1. No custom firmware process can guarantee zero brick risk. Development therefore uses a staged test and recovery policy.

## Before any experimental flash

- Build from a clean checkout.
- Run all host tests and DSP regression tests.
- Run the browser/WASM emulator.
- Inspect the linker map for XIP, RAM, pool and stack bounds.
- Verify the generated firmware package hash.
- Run the installer in information/preflight mode first.
- Confirm the connected device is actually an FM-1.
- Keep a copy of the official firmware/update path available.
- Read and prepare the FM-1-transporter recovery procedure before testing an experimental loader or image.

## Rules

1. Do not directly write arbitrary flash offsets for normal installation.
2. Preserve the proven Felucca update-loader/package path until there is a strong reason to change it.
3. Never change loader and application layout in the same first hardware test.
4. Do not flash an image whose linker/map validation failed.
5. Do not flash a package whose SHA-256 differs from the expected release manifest.
6. Do not allow the installer to silently select among multiple MIDI devices.
7. Development packages must be clearly marked experimental.
8. A corrupted settings/project area must never be able to overwrite loader or executable regions.

## Development bring-up order

1. Host/unit tests.
2. DSP golden renders.
3. Browser/WASM emulator.
4. Package parser tests.
5. Installer dry run / device identity query.
6. Experimental app image using unchanged proven loader.
7. Boot/display/audio verification.
8. USB MIDI/audio verification.
9. Persistence verification.
10. Return to official firmware/Felucca to prove the recovery/update route still works.

## Recovery

Felucca documentation states that an interrupted or failed install that leaves the FM-1 unable to start can require `FM-1-transporter`, which accesses the FM-1 flash through the chip's boot mode using external RP2040-based hardware. Treat that as a last-resort recovery path, not as part of the ordinary workflow.

Before the first experimental flash, review:

- Felucca BUILDING/installer documentation;
- Felucca README recovery notes;
- `kurogedelic/FM-1-transporter` instructions;
- the current official M-VAVE updater/firmware procedure.

## Release gate

A build is not marked `hardware-testable` unless CI confirms:

- tests pass;
- app image is inside XIP bounds;
- RAM and pool usage are inside bounds;
- stack guards are preserved;
- package header/product identity is valid;
- package checksum matches the release manifest;
- installer corruption/mismatch tests pass.

A build is not marked `stable` until it has also completed repeated update/rollback cycles on dedicated test hardware.
