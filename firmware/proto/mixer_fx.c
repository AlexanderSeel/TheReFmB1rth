// SPDX-License-Identifier: GPL-3.0-only
#include "mixer_fx.h"
#include <string.h>

#define MIX_MASTER_HEADROOM_Q15 8192 /* -12 dB: four full-scale tracks sum cleanly */
/* Bound before the master-headroom multiply. 262140 * 8192 stays below
 * INT32_MAX; anything above this would clamp to full Q15 after the -12 dB
 * master stage anyway. */
#define MIX_PREMASTER_LIMIT 262140

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi) { return v<lo?lo:v>hi?hi:v; }
static int16_t clip16(int32_t v) { return (int16_t)clamp32(v,-32768,32767); }
/* All hot-path operands passed here are bounded to <= 65536 in magnitude.
 * The worst product (65536 * 32767) remains inside signed 32 bit. This avoids
 * target/compiler 64-bit arithmetic in the shared audio path. */
static int32_t mul_q15_32(int32_t a, int32_t b) { return (a * b) >> 15; }

void mixer_fx_init(mixer_fx_t *m) {
    memset(m, 0, sizeof(*m));
    /* Keep track faders at unity and reserve summing headroom once at the
       master bus. That is easier to reason about than hiding attenuation in
       selected tracks and guarantees the default four-track mix stays linear. */
    for (unsigned i=0; i<MIX_TRACKS; ++i) {
        m->track[i].level = 32767;
        m->track[i].pan = 0;
    }
#if defined(__EMSCRIPTEN__)
    m->delay_len = 11025u; /* 250 ms */
    m->delay_feedback = 15000;
    m->delay_mix = 7000;
    m->reverb_feedback = 29400;
    m->reverb_mix = 6000;
    m->reverb_damp = 7000;
#else
    m->delay_len = 1102u;
    m->delay_feedback = 14000;
    m->delay_mix = 7000;
    m->reverb_feedback = 24500;
    m->reverb_mix = 6000;
    m->reverb_damp = 9000;
#endif
    /* DIST, COMP and PCF are performance effects. Fresh firmware is an
       intentionally transparent mixer, not an always-on mastering chain. */
    m->drive = 0;
    m->compressor_threshold = 32767;
    m->filter_cutoff = 32767;
}

void mixer_fx_set_delay(mixer_fx_t *m, uint16_t samples, int16_t feedback, int16_t mix) {
    if (samples == 0u) samples = 1u;
    if (samples > MIX_DELAY_MAX) samples = MIX_DELAY_MAX;
    m->delay_len = samples;
    m->delay_feedback = (int16_t)clamp32(feedback, 0, 31000);
    m->delay_mix = (int16_t)clamp32(mix, 0, 32767);
    if (m->delay_pos >= samples) m->delay_pos = 0u;
}

void mixer_fx_set_reverb(mixer_fx_t *m, int16_t feedback, int16_t mix, int16_t damp) {
    m->reverb_feedback = (int16_t)clamp32(feedback, 0, MIX_REVERB_FEEDBACK_MAX);
    m->reverb_mix = (int16_t)clamp32(mix, 0, 32767);
    m->reverb_damp = (int16_t)clamp32(damp, 256, 30000);
}

static int32_t saturate(int32_t x, int16_t drive) {
    if (drive <= 0) return clamp32(x, -32768, 32767);
    /* |x| <= 32768 and gain <= 65535 => product <= 2147450880. */
    int32_t y = (x * (32768 + (int32_t)drive)) >> 15;
    int32_t v = clamp32(y, -65536, 65535);
    int32_t a = v < 0 ? -v : v;
    if (a > 24576) {
        int32_t shaped = 24576 + (a - 24576) / 4;
        v = v < 0 ? -shaped : shaped;
    }
    return clamp32(v, -32768, 32767);
}

static int32_t compress(int32_t x, int16_t threshold) {
    int32_t a = x < 0 ? -x : x;
    if (threshold >= 32767 || a <= threshold) return x;
    int32_t v = threshold + (a - threshold) / 4;
    return x < 0 ? -v : v;
}

