// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_GROOVEBOX_MIDI_H
#define REFM_GROOVEBOX_MIDI_H
#include <stdint.h>
#include "groovebox.h"
#include "midi_router.h"
void groovebox_midi_init(midi_router_t*router);uint8_t groovebox_handle_channel_midi(groovebox_t*g,midi_router_t*router,uint8_t status,uint8_t data1,uint8_t data2);
#endif
