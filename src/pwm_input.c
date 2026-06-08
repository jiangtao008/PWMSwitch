/**
 * @file    pwm_input.c
 * @brief   8-channel RC receiver PWM input capture (TIM2 + TIM3)
 *
 * @note    Timer clock 8MHz → prescaler 8 → 1MHz counter → 1µs resolution.
 *          Each channel captures both edges, polarity swapped in ISR.
 */

#include "pwm_input.h"
#include "stm32f1xx_hal.h"

/* ── Timer configuration ───────────────────────────────────────────── */
#define TIMER_CLOCK     8000000U    /* HSI, APB1 prescaler = 1 */
#define TIM_PRESCALER   7U          /* 8MHz / (7+1) = 1MHz → 1µs/count */
#define TIM_PERIOD      0xFFFFU     /* 16-bit max */

/* ── Signal validation ─────────────────────────────────────────────── */
#define PULSE_MIN_US    500U        /* reject glitches < 0.5ms */
#define PULSE_MAX_US    2500U       /* reject glitches > 2.5ms */

/* ── Timer handles ─────────────────────────────────────────────────── */
static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim3;

/* ── Per-channel captured data ─────────────────────────────────────── */
static volatile uint32_t pwm_pulse[PWM_CHANNEL_COUNT];   /* latest pulse width (µs) */
static volatile uint32_t pwm_rise[PWM_CHANNEL_COUNT];    /* captured rise time */
static volatile uint8_t  pwm_valid[PWM_CHANNEL_COUNT];   /* signal valid flag */
static volatile uint8_t  wait_rising[PWM_CHANNEL_COUNT]; /* 1=expect next edge to be rising */

/* ── Forward declarations ──────────────────────────────────────────── */
static void capture_isr(TIM_TypeDef *tim, uint8_t base_ch);

/* ── Public API ────────────────────────────────────────────────────── */

/**
 * @brief  Initialise TIM2 & TIM3 input-capture for all 8 channels.
 */
void PWM_Input_Init(void)
{
    /* ── Enable peripheral clocks ── */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* ── GPIO: input floating for timer capture pins ──
     *  TIM2: PA0~PA3    TIM3: PA6,PA7,PB0,PB1                     */
    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLDOWN;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    /* PA0~PA3 */
    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &g);
    /* PA6,PA7 */
    g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &g);
    /* PB0,PB1 */
    g.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &g);

    /* ── Timer base init (common for TIM2 & TIM3) ── */
    TIM_Base_InitTypeDef tbase = {0};
    tbase.Prescaler         = TIM_PRESCALER;
    tbase.CounterMode       = TIM_COUNTERMODE_UP;
    tbase.Period            = TIM_PERIOD;
    tbase.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    tbase.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    /* ── Input-capture channel init ── */
    TIM_IC_InitTypeDef ic = {0};
    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = 0;  /* no digital filter */

    /* ── TIM2 ── */
    htim2.Instance = TIM2;
    htim2.Init     = tbase;
    HAL_TIM_Base_Init(&htim2);

    ic.ICPolarity = TIM_ICPOLARITY_RISING;
    HAL_TIM_IC_ConfigChannel(&htim2, &ic, TIM_CHANNEL_1);
    HAL_TIM_IC_ConfigChannel(&htim2, &ic, TIM_CHANNEL_2);
    HAL_TIM_IC_ConfigChannel(&htim2, &ic, TIM_CHANNEL_3);
    HAL_TIM_IC_ConfigChannel(&htim2, &ic, TIM_CHANNEL_4);

    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);

    /* ── TIM3 ── */
    htim3.Instance = TIM3;
    htim3.Init     = tbase;
    HAL_TIM_Base_Init(&htim3);

    ic.ICPolarity = TIM_ICPOLARITY_RISING;
    HAL_TIM_IC_ConfigChannel(&htim3, &ic, TIM_CHANNEL_1);
    HAL_TIM_IC_ConfigChannel(&htim3, &ic, TIM_CHANNEL_2);
    HAL_TIM_IC_ConfigChannel(&htim3, &ic, TIM_CHANNEL_3);
    HAL_TIM_IC_ConfigChannel(&htim3, &ic, TIM_CHANNEL_4);

    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4);

    /* ── Enable interrupts ── */
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    /* ── Init per-channel state ── */
    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        pwm_pulse[i]  = 0;
        pwm_rise[i]   = 0;
        pwm_valid[i]  = 0;
        wait_rising[i] = 1; /* start expecting rising edge */
    }
}

/**
 * @brief  Read latest pulse width.
 * @return Pulse width in µs (0 if never captured).
 */
