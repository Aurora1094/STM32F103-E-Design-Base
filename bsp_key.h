#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "stm32f10x.h"

#define BSP_KEY1_EVENT    0x01U
#define BSP_KEY2_EVENT    0x02U
#define BSP_KEY3_EVENT    0x04U

void BSP_Key_Init(void);
uint8_t BSP_Key_Scan(void);

#endif

