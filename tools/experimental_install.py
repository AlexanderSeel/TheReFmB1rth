#!/usr/bin/env python3
"""Guarded wrapper around Felucca's proven FM-1 installer.

Default behavior is dry-run only. Execution requires all safety gates, the
pinned upstream installer, and an additional --execute acknowledgement.
"""
from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

import hardware_gate

EXECUTE_TEXT = "I ACCEPT THE EXPERIMENTAL FLASH RISK"
UPSTREAM_INSTALLER = pathlib.Path(".upstream/felucca/tools/fm1_install.py")


def main() -> int:
    p=argparse.ArgumentParser(description="Guarded experimental FM-1 install wrapper")
    p.add_argument("package",type=pathlib.Path)
    p.add_argument("--sha256",required=True)
    p.add_argument("--device-identity",required=True)
    p.add_argument("--port")
    p.add_argument("--experimental",action="store_true")
    p.add_argument("--confirm",default="")
    p.add_argument("--execute",action="store_true")
    p.add_argument("--execute-confirm",default="")
    args=p.parse_args()
    try: raw=args.package.read_bytes()
    except OSError as exc: print(f"ERROR: {exc}",file=sys.stderr); return 2
    errors=hardware_gate.validate_gate(raw,args.sha256,args.device_identity,args.experimental,args.confirm)
    if errors:
        for e in errors: print(f"BLOCKED: {e}",file=sys.stderr)
        return 6
    print("Package/device safety gate passed.")
    if not args.execute:
        print("DRY RUN: no firmware was written. Add --execute only after recovery preparation.")
        return 0
    if args.execute_confirm != EXECUTE_TEXT:
        print(f"BLOCKED: --execute-confirm must equal {EXECUTE_TEXT!r}",file=sys.stderr)
        return 6
    if not UPSTREAM_INSTALLER.is_file():
        print("BLOCKED: pinned Felucca installer is missing; run tools/bootstrap_felucca.sh first",file=sys.stderr)
        return 6
    cmd=[sys.executable,str(UPSTREAM_INSTALLER),str(args.package),"--yes"]
    if args.port: cmd += ["--port",args.port]
    print("Executing pinned Felucca loader-mediated installer; do not disconnect power/USB.")
    return subprocess.call(cmd)

if __name__=="__main__": raise SystemExit(main())
