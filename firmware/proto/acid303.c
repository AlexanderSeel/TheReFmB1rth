// SPDX-License-Identifier: GPL-3.0-only
/*
 * TB-303-inspired fixed-point voice for the FM-1.
 *
 * The oscillator prototypes, four-stage filter topology, measured cutoff /
 * envelope mapping and timing are fixed-point adaptations informed by Robin
 * Schmidt's Open303 (MIT). The optional band-limited oscillator tables are
 * generated locally by tools/generate_303_wavetables.py.
 *
 * The realtime path intentionally avoids signed 64-bit arithmetic. The JieLi
 * pi32v2 target is much happier with bounded 32-bit Q15 operations, while
 * coefficient lookup tables preserve the Open303 TB-303 transfer law.
 */
#include "acid303.h"
#include <limits.h>
#include <string.h>

#ifndef REFM_ACID_WAVETABLE
#define REFM_ACID_WAVETABLE 1
#endif

#if REFM_ACID_WAVETABLE
#include "acid303_wavetable.h"
#endif

#define SR 44100u
#define Q15_ONE 32767
#define COEFF_PERIOD 32u
#define SLIDE_TAU_SAMPLES ((SR * 60u) / 1000u)
#define TB_COEFF_LUT_SIZE 65u
#define TB_COEFF_HZ_MIN 200u
#define TB_COEFF_HZ_SPAN 19800u

static int32_t clamp32(int32_t x, int32_t lo, int32_t hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static uint32_t abs_u32(int32_t x) {
    return x < 0 ? (uint32_t)(-(x + 1)) + 1u : (uint32_t)x;
}

/* Signed Q15 multiply without int64. Inputs in this voice are bounded so the
   decomposed result remains far inside int32 even when a direct a*b would
   overflow 32 bits. */
static int32_t mul_q15(int32_t a, int32_t b) {
    uint32_t ua = abs_u32(a);
    uint32_t ub = abs_u32(b);
    uint32_t ahi = ua >> 15;
    uint32_t alo = ua & 32767u;
    uint32_t bhi = ub >> 15;
    uint32_t blo = ub & 32767u;
    uint32_t v = ahi * ub + alo * bhi + ((alo * blo) >> 15);
    int neg = (a < 0) != (b < 0);
    if (v > (uint32_t)INT32_MAX) v = (uint32_t)INT32_MAX;
    return neg ? -(int32_t)v : (int32_t)v;
}

static uint16_t ratio_q15(uint32_t num, uint32_t den) {
    if (!den || num >= den) return 32767u;
    while (num > 131000u || den > 131000u) {
        num = (num + 1u) >> 1;
        den = (den + 1u) >> 1;
        if (!den) return 32767u;
    }
    return (uint16_t)((num * 32767u + den / 2u) / den);
}

static const uint32_t semitone_inc[12] = {
    796254u, 843601u, 893765u, 946911u, 1003217u, 1062871u,
    1126073u, 1193033u, 1263974u, 1339134u, 1418763u, 1503127u
};

uint32_t acid303_note_to_phase_inc(uint8_t note) {
    uint32_t base = semitone_inc[note % 12u];
    uint32_t octave = note / 12u;
    if (octave >= 32u || base > (UINT32_MAX >> octave)) return UINT32_MAX;
    return base << octave;
}

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
    uint32_t ratio, hi, lo, frac, result;
    if (t < -12) t = -12;
    if (t > 12) t = 12;
    ratio = t < 0 ? down_q16[-t] : up_q16[t];
    hi = phase_inc >> 16;
    lo = phase_inc & 65535u;
    if (hi && ratio > UINT32_MAX / hi) return UINT32_MAX;
    result = hi * ratio;
    frac = lo * (ratio >> 16);
    frac += (lo * (ratio & 65535u) + 32768u) >> 16;
    if (result > UINT32_MAX - frac) return UINT32_MAX;
    result += frac;
    return result ? result : 1u;
}

