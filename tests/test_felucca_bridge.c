// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

void refm_felucca_init(uint16_t bpm);
void refm_felucca_audio_block(int32_t *out, uint32_t frames);
void refm_felucca_midi_packet(uint32_t packet);
void refm_felucca_use_internal_clock(uint16_t bpm);
void refm_felucca_panic(void);
int refm_felucca_save(uint8_t *out, size_t capacity, size_t *written);
int refm_felucca_load(const uint8_t *blob, size_t blob_len);
uint16_t refm_felucca_bpm(void);
uint8_t refm_felucca_pattern(void);
uint8_t refm_felucca_step(void);

static uint32_t usb_midi(uint8_t status, uint8_t d1, uint8_t d2) {
    return (uint32_t)(status >> 4) | ((uint32_t)status << 8) |
           ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24);
}

int main(void) {
    int32_t audio[128 * 2];
    uint8_t blob[1024];
    size_t written = 0;

    refm_felucca_init(132u);
    assert(refm_felucca_bpm() == 132u);

    /* ACID 1 note on through Felucca's normalized USB-MIDI packet format. */
    refm_felucca_midi_packet(usb_midi(0x90u, 60u, 110u));
    refm_felucca_audio_block(audio, 128u);
    int nonzero = 0;
    for (unsigned i = 0; i < 256u; ++i) if (audio[i]) { nonzero = 1; break; }
    assert(nonzero);

    /* Realtime clock is mirrored without losing channel running state. */
    refm_felucca_midi_packet((uint32_t)0xF8u << 8 | 0x0Fu);
    refm_felucca_midi_packet((uint32_t)0xFAu << 8 | 0x0Fu);
    for (unsigned i = 0; i < 6u; ++i)
        refm_felucca_midi_packet((uint32_t)0xF8u << 8 | 0x0Fu);
    assert(refm_felucca_step() == 1u);

    refm_felucca_use_internal_clock(140u);
    assert(refm_felucca_bpm() == 140u);

    assert(refm_felucca_save(blob, sizeof(blob), &written) == 0);
    assert(written > 16u);
    refm_felucca_init(90u);
    assert(refm_felucca_load(blob, written) == 0);
    assert(refm_felucca_bpm() == 140u);

    refm_felucca_panic();
    return 0;
}
