#include "bsp_adc.h"

#include "bsp_gpio.h"

#define ADC_INPUT_GPIO_PORT      GPIOA
#define ADC_INPUT_GPIO_PIN       3U
#define ADC_INPUT_CHANNEL        3U

void BSP_ADC_Init(void)
{
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
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0U) {
    }

    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0U) {
    }
}

uint16_t BSP_ADC_ReadRaw(void)
{
    ADC1->SQR3 = ADC_INPUT_CHANNEL;
    ADC1->SR = 0U;
    ADC1->CR2 = (ADC1->CR2 & ~ADC_CR2_EXTSEL) | ADC_CR2_EXTSEL;
    ADC1->CR2 |= ADC_CR2_EXTTRIG | ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0U) {
    }

    return (uint16_t)ADC1->DR;
}

uint16_t BSP_ADC_ReadMilliVolt(void)
{
    uint32_t raw;

    raw = BSP_ADC_ReadRaw();
    return (uint16_t)((raw * 3300UL + 2047UL) / 4095UL);
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

uint16_t BSP_ADC_ReadInputMilliVolt(void)
{
    return BSP_ADC_ConvertToInputMilliVolt(BSP_ADC_ReadMilliVolt());
}

float BSP_ADC_ReadVoltage(void)
{
    return (float)BSP_ADC_ReadMilliVolt() / 1000.0f;
}
