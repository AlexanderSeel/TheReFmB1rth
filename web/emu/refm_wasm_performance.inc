/* SPDX-License-Identifier: GPL-3.0-only
 * Appended to refm_wasm.c by build_wasm.sh so it shares the exact VM/ui state.
 */
#include "../../firmware/proto/ui240_performance.h"

static uint8_t panel_modifier;
static uint8_t panel_focus_active;
static uint8_t panel_focus_region;

REFM_EXPORT void refm_wasm_panel_modifier(uint8_t modifier)
{
    if (modifier > UI240_MOD_FOCUS) modifier = UI240_MOD_NONE;
    if (modifier == UI240_MOD_FOCUS) {
        if (!panel_focus_active) {
            panel_focus_active = 1u;
            panel_focus_region = UI240_FOCUS_ACID;
            ui240_apply_focus(&panel_ui, panel_focus_region);
        }
    }
    panel_modifier = modifier;
}

REFM_EXPORT void refm_wasm_panel_focus_exit(void)
{
    panel_focus_active = 0u;
    if (panel_modifier == UI240_MOD_FOCUS) panel_modifier = UI240_MOD_NONE;
}

REFM_EXPORT void refm_wasm_panel_key_event(uint8_t key, uint8_t note, uint8_t on)
{
    ui240_key_result_t r = ui240_performance_key(&panel_ui, &vm.groovebox, panel_modifier, key, note, on);
    if (r.drum_trigger)
        drum_machine_trigger(&vm.groovebox.drum[r.drum_machine], (drum_voice_id_t)r.drum_lane, r.velocity);
    if (r.consumed) return;
    if (panel_ui.machine >= UI240_MACHINE_808 || note > 127u) return;
    refm_target_midi_byte(&vm, (uint8_t)((on ? 0x90u : 0x80u) | panel_ui.machine));
    refm_target_midi_byte(&vm, note);
    refm_target_midi_byte(&vm, on ? 108u : 0u);
}

REFM_EXPORT void refm_wasm_panel_performance_encoder(uint8_t encoder, int16_t delta)
{
    int8_t d = panel_sign(delta);
    if (!d) return;
    if (panel_focus_active && (encoder == 4u || encoder == 5u)) {
        unsigned n = (unsigned)(delta < 0 ? -delta : delta);
        if (n > 8u) n = 8u;
        while (n--)
            panel_focus_region = ui240_focus_move(&panel_ui, panel_focus_region, encoder == 4u ? -d : d);
        return;
    }
    refm_wasm_panel_encoder(encoder, delta);
}

REFM_EXPORT void refm_wasm_panel_performance_action(uint8_t action)
{
    if (action == 0u) refm_wasm_panel_focus_exit();
    refm_wasm_panel_action(action);
}

/* Fields 36..51 are a Virtual-FM-1 read-only pattern window.  Each field is
   one of the 16 steps for the machine currently shown on the panel.

   ACID A/B: note | flags<<8 | probability<<16 (same compact shape as
   refm_wasm_get_acid_step, minus micro-timing).

   808/909: low two bits are OFF/HIT/ACCENT for the currently selected drum
   lane. Bit 2 marks 909 FLAM.  Keeping this readback in the panel ABI means
   the LCD renderer shows the exact groovebox pattern rather than inferring a
   sequence from only selected_step/play_step.
 */
static uint32_t panel_sequence_step(uint8_t step)
{
    step &= 15u;
    if (panel_ui.machine < UI240_MACHINE_808) {
        const seq16_step_t *st = &vm.groovebox.acid_seq[panel_ui.machine].step[step];
        return (uint32_t)st->note |
               ((uint32_t)st->flags << 8) |
               ((uint32_t)st->probability << 16);
    } else {
        unsigned d = (unsigned)panel_ui.machine - UI240_MACHINE_808;
        uint16_t bit = (uint16_t)(1u << (panel_ui.selected_lane < DRUM_VOICES ? panel_ui.selected_lane : 0u));
        uint32_t state = (vm.groovebox.drum_accents[d][step] & bit) ? 2u :
                         (vm.groovebox.drum_hits[d][step] & bit) ? 1u : 0u;
        if (panel_ui.machine == UI240_MACHINE_909 && (vm.groovebox.drum_909_flams[step] & bit))
            state |= 4u;
        return state;
    }
}

REFM_EXPORT uint32_t refm_wasm_panel_performance_value(uint8_t field)
{
    if (field == 33u) return panel_modifier;
    if (field == 34u) return panel_focus_active;
    if (field == 35u) return panel_focus_region;
    if (field >= 36u && field < 52u) return panel_sequence_step((uint8_t)(field - 36u));
    return refm_wasm_panel_value(field);
}
