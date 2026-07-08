/**
 * @file    speed_sensor.c
 * @brief   2-motor quadrature decoding — software EXTI on PB12~PB15
 *
 * Pin mapping:
 *   PB12 → Left motor A phase   (EXTI12)
 *   PB13 → Left motor B phase   (EXTI13)
 *   PB14 → Right motor A phase  (EXTI14)
 *   PB15 → Right motor B phase  (EXTI15)
 *
 * Both edges trigger EXTI15_10_IRQHandler; the ISR reads the full
 * (A,B) pair for the affected motor and walks a 4-state lookup table
 * to produce +1 (CW) or –1 (CCW) counts at 4× encoder-line resolution.
 *
 * Speed_GetRPM() samples position twice with a short delay — caller
 * should avoid calling it more than ~20 Hz.
 */

#include "speed_sensor.h"
#include "stm32f1xx_hal.h"

/* ── Pin definitions ────────────────────────────────────────────────── */
#define LEFT_A_PIN   GPIO_PIN_12
#define LEFT_B_PIN   GPIO_PIN_13
#define RIGHT_A_PIN  GPIO_PIN_14
#define RIGHT_B_PIN  GPIO_PIN_15
#define ALL_PINS     (LEFT_A_PIN | LEFT_B_PIN | RIGHT_A_PIN | RIGHT_B_PIN)

/* ── Per-motor state ────────────────────────────────────────────────── */
static volatile int32_t encoder_pos[2];      /* signed position counter */
static volatile uint8_t last_ab[2];          /* bit[0]=A, bit[1]=B */

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
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* ── GPIO + EXTI: both edges, pull-up ── */
    GPIO_InitTypeDef g = {0};
    g.Pin   = ALL_PINS;
    g.Mode  = GPIO_MODE_IT_RISING_FALLING;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    /* ── NVIC ── */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* ── Read initial state ── */
    uint32_t idr = GPIOB->IDR;
    last_ab[0] = (uint8_t)(((idr & LEFT_B_PIN)  ? 2 : 0) |
                            ((idr & LEFT_A_PIN)  ? 1 : 0));
    last_ab[1] = (uint8_t)(((idr & RIGHT_B_PIN) ? 2 : 0) |
                            ((idr & RIGHT_A_PIN) ? 1 : 0));
    encoder_pos[0] = 0;
    encoder_pos[1] = 0;
}

int32_t SpeedSensor_GetPosition(uint8_t motor)
{
    if (motor > 1) return 0;
    return encoder_pos[motor];
}

void SpeedSensor_Reset(uint8_t motor)
{
    if (motor > 1) return;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    encoder_pos[motor] = 0;
    if (!primask) __enable_irq();
}

uint16_t SpeedSensor_GetRPM(uint8_t motor, uint16_t ppr)
{
    if (motor > 1 || ppr == 0) return 0;

    /* Sample position A */
    int32_t pos_a = encoder_pos[motor];
    HAL_Delay(50);                          /* 50 ms sampling window */

    /* Sample position B */
    int32_t pos_b = encoder_pos[motor];
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
    return encoder_pos[motor] != 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ISR  —  EXTI15_10  (shared for PB12~PB15)
 * ═══════════════════════════════════════════════════════════════════════ */

void EXTI15_10_IRQHandler(void)
{
    uint32_t pr = EXTI->PR;

    /* ── Left motor (PB12, PB13) ── */
    if (pr & (EXTI_PR_PR12 | EXTI_PR_PR13)) {
        uint32_t idr = GPIOB->IDR;
        uint8_t a = (idr & LEFT_A_PIN) ? 1 : 0;
        uint8_t b = (idr & LEFT_B_PIN) ? 1 : 0;
        uint8_t curr = (b << 1) | a;
        uint8_t idx  = ((last_ab[0] & 0x03) << 2) | curr;

        encoder_pos[0] += qtable[idx];
        last_ab[0]      = curr;

        EXTI->PR = EXTI_PR_PR12 | EXTI_PR_PR13;   /* clear flags */
    }

    /* ── Right motor (PB14, PB15) ── */
    if (pr & (EXTI_PR_PR14 | EXTI_PR_PR15)) {
        uint32_t idr = GPIOB->IDR;
        uint8_t a = (idr & RIGHT_A_PIN) ? 1 : 0;
        uint8_t b = (idr & RIGHT_B_PIN) ? 1 : 0;
        uint8_t curr = (b << 1) | a;
        uint8_t idx  = ((last_ab[1] & 0x03) << 2) | curr;

        encoder_pos[1] += qtable[idx];
        last_ab[1]      = curr;

        EXTI->PR = EXTI_PR_PR14 | EXTI_PR_PR15;   /* clear flags */
    }
}