static int32_t shape_q15(int32_t x) {
    int32_t x2, x3;
    x = clamp32(x, -46340, 46340);
    x2 = mul_q15(x, x);
    x3 = mul_q15(x2, x);
    return clamp32(x - x3 / 6, -49152, 49152);
}

static uint32_t decay_tau_samples(uint16_t decay, uint8_t accented) {
    if (accented) return (SR * 200u) / 1000u;
    return (SR * (200u + ((uint32_t)decay * 1800u) / 32767u)) / 1000u;
}

static void decay_env(int32_t *env, uint32_t tau_samples) {
    int32_t step;
    if (*env <= 0) { *env = 0; return; }
    step = *env / (int32_t)(tau_samples ? tau_samples : 1u);
    if (step < 1) step = 1;
    *env -= step;
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
    int32_t d;
    if (i >= 32u) return lut[32];
    d = (int32_t)lut[i + 1u] - (int32_t)lut[i];
    return (uint16_t)((int32_t)lut[i] + (d * (int32_t)frac >> 15));
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
    int32_t d;
    if (i >= 15u) return 32767;
    d = (int32_t)lut[i + 1u] - (int32_t)lut[i];
    return (int32_t)lut[i] + (d * (int32_t)frac >> 15);
}

/* The beta.12 Open303-derived fixed-point TB_303 coefficient formulas sampled
   at 65 uniformly spaced frequencies from 200 to 20,000 Hz at the 4x
   (176.4 kHz) ladder rate. Linear interpolation stays within a few Q-LSBs of
   that implementation while removing signed-64-bit polynomial/division work
   from the realtime target path. */
static const uint16_t tb_b0_lut[TB_COEFF_LUT_SIZE] = {
    175,418,653,882,1104,1320,1530,1734,1933,2126,2313,2497,2675,
    2848,3018,3182,3344,3500,3653,3803,3948,4090,4230,4365,4498,4628,
    4754,4878,4999,5118,5234,5347,5458,5567,5673,5778,5879,5979,6077,
    6173,6267,6359,6450,6538,6625,6710,6794,6876,6957,7035,7113,7189,
    7264,7337,7409,7480,7549,7618,7684,7750,7815,7879,7941,8003,8063
};

static const uint32_t tb_kbase_lut[TB_COEFF_LUT_SIZE] = {
    70330,71424,72526,73639,74756,75880,77015,78155,79306,80461,81623,
    82797,83974,85158,86353,87552,88762,89976,91197,92428,93663,94904,
    96156,97412,98679,99948,101224,102511,103800,105095,106402,107711,109030,
    110352,111680,113019,114359,115706,117062,118420,119789,121159,122535,123921,
    125308,126701,128104,129506,130920,131072,131072,131072,131072,131072,131072,
    131072,131072,131072,131072,131072,131072,131072,131072,131072,131072
};

static void tb_coeff_lookup(uint32_t hz, int32_t *b0, int32_t *kbase) {
    uint32_t scaled, i, rem;
    int32_t db, dk;
    if (hz <= TB_COEFF_HZ_MIN) {
        *b0 = tb_b0_lut[0];
        *kbase = (int32_t)tb_kbase_lut[0];
        return;
    }
    if (hz >= TB_COEFF_HZ_MIN + TB_COEFF_HZ_SPAN) {
        *b0 = tb_b0_lut[TB_COEFF_LUT_SIZE - 1u];
        *kbase = (int32_t)tb_kbase_lut[TB_COEFF_LUT_SIZE - 1u];
        return;
    }
    scaled = (hz - TB_COEFF_HZ_MIN) * (TB_COEFF_LUT_SIZE - 1u);
    i = scaled / TB_COEFF_HZ_SPAN;
    rem = scaled - i * TB_COEFF_HZ_SPAN;
    db = (int32_t)tb_b0_lut[i + 1u] - (int32_t)tb_b0_lut[i];
    dk = (int32_t)tb_kbase_lut[i + 1u] - (int32_t)tb_kbase_lut[i];
    *b0 = (int32_t)tb_b0_lut[i] + (db * (int32_t)rem) / (int32_t)TB_COEFF_HZ_SPAN;
    *kbase = (int32_t)tb_kbase_lut[i] + (dk * (int32_t)rem) / (int32_t)TB_COEFF_HZ_SPAN;
    if (*kbase > 131072) *kbase = 131072;
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
    s->cutoff = 18688u;
    s->resonance = 0u;
    s->env_mod = 8192u;
    s->decay = 14563u;
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
        return (int16_t)clamp32(mul_q15(tri, 49152 - a / 2), -32768, 32767);
    }
    }
}

