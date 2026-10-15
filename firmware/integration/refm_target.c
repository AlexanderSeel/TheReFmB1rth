// SPDX-License-Identifier: GPL-3.0-only
#include "refm_target.h"
#include "../proto/groovebox_midi.h"
#include <string.h>

#ifdef REFM_FELUCCA_PLATFORM
/* Physical beta.12 diagnostic oscillator.
 * It is generated directly at the target boundary and therefore bypasses the
 * groovebox, 303, drums, mixer and FX completely. */
static int32_t diag_tri(uint32_t p) {
    uint32_t q = p >> 16;
    return q < 32768u ? (int32_t)q * 2 - 32768 : 98302 - (int32_t)q * 2;
}

static int32_t diag_sineish(uint32_t p) {
    int32_t t = diag_tri(p);
    int32_t a = t < 0 ? -t : t;
    return (t * (49152 - (a >> 1))) >> 15;
}

#if REFM_DIAG_EXTENDED_303
static int32_t diag_limit_q15(int32_t x) {
    if (x < -32768) return -32768;
    if (x > 32767) return 32767;
    return x;
}
#endif
#endif

static uint8_t channel_data_len(uint8_t status) {
    switch (status & 0xF0u) {
        case 0xC0u:
        case 0xD0u:
            return 1u;
        case 0x80u:
        case 0x90u:
        case 0xA0u:
        case 0xB0u:
        case 0xE0u:
            return 2u;
        default:
            return 0u;
    }
}

#ifdef REFM_FELUCCA_PLATFORM
static void diag_prepare_acid(acid303_t *acid) {
    acid303_init(acid);
    acid->cutoff = 32767u;
    acid->resonance = 0u;
    acid->env_mod = 0u;
    acid->drive = 0u;
    acid->square = 0u;
}

static void diag_prepare_stage(refm_target_t *target) {
    groovebox_t *g = &target->groovebox;
    target->diag_phase = 0u;
    switch (target->diag_stage) {
    case 0u:
        /* Direct output-boundary reference tone. */
        break;
    case 1u:
        /* 808 BD direct from drum engine, mixer bypassed. */
        drum_machine_init(&g->drum[0], DRUM_MODEL_808);
        break;
    case 2u:
        /* Identical 808 BD path, now through the shared mixer. */
        drum_machine_init(&g->drum[0], DRUM_MODEL_808);
        mixer_fx_init(&g->mixer);
        break;
    case 3u:
        /* C2 SAW after the complete 303 engine/ladder, mixer bypassed. */
        diag_prepare_acid(&g->acid[0]);
        break;
    case 4u:
        /* Same C2 SAW engine output through the shared mixer. */
        diag_prepare_acid(&g->acid[0]);
        mixer_fx_init(&g->mixer);
        break;
#if REFM_DIAG_EXTENDED_303
    case 5u:
        /* Internal tap: pre-ladder high-pass state after a normal 303 process. */
        diag_prepare_acid(&g->acid[0]);
        break;
    case 6u:
        /* Internal tap: fourth ladder state after a normal 303 process. */
        diag_prepare_acid(&g->acid[0]);
        break;
    case 7u:
        /* Internal tap: post-ladder high-pass state before VCA/drive. */
        diag_prepare_acid(&g->acid[0]);
        break;
#endif
    default:
        break;
    }
}

static void diag_finish(refm_target_t *target) {
    groovebox_init(&target->groovebox, target->diag_bpm);
    groovebox_midi_init(&target->midi);
    target->midi_status = 0u;
    target->midi_needed = 0u;
    target->midi_have = 0u;
}

