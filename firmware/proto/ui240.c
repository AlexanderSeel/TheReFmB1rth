// SPDX-License-Identifier: GPL-3.0-only
#include "ui240.h"
#include <string.h>

static uint16_t add_u16(uint16_t v, int8_t delta, uint16_t step, uint16_t max)
{
    int32_t n = (int32_t)v + (int32_t)delta * (int32_t)step;
    if (n < 0) n = 0;
    if (n > max) n = max;
    return (uint16_t)n;
}

static int16_t add_s16(int16_t v, int8_t delta, int16_t step, int16_t lo, int16_t hi)
{
    int32_t n = (int32_t)v + (int32_t)delta * (int32_t)step;
    if (n < lo) n = lo;
    if (n > hi) n = hi;
    return (int16_t)n;
}

static uint8_t wrap_u8(uint8_t v, int8_t delta, uint8_t count)
{
    int n = (int)v + delta;
    while (n < 0) n += count;
    while (n >= count) n -= count;
    return (uint8_t)n;
}

void ui240_init(ui240_state_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->machine = UI240_MACHINE_ACID1;
    ui->fx_section = UI240_FX_PCF;
}

void ui240_set_page(ui240_state_t *ui, ui240_page_t page)
{
    if ((uint8_t)page < UI240_PAGE_COUNT) ui->page = (uint8_t)page;
}

void ui240_set_machine(ui240_state_t *ui, ui240_machine_t machine)
{
    if ((uint8_t)machine >= 4u) return;
    ui->machine = (uint8_t)machine;
    if (ui->machine >= UI240_MACHINE_808 &&
        (ui->page == UI240_ACID_MAIN || ui->page == UI240_ACID_STEP))
        ui->page = UI240_DRUM;
    if (ui->machine < UI240_MACHINE_808 && ui->page == UI240_DRUM)
        ui->page = UI240_ACID_MAIN;
}

void ui240_next_page(ui240_state_t *ui, int8_t delta)
{
    ui->page = wrap_u8(ui->page, delta, UI240_PAGE_COUNT);
}

void ui240_cycle_machine(ui240_state_t *ui, int8_t delta)
{
    ui240_set_machine(ui, (ui240_machine_t)wrap_u8(ui->machine, delta, 4u));
}

void ui240_select_step(ui240_state_t *ui, int8_t delta)
{
    ui->selected_step = wrap_u8(ui->selected_step, delta, 16u);
}

void ui240_select_lane(ui240_state_t *ui, int8_t delta)
{
    ui->selected_lane = wrap_u8(ui->selected_lane, delta, DRUM_VOICES);
}

void ui240_select_project(ui240_state_t *ui, int8_t delta)
{
    ui->project_slot = wrap_u8(ui->project_slot, delta, 4u);
}

void ui240_cycle_fx_section(ui240_state_t *ui, int8_t delta)
{
    ui->fx_section = wrap_u8(ui->fx_section, delta, UI240_FX_SECTION_COUNT);
}

