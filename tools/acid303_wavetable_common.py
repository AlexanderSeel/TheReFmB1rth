#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Shared Open303-derived wavetable construction helpers.

The target and browser/HQ generators both start from the same 2048-sample
SAW303/SQUARE303 prototypes used by Open303. Only storage resolution and the
number of pitch bands differ. Keeping this in one module prevents hardware and
Virtual from drifting into different oscillator models.

For each mip level we retain exactly the harmonic bins permitted by the target
final-audio bandwidth, then reconstruct a periodic table. Unlike the earlier
prototype this helper also preserves the source prototype's mean/DC component;
that matters for faithful reproduction because Open303's SAW303/SQUARE303
prototype shaping is intentionally asymmetric before the later AC-coupling
stages remove the audible DC component.
"""
from __future__ import annotations

import cmath
import math

REFERENCE_SIZE = 2048
OUT_SR = 44100.0
TANH_DRIVE_DB = 36.9
TANH_OFFSET = 4.37
SQUARE_PHASE_DEG = 180.0


def midi_hz(note: int) -> float:
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def phase_inc(note: int) -> int:
    return round((2.0**32) * midi_hz(note) / OUT_SR)


def open303_prototypes(size: int = REFERENCE_SIZE) -> tuple[list[float], list[float]]:
    """Return phase-aligned Open303 SAW303/SQUARE303 prototypes."""
    n1 = max(1, min(size - 1, math.floor(0.5 * (size - 1) + 0.5)))
    n2 = size - n1
    s1 = 1.0 / (n1 - 1)
    s2 = 1.0 / n2
    saw = [s1 * i if i < n1 else -1.0 + s2 * (i - n1) for i in range(size)]

    shaper = 10.0 ** (TANH_DRIVE_DB / 20.0)
    square = [-math.tanh(shaper * x + TANH_OFFSET) for x in saw]
    shift = math.floor(size * SQUARE_PHASE_DEG / 360.0 + 0.5) % size
    if shift:
        square = square[-shift:] + square[:-shift]
    return saw, square


def spectrum(proto: list[float], max_harmonics: int) -> tuple[float, list[complex]]:
    """Return normalized DC plus positive-frequency DFT bins.

    Only bins that a generated table can actually reproduce are evaluated. A
    1024-sample target therefore still derives its first 511 harmonics from the
    full 2048-sample Open303 prototype instead of from a coarsened prototype.
    """
    n = len(proto)
    hmax = min(max_harmonics, n // 2 - 1)
    dc = sum(proto) / n
    coeff = [0j] * (hmax + 1)
    for k in range(1, hmax + 1):
        step = cmath.exp(-2j * math.pi * k / n)
        ph = 1.0 + 0.0j
        acc = 0.0 + 0.0j
        for x in proto:
            acc += x * ph
            ph *= step
        coeff[k] = acc / n
    return dc, coeff


def positive_spectrum(proto: list[float], max_harmonics: int) -> list[complex]:
    """Compatibility helper returning only positive-frequency bins."""
    return spectrum(proto, max_harmonics)[1]


def render_band(
    coeff: list[complex],
    size: int,
    harmonics: int,
    scale: float,
    dc: float = 0.0,
) -> list[int]:
    """Render one periodic Q15 table from a truncated Open303 spectrum."""
    hmax = max(1, min(harmonics, len(coeff) - 1, size // 2 - 1))
    out: list[int] = []
    for i in range(size):
        step = cmath.exp(2j * math.pi * i / size)
        ph = step
        z = 0.0 + 0.0j
        for k in range(1, hmax + 1):
            z += coeff[k] * ph
            ph *= step
        v = dc + 2.0 * z.real
        out.append(max(-32768, min(32767, round(v * scale))))
    return out


def harmonic_budget(note: int, size: int, bandlimit_hz: float) -> int:
    return min(size // 2 - 1, max(1, int(bandlimit_hz // midi_hz(note))))


def q15_stats(values: list[int]) -> tuple[int, float]:
    peak = max(abs(v) for v in values)
    rms = math.sqrt(sum(float(v) * float(v) for v in values) / len(values))
    return peak, rms
