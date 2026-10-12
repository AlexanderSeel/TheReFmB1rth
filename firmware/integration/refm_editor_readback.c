// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>

/*
 * Compact device -> browser editor snapshot.
 *
 * Browser requests a snapshot with CC 107 value >= 64 on MIDI channel 16.
 * The USB receive path only raises a pending flag. The actual MIDI OUT packets
 * are emitted from the audio render context, preserving Felucca's single
 * producer assumption for its MIDI OUT ring.
 *
 * Response is a short CC envelope on channel 16:
 *   107 = 1   begin
 *   108       machine
 *   109       page
 *   110       pattern
 *   111       playing
 *   112       selected step
 *   113       selected lane
 *   114/115   BPM low/high 7-bit parts
 *   116..119  page-normalized P0..P3
 *   120       flags: wave bit0, accent bit1, slide bit2, mute bit3, solo bit4
 *   121       step type
 *   122       step probability
 *   123       drum state
 *   124       project slot
 *   107 = 127 end
 */

#ifdef REFM_FELUCCA_PLATFORM
extern uint16_t refm_felucca_ui_value(uint8_t id);
extern void refm_platform_midi_out_event(uint32_t packet);

static volatile uint8_t g_readback_pending;

static void emit_cc(uint8_t cc, uint8_t value)
{
    uint32_t packet = 0x0Bu | ((uint32_t)0xBFu << 8) |
                      ((uint32_t)(cc & 0x7Fu) << 16) |
                      ((uint32_t)(value & 0x7Fu) << 24);
    refm_platform_midi_out_event(packet);
}

static uint8_t scale_u16(uint16_t value, uint32_t max)
{
    uint32_t v = value;
    if (!max) return 0u;
    if (v > max) v = max;
    return (uint8_t)((v * 127u + max / 2u) / max);
}

static uint8_t normalized_param(uint8_t page, uint8_t slot, uint16_t value)
{
    /* UI240 pages: HOME=0, ACID=1, STEP=2, DRUM=3, MIXER=4, FX=5. */
    if (page == 2u) {
        if (slot == 0u) return value > 127u ? 127u : (uint8_t)value;
        if (slot == 1u || slot == 2u) return value > 3u ? 3u : (uint8_t)value;
        return value > 100u ? 100u : (uint8_t)value;
    }
    if (page == 4u) {
        if (slot == 0u) return scale_u16(value, 32767u);
        if (slot == 1u) return scale_u16(value, 65534u);
        return value > 127u ? 127u : (uint8_t)value;
    }
    if (page == 1u || page == 5u)
        return scale_u16(value, 32767u);
    return value > 127u ? 127u : (uint8_t)value;
}

void refm_felucca_request_state_readback(void)
{
    g_readback_pending = 1u;
}

void refm_felucca_emit_state_readback_if_pending(void)
{
    uint16_t bpm, p0, p1, p2, p3;
    uint8_t page, flags = 0u;
    if (!g_readback_pending) return;
    g_readback_pending = 0u;

    page = (uint8_t)refm_felucca_ui_value(0u);
    bpm = refm_felucca_ui_value(8u);
    p0 = refm_felucca_ui_value(9u);
    p1 = refm_felucca_ui_value(10u);
    p2 = refm_felucca_ui_value(11u);
    p3 = refm_felucca_ui_value(12u);

    if (refm_felucca_ui_value(13u)) flags |= 1u;
    if (refm_felucca_ui_value(15u)) flags |= 2u;
    if (refm_felucca_ui_value(16u)) flags |= 4u;
    if (refm_felucca_ui_value(19u)) flags |= 8u;
    if (refm_felucca_ui_value(20u)) flags |= 16u;

    emit_cc(107u, 1u);
    emit_cc(108u, (uint8_t)refm_felucca_ui_value(1u));
    emit_cc(109u, page);
    emit_cc(110u, (uint8_t)refm_felucca_ui_value(2u));
    emit_cc(111u, (uint8_t)refm_felucca_ui_value(3u));
    emit_cc(112u, (uint8_t)refm_felucca_ui_value(5u));
    emit_cc(113u, (uint8_t)refm_felucca_ui_value(6u));
    emit_cc(114u, (uint8_t)(bpm & 0x7Fu));
    emit_cc(115u, (uint8_t)((bpm >> 7) & 0x7Fu));
    emit_cc(116u, normalized_param(page, 0u, p0));
    emit_cc(117u, normalized_param(page, 1u, p1));
    emit_cc(118u, normalized_param(page, 2u, p2));
    emit_cc(119u, normalized_param(page, 3u, p3));
    emit_cc(120u, flags);
    emit_cc(121u, (uint8_t)refm_felucca_ui_value(14u));
    emit_cc(122u, (uint8_t)refm_felucca_ui_value(17u));
    emit_cc(123u, (uint8_t)refm_felucca_ui_value(18u));
    emit_cc(124u, (uint8_t)refm_felucca_ui_value(7u));
    emit_cc(107u, 127u);
}
#else
void refm_felucca_request_state_readback(void) {}
void refm_felucca_emit_state_readback_if_pending(void) {}
#endif
