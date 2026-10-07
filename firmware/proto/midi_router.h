// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_MIDI_ROUTER_H
#define REFM_MIDI_ROUTER_H
#include <stdint.h>
typedef enum{MIDI_TARGET_NONE=0,MIDI_TARGET_ACID1,MIDI_TARGET_ACID2,MIDI_TARGET_DRUM808,MIDI_TARGET_DRUM909}midi_target_t;typedef enum{MIDI_EVENT_NONE=0,MIDI_EVENT_NOTE_ON,MIDI_EVENT_NOTE_OFF,MIDI_EVENT_CC,MIDI_EVENT_PROGRAM}midi_event_type_t;typedef struct{midi_event_type_t type;midi_target_t target;uint8_t data1,data2;}midi_route_event_t;typedef struct{uint8_t acid1_ch,acid2_ch,drum808_ch,drum909_ch;uint8_t channel10_target;}midi_router_t;
void midi_router_init(midi_router_t*r);midi_route_event_t midi_router_decode(const midi_router_t*r,uint8_t status,uint8_t data1,uint8_t data2);int midi_router_drum_voice(uint8_t note);
#endif
