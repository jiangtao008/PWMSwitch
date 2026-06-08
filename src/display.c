/**
 * @file    display.c
 * @brief   128×64 OLED UI — 8-ch input bars + 8-ch output indicators.
 */

#include "display.h"
#include "ssd1306.h"
#include <string.h>

/* ── Layout constants ──────────────────────────────────────────────── */
#define ROW_H       8
#define BAR_X_L     8       /* left-panel bar start x */
#define BAR_W       56      /* bar width */
#define BAR_INNER   (BAR_W - 2)   /* inner fillable width (54px) */

#define LEFT_X      0
#define RIGHT_X     64
#define NUM_CH      8

/* ── Draw one bar row ──────────────────────────────────────────────── */

/**
 * @brief  Draw a single channel bar.
 * @param  x0    Left edge of the bar area
 * @param  y0    Top of the 8px row
 * @param  pct   0~100 fill percentage
 */
static void draw_bar(uint8_t x0, uint8_t y0, uint8_t pct)
{
    if (pct > 100) pct = 100;

    /* Draw empty frame */
    SSD1306_DrawRect(x0, y0, BAR_W, ROW_H, 1);

    if (pct > 0) {
        /* Fill inner area from left */
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
                    const uint8_t out_dig[4])
{
    SSD1306_Clear();

    /* ── Middle divider ──────────────────────────────────────── */
    SSD1306_VLine(63, 0, 64, 1);

    /* ── Left panel: input channels 1~8 ──────────────────────── */
    for (uint8_t i = 0; i < NUM_CH; i++) {
        uint8_t y0 = i * ROW_H;

        /* Channel number (1..8) */
        SSD1306_DrawChar(LEFT_X, y0, '1' + i);

        /* Bar graph */
        draw_bar(LEFT_X + BAR_X_L, y0, in_pct[i]);
    }

    /* ── Right panel: output channels ────────────────────────── */
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t y0 = i * ROW_H;

        /* Label '1'~'4' (PWM outputs) */
        SSD1306_DrawChar(RIGHT_X, y0, '1' + i);

        /* Bar graph */
        draw_bar(RIGHT_X + BAR_X_L, y0, out_pct[i]);
    }

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t y0 = (i + 4) * ROW_H;

        /* Label '5'~'8' (digital outputs) */
        SSD1306_DrawChar(RIGHT_X, y0, '5' + i);

        /* ON / OFF text */
        if (out_dig[i]) {
            SSD1306_DrawString(RIGHT_X + BAR_X_L, y0, "ON");
        } else {
            SSD1306_DrawString(RIGHT_X + BAR_X_L, y0, "--");
        }
    }

    SSD1306_Flush();
}
