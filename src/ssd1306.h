/**
 * @file    ssd1306.h
 * @brief   SSD1306 128×64 OLED driver (I2C)
 *
 * I2C2: PB10=SCL, PB11=SDA, 400 kHz, address 0x3C
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)

/** Init I2C2 and send SSD1306 init sequence. */
void SSD1306_Init(void);

/** Clear framebuffer. */
void SSD1306_Clear(void);

/** Flush framebuffer to display. */
void SSD1306_Flush(void);

/** Draw a filled rectangle.  color: 0=black, 1=white. */
void SSD1306_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/** Draw a 1-pixel outline rectangle. */
void SSD1306_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/** Draw a horizontal line (fast). */
void SSD1306_HLine(uint8_t x, uint8_t y, uint8_t w, uint8_t color);

/** Draw a vertical line. */
void SSD1306_VLine(uint8_t x, uint8_t y, uint8_t h, uint8_t color);

/** Draw an 8×8 character.  Uses built-in 8×8 font (ASCII 32~127). */
void SSD1306_DrawChar(uint8_t x, uint8_t y, char c);

/** Draw a null-terminated string (8×8 font). */
void SSD1306_DrawString(uint8_t x, uint8_t y, const char *s);

/** Draw a 2-digit decimal number (00~99), 8×8 font. */
void SSD1306_DrawNumber(uint8_t x, uint8_t y, uint8_t num);

#ifdef __cplusplus
}
#endif

#endif /* SSD1306_H */
