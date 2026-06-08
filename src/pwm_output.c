/**
 * @file    pwm_output.c
 * @brief   4-ch PWM output — TIM4 CH1~CH4 on PB6~PB9
 *
 * Timer clock 8 MHz (HSI).  Default frequency 10 kHz → ARR = 799.
 * Duty stored as percentage, recalculated when frequency changes.
 */

#include "pwm_output.h"
#include "stm32f1xx_hal.h"

/* ── Timer clock ───────────────────────────────────────────────────── */
#define TIMER_CLOCK  8000000U    /* HSI, APB1 prescaler = 1 */

/* ── Global frequency variable ─────────────────────────────────────── */
uint32_t pwm_output_freq_hz = 10000;   /* 10 kHz default */

/* ── Internal state ────────────────────────────────────────────────── */
static TIM_HandleTypeDef htim4;
static uint8_t duty_pct[PWM_OUT_CHANNELS];    /* stored 0~100 */
static uint32_t current_arr;                  /* shadow of TIM4->ARR */
static uint32_t current_freq;                 /* applied frequency */

/* ── Public API ────────────────────────────────────────────────────── */

void PWM_Output_Init(void)
{
    duty_pct[0] = 0;
    duty_pct[1] = 0;
    duty_pct[2] = 0;
    duty_pct[3] = 0;

    /* ── GPIO: PB6~PB9 → TIM4_CH1~CH4 ── */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_AF_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    g.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &g);

    /* ── Compute ARR ── */
    uint32_t arr = TIMER_CLOCK / pwm_output_freq_hz - 1U;
    if (arr > 65535U) arr = 65535U;     /* clamp to 16-bit */
    if (arr < 3U)     arr = 3U;         /* minimum for reasonable PWM */

    current_arr  = arr;
    current_freq = pwm_output_freq_hz;

    /* ── Timer base init ── */
    TIM_Base_InitTypeDef tbase = {0};
    tbase.Prescaler         = 0;                       /* 8 MHz → 1 tick = 125 ns */
    tbase.CounterMode       = TIM_COUNTERMODE_UP;
    tbase.Period            = (uint32_t)arr;
    tbase.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    tbase.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    htim4.Instance = TIM4;
    htim4.Init     = tbase;
    HAL_TIM_Base_Init(&htim4);

    /* ── PWM channel init ── */
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.Pulse      = 0;  /* 0% duty at startup */

    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_2);
    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_3);
    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_4);

    /* ── Start all channels ── */
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

void PWM_Output_ApplyFreq(void)
{
    if (pwm_output_freq_hz == current_freq) return;
    if (pwm_output_freq_hz < 10) pwm_output_freq_hz = 10;

    uint32_t arr = TIMER_CLOCK / pwm_output_freq_hz - 1U;
    if (arr > 65535U) arr = 65535U;
    if (arr < 3U)     arr = 3U;

    current_arr  = arr;
    current_freq = pwm_output_freq_hz;

    /* Update ARR */
    __HAL_TIM_SET_AUTORELOAD(&htim4, arr);
    /* Generate update event to latch the new ARR immediately */
    TIM4->EGR = TIM_EGR_UG;

    /* Recalculate CCR for all 4 channels */
    for (uint8_t i = 0; i < PWM_OUT_CHANNELS; i++) {
        uint32_t ccr = (uint32_t)duty_pct[i] * (arr + 1U) / 100U;
        switch (i) {
            case 0: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr); break;
            case 1: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, ccr); break;
            case 2: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, ccr); break;
            case 3: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, ccr); break;
        }
    }
}

void PWM_Output_Set(uint8_t channel, uint8_t percent)
{
    if (channel >= PWM_OUT_CHANNELS) return;
    if (percent > 100) percent = 100;

    duty_pct[channel] = percent;

    uint32_t ccr = (uint32_t)percent * (current_arr + 1U) / 100U;

    switch (channel) {
        case 0: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr); break;
        case 1: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, ccr); break;
        case 2: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, ccr); break;
        case 3: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, ccr); break;
    }
}
