/**
 * @file    control.c
 * @brief   输入 → 输出 转换逻辑（只改中间部分）
 *
 *  in_ch_1~8  输入通道，值 0~100（摇杆中位=50，无效=50）
 *  out_ch_1~4 PWM 输出，值 0~100（占空比）
 *  out_ch_9~12 数字输出，值 0 或 1  (PA15/PB3~PB5)
 *
 *  PB12~PB15 给 speed_sensor 做正交编码测速。
 */

#include "control.h"
#include "pwm_input.h"
#include "pwm_output.h"
#include "digital_output.h"
#include "display.h"
#include "speed_sensor.h"
#include "stm32f1xx_hal.h"      /* HAL_GetTick */

#define ABS(x)  ((x) < 0 ? -(x) : (x))

/* ═══════════════════════════════════════════════════════════════════
 *  PID 参数 — 根据实际电机 + 底盘调试
 * ═══════════════════════════════════════════════════════════════════ */

/** 油门 100% 时对应的编码器目标速度（counts/sec）.
 *  MG513 电机轴编码器 13PPR × 4 正交 × 30 减速比:
 *  11000 RPM / 60 × 52 ≈ 9500 counts/sec */
#define PID_MAX_SPEED_CPS   9500.0f

/* 归一化 PID 参数（误差 ±1.0 = ±100% 速度） */
#define PID_KP              80.0f   /* 比例: 100% 误差 → 80% PWM */
#define PID_KI              30.0f   /* 积分: 1 秒全误差累积 → 30% PWM */
#define PID_KD              0.0f    /* 微分暂不启用 */
#define PID_INTEGRAL_LIMIT  2.0f    /* 积分上限 ±2.0（≈ ±60% PWM 稳态输出） */

/* ── PID 控制器结构 ──────────────────────────────────────────────── */
typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_error;
    float out_min, out_max;
    float integral_limit;
} PID_t;

static void PID_Reset(PID_t *pid)
{
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
}

static float PID_Compute(PID_t *pid, float setpoint, float measurement, float dt)
{
    /* 归一化误差 ±1.0 = ±100% 速度，使 PID 参数不依赖 MAX_SPEED */
    float error = (setpoint - measurement) / PID_MAX_SPEED_CPS;

    float p = pid->kp * error;                         /* 比例 */

    pid->integral += error * dt;                       /* 积分 + anti-windup */
    if (pid->integral >  pid->integral_limit)
        pid->integral =  pid->integral_limit;
    if (pid->integral < -pid->integral_limit)
        pid->integral = -pid->integral_limit;
    float i = pid->ki * pid->integral;

    float d = 0.0f;                                     /* 微分 */
    if (dt > 0.001f)
        d = pid->kd * (error - pid->prev_error) / dt;
    pid->prev_error = error;

    float out = p + i + d;
    if (out > pid->out_max)  out = pid->out_max;
    if (out < pid->out_min)  out = pid->out_min;
    return out;
}

/* PID 实例（左右电机各一个） */
static PID_t pid_left  = {PID_KP, PID_KI, PID_KD, 0, 0, -100, 100, PID_INTEGRAL_LIMIT};
static PID_t pid_right = {PID_KP, PID_KI, PID_KD, 0, 0, -100, 100, PID_INTEGRAL_LIMIT};

/* 速度测量状态 */
static int32_t  prev_enc_pos[2] = {0, 0};
static uint32_t prev_tick       = 0;

/* ═══════════════════════════════════════════════════════════════════
 *  下面的 Init 和 Update 外层框架不要动
 *  只改 Update 中间标记出来的转换逻辑
 * ═══════════════════════════════════════════════════════════════════ */

