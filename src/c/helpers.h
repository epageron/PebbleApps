#pragma once
#include <pebble.h>

extern Window *window;

typedef struct {
  TextLayer *tl_obj;
  size_t buffer_len;
  char *buffer;
  char *initial_str;
  int  init_font_size;
  int  current_font_size;
} myTextLayer;

typedef struct {
  myTextLayer *big_digits;
  myTextLayer *exp_digits;
} myTimeLayer;

typedef struct {
  SimpleMenuSection *menu_section;
  SimpleMenuLayer   *menu_layer;
  Window            *window;
} myMenuLayer;

extern void get_time(struct tm *t);
extern void print_time(myTimeLayer *mtl, struct tm *t);
extern void print_duration(myTimeLayer *mtl, int d, bool nosign);

extern myTimeLayer *addTimeLayer(int align, int font_size, int y, GColor c);
extern void resetTimeLayer(myTimeLayer *mtl);
extern void destroyTimeLayer(myTimeLayer *mtl);
extern void changeFont(myTimeLayer *mtl, int font_size);
extern void setTimeColor(myTimeLayer *mtl, GColor c);
extern void setTextInTime(myTimeLayer *mtl, char *t);

extern void stop_vibrate();
extern void vibrate(bool long_vibe);

extern myMenuLayer *initMenu(const char *title);
extern void addMenuItem(myMenuLayer *ml,const char *title,const char *stitle,SimpleMenuLayerSelectCallback cb);
extern void refreshMenuItem(myMenuLayer *ml,int item_idx, const char *new_title,const char *new_stitle);
extern void displayMenu(myMenuLayer *ml);
extern void freeMenu(myMenuLayer *ml);
