/*
 * fsm_tuning.c
 *
 * Tuning / Configuration mode FSM (MODE_CONFIG).
 * No HAL calls allowed here: all hardware operations go through Step 5 modules.
 */

#include "fsm_tuning.h"
#include "global.h"
#include "button.h"
#include "traffic_light.h"
#include "lcd.h"

static uint8_t s_blink_on = 0U;

static void validate_config_values(void) {
    g_config_error = CONFIG_OK;

    if (g_time_preview.red <= g_time_preview.yellow) {
        g_config_error = CONFIG_ERR_RED_LE_YELLOW;
    } else if (g_time_preview.red <= g_time_preview.green) {
        g_config_error = CONFIG_ERR_RED_LE_GREEN;
    } else if (g_time_preview.red != (uint8_t)(g_time_preview.yellow + g_time_preview.green)) {
        g_config_error = CONFIG_ERR_RED_NE_Y_PLUS_G;
    }
}

static void lcd_draw_tuning_screen(void) {
    if (status == TUNE_ERROR) {
        lcd_goto_XY(0, 0);
        lcd_send_string("CONFIG ERR      ");
        lcd_goto_XY(1, 0);
        switch (g_config_error) {
        case CONFIG_ERR_RED_LE_YELLOW:
            lcd_send_string("RED <= YELLOW   ");
            break;
        case CONFIG_ERR_RED_LE_GREEN:
            lcd_send_string("RED <= GREEN    ");
            break;
        case CONFIG_ERR_RED_NE_Y_PLUS_G:
            lcd_send_string("R != Y + G      ");
            break;
        default:
            lcd_send_string("INVALID TIME    ");
            break;
        }
        return;
    }

    uint8_t val = 0U;
    lcd_goto_XY(0, 0);
    switch (g_config_item) {
    case CONFIG_ITEM_RED:
        lcd_send_string("CONFIG RED      ");
        val = g_time_preview.red;
        break;
    case CONFIG_ITEM_YELLOW:
        lcd_send_string("CONFIG YELLOW   ");
        val = g_time_preview.yellow;
        break;
    case CONFIG_ITEM_GREEN:
        lcd_send_string("CONFIG GREEN    ");
        val = g_time_preview.green;
        break;
    default:
        lcd_send_string("CONFIG ???      ");
        break;
    }

    lcd_goto_XY(1, 0);
    lcd_send_string("TIME = ");
    lcd_send_number_2d(val);
    lcd_send_string("s      ");
}

static uint8_t adjust_value(uint8_t *v) {
    uint8_t changed = 0U;

    if (isButtonPressed(BTN_INC) || isButtonHold(BTN_INC)) {
        *v = (*v >= TIME_MAX_S) ? TIME_MIN_S : (uint8_t)(*v + 1U);
        changed = 1U;
    } else if (isButtonPressed(BTN_DEC) || isButtonHold(BTN_DEC)) {
        *v = (*v <= TIME_MIN_S) ? TIME_MAX_S : (uint8_t)(*v - 1U);
        changed = 1U;
    }

    return changed;
}

static void exit_to_auto(void) {
    s_blink_on     = 0U;
    g_config_error = CONFIG_OK;
    button_clear_all();
    g_mode = MODE_NORMAL;
    status = AUTO_INIT;
}

void fsm_tuning_run(void) {
    if (g_mode != MODE_CONFIG) {
        return;
    }

    switch (status) {
    case TUNE_INIT:
        g_time_preview = g_time_cfg;
        g_config_item  = CONFIG_ITEM_RED;
        g_config_error = CONFIG_OK;
        s_blink_on     = 0U;
        traffic_set(1U, OFF);
        traffic_set(2U, OFF);
        status = TUNE_RED;
        lcd_draw_tuning_screen();
        break;

    case TUNE_RED:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_auto();
            break;
        }
        if (adjust_value(&g_time_preview.red)) {
            lcd_draw_tuning_screen();
        }
        if (isButtonPressed(BTN_SET)) {
            g_config_item = CONFIG_ITEM_YELLOW;
            status        = TUNE_YELLOW;
            lcd_draw_tuning_screen();
        }
        break;

    case TUNE_YELLOW:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_auto();
            break;
        }
        if (adjust_value(&g_time_preview.yellow)) {
            lcd_draw_tuning_screen();
        }
        if (isButtonPressed(BTN_SET)) {
            g_config_item = CONFIG_ITEM_GREEN;
            status        = TUNE_GREEN;
            lcd_draw_tuning_screen();
        }
        break;

    case TUNE_GREEN:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_auto();
            break;
        }
        if (adjust_value(&g_time_preview.green)) {
            lcd_draw_tuning_screen();
        }
        if (isButtonPressed(BTN_SET)) {
            validate_config_values();
            if (g_config_error == CONFIG_OK) {
                g_time_cfg = g_time_preview;
                exit_to_auto();
            } else {
                status = TUNE_ERROR;
                lcd_draw_tuning_screen();
            }
        }
        break;

    case TUNE_ERROR:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_auto();
        } else if (isButtonPressed(BTN_SET)) {
            g_time_preview = g_time_cfg;
            g_config_item  = CONFIG_ITEM_RED;
            g_config_error = CONFIG_OK;
            status         = TUNE_RED;
            lcd_draw_tuning_screen();
        }
        break;

    default:
        break;
    }
}

void fsm_tuning_blink(void) {
    if (g_mode != MODE_CONFIG) {
        return;
    }

    s_blink_on ^= 1U;
    if (!s_blink_on) {
        traffic_set(1U, OFF);
        traffic_set(2U, OFF);
        return;
    }

    Color_t c = RED;
    if (status == TUNE_YELLOW) {
        c = YELLOW;
    } else if (status == TUNE_GREEN) {
        c = GREEN;
    }

    traffic_set(1U, c);
    traffic_set(2U, c);
}
