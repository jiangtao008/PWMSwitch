/**
 * @file    control.c
 * @brief   输入 → 输出 转换逻辑（开环直驱，无 PID）
 *
 *  in_ch_1~8  输入通道，值 0~100（摇杆中位=50，无效=50）
 *  out_ch_1~4 PWM 输出，值 0~100（占空比）
 *  out_ch_9~12 数字输出，值 0 或 1  (PA15/PB3~PB5)
 */

#include "control.h"
#include "pwm_input.h"
#include "pwm_output.h"
#include "digital_output.h"
#include "display.h"
#include "speed_sensor.h"

void Control_Init(void)
{
    PWM_Input_Init();
    PWM_Output_Init();
    Digital_Output_Init();
    SpeedSensor_Init();
}

void Control_Update(void)
{
    /* ── 读取输入（0~100）────────────────────────────────────── */
    uint8_t in_ch_1 = PWM_Input_GetPercent(0);
    uint8_t in_ch_2 = PWM_Input_GetPercent(1);
    uint8_t in_ch_3 = PWM_Input_GetPercent(2);
    uint8_t in_ch_4 = PWM_Input_GetPercent(3);
    uint8_t in_ch_5 = PWM_Input_GetPercent(4);
    uint8_t in_ch_6 = PWM_Input_GetPercent(5);
    uint8_t in_ch_7 = PWM_Input_GetPercent(6);
    uint8_t in_ch_8 = PWM_Input_GetPercent(7);

    /* ── 输出变量（默认值）───────────────────────────────────── */
    uint8_t out_ch_1  = 0;    /* PWM  0~100 */
    uint8_t out_ch_2  = 0;
    uint8_t out_ch_3  = 0;
    uint8_t out_ch_4  = 0;
    uint8_t out_ch_5  = 0;    /* 数字 0/1 (PA15) */
    uint8_t out_ch_6  = 0;    /* (PB3) */
    uint8_t out_ch_7  = 0;    /* (PB4) */
    uint8_t out_ch_8  = 0;    /* (PB5) */

    /* ── 坦克差速混控 ────────── */
    /* CH2 油门, CH1 转向, 50=中位 */
    int16_t thr   = (int16_t)in_ch_2 - 50;
    int16_t steer = (int16_t)in_ch_1 - 50;

    /* 油门输入死区：±2 以内的偏差直接归零，防接收机中位偏移被 2x 放大 */
    if (thr > -2 && thr < 2) thr = 0;
    thr   *= 2;   /* -100..+100 */
    /* Expo 曲线：方向小幅度平缓、大幅度保持满量程 */
    {
        int16_t s_sign = (steer > 0) ? 1 : ((steer < 0) ? -1 : 0);
        int16_t s_abs  = (steer > 0) ? steer : -steer;
        steer = s_sign * (s_abs * s_abs / 50);
    }

    int16_t left  = thr + steer;
    int16_t right = thr - steer;

    /* 限幅 */
    if (left  >  100) left  =  100;
    if (left  < -100) left  = -100;
    if (right >  100) right =  100;
    if (right < -100) right = -100;

    /* 左电机 (CH1/CH2) */
    if (left > 3) {
        out_ch_1 = 0;
        out_ch_2 = (uint8_t)left;       /* 正转 */
    } else if (left < -3) {
        out_ch_1 = (uint8_t)(-left);    /* 反转 */
        out_ch_2 = 0;
    } else {
        out_ch_1 = 0;                   /* 死区 */
        out_ch_2 = 0;
    }

    /* 右电机 (CH3/CH4) */
    if (right > 3) {
        out_ch_3 = 0;
        out_ch_4 = (uint8_t)right;      /* 正转 */
    } else if (right < -3) {
        out_ch_3 = (uint8_t)(-right);   /* 反转 */
        out_ch_4 = 0;
    } else {
        out_ch_3 = 0;                   /* 死区 */
        out_ch_4 = 0;
    }

    /* ── 数字开关输出 ────────────────────────────────────────── */
    const int switchCenter = 75;
    out_ch_5 = (in_ch_5 > switchCenter) ? 1 : 0;
    out_ch_6 = (in_ch_6 > switchCenter) ? 1 : 0;
    out_ch_7 = (in_ch_7 > switchCenter) ? 1 : 0;
    out_ch_8 = (in_ch_8 > switchCenter) ? 1 : 0;

    /* ── 写入输出 ──────────────────────────────────────────── */
    Digital_Output_Set(0, out_ch_5);
    Digital_Output_Set(1, out_ch_6);
    Digital_Output_Set(2, out_ch_7);
    Digital_Output_Set(3, out_ch_8);

    PWM_Output_Set(0, out_ch_1);
    PWM_Output_Set(1, out_ch_2);
    PWM_Output_Set(2, out_ch_3);
    PWM_Output_Set(3, out_ch_4);

    /* ── Update display ──────────────────────────────────────── */
    int32_t l_pos = SpeedSensor_GetPosition(MOTOR_LEFT);
    int32_t r_pos = SpeedSensor_GetPosition(MOTOR_RIGHT);

    const uint8_t in_arr[8]   = {in_ch_1, in_ch_2, in_ch_3, in_ch_4,
                                 in_ch_5, in_ch_6, in_ch_7, in_ch_8};
    const uint8_t out_pct[4]  = {out_ch_1, out_ch_2, out_ch_3, out_ch_4};
    const uint8_t out_dig[8]  = {0, 0, 0, 0,
                                 out_ch_5, out_ch_6, out_ch_7, out_ch_8};

    Display_Update(in_arr, out_pct, out_dig, l_pos, r_pos);
}
