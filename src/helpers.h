#pragma once
#include <pebble.h>

extern Window *window;

typedef struct {
  TextLayer *tl_obj;
  size_t buffer_len;
  char *buffer;
  char *initial_str;
} myTextLayer;

typedef struct {
  SimpleMenuSection *menu_section;
  SimpleMenuLayer   *menu_layer;
  Window            *window;
} myMenuLayer;

extern void get_time(struct tm *t);
extern void print_time(myTextLayer *mtl, struct tm *t);
extern void print_duration(myTextLayer *mtl, int d, bool nosign);
extern void init_text_buffer(myTextLayer *mtl, char *default_buf);

extern myTextLayer *addTextLayer(int align, int font_size, int y, GColor c, char *init_string);
extern void resetTextLayer(myTextLayer *mtl);
extern void destroyTextLayer(myTextLayer *mtl);

extern void stop_vibrate();
extern void vibrate(bool long_vibe);

extern myMenuLayer *initMenu(const char *title);
extern void addMenuItem(myMenuLayer *ml,const char *title,const char *stitle,SimpleMenuLayerSelectCallback cb);
extern void refreshMenuItem(myMenuLayer *ml,int item_idx, const char *new_title,const char *new_stitle);
extern void displayMenu(myMenuLayer *ml);
extern void freeMenu(myMenuLayer *ml);