void mixer_fx_process(mixer_fx_t *m, const int16_t input[MIX_TRACKS], int16_t *out_l, int16_t *out_r) {
    int any_solo = 0;
    uint16_t delay_len = m->delay_len;
    int32_t left = 0, right = 0, delay_send_l = 0, delay_send_r = 0;
    int32_t reverb_send_l = 0, reverb_send_r = 0;

    if (delay_len == 0u) delay_len = 1u;
    if (delay_len > MIX_DELAY_MAX) delay_len = MIX_DELAY_MAX;
    if (m->delay_len != delay_len) m->delay_len = delay_len;
    if (m->delay_pos >= delay_len) m->delay_pos = 0u;
    if (m->reverb_pos_a >= MIX_REVERB_A) m->reverb_pos_a = 0u;
    if (m->reverb_pos_b >= MIX_REVERB_B) m->reverb_pos_b = 0u;

    for (unsigned i=0; i<MIX_TRACKS; ++i) if (m->track[i].solo) any_solo = 1;

    for (unsigned i=0; i<MIX_TRACKS; ++i) {
        const mixer_track_t *t = &m->track[i];
        int32_t s, lg, rg, l, r;
        if (t->mute || (any_solo && !t->solo)) continue;
        s = mul_q15_32((int32_t)input[i], t->level);
        lg = t->pan > 0 ? 32767 - t->pan : 32767;
        rg = t->pan < 0 ? 32767 + t->pan : 32767;
        l = mul_q15_32(s, lg);
        r = mul_q15_32(s, rg);
        left += l;
        right += r;
        delay_send_l += (l * (int32_t)t->delay_send) / 127;
        delay_send_r += (r * (int32_t)t->delay_send) / 127;
        reverb_send_l += (l * (int32_t)t->reverb_send) / 127;
        reverb_send_r += (r * (int32_t)t->reverb_send) / 127;
    }

    {
        int16_t dl = m->delay_l[m->delay_pos];
        int16_t dr = m->delay_r[m->delay_pos];
        int32_t write_l = delay_send_l + mul_q15_32(dl, m->delay_feedback);
        int32_t write_r = delay_send_r + mul_q15_32(dr, m->delay_feedback);
        m->delay_l[m->delay_pos] = clip16(write_l);
        m->delay_r[m->delay_pos] = clip16(write_r);
        m->delay_pos++;
        if (m->delay_pos >= delay_len) m->delay_pos = 0u;
        left += mul_q15_32(dl, m->delay_mix);
        right += mul_q15_32(dr, m->delay_mix);
    }

    {
        int32_t ra = m->reverb_a[m->reverb_pos_a];
        int32_t rb = m->reverb_b[m->reverb_pos_b];
        int32_t damp = clamp32(m->reverb_damp, 256, 30000);
        int32_t in_a = clamp32(reverb_send_l + (reverb_send_r >> 1), -65536, 65535);
        int32_t in_b = clamp32(reverb_send_r + (reverb_send_l >> 1), -65536, 65535);

        m->reverb_lp_a += mul_q15_32(ra - m->reverb_lp_a, damp);
        m->reverb_lp_b += mul_q15_32(rb - m->reverb_lp_b, damp);
        m->reverb_a[m->reverb_pos_a] = clip16(in_a + mul_q15_32(m->reverb_lp_b, m->reverb_feedback));
        m->reverb_b[m->reverb_pos_b] = clip16(in_b + mul_q15_32(m->reverb_lp_a, m->reverb_feedback));
        if (++m->reverb_pos_a >= MIX_REVERB_A) m->reverb_pos_a = 0u;
        if (++m->reverb_pos_b >= MIX_REVERB_B) m->reverb_pos_b = 0u;

        left += mul_q15_32(ra + (rb >> 1), m->reverb_mix);
        right += mul_q15_32(rb + (ra >> 1), m->reverb_mix);
    }

    {
        /* Narrow only after the complete linear sum. Clamp to the largest value
         * that keeps the fixed -12 dB multiply strictly inside signed 32 bit. */
        int32_t prem_l = clamp32(left, -MIX_PREMASTER_LIMIT, MIX_PREMASTER_LIMIT);
        int32_t prem_r = clamp32(right, -MIX_PREMASTER_LIMIT, MIX_PREMASTER_LIMIT);
        int32_t l = clamp32((prem_l * MIX_MASTER_HEADROOM_Q15) >> 15, -32768, 32767);
        int32_t r = clamp32((prem_r * MIX_MASTER_HEADROOM_Q15) >> 15, -32768, 32767);
        l = saturate(l, m->drive);
        r = saturate(r, m->drive);
        l = compress(l, m->compressor_threshold);
        r = compress(r, m->compressor_threshold);

        if (m->filter_cutoff >= 32760) {
            /* PCF fully open means no hidden one-pole coloration. */
            m->filter_l = l;
            m->filter_r = r;
        } else {
            int32_t fc = clamp32(m->filter_cutoff, 256, 32767);
            m->filter_l += mul_q15_32(l - m->filter_l, fc);
            m->filter_r += mul_q15_32(r - m->filter_r, fc);
        }
        *out_l = clip16(m->filter_l);
        *out_r = clip16(m->filter_r);
    }
}
