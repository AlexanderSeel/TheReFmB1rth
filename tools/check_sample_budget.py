#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Select processed sample candidates and enforce a conservative target PCM budget."""
from __future__ import annotations
import argparse, json
from pathlib import Path


def select(processed: dict, policy: dict) -> dict:
    wanted = {(str(x["kit"]), str(x["role"])) for x in policy.get("candidates", [])}
    assets = [a for a in processed.get("assets", []) if (str(a.get("kit")), str(a.get("role"))) in wanted]
    found = {(str(a["kit"]), str(a["role"])) for a in assets}
    missing = sorted(wanted - found)
    total = sum(int(a.get("bytes", 0)) for a in assets)
    limit = int(policy.get("max_pcm_bytes", 0))
    return {
        "version": 1,
        "target_rate": processed.get("target_rate"),
        "candidate_count": len(assets),
        "candidate_pcm_bytes": total,
        "max_pcm_bytes": limit,
        "headroom": limit - total,
        "missing": [{"kit": k, "role": r} for k, r in missing],
        "assets": assets,
        "pass": not missing and total <= limit,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("processed", type=Path)
    ap.add_argument("--policy", type=Path, default=Path("samples/target-candidates.json"))
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()
    report = select(json.loads(args.processed.read_text()), json.loads(args.policy.read_text()))
    print(f"FM-1 sample candidates: {report['candidate_count']} assets, {report['candidate_pcm_bytes']} / {report['max_pcm_bytes']} PCM bytes")
    if report["missing"]:
        print("FAIL missing candidates:", report["missing"])
    if report["candidate_pcm_bytes"] > report["max_pcm_bytes"]:
        print(f"FAIL sample budget exceeded by {-report['headroom']} bytes")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
