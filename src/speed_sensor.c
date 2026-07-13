/**
 * @file    speed_sensor.c
 * @brief   双电机正交解码 — 左硬件(TIM1) + 右软件(EXTI)
 *
 * Pin mapping:
 *   PA8  → Left motor A phase   (TIM1_CH1)
 *   PA9  → Left motor B phase   (TIM1_CH2)
 *   PB14 → Right motor A phase  (EXTI14)
 *   PB15 → Right motor B phase  (EXTI15)
 *
 * 左电机使用 TIM1 编码器模式硬件计数（无中断开销）。
 * 右电机使用 EXTI15_10 中断 + 软件状态机解码。
 *
 * Speed_GetRPM() 阻塞 50ms 采样测速，调用频率不要超过 ~20 Hz。
 */

#include "speed_sensor.h"
#include "stm32f1xx_hal.h"

/* ── Pin definitions ────────────────────────────────────────────────── *
 * Left  motor — TIM1 encoder mode on PA8/PA9  (hardware, no ISR)
 * Right motor — EXTI software decoding on PB14/PB15
 * ────────────────────────────────────────────────────────────────────── */
#define RIGHT_A_PIN  GPIO_PIN_14
#define RIGHT_B_PIN  GPIO_PIN_15
#define RIGHT_PINS   (RIGHT_A_PIN | RIGHT_B_PIN)

/* ── EXTI decoder state (right motor only) ──────────────────────────── */
static volatile int32_t  right_pos;       /* signed position counter */
static volatile uint8_t  right_last_ab;   /* bit[0]=A, bit[1]=B */

/* ── TIM1 16-bit encoder → 32-bit extension (left motor) ────────────── */
static TIM_HandleTypeDef htim1_enc;
static int32_t  tim1_accum;
static uint16_t tim1_last_cnt;

/* ── Quadrature state-transition table ──────────────────────────────── *
 * Index: (prev_AB ≪ 2) | curr_AB   (AB = (B≪1) | A)
 *   +1 = CW (forward),  –1 = CCW (reverse),  0 = invalid / glitch
 * ────────────────────────────────────────────────────────────────────── */
static const int8_t qtable[16] = {
     0,  1, -1,  0,   /* 00 → 00,01,10,11 */
    -1,  0,  0,  1,   /* 01 → 00,01,10,11 */
     1,  0,  0, -1,   /* 10 → 00,01,10,11 */
     0, -1,  1,  0,   /* 11 → 00,01,10,11 */
};

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════ */

void SpeedSensor_Init(void)
{
    /* ── TIM1 encoder mode — left motor (PA8/PA9) ──────────────────── */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    TIM_Encoder_InitTypeDef enc = {0};
    enc.EncoderMode        = TIM_ENCODERMODE_TI12;
    enc.IC1Polarity        = TIM_ICPOLARITY_RISING;
    enc.IC1Selection       = TIM_ICSELECTION_DIRECTTI;
    enc.IC1Prescaler       = TIM_ICPSC_DIV1;
    enc.IC1Filter          = 0;
    enc.IC2Polarity        = TIM_ICPOLARITY_RISING;
    enc.IC2Selection       = TIM_ICSELECTION_DIRECTTI;
    enc.IC2Prescaler       = TIM_ICPSC_DIV1;
    enc.IC2Filter          = 0;

    htim1_enc.Instance               = TIM1;
    htim1_enc.Init.Prescaler         = 0;
    htim1_enc.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1_enc.Init.Period            = 0xFFFF;
    htim1_enc.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1_enc.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Encoder_Init(&htim1_enc, &enc);

    HAL_TIM_Encoder_Start(&htim1_enc, TIM_CHANNEL_1);
    HAL_TIM_Encoder_Start(&htim1_enc, TIM_CHANNEL_2);

    tim1_last_cnt = 0;
    tim1_accum    = 0;

    /* ── EXTI software decoder — right motor (PB14/PB15) ──────────── */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    g.Pin   = RIGHT_PINS;
    g.Mode  = GPIO_MODE_IT_RISING_FALLING;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    uint32_t idr = GPIOB->IDR;
    right_last_ab = (uint8_t)(((idr & RIGHT_B_PIN) ? 2 : 0) |
                               ((idr & RIGHT_A_PIN) ? 1 : 0));
    right_pos = 0;
}

int32_t SpeedSensor_GetPosition(uint8_t motor)
{
    if (motor > 1) return 0;

    if (motor == MOTOR_LEFT) {
        /* 16-bit TIM1 counter → 32-bit accumulation */
        uint16_t cnt  = TIM1->CNT;
        int16_t  diff = (int16_t)(cnt - tim1_last_cnt);
        tim1_accum   += diff;
        tim1_last_cnt = cnt;
        return tim1_accum;
    }

    return right_pos;
}

void SpeedSensor_Reset(uint8_t motor)
{
    if (motor > 1) return;

    if (motor == MOTOR_LEFT) {
        /* 直接在 TIM1->CNT 写 0 复位硬件计数器 */
        TIM1->CNT = 0;
        tim1_last_cnt = 0;
        tim1_accum    = 0;
        return;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    right_pos = 0;
    if (!primask) __enable_irq();
}

uint16_t SpeedSensor_GetRPM(uint8_t motor, uint16_t ppr)
{
    if (motor > 1 || ppr == 0) return 0;

    /* 使用 GetPosition 统一读取（兼容 TIM1 与 EXTI） */
    int32_t pos_a = SpeedSensor_GetPosition(motor);
    HAL_Delay(50);                          /* 50 ms sampling window */

    int32_t pos_b = SpeedSensor_GetPosition(motor);
    int32_t delta = pos_b - pos_a;
    if (delta < 0) delta = -delta;          /* absolute speed */

    /* RPM = (delta_counts × 60000) / (50_ms × 4 × ppr) */
    uint32_t num = (uint32_t)delta * 60000UL;
    uint32_t den = 50UL * 4UL * (uint32_t)ppr;
    if (den == 0) return 0;

    uint32_t rpm = num / den;
    return (uint16_t)(rpm > 65535U ? 65535U : rpm);
}

uint8_t SpeedSensor_IsValid(uint8_t motor)
{
    if (motor > 1) return 0;
    return SpeedSensor_GetPosition(motor) != 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ISR  —  EXTI15_10  (右电机 PB14/PB15 软件解码)
 *  左电机已用 TIM1 硬件编码器，无需中断
 * ═══════════════════════════════════════════════════════════════════════ */

void EXTI15_10_IRQHandler(void)
{
    /* ── Right motor (PB14, PB15) ── */
    if (EXTI->PR & (EXTI_PR_PR14 | EXTI_PR_PR15)) {
        uint32_t idr = GPIOB->IDR;
        uint8_t a = (idr & RIGHT_A_PIN) ? 1 : 0;
        uint8_t b = (idr & RIGHT_B_PIN) ? 1 : 0;
        uint8_t curr = (b << 1) | a;
        uint8_t idx  = ((right_last_ab & 0x03) << 2) | curr;

        right_pos     += qtable[idx];
        right_last_ab  = curr;

        EXTI->PR = EXTI_PR_PR14 | EXTI_PR_PR15;   /* clear flags */
    }
}
