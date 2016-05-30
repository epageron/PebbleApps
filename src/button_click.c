#include "helpers.h"

static myTextLayer *tl_current_time;
static myTextLayer *tl_leg_start_time;
static myTextLayer *tl_leg_est_end_time;
static myTextLayer *tl_leg_est_duration;
static myTextLayer *tl_leg_act_duration;
static myTextLayer *tl_leg_delta;
static myTextLayer *tl_flight_start_time;
static myTextLayer *tl_flight_duration;
static myTextLayer *tl_flight_max_time;
static myTextLayer *tl_flight_delta;

static struct tm start_time;
static struct tm end_time;
static struct tm last_wp_time;
static struct tm wp_time;
static int max_flight_duration;
static int estimated_leg_duration;
static int delta_leg_duration;
static int delta_flight_duration;
static bool flight_started;
static bool time_limit_overlapsed;
static bool b_leg_overlapsed;
static bool b_flight_end_warning;
static bool start_new_leg;
static AppTimer *limit_timer;
static AppTimer *leg_timer;

const time_t FLIGHT_TIME_STEP = 10;       // In minutes
const time_t LEG_TIME_STEP = 30;          // In seconds
const time_t LEG_CONFIG_DURATION = 5;     // In seconds
const time_t INITIAL_LEG_DURATION = 8 * 60; // In seconds
const int LONG_CLICK_DELAY = 2000;        // In milliseconds

#define THRESHOLD(x)  (2*x)/3                 // Used to compute thresholds for warning

static void init_flight()
{
    flight_started = false;
    time_limit_overlapsed = false;
    start_new_leg = true; // True means that a click on down btn will start a new leg
    
    get_time(&start_time);

    limit_timer = NULL;
  
    max_flight_duration = 0;
    estimated_leg_duration = 0;
    delta_leg_duration = 0;
    delta_flight_duration = 0;
    b_flight_end_warning = false;
  
    resetTextLayer(tl_flight_start_time);
    resetTextLayer(tl_flight_duration);
    resetTextLayer(tl_leg_start_time);
    resetTextLayer(tl_leg_delta);
    resetTextLayer(tl_leg_est_end_time);
    resetTextLayer(tl_leg_est_duration);
    resetTextLayer(tl_leg_act_duration);
    resetTextLayer(tl_flight_max_time);
    resetTextLayer(tl_flight_delta);
}

static void print_actual_leg_duration() {
    struct tm tick_time;
    get_time(&tick_time);
  
    time_t actual_leg_duration = mktime(&tick_time) - mktime(&wp_time);
    print_duration(tl_leg_act_duration,actual_leg_duration,true);
    if((estimated_leg_duration > 0)&&(actual_leg_duration > estimated_leg_duration)&&!b_leg_overlapsed) {
        b_leg_overlapsed = true;
        vibrate(false);
        text_layer_set_text_color(tl_leg_act_duration->tl_obj,GColorFolly);
    }  
}

static void print_actual_flight_duration() {
    struct tm tick_time;
    get_time(&tick_time);
  
    time_t actual_f_duration = mktime(&tick_time) - mktime(&start_time);
    print_duration(tl_flight_duration,actual_f_duration,true);
    if((max_flight_duration > 0)&&(actual_f_duration > THRESHOLD(max_flight_duration))&&!b_flight_end_warning) {
      b_flight_end_warning = true;
      vibrate(false);
      text_layer_set_text_color(tl_flight_duration->tl_obj,GColorFolly);
    }
}

static void print_estimated_leg_end_time() {
  struct tm *t;
  time_t et = mktime(&wp_time) + estimated_leg_duration;
    
  t = localtime(&et);
  print_time(tl_leg_est_end_time, t);
}

static void print_estimated_leg_duration() {
    int eld_min = estimated_leg_duration / 60;
    int eld_sec = estimated_leg_duration % 60;

    if(start_new_leg) {
      snprintf(tl_leg_est_duration->buffer,tl_leg_est_duration->buffer_len,">%02d:%02d<",eld_min,eld_sec);
    } else {
      snprintf(tl_leg_est_duration->buffer,tl_leg_est_duration->buffer_len," %02d:%02d ",eld_min,eld_sec);
    }
    text_layer_set_text(tl_leg_est_duration->tl_obj,tl_leg_est_duration->buffer);
    print_estimated_leg_end_time();
    app_timer_reschedule(leg_timer, LEG_CONFIG_DURATION * 1000);
}

