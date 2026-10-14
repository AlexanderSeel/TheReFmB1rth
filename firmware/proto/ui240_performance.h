// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_UI240_PERFORMANCE_H
#define REFM_UI240_PERFORMANCE_H

#include <stdint.h>
#include "ui240.h"
#include "groovebox_pattern.h"

#define UI240_303_NOTE_MIN 24u
#define UI240_303_NOTE_MAX 60u

typedef enum {
    UI240_MOD_NONE = 0,
    UI240_MOD_SEQ,
    UI240_MOD_DRUM,
    UI240_MOD_MIXER,
    UI240_MOD_FX,
    UI240_MOD_PATTERN,
    UI240_MOD_EDIT,
    UI240_MOD_LIVE,
    UI240_MOD_FOCUS
} ui240_modifier_t;

typedef enum {
    UI240_FOCUS_ACID = 0,
    UI240_FOCUS_STEP,
    UI240_FOCUS_DRUM,
    UI240_FOCUS_MIXER,
    UI240_FOCUS_PCF,
    UI240_FOCUS_DELAY,
    UI240_FOCUS_DIST,
    UI240_FOCUS_COMP,
    UI240_FOCUS_COUNT
} ui240_focus_t;

typedef struct {
    uint8_t consumed;
    uint8_t drum_trigger;
    uint8_t drum_machine;
    uint8_t drum_lane;
    uint8_t velocity;
} ui240_key_result_t;

static inline uint8_t ui240_key_to_drum_lane(uint8_t machine, uint8_t key)
{
    static const uint8_t map808[DRUM_VOICES] = {
        DRUM_BD, DRUM_SD, DRUM_RS, DRUM_CP, DRUM_CH, DRUM_OH,
        DRUM_LT, DRUM_MT, DRUM_HT, DRUM_CR, DRUM_RD
    };
    static const uint8_t map909[DRUM_VOICES] = {
        DRUM_BD, DRUM_SD, DRUM_LT, DRUM_MT, DRUM_HT, DRUM_RS,
        DRUM_CP, DRUM_CH, DRUM_OH, DRUM_CR, DRUM_RD
    };
    key %= DRUM_VOICES;
    return machine == UI240_MACHINE_909 ? map909[key] : map808[key];
}

static inline void ui240_select_pattern_direct(groovebox_t *g, uint8_t pattern)
{
    groovebox_request_pattern(g, (uint8_t)(pattern & 7u));
}

static inline void ui240_apply_focus(ui240_state_t *ui, uint8_t focus)
{
    focus %= UI240_FOCUS_COUNT;
    switch (focus) {
    case UI240_FOCUS_ACID:
        if (ui->machine >= UI240_MACHINE_808) ui240_set_machine(ui, UI240_MACHINE_ACID1);
        ui240_set_page(ui, UI240_ACID_MAIN);
        break;
    case UI240_FOCUS_STEP:
        if (ui->machine >= UI240_MACHINE_808) ui240_set_machine(ui, UI240_MACHINE_ACID1);
        ui240_set_page(ui, UI240_ACID_STEP);
        break;
    case UI240_FOCUS_DRUM:
        if (ui->machine < UI240_MACHINE_808) ui240_set_machine(ui, UI240_MACHINE_808);
        ui240_set_page(ui, UI240_DRUM);
        break;
    case UI240_FOCUS_MIXER:
        ui240_set_page(ui, UI240_MIXER);
        break;
    case UI240_FOCUS_PCF:
    case UI240_FOCUS_DELAY:
    case UI240_FOCUS_DIST:
    case UI240_FOCUS_COMP:
        ui->fx_section = (uint8_t)(focus - UI240_FOCUS_PCF);
        ui240_set_page(ui, UI240_FX);
        break;
    default:
        break;
    }
}

static inline uint8_t ui240_focus_move(ui240_state_t *ui, uint8_t focus, int8_t delta)
{
    int v = (int)focus + (delta < 0 ? -1 : 1);
    if (v < 0) v = UI240_FOCUS_COUNT - 1;
    if (v >= UI240_FOCUS_COUNT) v = 0;
    ui240_apply_focus(ui, (uint8_t)v);
    return (uint8_t)v;
}

static inline void ui240_set_drum_hit(groovebox_t *g, ui240_state_t *ui,
                                      uint8_t key, uint8_t high, uint8_t live,
                                      ui240_key_result_t *r)
{
    uint8_t d = (uint8_t)(ui->machine - UI240_MACHINE_808);
    uint8_t lane = ui240_key_to_drum_lane(ui->machine, key);
    uint8_t step = live && g->transport.playing ? (uint8_t)(g->step & 15u) : (uint8_t)(ui->selected_step & 15u);
    uint16_t bit = (uint16_t)(1u << lane);
    ui->selected_lane = lane;
    ui->selected_step = step;
    g->drum_hits[d][step] |= bit;
    if (high) g->drum_accents[d][step] |= bit;
    else g->drum_accents[d][step] &= (uint16_t)~bit;
    r->drum_trigger = 1u;
    r->drum_machine = d;
    r->drum_lane = lane;
    r->velocity = high ? 122u : 94u;
}

