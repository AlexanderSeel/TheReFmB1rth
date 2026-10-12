// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/ui240.h"

int main(void)
{
    groovebox_t g;
    ui240_state_t ui;
    ui240_view_t v;

    groovebox_init(&g, 128u);
    ui240_init(&ui);
    assert(ui.page == UI240_HOME);
    assert(ui.machine == UI240_MACHINE_ACID1);

    ui240_set_page(&ui, UI240_ACID_MAIN);
    ui240_set_machine(&ui, UI240_MACHINE_ACID2);
    assert(ui.page == UI240_ACID_MAIN);
    assert(ui.machine == UI240_MACHINE_ACID2);

    g.acid[1].cutoff = 10000u;
    ui240_edit_acid_main(&ui, &g, 0u, 2);
    assert(g.acid[1].cutoff > 10000u);

    ui.selected_step = 5u;
    ui240_set_acid_step_type(&ui, &g, UI240_STEP_NOTE);
    ui240_toggle_acid_flag(&ui, &g, SEQ16_ACCENT);
    ui240_toggle_acid_flag(&ui, &g, SEQ16_SLIDE);
    ui240_set_page(&ui, UI240_ACID_STEP);
    ui240_snapshot(&ui, &g, &v);
    assert(v.selected_step == 5u);
    assert(v.step_type == UI240_STEP_NOTE);
    assert(v.accent == 1u);
    assert(v.slide == 1u);

    {
        uint8_t old_note = g.acid_seq[1].step[5].note;
        ui240_edit_acid_step(&ui, &g, 0u, 1);
        assert(g.acid_seq[1].step[5].note == old_note + 1u);
        ui240_edit_acid_step(&ui, &g, 3u, -1);
        assert(g.acid_seq[1].step[5].probability == 95u);
    }

    ui240_set_acid_step_type(&ui, &g, UI240_STEP_TIE);
    ui240_snapshot(&ui, &g, &v);
    assert(v.step_type == UI240_STEP_TIE);
    assert((g.acid_seq[1].step[5].flags & SEQ16_GATE) == 0u);

    ui240_set_machine(&ui, UI240_MACHINE_808);
    assert(ui.machine == UI240_MACHINE_808);
    assert(ui.page == UI240_DRUM);
    ui.selected_lane = DRUM_BD;
    ui240_edit_drum(&ui, &g, 1u, 1);
    ui240_snapshot(&ui, &g, &v);
    assert(v.drum_state == 1u);
    ui240_edit_drum(&ui, &g, 1u, 1);
    ui240_snapshot(&ui, &g, &v);
    assert(v.drum_state == 2u);

    ui240_set_page(&ui, UI240_MIXER);
    {
        int16_t before = g.mixer.track[2].level;
        ui240_edit_mixer(&ui, &g, 0u, -1);
        assert(g.mixer.track[2].level < before);
        ui240_edit_mixer(&ui, &g, 2u, 1);
        assert(g.mixer.track[2].delay_send > 0u);
    }

    ui240_set_page(&ui, UI240_FX);
    {
        int16_t before = g.mixer.drive;
        ui240_edit_fx(&ui, &g, 0u, 1);
        assert(g.mixer.drive > before);
    }

    ui240_set_page(&ui, UI240_PROJECT);
    ui240_select_project(&ui, 1);
    ui240_snapshot(&ui, &g, &v);
    assert(v.project_slot == 1u);

    ui240_set_page(&ui, UI240_HOME);
    ui240_next_page(&ui, -1);
    assert(ui.page == UI240_EXT);

    puts("ui240 compact hardware UI tests: ok");
    return 0;
}
