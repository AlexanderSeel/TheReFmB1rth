// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_UI240_H
#define REFM_UI240_H

#include <stdint.h>
#include "groovebox.h"

typedef enum {
    UI240_HOME = 0,
    UI240_ACID_MAIN,
    UI240_ACID_STEP,
    UI240_DRUM,
    UI240_MIXER,
    UI240_FX,
    UI240_SONG,
    UI240_PROJECT,
    UI240_EXT,
    UI240_PAGE_COUNT
} ui240_page_t;

typedef enum {
    UI240_MACHINE_ACID1 = 0,
    UI240_MACHINE_ACID2,
    UI240_MACHINE_808,
    UI240_MACHINE_909
} ui240_machine_t;

typedef enum {
    UI240_STEP_REST = 0,
    UI240_STEP_NOTE,
    UI240_STEP_TIE
} ui240_step_type_t;

typedef enum {
    UI240_FX_PCF = 0,
    UI240_FX_DELAY,
    UI240_FX_DIST,
    UI240_FX_COMP,
    UI240_FX_SECTION_COUNT
} ui240_fx_section_t;

typedef struct {
    uint8_t page;
    uint8_t machine;
    uint8_t selected_step;
    uint8_t selected_lane;
    uint8_t project_slot;
    uint8_t fx_section;
} ui240_state_t;

typedef struct {
    uint8_t page;
    uint8_t machine;
    uint8_t pattern;
    uint8_t playing;
    uint8_t playing_step;
    uint8_t selected_step;
    uint8_t selected_lane;
    uint8_t project_slot;
    uint8_t fx_section;
    uint16_t bpm;
    uint16_t p0;
    uint16_t p1;
    uint16_t p2;
    uint16_t p3;
    uint8_t wave_square;
    uint8_t step_type;
    uint8_t accent;
    uint8_t slide;
    uint8_t step_probability;
    uint8_t drum_state;
    uint8_t drum_flam;
    uint8_t flam_width_pct;
    uint8_t mute;
    uint8_t solo;
    uint16_t drum_hits;
    uint16_t drum_accents;
    int8_t acid_tune;
    uint8_t acid_accent;
    uint8_t acid_cutoff_pct;
    uint8_t acid_resonance_pct;
    uint8_t acid_env_pct;
    uint8_t acid_decay_pct;
    uint8_t song_loop;
    uint8_t song_length;
} ui240_view_t;

void ui240_init(ui240_state_t *ui);
void ui240_set_page(ui240_state_t *ui, ui240_page_t page);
void ui240_set_machine(ui240_state_t *ui, ui240_machine_t machine);
void ui240_next_page(ui240_state_t *ui, int8_t delta);
void ui240_cycle_machine(ui240_state_t *ui, int8_t delta);
void ui240_select_step(ui240_state_t *ui, int8_t delta);
void ui240_select_lane(ui240_state_t *ui, int8_t delta);
void ui240_select_project(ui240_state_t *ui, int8_t delta);
void ui240_cycle_fx_section(ui240_state_t *ui, int8_t delta);
void ui240_snapshot(const ui240_state_t *ui, const groovebox_t *g, ui240_view_t *out);
void ui240_edit_acid_main(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta);
void ui240_edit_acid_step(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta);
void ui240_set_acid_step_type(ui240_state_t *ui, groovebox_t *g, ui240_step_type_t type);
void ui240_toggle_acid_flag(ui240_state_t *ui, groovebox_t *g, uint8_t flag);
void ui240_edit_drum(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta);
void ui240_toggle_909_flam(ui240_state_t *ui, groovebox_t *g);
void ui240_edit_mixer(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta);
void ui240_edit_fx(ui240_state_t *ui, groovebox_t *g, uint8_t slot, int8_t delta);

#endif
