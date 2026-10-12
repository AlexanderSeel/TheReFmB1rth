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

void ui240_init(ui240_state_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->machine = UI240_MACHINE_ACID1;
}

void ui240_next_page(ui240_state_t *ui, int8_t delta)
{
    int p = (int)ui->page + delta;
    while (p < 0) p += UI240_PAGE_COUNT;
    while (p >= UI240_PAGE_COUNT) p -= UI240_PAGE_COUNT;
    ui->page = (uint8_t)p;
}

void ui240_cycle_machine(ui240_state_t *ui, int8_t delta)
{
    int m = (int)ui->machine + delta;
    while (m < 0) m += 4;
    while (m >= 4) m -= 4;
    ui->machine = (uint8_t)m;
    if (ui->machine >= UI240_MACHINE_808 && ui->page == UI240_ACID_MAIN)
        ui->page = UI240_DRUM;
    if (ui->machine < UI240_MACHINE_808 && ui->page == UI240_DRUM)
        ui->page = UI240_ACID_MAIN;
}

void ui240_select_step(ui240_state_t *ui, int8_t delta)
{
    int s = (int)ui->selected_step + delta;
    while (s < 0) s += 16;
    while (s >= 16) s -= 16;
    ui->selected_step = (uint8_t)s;
}

void ui240_snapshot(const ui240_state_t *ui, const groovebox_t *g, ui240_view_t *out)
{
    memset(out, 0, sizeof(*out));
    out->page = ui->page;
    out->machine = ui->machine;
    out->pattern = g->current_pattern & 7u;
    out->playing_step = g->step & 15u;
    out->selected_step = ui->selected_step & 15u;
    out->bpm = g->transport.bpm;

    if (ui->machine < UI240_MACHINE_808) {
        unsigned a = ui->machine;
        const acid303_t *s = &g->acid[a];
        const seq16_step_t *st = &g->acid_seq[a].step[out->selected_step];
        out->p0 = s->cutoff;
        out->p1 = s->resonance;
        out->p2 = s->env_mod;
        out->p3 = s->decay;
        out->wave_square = s->square;
        out->accent = (st->flags & SEQ16_ACCENT) != 0;
        out->slide = (st->flags & SEQ16_SLIDE) != 0;
        if ((st->flags & SEQ16_TIE) && !(st->flags & SEQ16_GATE)) out->step_type = UI240_STEP_TIE;
        else if (st->flags & SEQ16_GATE) out->step_type = UI240_STEP_NOTE;
        else out->step_type = UI240_STEP_REST;
    } else {
        unsigned d = ui->machine - UI240_MACHINE_808;
        out->drum_hits = g->drum_hits[d][out->selected_step];
        out->drum_accents = g->drum_accents[d][out->selected_step];
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
