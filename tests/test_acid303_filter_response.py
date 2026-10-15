#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
ACID = ROOT / "firmware" / "proto" / "acid303.c"
WT = ROOT / "firmware" / "proto" / "acid303_wavetable.h"

HARNESS = r"""
#include <stdint.h>
#include <stdio.h>
#include "acid303.h"

typedef struct {
    uint64_t energy;
    uint64_t diff_energy;
    int32_t b0;
    int32_t k;
    int32_t g;
} metric_t;

static metric_t render_metric(uint16_t cutoff, uint16_t resonance) {
    acid303_t s;
    metric_t m = {0,0,0,0,0};
    int32_t prev;
    acid303_init(&s);
    s.square = 0u;
    s.cutoff = cutoff;
    s.resonance = resonance;
    s.env_mod = 0u;
    s.accent = 0u;
    s.drive = 0u;
    acid303_set_note(&s, 36u, 0u, 0u);
    for (unsigned i = 0; i < 512u; ++i) (void)acid303_process(&s);
    prev = acid303_process(&s);
    for (unsigned i = 0; i < 4096u; ++i) {
        int32_t y = acid303_process(&s);
        int32_t a = y < 0 ? -y : y;
        int32_t d = y - prev;
        if (d < 0) d = -d;
        m.energy += (uint32_t)a;
        m.diff_energy += (uint32_t)d;
        prev = y;
    }
    m.b0 = s.coeff_b0;
    m.k = s.coeff_k;
    m.g = s.coeff_g;
    return m;
}

int main(void) {
    acid303_t live;
    metric_t low = render_metric(0u, 0u);
    metric_t high = render_metric(32767u, 0u);
    metric_t res = render_metric(32767u, 28000u);

    acid303_init(&live);
    live.square = 0u;
    live.env_mod = 0u;
    live.resonance = 0u;
    live.cutoff = 0u;
    acid303_set_note(&live, 36u, 0u, 0u);
    for (unsigned i = 0; i < 96u; ++i) (void)acid303_process(&live);
    int32_t live_low_b0 = live.coeff_b0;
    live.cutoff = 32767u;
    for (unsigned i = 0; i < 96u; ++i) (void)acid303_process(&live);
    int32_t live_high_b0 = live.coeff_b0;

    printf("%llu %llu %d %llu %llu %d %llu %llu %d %d\n",
        (unsigned long long)low.energy,
        (unsigned long long)low.diff_energy,
        low.b0,
        (unsigned long long)high.energy,
        (unsigned long long)high.diff_energy,
        high.b0,
        (unsigned long long)res.energy,
        (unsigned long long)res.diff_energy,
        live_low_b0,
        live_high_b0);
    return 0;
}
"""

class Acid303FilterResponseTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not WT.exists():
            subprocess.run(
                ["python3", "tools/generate_303_wavetables.py"],
                cwd=ROOT, check=True, stdout=subprocess.PIPE, text=True
            )

    def test_target_hot_path_avoids_64_bit_integer_math(self):
        src = ACID.read_text(encoding="utf-8")
        self.assertNotIn("int64_t", src)
        self.assertNotIn("uint64_t", src)
        self.assertNotIn("long long", src)

    def test_cutoff_and_resonance_materially_change_rendered_audio(self):
        with tempfile.TemporaryDirectory() as td:
            td = pathlib.Path(td)
            harness = td / "probe.c"
            exe = td / "probe"
            harness.write_text(HARNESS, encoding="utf-8")
            subprocess.run([
                "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2",
                "-I", str(ROOT / "firmware" / "proto"),
                str(ACID), str(harness), "-o", str(exe)
            ], cwd=ROOT, check=True)
            fields = [int(x) for x in subprocess.check_output([str(exe)], text=True).split()]

        (low_e, low_d, low_b0, high_e, high_d, high_b0,
         res_e, res_d, live_low_b0, live_high_b0) = fields

        self.assertGreater(high_b0, low_b0 * 4)
        self.assertGreater(high_e * 100, low_e * 140)
        self.assertGreater(live_high_b0, live_low_b0 * 4)
        self.assertGreater(high_d * low_e * 100, low_d * high_e * 112)
        self.assertGreater(res_d * 100, high_d * 145)
        self.assertNotEqual(res_e, high_e)

if __name__ == "__main__":
    unittest.main()
