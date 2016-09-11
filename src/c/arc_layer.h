#pragma once
#include <pebble.h>
#define OUTER_ARC     0
#define INNER_ARC     1

static Layer *arc_layer;

extern void init_arc_layer(Window *s_window, LayerUpdateProc lup);
extern void release_arc_layer();
extern void refresh_arc();
extern void draw_duration(GContext *ctx, uint8_t arc_id, uint16_t duration, GColor arc_color);