static void print_delta_leg_duration() {
    if(estimated_leg_duration > 0)
    {
      delta_leg_duration = (mktime(&wp_time) - mktime(&last_wp_time)) - estimated_leg_duration;
      delta_flight_duration += delta_leg_duration;
      if(delta_leg_duration <= 0) {
        text_layer_set_text_color(tl_leg_delta->tl_obj,GColorGreen);
      }
      else {
        text_layer_set_text_color(tl_leg_delta->tl_obj,GColorFolly);
      }
      if(delta_flight_duration <= 0) {
        text_layer_set_text_color(tl_flight_delta->tl_obj,GColorGreen);
      }
      else {
        text_layer_set_text_color(tl_flight_delta->tl_obj,GColorFolly);
      }
      print_duration(tl_leg_delta,delta_leg_duration,false);
      print_duration(tl_flight_delta,delta_flight_duration,false);
    }
    else {
      resetTextLayer(tl_leg_delta);
      text_layer_set_text_color(tl_leg_delta->tl_obj,GColorWhite);
    }
}

static void print_flight_max_time() {
  struct tm *et;

  if(flight_started) {
      time_t t = mktime(&start_time) + max_flight_duration;
        
      et = localtime(&t);
      print_time(tl_flight_max_time, et);
  }
  else {
    print_duration(tl_flight_max_time,max_flight_duration,true);
  }
}

static void flight_limit_is_reached() {
   time_limit_overlapsed = true;
   vibrate(true);
   text_layer_set_text_color(tl_current_time->tl_obj,GColorFolly);
}

static void leg_is_started() {
   APP_LOG(APP_LOG_LEVEL_DEBUG,"Leg duration fixed");
   start_new_leg = true;
   if(estimated_leg_duration < 0) {
      resetTextLayer(tl_leg_est_end_time);
   }
   else {
      print_estimated_leg_duration();
   }
}

static void waypoint_top() {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Top waypoint");
    
    last_wp_time = wp_time;
    get_time(&wp_time);
    print_time(tl_leg_start_time, &wp_time);
    print_delta_leg_duration();
    estimated_leg_duration = -1;
    start_new_leg = false;
    b_leg_overlapsed = false;
    leg_timer = app_timer_register(LEG_CONFIG_DURATION * 1000,leg_is_started,NULL);
    text_layer_set_text_color(tl_leg_act_duration->tl_obj,GColorWhite);
    text_layer_set_text(tl_leg_est_end_time->tl_obj, " ");
    resetTextLayer(tl_leg_est_duration);
}

static void start_flight() {
    window_set_background_color(window, GColorBlack);
    text_layer_set_text_color(tl_flight_delta->tl_obj,GColorWhite);
    text_layer_set_text_color(tl_flight_duration->tl_obj,GColorWhite);
    text_layer_set_text_color(tl_flight_max_time->tl_obj,GColorOrange);

    flight_started = true;
    get_time(&start_time);
    wp_time = start_time;
    print_time(tl_flight_start_time, &start_time);
    delta_leg_duration = 0;
    delta_flight_duration = 0;
    time_limit_overlapsed = false;
    resetTextLayer(tl_flight_delta);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Starting flight...");

    if(max_flight_duration > 0) {
      limit_timer = app_timer_register(max_flight_duration * 1000,flight_limit_is_reached,NULL);
      print_flight_max_time();
    } else {
      text_layer_set_text(tl_flight_max_time->tl_obj," ");
    }
  
    waypoint_top();
    vibes_short_pulse();
}

static void end_flight() {
    window_set_background_color(window, GColorBulgarianRose);
    text_layer_set_text_color(tl_leg_delta->tl_obj,GColorWhite);
    text_layer_set_text_color(tl_leg_act_duration->tl_obj,GColorWhite);
  
    flight_started = false;
    get_time(&end_time);
    print_time(tl_flight_max_time, &end_time);

    if(time_limit_overlapsed) {
      text_layer_set_text_color(tl_current_time->tl_obj,GColorWhite);
      text_layer_set_text_color(tl_flight_max_time->tl_obj,GColorFolly);
    }
      
    resetTextLayer(tl_leg_start_time);
    resetTextLayer(tl_leg_delta);
    resetTextLayer(tl_leg_act_duration);
    resetTextLayer(tl_leg_est_duration);
    resetTextLayer(tl_leg_est_end_time);
  
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Ending flight...");
  
    if(limit_timer != NULL) {
      app_timer_cancel(limit_timer);
    }      
    vibes_short_pulse();
}