void ui240_snapshot(const ui240_state_t *ui, const groovebox_t *g, ui240_view_t *out)
{
    unsigned m = ui->machine;
    memset(out, 0, sizeof(*out));
    out->page = ui->page;
    out->machine = ui->machine;
    out->pattern = g->current_pattern & 7u;
    out->playing = g->transport.playing ? 1u : 0u;
    out->playing_step = g->step & 15u;
    out->selected_step = ui->selected_step & 15u;
    out->selected_lane = ui->selected_lane < DRUM_VOICES ? ui->selected_lane : 0u;
    out->project_slot = ui->project_slot & 3u;
    out->fx_section = ui->fx_section % UI240_FX_SECTION_COUNT;
    out->bpm = g->transport.bpm;
    out->song_loop = g->song.loop ? 1u : 0u;
    out->song_length = g->song.length;

    if (m < UI240_MACHINE_808) {
        const acid303_t *s = &g->acid[m];
        const seq16_step_t *st = &g->acid_seq[m].step[out->selected_step];
        out->wave_square = s->square;
        out->acid_tune = s->tune;
        out->acid_accent = (uint8_t)(((uint32_t)s->accent * 100u) / 32767u);
        out->acid_cutoff_pct = (uint8_t)(((uint32_t)s->cutoff * 100u) / 32767u);
        out->acid_resonance_pct = (uint8_t)(((uint32_t)s->resonance * 100u) / 32767u);
        out->acid_env_pct = (uint8_t)(((uint32_t)s->env_mod * 100u) / 32767u);
        out->acid_decay_pct = (uint8_t)(((uint32_t)s->decay * 100u) / 32767u);
        out->accent = (st->flags & SEQ16_ACCENT) != 0;
        out->slide = (st->flags & SEQ16_SLIDE) != 0;
        out->step_probability = st->probability;
        if ((st->flags & SEQ16_TIE) && !(st->flags & SEQ16_GATE)) out->step_type = UI240_STEP_TIE;
        else if (st->flags & SEQ16_GATE) out->step_type = UI240_STEP_NOTE;
        else out->step_type = UI240_STEP_REST;

        if (ui->page == UI240_ACID_STEP) {
            out->p0 = st->note;
            out->p1 = out->step_type;
            out->p2 = (uint16_t)((out->accent ? 1u : 0u) | (out->slide ? 2u : 0u));
            out->p3 = st->probability;
        } else {
            out->p0 = s->cutoff;
            out->p1 = s->resonance;
            out->p2 = s->env_mod;
            out->p3 = s->decay;
        }
    } else {
        unsigned d = m - UI240_MACHINE_808;
        uint16_t bit = (uint16_t)(1u << out->selected_lane);
        out->drum_hits = g->drum_hits[d][out->selected_step];
        out->drum_accents = g->drum_accents[d][out->selected_step];
        out->drum_state = (out->drum_accents & bit) ? 2u : (out->drum_hits & bit) ? 1u : 0u;
        if (m == UI240_MACHINE_909) {
            uint32_t span = GROOVEBOX_909_FLAM_MAX_SAMPLES - GROOVEBOX_909_FLAM_MIN_SAMPLES;
            out->drum_flam = (g->drum_909_flams[out->selected_step] & bit) ? 1u : 0u;
            out->flam_width_pct = span ? (uint8_t)(((uint32_t)(g->flam_width_samples - GROOVEBOX_909_FLAM_MIN_SAMPLES) * 100u) / span) : 0u;
        }
    }

    if (ui->page == UI240_MIXER) {
        const mixer_track_t *t = &g->mixer.track[m & 3u];
        out->p0 = (uint16_t)t->level;
        out->p1 = (uint16_t)((int32_t)t->pan + 32767);
        out->p2 = t->delay_send;
        out->p3 = t->reverb_send;
        out->mute = t->mute;
        out->solo = t->solo;
    } else if (ui->page == UI240_FX) {
        switch (out->fx_section) {
        case UI240_FX_PCF:
            out->p0 = (uint16_t)g->mixer.filter_cutoff;
            out->p1 = 0u;
            out->p2 = 100u;
            out->p3 = 50u;
            break;
        case UI240_FX_DELAY:
            out->p0 = (uint16_t)(((uint32_t)g->mixer.delay_len * 16u + MIX_DELAY_MAX - 1u) / MIX_DELAY_MAX);
            if (out->p0 < 1u) out->p0 = 1u;
            if (out->p0 > 16u) out->p0 = 16u;
            out->p1 = 50u;
            out->p2 = (uint16_t)(((uint32_t)g->mixer.delay_feedback * 100u) / 31000u);
            out->p3 = (uint16_t)(((uint32_t)g->mixer.delay_mix * 100u) / 32767u);
            break;
        case UI240_FX_DIST:
            out->p0 = (uint16_t)(((uint32_t)g->mixer.drive * 100u) / 32767u);
            out->p1 = 50u;
            out->p2 = 0u;
            out->p3 = 0u;
            break;
        default:
            out->p0 = (uint16_t)(100u - ((uint32_t)g->mixer.compressor_threshold * 100u) / 32767u);
            out->p1 = 4u;
            out->p2 = 0u;
            out->p3 = 0u;
            break;
        }
    }
}

