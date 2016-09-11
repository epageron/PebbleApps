#include <pebble.h>
#include "arc_layer.h"

#define TIME_ANGLE(time) time * (TRIG_MAX_ANGLE / 3600)
#define ARC_PADDING      2
#define ARC_THICKNESS    7

// Draw an arc with given inner/outer radii
static void draw_arc(GContext *ctx,
                     GRect rect,
                     uint16_t thickness,
                     uint32_t end_angle) {
  if (end_angle == 0) {
    graphics_fill_radial(ctx,
                         rect,
                         GOvalScaleModeFitCircle,
                         thickness,
                         0,
                         TRIG_MAX_ANGLE);
  } else {
    graphics_fill_radial(ctx,
                         rect,
                         GOvalScaleModeFitCircle,
                         thickness,
                         0,
                         end_angle);
  }
}

static GRect calculate_rect(uint8_t arc_id) {
  return grect_inset(layer_get_bounds(arc_layer), GEdgeInsets(ARC_PADDING*(3*arc_id)));
}

static void set_color(GContext *ctx, GColor color) {
  graphics_context_set_fill_color(ctx, color);
}

void init_arc_layer(Window *s_window, LayerUpdateProc lup) {
  arc_layer = layer_create(layer_get_bounds(window_get_root_layer(s_window)));
  layer_add_child(window_get_root_layer(s_window), arc_layer);
  layer_set_update_proc(arc_layer, lup);  
}

void release_arc_layer() {
  layer_destroy(arc_layer);  
}

void refresh_arc() {
  layer_mark_dirty(arc_layer);  
}

// Handle representation for flight max duration
void draw_duration(GContext *ctx, uint8_t arc_id, uint16_t duration, GColor arc_color) {
    int32_t arc_angle = TIME_ANGLE(duration);
    GColor c =  PBL_IF_COLOR_ELSE(arc_color,(arc_id==OUTER_ARC)?GColorLightGray:GColorDarkGray);
    set_color(ctx,c);
    draw_arc(ctx, calculate_rect(arc_id), ARC_THICKNESS, arc_angle);
}
