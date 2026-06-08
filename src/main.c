/**
 * @file    main.c
 * @brief   Tank-drive RC receiver → motor controller
 *
 * Input:
 *   CH1 (PA0) — steering joystick, 1000~1500~2000 µs (left~center~right)
 *   CH2 (PA1) — throttle joystick, 1000~1500~2000 µs (back~center~fwd)
 *   CH3~CH8  — auxiliary channels
 *
 * Output:
 *   CH1 (PB6) — left  motor forward  PWM
 *   CH2 (PB7) — left  motor reverse  PWM
 *   CH3 (PB8) — right motor forward  PWM
 *   CH4 (PB9) — right motor reverse  PWM
 *   CH5~CH8   — digital outputs (PB12~PB15)
 */

#include "stm32f1xx_hal.h"
#include "pwm_input.h"
#include "pwm_output.h"
#include "digital_output.h"

/* ── RC signal constants ───────────────────────────────────────────── */
#define RC_MIN_US    1000U
#define RC_MID_US    1500U
#define RC_MAX_US    2000U

/* ── Forward declarations ──────────────────────────────────────────── */
static void     GPIO_Init(void);
static int16_t  map_signed(uint32_t pulse_us);   /* → -100..+100 */
static uint8_t  map_bool(uint32_t pulse_us);

/* ──────────────────────────────────────────────────────────────────── */
int main(void)
{
    SystemCoreClock = 8000000;
    HAL_Init();
    GPIO_Init();

    PWM_Input_Init();
    PWM_Output_Init();
    Digital_Output_Init();

    uint32_t last_tick = 0;

    while (1)
    {
        /* ── Tank mixing ──────────────────────────────────────── */

        int16_t throttle = 0;   /* CH2: forward / backward  -100..+100 */
        int16_t steering = 0;   /* CH1: right / left         -100..+100 */

        if (PWM_Input_IsValid(2 - 1))   /* CH2 = throttle */
            throttle = map_signed(PWM_Input_Read(2 - 1));
        if (PWM_Input_IsValid(1 - 1))   /* CH1 = steering */
            steering = map_signed(PWM_Input_Read(1 - 1));

        /* Tank mixing formula:
         *   left  = throttle + steering
         *   right = throttle - steering                           */
        int16_t left  = throttle + steering;
        int16_t right = throttle - steering;

        /* Clamp to ±100 */
        if (left  >  100) left  =  100;
        if (left  < -100) left  = -100;
        if (right >  100) right =  100;
        if (right < -100) right = -100;

        /* Left motor: CH1=forward, CH2=reverse */
        if (left >= 0) {
            PWM_Output_Set(0, (uint8_t)left);    /* CH1: fwd */
            PWM_Output_Set(1, 0);                /* CH2: rev */
        } else {
            PWM_Output_Set(0, 0);
            PWM_Output_Set(1, (uint8_t)(-left));
        }

        /* Right motor: CH3=forward, CH4=reverse */
        if (right >= 0) {
            PWM_Output_Set(2, (uint8_t)right);   /* CH3: fwd */
            PWM_Output_Set(3, 0);                /* CH4: rev */
        } else {
            PWM_Output_Set(2, 0);
            PWM_Output_Set(3, (uint8_t)(-right));
        }

        /* ── CH5~CH8 digital outputs ─────────────────────────── */
        for (uint8_t i = 4; i < 8; i++) {
            if (PWM_Input_IsValid(i))
                Digital_Output_Set(i - 4, map_bool(PWM_Input_Read(i)));
            else
                Digital_Output_Set(i - 4, 0);
        }

        /* ── LED: blink rate reflects throttle magnitude ────────
         *  stop → 1 Hz  /  full throttle → 10 Hz                  */
        uint32_t now = HAL_GetTick();

        if (PWM_Input_IsValid(1)) {  /* CH2 (throttle) */
            uint32_t pulse = PWM_Input_Read(1);

            /* map 1000~2000µs → half-period 500~50ms (1~10 Hz) */
            uint32_t half_period;
            if (pulse <= RC_MIN_US)
                half_period = 500;
            else if (pulse >= RC_MAX_US)
                half_period = 50;
            else
                half_period = 500U - (pulse - RC_MIN_US) * 450U / 1000U;

            if (now - last_tick >= half_period) {
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                last_tick = now;
            }
        } else {
            /* No signal: double-flash pattern every 1 second */
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

/* ── Helpers ───────────────────────────────────────────────────────── */

/**
 * @brief  Map RC pulse to signed value: 1000→-100, 1500→0, 2000→+100
 */
static int16_t map_signed(uint32_t pulse_us)
{
    if (pulse_us <= RC_MIN_US) return -100;
    if (pulse_us >= RC_MAX_US) return  100;

    /* (pulse - 1000) * 200 / 1000  -  100   →  -100 .. +100 */
    return (int16_t)(((pulse_us - RC_MIN_US) * 200U) / 1000U) - 100;
}

/**
 * @brief  Map RC pulse to boolean: above 1500µs = HIGH.
 */
static uint8_t map_bool(uint32_t pulse_us)
{
    return (pulse_us > RC_MID_US) ? 1 : 0;
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
