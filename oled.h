#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx.h"

#define OLED_WIDTH       128U
#define OLED_HEIGHT      64U

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t data);
void OLED_EntireDisplayOn(uint8_t enable);
void OLED_ClearLine(uint8_t page);
void OLED_ShowChar(uint8_t x, uint8_t page, char ch);
void OLED_ShowString(uint8_t x, uint8_t page, const char *str);
void OLED_ShowChinese(uint8_t x, uint8_t page, uint8_t index);

#endif
