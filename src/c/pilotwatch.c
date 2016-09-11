#include "helpers.h"
#include "settings.h"
#include "arc_layer.h"

static myTimeLayer *tl_current_time;
static myTimeLayer *tl_leg_start_time;
static myTimeLayer *tl_leg_est_end_time;
static myTimeLayer *tl_leg_est_duration;
static myTimeLayer *tl_leg_act_duration;
static myTimeLayer *tl_leg_delta;
static myTimeLayer *tl_flight_start_time;
static myTimeLayer *tl_flight_duration;
static myTimeLayer *tl_flight_max_time;
static myTimeLayer *tl_flight_delta;
static myTimeLayer *tl_arc_time;

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

const int LONG_CLICK_DELAY = 2000;        // In milliseconds

#define THRESHOLD(x)  (2*x)/3                 // Used to compute thresholds for warning
#define ARC_FLIGHT     OUTER_ARC
#define ARC_LEG        INNER_ARC

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
  
    resetTimeLayer(tl_flight_start_time);
    resetTimeLayer(tl_flight_duration);
    resetTimeLayer(tl_leg_start_time);
    resetTimeLayer(tl_leg_delta);
    resetTimeLayer(tl_leg_est_end_time);
    resetTimeLayer(tl_leg_est_duration);
    resetTimeLayer(tl_leg_act_duration);
    resetTimeLayer(tl_flight_max_time);
    resetTimeLayer(tl_flight_delta);
    resetTimeLayer(tl_arc_time);
}

static void update_arc_display(Layer *l, GContext *ctx) {
  struct tm tick_time;
  get_time(&tick_time);
  
  if(max_flight_duration > 0) {
    time_t remaining_flight_duration;
    remaining_flight_duration = max_flight_duration - (mktime(&tick_time) - mktime(&start_time));
    if(remaining_flight_duration > 0)
      draw_duration(ctx, ARC_FLIGHT, remaining_flight_duration, (remaining_flight_duration < 60)?GColorDarkCandyAppleRed:GColorMidnightGreen);
    else
      draw_duration(ctx, ARC_FLIGHT, 0, GColorDarkCandyAppleRed);
  } else {
    time_t current_flight_duration;
    current_flight_duration = (mktime(&tick_time) - mktime(&start_time));
    if(current_flight_duration > 0)
      draw_duration(ctx, ARC_FLIGHT, current_flight_duration,GColorMidnightGreen);
  }
    
  
  if(flight_started) {
    if(estimated_leg_duration > 0) {
      time_t remaining_leg_duration = estimated_leg_duration - (mktime(&tick_time) - mktime(&wp_time));
      if(remaining_leg_duration > 0) {
        print_duration(tl_arc_time, remaining_leg_duration, true);
        draw_duration(ctx, ARC_LEG, remaining_leg_duration, (remaining_leg_duration < 60)?GColorDarkCandyAppleRed:GColorImperialPurple);
      } else {
        draw_duration(ctx, ARC_LEG, 0, GColorDarkCandyAppleRed);
        setTextInTime(tl_arc_time, "OVER");
      }
    } else {
      time_t current_leg_duration;
      current_leg_duration = (mktime(&tick_time) - mktime(&wp_time));
      if(current_leg_duration > 0)
        draw_duration(ctx, ARC_LEG, current_leg_duration,GColorImperialPurple);
    }  
  }
}

