#include "tft.h"
#include "font8x16.h"
#include <stddef.h>

/* Global variable with detected controller ID (0x9341 or 0x8347) */
volatile uint16_t tft_controller_id = 0;

/* Control pin manipulation macros */
#define TFT_CS_LOW()    (GPIOE->BRR = (1 << 15))
#define TFT_CS_HIGH()   (GPIOE->BSRR = (1 << 15))

#define TFT_RS_LOW()    (GPIOE->BRR = (1 << 12))
#define TFT_RS_HIGH()   (GPIOE->BSRR = (1 << 12))

#define TFT_WR_LOW()    (GPIOE->BRR = (1 << 11))
#define TFT_WR_HIGH()   (GPIOE->BSRR = (1 << 11))

#define TFT_RD_LOW()    (GPIOE->BRR = (1 << 10))
#define TFT_RD_HIGH()   (GPIOE->BSRR = (1 << 10))

#define TFT_RST_LOW()   (GPIOE->BRR = (1 << 8))
#define TFT_RST_HIGH()  (GPIOE->BSRR = (1 << 8))

#define TFT_BLED_ON()   (GPIOE->BSRR = (1 << 9))
#define TFT_BLED_OFF()  (GPIOE->BRR = (1 << 9))

/* Fast bus byte write */
static inline void TFT_Write_Byte_Fast(uint8_t byte) {
    GPIOE->ODR = (GPIOE->ODR & ~0x00FF) | byte;
    TFT_WR_LOW();
    __NOP();
    TFT_WR_HIGH();
}

/* Command write (RS=0) */
static void TFT_Write_Command(uint8_t cmd) {
    TFT_CS_LOW();
    TFT_RS_LOW();
    TFT_Write_Byte_Fast(cmd);
    TFT_CS_HIGH();
}

/* Data write (RS=1) */
static void TFT_Write_Data(uint8_t data) {
    TFT_CS_LOW();
    TFT_RS_HIGH();
    TFT_Write_Byte_Fast(data);
    TFT_CS_HIGH();
}

/* Set bus pins PE0..PE7 to input (for reading ID) */
static inline void TFT_Bus_Input(void) {
    GPIOE->CRL = 0x88888888; // Input with pull-up/down
    GPIOE->ODR |= 0x00FF;    // Pull-up
}

/* Set bus pins PE0..PE7 to output (Push-Pull 50MHz) */
static inline void TFT_Bus_Output(void) {
    GPIOE->CRL = 0x33333333;
}

/* Read byte from bus */
static inline uint8_t TFT_Read_Byte(void) {
    uint8_t data;
    TFT_RD_LOW();
    for (volatile int i = 0; i < 25; i++) __NOP(); // Pulse width >= 355ns
    data = (uint8_t)(GPIOE->IDR & 0xFF);
    TFT_RD_HIGH();
    for (volatile int i = 0; i < 15; i++) __NOP();
    return data;
}

/* Read controller ID to detect ILI9341 vs HX8347D */
static uint16_t TFT_ReadID(void) {
    /* 1. Try ILI9341 Read ID4 (0xD3) */
    TFT_CS_LOW();
    TFT_RS_LOW();
    TFT_Write_Byte_Fast(0xD3);
    TFT_RS_HIGH();
    TFT_Bus_Input();

    (void)TFT_Read_Byte();        // Dummy byte
    (void)TFT_Read_Byte();        // ID1 (0x00)
    uint8_t id2 = TFT_Read_Byte(); // Version (0x93)
    uint8_t id3 = TFT_Read_Byte(); // Device ID (0x41)

    TFT_Bus_Output();
    TFT_CS_HIGH();

    if (id2 == 0x93 && id3 == 0x41) {
        return 0x9341;
    }

    /* 2. Try HX8347D register 0x00 */
    TFT_CS_LOW();
    TFT_RS_LOW();
    TFT_Write_Byte_Fast(0x00);
    TFT_RS_HIGH();
    TFT_Bus_Input();
    uint8_t hx0 = TFT_Read_Byte();
    TFT_Bus_Output();
    TFT_CS_HIGH();

    if (hx0 == 0x47 || hx0 == 0x46 || hx0 == 0x45) {
        return 0x8347;
    }

    /* 3. Try HX8347D register 0x67 */
    TFT_CS_LOW();
    TFT_RS_LOW();
    TFT_Write_Byte_Fast(0x67);
    TFT_RS_HIGH();
    TFT_Bus_Input();
    uint8_t hx1 = TFT_Read_Byte();
    TFT_Bus_Output();
    TFT_CS_HIGH();

    if (hx1 == 0x47) {
        return 0x8347;
    }

    /* Default to 0x9341 (most common EasyTFT revision) */
    return 0x9341;
}

