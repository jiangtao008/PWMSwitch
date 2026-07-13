/**
 * @file    digital_output.c
 * @brief   4-ch boolean output — PA15, PB3, PB4, PB5
 *
 * PB12~PB15 给 speed_sensor 做正交编码测速。
 */

#include "digital_output.h"
#include "stm32f1xx_hal.h"

/* ── Pin lookup ────────────────────────────────────────────────────── */
static GPIO_TypeDef *const port_map[DIGITAL_OUT_CHANNELS] = {
    GPIOA,   /* CH1 → PA15 */
    GPIOB,   /* CH2 → PB3  */
    GPIOB,   /* CH3 → PB4  */
    GPIOB,   /* CH4 → PB5  */
};

static const uint16_t pin_map[DIGITAL_OUT_CHANNELS] = {
    GPIO_PIN_15,   /* CH1 → PA15 */
    GPIO_PIN_3,    /* CH2 → PB3  */
    GPIO_PIN_4,    /* CH3 → PB4  */
    GPIO_PIN_5,    /* CH4 → PB5  */
};

void Digital_Output_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    /* Init PA15, PB3, PB4, PB5 */
    g.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOB, &g);

    g.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, &g);

    /* All LOW initially */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5,
                      GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
}

void Digital_Output_Set(uint8_t channel, uint8_t value)
{
    if (channel >= DIGITAL_OUT_CHANNELS) return;

    HAL_GPIO_WritePin(port_map[channel], pin_map[channel],
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
