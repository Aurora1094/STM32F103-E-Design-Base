#ifndef __TFT_H
#define __TFT_H

#include "stm32f1xx.h"

#define TFT_WIDTH       128U
#define TFT_HEIGHT      160U

#define TFT_BLACK       0x0000U
#define TFT_WHITE       0xFFFFU
#define TFT_RED         0xF800U
#define TFT_GREEN       0x07E0U
#define TFT_BLUE        0x001FU
#define TFT_CYAN        0x07FFU
#define TFT_YELLOW      0xFFE0U

void TFT_Init(void);
void TFT_Clear(void);
void TFT_FillScreen(uint16_t color);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void TFT_ClearLine(uint8_t line);
void TFT_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg, uint8_t scale);
void TFT_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);

#endif