void Control_Init(void)
{
    PWM_Input_Init();
    PWM_Output_Init();
    Digital_Output_Init();
    SpeedSensor_Init();

    /* 记录初始编码器位置，使首次速度差分为零 */
    prev_enc_pos[MOTOR_LEFT]  = SpeedSensor_GetPosition(MOTOR_LEFT);
    prev_enc_pos[MOTOR_RIGHT] = SpeedSensor_GetPosition(MOTOR_RIGHT);
    prev_tick = HAL_GetTick();      /* 避免首次 dt 过大 */

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
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
    uint8_t out_ch_6 = 0;    /* (PB3) */
    uint8_t out_ch_7 = 0;    /* (PB4) */
    uint8_t out_ch_8 = 0;    /* (PB5) */

    /* ── 坦克差速混控 ────────── */
    /* CH2 油门, CH1 转向, 50=中位 */
    int16_t thr   = (int16_t)in_ch_2 - 50;
    int16_t steer = (int16_t)in_ch_1 - 50;

    /* 油门输入死区：±2 以内的偏差直接归零，防接收机中位偏移被 2x 放大 */
    if (thr > -2 && thr < 2) thr = 0;
    thr   *= 2;   /* -100..+100 */
    steer  = steer * 1;   /* 减小转弯幅度 */

    int16_t left  = thr + steer;
    int16_t right = thr - steer;

    /* 限幅 */
    if (left  >  100) left  =  100;
    if (left  < -100) left  = -100;
    if (right >  100) right =  100;
    if (right < -100) right = -100;

    /* ── 读取编码器位置、计算实际速度 ────────────────────────── */
    uint32_t now     = HAL_GetTick();
    float    dt      = (float)(now - prev_tick) / 1000.0f;   /* 秒 */
    if (dt < 0.001f) dt = 0.001f;                             /* 防除零 */

    int32_t l_pos = SpeedSensor_GetPosition(MOTOR_LEFT);
    int32_t r_pos = SpeedSensor_GetPosition(MOTOR_RIGHT);

    float speed_left  = (float)(l_pos - prev_enc_pos[MOTOR_LEFT])  / dt;
    float speed_right = (float)(r_pos - prev_enc_pos[MOTOR_RIGHT]) / dt;

    /* 左编码器物理方向与电机正转方向相反，取反后 PID 才能正确闭环 */
    speed_left = -speed_left;

    prev_enc_pos[MOTOR_LEFT]  = l_pos;
    prev_enc_pos[MOTOR_RIGHT] = r_pos;
    prev_tick = now;

    /* ── 混控输出 → 目标速度 ────────────────────────────────── */
    float target_left  = (float)left  * PID_MAX_SPEED_CPS / 100.0f;
    float target_right = (float)right * PID_MAX_SPEED_CPS / 100.0f;

    /* ── PID 计算（输出 -100..+100） ─────────────────────────── */
    float pid_out_left  = PID_Compute(&pid_left,  target_left,  speed_left,  dt);
    float pid_out_right = PID_Compute(&pid_right, target_right, speed_right, dt);

    /* ── PID 输出 → PWM 通道 ─────────────────────────────────── */
    /* 左电机 (CH1=反转, CH2=正转) */
    if (pid_out_left > 3) {
        out_ch_1 = 0;
        out_ch_2 = (uint8_t)(pid_out_left + 0.5f);          /* 正转 */
    } else if (pid_out_left < -3) {
        out_ch_1 = (uint8_t)(-pid_out_left + 0.5f);         /* 反转 */
        out_ch_2 = 0;
    } else {
        out_ch_1 = 0;
        out_ch_2 = 0;
    }

    /* 右电机 (CH3=反转, CH4=正转) */
    if (pid_out_right > 3) {
        out_ch_3 = 0;
        out_ch_4 = (uint8_t)(pid_out_right + 0.5f);         /* 正转 */
    } else if (pid_out_right < -3) {
        out_ch_3 = (uint8_t)(-pid_out_right + 0.5f);        /* 反转 */
        out_ch_4 = 0;
    } else {
        out_ch_3 = 0;
        out_ch_4 = 0;
    }

    const int switchCenter = 75;
    out_ch_5  = (in_ch_5 > switchCenter) ? 1 : 0;
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
    const uint8_t in_arr[8]   = {in_ch_1, in_ch_2, in_ch_3, in_ch_4,
                                 in_ch_5, in_ch_6, in_ch_7, in_ch_8};
    const uint8_t out_pct[4]  = {out_ch_1, out_ch_2, out_ch_3, out_ch_4};
    const uint8_t out_dig[8]  = {0, 0, 0, 0,
                                 out_ch_5, out_ch_6, out_ch_7, out_ch_8};

    Display_Update(in_arr, out_pct, out_dig, l_pos, r_pos);
}
