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
#define Q15_ONE 32767
#define Q20_ONE 1048576ll

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
    if (octave > 0u) {
        if (octave >= 8u) octave = 7u;
        inc <<= octave;
    }
    return inc;
}

/* Bounded cubic used in the feedback loop. Open303 likewise uses a clipped
   x-x^3/6 transfer for its TB-303 model. */
static int32_t shape_q15(int32_t x) {
    x = clamp32(x, -46340, 46340);
    int64_t x2 = ((int64_t)x * x) >> 15;
    int64_t x3 = (x2 * x) >> 15;
    return clamp32((int64_t)x - x3 / 6, -49152, 49152);
}

static uint32_t decay_tau_samples(uint16_t decay, uint8_t accented) {
    if (accented) return (SR * 200u) / 1000u;
    /* Measured/commonly-modelled control range is roughly 200 ms .. 2 s. */
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
    /* Base cutoff control: approximately 250 Hz .. 2.4 kHz. */
    uint32_t n = s->cutoff > 32767u ? 32767u : s->cutoff;
    uint32_t curved = (n * n) >> 15;
    return (uint16_t)(250u + (curved * 2150u) / 32767u);
}

/* Open303 resonance mapping: (1-exp(-3r))/(1-exp(-3)).
   A 16-point Q15 LUT avoids libm on the embedded target. */
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

/* TB-303 b0 approximation adapted from Open303's published TeeBeeFilter model.
   fx = cutoff / (sampleRate*sqrt(2)); result is Q15. */
static int32_t tb_b0_q15(uint32_t hz) {
    if (hz < 80u) hz = 80u;
    if (hz > 12000u) hz = 12000u;
    int64_t fx = ((int64_t)hz << 20) / 62367ll; /* 44100*sqrt(2) */
    int64_t fx2 = (fx * fx) >> 20;
    int64_t num = 477ll + ((6493013ll * fx) >> 20);
    int64_t den = Q20_ONE + ((12958674ll * fx) >> 20) + ((4630128ll * fx2) >> 20);
    if (den <= 0) return 1;
    return clamp32((num << 15) / den, 1, 16384);
}

/* Open303 TB-303 feedback polynomial, represented in Q12 and evaluated with
   fx in Q20. The raw result is roughly 17 at low cutoff and rises with cutoff. */
static int32_t tb_kbase_q12(uint32_t hz) {
    if (hz < 80u) hz = 80u;
    if (hz > 12000u) hz = 12000u;
    int64_t x = ((int64_t)hz << 20) / 62367ll;
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
    s->amp_attack = 0u;
    s->amp_decay = 0u;
    s->amp_sustain = 127u;
    s->amp_release = 110u;
    s->lfo_rate = 35u;
    s->lfo_amount = 0u; /* stock TB-303 mode: no LFO */
    s->lfo_shape = ACID_LFO_SINE;
    s->amp_stage = ACID_ENV_OFF;
}