static void decrease_estimated_leg_duration() {
    estimated_leg_duration -= LEG_TIME_STEP;
    if(estimated_leg_duration < 0)
      estimated_leg_duration = INITIAL_LEG_DURATION;
    print_estimated_leg_duration();
}

static void increase_estimated_leg_duration() {
    if(estimated_leg_duration < 0)
      estimated_leg_duration = INITIAL_LEG_DURATION;
    estimated_leg_duration += LEG_TIME_STEP;
    print_estimated_leg_duration();
}

static void decrease_time_limit() {
    max_flight_duration -= FLIGHT_TIME_STEP * 60;
    print_flight_max_time();
}

static void increase_time_limit() {
    max_flight_duration += FLIGHT_TIME_STEP * 60;
    print_flight_max_time();
}

static void long_click_up_btn(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Up button pressed long");
  init_flight();  
  vibes_double_pulse();
}

static void single_click_up_btn(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Up button pressed");
  if(flight_started) {
      end_flight();
  }
  else {
      start_flight();
  }
}

static void single_click_sel_btn(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Select button pressed");
  if(flight_started) {
      if(!start_new_leg)
      {
        decrease_estimated_leg_duration();
      }
  }
  else {
      decrease_time_limit();
  }
}

static void single_click_dw_btn(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Down button pressed");
  if(flight_started) {
      if(start_new_leg) {
        waypoint_top();
      }
      else {
        increase_estimated_leg_duration();
      }
  }
  else {
      increase_time_limit();
  }
}

static void my_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  print_time(tl_current_time, tick_time);

  if(flight_started)
  {
    print_actual_leg_duration();
    print_actual_flight_duration();
  }
}

static void click_config_provider(void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Configuring clicks...");
  
  window_single_click_subscribe(BUTTON_ID_UP, single_click_up_btn);
  window_single_click_subscribe(BUTTON_ID_SELECT, single_click_sel_btn);
  window_single_click_subscribe(BUTTON_ID_DOWN, single_click_dw_btn);
  window_long_click_subscribe(BUTTON_ID_UP, LONG_CLICK_DELAY, long_click_up_btn , NULL);
}

static void window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Loading window...");

  tl_flight_start_time = addTextLayer(0,3,0,GColorGreen,"--:--:--");
  tl_flight_duration = addTextLayer(0,2,25,GColorWhite,"--:--:--");
  tl_leg_start_time = addTextLayer(1,2,46,GColorWhite,"--:--:--");
  tl_current_time = addTextLayer(1,2,67,GColorWhite,"--:--:--");
  tl_leg_est_end_time = addTextLayer(1,2,88,GColorWhite,"--:--:--");

  tl_leg_delta = addTextLayer(2,1,54,GColorWhite,"---:--");
  tl_leg_act_duration = addTextLayer(2,3,65,GColorWhite,"--:--");
  tl_leg_est_duration = addTextLayer(2,1,90,GColorWhite," --:-- ");
  tl_flight_delta = addTextLayer(0,2,108,GColorWhite,"---:--");
  tl_flight_max_time = addTextLayer(0,3,130,GColorOrange,"--:--:--");
    
  init_flight();
  
  tick_timer_service_subscribe(SECOND_UNIT, my_tick_handler);
}

static void window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Unloading window...");
  
  destroyTextLayer(tl_current_time);
  destroyTextLayer(tl_flight_start_time);
  destroyTextLayer(tl_leg_delta);
  destroyTextLayer(tl_leg_start_time);
  destroyTextLayer(tl_leg_est_duration);
  destroyTextLayer(tl_leg_act_duration);
  destroyTextLayer(tl_leg_est_end_time);
  destroyTextLayer(tl_flight_duration);
  destroyTextLayer(tl_flight_delta);
  destroyTextLayer(tl_flight_max_time);
  
  tick_timer_service_unsubscribe();
}

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Init...");
  window = window_create();
  window_set_background_color(window, GColorBulgarianRose);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(window, click_config_provider);
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Exit...");
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}