static void print_actual_leg_duration() {
    struct tm tick_time;
    get_time(&tick_time);
  
    time_t actual_leg_duration = mktime(&tick_time) - mktime(&wp_time);
    print_duration(tl_leg_act_duration,actual_leg_duration,true);
    if((estimated_leg_duration > 0)&&(actual_leg_duration > estimated_leg_duration)&&!b_leg_overlapsed) {
        b_leg_overlapsed = true;
        vibrate(false);
        setTimeColor(tl_leg_act_duration,GColorFolly);
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
      setTimeColor(tl_flight_duration,GColorFolly);
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

    changeFont(tl_leg_est_duration,3);
    changeFont(tl_leg_act_duration,1);
  
    if(start_new_leg) {
      snprintf(tl_leg_est_duration->big_digits->buffer,tl_leg_est_duration->big_digits->buffer_len,">%02d:%02d<",eld_min,eld_sec);
    } else {
      snprintf(tl_leg_est_duration->big_digits->buffer,tl_leg_est_duration->big_digits->buffer_len," %02d:%02d ",eld_min,eld_sec);
    }
    setTextInTime(tl_leg_est_duration,tl_leg_est_duration->big_digits->buffer);
    print_estimated_leg_end_time();
    app_timer_reschedule(leg_timer, setting_values[PRM_LEG_CONFIG_DURATION] * 1000);
}

static void print_delta_leg_duration() {
    if(estimated_leg_duration > 0)
    {
      delta_leg_duration = (mktime(&wp_time) - mktime(&last_wp_time)) - estimated_leg_duration;
      delta_flight_duration += delta_leg_duration;
      if(delta_leg_duration <= 0) {
        setTimeColor(tl_leg_delta,GColorGreen);
      }
      else {
        setTimeColor(tl_leg_delta,GColorFolly);
      }
      if(delta_flight_duration <= 0) {
        setTimeColor(tl_flight_delta,GColorGreen);
      }
      else {
        setTimeColor(tl_flight_delta,GColorFolly);
      }
      print_duration(tl_leg_delta,delta_leg_duration,false);
      print_duration(tl_flight_delta,delta_flight_duration,false);
    }
    else {
      resetTimeLayer(tl_leg_delta);
      setTimeColor(tl_leg_delta,GColorWhite);
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
    // To force arc to display the exact flight max duration
    get_time(&start_time);
  }
}

static void flight_limit_is_reached() {
   time_limit_overlapsed = true;
   vibrate(true);
   setTimeColor(tl_current_time,GColorFolly);
}

static void leg_is_started() {
  if(flight_started) {
   APP_LOG(APP_LOG_LEVEL_DEBUG,"Leg duration fixed");
   start_new_leg = true;
   if(estimated_leg_duration < 0) {
      resetTimeLayer(tl_leg_est_end_time);
   }
   else {
      print_estimated_leg_duration();
      changeFont(tl_leg_est_duration,1);
      changeFont(tl_leg_act_duration,3);
   }
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
    leg_timer = app_timer_register(setting_values[PRM_LEG_CONFIG_DURATION] * 1000,leg_is_started,NULL);
    setTimeColor(tl_leg_act_duration,GColorWhite);
    setTextInTime(tl_leg_est_end_time, " ");
    setTextInTime(tl_arc_time," ");
    resetTimeLayer(tl_leg_est_duration);
}

static void start_flight() {
    window_set_background_color(window, GColorBlack);
    setTimeColor(tl_flight_delta,GColorWhite);
    setTimeColor(tl_flight_duration,GColorWhite);
    setTimeColor(tl_flight_max_time,GColorOrange);
    setTimeColor(tl_arc_time, GColorPurpureus);
    changeFont(tl_flight_max_time, 2);
    changeFont(tl_flight_duration,3);
    flight_started = true;
    get_time(&start_time);
    wp_time = start_time;
    print_time(tl_flight_start_time, &start_time);
    delta_leg_duration = 0;
    delta_flight_duration = 0;
    time_limit_overlapsed = false;
    resetTimeLayer(tl_flight_delta);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Starting flight...");

    if(max_flight_duration > 0) {
      limit_timer = app_timer_register(max_flight_duration * 1000,flight_limit_is_reached,NULL);
      print_flight_max_time();
    } else {
      setTextInTime(tl_flight_max_time," ");
    }
  
    waypoint_top();
    vibes_short_pulse();
}

static void end_flight() {
    window_set_background_color(window, GColorBulgarianRose);
    setTimeColor(tl_leg_delta,GColorWhite);
    setTimeColor(tl_leg_act_duration,GColorWhite);
    changeFont(tl_flight_duration,2);
  
    flight_started = false;
    get_time(&end_time);
    print_time(tl_flight_max_time, &end_time);
    waypoint_top();
  
    resetTimeLayer(tl_arc_time);
    if(max_flight_duration > 0) {
      time_t remaining_flight_duration = max_flight_duration - (mktime(&end_time) - mktime(&start_time));
      setTimeColor(tl_arc_time, (remaining_flight_duration <0)?GColorFolly:GColorTiffanyBlue);
      print_duration(tl_arc_time,-remaining_flight_duration,false);
    }  
  
    if(time_limit_overlapsed) {
      setTimeColor(tl_current_time,GColorWhite);
      setTimeColor(tl_flight_max_time,GColorFolly);
    }
      
    resetTimeLayer(tl_leg_start_time);
    resetTimeLayer(tl_leg_delta);
    resetTimeLayer(tl_leg_act_duration);
    resetTimeLayer(tl_leg_est_duration);
    resetTimeLayer(tl_leg_est_end_time);
  
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Ending flight...");
  
    if(limit_timer != NULL) {
      app_timer_cancel(limit_timer);
    }      
    vibes_short_pulse();
}

static void decrease_estimated_leg_duration() {
    estimated_leg_duration -= setting_values[PRM_LEG_TIME_STEP];
    if(estimated_leg_duration < 0)
      estimated_leg_duration = setting_values[PRM_INITIAL_LEG_DURATION];
    print_estimated_leg_duration();
}

static void increase_estimated_leg_duration() {
    if(estimated_leg_duration < 0)
      estimated_leg_duration = setting_values[PRM_INITIAL_LEG_DURATION];
    estimated_leg_duration += setting_values[PRM_LEG_TIME_STEP];
    print_estimated_leg_duration();
}

static void decrease_time_limit() {
    max_flight_duration -= setting_values[PRM_FLIGHT_TIME_STEP] * 60;
    print_flight_max_time();
}

static void increase_time_limit() {
    max_flight_duration += setting_values[PRM_FLIGHT_TIME_STEP] * 60;
    print_flight_max_time();
}

static void long_click_up_btn(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Up button pressed long");
  init_flight();  
  vibes_double_pulse();
}

static void long_click_dw_btn(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Down button pressed long");
  edit_settings();  
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
    refresh_arc();
  }
}

static void click_config_provider(void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Configuring clicks...");
  
  window_single_click_subscribe(BUTTON_ID_UP, single_click_up_btn);
  window_single_click_subscribe(BUTTON_ID_SELECT, single_click_sel_btn);
  window_single_click_subscribe(BUTTON_ID_DOWN, single_click_dw_btn);
  window_long_click_subscribe(BUTTON_ID_UP, LONG_CLICK_DELAY, long_click_up_btn , NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, LONG_CLICK_DELAY, long_click_dw_btn , NULL);
}

static void window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Loading window...");

  init_arc_layer(window,update_arc_display);

  tl_arc_time = addTimeLayer(PBL_IF_ROUND_ELSE(0,1),PBL_IF_ROUND_ELSE(1,2),0,GColorPurpureus);
  
  tl_flight_duration = addTimeLayer(0,2,20,GColorWhite);
  tl_leg_start_time = addTimeLayer(1,2,46,GColorWhite);
  tl_current_time = addTimeLayer(1,2,67,GColorWhite);
  tl_leg_est_end_time = addTimeLayer(1,2,88,GColorWhite);

  tl_leg_delta = addTimeLayer(2,1,50,GColorWhite);
  tl_leg_act_duration = addTimeLayer(2,3,65,GColorWhite);
  tl_leg_est_duration = addTimeLayer(2,1,92,GColorWhite);
  
  tl_flight_delta = addTimeLayer(0,2,PBL_IF_ROUND_ELSE(112,116),GColorWhite);
  tl_flight_start_time = addTimeLayer(PBL_IF_ROUND_ELSE(0,1),PBL_IF_ROUND_ELSE(1,2), PBL_IF_ROUND_ELSE(135,140),GColorGreen);
  tl_flight_max_time = addTimeLayer(PBL_IF_ROUND_ELSE(0,2),PBL_IF_ROUND_ELSE(1,2),PBL_IF_ROUND_ELSE(155,140),GColorOrange);
    
  init_flight();
  
  tick_timer_service_subscribe(SECOND_UNIT, my_tick_handler);
}

static void window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Unloading window...");
  
  destroyTimeLayer(tl_current_time);
  destroyTimeLayer(tl_flight_start_time);
  destroyTimeLayer(tl_leg_delta);
  destroyTimeLayer(tl_leg_start_time);
  destroyTimeLayer(tl_leg_est_duration);
  destroyTimeLayer(tl_leg_act_duration);
  destroyTimeLayer(tl_leg_est_end_time);
  destroyTimeLayer(tl_flight_duration);
  destroyTimeLayer(tl_flight_delta);
  destroyTimeLayer(tl_flight_max_time);
  destroyTimeLayer(tl_arc_time);
  
  release_arc_layer();
  
  tick_timer_service_unsubscribe();
}

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Init...");
  init_settings();
  read_settings();
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
  release_settings();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}