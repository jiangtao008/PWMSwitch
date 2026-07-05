/**
 * @file    speed_sensor.c
 * @brief   2-motor speed measurement — TIM1 input capture on PA8~PA11
 *
 * TIM1 clock: 8 MHz (HSI, APB2 prescaler = 1).
 * Prescaler 71 → 111.1 kHz → 9 µs per tick.
 * Period 0xFFFF → max measurable ~589 ms (~1.7 Hz min).
 */

#include "speed_sensor.h"
#include "stm32f1xx_hal.h"

/* ── Timer config ────────────────────────────────────────────────────── */
#define TIM1_CLOCK      8000000U
#define TIM1_PRESCALER  71U             /* 8MHz / 72 = 111.1 kHz */
#define TIM1_PERIOD     0xFFFFU         /* 16-bit max */
#define TICK_US         9U              /* 1 tick = 9 µs */

/* ── Per-channel captured data ───────────────────────────────────────── */
static volatile uint32_t last_cap[4];   /* previous capture value */
static volatile uint32_t pulse_ticks[4];/* latest pulse period in ticks */
static volatile uint8_t  cap_valid[4];  /* 1 = at least one period measured */

/* ── Timer handle ────────────────────────────────────────────────────── */
static TIM_HandleTypeDef htim1;

/* ── Forward ─────────────────────────────────────────────────────────── */
static void process_channel(uint8_t ch, uint32_t ccr);

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════ */

void SpeedSensor_Init(void)
{
    /* ── GPIO: PA8~PA11 → TIM1_CH1~CH4 ── */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;          /* pull high when sensor open-drain */
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin   = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOA, &g);

    /* ── Timer base ── */
    TIM_Base_InitTypeDef tbase = {0};
    tbase.Prescaler         = TIM1_PRESCALER;
    tbase.CounterMode       = TIM_COUNTERMODE_UP;
    tbase.Period            = TIM1_PERIOD;
    tbase.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    tbase.RepetitionCounter = 0;
    tbase.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    htim1.Instance = TIM1;
    htim1.Init     = tbase;
    HAL_TIM_Base_Init(&htim1);

    /* ── Input capture: rising edge, no prescaler, light filter ── */
    TIM_IC_InitTypeDef ic = {0};
    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = 0x0F;          /* f_DTS/32, N=8 → ~23 µs glitch rejection */

    HAL_TIM_IC_ConfigChannel(&htim1, &ic, TIM_CHANNEL_1);
    HAL_TIM_IC_ConfigChannel(&htim1, &ic, TIM_CHANNEL_2);
    HAL_TIM_IC_ConfigChannel(&htim1, &ic, TIM_CHANNEL_3);
    HAL_TIM_IC_ConfigChannel(&htim1, &ic, TIM_CHANNEL_4);

    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_4);

    /* ── NVIC ── */
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);

    /* ── Init state ── */
    for (uint8_t i = 0; i < 4; i++) {
        last_cap[i]   = 0;
        pulse_ticks[i] = 0;
        cap_valid[i]   = 0;
    }
}

uint32_t SpeedSensor_GetPeriod(uint8_t motor)
{
    if (motor > 1) return 0;

    uint8_t a = motor * 2;       /* CH1 or CH3 */
    uint8_t b = motor * 2 + 1;   /* CH2 or CH4 */

    uint32_t pa = pulse_ticks[a];
    uint32_t pb = pulse_ticks[b];

    if (cap_valid[a] && cap_valid[b])
        return (pa + pb) / 2U;   /* average both lines */
    else if (cap_valid[a])
        return pa;
    else if (cap_valid[b])
        return pb;
    else
        return 0;
}

uint16_t SpeedSensor_GetRPM(uint8_t motor, uint8_t ppr)
{
    uint32_t ticks = SpeedSensor_GetPeriod(motor);
    if (ticks == 0 || ppr == 0) return 0;

    /* RPM = 60,000,000 / (ticks * TICK_US * ppr) */
    uint32_t period_us = ticks * TICK_US;
    if (period_us == 0) return 0;

    return (uint16_t)(60000000UL / (period_us * ppr));
}

uint8_t SpeedSensor_IsValid(uint8_t motor)
{
    if (motor > 1) return 0;
    return cap_valid[motor * 2] || cap_valid[motor * 2 + 1];
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ISR
 * ═══════════════════════════════════════════════════════════════════════ */

void TIM1_CC_IRQHandler(void)
{
    uint32_t sr = TIM1->SR;

    if (sr & TIM_SR_CC1IF) {
        TIM1->SR = ~TIM_SR_CC1IF;
        process_channel(0, TIM1->CCR1);
    }
    if (sr & TIM_SR_CC2IF) {
        TIM1->SR = ~TIM_SR_CC2IF;
        process_channel(1, TIM1->CCR2);
    }
    if (sr & TIM_SR_CC3IF) {
        TIM1->SR = ~TIM_SR_CC3IF;
        process_channel(2, TIM1->CCR3);
    }
    if (sr & TIM_SR_CC4IF) {
        TIM1->SR = ~TIM_SR_CC4IF;
        process_channel(3, TIM1->CCR4);
    }
}

/* ── Process one capture ─────────────────────────────────────────────── */

static void process_channel(uint8_t ch, uint32_t ccr)
{
    uint32_t prev = last_cap[ch];

    if (prev == 0) {
        /* First edge — store timestamp only */
        last_cap[ch] = ccr;
        return;
    }

    /* Calculate period, handle counter wrap */
    uint32_t period;
    if (ccr >= prev)
        period = ccr - prev;
    else
        period = (TIM1_PERIOD - prev) + ccr + 1U;

    /* Sanity: reject periods < 10 ticks (90 µs → > 11 kHz, noise) */
    if (period >= 10U) {
        pulse_ticks[ch] = period;
        cap_valid[ch]   = 1;
    }

    last_cap[ch] = ccr;
}
