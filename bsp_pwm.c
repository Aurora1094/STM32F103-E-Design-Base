#include "bsp_pwm.h"

#include <stdint.h>

#include "bsp_gpio.h"

#define WAVE_GPIO_PORT             GPIOA
#define WAVE_GPIO_PIN              0U
#define OFFSET_GPIO_PORT           GPIOA
#define OFFSET_GPIO_PIN            1U
#define PWM_TIMER                  TIM2
#define PWM_CARRIER_FREQ_HZ        20000UL
#define WAVE_SINE_TABLE_BITS       6U
#define WAVE_SINE_TABLE_SIZE       (1U << WAVE_SINE_TABLE_BITS)
#define WAVE_PHASE_INDEX_SHIFT     (32U - WAVE_SINE_TABLE_BITS)
#define WAVE_SINE_Q15_MAX          32767L

static const int16_t s_sine_q15[WAVE_SINE_TABLE_SIZE] = {
         0,   3212,   6393,   9512,  12539,  15446,  18204,  20787,
     23170,  25329,  27245,  28898,  30273,  31356,  32137,  32609,
     32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
     23170,  20787,  18204,  15446,  12539,   9512,   6393,   3212,
         0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539,  -9512,  -6393,  -3212
};

static uint32_t s_wave_freq_hz = BSP_PWM_DEFAULT_FREQ_HZ;
static volatile uint32_t s_wave_phase = 0UL;
static volatile uint32_t s_wave_phase_step = 0UL;
static volatile uint16_t s_wave_amplitude_permille = BSP_PWM_DEFAULT_AMPLITUDE;
static uint16_t s_pwm_arr = 3599U;
static uint16_t s_offset_duty_permille = BSP_PWM_DEFAULT_DUTY;

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

static void BSP_PWM_UpdatePhaseStep(void)
{
    uint64_t step;

    step = ((uint64_t)s_wave_freq_hz << 32U) / (uint64_t)PWM_CARRIER_FREQ_HZ;

    __disable_irq();
    s_wave_phase_step = (uint32_t)step;
    __enable_irq();
}

static void BSP_PWM_ApplyOffsetDuty(void)
{
    uint32_t compare;
    uint32_t period_ticks;

    period_ticks = (uint32_t)s_pwm_arr + 1UL;
    compare = (period_ticks * (uint32_t)s_offset_duty_permille) / 1000UL;

    if (compare > period_ticks) {
        compare = period_ticks;
    }

    PWM_TIMER->CCR2 = (uint16_t)compare;
}

static void BSP_PWM_ApplyWaveSample(void)
{
    uint32_t period_ticks;
    uint32_t mid_ticks;
    uint32_t max_deviation;
    uint32_t index;
    uint16_t amplitude_permille;
    int32_t sine_q15;
    int32_t deviation;
    int32_t compare;

    period_ticks = (uint32_t)s_pwm_arr + 1UL;
    mid_ticks = period_ticks / 2UL;
    max_deviation = period_ticks / 2UL;
    index = s_wave_phase >> WAVE_PHASE_INDEX_SHIFT;
    amplitude_permille = s_wave_amplitude_permille;

    sine_q15 = (int32_t)s_sine_q15[index];
    deviation = ((int32_t)max_deviation * sine_q15) / WAVE_SINE_Q15_MAX;
    deviation = (deviation * (int32_t)amplitude_permille) / 1000L;
    compare = (int32_t)mid_ticks + deviation;

    if (compare < 0L) {
        compare = 0L;
    } else if ((uint32_t)compare > period_ticks) {
        compare = (int32_t)period_ticks;
    }

    PWM_TIMER->CCR1 = (uint16_t)compare;
}

static void BSP_PWM_StepWave(void)
{
    BSP_PWM_ApplyWaveSample();
    s_wave_phase += s_wave_phase_step;
}

