// SPDX-License-Identifier: GPL-3.0-only
/*
 * TB-303-inspired fixed-point voice for the FM-1.
 *
 * The four-stage filter topology and coefficient shape are a fixed-point
 * adaptation informed by Robin Schmidt's Open303 TeeBeeFilter (MIT, 2009).
 * See docs/THIRD_PARTY_DSP.md for attribution and the retained MIT notice.
 */
#include "acid303.h"
#include <string.h>

#define SR 44100u
#define OS_SR 88200u
#define Q15_ONE 32767
#define Q20_ONE 1048576ll

static int32_t clamp32(int64_t x, int32_t lo, int32_t hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return (int32_t)x;
}

static const uint32_t semitone_inc[12] = {
    159321, 168796, 178834, 189468, 200734, 212670,
    225316, 238714, 252908, 267948, 283881, 300760
};

uint32_t acid303_note_to_phase_inc(uint8_t note) {
    uint32_t octave = note / 12u;
    uint32_t inc = semitone_inc[note % 12u];
    if (octave > 0u) {
        if (octave >= 8u) octave = 7u;
        inc <<= octave;
    }
    return inc;
}

uint32_t acid303_apply_tune(uint32_t phase_inc, int8_t tune) {
    /* The hardware tune pot is continuous. This fixed-point approximation gives
       roughly +/-70 cents over the UI range without libm/pow on the FM-1. */
    int64_t delta = ((int64_t)phase_inc * (int32_t)tune) / 1536;
    int64_t tuned = (int64_t)phase_inc + delta;
    if (tuned < 1) tuned = 1;
    if (tuned > 0xffffffffll) tuned = 0xffffffffll;
    return (uint32_t)tuned;
}

static int32_t shape_q15(int32_t x) {
    x = clamp32(x, -46340, 46340);
    int64_t x2 = ((int64_t)x * x) >> 15;
    int64_t x3 = (x2 * x) >> 15;
    return clamp32((int64_t)x - x3 / 6, -49152, 49152);
}

static uint32_t decay_tau_samples(uint16_t decay, uint8_t accented) {
    if (accented) return (SR * 200u) / 1000u;
    uint32_t ms = 200u + ((uint32_t)decay * 1800u) / 32767u;
    return (SR * ms) / 1000u;
}

static void decay_env(int32_t *env, uint32_t tau_samples) {
    if (*env <= 0) { *env = 0; return; }
    int32_t step = *env / (int32_t)(tau_samples ? tau_samples : 1u);
    if (step < 1) step = 1;
    *env -= step;
    if (*env < 8) *env = 0;
}

uint16_t acid303_cutoff_hz(const acid303_t *s) {
    uint32_t n = s->cutoff > 32767u ? 32767u : s->cutoff;
    uint32_t curved = (n * n) >> 15;
    return (uint16_t)(250u + (curved * 2150u) / 32767u);
}

static int32_t resonance_skew_q15(uint16_t resonance) {
    static const uint16_t lut[16] = {
        0,6252,11370,15564,19005,21797,24097,25981,
        27521,28782,29815,30663,31355,31921,32387,32767
    };
    uint32_t r = resonance > 32767u ? 32767u : resonance;
    uint32_t pos = r * 15u;
    uint32_t i = pos >> 15;
    uint32_t frac = pos & 32767u;
    if (i >= 15u) return 32767;
    return (int32_t)lut[i] + (int32_t)(((int64_t)((int32_t)lut[i+1] - lut[i]) * frac) >> 15);
}

/* Open303's TB coefficient shape evaluated for the 2x internal sample rate. */
static int32_t tb_b0_q15(uint32_t hz) {
    if (hz < 80u) hz = 80u;
    if (hz > 12000u) hz = 12000u;
    /* OS_SR * sqrt(2) ~= 124734. */
    int64_t fx = ((int64_t)hz << 20) / 124734ll;
    int64_t fx2 = (fx * fx) >> 20;
    int64_t num = 477ll + ((6493013ll * fx) >> 20);
    int64_t den = Q20_ONE + ((12958674ll * fx) >> 20) + ((4630128ll * fx2) >> 20);
    if (den <= 0) return 1;
    return clamp32((num << 15) / den, 1, 16384);
}

static int32_t tb_kbase_q12(uint32_t hz) {
    if (hz < 80u) hz = 80u;
    if (hz > 12000u) hz = 12000u;
    int64_t x = ((int64_t)hz << 20) / 124734ll;
    int64_t y = 4096ll;
    y = 29485874ll + ((y * x) >> 20);
    y = -23911595ll + ((y * x) >> 20);
    y = -1951634ll + ((y * x) >> 20);
    y = 2518860ll + ((y * x) >> 20);
    y = 876017ll + ((y * x) >> 20);
    y = 69627ll + ((y * x) >> 20);
    return clamp32(y, 4096, 131072);
}

