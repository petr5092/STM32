#ifndef TFT_H
#define TFT_H

#include "main.h"

/* Standard 16-bit RGB565 color definitions */
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xC618
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFD20
#define TFT_GREENYELLOW 0xAFE5
#define TFT_GOLD        0xFEA0

#define TFT_WIDTH       320
#define TFT_HEIGHT      240

/* Global detected controller ID (0x9341 or 0x8347) */
extern volatile uint16_t tft_controller_id;

/* Initialization and windowing */
void TFT_Init(void);
void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/* Graphic primitives */
void TFT_FillScreen(uint16_t color);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFT_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/* Text rendering (Supports ASCII and UTF-8 Cyrillic) */
void TFT_DrawChar(uint16_t x, uint16_t y, const uint8_t *glyph, uint16_t color, uint16_t bg, uint8_t scale);
void TFT_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);

#endif /* TFT_H */
