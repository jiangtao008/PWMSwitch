/**
 * @file    display.h
 * @brief   Application UI — 128×64 OLED layout renderer.
 *
 * ┌──────────────────┬──────────────────┐
 * │  Input (64×64)   │ Output (64×64)   │
 * │  CH1 [===   ]    │  O1  [====  ]    │
 * │  CH2 [======]    │  O2  [==    ]    │
 * │  ...             │  ...             │
 * │  CH8 [==    ]    │  O8  ON          │
 * └──────────────────┴──────────────────┘
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Init SSD1306 and clear screen. */
void Display_Init(void);

/**
 * @brief  Render one full frame.
 * @param  in_pct   Input  channels 1~8, 0~100 (%)
 * @param  out_pct  Output channels 1~4, 0~100 (%)
 * @param  out_dig  Output channels 5~12, 0 or 1
 */
void Display_Update(const uint8_t in_pct[8],
                    const uint8_t out_pct[4],
                    const uint8_t out_dig[8]);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
