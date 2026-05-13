#include "delay.h"

static uint32_t s_ticks_per_us = 0UL;
static uint8_t s_dwt_enabled = 0U;

static void Delay_FallbackCycles(uint32_t cycles)
{
    while (cycles > 0UL) {
        __NOP();
        cycles--;
    }
}

void Delay_Init(void)
{
    uint32_t start_tick;
    uint32_t i;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0UL;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_ticks_per_us = SystemCoreClock / 1000000UL;
    if (s_ticks_per_us == 0UL) {
        s_ticks_per_us = 1UL;
    }

    start_tick = DWT->CYCCNT;
    for (i = 0UL; i < 1000UL; i++) {
        __NOP();
    }

    s_dwt_enabled = (DWT->CYCCNT != start_tick) ? 1U : 0U;
}

void Delay_us(uint32_t us)
{
    uint32_t start_tick;
    uint32_t wait_ticks;

    if (s_ticks_per_us == 0UL) {
        Delay_Init();
    }

    if (s_dwt_enabled != 0U) {
        start_tick = DWT->CYCCNT;
        wait_ticks = us * s_ticks_per_us;

        while ((DWT->CYCCNT - start_tick) < wait_ticks) {
        }

        return;
    }

    while (us > 0UL) {
        Delay_FallbackCycles(s_ticks_per_us);
        us--;
    }
}

void Delay_ms(uint32_t ms)
{
    while (ms > 0UL) {
        Delay_us(1000UL);
        ms--;
    }
}