void acid303_init(acid303_t *s) {
    memset(s, 0, sizeof(*s));
    s->cutoff = 10500u;
    s->resonance = 19000u;
    s->env_mod = 23500u;
    s->decay = 12500u;
    s->accent = 21000u;
    s->drive = 0u;
    s->tune = 0;
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
    uint32_t inc = acid303_apply_tune(acid303_note_to_phase_inc(note), s->tune);
    if (slide && s->gate) {
        s->slide_target_inc = inc;
        s->sliding = 1u;
    } else {
        s->phase_inc = inc;
        s->slide_target_inc = inc;
        s->sliding = 0u;
        s->env = Q15_ONE;
        s->amp = Q15_ONE;
        s->amp_stage = ACID_ENV_DECAY;
        if (accent) {
            s->accent_env = Q15_ONE;
            s->accent_sweep += (Q15_ONE - s->accent_sweep) / 2;
        }
    }
    s->gate = 1u;
    s->accented = accent ? 1u : 0u;
}

void acid303_note_off(acid303_t *s) {
    s->gate = 0u;
    s->amp_stage = ACID_ENV_RELEASE;
}

int16_t acid303_lfo_value(const acid303_t *s) {
    uint32_t q = s->lfo_phase >> 16;
    int32_t saw = (int32_t)q - 32768;
    int32_t tri = q < 32768u ? (int32_t)q * 2 - 32768 : 98302 - (int32_t)q * 2;
    switch ((acid_lfo_shape_t)(s->lfo_shape & 3u)) {
    case ACID_LFO_TRI: return (int16_t)tri;
    case ACID_LFO_SAW: return (int16_t)saw;
    case ACID_LFO_SQUARE: return q < 32768u ? 32767 : -32768;
    default: {
        int32_t a = tri < 0 ? -tri : tri;
        int32_t y = (int32_t)(((int64_t)tri * (49152 - a / 2)) >> 15);
        return (int16_t)clamp32(y, -32768, 32767);
    }
    }
}

/* Fixed-point PolyBLEP correction in Q15. phase and dt are Q32 cycles.
   This removes most of the hard-discontinuity aliasing without tables/libm. */
static int32_t poly_blep_q15(uint32_t phase, uint32_t dt) {
    if (!dt) return 0;
    if (phase < dt) {
        int32_t x = (int32_t)(((uint64_t)phase << 15) / dt);
        /* 2x - x^2 - 1 */
        return (x << 1) - (int32_t)(((int64_t)x * x) >> 15) - 32768;
    }
    if (phase > 0xffffffffu - dt) {
        uint32_t remain = 0xffffffffu - phase;
        int32_t x = -(int32_t)(((uint64_t)remain << 15) / dt);
        /* x^2 + 2x + 1, x in [-1,0] */
        return (int32_t)(((int64_t)x * x) >> 15) + (x << 1) + 32768;
    }
    return 0;
}

static int32_t oscillator_substep(acid303_t *s, uint32_t sub_inc) {
    const uint32_t duty = 0xA6666666u; /* intentionally asymmetric ~65% */
    s->phase += sub_inc;
    int32_t blep_wrap = poly_blep_q15(s->phase, sub_inc);
    if (!s->square) {
        int32_t saw = (int32_t)(s->phase >> 16) - 32768;
        return clamp32((int64_t)saw - blep_wrap, -32768, 32767);
    }

    int32_t sq = s->phase < duty ? 25000 : -32000;
    /* Smooth both the cycle wrap and the falling duty-cycle edge. */
    sq += (int32_t)(((int64_t)blep_wrap * 57000) >> 16);
    uint32_t rel = s->phase - duty;
    int32_t blep_duty = poly_blep_q15(rel, sub_inc);
    sq -= (int32_t)(((int64_t)blep_duty * 57000) >> 16);
    return clamp32(sq, -32768, 32767);
}

static void slide_step(acid303_t *s) {
    if (!s->sliding) return;
    int64_t d = (int64_t)s->slide_target_inc - (int64_t)s->phase_inc;
    int64_t step = d / 768;
    if (!step && d) step = d > 0 ? 1 : -1;
    s->phase_inc = (uint32_t)((int64_t)s->phase_inc + step);
    if (d < 3 && d > -3) { s->phase_inc = s->slide_target_inc; s->sliding = 0u; }
}

static void envelope_step(acid303_t *s) {
    decay_env(&s->env, decay_tau_samples(s->decay, s->accented));
    decay_env(&s->accent_env, (SR * 200u) / 1000u);
    decay_env(&s->amp, s->gate ? (SR * 3500u) / 1000u : (SR * 60u) / 1000u);
    if (!s->amp) s->amp_stage = ACID_ENV_OFF;
    uint32_t base = 7000u + ((uint32_t)s->resonance * 22000u) / 32767u;
    decay_env(&s->accent_sweep, base);
}

static int32_t feedback_highpass(acid303_t *s, int32_t x) {
    int32_t d = x - s->resonance_hp_lp;
    /* ~150 Hz at the 88.2 kHz internal ladder rate. */
    s->resonance_hp_lp += (int32_t)(((int64_t)d * 346) >> 15);
    return x - s->resonance_hp_lp;
}