static inline ui240_key_result_t ui240_performance_key(ui240_state_t *ui,
                                                        groovebox_t *g,
                                                        uint8_t modifier,
                                                        uint8_t key,
                                                        uint8_t note,
                                                        uint8_t on)
{
    ui240_key_result_t r = {0u, 0u, 0u, 0u, 0u};
    uint8_t press = on ? 1u : 0u;

    if (modifier == UI240_MOD_FOCUS) {
        r.consumed = 1u;
        if (press && key < UI240_FOCUS_COUNT) ui240_apply_focus(ui, key);
        return r;
    }

    if (modifier == UI240_MOD_PATTERN) {
        r.consumed = 1u;
        if (press && key < 8u) ui240_select_pattern_direct(g, key);
        return r;
    }

    if (modifier == UI240_MOD_MIXER) {
        r.consumed = 1u;
        if (!press) return r;
        if (key < 4u) g->mixer.track[key].mute ^= 1u;
        else if (key < 8u) g->mixer.track[key - 4u].solo ^= 1u;
        else if (key < 12u) ui240_set_machine(ui, (ui240_machine_t)(key - 8u));
        return r;
    }

    if (modifier == UI240_MOD_FX) {
        r.consumed = 1u;
        if (!press) return r;
        if (key < 4u) {
            ui->fx_section = key;
            ui240_set_page(ui, UI240_FX);
        } else if (key == 4u) {
            g->mixer.filter_cutoff = g->mixer.filter_cutoff < 12000 ? 28000 : 7000;
        } else if (key == 5u) {
            g->mixer.delay_mix = g->mixer.delay_mix < 8000 ? 24500 : 3500;
        } else if (key == 6u) {
            g->mixer.drive = g->mixer.drive < 6000 ? 18000 : 2500;
        } else if (key == 7u) {
            g->mixer.compressor_threshold = g->mixer.compressor_threshold > 14000 ? 7000 : 24000;
        }
        return r;
    }

    if (ui->machine >= UI240_MACHINE_808 || modifier == UI240_MOD_DRUM || ui->page == UI240_DRUM) {
        uint8_t high;
        uint8_t local_key;
        if (ui->machine < UI240_MACHINE_808) ui240_set_machine(ui, UI240_MACHINE_808);
        r.consumed = 1u;
        if (!press) return r;
        if (key < 22u) {
            high = key >= DRUM_VOICES ? 1u : 0u;
            local_key = (uint8_t)(key % DRUM_VOICES);
            ui240_set_drum_hit(g, ui, local_key, high || modifier == UI240_MOD_EDIT,
                               modifier == UI240_MOD_LIVE, &r);
            return r;
        }
        if (ui->machine == UI240_MACHINE_909) {
            if (key == 22u) {
                ui240_toggle_909_flam(ui, g);
            } else if (key == 23u) {
                uint16_t width = g->flam_width_samples;
                groovebox_set_909_flam_width(g, width > 24u ? (uint16_t)(width - 24u) : 0u);
            } else if (key == 24u) {
                uint32_t width = (uint32_t)g->flam_width_samples + 24u;
                groovebox_set_909_flam_width(g, (uint16_t)(width > 65535u ? 65535u : width));
            }
        }
        return r;
    }

    if (modifier == UI240_MOD_EDIT && press && ui->machine < UI240_MACHINE_808) {
        r.consumed = 1u;
        if (key == 0u) ui240_toggle_acid_flag(ui, g, SEQ16_ACCENT);
        else if (key == 1u) ui240_toggle_acid_flag(ui, g, SEQ16_SLIDE);
        else if (key == 2u) ui240_set_acid_step_type(ui, g, UI240_STEP_TIE);
        else if (key == 3u) ui240_set_acid_step_type(ui, g, UI240_STEP_REST);
        else if (key == 4u) g->acid_seq[ui->machine].step[ui->selected_step & 15u].probability = 50u;
        else if (key == 5u) g->acid_seq[ui->machine].step[ui->selected_step & 15u].probability = 100u;
        return r;
    }

    if ((modifier == UI240_MOD_SEQ || modifier == UI240_MOD_LIVE || ui->page == UI240_ACID_STEP) &&
        ui->machine < UI240_MACHINE_808) {
        seq16_step_t *st;
        uint8_t step;
        r.consumed = 1u;
        if (!press) return r;
        if (note < UI240_303_NOTE_MIN) note = UI240_303_NOTE_MIN;
        if (note > UI240_303_NOTE_MAX) note = UI240_303_NOTE_MAX;
        step = modifier == UI240_MOD_LIVE && g->transport.playing ? (uint8_t)(g->step & 15u) : (uint8_t)(ui->selected_step & 15u);
        ui->selected_step = step;
        st = &g->acid_seq[ui->machine].step[step];
        st->note = note;
        st->flags &= (uint8_t)~SEQ16_TIE;
        st->flags |= SEQ16_GATE;
        if (!st->probability) st->probability = 100u;
        return r;
    }

    return r;
}

#endif