#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f10x.h"

void BSP_ADC_Init(void);
uint16_t BSP_ADC_ReadRaw(void);
uint16_t BSP_ADC_ReadMilliVolt(void);
float BSP_ADC_ReadVoltage(void);

#endif

