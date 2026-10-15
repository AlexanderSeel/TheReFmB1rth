// SPDX-License-Identifier: GPL-3.0-only
/*
 * TB-303-inspired fixed-point voice for the FM-1.
 *
 * The oscillator prototypes, four-stage filter topology, measured cutoff /
 * envelope mapping and timing are fixed-point adaptations informed by Robin
 * Schmidt's Open303 (MIT). The optional band-limited oscillator tables are
 * generated locally by tools/generate_303_wavetables.py.
 */
#include "acid303.h"
#include <string.h>

#ifndef REFM_ACID_WAVETABLE
#define REFM_ACID_WAVETABLE 1
#endif

#if REFM_ACID_WAVETABLE
#include "acid303_wavetable.h"
#endif

#define SR 44100u
#define Q15_ONE 32767
#define Q20_ONE 1048576ll
#define COEFF_PERIOD 32u
#define ACID_OVERSAMPLE 4u
#define SLIDE_TAU_SAMPLES ((SR * 60u) / 1000u)

static int32_t clamp32(int64_t x, int32_t lo, int32_t hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return (int32_t)x;
}

/* Equal-tempered DDS increments at 44.1 kHz for MIDI notes 0..11
   (C-1..B-1, A4=440 Hz). */
static const uint32_t semitone_inc[12] = {
    796254u, 843601u, 893765u, 946911u, 1003217u, 1062871u,
    1126073u, 1193033u, 1263974u, 1339134u, 1418763u, 1503127u
};

uint32_t acid303_note_to_phase_inc(uint8_t note) {
    uint32_t octave = note / 12u;
    uint64_t inc = (uint64_t)semitone_inc[note % 12u] << octave;
    /* MIDI 0..127 only needs shifts up to octave 10 and remains within Q32 at
       44.1 kHz. Keep the saturation anyway so this helper is well defined for
       every uint8_t input instead of folding all notes >=96 onto one octave. */
    if (inc > 0xffffffffull) inc = 0xffffffffull;
    return (uint32_t)inc;
}

/* ReBirth's Tune control is +/- one octave in semitone steps. */
uint32_t acid303_apply_tune(uint32_t phase_inc, int8_t tune) {
    static const uint32_t up_q16[13] = {
        65536u,69433u,73562u,77936u,82570u,87480u,92682u,
        98193u,104032u,110218u,116772u,123715u,131072u
    };
    static const uint32_t down_q16[13] = {
        65536u,61858u,58386u,55109u,52016u,49097u,46341u,
        43740u,41285u,38968u,36781u,34716u,32768u
    };
    int t = tune;
    uint32_t ratio;
    uint64_t tuned;
    if (t < -12) t = -12;
    if (t > 12) t = 12;
    ratio = t < 0 ? down_q16[-t] : up_q16[t];
    tuned = ((uint64_t)phase_inc * ratio + 32768u) >> 16;
    if (tuned < 1u) tuned = 1u;
    if (tuned > 0xffffffffull) tuned = 0xffffffffull;
    return (uint32_t)tuned;
}

static int32_t shape_q15(int32_t x) {
    x = clamp32(x, -46340, 46340);
    {
        int64_t x2 = ((int64_t)x * x) >> 15;
        int64_t x3 = (x2 * x) >> 15;
        return clamp32((int64_t)x - x3 / 6, -49152, 49152);
    }
}

static uint32_t decay_tau_samples(uint16_t decay, uint8_t accented) {
    if (accented) return (SR * 200u) / 1000u;
    {
        uint32_t ms = 200u + ((uint32_t)decay * 1800u) / 32767u;
        return (SR * ms) / 1000u;
    }
}

static void decay_env(int32_t *env, uint32_t tau_samples) {
    if (*env <= 0) { *env = 0; return; }
    {
        int32_t step = *env / (int32_t)(tau_samples ? tau_samples : 1u);
        if (step < 1) step = 1;
        *env -= step;
    }
    if (*env < 8) *env = 0;
}

