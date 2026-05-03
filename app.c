#include "app.h"

#include <stdio.h>

#include "bsp_adc.h"
#include "bsp_key.h"
#include "delay.h"
#include "oled.h"

static unsigned long s_key1_count = 0;
static unsigned long s_key2_count = 0;
static unsigned long s_key3_count = 0;

static void APP_ShowTitle(void)
{
    OLED_ShowChinese(32, 0, 0);
    OLED_ShowChinese(48, 0, 1);
    OLED_ShowChinese(64, 0, 2);
    OLED_ShowChinese(80, 0, 3);
}

static void APP_ShowRuntimeInfo(void)
{
    char line[24];
    uint16_t mv;
    uint16_t centivolt;

    OLED_ClearLine(2);
    sprintf(line, "KEY1: %lu", s_key1_count);
    OLED_ShowString(0, 2, line);

    OLED_ClearLine(3);
    sprintf(line, "KEY2: %lu", s_key2_count);
    OLED_ShowString(0, 3, line);

    OLED_ClearLine(4);
    sprintf(line, "KEY3: %lu", s_key3_count);
    OLED_ShowString(0, 4, line);

    mv = BSP_ADC_ReadMilliVolt();
    centivolt = (uint16_t)((mv + 5U) / 10U);

    OLED_ClearLine(5);
    sprintf(line, "ADC: %u.%02uV",
            (unsigned int)(centivolt / 100U),
            (unsigned int)(centivolt % 100U));
    OLED_ShowString(0, 5, line);
}

void APP_Init(void)
{
    Delay_Init();
    BSP_Key_Init();
    BSP_ADC_Init();
    OLED_Init();

    OLED_Clear();
    APP_ShowTitle();
    APP_ShowRuntimeInfo();
}

void APP_Task(void)
{
    static uint8_t refresh_count = 0;
    uint8_t key_event;
    uint8_t need_refresh;

    need_refresh = 0;
    key_event = BSP_Key_Scan();

    if ((key_event & BSP_KEY1_EVENT) != 0U) {
        s_key1_count++;
        need_refresh = 1;
    }

    if ((key_event & BSP_KEY2_EVENT) != 0U) {
        s_key2_count++;
        need_refresh = 1;
    }

    if ((key_event & BSP_KEY3_EVENT) != 0U) {
        s_key3_count++;
        need_refresh = 1;
    }

    refresh_count++;
    if (refresh_count >= 10U) {
        refresh_count = 0;
        need_refresh = 1;
    }

    if (need_refresh != 0U) {
        APP_ShowRuntimeInfo();
    }

    Delay_ms(10);
}

void APP_Run(void)
{
    while (1) {
        APP_Task();
    }
}

