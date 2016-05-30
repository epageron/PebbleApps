#include <pebble.h>

static Window *window;

static uint32_t const long_vibe_segments[] = { 200, 100, 200, 100, 100, 100, 100 };
static uint32_t const short_vibe_segments[] = { 100, 100, 100 };

typedef struct {
  TextLayer *tl_obj;
  size_t buffer_len;
  char *buffer;
  char *initial_str;
} myTextLayer;

static void get_time(struct tm *t) {
    time_t temp = time(NULL);
    memcpy(t,localtime(&temp),sizeof(struct tm));
}

static void print_time(myTextLayer *mtl, struct tm *t) {
  strftime(mtl->buffer,mtl->buffer_len,"%T",t);
  text_layer_set_text(mtl->tl_obj,mtl->buffer);
}

static void print_duration(myTextLayer *mtl, int d, bool nosign) {
    char sign[] = "+";

    if(nosign) {
      sign[0] = 0;
    } else {
      if(d < 0) {
        sign[0] = '-';
        d = -d;
      }
    }
      
    int d_min = (d % 3600) / 60;
    int d_sec = d % 60;
    int d_hrs = d / 3600;

    if(d_hrs==0) {
      snprintf(mtl->buffer,mtl->buffer_len,"%s%02d:%02d",sign,d_min,d_sec);
    } else {
      snprintf(mtl->buffer,mtl->buffer_len,"%s%02d:%02d:%02d",sign,d_hrs,d_min,d_sec);
    }
    text_layer_set_text(mtl->tl_obj,mtl->buffer);
}

static void init_text_buffer(myTextLayer *mtl, char *default_buf)
{
    size_t l = 1+strlen(default_buf)*2;
    mtl->buffer = (char *)malloc(l);
    mtl->initial_str = (char *)malloc(l);
    mtl->buffer_len = l;
    strcpy(mtl->buffer,default_buf);
    strcpy(mtl->initial_str,default_buf);
    text_layer_set_text(mtl->tl_obj,default_buf);
}

static myTextLayer *addTextLayer(int align, int font_size, int y, GColor c, char *init_string) {
  myTextLayer *mtl;
  
  mtl = (myTextLayer *)malloc(sizeof(myTextLayer));
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int font_height;
  const char *font_name;
  
  if(font_size==1) {
    font_height = 20;
    font_name = FONT_KEY_GOTHIC_18;
  }
  if(font_size==2) {
    font_height = 30;
    font_name = FONT_KEY_GOTHIC_24;
  }
  if(font_size==3) {
    font_height = 35;
    font_name = FONT_KEY_GOTHIC_28_BOLD;
  }
  if(align==0) {
    mtl->tl_obj = text_layer_create(GRect(0, y, bounds.size.w, font_height));
  }
  if(align==1){
    mtl->tl_obj = text_layer_create(GRect(0, y, bounds.size.w/2, font_height));
  }
  if(align==2){
    mtl->tl_obj = text_layer_create(GRect(bounds.size.w/2, y, bounds.size.w/2, font_height));
  }
  text_layer_set_text_alignment(mtl->tl_obj, GTextAlignmentCenter);
  text_layer_set_background_color(mtl->tl_obj, GColorClear);
  text_layer_set_text_color(mtl->tl_obj,c);
  text_layer_set_font(mtl->tl_obj, fonts_get_system_font(font_name));
  layer_add_child(window_layer, text_layer_get_layer(mtl->tl_obj));
  init_text_buffer(mtl,init_string);
  
  return mtl;
}

static void resetTextLayer(myTextLayer *mtl) {
    strcpy(mtl->buffer,mtl->initial_str);
    text_layer_set_text(mtl->tl_obj,mtl->buffer);
}

static void destroyTextLayer(myTextLayer *mtl) {
  text_layer_destroy(mtl->tl_obj);
  free(mtl->buffer);
  free(mtl->initial_str);
}

static void stop_vibrate() {
   vibes_cancel();
}

static void vibrate(bool long_vibe) {
// Vibe pattern: ON for 200ms, OFF for 100ms, ON for 400ms:
  if(long_vibe) {
    VibePattern pat = {
      .durations = long_vibe_segments,
      .num_segments = ARRAY_LENGTH(long_vibe_segments),
    };
    vibes_enqueue_custom_pattern(pat);  
  } else {
    VibePattern pat = {
      .durations = short_vibe_segments,
      .num_segments = ARRAY_LENGTH(short_vibe_segments),
    };
    vibes_enqueue_custom_pattern(pat);  
  }
}