/* ----------------- ILI9341 Driver ----------------- */

static void ILI9341_Init(void) {
    TFT_Write_Command(0x01); // Software reset
    delay_ms(15);

    TFT_Write_Command(0x28); // Display OFF

    TFT_Write_Command(0xCB); // Power control A
    TFT_Write_Data(0x39);
    TFT_Write_Data(0x2C);
    TFT_Write_Data(0x00);
    TFT_Write_Data(0x34);
    TFT_Write_Data(0x02);

    TFT_Write_Command(0xCF); // Power control B
    TFT_Write_Data(0x00);
    TFT_Write_Data(0xC1);
    TFT_Write_Data(0x30);

    TFT_Write_Command(0xE8); // Driver timing control A
    TFT_Write_Data(0x85);
    TFT_Write_Data(0x00);
    TFT_Write_Data(0x78);

    TFT_Write_Command(0xEA); // Driver timing control B
    TFT_Write_Data(0x00);
    TFT_Write_Data(0x00);

    TFT_Write_Command(0xED); // Power on sequence control
    TFT_Write_Data(0x64);
    TFT_Write_Data(0x03);
    TFT_Write_Data(0x12);
    TFT_Write_Data(0x81);

    TFT_Write_Command(0xF7); // Pump ratio control
    TFT_Write_Data(0x20);

    TFT_Write_Command(0xC0); // Power control 1
    TFT_Write_Data(0x23);

    TFT_Write_Command(0xC1); // Power control 2
    TFT_Write_Data(0x10);

    TFT_Write_Command(0xC5); // VCOM control 1
    TFT_Write_Data(0x3E);
    TFT_Write_Data(0x28);

    TFT_Write_Command(0xC7); // VCOM control 2
    TFT_Write_Data(0x86);

    /* MADCTL: Landscape 320x240 (MV=1, BGR=1) */
    TFT_Write_Command(0x36);
    TFT_Write_Data(0x28);

    /* COLMOD: 16-bit RGB565 */
    TFT_Write_Command(0x3A);
    TFT_Write_Data(0x55);

    TFT_Write_Command(0xB1); // Frame rate control (70Hz)
    TFT_Write_Data(0x00);
    TFT_Write_Data(0x18);

    TFT_Write_Command(0xB6); // Display function control
    TFT_Write_Data(0x08);
    TFT_Write_Data(0x82);
    TFT_Write_Data(0x27);

    TFT_Write_Command(0xF2); // 3Gamma Disable
    TFT_Write_Data(0x00);

    TFT_Write_Command(0x26); // Gamma curve selected
    TFT_Write_Data(0x01);

    /* Positive gamma correction */
    TFT_Write_Command(0xE0);
    const uint8_t pos_gamma[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                                 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    for (int i = 0; i < 15; i++) TFT_Write_Data(pos_gamma[i]);

    /* Negative gamma correction */
    TFT_Write_Command(0xE1);
    const uint8_t neg_gamma[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                                 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    for (int i = 0; i < 15; i++) TFT_Write_Data(neg_gamma[i]);

    TFT_Write_Command(0x11); // Sleep OUT
    delay_ms(120);

    TFT_Write_Command(0x29); // Display ON
    delay_ms(20);
}

/* ----------------- HX8347D Driver ----------------- */

static void HX8347D_Write_Reg(uint8_t reg, uint8_t val) {
    TFT_Write_Command(reg);
    TFT_Write_Data(val);
}

static void HX8347D_Init(void) {
    HX8347D_Write_Reg(0xEA, 0x00);
    HX8347D_Write_Reg(0xEB, 0x20);
    HX8347D_Write_Reg(0xEC, 0x0C);
    HX8347D_Write_Reg(0xED, 0xC4);
    HX8347D_Write_Reg(0xE8, 0x38);
    HX8347D_Write_Reg(0xE9, 0x38);
    HX8347D_Write_Reg(0xF1, 0x01);
    HX8347D_Write_Reg(0xF2, 0x10);

    HX8347D_Write_Reg(0x27, 0x1A);
    HX8347D_Write_Reg(0x40, 0x00);
    HX8347D_Write_Reg(0x41, 0x00);
    HX8347D_Write_Reg(0x42, 0x00);
    HX8347D_Write_Reg(0x43, 0x10);
    HX8347D_Write_Reg(0x44, 0x0E);

    /* Gamma Control */
    HX8347D_Write_Reg(0x30, 0x00);
    HX8347D_Write_Reg(0x31, 0x07);
    HX8347D_Write_Reg(0x32, 0x14);
    HX8347D_Write_Reg(0x33, 0x21);
    HX8347D_Write_Reg(0x34, 0x07);
    HX8347D_Write_Reg(0x35, 0x14);
    HX8347D_Write_Reg(0x36, 0x21);
    HX8347D_Write_Reg(0x37, 0x07);
    HX8347D_Write_Reg(0x38, 0x14);
    HX8347D_Write_Reg(0x39, 0x21);
    HX8347D_Write_Reg(0x3A, 0x0A);
    HX8347D_Write_Reg(0x3B, 0x14);
    HX8347D_Write_Reg(0x3C, 0x07);
    HX8347D_Write_Reg(0x3D, 0x10);
    HX8347D_Write_Reg(0x3E, 0x07);
    HX8347D_Write_Reg(0x3F, 0x00);

    /* Power supply settings */
    HX8347D_Write_Reg(0x1B, 0x1B);
    HX8347D_Write_Reg(0x1A, 0x01);
    HX8347D_Write_Reg(0x24, 0x2F);
    HX8347D_Write_Reg(0x25, 0x57);
    HX8347D_Write_Reg(0x23, 0x88);
    delay_ms(5);
    HX8347D_Write_Reg(0x18, 0x36);
    HX8347D_Write_Reg(0x19, 0x01);
    HX8347D_Write_Reg(0x01, 0x00); // Sleep OUT
    HX8347D_Write_Reg(0x1F, 0x88);
    delay_ms(5);
    HX8347D_Write_Reg(0x1F, 0x80);
    delay_ms(5);
    HX8347D_Write_Reg(0x1F, 0x90);
    delay_ms(5);
    HX8347D_Write_Reg(0x1F, 0xD0);
    delay_ms(5);

    /* Landscape 320x240 */
    HX8347D_Write_Reg(0x16, 0x68);
    HX8347D_Write_Reg(0x17, 0x05); // 16-bit RGB565

    HX8347D_Write_Reg(0x28, 0x38);
    delay_ms(40);
    HX8347D_Write_Reg(0x28, 0x3C);
}

/* ----------------- High-Level Interface ----------------- */

void TFT_Init(void) {
    /* 1. Enable GPIOE clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPEEN;

    /* 2. Configure PE0..PE7 as Output Push-Pull 50MHz */
    GPIOE->CRL = 0x33333333;

    /* 3. Configure PE8..PE15 as Output Push-Pull 50MHz */
    GPIOE->CRH = 0x33333333;

    /* 4. Set idle state for control lines */
    TFT_CS_HIGH();
    TFT_RS_HIGH();
    TFT_RD_HIGH();
    TFT_WR_HIGH();
    TFT_RST_HIGH();
    TFT_BLED_ON();

    /* 5. Hardware reset cycle */
    delay_ms(20);
    TFT_RST_LOW();
    delay_ms(30);
    TFT_RST_HIGH();
    delay_ms(150);

    /* 6. Detect controller ID */
    tft_controller_id = TFT_ReadID();

    /* 7. Initialize according to detected controller */
    if (tft_controller_id == 0x8347) {
        HX8347D_Init();
    } else {
        ILI9341_Init();
    }
}

void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    if (tft_controller_id == 0x8347) {
        HX8347D_Write_Reg(0x02, x0 >> 8);
        HX8347D_Write_Reg(0x03, x0 & 0xFF);
        HX8347D_Write_Reg(0x04, x1 >> 8);
        HX8347D_Write_Reg(0x05, x1 & 0xFF);
        HX8347D_Write_Reg(0x06, y0 >> 8);
        HX8347D_Write_Reg(0x07, y0 & 0xFF);
        HX8347D_Write_Reg(0x08, y1 >> 8);
        HX8347D_Write_Reg(0x09, y1 & 0xFF);
        TFT_Write_Command(0x22); // Memory write command
    } else {
        /* ILI9341 */
        TFT_Write_Command(0x2A); // Column Address Set
        TFT_Write_Data(x0 >> 8);
        TFT_Write_Data(x0 & 0xFF);
        TFT_Write_Data(x1 >> 8);
        TFT_Write_Data(x1 & 0xFF);

        TFT_Write_Command(0x2B); // Page Address Set
        TFT_Write_Data(y0 >> 8);
        TFT_Write_Data(y0 & 0xFF);
        TFT_Write_Data(y1 >> 8);
        TFT_Write_Data(y1 & 0xFF);

        TFT_Write_Command(0x2C); // Memory Write
    }
}

void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH) w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    TFT_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t total = (uint32_t)w * h;

    TFT_CS_LOW();
    TFT_RS_HIGH();
    while (total--) {
        TFT_Write_Byte_Fast(hi);
        TFT_Write_Byte_Fast(lo);
    }
    TFT_CS_HIGH();
}

