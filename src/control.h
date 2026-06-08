/**
 * @file    control.h
 * @brief   Input → Output control logic.
 *
 * Edit control.c to implement your own mapping.
 * All values are 0~100 (PWM) or 0/1 (digital).
 */

#ifndef CONTROL_H
#define CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/** One-time initialisation: calls all sub-module init functions. */
void Control_Init(void);

/** Called every loop iteration.  Reads inputs, writes outputs. */
void Control_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_H */
