/*
 * traffic_light.c
 *
 * Signal truth table (verify on hardware in Step 3.2):
 *   (A=0, B=0) -> OFF
 *   (A=0, B=1) -> GREEN
 *   (A=1, B=0) -> YELLOW
 *   (A=1, B=1) -> RED
 */

#include "traffic_light.h"
#include "main.h"

/* Fallback pin aliases if .ioc still uses legacy labels (A1/B1/A2/B2) */
#ifndef TL1_A_Pin
#define TL1_A_GPIO_Port  A1_GPIO_Port
#define TL1_A_Pin        A1_Pin
#endif
#ifndef TL1_B_Pin
#define TL1_B_GPIO_Port  B1_GPIO_Port
#define TL1_B_Pin        B1_Pin
#endif
#ifndef TL2_A_Pin
#define TL2_A_GPIO_Port  A2_GPIO_Port
#define TL2_A_Pin        A2_Pin
#endif
#ifndef TL2_B_Pin
#define TL2_B_GPIO_Port  B2_GPIO_Port
#define TL2_B_Pin        B2_Pin
#endif

static void write_signals(GPIO_TypeDef *port_a, uint16_t pin_a,
                          GPIO_TypeDef *port_b, uint16_t pin_b,
                          Color_t color) {
    GPIO_PinState a = GPIO_PIN_RESET;
    GPIO_PinState b = GPIO_PIN_RESET;

    switch (color) {
    case RED:
        a = GPIO_PIN_SET;
        b = GPIO_PIN_SET;
        break;
    case YELLOW:
        a = GPIO_PIN_SET;
        b = GPIO_PIN_RESET;
        break;
    case GREEN:
        a = GPIO_PIN_RESET;
        b = GPIO_PIN_SET;
        break;
    case OFF:
    default:
        a = GPIO_PIN_RESET;
        b = GPIO_PIN_RESET;
        break;
    }

    HAL_GPIO_WritePin(port_a, pin_a, a);
    HAL_GPIO_WritePin(port_b, pin_b, b);
}

void traffic_set(uint8_t road, Color_t color) {
    if (road == 1U) {
        write_signals(TL1_A_GPIO_Port, TL1_A_Pin, TL1_B_GPIO_Port, TL1_B_Pin, color);
    } else if (road == 2U) {
        write_signals(TL2_A_GPIO_Port, TL2_A_Pin, TL2_B_GPIO_Port, TL2_B_Pin, color);
    }
}
