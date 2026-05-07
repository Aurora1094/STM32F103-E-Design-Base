#include "delay.h"

static uint32_t s_ticks_per_us = 0UL;

void Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0UL;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_ticks_per_us = SystemCoreClock / 1000000UL;
    if (s_ticks_per_us == 0UL) {
        s_ticks_per_us = 1UL;
    }
}

void Delay_us(uint32_t us)
{
    uint32_t start_tick;
    uint32_t wait_ticks;

    if (s_ticks_per_us == 0UL) {
        Delay_Init();
    }

    start_tick = DWT->CYCCNT;
    wait_ticks = us * s_ticks_per_us;

    while ((DWT->CYCCNT - start_tick) < wait_ticks) {
    }
}

void Delay_ms(uint32_t ms)
{
    while (ms > 0UL) {
        Delay_us(1000UL);
        ms--;
    }
}
