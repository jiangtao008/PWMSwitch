/**
 * @file    control.c
 * @brief   输入 → 输出 转换逻辑（只改中间部分）
 *
 *  in_ch_1~8  输入通道，值 0~100（摇杆中位=50，无效=50）
 *  out_ch_1~4 PWM 输出，值 0~100（占空比）
 *  out_ch_5~8 数字输出，值 0 或 1
 */

#include "control.h"
#include "pwm_input.h"
#include "pwm_output.h"
#include "digital_output.h"
#include "display.h"

#define ABS(x)  ((x) < 0 ? -(x) : (x))

/* ═══════════════════════════════════════════════════════════════════
 *  下面的 Init 和 Update 外层框架不要动
 *  只改 Update 中间标记出来的转换逻辑
 * ═══════════════════════════════════════════════════════════════════ */

void Control_Init(void)
{
    PWM_Input_Init();
    PWM_Output_Init();
    Digital_Output_Init();
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
    uint8_t out_ch_5  = 0;    /* 数字 0/1 */
    uint8_t out_ch_6  = 0;
    uint8_t out_ch_7  = 0;
    uint8_t out_ch_8  = 0;
    uint8_t out_ch_9  = 0;    /* 新增数字 0/1 (PA15) */
    uint8_t out_ch_10 = 0;    /* (PB3) */
    uint8_t out_ch_11 = 0;    /* (PB4) */
    uint8_t out_ch_12 = 0;    /* (PB5) */

    (void)in_ch_3; (void)in_ch_4; (void)in_ch_7; (void)in_ch_8;   /* demo 未用，消 warning */

    /* ── 坦克混控 demo（替换成你自己的逻辑） ──────────────── */
    int16_t thr   = (int16_t)in_ch_2 - 50;   /* CH2 油门:  -50..+50 */
    int16_t steer = (int16_t)in_ch_1 - 50;   /* CH1 转向:  -50..+50 */

    int16_t left  = thr + steer;
    int16_t right = thr - steer;

    if (left  >  100) left  =  100;
    if (left  < -100) left  = -100;
    if (right >  100) right =  100;
    if (right < -100) right = -100;

    // 输出量
    out_ch_1 = (uint8_t)ABS(left);
    out_ch_9  = (left < 0) ? 1 : 0;
    out_ch_10 = (left >= 0) ? 1 : 0;

    out_ch_2 = (uint8_t)ABS(left);
    out_ch_11 = (right < 0) ? 1 : 0;
    out_ch_12 = (right >= 0) ? 1 : 0;

    out_ch_5 = (in_ch_5 > 50) ? 1 : 0;
    out_ch_6 = (in_ch_6 > 50) ? 1 : 0;
    out_ch_7 = (in_ch_7 > 50) ? 1 : 0;
    out_ch_8 = (in_ch_8 > 50) ? 1 : 0;

    /* ── 写入输出 ──────────────────────────────────────────── */
    Digital_Output_Set(0, out_ch_5);
    Digital_Output_Set(1, out_ch_6);
    Digital_Output_Set(2, out_ch_7);
    Digital_Output_Set(3, out_ch_8);
    Digital_Output_Set(4, out_ch_9);
    Digital_Output_Set(5, out_ch_10);
    Digital_Output_Set(6, out_ch_11);
    Digital_Output_Set(7, out_ch_12);

    PWM_Output_Set(0, out_ch_1);
    PWM_Output_Set(1, out_ch_2);
    PWM_Output_Set(2, out_ch_3);
    PWM_Output_Set(3, out_ch_4);

    /* ── Update display ──────────────────────────────────────── */
    const uint8_t in_arr[8]   = {in_ch_1, in_ch_2, in_ch_3, in_ch_4,
                                 in_ch_5, in_ch_6, in_ch_7, in_ch_8};
    const uint8_t out_pct[4]  = {out_ch_1, out_ch_2, out_ch_3, out_ch_4};
    const uint8_t out_dig[8]  = {out_ch_5, out_ch_6, out_ch_7, out_ch_8,
                                 out_ch_9, out_ch_10, out_ch_11, out_ch_12};

    Display_Update(in_arr, out_pct, out_dig);
}
