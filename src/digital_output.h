/**
 * @file    digital_output.h
 * @brief   4-channel boolean output — PA15, PB3, PB4, PB5
 *
 * PB12~PB15 were freed for quadrature encoder inputs (see speed_sensor.c).
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
 *          CH3: PB4
 *          CH4: PB5
 */
void Digital_Output_Init(void);

/**
 * @brief  Set output level.
 * @param  channel  0~3  (0: PA15, 1: PB3, 2: PB4, 3: PB5)
 * @param  value    0 = LOW, non-zero = HIGH
 */
void Digital_Output_Set(uint8_t channel, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* DIGITAL_OUTPUT_H */
