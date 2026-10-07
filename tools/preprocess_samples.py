#!/usr/bin/env python3
"""Convert approved WAV assets into deterministic compact PCM16/C assets.

Input is samples/sources.json after tools/fetch_samples.py has populated build/samples.
The generated C unit is intended for WASM audition first; target firmware embedding is opt-in.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, wave
from pathlib import Path

ROLE_TO_VOICE = {
    "kick": 0, "snare": 1, "clap": 2, "rim": 3,
    "closed_hat": 4, "open_hat": 5,
    "low_tom": 6, "mid_tom": 7, "high_tom": 8,
    "crash": 9, "ride": 10,
}


def read_wav(path: Path):
    with wave.open(str(path), "rb") as w:
        channels, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        if width != 2:
            raise ValueError(f"{path}: only PCM16 WAV is supported, got {width * 8}-bit")
        if channels not in (1, 2):
            raise ValueError(f"{path}: only mono/stereo WAV is supported")
        raw = w.readframes(frames)
    vals = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    if channels == 2:
        mono = [int((vals[i] + vals[i + 1]) / 2) for i in range(0, len(vals), 2)]
    else:
        mono = list(vals)
    return rate, mono


def trim(samples, threshold=96):
    if not samples:
        return samples
    first = 0
    while first < len(samples) and abs(samples[first]) <= threshold:
        first += 1
    if first == len(samples):
        return [0]
    last = len(samples) - 1
    while last > first and abs(samples[last]) <= threshold:
        last -= 1
    pad = 48
    return samples[max(0, first - pad):min(len(samples), last + pad + 1)]


def resample_linear(samples, src_rate, dst_rate):
    if src_rate == dst_rate or len(samples) < 2:
        return samples[:]
    out_len = max(1, int(round(len(samples) * dst_rate / src_rate)))
    scale = src_rate / dst_rate
    out = []
    for i in range(out_len):
        pos = i * scale
        j = min(int(pos), len(samples) - 1)
        k = min(j + 1, len(samples) - 1)
        frac = pos - j
        out.append(int(round(samples[j] + (samples[k] - samples[j]) * frac)))
    return out


def normalize(samples, peak_target=30000):
    peak = max((abs(x) for x in samples), default=0)
    if peak == 0 or peak <= peak_target:
        return samples
    gain = peak_target / peak
    return [max(-32768, min(32767, int(round(x * gain)))) for x in samples]


def ident(kit, role):
    return f"refm_{kit}_{role}".replace("-", "_")


def select_entries(entries, selection):
    if selection is None:
        return list(entries)
    wanted = {(str(x["kit"]), str(x["role"])) for x in selection.get("candidates", [])}
    selected = [e for e in entries if (str(e.get("kit")), str(e.get("role"))) in wanted]
    found = {(str(e["kit"]), str(e["role"])) for e in selected}
    missing = sorted(wanted - found)
    if missing:
        raise ValueError(f"selection references missing manifest entries: {missing}")
    return selected


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", default="samples/sources.json")
    ap.add_argument("--selection", help="optional candidate JSON containing kit/role pairs")
    ap.add_argument("--rate", type=int, default=22050, choices=(11025, 22050, 44100))
    ap.add_argument("--out-dir", default="build/samples/processed")
    args = ap.parse_args()
    manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    selection = json.loads(Path(args.selection).read_text(encoding="utf-8")) if args.selection else None
    entries = select_entries(manifest["sources"], selection)
    out = Path(args.out_dir); out.mkdir(parents=True, exist_ok=True)
    assets = []
    arrays = []
    table = {"808": [None] * 11, "909": [None] * 11}
    for entry in entries:
        role = entry["role"]
        if role not in ROLE_TO_VOICE:
            continue
        src = Path(entry["destination"])
        if not src.exists():
            raise SystemExit(f"missing fetched sample: {src}; run tools/fetch_samples.py first")
        src_rate, pcm = read_wav(src)
        pcm = normalize(resample_linear(trim(pcm), src_rate, args.rate))
        raw = struct.pack("<" + "h" * len(pcm), *pcm)
        name = ident(entry["kit"], role)
        pcm_path = out / f"{entry['kit']}-{role}.pcm16le"
        pcm_path.write_bytes(raw)
        sha = hashlib.sha256(raw).hexdigest()
        arrays.append((name, pcm))
        table[entry["kit"]][ROLE_TO_VOICE[role]] = (name, len(pcm), args.rate)
        assets.append({
            "kit": entry["kit"], "role": role, "voice": ROLE_TO_VOICE[role],
            "source": str(src), "source_rate": src_rate, "target_rate": args.rate,
            "frames": len(pcm), "bytes": len(raw), "sha256": sha,
            "license": entry["license"], "repository": entry["repository"],
            "commit": entry["commit"], "path": entry["path"],
        })
    h = out / "refm_sample_assets.h"
    c = out / "refm_sample_assets.c"
    h.write_text("""// generated by tools/preprocess_samples.py\n#ifndef REFM_SAMPLE_ASSETS_H\n#define REFM_SAMPLE_ASSETS_H\n#include \"drum_machine.h\"\nvoid refm_attach_bundled_samples(drum_machine_t *d, drum_model_t model);\n#endif\n""", encoding="utf-8")
    lines = ["// generated by tools/preprocess_samples.py", '#include "refm_sample_assets.h"', ""]
    for name, pcm in arrays:
        lines.append(f"static const int16_t {name}[{len(pcm)}] = {{")
        for i in range(0, len(pcm), 16):
            lines.append("  " + ",".join(str(x) for x in pcm[i:i+16]) + ",")
        lines.append("};")
    lines.append("void refm_attach_bundled_samples(drum_machine_t *d, drum_model_t model) {")
    lines.append("  drum_machine_clear_samples(d);")
    lines.append("  if (model == DRUM_MODEL_808) {")
    for voice, data in enumerate(table["808"]):
        if data:
            name, frames, rate = data
            lines.append(f"    drum_machine_set_sample(d, {voice}u, {name}, {frames}u, {rate}u);")
    lines.append("  } else {")
    for voice, data in enumerate(table["909"]):
        if data:
            name, frames, rate = data
            lines.append(f"    drum_machine_set_sample(d, {voice}u, {name}, {frames}u, {rate}u);")
    lines.append("  }")
    lines.append("}")
    c.write_text("\n".join(lines) + "\n", encoding="utf-8")
    report = {
        "version": 1, "target_rate": args.rate,
        "selection": args.selection,
        "asset_count": len(assets), "total_pcm_bytes": sum(x["bytes"] for x in assets),
        "assets": assets,
    }
    (out / "manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"processed {len(assets)} samples, {report['total_pcm_bytes']} PCM bytes at {args.rate} Hz")

if __name__ == "__main__":
    main()
