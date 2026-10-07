import importlib.util
import struct
import tempfile
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("preprocess_samples", ROOT / "tools" / "preprocess_samples.py")
mod = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(mod)


class SamplePreprocessTests(unittest.TestCase):
    def test_read_downmix_trim_resample_normalize(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "stereo.wav"
            frames = [(0, 0)] * 20 + [(32000, 16000), (24000, 8000), (-32000, -16000)] * 20 + [(0, 0)] * 20
            with wave.open(str(p), "wb") as w:
                w.setnchannels(2)
                w.setsampwidth(2)
                w.setframerate(44100)
                raw = b"".join(struct.pack("<hh", l, r) for l, r in frames)
                w.writeframes(raw)
            rate, pcm = mod.read_wav(p)
            self.assertEqual(rate, 44100)
            self.assertEqual(len(pcm), len(frames))
            trimmed = mod.trim(pcm)
            self.assertLessEqual(len(trimmed), len(pcm))
            down = mod.resample_linear(trimmed, 44100, 22050)
            self.assertGreater(len(down), 1)
            self.assertLessEqual(len(down), len(trimmed))
            norm = mod.normalize([32767, -32768, 1000])
            self.assertLessEqual(max(abs(x) for x in norm), 30000)

    def test_role_mapping_is_stable(self):
        self.assertEqual(mod.ROLE_TO_VOICE["kick"], 0)
        self.assertEqual(mod.ROLE_TO_VOICE["closed_hat"], 4)
        self.assertEqual(mod.ROLE_TO_VOICE["ride"], 10)
        self.assertEqual(len(set(mod.ROLE_TO_VOICE.values())), len(mod.ROLE_TO_VOICE))

    def test_selection_filters_and_rejects_missing_entries(self):
        entries = [
            {"kit": "808", "role": "kick"},
            {"kit": "909", "role": "closed_hat"},
            {"kit": "909", "role": "ride"},
        ]
        selected = mod.select_entries(entries, {"candidates": [
            {"kit": "909", "role": "ride"},
            {"kit": "909", "role": "closed_hat"},
        ]})
        self.assertEqual([(e["kit"], e["role"]) for e in selected], [("909", "closed_hat"), ("909", "ride")])
        with self.assertRaises(ValueError):
            mod.select_entries(entries, {"candidates": [{"kit": "909", "role": "crash"}]})


if __name__ == "__main__":
    unittest.main()
