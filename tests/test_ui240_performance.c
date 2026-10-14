// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/ui240_performance.h"

int main(void)
{
    groovebox_t g;
    ui240_state_t ui;
    ui240_key_result_t r;

    groovebox_init(&g, 128u);
    ui240_init(&ui);

    /* 808 and 909 are deliberately different workflows, not two labels over
       the same pad order. */
    assert(ui240_key_to_drum_lane(UI240_MACHINE_808, 2u) == DRUM_RS);
    assert(ui240_key_to_drum_lane(UI240_MACHINE_909, 2u) == DRUM_LT);

    ui240_set_machine(&ui, UI240_MACHINE_909);
    ui240_set_page(&ui, UI240_DRUM);
    ui.selected_step = 3u;
    r = ui240_performance_key(&ui, &g, UI240_MOD_DRUM, 0u, 36u, 1u);
    assert(r.consumed && r.drum_trigger && r.drum_lane == DRUM_BD && r.velocity == 94u);
    assert(g.drum_hits[1][3] & (1u << DRUM_BD));
    assert(!(g.drum_accents[1][3] & (1u << DRUM_BD)));

    /* The second 11-key bank is HIGH/accent on 909. */
    r = ui240_performance_key(&ui, &g, UI240_MOD_DRUM, 11u, 47u, 1u);
    assert(r.drum_lane == DRUM_BD && r.velocity == 122u);
    assert(g.drum_accents[1][3] & (1u << DRUM_BD));

    ui.selected_lane = DRUM_BD;
    (void)ui240_performance_key(&ui, &g, UI240_MOD_DRUM, 22u, 58u, 1u);
    assert(g.drum_909_flams[3] & (1u << DRUM_BD));

    /* Pattern keyboard selection is immediate while stopped, but quantized
       to the next 16-step boundary while transport is running. */
    ui240_select_pattern_direct(&g, 3u);
    assert(g.current_pattern == 3u);
    assert(g.pending_pattern == GROOVEBOX_NO_PENDING_PATTERN);
    g.transport.playing = 1u;
    ui240_select_pattern_direct(&g, 5u);
    assert(g.current_pattern == 3u);
    assert(g.pending_pattern == 5u);
    g.step = 15u;
    groovebox_advance_step(&g);
    assert(g.current_pattern == 5u);
    assert(g.pending_pattern == GROOVEBOX_NO_PENDING_PATTERN);
    assert(g.step == 0u);
    g.transport.playing = 0u;

    /* Mixer performance bank: four mutes, then four solos. */
    (void)ui240_performance_key(&ui, &g, UI240_MOD_MIXER, 0u, 36u, 1u);
    assert(g.mixer.track[0].mute == 1u);
    (void)ui240_performance_key(&ui, &g, UI240_MOD_MIXER, 4u, 40u, 1u);
    assert(g.mixer.track[0].solo == 1u);

    /* FX bank directly selects the four requested sections. */
    (void)ui240_performance_key(&ui, &g, UI240_MOD_FX, 2u, 38u, 1u);
    assert(ui.page == UI240_FX && ui.fx_section == UI240_FX_DIST);

    /* FOCUS key selectors use the same page model as the normal UI. */
    (void)ui240_performance_key(&ui, &g, UI240_MOD_FOCUS, UI240_FOCUS_COMP, 43u, 1u);
    assert(ui.page == UI240_FX && ui.fx_section == UI240_FX_COMP);
    assert(ui240_focus_move(&ui, UI240_FOCUS_COMP, 1) == UI240_FOCUS_ACID);
    assert(ui.page == UI240_ACID_MAIN);

    /* ReBirth/TB-303 step entry uses one physical C..C pitch octave. The
       incoming live MIDI-note value is deliberately ignored here: key index
       selects the pitch, while DOWN/UP changes the octave state separately.
       Corrected register: DOWN C=48, normal C=60, UP C=72. */
    ui240_set_machine(&ui, UI240_MACHINE_ACID1);
    ui240_set_page(&ui, UI240_ACID_STEP);
    ui.selected_step = 6u;
    r = ui240_performance_key(&ui, &g, UI240_MOD_SEQ, 5u, 99u, 1u);
    assert(r.consumed);
    assert(g.acid_seq[0].step[6].note == 65u); /* F4 in normal range */
    assert(g.acid_seq[0].step[6].flags & SEQ16_GATE);

    g.acid_seq[0].step[6].flags |= SEQ16_DOWN;
    r = ui240_performance_key(&ui, &g, UI240_MOD_SEQ, 12u, 1u, 1u);
    assert(r.consumed);
    assert(g.acid_seq[0].step[6].note == 60u); /* top C of DOWN range */

    g.acid_seq[0].step[6].flags &= (uint8_t)~SEQ16_DOWN;
    g.acid_seq[0].step[6].flags |= SEQ16_UP;
    r = ui240_performance_key(&ui, &g, UI240_MOD_SEQ, 0u, 127u, 1u);
    assert(r.consumed);
    assert(g.acid_seq[0].step[6].note == 72u); /* bottom C of UP range */

    assert(UI240_303_NOTE_MIN == 48u);
    assert(UI240_303_NORMAL_C == 60u);
    assert(UI240_303_NOTE_MAX == 84u);

    puts("ui240 keyboard performance/focus tests: ok");
    return 0;
}
