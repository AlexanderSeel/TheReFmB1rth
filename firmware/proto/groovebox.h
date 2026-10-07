// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_GROOVEBOX_H
#define REFM_GROOVEBOX_H
#include <stddef.h>
#include <stdint.h>
#include "acid303.h"
#include "drum_machine.h"
#include "midi_transport.h"
#include "mixer_fx.h"
#include "pattern_bank.h"
#include "project_store.h"
#include "seq16.h"
#include "song.h"
#define GROOVEBOX_DRUM_TRACKS 2u
#define GROOVEBOX_STEPS 16u
#define GROOVEBOX_PROJECT_PAYLOAD_MAX 3072u
typedef struct {acid303_t acid[2];seq16_t acid_seq[2];drum_machine_t drum[2];uint16_t drum_hits[2][16];uint16_t drum_accents[2][16];pattern_bank_t patterns;mixer_fx_t mixer;midi_transport_t transport;song_t song;uint8_t step,current_pattern,external_clock;} groovebox_t;
void groovebox_init(groovebox_t*g,uint16_t bpm);void groovebox_set_external_clock(groovebox_t*g,uint8_t external);void groovebox_set_drum_step(groovebox_t*g,uint8_t drum_track,uint8_t step,uint16_t hits,uint16_t accents);uint32_t groovebox_feed_midi_realtime(groovebox_t*g,uint8_t status);void groovebox_advance_step(groovebox_t*g);void groovebox_process(groovebox_t*g,int16_t*left,int16_t*right);int groovebox_save(const groovebox_t*g,uint8_t*out,size_t capacity,size_t*written);int groovebox_load(groovebox_t*g,const uint8_t*blob,size_t blob_len);
#endif
