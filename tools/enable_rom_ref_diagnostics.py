#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make Felucca's existing ROM-reference safety error actionable.

This does not relax the safety check. It maps each offending instruction back
to the nearest .text symbol and prints a small disassembly window around it so
target-only compiler helper loads can be removed from source instead of being
whitelisted.
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
    new = '''    rom_symbols = []
    for sl in syms.splitlines():
        sp = sl.split()
        if len(sp) >= 5 and ".text" in sp:
            try:
                rom_symbols.append((int(sp[0], 16), sp[-1]))
            except ValueError:
                pass
    rom_symbols.sort()
    dis_lines = dis.splitlines()
    for li, ln in enumerate(dis_lines):     # nothing may call or load an address in the chip ROM
        mm = LINE.match(ln)
        if not mm:
            continue
        pc = int(mm.group(1), 16)
        rom_owner = "<unknown>"
        for addr, name in rom_symbols:
            if addr > pc:
                break
            rom_owner = name
        for v in re.findall(r"= (-?\\d+) <|call -?\\d+ <[^:>]*: ([0-9a-f]+) >", mm.group(3)):
            val = (int(v[0]) & 0xFFFFFFFF) if v[0] else int(v[1], 16) & 0xFFFFFFFF
            if 0xFFC00000 <= val < 0xFFD00000:
                lo = max(0, li - 6)
                hi = min(len(dis_lines), li + 7)
                ctx = "\\n      ".join(x.rstrip() for x in dis_lines[lo:hi])
                errors.append(
                    f"reference to ROM address {val:#010x} in {rom_owner} at {pc:#010x}: {ln.strip()}"
                    f"\\n      disassembly context:\\n      {ctx}"
                )
'''
    if s.count(old) != 1:
        raise SystemExit("Felucca ROM diagnostic anchor mismatch")
    p.write_text(s.replace(old, new, 1))
    print("Felucca ROM-reference diagnostics enabled with symbol + instruction context (safety gate unchanged)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
