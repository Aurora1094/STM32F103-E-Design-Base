#include "delay.h"

static uint8_t s_ticks_per_us = 0;

void Delay_Init(void)
{
    SysTick->CTRL = 0x00000000UL;
    SysTick->LOAD = 0x00000000UL;
    SysTick->VAL = 0x00000000UL;

    s_ticks_per_us = (uint8_t)(SystemCoreClock / 8000000UL);
    if (s_ticks_per_us == 0U) {
        s_ticks_per_us = 1U;
    }
}

void Delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t load_ticks;
    uint32_t ctrl;

    if (s_ticks_per_us == 0U) {
        Delay_Init();
    }

    ticks = us * (uint32_t)s_ticks_per_us;

    while (ticks > 0UL) {
        if (ticks > 0x00FFFFFFUL) {
            load_ticks = 0x00FFFFFFUL;
        } else {
            load_ticks = ticks;
        }

        SysTick->LOAD = load_ticks;
        SysTick->VAL = 0x00000000UL;
        SysTick->CTRL = 0x00000001UL;

        do {
            ctrl = SysTick->CTRL;
        } while (((ctrl & 0x00000001UL) != 0UL) && ((ctrl & 0x00010000UL) == 0UL));

        SysTick->CTRL = 0x00000000UL;
        SysTick->VAL = 0x00000000UL;

        ticks -= load_ticks;
    }
}

void Delay_ms(uint32_t ms)
{
    while (ms > 0UL) {
        Delay_us(1000UL);
        ms--;
    }
}

