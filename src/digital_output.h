/**
 * @file    digital_output.h
 * @brief   4-channel boolean output — PB12 / PB13 / PB14 / PB15
 */

#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIGITAL_OUT_CHANNELS  4

/**
 * @brief  Initialise PB12~PB15 as push-pull outputs, all LOW.
 */
void Digital_Output_Init(void);

/**
 * @brief  Set output level.
 * @param  channel  0~3  (PB12 ~ PB15)
 * @param  value    0 = LOW, non-zero = HIGH
 */
void Digital_Output_Set(uint8_t channel, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* DIGITAL_OUTPUT_H */
