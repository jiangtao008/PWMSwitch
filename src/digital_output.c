/**
 * @file    digital_output.c
 * @brief   4-ch boolean output on PB12~PB15
 */

#include "digital_output.h"
#include "stm32f1xx_hal.h"

/* ── Pin lookup ────────────────────────────────────────────────────── */
static const uint16_t pin_map[DIGITAL_OUT_CHANNELS] = {
    GPIO_PIN_12,   /* CH1 → PB12 */
    GPIO_PIN_13,   /* CH2 → PB13 */
    GPIO_PIN_14,   /* CH3 → PB14 */
    GPIO_PIN_15,   /* CH4 → PB15 */
};

void Digital_Output_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin   = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;

    HAL_GPIO_Init(GPIOB, &g);

    /* All LOW initially */
    HAL_GPIO_WritePin(GPIOB, g.Pin, GPIO_PIN_RESET);
}

void Digital_Output_Set(uint8_t channel, uint8_t value)
{
    if (channel >= DIGITAL_OUT_CHANNELS) return;

    HAL_GPIO_WritePin(GPIOB, pin_map[channel],
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
