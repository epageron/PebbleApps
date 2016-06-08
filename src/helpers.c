#include "helpers.h"

Window *window;

static uint32_t const long_vibe_segments[] = { 200, 100, 200, 100, 100, 100, 100 };
static uint32_t const short_vibe_segments[] = { 100, 100, 100 };

void get_time(struct tm *t) {
    time_t temp = time(NULL);
    memcpy(t,localtime(&temp),sizeof(struct tm));
}

void print_time(myTextLayer *mtl, struct tm *t) {
  strftime(mtl->buffer,mtl->buffer_len,"%T",t);
  text_layer_set_text(mtl->tl_obj,mtl->buffer);
}

void print_duration(myTextLayer *mtl, int d, bool nosign) {
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

void init_text_buffer(myTextLayer *mtl, char *default_buf)
{
    size_t l = 1+strlen(default_buf)*2;
    mtl->buffer = (char *)malloc(l);
    mtl->initial_str = (char *)malloc(l);
    mtl->buffer_len = l;
    strcpy(mtl->buffer,default_buf);
    strcpy(mtl->initial_str,default_buf);
    text_layer_set_text(mtl->tl_obj,default_buf);
}

myTextLayer *addTextLayer(int align, int font_size, int y, GColor c, char *init_string) {
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

void resetTextLayer(myTextLayer *mtl) {
    strcpy(mtl->buffer,mtl->initial_str);
    text_layer_set_text(mtl->tl_obj,mtl->buffer);
}

void destroyTextLayer(myTextLayer *mtl) {
  text_layer_destroy(mtl->tl_obj);
  free(mtl->buffer);
  free(mtl->initial_str);
}

void stop_vibrate() {
   vibes_cancel();
}

void vibrate(bool long_vibe) {
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

myMenuLayer *initMenu(const char *title) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"initialize menu %s",title);
  myMenuLayer *ml = (myMenuLayer *)malloc(sizeof(myMenuLayer));
  ml->window = NULL;
  ml->menu_section = (SimpleMenuSection *)malloc(sizeof(SimpleMenuSection));
  ml->menu_section->num_items = 0;
  ml->menu_section->items = NULL;
  ml->menu_section->title = (char *)malloc(1+(strlen(title) * 2));
  strcpy((char *)ml->menu_section->title,title);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Menu %s initialized",title);
  
  return ml;
}

void addMenuItem(myMenuLayer *ml,const char *title,const char *stitle,SimpleMenuLayerSelectCallback cb) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Adding item %s to menu %s",title,ml->menu_section->title);
  SimpleMenuItem *smi;
  ml->menu_section->num_items++;
  uint32_t n = ml->menu_section->num_items;
  if(ml->menu_section->items==NULL) {
    ml->menu_section->items = (SimpleMenuItem *)malloc(sizeof(SimpleMenuItem));
  } else {
    ml->menu_section->items = (SimpleMenuItem *)realloc((SimpleMenuItem *)ml->menu_section->items, n * sizeof(SimpleMenuItem));
  }
  smi = (SimpleMenuItem *)&(ml->menu_section->items[n-1]);
  smi->title = (char *)malloc(1+(strlen(title) * 2));
  strcpy((char *)smi->title,title);
  if(stitle!=NULL) {
    smi->subtitle = (char *)malloc(1+(strlen(stitle) * 2));
    strcpy((char *)smi->subtitle,stitle);
  } else {
    smi->subtitle = NULL;
  }
  smi->icon = NULL;
  smi->callback = cb;
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Menu item %s added to menu %s",title,ml->menu_section->title);
}

void refreshMenuItem(myMenuLayer *ml,int item_idx, const char *new_title,const char *new_stitle) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Editing item %d to menu %s",item_idx,ml->menu_section->title);
  if((item_idx >= 0)&&(item_idx < (int)ml->menu_section->num_items)) {
    SimpleMenuItem *smi = (SimpleMenuItem *)&(ml->menu_section->items[item_idx]);
    if(smi->title)
      free((char *)smi->title);
    smi->title = (char *)malloc(1+(strlen(new_title) * 2));
    strcpy((char *)smi->title,new_title);
    if(new_stitle!=NULL) {
      if(smi->subtitle)
        free((char *)smi->subtitle);
      smi->subtitle = (char *)malloc(1+(strlen(new_stitle) * 2));
      strcpy((char *)smi->subtitle,new_stitle);
    } else {
      if(smi->subtitle)
        free((char *)smi->subtitle);
      smi->subtitle = NULL;
    }
  }
  menu_layer_reload_data(simple_menu_layer_get_menu_layer(ml->menu_layer));
}

void displayMenu(myMenuLayer *ml) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Displaying menu %s",ml->menu_section->title);
  if(ml->window==NULL) {
    ml->window = window_create();
    Layer *window_layer = window_get_root_layer(ml->window);
    GRect bounds = layer_get_bounds(window_layer);
    ml->menu_layer = simple_menu_layer_create(bounds,ml->window,ml->menu_section, 1, ml);
    layer_add_child(window_layer, simple_menu_layer_get_layer(ml->menu_layer));
  }
  window_stack_push(ml->window,true);
}

void freeMenu(myMenuLayer *ml) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Releasing menu resources for %s",ml->menu_section->title);
  
  if(ml->menu_layer) {
    simple_menu_layer_destroy(ml->menu_layer);
    ml->menu_layer = NULL;
  }
  if(ml->window)
    window_destroy(ml->window);

  if(ml->menu_section!=NULL) {
    if(ml->menu_section->items!=NULL) {
      uint32_t i = 0;
      SimpleMenuItem *smi;
      while(i < ml->menu_section->num_items) {
        smi = (SimpleMenuItem *)&(ml->menu_section->items[i]);
        if(smi->title!=NULL)
          free((char *)smi->title);
        if(smi->subtitle!=NULL) 
          free((char *)smi->subtitle);
        i++;
      }
      free((SimpleMenuItem *)ml->menu_section->items);
    }
    free((void *)ml->menu_section->title);
    free(ml->menu_section);
    ml->menu_section = NULL;
  }
}
  