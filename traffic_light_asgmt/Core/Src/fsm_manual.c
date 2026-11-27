/*
 * fsm_manual.c
 *
 * Manual mode FSM (MODE_MANUAL).
 * No HAL calls allowed here: all hardware operations go through Step 5 modules.
 */

#include "fsm_manual.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"
#include "lcd.h"

#define SAFE_YELLOW_DURATION_MS 2000U

static const char *color_to_str3(Color_t c) {
    switch (c) {
    case RED:    return "RED";
    case YELLOW: return "YEL";
    case GREEN:  return "GRN";
    default:     return "OFF";
    }
}

static void lcd_draw_manual_screen(const char *tag) {
    lcd_goto_XY(0, 0);
    lcd_send_string(tag);

    lcd_goto_XY(1, 0);
    lcd_send_string("L1:");
    lcd_send_string(color_to_str3(g_color_dir1));
    lcd_send_string("   L2:");
    lcd_send_string(color_to_str3(g_color_dir2));
    lcd_send_string(" ");
}

static void enter_manual_state(SystemState_t next_state, Color_t c1, Color_t c2) {
    status       = next_state;
    g_color_dir1 = c1;
    g_color_dir2 = c2;

    traffic_set(1U, c1);
    traffic_set(2U, c2);

    lcd_draw_manual_screen("MODE: MANUAL    ");
}

static void exit_to_tuning(void) {
    clearTimer(TIMER_MANUAL_SAFE);
    button_clear_all();
    g_mode = MODE_CONFIG;
    status = TUNE_INIT;
}

void fsm_manual_run(void) {
    if (g_mode != MODE_MANUAL) {
        return;
    }

    switch (status) {
    case MAN_INIT:
        /* Inherit the exact active phase from Automatic mode (including Yellow) */
        if (g_color_dir1 == GREEN && g_color_dir2 == RED) {
            enter_manual_state(MAN_GREEN1_RED2, GREEN, RED);
        } else if (g_color_dir1 == YELLOW && g_color_dir2 == RED) {
            enter_manual_state(MAN_YELLOW1_RED2, YELLOW, RED);
        } else if (g_color_dir1 == RED && g_color_dir2 == GREEN) {
            enter_manual_state(MAN_RED1_GREEN2, RED, GREEN);
        } else if (g_color_dir1 == RED && g_color_dir2 == YELLOW) {
            enter_manual_state(MAN_RED1_YELLOW2, RED, YELLOW);
        } else {
            enter_manual_state(MAN_GREEN1_RED2, GREEN, RED);
        }
        break;

    case MAN_GREEN1_RED2:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_tuning();
        } else if (isButtonPressed(BTN_INC)) {
            enter_manual_state(MAN_YELLOW1_RED2, YELLOW, RED);
        } else if (isButtonPressed(BTN_DEC)) {
            enter_manual_state(MAN_RED1_YELLOW2, RED, YELLOW);
        }
        break;

    case MAN_YELLOW1_RED2:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_tuning();
        } else if (isButtonPressed(BTN_INC)) {
            enter_manual_state(MAN_RED1_GREEN2, RED, GREEN);
        } else if (isButtonPressed(BTN_DEC) || isButtonPressed(BTN_SET)) {
            enter_manual_state(MAN_GREEN1_RED2, GREEN, RED);
        }
        break;

    case MAN_RED1_GREEN2:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_tuning();
        } else if (isButtonPressed(BTN_INC)) {
            enter_manual_state(MAN_RED1_YELLOW2, RED, YELLOW);
        } else if (isButtonPressed(BTN_DEC)) {
            enter_manual_state(MAN_YELLOW1_RED2, YELLOW, RED);
        } else if (isButtonPressed(BTN_SET)) {
            /* Safe transition: go through Yellow on Road 2 before Road 1 Green */
            status       = MAN_SAFE_TRANSITION;
            g_color_dir1 = RED;
            g_color_dir2 = YELLOW;
            traffic_set(1U, RED);
            traffic_set(2U, YELLOW);
            setTimer(TIMER_MANUAL_SAFE, SAFE_YELLOW_DURATION_MS);
            lcd_draw_manual_screen("MANUAL SAFE...  ");
        }
        break;

    case MAN_RED1_YELLOW2:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_tuning();
        } else if (isButtonPressed(BTN_INC) || isButtonPressed(BTN_SET)) {
            enter_manual_state(MAN_GREEN1_RED2, GREEN, RED);
        } else if (isButtonPressed(BTN_DEC)) {
            enter_manual_state(MAN_RED1_GREEN2, RED, GREEN);
        }
        break;

    case MAN_SAFE_TRANSITION:
        if (isButtonPressed(BTN_MODE)) {
            exit_to_tuning();
        } else if (timer_flag[TIMER_MANUAL_SAFE]) {
            enter_manual_state(MAN_GREEN1_RED2, GREEN, RED);
        }
        break;

    default:
        break;
    }
}
