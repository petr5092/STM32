#include "main.h"
#include "tft.h"

/* Global debug variables observable in VS Code debugger */
volatile uint32_t loop_counter = 0;
volatile const char banner[] = "EasyMx PRO v7 - EasyTFT Display Running!";

static void delay_init(void) {
    /* Initialize SysTick for 1ms polling delays */
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    }
}

static void leds_init(void) {
    /*
     * Enable clocks for GPIO Ports C and D (LEDs).
     * Note: GPIOE is fully dedicated to the EasyTFT display bus!
     */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN;

    /* Configure Pins 0..7 of GPIOC and GPIOD as Output Push-Pull 50MHz */
    GPIOC->CRL = 0x33333333;
    GPIOD->CRL = 0x33333333;

    /* Turn off LEDs initially */
    GPIOC->BSRR = (0xFF << 16);
    GPIOD->BSRR = (0xFF << 16);
}

static void set_leds_pattern(uint8_t pattern) {
    uint32_t set_mask = pattern;
    uint32_t rst_mask = ((uint32_t)(~pattern & 0xFF)) << 16;
    uint32_t val = set_mask | rst_mask;

    GPIOC->BSRR = val;
    GPIOD->BSRR = val;
}

/* Simple integer-to-string formatter */
static void int_to_str(uint32_t num, char *out) {
    char buf[12];
    int i = 0;
    if (num == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    int j = 0;
    while (i > 0) {
        out[j++] = buf[--i];
    }
    out[j] = '\0';
}

static void draw_color_palette(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    const uint16_t colors[] = {
        TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN,
        TFT_CYAN, TFT_BLUE, TFT_MAGENTA, TFT_WHITE
    };
    uint16_t bar_w = w / 8;
    for (int i = 0; i < 8; i++) {
        TFT_FillRect(x + i * bar_w, y, bar_w, h, colors[i]);
        TFT_DrawRect(x + i * bar_w, y, bar_w, h, TFT_DARKGREY);
    }
}

int main(void) {
    SystemCoreClockUpdate();
    delay_init();
    leds_init();

    /* Initialize EasyTFT display (auto-detects ILI9341 or HX8347D) */
    TFT_Init();

    /* 1. Background fill: Dark Navy */
    TFT_FillScreen(0x0845);

    /* 2. Top Header Bar */
    TFT_FillRect(0, 0, TFT_WIDTH, 28, 0x01D3);
    TFT_DrawRect(0, 0, TFT_WIDTH, 29, TFT_CYAN);
    TFT_DrawString(12, 6, "EasyMx PRO v7 * STM32F107", TFT_WHITE, 0x01D3, 1);

    /* 3. Main Card Frame */
    TFT_DrawRect(10, 36, 300, 150, TFT_CYAN);
    TFT_DrawRect(12, 38, 296, 146, 0x0276);
    TFT_FillRect(14, 40, 292, 142, 0x00A8); // Soft dark slate box

    /* 4. Words on Screen */
    /* Big Greeting: Russian */
    TFT_DrawString(60, 52, "ПРИВЕТ, МИР!", TFT_GOLD, 0x00A8, 2);

    /* Big Greeting: English */
    TFT_DrawString(64, 92, "HELLO STM32!", TFT_GREENYELLOW, 0x00A8, 2);

    /* Subtitle */
    TFT_DrawString(44, 134, "EasyTFT 262K Colors (320x240)", TFT_WHITE, 0x00A8, 1);

    /* Controller Info */
    if (tft_controller_id == 0x8347) {
        TFT_DrawString(76, 154, "Driver: Himax HX8347D", TFT_CYAN, 0x00A8, 1);
    } else {
        TFT_DrawString(84, 154, "Driver: Ilitek ILI9341", TFT_CYAN, 0x00A8, 1);
    }

    /* 5. Color Palette preview */
    draw_color_palette(20, 196, 280, 14);

    /* 6. Bottom Status Line */
    TFT_DrawString(14, 218, "Status: ACTIVE", TFT_GREEN, 0x0845, 1);

    char num_buf[12];
    uint8_t led_bit = 0;

    /* Main Loop: Running LED lights + Dynamic Screen Update */
    while (1) {
        loop_counter++;

        /* Update dynamic counter on screen */
        int_to_str(loop_counter, num_buf);
        TFT_FillRect(220, 218, 90, 16, 0x0845);
        TFT_DrawString(160, 218, "Ticks:", TFT_LIGHTGREY, 0x0845, 1);
        TFT_DrawString(220, 218, num_buf, TFT_YELLOW, 0x0845, 1);

        /* Running light on PORTD & PORTC LEDs */
        set_leds_pattern(1 << led_bit);
        led_bit = (led_bit + 1) % 8;

        delay_ms(150);
    }
}