uint16_t acid303_cutoff_hz(const acid303_t *s) {
    static const uint16_t lut[33] = {
        314,334,356,380,405,431,459,489,522,556,592,
        631,672,716,763,814,867,924,984,1049,1118,1191,
        1269,1352,1441,1535,1636,1743,1857,1979,2109,2247,2394
    };
    uint32_t n = s->cutoff > 32767u ? 32767u : s->cutoff;
    uint32_t pos = n * 32u;
    uint32_t i = pos >> 15;
    uint32_t frac = pos & 32767u;
    if (i >= 32u) return lut[32];
    return (uint16_t)((uint32_t)lut[i] +
        (uint32_t)(((int64_t)((int32_t)lut[i + 1u] - (int32_t)lut[i]) * frac) >> 15));
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
    return (int32_t)lut[i] + (int32_t)(((int64_t)((int32_t)lut[i + 1u] - lut[i]) * frac) >> 15);
}

/* Open303 TB_303 coefficient formulas evaluated at the same 4x (176.4 kHz)
   oscillator/filter rate used by Open303. */
static int32_t tb_b0_q15(uint32_t hz) {
    if (hz < 200u) hz = 200u;
    if (hz > 20000u) hz = 20000u;
    {
        int64_t fx = ((int64_t)hz << 20) / 249468ll;
        int64_t fx2 = (fx * fx) >> 20;
        int64_t num = 477ll + ((6493013ll * fx) >> 20);
        int64_t den = Q20_ONE + ((12958674ll * fx) >> 20) + ((4630128ll * fx2) >> 20);
        if (den <= 0) return 1;
        return clamp32((num << 15) / den, 1, 16384);
    }
}

static int32_t tb_kbase_q12(uint32_t hz) {
    if (hz < 200u) hz = 200u;
    if (hz > 20000u) hz = 20000u;
    {
        int64_t x = ((int64_t)hz << 20) / 249468ll;
        int64_t y = 4096ll;
        y = 29485874ll + ((y * x) >> 20);
        y = -23911595ll + ((y * x) >> 20);
        y = -1951634ll + ((y * x) >> 20);
        y = 2518860ll + ((y * x) >> 20);
        y = 876017ll + ((y * x) >> 20);
        y = 69627ll + ((y * x) >> 20);
        return clamp32(y, 4096, 131072);
    }
}

static void reset_analog_path(acid303_t *s) {
    s->phase = 0u;
    s->lp1 = s->lp2 = s->lp3 = s->lp4 = 0;
    s->resonance_hp_lp = 0;
    s->output_hp_x = 0;
    s->output_hp_y = 0;
    s->post_hp_x = 0;
    s->post_hp_y = 0;
    s->gain_smooth1 = 0;
    s->gain_smooth2 = 0;
    s->env_rc = 0;
    s->accent_env = 0;
    s->accent_sweep = 0;
    s->coeff_valid = 0u;
    s->coeff_countdown = 0u;
}

void acid303_init(acid303_t *s) {
    memset(s, 0, sizeof(*s));
    /* Reference-like neutral startup, close to Open303's defaults. The old
       startup was much darker: ~560 Hz cutoff, ~58% resonance and ~72% env
       modulation, which made the voice sound like sub-bass rumble before the
       user had touched a control. */
    s->cutoff = 18688u;      /* ~1000 Hz nominal cutoff */
    s->resonance = 0u;
    s->env_mod = 8192u;      /* 25% */
    s->decay = 14563u;       /* ~1000 ms in the stock 200..2000 ms range */
    s->accent = 0u;
    s->drive = 0u;
    s->tune = 0;
    s->amp_sustain = 0u;
    s->amp_release = 0u;
    s->lfo_rate = 35u;
    s->lfo_amount = 0u;
    s->lfo_shape = ACID_LFO_SINE;
    s->amp_stage = ACID_ENV_OFF;
    s->idle = 1u;
}