static int32_t teebee_ladder_substep(acid303_t *s, int32_t in, uint32_t cutoff_hz) {
    int32_t b0 = tb_b0_q15(cutoff_hz);
    int32_t r = resonance_skew_q15(s->resonance);
    int32_t kbase = tb_kbase_q12(cutoff_hz);
    int32_t k_q15 = (int32_t)(((int64_t)kbase * r) >> 12);
    int32_t fb_shape = shape_q15(s->lp4);
    int32_t fb = clamp32(((int64_t)fb_shape * k_q15) >> 15, -262144, 262144);
    int32_t y0 = clamp32((int64_t)in - feedback_highpass(s, fb), -196608, 196608);
    int32_t d1 = clamp32((int64_t)y0 - s->lp1 + s->lp2, -262144, 262144);
    s->lp1 = clamp32((int64_t)s->lp1 + (((int64_t)2 * b0 * d1) >> 15), -262144, 262144);
    int32_t d2 = clamp32((int64_t)s->lp1 - 2ll * s->lp2 + s->lp3, -262144, 262144);
    s->lp2 = clamp32((int64_t)s->lp2 + (((int64_t)b0 * d2) >> 15), -262144, 262144);
    int32_t d3 = clamp32((int64_t)s->lp2 - 2ll * s->lp3 + s->lp4, -262144, 262144);
    s->lp3 = clamp32((int64_t)s->lp3 + (((int64_t)b0 * d3) >> 15), -262144, 262144);
    int32_t d4 = clamp32((int64_t)s->lp3 - 2ll * s->lp4, -262144, 262144);
    s->lp4 = clamp32((int64_t)s->lp4 + (((int64_t)b0 * d4) >> 15), -262144, 262144);
    int32_t gbase_q15 = (int32_t)(((int64_t)kbase * 32768) / (17ll * 4096ll));
    int32_t g_q15 = 32768 + (int32_t)(((int64_t)(gbase_q15 - 32768) * r) >> 15);
    g_q15 = (int32_t)(((int64_t)g_q15 * (32768 + r)) >> 15);
    return clamp32(((int64_t)2 * g_q15 * s->lp4) >> 15, -131072, 131072);
}

static int32_t output_highpass(acid303_t *s, int32_t x) {
    int32_t y = x - s->output_hp_x + (int32_t)(((int64_t)s->output_hp_y * 32675) >> 15);
    s->output_hp_x = x;
    s->output_hp_y = y;
    return y;
}

static uint32_t modulated_cutoff(acid303_t *s) {
    uint32_t fc = acid303_cutoff_hz(s);
    fc += (uint32_t)(((int64_t)s->env * s->env_mod * 6200) >> 30);
    if (s->accented || s->accent_sweep > 0) {
        int32_t sweep = (int32_t)(((int64_t)s->accent_sweep * s->accent) >> 15);
        /* Accent/filter interaction increases with resonance, but is intentionally
           bounded to avoid the exaggerated modern-synth 'laser' sweep. */
        sweep = (int32_t)(((int64_t)sweep * (7000 + (s->resonance * 3u) / 4u)) >> 15);
        if (sweep > 0) fc += (uint32_t)(((int64_t)sweep * 4700) >> 15);
    }
    if (s->lfo_amount) {
        int16_t lfo = acid303_lfo_value(s);
        int32_t delta = (int32_t)(((int64_t)lfo * s->lfo_amount * 1600) >> 22);
        int32_t m = (int32_t)fc + delta;
        fc = (uint32_t)(m < 80 ? 80 : m > 12000 ? 12000 : m);
    }
    if (fc > 12000u) fc = 12000u;
    return fc;
}

int32_t acid303_process(acid303_t *s) {
    slide_step(s);
    if (s->lfo_amount) s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u;
    envelope_step(s);

    uint32_t fc = modulated_cutoff(s);
    uint32_t sub_inc = s->phase_inc >> 1;
    if (!sub_inc && s->phase_inc) sub_inc = 1u;

    /* 2x oscillator/filter processing dramatically reduces harsh high-frequency
       aliases and improves stability/shape near strong resonance. The two
       internal samples are averaged before the VCA/output stage. */
    int32_t y0 = teebee_ladder_substep(s, oscillator_substep(s, sub_inc), fc);
    int32_t y1 = teebee_ladder_substep(s, oscillator_substep(s, sub_inc), fc);
    int32_t y = (y0 + y1) / 2;

    int32_t gain = s->amp;
    if (s->accented || s->accent_env > 0) {
        int32_t ag = (int32_t)(((int64_t)s->accent_env * s->accent) >> 16);
        gain = clamp32((int64_t)gain + ag, 0, 41000);
    }
    y = (int32_t)(((int64_t)y * gain) >> 15);
    if (s->drive) {
        int32_t k = 32768 + s->drive;
        y = shape_q15((int32_t)(((int64_t)y * k) >> 15));
    }
    y = output_highpass(s, y);
    return clamp32(y, -32768, 32767);
}
