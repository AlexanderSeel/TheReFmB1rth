#!/usr/bin/env python3
"""Create or verify a deterministic manifest for an FM-1 .fwsc artifact."""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import sys

UPSTREAM_FELUCCA = "3dd2b0852bc310a2c1bf00c2d443ac19202c2140"
BLOCKS = 20
BLK = 0x30
KEEP = 0x2F


def product_of(raw: bytes) -> str:
    if len(raw) < BLOCKS * BLK:
        raise ValueError("package is too short")
    return "".join(chr((m-i-1)&0xFF) for i in range(BLOCKS) if (m:=raw[i*BLK+KEEP]) != 0x7D)


def make_manifest(package: pathlib.Path, local_commit: str) -> dict[str, object]:
    raw = package.read_bytes()
    product = product_of(raw)
    if not product.startswith("FM-1_") and not product.startswith("ota-FM-1_"):
        raise ValueError(f"unexpected product identity {product!r}")
    return {
        "schema": 1,
        "artifact": package.name,
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "product": product,
        "felucca_upstream": UPSTREAM_FELUCCA,
        "local_commit": local_commit,
        "install_protocol": "felucca-loader-mediated",
        "experimental": True,
    }


def verify(package: pathlib.Path, manifest: dict[str, object]) -> list[str]:
    errors: list[str] = []
    try:
        actual = make_manifest(package, str(manifest.get("local_commit", "")))
    except (OSError, ValueError) as exc:
        return [str(exc)]
    for key in ("schema","artifact","bytes","sha256","product","felucca_upstream","install_protocol","experimental"):
        if manifest.get(key) != actual.get(key):
            errors.append(f"manifest mismatch: {key}")
    return errors


def main() -> int:
    p=argparse.ArgumentParser()
    p.add_argument("package",type=pathlib.Path)
    p.add_argument("--commit",default="unknown")
    p.add_argument("--output",type=pathlib.Path)
    p.add_argument("--verify",type=pathlib.Path)
    args=p.parse_args()
    if args.verify:
        try: manifest=json.loads(args.verify.read_text(encoding="utf-8"))
        except (OSError,json.JSONDecodeError) as exc:
            print(f"ERROR: {exc}",file=sys.stderr); return 2
        errors=verify(args.package,manifest)
        if errors:
            for e in errors: print(f"ERROR: {e}",file=sys.stderr)
            return 6
        print("Manifest verified")
        return 0
    try: manifest=make_manifest(args.package,args.commit)
    except (OSError,ValueError) as exc:
        print(f"ERROR: {exc}",file=sys.stderr); return 2
    text=json.dumps(manifest,indent=2,sort_keys=True)+"\n"
    if args.output: args.output.write_text(text,encoding="utf-8")
    else: print(text,end="")
    return 0

if __name__=="__main__": raise SystemExit(main())
