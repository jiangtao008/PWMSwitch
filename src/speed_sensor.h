/**
 * @file    speed_sensor.h
 * @brief   2-motor speed measurement via TIM1 input capture (4 channels)
 *
 * TIM1 CH1~CH4 on PA8~PA11 capture pulse period from motor Hall/encoder lines.
 * Each motor has 2 sensor lines; the module averages them for one speed value.
 */

#ifndef SPEED_SENSOR_H
#define SPEED_SENSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Motor index for API functions. */
#define MOTOR_LEFT   0
#define MOTOR_RIGHT  1

/**
 * @brief  Initialise TIM1 with 4 input-capture channels.
 *         PA8=CH1, PA9=CH2, PA10=CH3, PA11=CH4.
 *         All capture rising edges.
 */
void SpeedSensor_Init(void);

/**
 * @brief  Get latest pulse period for a motor.
 * @param  motor  MOTOR_LEFT (0) or MOTOR_RIGHT (1)
 * @return Pulse period in timer ticks (9 µs/tick @ 8 MHz).
 *         Returns 0 if no valid pulse has been captured.
 *         The value is averaged from the motor's two sensor lines.
 */
uint32_t SpeedSensor_GetPeriod(uint8_t motor);

/**
 * @brief  Get motor speed in RPM.
 * @param  motor  MOTOR_LEFT or MOTOR_RIGHT
 * @param  ppr    Pulses per revolution (total from both sensor lines)
 * @return Speed in RPM, or 0 if no valid measurement.
 */
uint16_t SpeedSensor_GetRPM(uint8_t motor, uint8_t ppr);

/**
 * @brief  Check whether a valid speed measurement is available.
 * @param  motor  MOTOR_LEFT or MOTOR_RIGHT
 * @return 1 if at least one sensor line has captured a pulse.
 */
uint8_t SpeedSensor_IsValid(uint8_t motor);

#ifdef __cplusplus
}
#endif

#endif /* SPEED_SENSOR_H */
