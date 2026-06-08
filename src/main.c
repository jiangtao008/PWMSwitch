/**
 * @file    main.c
 * @brief   Entry point — init, loop, LED heartbeat.
 *
 * Control logic lives in control.c — edit that file, not this one.
 */

#include "stm32f1xx_hal.h"
#include "control.h"
#include "pwm_input.h"
#include "display.h"

static void GPIO_Init(void);

/* ──────────────────────────────────────────────────────────────────── */
int main(void)
{
    SystemCoreClock = 8000000;
    HAL_Init();
    GPIO_Init();
    Display_Init();
    Control_Init();

    uint32_t last_tick = 0;

    while (1)
    {
        Control_Update();

        /* ── LED ─────────────────────────────────────────────── */
        uint32_t now = HAL_GetTick();

        if (PWM_Input_IsValid(1)) {   /* CH2 = throttle */
            uint32_t pulse = PWM_Input_Read(1);
            uint32_t half_period;

            if (pulse <= 1000U)
                half_period = 500;
            else if (pulse >= 2000U)
                half_period = 50;
            else
                half_period = 500U - (pulse - 1000U) * 450U / 1000U;

            if (now - last_tick >= half_period) {
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                last_tick = now;
            }
        } else {
            uint32_t cycle = now % 1000;
            if (cycle < 50)
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            else if (cycle < 100)
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            else if (cycle < 150)
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            else
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        }

        HAL_Delay(10);
    }
}

/* ── SysTick / GPIO ────────────────────────────────────────────────── */

void SysTick_Handler(void)
{
    HAL_IncTick();
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_13;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &g);
}
