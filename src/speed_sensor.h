/**
 * @file    speed_sensor.h
 * @brief   2-motor quadrature encoder decoding (software EXTI)
 *
 * PB12/PB13 = left motor A/B phase
 * PB14/PB15 = right motor A/B phase
 * Software quadrature state-machine via EXTI10-15 ISR.
 */

#ifndef SPEED_SENSOR_H
#define SPEED_SENSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Motor index. */
#define MOTOR_LEFT   0
#define MOTOR_RIGHT  1

/**
 * @brief  Initialise EXTI on PB12~PB15 for quadrature reading.
 *         Both-edge triggers, internal pull-up, priority 2.
 */
void SpeedSensor_Init(void);

/**
 * @brief  Get accumulated encoder position (signed, 4× counts per line).
 *         Positive = forward, negative = reverse.
 * @param  motor  MOTOR_LEFT or MOTOR_RIGHT
 * @return Encoder position in quadrature counts.
 */
int32_t SpeedSensor_GetPosition(uint8_t motor);

/**
 * @brief  Reset encoder position counter to zero.
 * @param  motor  MOTOR_LEFT or MOTOR_RIGHT
 */
void SpeedSensor_Reset(uint8_t motor);

/**
 * @brief  Calculate motor speed in RPM.
 *
 *         Reads position twice with a small delay between samples,
 *         so this call blocks ~50 ms.
 *
 * @param  motor  MOTOR_LEFT or MOTOR_RIGHT
 * @param  ppr    Encoder lines (pulses) per revolution (before 4× quadrature)
 * @return RPM, or 0 if no movement detected.
 */
uint16_t SpeedSensor_GetRPM(uint8_t motor, uint16_t ppr);

/**
 * @brief  Check whether the encoder has produced any counts.
 * @param  motor  MOTOR_LEFT or MOTOR_RIGHT
 * @return 1 if position is non-zero, 0 if still at zero.
 */
uint8_t SpeedSensor_IsValid(uint8_t motor);

#ifdef __cplusplus
}
#endif

#endif /* SPEED_SENSOR_H */
