#include <pebble.h>
#include "settings.h"

PinWindow *settings_pw;
myMenuLayer *settings_ml;

time_t setting_values[PRM_NUMBER_OF_VALUES];
const setting_def settings[PRM_NUMBER_OF_VALUES] = 
{
  {"Flight time step", "hh:mm", "mn", FLIGHT_TIME_STEP },
  {"Leg time step","hh:mm:ss","s",LEG_TIME_STEP },
  {"Leg valid delay","hh:mm:ss","s", LEG_CONFIG_DURATION},
  {"Init leg time","hh:mm:ss","s", INITIAL_LEG_DURATION} 
};

void read_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Loading prefs...");
  if(persist_exists(SETTINGS_KEY)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Prefs found !");
    persist_read_data(SETTINGS_KEY, &setting_values, PRM_DATA_SIZE);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"No prefs...initializing defaults");
    int i = 0;
    while(i < PRM_NUMBER_OF_VALUES)
    {
      setting_values[i] = settings[i].default_value;
      i++;  
    }
  }
}

void format_setting(int setting_idx, char *buf, int size_of_buf) {
  snprintf(buf,size_of_buf,"%d %s",(int)setting_values[setting_idx],settings[setting_idx].short_unit_name);
}

void save_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Saving prefs...");
  persist_write_data(SETTINGS_KEY, &setting_values, PRM_DATA_SIZE);
}

PIN *seconds_to_pin(time_t v)
{
  PIN *pinv = (PIN *)malloc(sizeof(PIN));
  pinv->digits[0] = v / 3600;
  pinv->digits[1] = (v % 3600) / 60;
  pinv->digits[2] = v % 60;
  return pinv;
}

time_t pin_to_seconds(PIN *p)
{
  if(p)
    return (p->digits[0] * 3600 + p->digits[1] * 60 + p->digits[2]);
  return 0;
}

PIN *minutes_to_pin(time_t v)
{
  PIN *pinv = (PIN *)malloc(sizeof(PIN));
  pinv->digits[0] = v / 60;
  pinv->digits[1] = v % 60;
  return pinv;
}

time_t pin_to_minutes(PIN *p)
{
  if(p)
    return (p->digits[0] * 60 + p->digits[1]);
  return 0;
}

void pin_window_cb(PIN p, void *ctx) {
  PinWindow *pw = (PinWindow *)ctx;
  setting_ctx *sctx = (setting_ctx *)pw->context;
  setting_values[sctx->selected_idx] = IS_SETTINGS_IN_SECONDS(sctx->selected_idx)?pin_to_seconds(&p):pin_to_minutes(&p);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Pref set to %d", (int)setting_values[sctx->selected_idx]);

  char buf[30];
  format_setting(sctx->selected_idx,buf,30);
  refreshMenuItem(settings_ml,sctx->selected_idx,settings[sctx->selected_idx].name,buf);
  
  free(settings_pw->context);
  pin_window_pop(pw,true);
  settings_pw = NULL;
}

void setting_menu_exit_cb() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Exiting setup...");
  save_settings();
  window_stack_remove(settings_ml->window,true);
}

void setting_select_cb(int index, void *context) {
 
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Pref %s selected",settings[index].name);
  // Create the pin window to set the parameter value
  const PinWindowData data = { .main_text = settings[index].name,
                               .sub_text = settings[index].unit_name,
                               .nb_digits = IS_SETTINGS_IN_SECONDS(index)?3:2,
                               .init_pin = IS_SETTINGS_IN_SECONDS(index)?seconds_to_pin(setting_values[index]):minutes_to_pin(setting_values[index]),
                             };
  if(settings_pw) {
    pin_window_destroy(settings_pw);
  }
    
  settings_pw = pin_window_create((PinWindowCallbacks) { 
    .pin_complete = pin_window_cb,
  }, data);
  free(data.init_pin);
  setting_ctx *ctx = (setting_ctx *)malloc(sizeof(setting_ctx));
  ctx->selected_idx = index;
  settings_pw->context = ctx;
  pin_window_push(settings_pw, true);  
}

void edit_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Entering setup...");
  
  if(settings_ml==NULL) {
    settings_ml = initMenu("Settings");
    char buf[30];
    uint32_t i = 0;
    while(i < PRM_NUMBER_OF_VALUES) {
      format_setting(i,buf,30);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Adding item %s",settings[i].name);
      addMenuItem(settings_ml,settings[i].name,buf,setting_select_cb);
      i++;
    }
    addMenuItem(settings_ml,"Save & Exit","",setting_menu_exit_cb);
  }
  displayMenu(settings_ml);
}

void init_settings() {
  settings_ml = NULL;
  settings_pw = NULL;
}

void release_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Releasing settings ressources...");
  if(settings_ml) {
    freeMenu(settings_ml);
    free(settings_ml);
    settings_ml = NULL;
  }
} 