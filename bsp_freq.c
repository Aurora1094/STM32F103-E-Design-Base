#include "bsp_freq.h"

#include "bsp_gpio.h"

#define FREQ_GPIO_PORT             GPIOA
#define FREQ_GPIO_PIN              6U
#define FREQ_GPIO_MASK             (1U << FREQ_GPIO_PIN)
#define FREQ_TIMER                 TIM3
#define FREQ_UPDATE_CYCLES         (SystemCoreClock / 2UL)

static volatile uint32_t s_freq_millihz = 0UL;
static uint16_t s_freq_last_count = 0U;
static uint32_t s_freq_last_cycle = 0UL;

void BSP_Freq_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    BSP_GPIO_ConfigPin(FREQ_GPIO_PORT, FREQ_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    FREQ_GPIO_PORT->ODR &= (uint16_t)~FREQ_GPIO_MASK;

    FREQ_TIMER->CR1 = 0U;
    FREQ_TIMER->PSC = 0U;
    FREQ_TIMER->ARR = 0xFFFFU;
    FREQ_TIMER->CNT = 0U;

    FREQ_TIMER->CCMR1 = (FREQ_TIMER->CCMR1 & ~(TIM_CCMR1_CC1S | TIM_CCMR1_IC1F)) |
                        TIM_CCMR1_CC1S_0 | (3UL << TIM_CCMR1_IC1F_Pos);
    FREQ_TIMER->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
    FREQ_TIMER->CCER |= TIM_CCER_CC1E;

    s_freq_millihz = 0UL;
    s_freq_last_count = 0U;
    s_freq_last_cycle = DWT->CYCCNT;

    FREQ_TIMER->SR = 0U;
    FREQ_TIMER->DIER = 0U;
    FREQ_TIMER->SMCR = (7UL << 0U) | (5UL << 4U);

    FREQ_TIMER->CR1 |= TIM_CR1_CEN;
}

void BSP_Freq_Task(void)
{
    uint32_t now_cycle;
    uint32_t elapsed_cycle;
    uint16_t now_count;
    uint16_t delta_count;
    uint64_t millihz;

    now_cycle = DWT->CYCCNT;
    elapsed_cycle = now_cycle - s_freq_last_cycle;

    if (elapsed_cycle < FREQ_UPDATE_CYCLES) {
        return;
    }

    now_count = (uint16_t)FREQ_TIMER->CNT;
    delta_count = (uint16_t)(now_count - s_freq_last_count);

    millihz = ((uint64_t)delta_count * (uint64_t)SystemCoreClock * 1000ULL) /
              (uint64_t)elapsed_cycle;

    s_freq_millihz = (uint32_t)millihz;
    s_freq_last_count = now_count;
    s_freq_last_cycle = now_cycle;
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
    FREQ_TIMER->SR = 0U;
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
