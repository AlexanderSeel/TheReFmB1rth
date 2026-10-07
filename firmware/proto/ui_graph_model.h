// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_UI_GRAPH_MODEL_H
#define REFM_UI_GRAPH_MODEL_H
#include <stdint.h>
#define UI_GRAPH_POINTS 64
typedef enum { UI_LFO_SINE, UI_LFO_TRI, UI_LFO_SAW, UI_LFO_SQUARE } ui_lfo_shape_t;
typedef struct { int16_t y[UI_GRAPH_POINTS]; } ui_graph_curve_t;
void ui_graph_adsr(ui_graph_curve_t *g,uint8_t attack,uint8_t decay,uint8_t sustain,uint8_t release);
void ui_graph_lfo(ui_graph_curve_t *g,ui_lfo_shape_t shape,uint8_t phase,uint8_t amount);
void ui_graph_filter(ui_graph_curve_t *g,uint8_t cutoff,uint8_t resonance,uint8_t env_amount);
#endif
