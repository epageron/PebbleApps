#include "helpers.h"

Window *window;

static uint32_t const long_vibe_segments[] = { 200, 100, 200, 100, 100, 100, 100 };
static uint32_t const short_vibe_segments[] = { 100, 100, 100 };

void get_time(struct tm *t) {
    time_t temp = time(NULL);
    memcpy(t,localtime(&temp),sizeof(struct tm));
}

void hide_seconds(myTimeLayer *mtl) {
  Layer *l = text_layer_get_layer(mtl->exp_digits->tl_obj);
  if(!layer_get_hidden(l)) {
    layer_set_hidden(l,true);
    
    l = text_layer_get_layer(mtl->big_digits->tl_obj);
    GRect bounds = layer_get_frame(l);
    bounds.origin.x += bounds.size.w / 4;
    layer_set_frame(l,bounds);
    }
}

void show_seconds(myTimeLayer *mtl) {
  
  Layer *l = text_layer_get_layer(mtl->exp_digits->tl_obj);
  if(layer_get_hidden(l)) {
    layer_set_hidden(l,false);
    
    l = text_layer_get_layer(mtl->big_digits->tl_obj);
    GRect bounds = layer_get_frame(l);
    bounds.origin.x -= bounds.size.w / 4;
    layer_set_frame(l,bounds);
  }
}

void print_time(myTimeLayer *mtl, struct tm *t) {
  strftime(mtl->big_digits->buffer,mtl->big_digits->buffer_len,"%H:%M",t);
  text_layer_set_text(mtl->big_digits->tl_obj,mtl->big_digits->buffer);
  strftime(mtl->exp_digits->buffer,mtl->exp_digits->buffer_len,"%S",t);
  text_layer_set_text(mtl->exp_digits->tl_obj,mtl->exp_digits->buffer);
  show_seconds(mtl);
}

void print_duration(myTimeLayer *mtl, int d, bool nosign) {
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
      snprintf(mtl->big_digits->buffer,mtl->big_digits->buffer_len,"%s%02d:%02d",sign,d_min,d_sec);
      text_layer_set_text(mtl->big_digits->tl_obj,mtl->big_digits->buffer);
      hide_seconds(mtl);
    } else {
      snprintf(mtl->big_digits->buffer,mtl->big_digits->buffer_len,"%s%02d:%02d",sign,d_hrs,d_min);
      snprintf(mtl->exp_digits->buffer,mtl->exp_digits->buffer_len,"%02d",d_sec);
      text_layer_set_text(mtl->big_digits->tl_obj,mtl->big_digits->buffer);
      text_layer_set_text(mtl->exp_digits->tl_obj,mtl->exp_digits->buffer);
      show_seconds(mtl);
    }
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

void changeTextFont(myTextLayer *mtl, int font_size) {
  int font_height;
  int font_id;
  int delta_y = 0;
  
  if(font_size==-1) {
    font_height = 10;
    font_id = RESOURCE_ID_FONT_XXS_10;
  }
  if(font_size==0) {
    font_height = 15;
    font_id = RESOURCE_ID_FONT_SMALL_14;
  }
  if(font_size==1) {
    font_height = 20;
    font_id = RESOURCE_ID_FONT_SMALL_18;
  }
  if(font_size==2) {
    font_height = 30;
    font_id = RESOURCE_ID_FONT_MEDIUM_22;
  }
  if(font_size==3) {
    font_height = 35;
    font_id = RESOURCE_ID_FONT_BIG_28;
  }
  if(font_size!=mtl->current_font_size) {
    delta_y = 2*(font_size - mtl->current_font_size);
  }
  
  mtl->current_font_size = font_size;
  Layer *l = text_layer_get_layer(mtl->tl_obj);
  GRect bounds = layer_get_frame(l);
  bounds.size.h = font_height;
  bounds.origin.y -= delta_y;
  layer_set_frame(l,bounds);
  text_layer_set_font(mtl->tl_obj, fonts_load_custom_font(resource_get_handle(font_id)));
}

void changeFont(myTimeLayer *mtl, int font_size) {
  
  if(font_size==1) {
    changeTextFont(mtl->big_digits,1);
    changeTextFont(mtl->exp_digits,-1);
  }
  if(font_size==2) {
    changeTextFont(mtl->big_digits,2);
    changeTextFont(mtl->exp_digits,0);
  }
  if(font_size==3) {
    changeTextFont(mtl->big_digits,3);
    changeTextFont(mtl->exp_digits,1);
  }
}

myTextLayer *addTextLayer(GRect bounds, GTextAlignment a, int font_size, int y, GColor c, char *init_string) {
  myTextLayer *mtl;
  
  mtl = (myTextLayer *)malloc(sizeof(myTextLayer));
  
  Layer *window_layer = window_get_root_layer(window);

  mtl->current_font_size = font_size;
  mtl->init_font_size = font_size;
  mtl->tl_obj = text_layer_create(bounds);
  text_layer_set_text_alignment(mtl->tl_obj, a);
  text_layer_set_background_color(mtl->tl_obj, GColorClear);
  text_layer_set_text_color(mtl->tl_obj,c);
  changeTextFont(mtl, font_size);
  layer_add_child(window_layer, text_layer_get_layer(mtl->tl_obj));
  init_text_buffer(mtl,init_string);
  
  return mtl;
}

void resetTextLayer(myTextLayer *mtl) {
    strcpy(mtl->buffer,mtl->initial_str);
    changeTextFont(mtl, mtl->init_font_size);
    text_layer_set_text(mtl->tl_obj,mtl->buffer);
}

void destroyTextLayer(myTextLayer *mtl) {
  text_layer_destroy(mtl->tl_obj);
  free(mtl->buffer);
  free(mtl->initial_str);
}

myTimeLayer *addTimeLayer(int align, int font_size, int y, GColor c) {
  
  myTimeLayer *mtl;
  
  mtl = (myTimeLayer *)malloc(sizeof(myTimeLayer));  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  int x1;
  int x2;
  int x3;
  int col_size = (bounds.size.w) / 12;
  switch(align) {
  case 2:
    x1 = 4 * col_size;
    x2 = 10 * col_size;
    x3 = 12 * col_size;
    break;
  case 1:
    x1 = 0 * col_size;
    x2 = 4 * col_size;
    x3 = 6 * col_size;
    break;
  default:
    x1 = 1 * col_size;
    x2 = 7 * col_size;
    x3 = 11 * col_size;
    break;
  }    
  mtl->big_digits = addTextLayer(GRect(x1, y, x2-x1, 20),GTextAlignmentRight,font_size,y,c," --:-- ");
  mtl->exp_digits = addTextLayer(GRect(x2, y, x3-x2, 10),GTextAlignmentLeft,font_size-2,y,c,"--");
  setTimeColor(mtl,c);
  return mtl;
}

void resetTimeLayer(myTimeLayer *mtl) {
  resetTextLayer(mtl->exp_digits);
  resetTextLayer(mtl->big_digits);
}

void destroyTimeLayer(myTimeLayer *mtl) {
  destroyTextLayer(mtl->exp_digits);
  destroyTextLayer(mtl->big_digits);
}

void setTextInTime(myTimeLayer *mtl, char *t) {
  text_layer_set_text(mtl->big_digits->tl_obj, t); 
  hide_seconds(mtl);
}

void setTimeColor(myTimeLayer *mtl, GColor c) {
  c = COLOR_FALLBACK(c, GColorWhite);
  text_layer_set_text_color(mtl->big_digits->tl_obj,c);
  text_layer_set_text_color(mtl->exp_digits->tl_obj,c);
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
  