#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make Felucca's existing ROM-reference safety error actionable.

This does not relax the safety check. It only adds the owning disassembly symbol
and instruction to the error so target-only compiler helper calls can be removed.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    p = args.felucca.resolve() / "tools" / "build.py"
    s = p.read_text()
    old = '''    for ln in dis.splitlines():     # nothing may call or load an address in the chip ROM
        mm = LINE.match(ln)
        if not mm:
            continue
        for v in re.findall(r"= (-?\\d+) <|call -?\\d+ <[^:>]*: ([0-9a-f]+) >", mm.group(3)):
            val = (int(v[0]) & 0xFFFFFFFF) if v[0] else int(v[1], 16) & 0xFFFFFFFF
            if 0xFFC00000 <= val < 0xFFD00000:
                errors.append(f"reference to ROM address {val:#010x}")
'''
    new = '''    rom_owner = "<unknown>"
    for ln in dis.splitlines():     # nothing may call or load an address in the chip ROM
        fn = re.match(r"^[0-9a-f]+ <([^>]+)>:$", ln.strip())
        if fn:
            rom_owner = fn.group(1)
        mm = LINE.match(ln)
        if not mm:
            continue
        for v in re.findall(r"= (-?\\d+) <|call -?\\d+ <[^:>]*: ([0-9a-f]+) >", mm.group(3)):
            val = (int(v[0]) & 0xFFFFFFFF) if v[0] else int(v[1], 16) & 0xFFFFFFFF
            if 0xFFC00000 <= val < 0xFFD00000:
                errors.append(f"reference to ROM address {val:#010x} in {rom_owner}: {ln.strip()}")
'''
    if s.count(old) != 1:
        raise SystemExit("Felucca ROM diagnostic anchor mismatch")
    p.write_text(s.replace(old, new, 1))
    print("Felucca ROM-reference diagnostics enabled (safety gate unchanged)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
