/**
 * @file    digital_output.h
 * @brief   8-channel boolean output — PB12~PB15, PA15, PB3~PB5
 */

#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIGITAL_OUT_CHANNELS  8

/**
 * @brief  Initialise digital output pins, all LOW.
 *
 *          CH1~CH4: PB12~PB15
 *          CH5:     PA15
 *          CH6~CH8: PB3~PB5
 */
void Digital_Output_Init(void);

/**
 * @brief  Set output level.
 * @param  channel  0~7  (0-3: PB12~PB15, 4: PA15, 5-7: PB3~PB5)
 * @param  value    0 = LOW, non-zero = HIGH
 */
void Digital_Output_Set(uint8_t channel, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* DIGITAL_OUTPUT_H */