#if !REFM_ACID_WAVETABLE
static int32_t poly_blep_q15(uint32_t phase, uint32_t dt) {
    if (!dt) return 0;
    if (phase < dt) {
        int32_t x = (int32_t)ratio_q15(phase, dt);
        return (x << 1) - mul_q15(x, x) - 32768;
    }
    if (phase > 0xffffffffu - dt) {
        uint32_t remain = 0xffffffffu - phase;
        int32_t x = -(int32_t)ratio_q15(remain, dt);
        return mul_q15(x, x) + (x << 1) + 32768;
    }
    return 0;
}
#endif

#if REFM_ACID_WAVETABLE
static int32_t wavetable_lookup(const int16_t *table, uint32_t phase) {
    uint32_t idx = phase >> (32u - REFM_ACID_WT_BITS);
    uint32_t frac = (phase >> (17u - REFM_ACID_WT_BITS)) & 32767u;
    int32_t a = table[idx & REFM_ACID_WT_MASK];
    int32_t b = table[(idx + 1u) & REFM_ACID_WT_MASK];
    return a + (((b - a) * (int32_t)frac) >> 15);
}
#endif

static int32_t oscillator_substep(acid303_t *s, uint32_t sub_inc) {
    s->phase += sub_inc;
#if REFM_ACID_WAVETABLE
    {
        uint32_t band = 0u;
        const int16_t *const *bank = s->square ? refm_acid_wt_square : refm_acid_wt_saw;
        while (band + 1u < REFM_ACID_WT_BANDS &&
               s->phase_inc >= refm_acid_wt_threshold_inc[band]) ++band;
        if (band + 1u < REFM_ACID_WT_BANDS &&
            s->phase_inc > refm_acid_wt_xfade_start_inc[band]) {
            uint32_t start = refm_acid_wt_xfade_start_inc[band];
            uint32_t end = refm_acid_wt_threshold_inc[band];
            uint16_t mix = ratio_q15(s->phase_inc - start, end - start);
            int32_t lo = wavetable_lookup(bank[band], s->phase);
            int32_t hi = wavetable_lookup(bank[band + 1u], s->phase);
            return lo + (((hi - lo) * (int32_t)mix) >> 15);
        }
        return wavetable_lookup(bank[band], s->phase);
    }
#else
    {
        int32_t blep_wrap = poly_blep_q15(s->phase, sub_inc);
        if (!s->square) {
            int32_t saw = (int32_t)(s->phase >> 16) - 32768;
            return clamp32(saw - blep_wrap, -32768, 32767);
        }
        {
            int32_t sq = s->phase < 0x80000000u ? 16000 : -16000;
            int32_t blep_duty = poly_blep_q15(s->phase - 0x80000000u, sub_inc);
            sq += (blep_wrap * 16000) >> 15;
            sq -= (blep_duty * 16000) >> 15;
            return clamp32(sq, -32768, 32767);
        }
    }
#endif
}

static void slide_step(acid303_t *s) {
    uint32_t d, step;
    if (!s->sliding) return;
    if (s->slide_target_inc > s->phase_inc) {
        d = s->slide_target_inc - s->phase_inc;
        step = d / SLIDE_TAU_SAMPLES;
        if (!step) step = 1u;
        if (step >= d) { s->phase_inc = s->slide_target_inc; s->sliding = 0u; }
        else s->phase_inc += step;
    } else if (s->slide_target_inc < s->phase_inc) {
        d = s->phase_inc - s->slide_target_inc;
        step = d / SLIDE_TAU_SAMPLES;
        if (!step) step = 1u;
        if (step >= d) { s->phase_inc = s->slide_target_inc; s->sliding = 0u; }
        else s->phase_inc -= step;
    } else {
        s->sliding = 0u;
    }
}

