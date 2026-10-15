// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include "../firmware/proto/project_store.h"

void refm_felucca_init(uint16_t bpm);
void refm_felucca_audio_block(int32_t *out, uint32_t frames);
void refm_felucca_midi_packet(uint32_t packet);
void refm_felucca_use_internal_clock(uint16_t bpm);
void refm_felucca_panic(void);
void refm_felucca_ui_action(uint8_t action);
void refm_felucca_ui_encoder(uint8_t encoder, int16_t delta);
uint16_t refm_felucca_ui_value(uint8_t id);
int refm_felucca_save(uint8_t *out, size_t capacity, size_t *written);
int refm_felucca_load(const uint8_t *blob, size_t blob_len);
uint16_t refm_felucca_bpm(void);
uint8_t refm_felucca_pattern(void);
uint8_t refm_felucca_step(void);

static uint32_t usb_midi(uint8_t status, uint8_t d1, uint8_t d2) {
    return (uint32_t)(status >> 4) | ((uint32_t)status << 8) |
           ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24);
}

static void settle_boot_probe(int32_t *audio) {
    /* Harmless stopped-silence rendering on normal host builds; on the
       REFM_FELUCCA_PLATFORM overlay this consumes the beta.11 3 s direct tone
       before assertions that intentionally exercise normal groovebox audio. */
    for (unsigned block = 0u; block < 1034u; ++block)
        refm_felucca_audio_block(audio, 128u);
}

int main(void) {
    int32_t audio[128 * 2];
    uint8_t blob[PROJECT_STORE_MAX_PAYLOAD];
    size_t written = 0;

    refm_felucca_init(132u);
    assert(refm_felucca_bpm() == 132u);
    assert(refm_felucca_ui_value(0u) == 0u); /* HOME */
    assert(refm_felucca_ui_value(1u) == 0u); /* ACID A */
    settle_boot_probe(audio);

    /* Direct MIDI remains usable after the physical boot probe. */
    refm_felucca_midi_packet(usb_midi(0x90u, 60u, 110u));
    refm_felucca_audio_block(audio, 128u);
    {
        int nonzero = 0;
        for (unsigned i = 0; i < 256u; ++i) if (audio[i]) { nonzero = 1; break; }
        assert(nonzero);
    }
    refm_felucca_panic();

    /* The hardware beta path: select ACID page, edit, then PLAY the seeded
       factory Pattern A through the internal clock. */
    refm_felucca_ui_action(1u); /* ACID A */
    assert(refm_felucca_ui_value(0u) == 1u);
    assert(refm_felucca_ui_value(1u) == 0u);
    {
        uint16_t cutoff = refm_felucca_ui_value(9u);
        refm_felucca_ui_encoder(0u, 1);
        assert(refm_felucca_ui_value(9u) > cutoff);
    }
    refm_felucca_ui_action(9u); /* PLAY */
    assert(refm_felucca_ui_value(3u) == 1u);
    {
        int heard = 0;
        for (unsigned block = 0; block < 60u; ++block) {
            refm_felucca_audio_block(audio, 128u);
            for (unsigned i = 0; i < 256u; ++i) if (audio[i]) heard = 1;
        }
        assert(heard);
        assert(refm_felucca_step() != 0u);
    }
    refm_felucca_ui_action(9u); /* STOP */
    assert(refm_felucca_ui_value(3u) == 0u);

    /* Dedicated page routing. */
    refm_felucca_ui_action(3u); /* STEP */
    assert(refm_felucca_ui_value(0u) == 2u);
    refm_felucca_ui_encoder(4u, 1); /* PRESET knob selects next step here */
    assert(refm_felucca_ui_value(5u) == 1u);
    refm_felucca_ui_action(4u); /* DRUM */
    assert(refm_felucca_ui_value(0u) == 3u);
    assert(refm_felucca_ui_value(1u) >= 2u);
    refm_felucca_ui_action(5u); /* MIXER */
    assert(refm_felucca_ui_value(0u) == 4u);
    refm_felucca_ui_action(6u); /* FX */
    assert(refm_felucca_ui_value(0u) == 5u);
    refm_felucca_ui_action(8u); /* PROJECT */
    assert(refm_felucca_ui_value(0u) == 7u);

    /* External MIDI clock path still works after switching back. */
    refm_felucca_init(132u);
    refm_felucca_midi_packet((uint32_t)0xF8u << 8 | 0x0Fu);
    refm_felucca_midi_packet((uint32_t)0xFAu << 8 | 0x0Fu);
    for (unsigned i = 0; i < 6u; ++i)
        refm_felucca_midi_packet((uint32_t)0xF8u << 8 | 0x0Fu);
    assert(refm_felucca_step() == 1u);

    refm_felucca_use_internal_clock(140u);
    assert(refm_felucca_bpm() == 140u);

    assert(refm_felucca_save(blob, sizeof(blob), &written) == 0);
    assert(written > 2000u);
    refm_felucca_init(90u);
    assert(refm_felucca_load(blob, written) == 0);
    assert(refm_felucca_bpm() == 140u);

    refm_felucca_panic();
    return 0;
}
