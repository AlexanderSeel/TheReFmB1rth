#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make Felucca's existing ROM-reference safety error actionable.

This does not relax the safety check. It maps the offending instruction address
back to the nearest .text symbol from objdump's symbol table and includes that
symbol plus the instruction in the existing safety error.
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
    for ln in dis.splitlines():     # nothing may call or load an address in the chip ROM
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
                errors.append(f"reference to ROM address {val:#010x} in {rom_owner} at {pc:#010x}: {ln.strip()}")
'''
    if s.count(old) != 1:
        raise SystemExit("Felucca ROM diagnostic anchor mismatch")
    p.write_text(s.replace(old, new, 1))
    print("Felucca ROM-reference diagnostics enabled with symbol-table ownership (safety gate unchanged)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
