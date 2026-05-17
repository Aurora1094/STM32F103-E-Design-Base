#include "bsp_pwm.h"

#include "bsp_gpio.h"

#define PWM_GPIO_PORT        GPIOA
#define PWM_GPIO_PIN         2U
#define PWM_TIMER            TIM2
#define AMP_GPIO_PORT        GPIOA
#define AMP_GPIO_PIN         8U
#define AMP_TIMER            TIM1
#define AMP_PWM_FREQ_HZ      20000UL

#ifndef RCC_CFGR_PPRE2_Pos
#define RCC_CFGR_PPRE2_Pos   11U
#endif

static uint32_t s_pwm_freq_hz = BSP_PWM_DEFAULT_FREQ_HZ;
static uint16_t s_pwm_duty_permille = BSP_PWM_DEFAULT_DUTY;
static uint16_t s_pwm_arr = 999U;
static uint16_t s_amp_duty_permille = 300U;
static uint16_t s_amp_arr = 3599U;

static uint32_t BSP_PWM_GetTimerClock(void)
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

static uint32_t BSP_PWM_GetAPB2TimerClock(void)
{
    static const uint8_t apb_prescaler_table[8] = {1U, 1U, 1U, 1U, 2U, 4U, 8U, 16U};
    uint32_t ppre2_bits;
    uint32_t ppre2_div;
    uint32_t pclk2;

    ppre2_bits = (RCC->CFGR >> RCC_CFGR_PPRE2_Pos) & 0x7UL;
    ppre2_div = apb_prescaler_table[ppre2_bits];
    pclk2 = SystemCoreClock / ppre2_div;

    if (ppre2_div != 1UL) {
        pclk2 *= 2UL;
    }

    return pclk2;
}

static void BSP_PWM_ApplyDuty(void)
{
    uint32_t compare;
    uint32_t period_ticks;

    period_ticks = (uint32_t)s_pwm_arr + 1UL;
    compare = (period_ticks * (uint32_t)s_pwm_duty_permille) / 1000UL;

    if (compare > 65535UL) {
        compare = 65535UL;
    }

    PWM_TIMER->CCR3 = (uint16_t)compare;
}

static void BSP_PWM_ApplyAmplitudeDuty(void)
{
    uint32_t compare;
    uint32_t period_ticks;

    period_ticks = (uint32_t)s_amp_arr + 1UL;
    compare = (period_ticks * (uint32_t)s_amp_duty_permille) / 1000UL;

    if (compare > 65535UL) {
        compare = 65535UL;
    }

    AMP_TIMER->CCR1 = (uint16_t)compare;
}

static void BSP_PWM_InitAmplitudeControl(void)
{
    uint32_t timer_clock;
    uint32_t prescaler;
    uint32_t period_ticks;

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_TIM1EN;

    BSP_GPIO_ConfigPin(AMP_GPIO_PORT, AMP_GPIO_PIN, BSP_GPIO_AF_PP_50MHZ);

    timer_clock = BSP_PWM_GetAPB2TimerClock();
    period_ticks = timer_clock / AMP_PWM_FREQ_HZ;
    prescaler = (period_ticks + 65535UL) / 65536UL;

    if (prescaler < 1UL) {
        prescaler = 1UL;
    } else if (prescaler > 65536UL) {
        prescaler = 65536UL;
    }

    period_ticks = timer_clock / (AMP_PWM_FREQ_HZ * prescaler);
    if (period_ticks < 2UL) {
        period_ticks = 2UL;
    } else if (period_ticks > 65536UL) {
        period_ticks = 65536UL;
    }

    s_amp_arr = (uint16_t)(period_ticks - 1UL);

    AMP_TIMER->CR1 = 0U;
    AMP_TIMER->PSC = (uint16_t)(prescaler - 1UL);
    AMP_TIMER->ARR = s_amp_arr;
    AMP_TIMER->CNT = 0U;

    AMP_TIMER->CCMR1 = (AMP_TIMER->CCMR1 & ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M)) |
                       TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE;
    AMP_TIMER->CCER = (AMP_TIMER->CCER & ~(TIM_CCER_CC1P | TIM_CCER_CC1NP)) |
                      TIM_CCER_CC1E;
    AMP_TIMER->BDTR |= TIM_BDTR_MOE;
    AMP_TIMER->CR1 |= TIM_CR1_ARPE;

    BSP_PWM_ApplyAmplitudeDuty();
    AMP_TIMER->EGR = TIM_EGR_UG;
    AMP_TIMER->CR1 |= TIM_CR1_CEN;
}

