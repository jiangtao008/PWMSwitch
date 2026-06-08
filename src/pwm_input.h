#ifndef PWM_INPUT_H
#define PWM_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_CHANNEL_COUNT 8

/** PWM input pin mapping:
 *  CH1: PA0 (TIM2_CH1)    CH5: PA6 (TIM3_CH1)
 *  CH2: PA1 (TIM2_CH2)    CH6: PA7 (TIM3_CH2)
 *  CH3: PA2 (TIM2_CH3)    CH7: PB0 (TIM3_CH3)
 *  CH4: PA3 (TIM2_CH4)    CH8: PB1 (TIM3_CH4)
 */

/**
 * @brief  Initialize TIM2 & TIM3 for 8-channel PWM input capture.
 *         Timer clock = 1MHz → 1µs resolution.
 *         Call once at startup.
 */
void PWM_Input_Init(void);

/**
 * @brief  Read latest pulse width for a channel.
 * @param  channel  0~7
 * @return Pulse width in microseconds (e.g. 1000~2000 for standard RC),
 *         or 0 if no valid signal detected yet.
 */
uint32_t PWM_Input_Read(uint8_t channel);

/**
 * @brief  Check if channel has received a valid signal.
 * @param  channel  0~7
 * @return 1 = valid signal, 0 = no signal or signal lost
 */
uint8_t PWM_Input_IsValid(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* PWM_INPUT_H */
