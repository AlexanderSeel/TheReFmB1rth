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
    if (octave > 0u) {
        if (octave >= 8u) octave = 7u;
        inc <<= octave;
    }
    return inc;
}

/* Cheap bounded transistor/diode-style non-linearity. Input/output Q15-ish. */
static int32_t sat_q15(int32_t x) {
    int32_t ax = x < 0 ? -x : x;
    if (ax >= 49152) return x < 0 ? -32767 : 32767;
    /* y ~= x - x^3/3, scaled and slightly driven. */
    int64_t x2 = ((int64_t)x * x) >> 15;
    int64_t x3 = (x2 * x) >> 15;
    return clamp32((int64_t)x - x3 / 3, -32767, 32767);
}

static uint32_t decay_tau_samples(uint16_t decay, uint8_t accented) {
    if (accented) return (SR * 200u) / 1000u; /* stock accent MEG is fixed around 200 ms */
    /* Hardware control range: approximately 200 ms .. 2 s. */
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
    /* Pot-like non-linear mapping, roughly 250 Hz .. 2.4 kHz before envelope/accent. */
    uint32_t n = s->cutoff > 32767u ? 32767u : s->cutoff;
    uint32_t curved = (n * n) >> 15;
    return (uint16_t)(250u + (curved * 2150u) / 32767u);
}

static int32_t filter_coeff_q15(uint32_t hz) {
    /* g ~= 2*pi*f / (Fs + 2*pi*f), integer rational approximation. */
    if (hz > 12000u) hz = 12000u;
    uint64_t w = (uint64_t)6283u * hz;
    uint64_t d = (uint64_t)SR * 1000u + w;
    return (int32_t)((w * 32767u) / d);
}

void acid303_init(acid303_t *s) {
    memset(s, 0, sizeof(*s));
    s->cutoff = 11500u;
    s->resonance = 20500u;
    s->env_mod = 22500u;
    s->decay = 13500u;
    s->accent = 20000u;
    s->drive = 2500u;
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
        /* Authentic slide is legato: glide pitch, do not retrigger MEG/VCA. */
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
            /* Accent sweep capacitor does not fully discharge between accents. */
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
    if (!s->square) return (int32_t)(s->phase >> 16) - 32768;
    /* The real square is derived from the saw and is noticeably asymmetric.
       ~66% duty is a much closer starting point than a textbook 50% square. */
    return (s->phase < 0xAAAAAAAAu) ? 24576 : -32768;
}

static void slide_step(acid303_t *s) {
    if (!s->sliding) return;
    int64_t d = (int64_t)s->slide_target_inc - (int64_t)s->phase_inc;
    /* ~60 ms audible glide: 1/512 per sample reaches >99% in roughly 54 ms. */
    int64_t step = d / 512;
    if (!step && d) step = d > 0 ? 1 : -1;
    s->phase_inc = (uint32_t)((int64_t)s->phase_inc + step);
    if (d < 3 && d > -3) { s->phase_inc = s->slide_target_inc; s->sliding = 0u; }
}

static void envelope_step(acid303_t *s) {
    decay_env(&s->env, decay_tau_samples(s->decay, s->accented));
    decay_env(&s->accent_env, (SR * 200u) / 1000u);

    /* Fixed VCA contour: fast attack is implicit at trigger, long ~3.5 s decay.
       Once the gate falls, close faster but not instantaneously. */
    decay_env(&s->amp, s->gate ? (SR * 3500u) / 1000u : (SR * 60u) / 1000u);
    if (!s->amp) s->amp_stage = ACID_ENV_OFF;

    /* Accent sweep capacitor: resonance makes the sweep hang longer. */
    uint32_t base = 8000u + ((uint32_t)s->resonance * 18000u) / 32767u;
    decay_env(&s->accent_sweep, base);
}

