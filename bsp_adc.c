#include "bsp_adc.h"

#include "bsp_gpio.h"

#define ADC_INPUT_GPIO_PORT      GPIOA
#define ADC_INPUT_GPIO_PIN       3U
#define ADC_INPUT_CHANNEL        3U
#define ADC_TIMEOUT_COUNT        100000UL

static uint8_t s_adc_ready = 0U;

static uint8_t BSP_ADC_WaitClear(uint32_t mask)
{
    uint32_t timeout;

    timeout = ADC_TIMEOUT_COUNT;
    while ((ADC1->CR2 & mask) != 0U) {
        if (timeout == 0UL) {
            return 0U;
        }
        timeout--;
    }

    return 1U;
}

void BSP_ADC_Init(void)
{
    s_adc_ready = 0U;

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV6;

    BSP_GPIO_ConfigPin(ADC_INPUT_GPIO_PORT, ADC_INPUT_GPIO_PIN, BSP_GPIO_ANALOG_INPUT);

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;
    ADC1->SQR1 = 0U;
    ADC1->SMPR2 = (ADC1->SMPR2 & ~(7UL << (ADC_INPUT_CHANNEL * 3U))) |
                  (5UL << (ADC_INPUT_CHANNEL * 3U));

    ADC1->CR2 |= ADC_CR2_ADON;

    ADC1->CR2 |= ADC_CR2_RSTCAL;
    if (BSP_ADC_WaitClear(ADC_CR2_RSTCAL) == 0U) {
        return;
    }

    ADC1->CR2 |= ADC_CR2_CAL;
    if (BSP_ADC_WaitClear(ADC_CR2_CAL) == 0U) {
        return;
    }

    s_adc_ready = 1U;
}

uint8_t BSP_ADC_IsReady(void)
{
    return s_adc_ready;
}

uint16_t BSP_ADC_ReadRaw(void)
{
    uint32_t timeout;

    if (s_adc_ready == 0U) {
        return 0U;
    }

    ADC1->SQR3 = ADC_INPUT_CHANNEL;
    ADC1->SR = 0U;
    ADC1->CR2 = (ADC1->CR2 & ~ADC_CR2_EXTSEL) | ADC_CR2_EXTSEL;
    ADC1->CR2 |= ADC_CR2_EXTTRIG | ADC_CR2_SWSTART;

    timeout = ADC_TIMEOUT_COUNT;
    while ((ADC1->SR & ADC_SR_EOC) == 0U) {
        if (timeout == 0UL) {
            return 0U;
        }
        timeout--;
    }

    return (uint16_t)ADC1->DR;
}

uint16_t BSP_ADC_ReadMilliVolt(void)
{
    uint32_t raw;

    if (s_adc_ready == 0U) {
        return 0U;
    }

    raw = BSP_ADC_ReadRaw();
    return (uint16_t)((raw * 3300UL + 2047UL) / 4095UL);
}

uint16_t BSP_ADC_ReadMilliVoltSafe(uint16_t fallback_mv)
{
    if (s_adc_ready == 0U) {
        return fallback_mv;
    }

    return BSP_ADC_ReadMilliVolt();
}

uint16_t BSP_ADC_ConvertToInputMilliVolt(uint16_t adc_mv)
{
    int32_t input_mv;

    input_mv = 5000L - ((int32_t)adc_mv * 2L);

    if (input_mv < 0L) {
        input_mv = 0L;
    } else if (input_mv > 5000L) {
        input_mv = 5000L;
    }

    return (uint16_t)input_mv;
}

int16_t BSP_ADC_ConvertToInputSignedMilliVolt(uint16_t adc_mv)
{
    int32_t input_mv;

    input_mv = 5000L - ((int32_t)adc_mv * 2L);

    if (input_mv < -9999L) {
        input_mv = -9999L;
    } else if (input_mv > 9999L) {
        input_mv = 9999L;
    }

    return (int16_t)input_mv;
}

uint16_t BSP_ADC_ReadInputMilliVolt(void)
{
    return BSP_ADC_ConvertToInputMilliVolt(BSP_ADC_ReadMilliVolt());
}

float BSP_ADC_ReadVoltage(void)
{
    return (float)BSP_ADC_ReadMilliVolt() / 1000.0f;
}
