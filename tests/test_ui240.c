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
    assert(ui.fx_section == UI240_FX_PCF);

    ui240_set_page(&ui, UI240_ACID_MAIN);
    ui240_set_machine(&ui, UI240_MACHINE_ACID2);
    g.acid[1].cutoff = 10000u;
    g.acid[1].resonance = 9000u;
    g.acid[1].env_mod = 8000u;
    g.acid[1].decay = 7000u;
    ui240_edit_acid_main(&ui, &g, 0u, 2);
    ui240_snapshot(&ui, &g, &v);
    assert(g.acid[1].cutoff > 10000u);
    assert(v.acid_cutoff_pct && v.acid_resonance_pct && v.acid_env_pct && v.acid_decay_pct);

    ui.selected_step = 5u;
    ui240_set_acid_step_type(&ui, &g, UI240_STEP_NOTE);
    ui240_toggle_acid_flag(&ui, &g, SEQ16_ACCENT);
    ui240_toggle_acid_flag(&ui, &g, SEQ16_SLIDE);
    ui240_set_page(&ui, UI240_ACID_STEP);
    ui240_snapshot(&ui, &g, &v);
    assert(v.step_type == UI240_STEP_NOTE && v.accent && v.slide);
    ui240_edit_acid_step(&ui, &g, 0u, 1);
    assert(g.acid_seq[1].step[5].note == 24u);
    ui240_edit_acid_step(&ui, &g, 0u, 1);
    assert(g.acid_seq[1].step[5].note == 25u);
    ui240_edit_acid_step(&ui, &g, 3u, -1);
    assert(g.acid_seq[1].step[5].probability == 95u);
    ui240_set_acid_step_type(&ui, &g, UI240_STEP_TIE);
    ui240_snapshot(&ui, &g, &v);
    assert(v.step_type == UI240_STEP_TIE);

    ui240_set_machine(&ui, UI240_MACHINE_808);
    ui.selected_lane = DRUM_BD;
    ui.selected_step = 0u;
    ui240_edit_drum(&ui, &g, 1u, 1);
    ui240_snapshot(&ui, &g, &v);
    assert(v.drum_state == 1u && (v.drum_hits & (1u << DRUM_BD)));
    ui240_edit_drum(&ui, &g, 1u, 1);
    ui240_snapshot(&ui, &g, &v);
    assert(v.drum_state == 2u && (v.drum_accents & (1u << DRUM_BD)));

    ui240_set_machine(&ui, UI240_MACHINE_909);
    ui.selected_lane = DRUM_SD;
    ui.selected_step = 2u;
    ui240_edit_drum(&ui, &g, 1u, 1); /* LOW hit */
    ui240_edit_drum(&ui, &g, 2u, 1); /* FLAM */
    ui240_snapshot(&ui, &g, &v);
    assert(v.drum_state == 1u);
    assert(v.drum_flam == 1u); /* p2 alias on DRUM page */
    {
        uint16_t before = g.flam_width_samples;
        ui240_edit_drum(&ui, &g, 3u, 1);
        assert(g.flam_width_samples > before);
        ui240_snapshot(&ui, &g, &v);
        assert(v.flam_width_pct > 0u); /* p3 alias */
    }
    ui240_edit_drum(&ui, &g, 1u, -1); /* LOW -> OFF clears flam */
    ui240_snapshot(&ui, &g, &v);
    assert(v.drum_state == 0u && v.drum_flam == 0u);

    ui240_set_page(&ui, UI240_MIXER);
    {
        int16_t before = g.mixer.track[3].level;
        ui240_edit_mixer(&ui, &g, 0u, -1);
        assert(g.mixer.track[3].level < before);
        ui240_edit_mixer(&ui, &g, 2u, 1);
        assert(g.mixer.track[3].delay_send > 0u);
    }

    ui240_set_page(&ui, UI240_FX);
    assert(ui.fx_section == UI240_FX_PCF);
    { int16_t before=g.mixer.filter_cutoff; ui240_edit_fx(&ui,&g,0u,1); assert(g.mixer.filter_cutoff>before); }
    ui240_cycle_fx_section(&ui,1); assert(ui.fx_section==UI240_FX_DELAY);
    { int16_t before=g.mixer.delay_feedback; ui240_edit_fx(&ui,&g,2u,1); assert(g.mixer.delay_feedback>before); }
    ui240_cycle_fx_section(&ui,1); assert(ui.fx_section==UI240_FX_DIST);
    { int16_t before=g.mixer.drive; ui240_edit_fx(&ui,&g,0u,1); assert(g.mixer.drive>before); }
    ui240_cycle_fx_section(&ui,1); assert(ui.fx_section==UI240_FX_COMP);
    { int16_t before=g.mixer.compressor_threshold; ui240_edit_fx(&ui,&g,0u,1); assert(g.mixer.compressor_threshold<before); }

    ui240_set_page(&ui,UI240_PROJECT);ui240_select_project(&ui,1);ui240_snapshot(&ui,&g,&v);assert(v.project_slot==1u);
    ui240_set_page(&ui,UI240_HOME);ui240_next_page(&ui,-1);assert(ui.page==UI240_EXT);
    puts("ui240 compact hardware UI tests: ok");
    return 0;
}
