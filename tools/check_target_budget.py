#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Enforce conservative FM-1 memory headroom for TheReFmB1rth target builds."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

XIP_CAPACITY = 0x8DFBC
RAM_CAPACITY = 96 * 1024
POOL_CAPACITY = 0x54000
RAM_BASE = 0x01C08000

DEFAULT_MIN_XIP_HEADROOM = 64 * 1024
DEFAULT_MIN_RAM_HEADROOM = 12 * 1024
DEFAULT_MIN_POOL_HEADROOM = 64 * 1024


def parse_int(value: str) -> int:
    return int(value, 0)


def symbol_value(symbols: str, name: str) -> int:
    for line in symbols.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[-1] == name:
            try:
                return int(parts[0], 16)
            except ValueError:
                continue
    raise ValueError(f"missing linker symbol: {name}")


def inspect(image_size: int, symbols: str) -> dict:
    if image_size < 0:
        raise ValueError("negative image size")
    bss_end = symbol_value(symbols, "_bss_end")
    pool_start = symbol_value(symbols, "_pool_start")
    pool_end = symbol_value(symbols, "_pool_end")
    ram_used = bss_end - RAM_BASE
    pool_used = pool_end - pool_start
    if ram_used < 0 or pool_used < 0:
        raise ValueError("invalid linker memory ranges")
    return {
        "xip": {"used": image_size, "capacity": XIP_CAPACITY, "headroom": XIP_CAPACITY - image_size},
        "ram": {"used": ram_used, "capacity": RAM_CAPACITY, "headroom": RAM_CAPACITY - ram_used},
        "pool": {"used": pool_used, "capacity": POOL_CAPACITY, "headroom": POOL_CAPACITY - pool_used},
    }


def violations(report: dict, min_xip: int, min_ram: int, min_pool: int) -> list[str]:
    limits = {"xip": min_xip, "ram": min_ram, "pool": min_pool}
    errors: list[str] = []
    for region, minimum in limits.items():
        data = report[region]
        if data["used"] > data["capacity"]:
            errors.append(f"{region.upper()} overflow: {data['used']} > {data['capacity']} bytes")
        elif data["headroom"] < minimum:
            errors.append(
                f"{region.upper()} headroom {data['headroom']} B < required {minimum} B"
            )
    return errors


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("image", type=Path, help="built FM-1 application binary")
    ap.add_argument("symbols", type=Path, help="objdump -t symbol table")
    ap.add_argument("--output", type=Path, help="write JSON report")
    ap.add_argument("--min-xip-headroom", type=parse_int, default=DEFAULT_MIN_XIP_HEADROOM)
    ap.add_argument("--min-ram-headroom", type=parse_int, default=DEFAULT_MIN_RAM_HEADROOM)
    ap.add_argument("--min-pool-headroom", type=parse_int, default=DEFAULT_MIN_POOL_HEADROOM)
    args = ap.parse_args()

    report = inspect(args.image.stat().st_size, args.symbols.read_text())
    report["minimum_headroom"] = {
        "xip": args.min_xip_headroom,
        "ram": args.min_ram_headroom,
        "pool": args.min_pool_headroom,
    }
    errors = violations(report, args.min_xip_headroom, args.min_ram_headroom, args.min_pool_headroom)
    report["pass"] = not errors
    report["violations"] = errors

    for region in ("xip", "ram", "pool"):
        data = report[region]
        pct = (data["used"] * 100.0 / data["capacity"]) if data["capacity"] else 0.0
        print(
            f"{region.upper():4} used {data['used']:6} / {data['capacity']:6} B "
            f"({pct:5.1f}%), headroom {data['headroom']:6} B"
        )
    for error in errors:
        print(f"FAIL {error}")

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
