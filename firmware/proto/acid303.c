// SPDX-License-Identifier: GPL-3.0-only
/*
 * FM-1 TB-303 engine.
 *
 * The public API/project values are TheReFmB1rth's, while the audible signal
 * path follows the strategy proven on the same hardware by fm1-x0x's GPL-3.0
 * Open303 port: float-only/no-libm maths, polyBLEP oscillator, Open303's
 * measured envelope-modulation mapping, TeeBee TB_303 filter coefficients and
 * very short control intervals with interpolation. This replaces the beta.13
 * Q15/LUT ladder whose coarse state transitions produced physical gain jumps
 * and runaway distortion when cutoff/resonance/env-mod crossed zero.
 */
#include "acid303.h"
#include <limits.h>
#include <string.h>

#define SR 44100.0f
#define OS 2.0f
#define FSO (SR * OS)
#define CTRL 4u
#define Q15_ONE 32767

typedef union { float f; uint32_t u; int32_t i; } fbits_t;

static float ffabs(float x) {
    fbits_t b;
    b.f = x;
    b.u &= 0x7fffffffu;
    return b.f;
}

static float fclamp(float x, float lo, float hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

static float ffloor(float x) {
    int32_t i = (int32_t)x;
    float f = (float)i;
    return f > x ? f - 1.0f : f;
}

static float exp2_nf(int32_t n, float f) {
    fbits_t b;
    float p;
    if (n < -126) return 0.0f;
    p = 1.534580891e-04f;
    p = p * f + 1.339993158e-03f;
    p = p * f + 9.618488972e-03f;
    p = p * f + 5.550328776e-02f;
    p = p * f + 2.402264689e-01f;
    p = p * f + 6.931472057e-01f;
    p = p * f + 1.0f;
    b.f = p;
    b.i += n << 23;
    return b.f;
}

static float fexp2(float x) {
    float n;
    if (x < -126.0f) return 0.0f;
    if (x > 127.0f) x = 127.0f;
    n = ffloor(x + 0.5f);
    return exp2_nf((int32_t)n, x - n);
}

static float fexp(float x) {
    float n;
    if (x < -87.3f) return 0.0f;
    if (x > 88.7f) x = 88.7f;
    n = ffloor(x * 1.44269504088896f + 0.5f);
    x = (x - n * 0.693145752f) - n * 1.42860677e-6f;
    return exp2_nf((int32_t)n, x * 1.44269504088896f);
}

static float ftanh(float x) {
    float a = ffabs(x), e, r;
    if (a < 0.0004f) return x;
    if (a < 0.5f) {
        float z = x * x;
        return x * (1.0f + z * (-0.333333333f + z * (0.133333333f +
               z * (-0.0539682540f + z * (0.0218694885f +
               z * (-0.00886323552f + z * 0.00359212803f))))));
    }
    if (a > 9.0f) return x > 0.0f ? 1.0f : -1.0f;
    e = fexp2(2.88539008177793f * a);
    r = 1.0f - 2.0f / (e + 1.0f);
    return x > 0.0f ? r : -r;
}

static int32_t clamp32(int32_t x, int32_t lo, int32_t hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

static int16_t q15_from_float(float x) {
    x = fclamp(x, -1.0f, 0.9999695f);
    return (int16_t)(x * 32768.0f);
}

static int32_t q15_state(float x) {
    x = fclamp(x, -8.0f, 8.0f);
    return (int32_t)(x * 32768.0f);
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
        65536u, 69433u, 73562u, 77936u, 82570u, 87480u, 92682u,
        98193u, 104032u, 110218u, 116772u, 123715u, 131072u
    };
    static const uint32_t down_q16[13] = {
        65536u, 61858u, 58386u, 55109u, 52016u, 49097u, 46341u,
        43740u, 41285u, 38968u, 36781u, 34716u, 32768u
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

static float inc_to_hz(uint32_t inc) {
    return (float)inc * (SR / 4294967296.0f);
}

uint16_t acid303_cutoff_hz(const acid303_t *s) {
    static const uint16_t lut[33] = {
        314, 334, 356, 380, 405, 431, 459, 489, 522, 556, 592,
        631, 672, 716, 763, 814, 867, 924, 984, 1049, 1118, 1191,
        1269, 1352, 1441, 1535, 1636, 1743, 1857, 1979, 2109, 2247, 2394
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
        int32_t a = tri < 0 ? -tri : tri;
        return (int16_t)clamp32((tri * (49152 - (a >> 1))) >> 15, -32768, 32767);
    }
    }
}

static float resonance_skew(const acid303_t *s) {
    float n = (float)(s->resonance > 32767u ? 32767u : s->resonance) * (1.0f / 32767.0f);
    return (1.0f - fexp(-3.0f * n)) * (1.0f / 0.950212932f);
}

static float decay_coeff(const acid303_t *s) {
    float n = (float)(s->decay > 32767u ? 32767u : s->decay) * (1.0f / 32767.0f);
    float ms = 200.0f * fexp2(3.321928095f * n);
    return fexp(-1000.0f / (ms * SR));
}

static void clear_filter(acid303_t *s) {
    s->f_hp1_x = s->f_hp1_y = s->f_fb_x = s->f_fb_y = 0.0f;
    s->f_y1 = s->f_y2 = s->f_y3 = s->f_y4 = 0.0f;
    s->f_post_x = s->f_post_y = 0.0f;
    s->f_decim_x1 = s->f_decim_y1 = 0.0f;
    s->lp1 = s->lp2 = s->lp3 = s->lp4 = 0;
    s->output_hp_x = s->output_hp_y = 0;
    s->post_hp_x = s->post_hp_y = 0;
    s->resonance_hp_lp = 0;
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
    s->lfo_rate = 35u;
    s->lfo_amount = 0u;
    s->lfo_shape = ACID_LFO_SINE;
    s->amp_stage = ACID_ENV_OFF;
    s->idle = 1u;
    s->f_freq = s->f_target_freq = 440.0f;
    s->f_slew = 440.0f;
    s->f_ctrl_left = 0u;
}

void acid303_set_note(acid303_t *s, uint8_t note, uint8_t accent, uint8_t slide) {
    uint32_t inc = acid303_apply_tune(acid303_note_to_phase_inc(note), s->tune);
    float f = inc_to_hz(inc);

    if (slide && s->gate) {
        s->slide_target_inc = inc;
        s->f_target_freq = f;
        s->sliding = 1u;
        s->accented = accent ? 1u : 0u;
    } else {
        if (s->idle) {
            clear_filter(s);
            s->phase = 0u;
            s->f_phase = 0.0f;
        }
        s->phase_inc = inc;
        s->slide_target_inc = inc;
        s->f_freq = s->f_target_freq = s->f_slew = f;
        s->sliding = 0u;
        s->f_env = 1.0f;
        s->f_env_rc1 = 1.0f;
        s->f_env_rc2 = accent ? 0.05f : 0.0f;
        s->f_amp = 1.0f;
        s->env = Q15_ONE;
        s->env_rc = Q15_ONE;
        s->accent_env = accent ? 1638 : 0;
        s->accent_sweep = s->accent_env;
        s->amp = Q15_ONE;
        s->amp_stage = ACID_ENV_DECAY;
        s->accented = accent ? 1u : 0u;
        s->f_ctrl_left = 0u;
    }
    s->gate = 1u;
    s->idle = 0u;
}

void acid303_note_off(acid303_t *s) {
    s->gate = 0u;
    s->amp_stage = ACID_ENV_RELEASE;
}

static void slide_step(acid303_t *s) {
    if (!s->sliding) return;
    if (s->phase_inc == s->slide_target_inc) {
        s->sliding = 0u;
        s->f_freq = s->f_target_freq;
        return;
    }
    if (s->phase_inc < s->slide_target_inc) {
        uint32_t d = s->slide_target_inc - s->phase_inc;
        uint32_t st = d / 2646u;
        if (!st) st = 1u;
        s->phase_inc += st;
        if (s->phase_inc > s->slide_target_inc) s->phase_inc = s->slide_target_inc;
    } else {
        uint32_t d = s->phase_inc - s->slide_target_inc;
        uint32_t st = d / 2646u;
        if (!st) st = 1u;
        s->phase_inc -= st;
        if (s->phase_inc < s->slide_target_inc) s->phase_inc = s->slide_target_inc;
    }
    s->f_freq = inc_to_hz(s->phase_inc);
}

static void envelope_step(acid303_t *s) {
    const float rc1 = 0.9924690f;
    const float rc2 = 0.9984890f;
    float dc = decay_coeff(s);
    float release = s->accented ? 0.9995466f : 0.9775794f;

    s->f_env *= dc;
    s->f_env_rc1 = rc1 * s->f_env_rc1 + (1.0f - rc1) * s->f_env;
    if (s->accented)
        s->f_env_rc2 = rc2 * s->f_env_rc2 + (1.0f - rc2) * s->f_env;
    else
        s->f_env_rc2 *= rc2;

    if (s->gate)
        s->f_amp *= 0.99998156f;
    else
        s->f_amp *= release;

    if (s->f_env < 0.0000005f) s->f_env = 0.0f;
    if (s->f_amp < 0.0000305f) {
        s->f_amp = 0.0f;
        s->amp = 0;
        s->amp_stage = ACID_ENV_OFF;
        if (!s->gate) s->idle = 1u;
    } else {
        s->amp = (int32_t)(fclamp(s->f_amp, 0.0f, 1.0f) * 32767.0f);
    }
    s->env = (int32_t)(fclamp(s->f_env, 0.0f, 1.0f) * 32767.0f);
    s->env_rc = s->env;
    s->accent_env = s->accented ?
        (int32_t)(fclamp(s->f_env_rc2, 0.0f, 0.999f) * 32767.0f) : 0;
    s->accent_sweep = s->accent_env;
}

static float blep(float t, float dt) {
    float x;
    if (dt <= 0.0f) return 0.0f;
    if (t < dt) {
        x = t / dt;
        return x + x - x * x - 1.0f;
    }
    if (t > 1.0f - dt) {
        x = (t - 1.0f) / dt;
        return x * x + x + x + 1.0f;
    }
    return 0.0f;
}

static float osc(acid303_t *s, float inc) {
    float p = s->f_phase;
    float y, t;
    if (!s->square) {
        t = p + 0.5f;
        if (t >= 1.0f) t -= 1.0f;
        y = -(t + t - 1.0f - blep(t, inc));
    } else {
        y = p < 0.5f ? 1.0f : -1.0f;
        y += blep(p, inc);
        t = p + 0.5f;
        if (t >= 1.0f) t -= 1.0f;
        y -= blep(t, inc);
        y *= -0.5f;
    }
    p += inc;
    if (p >= 1.0f) p -= 1.0f;
    s->f_phase = p;
    return y;
}

static float modulated_cutoff(acid303_t *s) {
    float c = (float)(s->cutoff > 32767u ? 32767u : s->cutoff) * (1.0f / 32767.0f);
    float e = (float)(s->env_mod > 32767u ? 32767u : s->env_mod) * (1.0f / 32767.0f);
    float slo = 3.773996325f * e + 0.736965594f;
    float shi = 4.194548788f * e + 0.864344901f;
    float scaler = (1.0f - c) * slo + c * shi;
    float offset = 0.048292931f * c + 0.294391201f;
    float a = (float)(s->accent > 32767u ? 32767u : s->accent) * (1.0f / 32767.0f);
    float fc = (float)acid303_cutoff_hz(s) *
        fexp2(scaler * (s->f_env_rc1 - offset) +
              (s->accented ? a * s->f_env_rc2 : 0.0f));

    if (s->lfo_amount) {
        int16_t lv = acid303_lfo_value(s);
        float d = (float)lv * (float)s->lfo_amount *
                  (1600.0f / (32768.0f * 127.0f));
        fc += d;
    }
    return fclamp(fc, 200.0f, 20000.0f);
}

static void update_coeff_target(acid303_t *s) {
    float fc = modulated_cutoff(s);
    float r = resonance_skew(s);
    float fx = fc * (0.707106781f / FSO);
    float b0, k, g2;

    b0 = (0.00045522346f + 6.1922189f * fx) /
         (1.0f + 12.358354f * fx + 4.4156345f * fx * fx);
    k = fx * (fx * (fx * (fx * (fx * (fx + 7198.6997f) - 5837.7917f) -
        476.47308f) + 614.95611f) + 213.87126f) + 16.998792f;
    g2 = 2.0f * ((k * (1.0f / 17.0f) - 1.0f) * r + 1.0f) * (1.0f + r);
    k *= r;

    if (!s->coeff_valid) {
        s->f_b0 = b0;
        s->f_k = k;
        s->f_g2 = g2;
        s->coeff_valid = 1u;
    }
    s->f_b0_target = b0;
    s->f_k_target = k;
    s->f_g2_target = g2;
    s->f_b0_step = (b0 - s->f_b0) * (1.0f / (float)CTRL);
    s->f_k_step = (k - s->f_k) * (1.0f / (float)CTRL);
    s->f_g2_step = (g2 - s->f_g2) * (1.0f / (float)CTRL);
    s->f_ctrl_left = CTRL;
    s->f_last_cutoff = fc;
    s->f_last_reso = r;
    s->f_last_envmod = (float)s->env_mod;
}

static void advance_coeffs(acid303_t *s) {
    if (!s->f_ctrl_left) update_coeff_target(s);
    s->f_b0 += s->f_b0_step;
    s->f_k += s->f_k_step;
    s->f_g2 += s->f_g2_step;
    if (--s->f_ctrl_left == 0u) {
        s->f_b0 = s->f_b0_target;
        s->f_k = s->f_k_target;
        s->f_g2 = s->f_g2_target;
    }
    s->coeff_b0 = (int32_t)(s->f_b0 * 32768.0f);
    s->coeff_k = (int32_t)(s->f_k * 32768.0f);
    s->coeff_g = (int32_t)(s->f_g2 * 32768.0f);
}

static float ladder_substep(acid303_t *s, float in) {
    const float hp1_b0 = 0.998417963f, hp1_a1 = 0.996835926f;
    const float fb_b0 = 0.994685600f, fb_a1 = 0.989371200f;
    float hp = hp1_b0 * (in - s->f_hp1_x) + hp1_a1 * s->f_hp1_y;
    float t;

    s->f_hp1_x = in;
    s->f_hp1_y = hp;
    t = s->f_k * s->f_y4;
    s->f_fb_y = fb_b0 * (t - s->f_fb_x) + fb_a1 * s->f_fb_y;
    s->f_fb_x = t;
    t = hp - s->f_fb_y;
    s->f_y1 += 2.0f * s->f_b0 * (t - s->f_y1 + s->f_y2);
    s->f_y2 += s->f_b0 * (s->f_y1 - 2.0f * s->f_y2 + s->f_y3);
    s->f_y3 += s->f_b0 * (s->f_y2 - 2.0f * s->f_y3 + s->f_y4);
    s->f_y4 += s->f_b0 * (s->f_y3 - 2.0f * s->f_y4);
    return s->f_g2 * s->f_y4;
}

static float post_highpass(acid303_t *s, float x) {
    const float b0 = 0.998281354f, a1 = 0.996562707f;
    float y = b0 * (x - s->f_post_x) + a1 * s->f_post_y;
    s->f_post_x = x;
    s->f_post_y = y;
    return y;
}

int32_t acid303_process(acid303_t *s) {
    float inc, o0, o1, y, target_amp, out;

    slide_step(s);
    if (s->lfo_amount)
        s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u;
    envelope_step(s);
    if (s->idle && !s->gate && s->amp_stage == ACID_ENV_OFF) return 0;
    advance_coeffs(s);

    inc = s->f_freq / FSO;
    if (inc > 0.45f) inc = 0.45f;
    if (inc < 0.0f) inc = 0.0f;
    o0 = ladder_substep(s, osc(s, inc));
    o1 = ladder_substep(s, osc(s, inc));
    y = post_highpass(s, 0.5f * (o0 + o1));

    target_amp = s->f_amp;
    if (s->gate) {
        target_amp += 0.45f * s->f_env;
        if (s->accented && s->accent)
            target_amp += 4.0f * ((float)s->accent * (1.0f / 32767.0f)) * s->f_env;
    }
    s->f_amp_smooth = 0.971906f * s->f_amp_smooth + 0.028094f * target_amp;
    out = y * s->f_amp_smooth * 0.38f;
    if (s->drive) {
        float g = 1.0f + 7.0f * ((float)s->drive * (1.0f / 32767.0f));
        out = ftanh(out * g) / g;
    }

    s->phase = (uint32_t)(s->f_phase * 4294967040.0f);
    s->lp1 = q15_state(s->f_y1);
    s->lp2 = q15_state(s->f_y2);
    s->lp3 = q15_state(s->f_y3);
    s->lp4 = q15_state(s->f_y4);
    s->output_hp_x = q15_state(s->f_hp1_x);
    s->output_hp_y = q15_state(s->f_hp1_y);
    s->post_hp_x = q15_state(s->f_post_x);
    s->post_hp_y = q15_state(s->f_post_y);
    s->gain_smooth1 = (int32_t)(fclamp(s->f_amp_smooth, 0.0f, 6.0f) * 32768.0f);
    s->gain_smooth2 = s->gain_smooth1;
    return (int32_t)q15_from_float(out);
}