static int32_t diode_ladder(acid303_t *s, int32_t x, uint32_t cutoff_hz) {
    int32_t g = filter_coeff_q15(cutoff_hz);
    int32_t res = (int32_t)((uint32_t)s->resonance * 30000u / 32767u);

    /* The 303 resonance path is AC-coupled/high-passed. This keeps high-resonance
       bass from behaving like a generic Moog-style feedback ladder. */
    int32_t hp = s->lp3 - s->resonance_hp_lp;
    s->resonance_hp_lp += (hp * 140) >> 15; /* fixed very-low feedback HP pole */
    int32_t feedback = (int32_t)(((int64_t)hp * res) >> 15);
    int32_t u = sat_q15(x - feedback);

    /* Three effective low-pass integrations -> ~18 dB/oct. Per-stage saturation
       approximates the loading/non-linearity of the diode ladder. */
    int32_t d1 = sat_q15(u - s->lp1);
    s->lp1 += (int32_t)(((int64_t)d1 * g) >> 15);
    int32_t d2 = sat_q15(s->lp1 - s->lp2);
    s->lp2 += (int32_t)(((int64_t)d2 * g) >> 15);
    int32_t d3 = sat_q15(s->lp2 - s->lp3);
    s->lp3 += (int32_t)(((int64_t)d3 * g) >> 15);
    return s->lp3;
}

static int32_t output_highpass(acid303_t *s, int32_t x) {
    /* Fixed output coupling capacitor/DC blocker, approximately 20 Hz. */
    int32_t y = x - s->output_hp_x + (int32_t)(((int64_t)s->output_hp_y * 32675) >> 15);
    s->output_hp_x = x;
    s->output_hp_y = y;
    return y;
}

int32_t acid303_process(acid303_t *s) {
    slide_step(s);
    s->lfo_phase += 4870u + (uint32_t)s->lfo_rate * 15310u;
    envelope_step(s);

    int32_t osc = oscillator(s);
    int16_t lfo = acid303_lfo_value(s);
    uint32_t fc = acid303_cutoff_hz(s);

    /* Main envelope can sweep several kHz above the base cutoff. */
    fc += (uint32_t)(((int64_t)s->env * s->env_mod * 5200) >> 30);

    /* Accent raises cutoff through its own slowly discharging sweep circuit and
       gets stronger as resonance rises, which is key to repeated-accent 'wow'. */
    if (s->accented || s->accent_sweep > 0) {
        int32_t sweep = (int32_t)(((int64_t)s->accent_sweep * s->accent) >> 15);
        sweep = (int32_t)(((int64_t)sweep * (8192 + s->resonance)) >> 15);
        if (sweep > 0) fc += (uint32_t)(((int64_t)sweep * 4500) >> 15);
    }

    /* Non-stock extension: four selectable LFO shapes really modulate the filter.
       Amount 0 gives the authentic stock 303 path. */
    if (s->lfo_amount) {
        int32_t delta = (int32_t)(((int64_t)lfo * s->lfo_amount * 1800) >> 22);
        int32_t m = (int32_t)fc + delta;
        fc = (uint32_t)(m < 80 ? 80 : m > 12000 ? 12000 : m);
    }
    if (fc > 12000u) fc = 12000u;

    int32_t y = diode_ladder(s, osc, fc);

    int32_t gain = s->amp;
    if (s->accented || s->accent_env > 0) {
        int32_t ag = (int32_t)(((int64_t)s->accent_env * s->accent) >> 16);
        gain = clamp32((int64_t)gain + ag, 0, 42000);
    }
    y = (int32_t)(((int64_t)y * gain) >> 15);

    /* Mild post-filter/VCA saturation; drive=0 remains close to the stock path. */
    if (s->drive) {
        int32_t k = 32768 + s->drive;
        y = sat_q15((int32_t)(((int64_t)y * k) >> 15));
    }
    y = output_highpass(s, y);
    return clamp32(y, -32768, 32767);
}
