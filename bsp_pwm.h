#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "stm32f1xx.h"

#define BSP_PWM_MIN_FREQ_HZ          1UL
#define BSP_PWM_MAX_FREQ_HZ          1000UL
#define BSP_PWM_DEFAULT_FREQ_HZ      1000UL
#define BSP_PWM_DEFAULT_DUTY         500U
#define BSP_PWM_DEFAULT_AMPLITUDE    300U

void BSP_PWM_Init(void);
void BSP_PWM_Start(void);
void BSP_PWM_Stop(void);
void BSP_PWM_SetFrequency(uint32_t freq_hz);
void BSP_PWM_SetDutyPermille(uint16_t duty_permille);
void BSP_PWM_SetAmplitudePermille(uint16_t amplitude_permille);
uint32_t BSP_PWM_GetFrequency(void);
uint16_t BSP_PWM_GetDutyPermille(void);
uint16_t BSP_PWM_GetAmplitudePermille(void);
void BSP_PWM_TIM2_IRQHandler(void);

#endif
