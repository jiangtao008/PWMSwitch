/**
 * @file    display.c
 * @brief   128x64 OLED UI — 输入 CH1~CH4 + 输出 CH1~CH4 + 编码器速度
 */

#include "display.h"
#include "ssd1306.h"

/* ── Layout constants ──────────────────────────────────────────────── */
#define ROW_H       8
#define BAR_X_L     8       /* bar start x (relative to panel) */
#define BAR_W       56      /* bar width */
#define BAR_INNER   (BAR_W - 2)

#define LEFT_X      0
#define RIGHT_X     64
#define IN_CH       4       /* 输入只显示 CH1~CH4 */
#define OUT_CH      4       /* 输出只显示 PWM CH1~CH4 */

#define SPEED_BAR_Y 56      /* speed bar y (bottom 8 rows) */
#define SPEED_BAR_H 8
#define SPEED_MAX   64      /* half screen = max bar width in pixels */
#define SPEED_SCALE 100     /* speed value range (0..100%) */

/* ── Draw one bar row ──────────────────────────────────────────────── */

static void draw_bar(uint8_t x0, uint8_t y0, uint8_t pct)
{
    if (pct > 100) pct = 100;

    SSD1306_DrawRect(x0, y0, BAR_W, ROW_H, 1);

    if (pct > 0) {
        uint8_t fill_w = (uint16_t)pct * BAR_INNER / 100U;
        if (fill_w > BAR_INNER) fill_w = BAR_INNER;
        if (fill_w > 0) {
            SSD1306_FillRect(x0 + 1, y0 + 1, fill_w, ROW_H - 2, 1);
        }
    }
}

/* ── Draw speed progress bar (one side) ────────────────────────────── */
/**
 * @brief  Draw a horizontal speed bar that fills toward the center.
 * @param  speed    Signed speed, -100..+100.
 * @param  is_right 0 = left half (x=0..63), 1 = right half (x=64..127).
 *
 * Left  half: forward (+) fills from left edge  rightward → center.
 *             reverse (-) fills from center      leftward  → edge.
 * Right half: forward (+) fills from right edge  leftward  → center.
 *             reverse (-) fills from center      rightward → edge.
 */
static void draw_speed_bar(int16_t speed, uint8_t is_right)
{
    uint8_t abs_w;
    uint8_t x;
    uint8_t y = SPEED_BAR_Y;

    /* Clamp */
    if (speed > SPEED_SCALE)  speed = SPEED_SCALE;
    if (speed < -SPEED_SCALE) speed = -SPEED_SCALE;

    /* Compute bar width from absolute speed */
    if (speed > 0) {
        abs_w = (uint16_t)speed * SPEED_MAX / (uint16_t)SPEED_SCALE;
    } else if (speed < 0) {
        abs_w = (uint16_t)(-speed) * SPEED_MAX / (uint16_t)SPEED_SCALE;
    } else {
        return;  /* zero speed → draw nothing */
    }
    if (abs_w > SPEED_MAX) abs_w = SPEED_MAX;
    if (abs_w == 0) return;

    if (is_right) {
        /* Right half: x ∈ [64, 127] */
        if (speed > 0) {
            /* Forward: bar grows from right edge (127) leftward toward center */
            x = 127 - abs_w + 1;
        } else {
            /* Reverse: bar grows from center (64) rightward toward edge */
            x = 64;
        }
    } else {
        /* Left half: x ∈ [0, 63] */
        if (speed > 0) {
            /* Forward: bar grows from left edge (0) rightward toward center */
            x = 0;
        } else {
            /* Reverse: bar grows from center (63) leftward toward edge */
            x = 63 - abs_w + 1;
        }
    }

    SSD1306_FillRect(x, y, abs_w, SPEED_BAR_H, 1);
}

/* ── Public ────────────────────────────────────────────────────────── */

void Display_Init(void)
{
    SSD1306_Init();
}

void Display_Update(const uint8_t in_pct[8],
                    const uint8_t out_pct[4],
                    const uint8_t out_dig[8],
                    int16_t left_speed, int16_t right_speed)
{
    (void)out_dig;      /* 不用数字输出显示 */

    SSD1306_Clear();

    /* ── Middle divider ──────────────────────────────────────── */
    SSD1306_VLine(63, 0, 64, 1);

    /* ── Left panel: input channels 1~4 ──────────────────────── */
    for (uint8_t i = 0; i < IN_CH; i++) {
        uint8_t y0 = i * ROW_H;
        SSD1306_DrawChar(LEFT_X, y0, '1' + i);
        draw_bar(LEFT_X + BAR_X_L, y0, in_pct[i]);
    }

    /* ── Right panel: PWM output channels 1~4 ────────────────── */
    for (uint8_t i = 0; i < OUT_CH; i++) {
        uint8_t y0 = i * ROW_H;
        SSD1306_DrawChar(RIGHT_X, y0, '1' + i);
        draw_bar(RIGHT_X + BAR_X_L, y0, out_pct[i]);
    }

    /* ── Speed progress bar (bottom 8 rows, full width) ──────── */
    SSD1306_HLine(0, SPEED_BAR_Y - 1, SSD1306_WIDTH, 1);  /* top border */
    draw_speed_bar(left_speed, 0);    /* left  half: x=0..63 */
    draw_speed_bar(right_speed, 1);   /* right half: x=64..127 */

    /* ── Single flush — no flicker ──────────────────────────── */
    SSD1306_Flush();
}