static void envelope_step(acid303_t *s) {
    int32_t accent_target, d;
    decay_env(&s->env, decay_tau_samples(s->decay, s->accented));
    s->env_rc = s->env;
    accent_target = (s->accented && s->accent) ? s->env : 0;
    d = accent_target - s->accent_env;
    s->accent_env += (d * 50) >> 15;
    if (s->gate) decay_env(&s->amp, (SR * 1230u) / 1000u);
    else decay_env(&s->amp, (SR * (s->accented ? 50u : 1u)) / 1000u);
    if (!s->amp) {
        s->amp_stage = ACID_ENV_OFF;
        if (!s->gate) s->idle = 1u;
    }
}

static int32_t feedback_highpass(acid303_t *s, int32_t x) {
    int32_t d = x - s->resonance_hp_lp;
    s->resonance_hp_lp += (d * 175) >> 15;
    return x - s->resonance_hp_lp;
}

static int32_t pre_ladder_highpass(acid303_t *s, int32_t x) {
    int32_t diff = x - s->output_hp_x;
    int32_t y = mul_q15(s->output_hp_y + diff, 32716);
    s->output_hp_x = x;
    s->output_hp_y = y;
    return y;
}

static int32_t post_ladder_highpass(acid303_t *s, int32_t x) {
    int32_t diff = x - s->post_hp_x;
    int32_t y = mul_q15(s->post_hp_y + diff, 32655);
    s->post_hp_x = x;
    s->post_hp_y = y;
    return y;
}

static int32_t declick_gain(acid303_t *s, int32_t target) {
    s->gain_smooth1 += ((target - s->gain_smooth1) * 920) >> 15;
    s->gain_smooth2 += ((s->gain_smooth1 - s->gain_smooth2) * 920) >> 15;
    return mul_q15(s->gain_smooth2, 23170);
}

static void set_filter_coeff_target(acid303_t *s, uint32_t cutoff_hz) {
    int32_t b0, kbase;
    int32_t r = resonance_skew_q15(s->resonance);
    int32_t k, gbase, g;
    tb_coeff_lookup(cutoff_hz, &b0, &kbase);
    k = (int32_t)(((uint32_t)kbase * (uint32_t)r) >> 12);
    gbase = (kbase * 8) / 17;
    g = 32768 + mul_q15(gbase - 32768, r);
    g = mul_q15(g, 32768 + r);

    s->coeff_b0 = b0;
    s->coeff_k = k;
    s->coeff_g = g;
    s->coeff_b0_step = 0;
    s->coeff_k_step = 0;
    s->coeff_g_step = 0;
    s->coeff_valid = 1u;
    s->coeff_countdown = COEFF_PERIOD;
}

static int32_t teebee_ladder_substep(acid303_t *s, int32_t in) {
    int32_t fb = clamp32(mul_q15(s->lp4, s->coeff_k), -262144, 262144);
    int32_t y0 = clamp32(in - feedback_highpass(s, fb), -196608, 196608);
    int32_t d1 = clamp32(y0 - s->lp1 + s->lp2, -262144, 262144);
    int32_t d2, d3, d4;

    s->lp1 = clamp32(s->lp1 + 2 * mul_q15(s->coeff_b0, d1), -262144, 262144);
    d2 = clamp32(s->lp1 - 2 * s->lp2 + s->lp3, -262144, 262144);
    s->lp2 = clamp32(s->lp2 + mul_q15(s->coeff_b0, d2), -262144, 262144);
    d3 = clamp32(s->lp2 - 2 * s->lp3 + s->lp4, -262144, 262144);
    s->lp3 = clamp32(s->lp3 + mul_q15(s->coeff_b0, d3), -262144, 262144);
    d4 = clamp32(s->lp3 - 2 * s->lp4, -262144, 262144);
    s->lp4 = clamp32(s->lp4 + mul_q15(s->coeff_b0, d4), -262144, 262144);
    return clamp32(2 * mul_q15(s->coeff_g, s->lp4), -131072, 131072);
}