uint32_t PWM_Input_Read(uint8_t channel)
{
    if (channel >= PWM_CHANNEL_COUNT) return 0;
    return pwm_pulse[channel];
}

/**
 * @brief  Check valid-signal flag.
 */
uint8_t PWM_Input_IsValid(uint8_t channel)
{
    if (channel >= PWM_CHANNEL_COUNT) return 0;
    return pwm_valid[channel];
}

/* ── ISR entry points ──────────────────────────────────────────────── */

void TIM2_IRQHandler(void)
{
    capture_isr(TIM2, 0);  /* channels 0~3 */
}

void TIM3_IRQHandler(void)
{
    capture_isr(TIM3, 4);  /* channels 4~7 */
}

/* ── Shared capture processing ─────────────────────────────────────── */

static void capture_isr(TIM_TypeDef *tim, uint8_t base_ch)
{
    uint32_t sr = tim->SR;

    /* ── Process CC1 ── */
    if (sr & TIM_SR_CC1IF) {
        tim->SR = ~TIM_SR_CC1IF;
        uint32_t cap = tim->CCR1;
        uint8_t  ch  = base_ch;

        if (wait_rising[ch]) {
            /* Rising edge captured */
            pwm_rise[ch]  = cap;
            wait_rising[ch] = 0;
            /* Swap to falling edge */
            tim->CCER |= TIM_CCER_CC1P;
        } else {
            /* Falling edge captured */
            uint32_t rise = pwm_rise[ch];
            if (cap < rise) cap += (TIM_PERIOD + 1U);
            uint32_t pulse = cap - rise;

            if (pulse >= PULSE_MIN_US && pulse <= PULSE_MAX_US) {
                pwm_pulse[ch] = pulse;
                pwm_valid[ch] = 1;
            }
            wait_rising[ch] = 1;
            /* Swap back to rising edge */
            tim->CCER &= ~TIM_CCER_CC1P;
        }
    }

    /* ── Process CC2 ── */
    if (sr & TIM_SR_CC2IF) {
        tim->SR = ~TIM_SR_CC2IF;
        uint32_t cap = tim->CCR2;
        uint8_t  ch  = base_ch + 1;

        if (wait_rising[ch]) {
            pwm_rise[ch]  = cap;
            wait_rising[ch] = 0;
            tim->CCER |= TIM_CCER_CC2P;
        } else {
            uint32_t rise = pwm_rise[ch];
            if (cap < rise) cap += (TIM_PERIOD + 1U);
            uint32_t pulse = cap - rise;

            if (pulse >= PULSE_MIN_US && pulse <= PULSE_MAX_US) {
                pwm_pulse[ch] = pulse;
                pwm_valid[ch] = 1;
            }
            wait_rising[ch] = 1;
            tim->CCER &= ~TIM_CCER_CC2P;
        }
    }

    /* ── Process CC3 ── */
    if (sr & TIM_SR_CC3IF) {
        tim->SR = ~TIM_SR_CC3IF;
        uint32_t cap = tim->CCR3;
        uint8_t  ch  = base_ch + 2;

        if (wait_rising[ch]) {
            pwm_rise[ch]  = cap;
            wait_rising[ch] = 0;
            tim->CCER |= TIM_CCER_CC3P;
        } else {
            uint32_t rise = pwm_rise[ch];
            if (cap < rise) cap += (TIM_PERIOD + 1U);
            uint32_t pulse = cap - rise;

            if (pulse >= PULSE_MIN_US && pulse <= PULSE_MAX_US) {
                pwm_pulse[ch] = pulse;
                pwm_valid[ch] = 1;
            }
            wait_rising[ch] = 1;
            tim->CCER &= ~TIM_CCER_CC3P;
        }
    }

    /* ── Process CC4 ── */
    if (sr & TIM_SR_CC4IF) {
        tim->SR = ~TIM_SR_CC4IF;
        uint32_t cap = tim->CCR4;
        uint8_t  ch  = base_ch + 3;

        if (wait_rising[ch]) {
            pwm_rise[ch]  = cap;
            wait_rising[ch] = 0;
            tim->CCER |= TIM_CCER_CC4P;
        } else {
            uint32_t rise = pwm_rise[ch];
            if (cap < rise) cap += (TIM_PERIOD + 1U);
            uint32_t pulse = cap - rise;

            if (pulse >= PULSE_MIN_US && pulse <= PULSE_MAX_US) {
                pwm_pulse[ch] = pulse;
                pwm_valid[ch] = 1;
            }
            wait_rising[ch] = 1;
            tim->CCER &= ~TIM_CCER_CC4P;
        }
    }
}
