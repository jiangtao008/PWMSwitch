/**
 * @file    digital_output.h
 * @brief   4-channel boolean output — PA15, PB3, PB12, PB13
 *
 * PB12/PB13 原为左电机编码器，已迁移到 TIM1 硬件模式 (PA8/PA9)。
 */

#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIGITAL_OUT_CHANNELS  4

/**
 * @brief  Initialise digital output pins, all LOW.
 *
 *          CH1: PA15
 *          CH2: PB3
 *          CH3: PB12
 *          CH4: PB13
 */
void Digital_Output_Init(void);

/**
 * @brief  Set output level.
 * @param  channel  0~3  (0: PA15, 1: PB3, 2: PB12, 3: PB13)
 * @param  value    0 = LOW, non-zero = HIGH
 */
void Digital_Output_Set(uint8_t channel, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* DIGITAL_OUTPUT_H */
