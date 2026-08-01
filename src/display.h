/**
 * @file    display.h
 * @brief   Application UI — 128×64 OLED layout renderer.
 *
 * ┌──────────────────┬──────────────────┐
 * │  Input (64×64)   │ Output (64×64)   │
 * │  CH1 [===   ]    │  1   [====  ]    │
 * │  CH2 [======]    │  2   [==    ]    │
 * │  CH3 [=     ]    │  3   [======]    │
 * │  CH4 [  ==  ]    │  4   [=     ]    │
 * ├──────────────────┴──────────────────┤
 * │  ██████████░░░░░░│░░░░░░██████████  │  ← speed bar (y=56..63)
 * │  ◄── left ──────►│◄──── right ───►  │
 * │  forward → center│center ← forward  │
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
 * @brief  Render one full frame (bars + speed progress bar, single flush).
 * @param  in_pct      Input  channels 1~8, 0~100 (%)
 * @param  out_pct     Output channels 1~4, 0~100 (%)
 * @param  out_dig     Output channels 5~12, 0 or 1
 * @param  left_speed  Left  motor commanded speed, -100..+100
 * @param  right_speed Right motor commanded speed, -100..+100
 */
void Display_Update(const uint8_t in_pct[8],
                    const uint8_t out_pct[4],
                    const uint8_t out_dig[8],
                    int16_t left_speed, int16_t right_speed);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
