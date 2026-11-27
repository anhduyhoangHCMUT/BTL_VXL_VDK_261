/*
 * fsm_automatic.c
 *
 * Automatic mode FSM (MODE_NORMAL).
 * No HAL calls allowed here: all hardware operations go through Step 5 modules.
 */

#include "fsm_automatic.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"
#include "lcd.h"

#define LCD_VIEW_TRAFFIC 0U
#define LCD_VIEW_DHT20   1U

static uint8_t s_rem_phase = 0U;
static uint8_t s_lcd_view  = LCD_VIEW_TRAFFIC;

static char color_to_char(Color_t c) {
    switch (c) {
    case RED:    return 'R';
    case YELLOW: return 'Y';
    case GREEN:  return 'G';
    default:     return '-';
    }
}

static uint8_t current_road_time(Color_t c, const TimeTriple_t *rt) {
    switch (c) {
    case RED:    return rt->red;
    case YELLOW: return rt->yellow;
    case GREEN:  return rt->green;
    default:     return 0U;
    }
}

static void lcd_print_temp_x10(int16_t t_x10) {
    if (t_x10 < 0) {
        lcd_send_data('-');
        t_x10 = (int16_t)(-t_x10);
    }
    uint8_t whole = (uint8_t)(t_x10 / 10);
    uint8_t frac  = (uint8_t)(t_x10 % 10);
    lcd_send_number_2d(whole);
    lcd_send_data('.');
    lcd_send_data((char)('0' + frac));
}

static void lcd_draw_auto_screen(void) {
    if (s_lcd_view == LCD_VIEW_DHT20) {
        lcd_goto_XY(0, 0);
        if (g_dht20_valid) {
            lcd_send_string("TEMP: ");
            lcd_print_temp_x10(g_temperature_x10);
            lcd_send_string(" C    ");

            lcd_goto_XY(1, 0);
            lcd_send_string("HUMI: ");
            lcd_send_number_2d((uint8_t)(g_humidity_x10 / 10U));
            lcd_send_data('.');
            lcd_send_data((char)('0' + (g_humidity_x10 % 10U)));
            lcd_send_string(" %RH  ");
        } else {
            lcd_send_string("DHT20 SENSOR    ");
            lcd_goto_XY(1, 0);
            lcd_send_string("READING...      ");
        }
        return;
    }

    /* Row 0: AUTO + compact DHT20 */
    lcd_goto_XY(0, 0);
    lcd_send_string("AUTO ");
    if (g_dht20_valid) {
        lcd_print_temp_x10(g_temperature_x10);
        lcd_send_string("C ");
        lcd_send_number_2d((uint8_t)(g_humidity_x10 / 10U));
        lcd_send_string("%  ");
    } else {
        lcd_send_string("--.-C --%  ");
    }

    /* Row 1: Road 1 & Road 2 countdown */
    uint8_t t1 = current_road_time(g_color_dir1, &g_time_runtime1);
    uint8_t t2 = current_road_time(g_color_dir2, &g_time_runtime2);

    lcd_goto_XY(1, 0);
    lcd_send_string("L1:");
    lcd_send_data(color_to_char(g_color_dir1));
    lcd_send_number_2d(t1);
    lcd_send_string("   L2:");
    lcd_send_data(color_to_char(g_color_dir2));
    lcd_send_number_2d(t2);
    lcd_send_string(" ");
}

static void update_runtime_counters(void) {
    g_time_runtime1.red = g_time_runtime1.yellow = g_time_runtime1.green = 0U;
    g_time_runtime2.red = g_time_runtime2.yellow = g_time_runtime2.green = 0U;

    switch (status) {
    case AUTO_GREEN1_RED2:
        g_time_runtime1.green = s_rem_phase;
        g_time_runtime2.red   = (uint8_t)(s_rem_phase + g_time_cfg.yellow);
        break;
    case AUTO_YELLOW1_RED2:
        g_time_runtime1.yellow = s_rem_phase;
        g_time_runtime2.red    = s_rem_phase;
        break;
    case AUTO_RED1_GREEN2:
        g_time_runtime1.red   = (uint8_t)(s_rem_phase + g_time_cfg.yellow);
        g_time_runtime2.green = s_rem_phase;
        break;
    case AUTO_RED1_YELLOW2:
        g_time_runtime1.red    = s_rem_phase;
        g_time_runtime2.yellow = s_rem_phase;
        break;
    default:
        break;
    }
}

static void enter_auto_phase(SystemState_t next_state, Color_t c1, Color_t c2, uint8_t duration_s) {
    status       = next_state;
    g_color_dir1 = c1;
    g_color_dir2 = c2;
    s_rem_phase  = duration_s;

    traffic_set(1U, c1);
    traffic_set(2U, c2);

    setTimer(TIMER_AUTO_PHASE, (uint32_t)duration_s * 1000U);
    setTimer(TIMER_AUTO_1S, 1000U);

    update_runtime_counters();
    lcd_draw_auto_screen();
}

static void handle_auto_common_events(void) {
    if (isButtonPressed(BTN_MODE)) {
        clearTimer(TIMER_AUTO_PHASE);
        clearTimer(TIMER_AUTO_1S);
        button_clear_all();
        g_mode = MODE_MANUAL;
        status = MAN_INIT;
        return;
    }

    if (isButtonPressed(BTN_SET)) {
        s_lcd_view = (s_lcd_view == LCD_VIEW_TRAFFIC) ? LCD_VIEW_DHT20 : LCD_VIEW_TRAFFIC;
        lcd_draw_auto_screen();
    }

    if (timer_flag[TIMER_AUTO_1S]) {
        setTimer(TIMER_AUTO_1S, 1000U);
        if (s_rem_phase > 1U) {
            s_rem_phase--;
        }
        update_runtime_counters();
        lcd_draw_auto_screen();
    }
}

void fsm_automatic_run(void) {
    if (g_mode != MODE_NORMAL) {
        return;
    }

    switch (status) {
    case AUTO_INIT:
        g_config_error = CONFIG_OK;
        s_lcd_view     = LCD_VIEW_TRAFFIC;
        button_clear_all();
        enter_auto_phase(AUTO_GREEN1_RED2, GREEN, RED, g_time_cfg.green);
        break;

    case AUTO_GREEN1_RED2:
        if (timer_flag[TIMER_AUTO_PHASE]) {
            enter_auto_phase(AUTO_YELLOW1_RED2, YELLOW, RED, g_time_cfg.yellow);
            break;
        }
        handle_auto_common_events();
        break;

    case AUTO_YELLOW1_RED2:
        if (timer_flag[TIMER_AUTO_PHASE]) {
            enter_auto_phase(AUTO_RED1_GREEN2, RED, GREEN, g_time_cfg.green);
            break;
        }
        handle_auto_common_events();
        break;

    case AUTO_RED1_GREEN2:
        if (timer_flag[TIMER_AUTO_PHASE]) {
            enter_auto_phase(AUTO_RED1_YELLOW2, RED, YELLOW, g_time_cfg.yellow);
            break;
        }
        handle_auto_common_events();
        break;

    case AUTO_RED1_YELLOW2:
        if (timer_flag[TIMER_AUTO_PHASE]) {
            enter_auto_phase(AUTO_GREEN1_RED2, GREEN, RED, g_time_cfg.green);
            break;
        }
        handle_auto_common_events();
        break;

    default:
        break;
    }
}