void acid303_set_note(acid303_t *s, uint8_t note, uint8_t accent, uint8_t slide) {
    uint32_t inc = acid303_apply_tune(acid303_note_to_phase_inc(note), s->tune);
    if (slide && s->gate) {
        s->slide_target_inc = inc;
        s->sliding = 1u;
        s->accented = accent ? 1u : 0u;
    } else {
        if (s->idle) reset_analog_path(s);
        s->phase_inc = inc;
        s->slide_target_inc = inc;
        s->sliding = 0u;
        s->env = Q15_ONE;
        s->amp = Q15_ONE;
        s->amp_stage = ACID_ENV_DECAY;
        s->accented = accent ? 1u : 0u;
    }
    s->gate = 1u;
    s->idle = 0u;
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

#if !REFM_ACID_WAVETABLE
static int32_t poly_blep_q15(uint32_t phase, uint32_t dt) {
    if (!dt) return 0;
    if (phase < dt) {
        int32_t x = (int32_t)(((uint64_t)phase << 15) / dt);
        return (x << 1) - (int32_t)(((int64_t)x * x) >> 15) - 32768;
    }
    if (phase > 0xffffffffu - dt) {
        uint32_t remain = 0xffffffffu - phase;
        int32_t x = -(int32_t)(((uint64_t)remain << 15) / dt);
        return (int32_t)(((int64_t)x * x) >> 15) + (x << 1) + 32768;
    }
    return 0;
}
#endif

static int32_t oscillator_substep(acid303_t *s, uint32_t sub_inc) {
    s->phase += sub_inc;
#if REFM_ACID_WAVETABLE
    {
        uint32_t band = 0u;
        uint32_t idx;
        uint32_t frac;
        const int16_t *table;
        int32_t a, b;
        while (band + 1u < REFM_ACID_WT_BANDS && s->phase_inc > refm_acid_wt_threshold_inc[band]) ++band;
        table = s->square ? refm_acid_wt_square[band] : refm_acid_wt_saw[band];
        idx = s->phase >> (32u - REFM_ACID_WT_BITS);
        frac = (s->phase >> (17u - REFM_ACID_WT_BITS)) & 32767u;
        a = table[idx & REFM_ACID_WT_MASK];
        b = table[(idx + 1u) & REFM_ACID_WT_MASK];
        return a + (int32_t)(((int64_t)(b - a) * frac) >> 15);
    }
#else
    {
        const uint32_t duty = 0x80000000u;
        int32_t blep_wrap = poly_blep_q15(s->phase, sub_inc);
        if (!s->square) {
            int32_t saw = (int32_t)(s->phase >> 16) - 32768;
            return clamp32((int64_t)saw - blep_wrap, -32768, 32767);
        }
        {
            int32_t sq = s->phase < duty ? 16000 : -16000;
            uint32_t rel = s->phase - duty;
            int32_t blep_duty = poly_blep_q15(rel, sub_inc);
            sq += (int32_t)(((int64_t)blep_wrap * 32000) >> 16);
            sq -= (int32_t)(((int64_t)blep_duty * 32000) >> 16);
            return clamp32(sq, -32768, 32767);
        }
    }
#endif
}

static void slide_step(acid303_t *s) {
    if (!s->sliding) return;
    {
        int64_t d = (int64_t)s->slide_target_inc - (int64_t)s->phase_inc;
        /* Open303's stock pitch-slew time constant is 60 ms. This function is
           called at the 44.1-kHz output rate, so one e-fold is 2646 samples. */
        int64_t step = d / (int64_t)SLIDE_TAU_SAMPLES;
        if (!step && d) step = d > 0 ? 1 : -1;
        s->phase_inc = (uint32_t)((int64_t)s->phase_inc + step);
        if (d < 3 && d > -3) { s->phase_inc = s->slide_target_inc; s->sliding = 0u; }
    }
}

static void envelope_step(acid303_t *s) {
    decay_env(&s->env, decay_tau_samples(s->decay, s->accented));
    s->env_rc = s->env;
    {
        int32_t accent_target = (s->accented && s->accent) ? s->env : 0;
        s->accent_env += (int32_t)(((int64_t)(accent_target - s->accent_env) * 50) >> 15);
    }
    if (s->gate) decay_env(&s->amp, (SR * 1230u) / 1000u);
    else decay_env(&s->amp, (SR * (s->accented ? 50u : 1u)) / 1000u);
    if (!s->amp) {
        s->amp_stage = ACID_ENV_OFF;
        if (!s->gate) s->idle = 1u;
    }
}

static int32_t feedback_highpass(acid303_t *s, int32_t x) {
    /* 150 Hz feedback HP at the 176.4 kHz ladder rate. */
    int32_t d = x - s->resonance_hp_lp;
    s->resonance_hp_lp += (int32_t)(((int64_t)d * 175) >> 15);
    return x - s->resonance_hp_lp;
}

static int32_t pre_ladder_highpass(acid303_t *s, int32_t x) {
    /* Standard one-pole HP: y[n] = alpha * (y[n-1] + x[n] - x[n-1]).
       Open303 uses 44.486 Hz before the ladder at the oversampled rate. */
    int32_t diff = x - s->output_hp_x;
    int32_t y = (int32_t)(((int64_t)(s->output_hp_y + diff) * 32716) >> 15);
    s->output_hp_x = x;
    s->output_hp_y = y;
    return y;
}

static int32_t post_ladder_highpass(acid303_t *s, int32_t x) {
    /* Open303's 24.167 Hz output HP at 44.1 kHz. */
    int32_t diff = x - s->post_hp_x;
    int32_t y = (int32_t)(((int64_t)(s->post_hp_y + diff) * 32655) >> 15);
    s->post_hp_x = x;
    s->post_hp_y = y;
    return y;
}

static int32_t declick_gain(acid303_t *s, int32_t target) {
    s->gain_smooth1 += (int32_t)(((int64_t)(target - s->gain_smooth1) * 920) >> 15);
    s->gain_smooth2 += (int32_t)(((int64_t)(s->gain_smooth1 - s->gain_smooth2) * 920) >> 15);
    return (int32_t)(((int64_t)s->gain_smooth2 * 23170) >> 15);
}

static void set_filter_coeff_target(acid303_t *s, uint32_t cutoff_hz) {
    int32_t b0 = tb_b0_q15(cutoff_hz);
    int32_t r = resonance_skew_q15(s->resonance);
    int32_t kbase = tb_kbase_q12(cutoff_hz);
    int32_t k = (int32_t)(((int64_t)kbase * r) >> 12);
    int32_t gbase = (int32_t)(((int64_t)kbase * 32768) / (17ll * 4096ll));
    int32_t g = 32768 + (int32_t)(((int64_t)(gbase - 32768) * r) >> 15);
    g = (int32_t)(((int64_t)g * (32768 + r)) >> 15);
    if (!s->coeff_valid) {
        s->coeff_b0 = b0; s->coeff_k = k; s->coeff_g = g;
        s->coeff_b0_step = s->coeff_k_step = s->coeff_g_step = 0;
        s->coeff_valid = 1u;
    } else {
        s->coeff_b0_step = (b0 - s->coeff_b0) / (int32_t)COEFF_PERIOD;
        s->coeff_k_step = (k - s->coeff_k) / (int32_t)COEFF_PERIOD;
        s->coeff_g_step = (g - s->coeff_g) / (int32_t)COEFF_PERIOD;
    }
    s->coeff_countdown = COEFF_PERIOD;
}

static void advance_filter_coeffs(acid303_t *s) {
    if (!s->coeff_countdown) return;
    s->coeff_b0 += s->coeff_b0_step;
    s->coeff_k += s->coeff_k_step;
    s->coeff_g += s->coeff_g_step;
    --s->coeff_countdown;
}

static int32_t teebee_ladder_substep(acid303_t *s, int32_t in) {
    int32_t fb = clamp32(((int64_t)s->lp4 * s->coeff_k) >> 15, -262144, 262144);
    int32_t y0 = clamp32((int64_t)in - feedback_highpass(s, fb), -196608, 196608);
    int32_t d1 = clamp32((int64_t)y0 - s->lp1 + s->lp2, -262144, 262144);
    s->lp1 = clamp32((int64_t)s->lp1 + (((int64_t)2 * s->coeff_b0 * d1) >> 15), -262144, 262144);
    {
        int32_t d2 = clamp32((int64_t)s->lp1 - 2ll * s->lp2 + s->lp3, -262144, 262144);
        s->lp2 = clamp32((int64_t)s->lp2 + (((int64_t)s->coeff_b0 * d2) >> 15), -262144, 262144);
    }
    {
        int32_t d3 = clamp32((int64_t)s->lp2 - 2ll * s->lp3 + s->lp4, -262144, 262144);
        s->lp3 = clamp32((int64_t)s->lp3 + (((int64_t)s->coeff_b0 * d3) >> 15), -262144, 262144);
    }
    {
        int32_t d4 = clamp32((int64_t)s->lp3 - 2ll * s->lp4, -262144, 262144);
        s->lp4 = clamp32((int64_t)s->lp4 + (((int64_t)s->coeff_b0 * d4) >> 15), -262144, 262144);
    }
    return clamp32(((int64_t)2 * s->coeff_g * s->lp4) >> 15, -131072, 131072);
}

static int32_t exp2_q15(int32_t x_q15) {
    int32_t ip;
    int32_t frac;
    int64_t f, f2, f3, f4, y;
    if (x_q15 < -(2 << 15)) x_q15 = -(2 << 15);
    if (x_q15 >  (5 << 15)) x_q15 =  (5 << 15);
    if (x_q15 >= 0) ip = x_q15 >> 15;
    else ip = -(int32_t)(((uint32_t)(-x_q15) + 32767u) >> 15);
    frac = x_q15 - ip * 32768;
    f = frac;
    f2 = (f * f) >> 15;
    f3 = (f2 * f) >> 15;
    f4 = (f3 * f) >> 15;
    y = 32768ll + ((22713ll * f) >> 15) + ((7874ll * f2) >> 15) +
        ((1819ll * f3) >> 15) + ((315ll * f4) >> 15);
    if (ip >= 0) y <<= ip;
    else y >>= -ip;
    return clamp32(y, 4096, 1048576);
}

static uint32_t measured_env_cutoff(acid303_t *s) {
    int32_t c = s->cutoff > 32767u ? 32767 : (int32_t)s->cutoff;
    int32_t e = s->env_mod > 32767u ? 32767 : (int32_t)s->env_mod;
    int32_t s_lo = 24149 + (int32_t)(((int64_t)123666 * e) >> 15);
    int32_t s_hi = 28323 + (int32_t)(((int64_t)137447 * e) >> 15);
    int32_t scaler = s_lo + (int32_t)(((int64_t)(s_hi - s_lo) * c) >> 15);
    int32_t offset = 9647 + (int32_t)(((int64_t)1582 * c) >> 15);
    int32_t tmp1 = (int32_t)(((int64_t)scaler * (s->env_rc - offset)) >> 15);
    int32_t tmp2 = (s->accented && s->accent) ?
        (int32_t)(((int64_t)s->accent * s->accent_env) >> 15) : 0;
    int32_t factor = exp2_q15(tmp1 + tmp2);
    uint32_t fc = (uint32_t)(((int64_t)acid303_cutoff_hz(s) * factor) >> 15);
    if (fc < 200u) fc = 200u;
    if (fc > 20000u) fc = 20000u;
    return fc;
}

static uint32_t modulated_cutoff(acid303_t *s) {
    uint32_t fc = measured_env_cutoff(s);
    if (s->lfo_amount) {
        int16_t lfo = acid303_lfo_value(s);
        int32_t delta = (int32_t)(((int64_t)lfo * s->lfo_amount * 1600) >> 22);
        int32_t m = (int32_t)fc + delta;
        fc = (uint32_t)(m < 200 ? 200 : m > 20000 ? 20000 : m);
    }
    return fc;
}

int32_t acid303_process(acid303_t *s) {
    slide_step(s);
    if (s->lfo_amount) s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u;
    envelope_step(s);

    {
        uint32_t fc = modulated_cutoff(s);
        if (!s->coeff_countdown) set_filter_coeff_target(s, fc);
        advance_filter_coeffs(s);
    }

    {
        uint32_t sub_inc = s->phase_inc >> 2;
        int32_t y0, y1, y2, y3, y, gain;
        if (!sub_inc && s->phase_inc) sub_inc = 1u;

        /* Match Open303's 4x oscillator/filter processing rate. The generated
           source tables are already band-limited to the final 44.1 kHz output
           Nyquist, so averaging the four ladder samples is a conservative
           embedded downsampler without reintroducing ultrasonic aliases. */
        y0 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
        y1 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
        y2 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
        y3 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
        y = post_ladder_highpass(s, (y0 + y1 + y2 + y3) / 4);

        gain = s->amp;
        if (s->gate) {
            gain += (int32_t)(((int64_t)s->env * 14746) >> 15);
            if (s->accented && s->accent) {
                int32_t accent_gain = (int32_t)(((int64_t)s->env * s->accent) >> 15);
                gain = clamp32((int64_t)gain + 4ll * accent_gain, 0, 196608);
            }
        }
        gain = declick_gain(s, gain);
        y = (int32_t)(((int64_t)y * gain) >> 15);
        if (s->drive) {
            int32_t k = 32768 + s->drive;
            y = shape_q15((int32_t)(((int64_t)y * k) >> 15));
        }
        return clamp32(y, -32768, 32767);
    }
}