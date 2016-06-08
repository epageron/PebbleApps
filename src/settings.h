#pragma once
#include <pebble.h>
#include "pin_window.h"
#include "helpers.h"

#define FLIGHT_TIME_STEP     10       // In minutes
#define LEG_TIME_STEP        30       // In seconds
#define LEG_CONFIG_DURATION  5        // In seconds
#define INITIAL_LEG_DURATION 8 * 60   // In seconds

#define PRM_FLIGHT_TIME_STEP     0
#define PRM_LEG_TIME_STEP        1
#define PRM_LEG_CONFIG_DURATION  2
#define PRM_INITIAL_LEG_DURATION 3
#define PRM_NUMBER_OF_VALUES     4
#define PRM_DATA_SIZE            PRM_NUMBER_OF_VALUES * sizeof(uint32_t)

#define SETTINGS_KEY      0                   // Change the key if the format of the parameters is nomore compatible
#define IS_SETTINGS_IN_SECONDS(x) settings[x].short_unit_name[0]=='s'

typedef struct setting_def {
  const char     *name;
  const char     *unit_name;
  const char     *short_unit_name;
  const time_t   default_value;
} setting_def;

typedef struct setting_ctx {
  int  selected_idx;
} setting_ctx;

extern time_t setting_values[PRM_NUMBER_OF_VALUES];
extern PinWindow *settings_pw;
extern myMenuLayer *settings_ml;

extern void init_settings();
extern void read_settings();
extern void save_settings();
extern void edit_settings();
extern void release_settings();