void acid303_set_note(acid303_t *s, uint8_t note, uint8_t accent, uint8_t slide) {
    uint32_t inc = acid303_note_to_phase_inc(note);
    if (slide && s->gate) {
        /* Slide is legato: pitch slews but filter/VCA envelopes do not retrigger. */
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
            /* Repeated accents charge a persistent sweep state instead of fully resetting. */
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

static int32_t oscillator(acid303_t *s) {
    s->phase += s->phase_inc;
    int32_t saw = (int32_t)(s->phase >> 16) - 32768;
    if (!s->square) return saw;
    /* 303 square is generated from the VCO saw path and is not a mathematically
       ideal square. Slight asymmetry is intentional. */
    return (s->phase < 0xA6666666u) ? 25000 : -32000;
}

static void slide_step(acid303_t *s) {
    if (!s->sliding) return;
    int64_t d = (int64_t)s->slide_target_inc - (int64_t)s->phase_inc;
    /* ~88 ms RC-like portamento, close to commonly measured 303 glide. */
    int64_t step = d / 768;
    if (!step && d) step = d > 0 ? 1 : -1;
    s->phase_inc = (uint32_t)((int64_t)s->phase_inc + step);
    if (d < 3 && d > -3) { s->phase_inc = s->slide_target_inc; s->sliding = 0u; }
}

static void envelope_step(acid303_t *s) {
    decay_env(&s->env, decay_tau_samples(s->decay, s->accented));
    decay_env(&s->accent_env, (SR * 200u) / 1000u);

    /* The stock VCA contour is not a user ADSR: very fast attack and a long fall. */
    decay_env(&s->amp, s->gate ? (SR * 3500u) / 1000u : (SR * 60u) / 1000u);
    if (!s->amp) s->amp_stage = ACID_ENV_OFF;

    /* Resonance slows the accent capacitor discharge, producing the repeated
       accented-note 'wow' behavior central to the 303. */
    uint32_t base = 7000u + ((uint32_t)s->resonance * 22000u) / 32767u;
    decay_env(&s->accent_sweep, base);
}

static int32_t feedback_highpass(acid303_t *s, int32_t x) {
    /* Open303 uses 150 Hz in the TB-303 resonance path. Q15 one-pole coefficient
       here is ~2*pi*150/(Fs+2*pi*150). */
    int32_t d = x - s->resonance_hp_lp;
    s->resonance_hp_lp += (int32_t)(((int64_t)d * 685) >> 15);
    return x - s->resonance_hp_lp;
}

static int32_t teebee_ladder(acid303_t *s, int32_t in, uint32_t cutoff_hz) {
    int32_t b0 = tb_b0_q15(cutoff_hz);
    int32_t r = resonance_skew_q15(s->resonance);
    int32_t kbase = tb_kbase_q12(cutoff_hz);
    int32_t k_q15 = (int32_t)(((int64_t)kbase * r) >> 12);

    int32_t fb_shape = shape_q15(s->lp4);
    int32_t fb = clamp32(((int64_t)fb_shape * k_q15) >> 15, -262144, 262144);
    int32_t y0 = clamp32((int64_t)in - feedback_highpass(s, fb), -196608, 196608);

    /* Open303 TB_303 mode's four coupled stage update, adapted to Q15. */
    int32_t d1 = clamp32((int64_t)y0 - s->lp1 + s->lp2, -262144, 262144);
    s->lp1 = clamp32((int64_t)s->lp1 + (((int64_t)2 * b0 * d1) >> 15), -262144, 262144);
    int32_t d2 = clamp32((int64_t)s->lp1 - 2ll * s->lp2 + s->lp3, -262144, 262144);
    s->lp2 = clamp32((int64_t)s->lp2 + (((int64_t)b0 * d2) >> 15), -262144, 262144);
    int32_t d3 = clamp32((int64_t)s->lp2 - 2ll * s->lp3 + s->lp4, -262144, 262144);
    s->lp3 = clamp32((int64_t)s->lp3 + (((int64_t)b0 * d3) >> 15), -262144, 262144);
    int32_t d4 = clamp32((int64_t)s->lp3 - 2ll * s->lp4, -262144, 262144);
    s->lp4 = clamp32((int64_t)s->lp4 + (((int64_t)b0 * d4) >> 15), -262144, 262144);

    /* Open303 gain compensation: g=(kbase/17 blended by resonance)*(1+r). */
    int32_t gbase_q15 = (int32_t)(((int64_t)kbase * 32768) / (17ll * 4096ll));
    int32_t g_q15 = 32768 + (int32_t)(((int64_t)(gbase_q15 - 32768) * r) >> 15);
    g_q15 = (int32_t)(((int64_t)g_q15 * (32768 + r)) >> 15);
    return clamp32(((int64_t)2 * g_q15 * s->lp4) >> 15, -131072, 131072);
}

static int32_t output_highpass(acid303_t *s, int32_t x) {
    /* AC coupling/output stage removes DC and some sub-bass, as the hardware does. */
    int32_t y = x - s->output_hp_x + (int32_t)(((int64_t)s->output_hp_y * 32675) >> 15);
    s->output_hp_x = x;
    s->output_hp_y = y;
    return y;
}

int32_t acid303_process(acid303_t *s) {
    slide_step(s);
    if (s->lfo_amount) s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u;
    envelope_step(s);

    int32_t osc = oscillator(s);
    uint32_t fc = acid303_cutoff_hz(s);

    /* Filter envelope is intentionally much stronger than a generic subtractive
       synth envelope; it is the core of the acid 'pluck'. */
    fc += (uint32_t)(((int64_t)s->env * s->env_mod * 6200) >> 30);

    if (s->accented || s->accent_sweep > 0) {
        int32_t sweep = (int32_t)(((int64_t)s->accent_sweep * s->accent) >> 15);
        sweep = (int32_t)(((int64_t)sweep * (8192 + s->resonance)) >> 15);
        if (sweep > 0) fc += (uint32_t)(((int64_t)sweep * 5200) >> 15);
    }

    /* Optional MOD extension only. Stock mode leaves lfo_amount == 0. */
    if (s->lfo_amount) {
        int16_t lfo = acid303_lfo_value(s);
        int32_t delta = (int32_t)(((int64_t)lfo * s->lfo_amount * 1600) >> 22);
        int32_t m = (int32_t)fc + delta;
        fc = (uint32_t)(m < 80 ? 80 : m > 12000 ? 12000 : m);
    }
    if (fc > 12000u) fc = 12000u;

    int32_t y = teebee_ladder(s, osc, fc);
    int32_t gain = s->amp;
    if (s->accented || s->accent_env > 0) {
        int32_t ag = (int32_t)(((int64_t)s->accent_env * s->accent) >> 16);
        gain = clamp32((int64_t)gain + ag, 0, 42000);
    }
    y = (int32_t)(((int64_t)y * gain) >> 15);

    if (s->drive) {
        int32_t k = 32768 + s->drive;
        y = shape_q15((int32_t)(((int64_t)y * k) >> 15));
    }
    y = output_highpass(s, y);
    return clamp32(y, -32768, 32767);
}
