#!/usr/bin/env python3
"""Fetch only manifest-approved drum samples and verify pinned Git blob identity.

The source manifest intentionally stores the upstream commit/path plus Git blob SHA-1.
After download this tool records the stronger SHA-256 in build/samples.lock.json.
A release may only embed samples from a checked-in/approved lock generated from the
manifest; arbitrary URLs are not accepted here.
"""
from __future__ import annotations
import argparse, hashlib, json, pathlib, sys, urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "samples" / "sources.json"
DEFAULT_LOCK = ROOT / "build" / "samples.lock.json"


def git_blob_sha1(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def raw_url(entry: dict) -> str:
    repo = entry["repository"]
    commit = entry["commit"]
    path = entry["path"]
    return f"https://raw.githubusercontent.com/{repo}/{commit}/{path}"


def validate_entry(entry: dict) -> None:
    required = {"kit", "role", "license", "repository", "commit", "path", "git_blob_sha1", "destination"}
    missing = required - entry.keys()
    if missing:
        raise ValueError(f"sample entry missing {sorted(missing)}")
    if entry["license"] not in {"CC0-1.0", "Public-Domain", "Unlicense"}:
        raise ValueError(f"unsupported redistribution license for {entry['kit']}:{entry['role']}: {entry['license']}")
    dest = pathlib.PurePosixPath(entry["destination"])
    if dest.is_absolute() or ".." in dest.parts or not str(dest).startswith("build/samples/"):
        raise ValueError(f"unsafe sample destination: {dest}")


def fetch(entry: dict, timeout: int) -> dict:
    validate_entry(entry)
    url = raw_url(entry)
    with urllib.request.urlopen(url, timeout=timeout) as response:
        data = response.read()
    actual_blob = git_blob_sha1(data)
    if actual_blob != entry["git_blob_sha1"]:
        raise RuntimeError(
            f"blob mismatch for {entry['kit']}:{entry['role']}: expected {entry['git_blob_sha1']} got {actual_blob}"
        )
    destination = ROOT / pathlib.PurePosixPath(entry["destination"])
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    return {
        "kit": entry["kit"],
        "role": entry["role"],
        "license": entry["license"],
        "repository": entry["repository"],
        "commit": entry["commit"],
        "path": entry["path"],
        "git_blob_sha1": actual_blob,
        "sha256": hashlib.sha256(data).hexdigest(),
        "bytes": len(data),
        "destination": entry["destination"],
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", type=pathlib.Path, default=DEFAULT_MANIFEST)
    ap.add_argument("--lock", type=pathlib.Path, default=DEFAULT_LOCK)
    ap.add_argument("--kit", choices=["808", "909"])
    ap.add_argument("--timeout", type=int, default=30)
    ap.add_argument("--validate-only", action="store_true")
    args = ap.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    entries = manifest.get("sources", [])
    if not entries:
        raise SystemExit("sample manifest has no sources")
    for entry in entries:
        validate_entry(entry)
    if args.validate_only:
        print(f"sample manifest ok: {len(entries)} approved entries")
        return 0

    selected = [e for e in entries if not args.kit or e["kit"] == args.kit]
    locked = []
    for entry in selected:
        print(f"fetch {entry['kit']}:{entry['role']} from {entry['repository']}@{entry['commit']}")
        locked.append(fetch(entry, args.timeout))

    args.lock.parent.mkdir(parents=True, exist_ok=True)
    lock = {"version": 1, "manifest": str(args.manifest.relative_to(ROOT)), "samples": locked}
    args.lock.write_text(json.dumps(lock, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {args.lock} with SHA-256 for {len(locked)} samples")
    return 0


if __name__ == "__main__":
    sys.exit(main())
