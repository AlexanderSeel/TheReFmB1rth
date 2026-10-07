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

static int32_t env_coeff(uint8_t time) {
    uint32_t x = 128u - (uint32_t)(time > 127u ? 127u : time);
    uint32_t c = 1u + x * x * 2u;
    return c > 32767u ? 32767 : (int32_t)c;
}

static int32_t sustain_q15(const acid303_t *s) {
    return (int32_t)s->amp_sustain * 258;
}

static void amp_envelope(acid303_t *s) {
    int32_t target, d, step;
    switch ((acid_env_stage_t)s->amp_stage) {
    case ACID_ENV_ATTACK:
        if (!s->amp_attack) {
            s->amp = Q15_ONE;
            s->amp_stage = ACID_ENV_DECAY;
            break;
        }
        d = Q15_ONE - s->amp;
        step = (int32_t)(((int64_t)d * env_coeff(s->amp_attack)) >> 15);
        if (step < 1) step = 1;
        s->amp += step;
        if (s->amp >= Q15_ONE - 32) {
            s->amp = Q15_ONE;
            s->amp_stage = ACID_ENV_DECAY;
        }
        break;
    case ACID_ENV_DECAY:
        target = sustain_q15(s);
        if (!s->amp_decay) {
            s->amp = target;
            s->amp_stage = ACID_ENV_SUSTAIN;
            break;
        }
        d = s->amp - target;
        if (d <= 32) {
            s->amp = target;
            s->amp_stage = ACID_ENV_SUSTAIN;
            break;
        }
        step = (int32_t)(((int64_t)d * env_coeff(s->amp_decay)) >> 15);
        if (step < 1) step = 1;
        s->amp -= step;
        break;
    case ACID_ENV_SUSTAIN:
        s->amp = sustain_q15(s);
        break;
    case ACID_ENV_RELEASE:
        if (!s->amp_release) {
            s->amp = 0;
            s->amp_stage = ACID_ENV_OFF;
            break;
        }
        step = (int32_t)(((int64_t)s->amp * env_coeff(s->amp_release)) >> 15);
        if (step < 1) step = 1;
        s->amp -= step;
        if (s->amp <= 32) {
            s->amp = 0;
            s->amp_stage = ACID_ENV_OFF;
        }
        break;
    default:
        s->amp = 0;
        break;
    }
}

void acid303_init(acid303_t *s) {
    memset(s, 0, sizeof(*s));
    s->cutoff = 12000;
    s->resonance = 15000;
    s->env_mod = 18000;
    s->decay = 900;
    s->accent = 18000;
    s->drive = 5000;
    s->amp_attack = 0u;
    s->amp_decay = 0u;
    s->amp_sustain = 127u;
    s->amp_release = 110u;
    s->lfo_rate = 35u;
    s->lfo_amount = 0u;
    s->lfo_shape = ACID_LFO_SINE;
    s->amp_stage = ACID_ENV_OFF;
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
        s->amp = s->amp_attack ? 0 : Q15_ONE;
        s->amp_stage = s->amp_attack ? ACID_ENV_ATTACK : ACID_ENV_DECAY;
    }
    s->gate = 1;
    s->accented = accent ? 1u : 0u;
    if (accent) s->env = Q15_ONE;
}

void acid303_note_off(acid303_t *s) {
    s->gate = 0;
    if (s->amp > 0)
        s->amp_stage = ACID_ENV_RELEASE;
}

int16_t acid303_lfo_value(const acid303_t *s) {
    uint32_t q = s->lfo_phase >> 16;
    int32_t saw = (int32_t)q - 32768;
    int32_t tri = q < 32768u ? (int32_t)q * 2 - 32768 : 98302 - (int32_t)q * 2;
    switch ((acid_lfo_shape_t)(s->lfo_shape & 3u)) {
    case ACID_LFO_TRI:
        return (int16_t)tri;
    case ACID_LFO_SAW:
        return (int16_t)saw;
    case ACID_LFO_SQUARE:
        return q < 32768u ? 32767 : -32768;
    default: {
        /* Parabolic sine-like curve: smooth and target-cheap, no table/libm. */
        int32_t a = tri < 0 ? -tri : tri;
        int32_t y = (int32_t)(((int64_t)tri * (49152 - a / 2)) >> 15);
        return (int16_t)clamp32(y, -32768, 32767);
    }
    }
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

    s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u; /* ~0.05 .. 20 Hz at 44.1 kHz */
    int16_t lfo = acid303_lfo_value(s);
    int32_t osc = oscillator(s);
    int32_t decay_step = 12 + (int32_t)s->decay / 32;
    if (s->env > 0) s->env = s->env > decay_step ? s->env - decay_step : 0;
    amp_envelope(s);

    int32_t accent_boost = s->accented ? (int32_t)s->accent / 3 : 0;
    int32_t fc = (int32_t)s->cutoff + (int32_t)(((int64_t)s->env * s->env_mod) >> 15) + accent_boost;
    fc += ((int32_t)lfo * s->lfo_amount) >> 8; /* bipolar filter modulation */
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
