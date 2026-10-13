#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Mirror the eight-position FOCUS location indicator in the built Virtual FM-1 renderer."""
from __future__ import annotations
import sys
from pathlib import Path


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise SystemExit(f"virtual RB-240 focus anchor mismatch: {old[:100]!r}")
    return text.replace(old, new, 1)


def main() -> int:
    path = Path(sys.argv[1])
    text = path.read_text()
    text = replace_once(
        text,
        "lamp(g,124,40,true);lamp(g,164,40,false);lamp(g,211,40,false);}",
        "lamp(g,124,40,true);lamp(g,164,40,false);lamp(g,211,40,false);if(s.focus){for(let i=0;i<8;i++)lamp(g,48+i*6,7,i===s.focusRegion,C.cyan);}}",
    )
    text = replace_once(
        text,
        "songLoop=v(27),songLength=v(28);",
        "songLoop=v(27),songLength=v(28),focus=!!v(34),focusRegion=v(35)&7;",
    )
    text = replace_once(
        text,
        "songLoop,songLength,projectResult,cutoff:",
        "songLoop,songLength,focus,focusRegion,projectResult,cutoff:",
    )
    path.write_text(text)
    print("Virtual RB-240 FOCUS location indicator applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
