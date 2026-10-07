import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import check_sample_budget as sb


class SampleBudgetTests(unittest.TestCase):
    def test_selects_only_requested_assets(self):
        processed = {"target_rate": 22050, "assets": [
            {"kit": "909", "role": "closed_hat", "bytes": 1000},
            {"kit": "909", "role": "ride", "bytes": 2000},
            {"kit": "808", "role": "kick", "bytes": 9000},
        ]}
        policy = {"max_pcm_bytes": 4000, "candidates": [
            {"kit": "909", "role": "closed_hat"},
            {"kit": "909", "role": "ride"},
        ]}
        r = sb.select(processed, policy)
        self.assertTrue(r["pass"])
        self.assertEqual(r["candidate_pcm_bytes"], 3000)
        self.assertEqual(r["candidate_count"], 2)

    def test_missing_candidate_fails(self):
        r = sb.select({"assets": []}, {"max_pcm_bytes": 1000, "candidates": [{"kit": "909", "role": "ride"}]})
        self.assertFalse(r["pass"])
        self.assertEqual(len(r["missing"]), 1)

    def test_budget_overflow_fails(self):
        processed = {"assets": [{"kit": "909", "role": "ride", "bytes": 2001}]}
        policy = {"max_pcm_bytes": 2000, "candidates": [{"kit": "909", "role": "ride"}]}
        r = sb.select(processed, policy)
        self.assertFalse(r["pass"])
        self.assertEqual(r["headroom"], -1)


if __name__ == "__main__":
    unittest.main()
