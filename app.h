#ifndef __APP_H
#define __APP_H

#include "stm32f1xx.h"

#ifndef APP_USE_TFT
#define APP_USE_TFT 0U
#endif

#ifdef APP_OLED_SELF_TEST
#undef APP_OLED_SELF_TEST
#endif
#define APP_OLED_SELF_TEST 0U

#ifndef APP_SAFE_DISPLAY_ONLY
#define APP_SAFE_DISPLAY_ONLY 0U
#endif

void APP_Init(void);
void APP_Task(void);
void APP_Run(void);

#endif