static void BSP_PWM_ConfigCarrierTimer(void)
{
    uint32_t timer_clock;
    uint32_t prescaler;
    uint32_t period_ticks;

    timer_clock = BSP_PWM_GetTimerClock();
    period_ticks = timer_clock / PWM_CARRIER_FREQ_HZ;
    prescaler = (period_ticks + 65535UL) / 65536UL;

    if (prescaler < 1UL) {
        prescaler = 1UL;
    } else if (prescaler > 65536UL) {
        prescaler = 65536UL;
    }

    period_ticks = timer_clock / (PWM_CARRIER_FREQ_HZ * prescaler);
    if (period_ticks < 2UL) {
        period_ticks = 2UL;
    } else if (period_ticks > 65536UL) {
        period_ticks = 65536UL;
    }

    s_pwm_arr = (uint16_t)(period_ticks - 1UL);

    PWM_TIMER->CR1 = 0U;
    PWM_TIMER->PSC = (uint16_t)(prescaler - 1UL);
    PWM_TIMER->ARR = s_pwm_arr;
    PWM_TIMER->CNT = 0U;

    PWM_TIMER->CCMR1 = (PWM_TIMER->CCMR1 &
                        ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M |
                          TIM_CCMR1_CC2S | TIM_CCMR1_OC2M)) |
                       TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE |
                       TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;
    PWM_TIMER->CCER = (PWM_TIMER->CCER &
                       ~(TIM_CCER_CC1P | TIM_CCER_CC1NP |
                         TIM_CCER_CC2P | TIM_CCER_CC2NP)) |
                      TIM_CCER_CC1E | TIM_CCER_CC2E;
    PWM_TIMER->CR1 |= TIM_CR1_ARPE;
}

void BSP_PWM_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    BSP_GPIO_ConfigPin(WAVE_GPIO_PORT, WAVE_GPIO_PIN, BSP_GPIO_AF_PP_50MHZ);
    BSP_GPIO_ConfigPin(OFFSET_GPIO_PORT, OFFSET_GPIO_PIN, BSP_GPIO_AF_PP_50MHZ);

    BSP_PWM_ConfigCarrierTimer();
    BSP_PWM_UpdatePhaseStep();
    BSP_PWM_ApplyWaveSample();
    BSP_PWM_ApplyOffsetDuty();

    PWM_TIMER->SR = 0U;
    PWM_TIMER->DIER = TIM_DIER_UIE;
    PWM_TIMER->EGR = TIM_EGR_UG;

    NVIC_EnableIRQ(TIM2_IRQn);
    BSP_PWM_Start();
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
    if (freq_hz < BSP_PWM_MIN_FREQ_HZ) {
        freq_hz = BSP_PWM_MIN_FREQ_HZ;
    } else if (freq_hz > BSP_PWM_MAX_FREQ_HZ) {
        freq_hz = BSP_PWM_MAX_FREQ_HZ;
    }

    s_wave_freq_hz = freq_hz;
    BSP_PWM_UpdatePhaseStep();
}

void BSP_PWM_SetDutyPermille(uint16_t duty_permille)
{
    if (duty_permille > 1000U) {
        duty_permille = 1000U;
    }

    s_offset_duty_permille = duty_permille;
    BSP_PWM_ApplyOffsetDuty();
}

void BSP_PWM_SetAmplitudePermille(uint16_t amplitude_permille)
{
    if (amplitude_permille > 1000U) {
        amplitude_permille = 1000U;
    }

    s_wave_amplitude_permille = amplitude_permille;
}

uint32_t BSP_PWM_GetFrequency(void)
{
    return s_wave_freq_hz;
}

uint16_t BSP_PWM_GetDutyPermille(void)
{
    return s_offset_duty_permille;
}

uint16_t BSP_PWM_GetAmplitudePermille(void)
{
    return s_wave_amplitude_permille;
}

void BSP_PWM_TIM2_IRQHandler(void)
{
    if ((PWM_TIMER->SR & TIM_SR_UIF) != 0U) {
        PWM_TIMER->SR = (uint16_t)~TIM_SR_UIF;
        BSP_PWM_StepWave();
    }
}

#ifndef __WEAK
#if defined(__GNUC__)
#define __WEAK __attribute__((weak))
#else
#define __WEAK
#endif
#endif

__WEAK void TIM2_IRQHandler(void)
{
    BSP_PWM_TIM2_IRQHandler();
}