void TFT_FillScreen(uint16_t color) {
    TFT_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    TFT_FillRect(x, y, w, 1, color);             // Top
    TFT_FillRect(x, y + h - 1, w, 1, color);     // Bottom
    TFT_FillRect(x, y, 1, h, color);             // Left
    TFT_FillRect(x + w - 1, y, 1, h, color);     // Right
}

void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    TFT_SetAddressWindow(x, y, x, y);
    TFT_CS_LOW();
    TFT_RS_HIGH();
    TFT_Write_Byte_Fast(color >> 8);
    TFT_Write_Byte_Fast(color & 0xFF);
    TFT_CS_HIGH();
}

void TFT_DrawChar(uint16_t x, uint16_t y, const uint8_t *glyph, uint16_t color, uint16_t bg, uint8_t scale) {
    if (x + 8 * scale > TFT_WIDTH || y + 16 * scale > TFT_HEIGHT) return;

    TFT_SetAddressWindow(x, y, x + 8 * scale - 1, y + 16 * scale - 1);

    uint8_t hi_col = color >> 8;
    uint8_t lo_col = color & 0xFF;
    uint8_t hi_bg  = bg >> 8;
    uint8_t lo_bg  = bg & 0xFF;

    TFT_CS_LOW();
    TFT_RS_HIGH();

    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int sy = 0; sy < scale; sy++) {
            for (int col = 0; col < 8; col++) {
                uint8_t is_pixel = (bits & (0x80 >> col)) != 0;
                uint8_t hi = is_pixel ? hi_col : hi_bg;
                uint8_t lo = is_pixel ? lo_col : lo_bg;
                for (int sx = 0; sx < scale; sx++) {
                    TFT_Write_Byte_Fast(hi);
                    TFT_Write_Byte_Fast(lo);
                }
            }
        }
    }

    TFT_CS_HIGH();
}

void TFT_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale) {
    uint16_t cur_x = x;
    uint16_t cur_y = y;

    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            cur_y += 16 * scale;
            str++;
            continue;
        }

        const uint8_t *glyph = NULL;
        uint8_t ch = (uint8_t)*str;

        if (ch >= 32 && ch <= 126) {
            glyph = &font_ascii[ch - 32][0];
            str++;
        } else if (ch == 0xD0 || ch == 0xD1) {
            uint8_t b2 = (uint8_t)*(str + 1);
            if (b2 != 0) {
                glyph = get_cyrillic_glyph(ch, b2);
                str += 2;
            } else {
                str++;
            }
        } else {
            glyph = &font_ascii[0][0];
            str++;
        }

        if (glyph) {
            if (cur_x + 8 * scale <= TFT_WIDTH) {
                TFT_DrawChar(cur_x, cur_y, glyph, color, bg, scale);
            }
            cur_x += 8 * scale;
        }
    }
}
