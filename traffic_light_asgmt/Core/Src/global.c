/*
 * global.c
 *
 * Definitions of shared variables declared in global.h.
 */

#include "global.h"

Mode_t        g_mode         = MODE_NORMAL;
SystemState_t status         = AUTO_INIT;
ConfigItem_t  g_config_item  = CONFIG_ITEM_RED;
ConfigError_t g_config_error = CONFIG_OK;

Color_t       g_color_dir1 = GREEN;
Color_t       g_color_dir2 = RED;

TimeTriple_t  g_time_cfg = {
    .red    = 8,
    .yellow = 3,
    .green  = 5
};

TimeTriple_t  g_time_runtime1 = {
    .red    = 0,
    .yellow = 0,
    .green  = 5
};

TimeTriple_t  g_time_runtime2 = {
    .red    = 8,
    .yellow = 0,
    .green  = 0
};

TimeTriple_t  g_time_preview = {
    .red    = 8,
    .yellow = 3,
    .green  = 5
};

int16_t       g_temperature_x10 = 0;
uint16_t      g_humidity_x10    = 0;
uint8_t       g_dht20_valid     = 0;
