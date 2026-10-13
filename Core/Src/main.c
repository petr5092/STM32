#include "main.h"

/* Global debug variables observable in VS Code debugger */
volatile uint32_t loop_counter = 0;
volatile const char banner[] = "EasyMx PRO v7 - STM32F107 Running!";

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

static void gpio_init(void) {
    /* Enable clocks for GPIO Ports A, B, C, D, E */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | 
                    RCC_APB2ENR_IOPBEN | 
                    RCC_APB2ENR_IOPCEN | 
                    RCC_APB2ENR_IOPDEN | 
                    RCC_APB2ENR_IOPEEN;

    /*
     * Configure Pins 0..7 as General Purpose Output Push-Pull (50MHz max):
     * MODE = 11 (50MHz), CNF = 00 (Push-Pull) -> 0x3 per pin
     * Note: We only configure CRL (pins 0..7), preserving PA13/PA14 (SWD pins) in CRH!
     */
    GPIOA->CRL = 0x33333333;
    GPIOB->CRL = 0x33333333;
    GPIOC->CRL = 0x33333333;
    GPIOD->CRL = 0x33333333;
    GPIOE->CRL = 0x33333333;

    /* Turn off all LEDs initially (low on pins 0..7) */
    GPIOA->BSRR = (0xFF << 16);
    GPIOB->BSRR = (0xFF << 16);
    GPIOC->BSRR = (0xFF << 16);
    GPIOD->BSRR = (0xFF << 16);
    GPIOE->BSRR = (0xFF << 16);
}

static void set_all_ports_leds(uint8_t pattern) {
    /* Write 8-bit pattern to pins 0..7 on all ports */
    uint32_t set_mask = pattern;
    uint32_t rst_mask = ((uint32_t)(~pattern & 0xFF)) << 16;
    uint32_t bsrr_val = set_mask | rst_mask;

    GPIOA->BSRR = bsrr_val;
    GPIOB->BSRR = bsrr_val;
    GPIOC->BSRR = bsrr_val;
    GPIOD->BSRR = bsrr_val;
    GPIOE->BSRR = bsrr_val;
}

int main(void) {
    SystemCoreClockUpdate();
    delay_init();
    gpio_init();

    while (1) {
        loop_counter++;

        /* 1. Running lights: 0 -> 7 */
        for (int i = 0; i < 8; i++) {
            set_all_ports_leds(1 << i);
            delay_ms(80);
        }

        /* 2. Running lights: 6 -> 1 */
        for (int i = 6; i > 0; i--) {
            set_all_ports_leds(1 << i);
            delay_ms(80);
        }

        /* 3. Blink all 8 LEDs 3 times */
        for (int flash = 0; flash < 3; flash++) {
            set_all_ports_leds(0xFF);
            delay_ms(150);
            set_all_ports_leds(0x00);
            delay_ms(150);
        }
    }
}