static int32_t exp2_q15(int32_t x_q15) {
    int32_t ip, frac, f2, f3, f4, y;
    if (x_q15 < -(2 << 15)) x_q15 = -(2 << 15);
    if (x_q15 >  (5 << 15)) x_q15 =  (5 << 15);
    if (x_q15 >= 0) ip = x_q15 >> 15;
    else ip = -(int32_t)(((uint32_t)(-x_q15) + 32767u) >> 15);
    frac = x_q15 - ip * 32768;
    f2 = (frac * frac) >> 15;
    f3 = (f2 * frac) >> 15;
    f4 = (f3 * frac) >> 15;
    y = 32768 + ((22713 * frac) >> 15) + ((7874 * f2) >> 15) +
        ((1819 * f3) >> 15) + ((315 * f4) >> 15);
    if (ip >= 0) y <<= ip;
    else y >>= -ip;
    return clamp32(y, 4096, 1048576);
}

static uint32_t measured_env_cutoff(acid303_t *s) {
    int32_t c = s->cutoff > 32767u ? 32767 : (int32_t)s->cutoff;
    int32_t e = s->env_mod > 32767u ? 32767 : (int32_t)s->env_mod;
    int32_t s_lo = 24149 + mul_q15(123666, e);
    int32_t s_hi = 28323 + mul_q15(137447, e);
    int32_t scaler = s_lo + mul_q15(s_hi - s_lo, c);
    int32_t offset = 9647 + mul_q15(1582, c);
    int32_t tmp1 = mul_q15(scaler, s->env_rc - offset);
    int32_t tmp2 = (s->accented && s->accent) ? mul_q15((int32_t)s->accent, s->accent_env) : 0;
    int32_t factor = exp2_q15(tmp1 + tmp2);
    int32_t fc = mul_q15((int32_t)acid303_cutoff_hz(s), factor);
    if (fc < 200) fc = 200;
    if (fc > 20000) fc = 20000;
    return (uint32_t)fc;
}

static uint32_t modulated_cutoff(acid303_t *s) {
    uint32_t fc = measured_env_cutoff(s);
    if (s->lfo_amount) {
        int16_t lfo = acid303_lfo_value(s);
        int32_t coarse = ((int32_t)lfo * (int32_t)s->lfo_amount) >> 7;
        int32_t delta = (coarse * 1600) >> 15;
        int32_t m = (int32_t)fc + delta;
        fc = (uint32_t)(m < 200 ? 200 : m > 20000 ? 20000 : m);
    }
    return fc;
}

int32_t acid303_process(acid303_t *s) {
    uint32_t sub_inc, fc;
    int32_t y0, y1, y2, y3, y, gain;
    slide_step(s);
    if (s->lfo_amount) s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u;
    envelope_step(s);

    fc = modulated_cutoff(s);
    if (!s->coeff_countdown) set_filter_coeff_target(s, fc);
    else --s->coeff_countdown;

    sub_inc = s->phase_inc >> 2;
    if (!sub_inc && s->phase_inc) sub_inc = 1u;

    y0 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
    y1 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
    y2 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
    y3 = teebee_ladder_substep(s, pre_ladder_highpass(s, -oscillator_substep(s, sub_inc)));
    y = post_ladder_highpass(s, (y0 + y1 + y2 + y3) / 4);

    gain = s->amp;
    if (s->gate) {
        gain += (s->env * 14746) >> 15;
        if (s->accented && s->accent) {
            int32_t accent_gain = ((int32_t)s->env * (int32_t)s->accent) >> 15;
            gain = clamp32(gain + 4 * accent_gain, 0, 196608);
        }
    }
    gain = declick_gain(s, gain);
    y = mul_q15(y, gain);
    if (s->drive) {
        int32_t k = 32768 + s->drive;
        y = shape_q15(mul_q15(y, k));
    }
    return clamp32(y, -32768, 32767);
}