static void diag_render(refm_target_t *target, int32_t *left, int32_t *right) {
    const uint32_t tone_inc = 97391549u; /* 1 kHz Q32 phase increment at 44.1 kHz */
    const uint32_t half_second = REFM_TARGET_SAMPLE_RATE / 2u;
    groovebox_t *g = &target->groovebox;
    int32_t l = 0, r = 0;

    if (target->diag_stage_frame == 0u)
        diag_prepare_stage(target);

    switch (target->diag_stage) {
    case 0u:
        target->diag_phase += tone_inc;
        l = r = diag_sineish(target->diag_phase) >> 2; /* ~-12 dBFS */
        break;
    case 1u: {
        int32_t s;
        if ((target->diag_stage_frame % half_second) == 0u)
            drum_machine_trigger(&g->drum[0], DRUM_BD, 112u);
        s = drum_machine_process(&g->drum[0]);
        l = r = s >> 2; /* match mixer's -12 dB summing reference */
        break;
    }
    case 2u: {
        int16_t in[MIX_TRACKS] = {0, 0, 0, 0};
        int16_t ml = 0, mr = 0;
        if ((target->diag_stage_frame % half_second) == 0u)
            drum_machine_trigger(&g->drum[0], DRUM_BD, 112u);
        in[2] = drum_machine_process(&g->drum[0]);
        mixer_fx_process(&g->mixer, in, &ml, &mr);
        l = ml; r = mr;
        break;
    }
    case 3u: {
        int32_t s;
        if ((target->diag_stage_frame % REFM_TARGET_SAMPLE_RATE) == 0u)
            acid303_set_note(&g->acid[0], 36u, 0u, 0u); /* C2 */
        s = acid303_process(&g->acid[0]);
        l = r = s >> 2; /* match mixer's -12 dB summing reference */
        break;
    }
    case 4u: {
        int16_t in[MIX_TRACKS] = {0, 0, 0, 0};
        int16_t ml = 0, mr = 0;
        if ((target->diag_stage_frame % REFM_TARGET_SAMPLE_RATE) == 0u)
            acid303_set_note(&g->acid[0], 36u, 0u, 0u); /* C2 */
        in[0] = (int16_t)acid303_process(&g->acid[0]);
        mixer_fx_process(&g->mixer, in, &ml, &mr);
        l = ml; r = mr;
        break;
    }
#if REFM_DIAG_EXTENDED_303
    case 5u:
        if ((target->diag_stage_frame % REFM_TARGET_SAMPLE_RATE) == 0u)
            acid303_set_note(&g->acid[0], 36u, 0u, 0u);
        (void)acid303_process(&g->acid[0]);
        l = r = diag_limit_q15(g->acid[0].output_hp_y >> 2);
        break;
    case 6u:
        if ((target->diag_stage_frame % REFM_TARGET_SAMPLE_RATE) == 0u)
            acid303_set_note(&g->acid[0], 36u, 0u, 0u);
        (void)acid303_process(&g->acid[0]);
        l = r = diag_limit_q15(g->acid[0].lp4 >> 3);
        break;
    case 7u:
        if ((target->diag_stage_frame % REFM_TARGET_SAMPLE_RATE) == 0u)
            acid303_set_note(&g->acid[0], 36u, 0u, 0u);
        (void)acid303_process(&g->acid[0]);
        l = r = diag_limit_q15(g->acid[0].post_hp_y >> 2);
        break;
#endif
    default:
        break;
    }

    *left = l;
    *right = r;

    target->diag_stage_frame++;
    target->diag_frames_remaining--;
    if (target->diag_stage_frame >= REFM_DIAG_STAGE_FRAMES) {
        target->diag_stage_frame = 0u;
        target->diag_stage++;
        if (!target->diag_frames_remaining)
            diag_finish(target);
    }
}
#endif

void refm_target_init(refm_target_t *target, uint16_t bpm) {
    memset(target, 0, sizeof(*target));
    groovebox_init(&target->groovebox, bpm);
    groovebox_midi_init(&target->midi);
#ifdef REFM_FELUCCA_PLATFORM
    target->diag_bpm = bpm;
    target->diag_frames_remaining = REFM_DIAG_TOTAL_FRAMES;
#endif
}

void refm_target_set_external_clock(refm_target_t *target, uint8_t external) {
    groovebox_set_external_clock(&target->groovebox, external ? 1u : 0u);
}

void refm_target_render_q15(refm_target_t *target, int32_t *interleaved_stereo, uint32_t frames) {
    for (uint32_t i = 0; i < frames; ++i) {
#ifdef REFM_FELUCCA_PLATFORM
        if (target->diag_frames_remaining) {
            int32_t l = 0, r = 0;
            diag_render(target, &l, &r);
            interleaved_stereo[2u * i] = l;
            interleaved_stereo[2u * i + 1u] = r;
            continue;
        }
#endif
        {
            int16_t l = 0, r = 0;
            groovebox_process(&target->groovebox, &l, &r);
            interleaved_stereo[2u * i] = l;
            interleaved_stereo[2u * i + 1u] = r;
        }
    }
}

void refm_target_midi_byte(refm_target_t *target, uint8_t byte) {
    if (byte >= 0xF8u) {
        groovebox_feed_midi_realtime(&target->groovebox, byte);
        return;
    }
    if (byte & 0x80u) {
        if (byte >= 0xF0u) {
            target->midi_status = 0u;
            target->midi_needed = 0u;
            target->midi_have = 0u;
            return;
        }
        target->midi_status = byte;
        target->midi_needed = channel_data_len(byte);
        target->midi_have = 0u;
        return;
    }
    if (!target->midi_status || !target->midi_needed) return;
    target->midi_data[target->midi_have++] = byte & 0x7Fu;
    if (target->midi_have >= target->midi_needed) {
        uint8_t d1 = target->midi_data[0];
        uint8_t d2 = target->midi_needed > 1u ? target->midi_data[1] : 0u;
        groovebox_handle_channel_midi(&target->groovebox, &target->midi, target->midi_status, d1, d2);
        target->midi_have = 0u;
    }
}

void refm_target_panic(refm_target_t *target) {
    acid303_note_off(&target->groovebox.acid[0]);
    acid303_note_off(&target->groovebox.acid[1]);
    for (uint8_t ch = 0u; ch < 16u; ++ch)
        groovebox_handle_channel_midi(&target->groovebox, &target->midi, (uint8_t)(0xB0u | ch), 123u, 0u);
}

int refm_target_save(const refm_target_t *target, uint8_t *out, size_t capacity, size_t *written) {
    return groovebox_save(&target->groovebox, out, capacity, written);
}

int refm_target_load(refm_target_t *target, const uint8_t *blob, size_t blob_len) {
    int rc = groovebox_load(&target->groovebox, blob, blob_len);
    if (rc == PROJECT_OK) groovebox_midi_init(&target->midi);
    return rc;
}
