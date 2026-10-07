// SPDX-License-Identifier: GPL-3.0-only
#include "acid303.h"
#include <string.h>

#define SR 44100u
#define Q15_ONE 32767

static int32_t clamp32(int64_t x, int32_t lo, int32_t hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return (int32_t)x;
}

/* Equal-tempered note increments without libm on target. Table is C0..B0 in Q32 phase/sample. */
static const uint32_t semitone_inc[12] = {
    159321, 168796, 178834, 189468, 200734, 212670,
    225316, 238714, 252908, 267948, 283881, 300760
};

uint32_t acid303_note_to_phase_inc(uint8_t note) {
    uint32_t octave = note / 12u;
    uint32_t inc = semitone_inc[note % 12u];
    if (octave > 0) {
        if (octave >= 8) octave = 7;
        inc <<= octave;
    }
    return inc;
}

void acid303_init(acid303_t *s) {
    memset(s, 0, sizeof(*s));
    s->cutoff = 12000;
    s->resonance = 15000;
    s->env_mod = 18000;
    s->decay = 900;
    s->accent = 18000;
    s->drive = 5000;
    s->amp = 0;
}

void acid303_set_note(acid303_t *s, uint8_t note, uint8_t accent, uint8_t slide) {
    uint32_t inc = acid303_note_to_phase_inc(note);
    if (slide && s->gate) {
        s->slide_target_inc = inc;
        s->sliding = 1;
    } else {
        s->phase_inc = inc;
        s->slide_target_inc = inc;
        s->sliding = 0;
        s->env = Q15_ONE;
        s->amp = Q15_ONE;
    }
    s->gate = 1;
    s->accented = accent ? 1u : 0u;
    if (accent) s->env = Q15_ONE;
}

void acid303_note_off(acid303_t *s) {
    s->gate = 0;
}

static int32_t oscillator(acid303_t *s) {
    s->phase += s->phase_inc;
    if (s->square) return (s->phase & 0x80000000u) ? Q15_ONE : -Q15_ONE;
    return (int32_t)(s->phase >> 16) - 32768;
}

int32_t acid303_process(acid303_t *s) {
    if (s->sliding) {
        int64_t d = (int64_t)s->slide_target_inc - (int64_t)s->phase_inc;
        s->phase_inc = (uint32_t)((int64_t)s->phase_inc + d / 256);
        if (d < 4 && d > -4) { s->phase_inc = s->slide_target_inc; s->sliding = 0; }
    }

    int32_t osc = oscillator(s);
    int32_t decay_step = 12 + (int32_t)s->decay / 32;
    if (s->env > 0) s->env = s->env > decay_step ? s->env - decay_step : 0;
    if (!s->gate && s->amp > 0) s->amp -= s->amp > 64 ? 64 : s->amp;

    int32_t accent_boost = s->accented ? (int32_t)s->accent / 3 : 0;
    int32_t fc = (int32_t)s->cutoff + (int32_t)(((int64_t)s->env * s->env_mod) >> 15) + accent_boost;
    fc = clamp32(fc, 256, 30000);

    /* Two-pole fixed-point prototype. This is deliberately conservative and stable;
       a transistor-ladder/diode-ladder approximation replaces it after profiling. */
    int32_t feedback = (int32_t)(((int64_t)s->lp2 * s->resonance) >> 15);
    int32_t x = osc - feedback;
    s->lp1 += (int32_t)(((int64_t)(x - s->lp1) * fc) >> 15);
    s->lp2 += (int32_t)(((int64_t)(s->lp1 - s->lp2) * fc) >> 15);

    int32_t gain = s->amp;
    if (s->accented) gain = clamp32(gain + s->accent / 2, 0, Q15_ONE);
    int32_t y = (int32_t)(((int64_t)s->lp2 * gain) >> 15);

    /* Cheap symmetric soft clipping; bounded for fixed-point target. */
    int32_t drive = 32768 + s->drive;
    y = (int32_t)(((int64_t)y * drive) >> 15);
    if (y > 28000) y = 28000 + (y - 28000) / 8;
    if (y < -28000) y = -28000 + (y + 28000) / 8;
    return clamp32(y, -32768, 32767);
}
