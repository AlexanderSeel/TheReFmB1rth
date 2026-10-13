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
    assert(g.drum_flams[3] & (1u << DRUM_BD));

    /* Direct pattern keyboard switching preserves the edited source pattern. */
    ui240_select_pattern_direct(&g, 3u);
    assert(g.current_pattern == 3u);

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

    /* ACID keyboard note entry writes the selected step instead of only
       auditioning a live note. */
    ui240_set_machine(&ui, UI240_MACHINE_ACID1);
    ui240_set_page(&ui, UI240_ACID_STEP);
    ui.selected_step = 6u;
    r = ui240_performance_key(&ui, &g, UI240_MOD_SEQ, 5u, 52u, 1u);
    assert(r.consumed);
    assert(g.acid_seq[0].step[6].note == 52u);
    assert(g.acid_seq[0].step[6].flags & SEQ16_GATE);

    puts("ui240 keyboard performance/focus tests: ok");
    return 0;
}
