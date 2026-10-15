# SPDX-License-Identifier: GPL-3.0-only
import math
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import acid303_wavetable_common as common
import generate_303_wavetables as target


class WavetableGeneratorTests(unittest.TestCase):
    def test_target_uses_full_open303_reference_prototype(self):
        self.assertEqual(common.REFERENCE_SIZE, 2048)
        self.assertEqual(target.SIZE, 1024)
        self.assertGreater(target.SIZE, 512)

    def test_square_keeps_open303_body_and_saw_relative_fundamental(self):
        saw, square = common.open303_prototypes()
        saw_spec = common.positive_spectrum(saw, 4)
        square_spec = common.positive_spectrum(square, 4)
        saw_fund = 2.0 * abs(saw_spec[1]) * target.SAW_SCALE
        square_fund = 2.0 * abs(square_spec[1]) * target.SQUARE_SCALE
        # Open303 halves the square path; the resulting fundamentals should be
        # nearly level matched rather than a thin low-level pulse.
        self.assertGreater(square_fund / saw_fund, 0.92)
        self.assertLess(square_fund / saw_fund, 1.08)
        # The shaped 303 square deliberately retains a strong third harmonic
        # and a small even component; it is not an ideal/metallic odd-only FFT.
        self.assertGreater(abs(square_spec[3]), abs(square_spec[1]) * 0.25)
        self.assertLess(abs(square_spec[2]), abs(square_spec[1]) * 0.15)

    def test_pitch_bands_cover_full_midi_and_have_guard_band(self):
        uppers = target.upper_notes()
        self.assertEqual(len(uppers), len(target.BAND_CENTERS))
        self.assertEqual(uppers[-1], 127)
        budgets = [common.harmonic_budget(n, target.SIZE, target.BANDLIMIT_HZ) for n in uppers]
        self.assertTrue(all(a >= b for a, b in zip(budgets, budgets[1:])))
        for note, h in zip(uppers, budgets):
            self.assertLessEqual(h * common.midi_hz(note), target.BANDLIMIT_HZ + 1e-6)
        self.assertLess(target.BANDLIMIT_HZ, common.OUT_SR * 0.5)

    def test_every_band_transition_has_two_semitone_crossfade(self):
        notes = target.threshold_notes()
        self.assertEqual(len(notes), len(target.BAND_CENTERS) - 1)
        for note in notes:
            start = common.phase_inc(note - target.CROSSFADE_SEMITONES)
            end = common.phase_inc(note)
            self.assertLess(start, end)
            ratio = end / start
            expected = 2.0 ** (target.CROSSFADE_SEMITONES / 12.0)
            self.assertAlmostEqual(ratio, expected, places=5)

    def test_generated_bank_declares_crossfade_metadata(self):
        text = target.generate()
        self.assertIn("#define REFM_ACID_WT_BANDS 12u", text)
        self.assertIn("#define REFM_ACID_WT_SIZE 1024u", text)
        self.assertIn("refm_acid_wt_xfade_start_inc", text)
        self.assertIn("refm_acid_wt_harmonics", text)


if __name__ == "__main__":
    unittest.main()
