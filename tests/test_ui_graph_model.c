// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/ui_graph_model.h"
static void bounded(const ui_graph_curve_t *g){for(int i=0;i<UI_GRAPH_POINTS;i++) assert(g->y[i]>=-32768&&g->y[i]<=32767);}int main(void){ui_graph_curve_t g; ui_graph_adsr(&g,32,64,80,48); bounded(&g); assert(g.y[0]==0); ui_graph_lfo(&g,UI_LFO_TRI,0,127); bounded(&g); ui_graph_filter(&g,80,100,64); bounded(&g); puts("ui graph model tests: ok"); return 0;}
