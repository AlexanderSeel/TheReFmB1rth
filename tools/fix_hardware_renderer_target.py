#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Remove division/modulo helper calls from the physical 240x240 renderer.

The JieLi target safety gate rejects undocumented ROM references. The clean
hardware renderer does not need exact integer division: percentages can use
Q15/Q7 shifts and MIDI note names can be derived by bounded subtraction.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-hardware-render32"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{label} anchor mismatch: expected 1, got {n}")
    return text.replace(old, new, 1)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-gain32").is_file():
        raise SystemExit("apply target-safe gain correction before renderer correction")

    draw = root / "firmware" / "src" / "ui_draw.c"
    text = draw.read_text()

    text = replace_once(
        text,
        "static uint32_t rb5_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }",
        "static uint32_t rb5_pct(uint32_t v) { return (v * 100u + 16384u) >> 15; }",
        "Q15 percent",
    )

    old_note = '''static void rb5_note_name(uint8_t note, char out[5])\n{\n    static const char *const nn[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};\n    const char *n = nn[note % 12u];\n    int32_t oct = (int32_t)(note / 12u) - 1;\n    uint8_t p = 0u;\n    out[p++] = n[0];\n    if (n[1]) out[p++] = n[1];\n    if (oct < 0) out[p++] = '-';\n    else out[p++] = (char)('0' + (oct > 9 ? 9 : oct));\n    out[p] = 0;\n}\n'''
    new_note = '''static void rb5_note_name(uint8_t note, char out[5])\n{\n    static const char *const nn[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};\n    uint8_t pc = note;\n    int32_t oct = -1;\n    uint8_t p = 0u;\n    while (pc >= 12u) { pc = (uint8_t)(pc - 12u); ++oct; }\n    out[p++] = nn[pc][0];\n    if (nn[pc][1]) out[p++] = nn[pc][1];\n    if (oct < 0) out[p++] = '-';\n    else out[p++] = (char)('0' + (oct > 9 ? 9 : oct));\n    out[p] = 0;\n}\n'''
    text = replace_once(text, old_note, new_note, "note name")

    text = replace_once(
        text,
        "rb5_numbox(62,66,56,42,\"PAN\",((int32_t)p1-32767)*100/32767,RB5_RED);",
        "rb5_numbox(62,66,56,42,\"PAN\",(((int32_t)p1-32767)*100)>>15,RB5_RED);",
        "pan percent",
    )
    text = replace_once(
        text,
        "rb5_numbox(121,66,56,42,\"DELAY\",(int32_t)p2*100/127,RB5_RED);",
        "rb5_numbox(121,66,56,42,\"DELAY\",((int32_t)p2*100+64)>>7,RB5_RED);",
        "delay percent",
    )
    text = replace_once(
        text,
        "rb5_numbox(180,66,56,42,\"REVERB\",(int32_t)p3*100/127,RB5_RED);",
        "rb5_numbox(180,66,56,42,\"REVERB\",((int32_t)p3*100+64)>>7,RB5_RED);",
        "reverb percent",
    )

    # Power-of-two divisions are fine, but write the master percentage as an
    # explicit shift so the generated target contract is obvious/auditable.
    text = replace_once(
        text,
        "uint16_t master_pct = (uint16_t)(((song.master_q12 > 4096u ? 4096u : song.master_q12) * 100u) / 4096u);",
        "uint16_t master_pct = (uint16_t)(((song.master_q12 > 4096u ? 4096u : song.master_q12) * 100u) >> 12);",
        "master percent",
    )

    draw.write_text(text)
    (root / STAMP).write_text(
        "renderer-arithmetic=q15-q7-shifts\n"
        "note-name=bounded-subtraction\n"
        "integer-divmod=none-in-rb5\n"
    )
    print("FM-1 clean renderer converted to target-safe shift/subtraction arithmetic")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
