#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f1xx.h"

void BSP_ADC_Init(void);
uint8_t BSP_ADC_IsReady(void);
uint16_t BSP_ADC_ReadRaw(void);
uint16_t BSP_ADC_ReadMilliVolt(void);
uint16_t BSP_ADC_ReadMilliVoltSafe(uint16_t fallback_mv);
uint16_t BSP_ADC_ConvertToInputMilliVolt(uint16_t adc_mv);
int16_t BSP_ADC_ConvertToInputSignedMilliVolt(uint16_t adc_mv);
uint16_t BSP_ADC_ReadInputMilliVolt(void);
float BSP_ADC_ReadVoltage(void);

#endif
