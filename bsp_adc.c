#include "bsp_adc.h"

#include "stm32f10x_adc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define ADC_INPUT_GPIO_PORT      GPIOA
#define ADC_INPUT_GPIO_RCC       RCC_APB2Periph_GPIOA
#define ADC_INPUT_GPIO_PIN       GPIO_Pin_3
#define ADC_INPUT_CHANNEL        ADC_Channel_3

void BSP_ADC_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    RCC_APB2PeriphClockCmd(ADC_INPUT_GPIO_RCC | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    gpio.GPIO_Pin = ADC_INPUT_GPIO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ADC_INPUT_GPIO_PORT, &gpio);

    ADC_DeInit(ADC1);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) != RESET) {
    }

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) != RESET) {
    }
}

uint16_t BSP_ADC_ReadRaw(void)
{
    ADC_RegularChannelConfig(ADC1, ADC_INPUT_CHANNEL, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
    }

    return ADC_GetConversionValue(ADC1);
}

uint16_t BSP_ADC_ReadMilliVolt(void)
{
    uint32_t raw;

    raw = BSP_ADC_ReadRaw();
    return (uint16_t)((raw * 3300UL + 2047UL) / 4095UL);
}

float BSP_ADC_ReadVoltage(void)
{
    return (float)BSP_ADC_ReadMilliVolt() / 1000.0f;
}