void BSP_PWM_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    BSP_GPIO_ConfigPin(PWM_GPIO_PORT, PWM_GPIO_PIN, BSP_GPIO_AF_PP_50MHZ);

    PWM_TIMER->CR1 = 0U;
    PWM_TIMER->PSC = 71U;
    PWM_TIMER->ARR = s_pwm_arr;
    PWM_TIMER->CNT = 0U;

    PWM_TIMER->CCMR2 = (PWM_TIMER->CCMR2 & ~(TIM_CCMR2_CC3S | TIM_CCMR2_OC3M)) |
                       TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE;
    PWM_TIMER->CCER = (PWM_TIMER->CCER & ~(TIM_CCER_CC3P | TIM_CCER_CC3NP)) |
                      TIM_CCER_CC3E;
    PWM_TIMER->CR1 |= TIM_CR1_ARPE;

    BSP_PWM_SetFrequency(s_pwm_freq_hz);
    BSP_PWM_SetDutyPermille(s_pwm_duty_permille);
    BSP_PWM_Start();
    BSP_PWM_InitAmplitudeControl();
}

void BSP_PWM_Start(void)
{
    PWM_TIMER->CR1 |= TIM_CR1_CEN;
}

void BSP_PWM_Stop(void)
{
    PWM_TIMER->CR1 &= ~TIM_CR1_CEN;
}

void BSP_PWM_SetFrequency(uint32_t freq_hz)
{
    uint32_t timer_clock;
    uint32_t prescaler;
    uint32_t period_ticks;
    uint32_t arr;

    if (freq_hz < BSP_PWM_MIN_FREQ_HZ) {
        freq_hz = BSP_PWM_MIN_FREQ_HZ;
    } else if (freq_hz > BSP_PWM_MAX_FREQ_HZ) {
        freq_hz = BSP_PWM_MAX_FREQ_HZ;
    }

    timer_clock = BSP_PWM_GetTimerClock();
    period_ticks = timer_clock / freq_hz;
    prescaler = (period_ticks + 65535UL) / 65536UL;

    if (prescaler < 1UL) {
        prescaler = 1UL;
    } else if (prescaler > 65536UL) {
        prescaler = 65536UL;
    }

    period_ticks = timer_clock / (freq_hz * prescaler);
    if (period_ticks < 2UL) {
        period_ticks = 2UL;
    } else if (period_ticks > 65536UL) {
        period_ticks = 65536UL;
    }

    arr = period_ticks - 1UL;
    s_pwm_freq_hz = freq_hz;
    s_pwm_arr = (uint16_t)arr;

    PWM_TIMER->PSC = (uint16_t)(prescaler - 1UL);
    PWM_TIMER->ARR = s_pwm_arr;
    BSP_PWM_ApplyDuty();
    PWM_TIMER->EGR = TIM_EGR_UG;
}

void BSP_PWM_SetDutyPermille(uint16_t duty_permille)
{
    if (duty_permille > 1000U) {
        duty_permille = 1000U;
    }

    s_pwm_duty_permille = duty_permille;
    BSP_PWM_ApplyDuty();
    PWM_TIMER->EGR = TIM_EGR_UG;
}

void BSP_PWM_SetAmplitudePermille(uint16_t duty_permille)
{
    if (duty_permille > 1000U) {
        duty_permille = 1000U;
    }

    s_amp_duty_permille = duty_permille;
    BSP_PWM_ApplyAmplitudeDuty();
    AMP_TIMER->EGR = TIM_EGR_UG;
}

uint32_t BSP_PWM_GetFrequency(void)
{
    return s_pwm_freq_hz;
}

uint16_t BSP_PWM_GetDutyPermille(void)
{
    return s_pwm_duty_permille;
}

uint16_t BSP_PWM_GetAmplitudePermille(void)
{
    return s_amp_duty_permille;
}