void ui240_edit_acid_main(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta)
{
    acid303_t *a;
    if (ui->machine >= UI240_MACHINE_808) return;
    a = &g->acid[ui->machine];
    if (slot == 0u) a->cutoff = add_u16(a->cutoff, delta, 768u, 32767u);
    else if (slot == 1u) a->resonance = add_u16(a->resonance, delta, 768u, 32767u);
    else if (slot == 2u) a->env_mod = add_u16(a->env_mod, delta, 768u, 32767u);
    else if (slot == 3u) a->decay = add_u16(a->decay, delta, 768u, 32767u);
}

void ui240_edit_acid_step(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta)
{
    seq16_step_t *st;
    int n;
    if (ui->machine >= UI240_MACHINE_808) return;
    st = &g->acid_seq[ui->machine].step[ui->selected_step & 15u];
    if (slot == 0u) {
        n = (int)st->note + delta;
        if (n < 24) n = 24;
        if (n > 96) n = 96;
        st->note = (uint8_t)n;
    } else if (slot == 1u) {
        int type = (st->flags & SEQ16_TIE) && !(st->flags & SEQ16_GATE) ? UI240_STEP_TIE :
                   (st->flags & SEQ16_GATE) ? UI240_STEP_NOTE : UI240_STEP_REST;
        type += delta > 0 ? 1 : -1;
        if (type < UI240_STEP_REST) type = UI240_STEP_TIE;
        if (type > UI240_STEP_TIE) type = UI240_STEP_REST;
        ui240_set_acid_step_type(ui, g, (ui240_step_type_t)type);
    } else if (slot == 2u) {
        uint8_t flags = (uint8_t)(((st->flags & SEQ16_ACCENT) ? 1u : 0u) |
                                  ((st->flags & SEQ16_SLIDE) ? 2u : 0u));
        flags = wrap_u8(flags, delta > 0 ? 1 : -1, 4u);
        st->flags &= (uint8_t)~(SEQ16_ACCENT | SEQ16_SLIDE);
        if (flags & 1u) st->flags |= SEQ16_ACCENT;
        if (flags & 2u) st->flags |= SEQ16_SLIDE;
    } else if (slot == 3u) {
        n = (int)st->probability + (delta > 0 ? 5 : -5);
        if (n < 5) n = 5;
        if (n > 100) n = 100;
        st->probability = (uint8_t)n;
    }
}

void ui240_set_acid_step_type(ui240_state_t *ui, groovebox_t *g, ui240_step_type_t type)
{
    seq16_step_t *st;
    if (ui->machine >= UI240_MACHINE_808) return;
    st = &g->acid_seq[ui->machine].step[ui->selected_step & 15u];
    st->flags &= (uint8_t)~(SEQ16_GATE | SEQ16_TIE);
    if (type == UI240_STEP_NOTE) st->flags |= SEQ16_GATE;
    else if (type == UI240_STEP_TIE) st->flags |= SEQ16_TIE;
}

void ui240_toggle_acid_flag(ui240_state_t *ui, groovebox_t *g, uint8_t flag)
{
    seq16_step_t *st;
    if (ui->machine >= UI240_MACHINE_808) return;
    if (flag != SEQ16_ACCENT && flag != SEQ16_SLIDE) return;
    st = &g->acid_seq[ui->machine].step[ui->selected_step & 15u];
    st->flags ^= flag;
}

