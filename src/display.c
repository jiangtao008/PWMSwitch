/**
 * @file    display.c
 * @brief   128x64 OLED UI — 输入 CH1~CH4 + 输出 CH1~CH4 + 编码器速度
 */

#include "display.h"
#include "ssd1306.h"
#include <string.h>
#include <stdio.h>

/* ── Layout constants ──────────────────────────────────────────────── */
#define ROW_H       8
#define BAR_X_L     8       /* bar start x (relative to panel) */
#define BAR_W       56      /* bar width */
#define BAR_INNER   (BAR_W - 2)

#define LEFT_X      0
#define RIGHT_X     64
#define IN_CH       4       /* 输入只显示 CH1~CH4 */
#define OUT_CH      4       /* 输出只显示 PWM CH1~CH4 */

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

/* ── Public ────────────────────────────────────────────────────────── */

void Display_Init(void)
{
    SSD1306_Init();
}

void Display_Update(const uint8_t in_pct[8],
                    const uint8_t out_pct[4],
                    const uint8_t out_dig[8])
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

    SSD1306_Flush();
}

/* ── Speed overlay (rows 6~7 of right panel) ───────────────────────── */

void Display_ShowSpeed(int32_t left_pos, int32_t right_pos)
{
    char buf[13];

    snprintf(buf, sizeof(buf), "L:%+6ld", left_pos);
    SSD1306_DrawString(64, 48, buf);

    snprintf(buf, sizeof(buf), "R:%+6ld", right_pos);
    SSD1306_DrawString(64, 56, buf);

    SSD1306_Flush();
}
