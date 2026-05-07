#ifndef __BSP_FREQ_H
#define __BSP_FREQ_H

#include "stm32f1xx.h"

void BSP_Freq_Init(void);
void BSP_Freq_Task(void);
uint32_t BSP_Freq_GetMilliHz(void);
uint32_t BSP_Freq_GetHz(void);
void BSP_Freq_TIM3_IRQHandler(void);

#endif
