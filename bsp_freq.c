#include "bsp_freq.h"

#include "bsp_gpio.h"

#define FREQ_GPIO_PORT             GPIOA
#define FREQ_GPIO_PIN              6U
#define FREQ_GPIO_MASK             (1U << FREQ_GPIO_PIN)
#define FREQ_TIMER                 TIM3
#define FREQ_TIMER_TICK_HZ         1000000UL
#define FREQ_TIMEOUT_US            1000000UL

static volatile uint32_t s_freq_overflow_count = 0UL;
static volatile uint32_t s_freq_last_capture_time = 0UL;
static volatile uint32_t s_freq_millihz = 0UL;
static volatile uint8_t s_freq_have_capture = 0U;

static uint32_t BSP_Freq_GetTimerClock(void)
{
    static const uint8_t apb_prescaler_table[8] = {1U, 1U, 1U, 1U, 2U, 4U, 8U, 16U};
    uint32_t ppre1_bits;
    uint32_t ppre1_div;
    uint32_t pclk1;

    ppre1_bits = (RCC->CFGR >> RCC_CFGR_PPRE1_Pos) & 0x7UL;
    ppre1_div = apb_prescaler_table[ppre1_bits];
    pclk1 = SystemCoreClock / ppre1_div;

    if (ppre1_div != 1UL) {
        pclk1 *= 2UL;
    }

    return pclk1;
}

static uint32_t BSP_Freq_GetTimestampUnsafe(void)
{
    return (s_freq_overflow_count << 16) | (uint32_t)FREQ_TIMER->CNT;
}

void BSP_Freq_Init(void)
{
    uint32_t timer_clock;
    uint32_t prescaler;

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    BSP_GPIO_ConfigPin(FREQ_GPIO_PORT, FREQ_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    FREQ_GPIO_PORT->ODR &= (uint16_t)~FREQ_GPIO_MASK;

    timer_clock = BSP_Freq_GetTimerClock();
    prescaler = timer_clock / FREQ_TIMER_TICK_HZ;
    if (prescaler == 0UL) {
        prescaler = 1UL;
    }

    FREQ_TIMER->CR1 = 0U;
    FREQ_TIMER->PSC = (uint16_t)(prescaler - 1UL);
    FREQ_TIMER->ARR = 0xFFFFU;
    FREQ_TIMER->CNT = 0U;

    FREQ_TIMER->CCMR1 = (FREQ_TIMER->CCMR1 & ~(TIM_CCMR1_CC1S | TIM_CCMR1_IC1F)) |
                        TIM_CCMR1_CC1S_0 | (3UL << TIM_CCMR1_IC1F_Pos);
    FREQ_TIMER->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
    FREQ_TIMER->CCER |= TIM_CCER_CC1E;

    s_freq_overflow_count = 0UL;
    s_freq_last_capture_time = 0UL;
    s_freq_millihz = 0UL;
    s_freq_have_capture = 0U;

    FREQ_TIMER->SR = 0U;
    FREQ_TIMER->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE;

    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, 1U);
    NVIC_EnableIRQ(TIM3_IRQn);

    FREQ_TIMER->CR1 |= TIM_CR1_CEN;
}

void BSP_Freq_Task(void)
{
    uint32_t now;
    uint32_t last_capture;
    uint8_t have_capture;

    __disable_irq();
    now = BSP_Freq_GetTimestampUnsafe();
    last_capture = s_freq_last_capture_time;
    have_capture = s_freq_have_capture;
    __enable_irq();

    if ((have_capture != 0U) && ((now - last_capture) > FREQ_TIMEOUT_US)) {
        __disable_irq();
        s_freq_millihz = 0UL;
        __enable_irq();
    }
}

uint32_t BSP_Freq_GetMilliHz(void)
{
    uint32_t millihz;

    __disable_irq();
    millihz = s_freq_millihz;
    __enable_irq();

    return millihz;
}

uint32_t BSP_Freq_GetHz(void)
{
    return (BSP_Freq_GetMilliHz() + 500UL) / 1000UL;
}

void BSP_Freq_TIM3_IRQHandler(void)
{
    uint16_t capture;
    uint32_t timestamp;
    uint32_t delta;
    uint32_t status;

    status = FREQ_TIMER->SR;

    if ((status & TIM_SR_UIF) != 0U) {
        FREQ_TIMER->SR = (uint16_t)~TIM_SR_UIF;
        s_freq_overflow_count++;
    }

    if ((status & TIM_SR_CC1IF) != 0U) {
        capture = (uint16_t)FREQ_TIMER->CCR1;
        FREQ_TIMER->SR = (uint16_t)~TIM_SR_CC1IF;

        timestamp = (s_freq_overflow_count << 16) | (uint32_t)capture;

        if (s_freq_have_capture != 0U) {
            delta = timestamp - s_freq_last_capture_time;
            if (delta != 0UL) {
                s_freq_millihz = 1000000000UL / delta;
            }
        }

        s_freq_last_capture_time = timestamp;
        s_freq_have_capture = 1U;
    }
}

#ifndef __WEAK
#if defined(__GNUC__)
#define __WEAK __attribute__((weak))
#else
#define __WEAK
#endif
#endif

__WEAK void TIM3_IRQHandler(void)
{
    BSP_Freq_TIM3_IRQHandler();
}