void ui240_toggle_909_flam(ui240_state_t *ui, groovebox_t *g)
{
    uint16_t bit;
    if (ui->machine != UI240_MACHINE_909) return;
    bit = (uint16_t)(1u << ui->selected_lane);
    if (!(g->drum_hits[1][ui->selected_step] & bit)) return;
    g->drum_909_flams[ui->selected_step] ^= bit;
}

void ui240_edit_drum(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta)
{
    unsigned d;
    uint16_t bit;
    uint8_t state;
    if (ui->machine < UI240_MACHINE_808) return;
    d = ui->machine - UI240_MACHINE_808;
    if (slot == 0u) {
        ui240_select_lane(ui, delta > 0 ? 1 : -1);
        return;
    }
    if (slot == 2u && ui->machine == UI240_MACHINE_909) {
        ui240_toggle_909_flam(ui, g);
        return;
    }
    if (slot == 3u && ui->machine == UI240_MACHINE_909) {
        int32_t width = (int32_t)g->flam_width_samples + (delta > 0 ? 24 : -24);
        groovebox_set_909_flam_width(g, (uint16_t)(width < 0 ? 0 : width));
        return;
    }
    if (slot != 1u) return;
    bit = (uint16_t)(1u << ui->selected_lane);
    state = (g->drum_accents[d][ui->selected_step] & bit) ? 2u :
            (g->drum_hits[d][ui->selected_step] & bit) ? 1u : 0u;
    state = wrap_u8(state, delta > 0 ? 1 : -1, 3u);
    g->drum_hits[d][ui->selected_step] &= (uint16_t)~bit;
    g->drum_accents[d][ui->selected_step] &= (uint16_t)~bit;
    if (state) g->drum_hits[d][ui->selected_step] |= bit;
    if (state == 2u) g->drum_accents[d][ui->selected_step] |= bit;
    if (ui->machine == UI240_MACHINE_909 && !state) g->drum_909_flams[ui->selected_step] &= (uint16_t)~bit;
}

void ui240_edit_mixer(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta)
{
    mixer_track_t *t = &g->mixer.track[ui->machine & 3u];
    if (slot == 0u) t->level = add_s16(t->level, delta, 1024, 0, 32767);
    else if (slot == 1u) t->pan = add_s16(t->pan, delta, 2048, -32767, 32767);
    else if (slot == 2u) {
        int n = (int)t->delay_send + (delta > 0 ? 4 : -4);
        if (n < 0) n = 0;
        if (n > 127) n = 127;
        t->delay_send = (uint8_t)n;
    } else if (slot == 3u) {
        int n = (int)t->reverb_send + (delta > 0 ? 4 : -4);
        if (n < 0) n = 0;
        if (n > 127) n = 127;
        t->reverb_send = (uint8_t)n;
    }
}

void ui240_edit_fx(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta)
{
    mixer_fx_t *m = &g->mixer;
    if (ui->fx_section == UI240_FX_PCF) {
        if (slot == 0u) m->filter_cutoff = add_s16(m->filter_cutoff, delta, 768, 256, 32767);
    } else if (ui->fx_section == UI240_FX_DELAY) {
        if (slot == 0u) {
            uint16_t step = (uint16_t)(MIX_DELAY_MAX / 32u);
            if (!step) step = 1u;
            m->delay_len = add_u16(m->delay_len, delta, step, MIX_DELAY_MAX);
            if (!m->delay_len) m->delay_len = 1u;
        } else if (slot == 2u) {
            m->delay_feedback = add_s16(m->delay_feedback, delta, 768, 0, 31000);
        } else if (slot == 3u) {
            m->delay_mix = add_s16(m->delay_mix, delta, 768, 0, 32767);
        }
    } else if (ui->fx_section == UI240_FX_DIST) {
        if (slot == 0u) m->drive = add_s16(m->drive, delta, 768, 0, 32767);
    } else {
        if (slot == 0u) m->compressor_threshold = add_s16(m->compressor_threshold, -delta, 768, 512, 32767);
    }
}
