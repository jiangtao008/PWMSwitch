/**
 * @file    pwm_output.h
 * @brief   4-channel PWM output using TIM4 (CH1~CH4)
 *
 * Frequency is configurable via the global variable `pwm_output_freq_hz`.
 * Default: 10000 Hz (10 kHz).
 */

#ifndef PWM_OUTPUT_H
#define PWM_OUTPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_OUT_CHANNELS  4

/** Global: PWM frequency in Hz. Change at runtime then call PWM_Output_ApplyFreq(). */
extern uint32_t pwm_output_freq_hz;

/**
 * @brief  Initialise TIM4 with 4 PWM channels.
 *         Sets default frequency (10 kHz) and starts output.
 */
void PWM_Output_Init(void);

/**
 * @brief  Apply the current value of `pwm_output_freq_hz`.
 *         Call this after changing the global variable.
 */
void PWM_Output_ApplyFreq(void);

/**
 * @brief  Set duty cycle for one channel.
 * @param  channel  0~3  (CH1~CH4)
 * @param  percent  0~100 (0% = always low, 100% = always high)
 */
void PWM_Output_Set(uint8_t channel, uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif /* PWM_OUTPUT_H */
