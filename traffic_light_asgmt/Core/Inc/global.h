/*
 * global.h
 *
 * Shared types and variables for the traffic light system.
 * Intentionally does NOT include main.h / HAL so fsm_*.c stays hardware-independent.
 */

#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#include <stdint.h>

typedef enum {
    MODE_NORMAL = 0,
    MODE_MANUAL,
    MODE_CONFIG
} Mode_t;

typedef enum {
    CONFIG_ITEM_RED = 0,
    CONFIG_ITEM_YELLOW,
    CONFIG_ITEM_GREEN
} ConfigItem_t;

typedef enum {
    RED = 0,
    YELLOW,
    GREEN,
    OFF
} Color_t;

typedef enum {
    CONFIG_OK = 0,
    CONFIG_ERR_RED_LE_YELLOW,
    CONFIG_ERR_RED_LE_GREEN,
    CONFIG_ERR_RED_NE_Y_PLUS_G
} ConfigError_t;

typedef enum {
    /* Automatic mode states */
    AUTO_INIT = 0,
    AUTO_GREEN1_RED2,
    AUTO_YELLOW1_RED2,
    AUTO_RED1_GREEN2,
    AUTO_RED1_YELLOW2,

    /* Manual mode states */
    MAN_INIT,
    MAN_GREEN1_RED2,
    MAN_YELLOW1_RED2,
    MAN_RED1_GREEN2,
    MAN_RED1_YELLOW2,
    MAN_SAFE_TRANSITION,

    /* Tuning / Config mode states */
    TUNE_INIT,
    TUNE_RED,
    TUNE_YELLOW,
    TUNE_GREEN,
    TUNE_ERROR
} SystemState_t;

typedef struct {
    uint8_t red;
    uint8_t yellow;
    uint8_t green;
} TimeTriple_t;

#define TIME_MIN_S  1U
#define TIME_MAX_S  99U

extern Mode_t        g_mode;
extern SystemState_t status;
extern ConfigItem_t  g_config_item;
extern ConfigError_t g_config_error;

extern Color_t       g_color_dir1;
extern Color_t       g_color_dir2;

extern TimeTriple_t  g_time_cfg;
extern TimeTriple_t  g_time_runtime1;
extern TimeTriple_t  g_time_runtime2;
extern TimeTriple_t  g_time_preview;

/* DHT20 sensor readings (fixed-point x10: 25.3 C -> 253, 60.5 %RH -> 605) */
extern int16_t       g_temperature_x10;
extern uint16_t      g_humidity_x10;
extern uint8_t       g_dht20_valid;

#endif /* INC_GLOBAL_H_ */
