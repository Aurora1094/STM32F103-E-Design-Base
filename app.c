#include "app.h"

#include <stdio.h>

#include "bsp_adc.h"
#include "bsp_freq.h"
#include "bsp_key.h"
#include "bsp_pwm.h"
#include "delay.h"
#include "oled.h"
#if APP_USE_TFT
#include "tft.h"
#endif

#define APP_SELECT_FREQ        0U
#define APP_SELECT_DUTY        1U
#define APP_PWM_DUTY_STEP      50U
#define APP_TFT_TEXT_Y_OFFSET  4U

static uint8_t s_selected_item = APP_SELECT_FREQ;
static uint32_t s_pwm_freq_hz = BSP_PWM_DEFAULT_FREQ_HZ;
static uint16_t s_pwm_duty_permille = BSP_PWM_DEFAULT_DUTY;

#if APP_OLED_SELF_TEST
static void APP_RunOledSelfTest(void)
{
    while (1) {
        OLED_Fill(0xFF);
        Delay_ms(500);

        OLED_Clear();
        OLED_ShowString(0U, 0U, "OLED TEST");
        OLED_ShowString(0U, 2U, "PB10/11");
        OLED_ShowString(0U, 3U, "OR PB6/7");
        Delay_ms(1000);
    }
}
#endif

static void APP_DisplayLine(uint8_t line, const char *text)
{
#if APP_USE_TFT
    uint16_t tft_y;
#endif

    OLED_ClearLine(line);
    OLED_ShowString(0U, line, text);

#if APP_USE_TFT
    TFT_ClearLine(line);
    tft_y = (uint16_t)((uint16_t)line * 16U + APP_TFT_TEXT_Y_OFFSET);
    TFT_ShowString(0U, tft_y, text, TFT_WHITE, TFT_BLACK, 1U);
#endif
}

static uint32_t APP_GetFrequencyStep(void)
{
    if (s_pwm_freq_hz < 1000UL) {
        return 100UL;
    }

    if (s_pwm_freq_hz < 10000UL) {
        return 1000UL;
    }

    return 10000UL;
}

static void APP_FormatVoltage(char *line, const char *name, uint16_t mv)
{
    uint16_t centivolt;

    centivolt = (uint16_t)((mv + 5U) / 10U);
    sprintf(line, "%s %u.%02uV",
            name,
            (unsigned int)(centivolt / 100U),
            (unsigned int)(centivolt % 100U));
}

static void APP_AdjustSelected(int8_t direction)
{
    uint32_t freq_step;

    if (s_selected_item == APP_SELECT_FREQ) {
        freq_step = APP_GetFrequencyStep();

        if (direction > 0) {
            if ((BSP_PWM_MAX_FREQ_HZ - s_pwm_freq_hz) < freq_step) {
                s_pwm_freq_hz = BSP_PWM_MAX_FREQ_HZ;
            } else {
                s_pwm_freq_hz += freq_step;
            }
        } else {
            if ((s_pwm_freq_hz - BSP_PWM_MIN_FREQ_HZ) < freq_step) {
                s_pwm_freq_hz = BSP_PWM_MIN_FREQ_HZ;
            } else {
                s_pwm_freq_hz -= freq_step;
            }
        }

        BSP_PWM_SetFrequency(s_pwm_freq_hz);
    } else {
        if (direction > 0) {
            if ((1000U - s_pwm_duty_permille) < APP_PWM_DUTY_STEP) {
                s_pwm_duty_permille = 1000U;
            } else {
                s_pwm_duty_permille = (uint16_t)(s_pwm_duty_permille + APP_PWM_DUTY_STEP);
            }
        } else {
            if (s_pwm_duty_permille < APP_PWM_DUTY_STEP) {
                s_pwm_duty_permille = 0U;
            } else {
                s_pwm_duty_permille = (uint16_t)(s_pwm_duty_permille - APP_PWM_DUTY_STEP);
            }
        }

        BSP_PWM_SetDutyPermille(s_pwm_duty_permille);
    }
}

static void APP_HandleKeys(uint8_t key_event)
{
    if ((key_event & (BSP_KEY1_EVENT | BSP_KEYA_EVENT)) != 0U) {
        if (s_selected_item == APP_SELECT_FREQ) {
            s_selected_item = APP_SELECT_DUTY;
        } else {
            s_selected_item = APP_SELECT_FREQ;
        }
    }

    if ((key_event & (BSP_KEY2_EVENT | BSP_KEYB_EVENT)) != 0U) {
        APP_AdjustSelected(1);
    }

    if ((key_event & (BSP_KEY3_EVENT | BSP_KEYD_EVENT)) != 0U) {
        APP_AdjustSelected(-1);
    }
}

static void APP_ShowRuntimeInfo(void)
{
    char line[24];
    uint16_t adc_mv;
    uint16_t input_mv;
    uint32_t freq_hz;
    uint16_t duty_percent;

    APP_DisplayLine(0U, "WAVE GEN");

    if (s_selected_item == APP_SELECT_FREQ) {
        APP_DisplayLine(1U, "SEL FREQ");
    } else {
        APP_DisplayLine(1U, "SEL DUTY");
    }

    sprintf(line, "PWM %luHZ", (unsigned long)s_pwm_freq_hz);
    APP_DisplayLine(2U, line);

    duty_percent = (uint16_t)((s_pwm_duty_permille + 5U) / 10U);
    sprintf(line, "DUTY %03u%%", (unsigned int)duty_percent);
    APP_DisplayLine(3U, line);

    BSP_Freq_Task();
    freq_hz = BSP_Freq_GetHz();
    sprintf(line, "FIN %luHZ", (unsigned long)freq_hz);
    APP_DisplayLine(4U, line);

    adc_mv = BSP_ADC_ReadMilliVolt();
    input_mv = BSP_ADC_ConvertToInputMilliVolt(adc_mv);

    APP_FormatVoltage(line, "ADC", adc_mv);
    APP_DisplayLine(5U, line);

    APP_FormatVoltage(line, "VIN", input_mv);
    APP_DisplayLine(6U, line);

    APP_DisplayLine(7U, "K1SEL K2+ K3-");
}

void APP_Init(void)
{
    Delay_Init();
    OLED_Init();

#if APP_OLED_SELF_TEST
    APP_RunOledSelfTest();
#endif

    OLED_Clear();

    BSP_Key_Init();
    BSP_ADC_Init();
    BSP_PWM_Init();
    BSP_Freq_Init();

#if APP_USE_TFT
    TFT_Init();
    TFT_Clear();
#endif

    APP_ShowRuntimeInfo();
}

void APP_Task(void)
{
    static uint8_t refresh_count = 0U;
    uint8_t key_event;
    uint8_t need_refresh;

    need_refresh = 0U;
    key_event = BSP_Key_Scan();

    if (key_event != 0U) {
        APP_HandleKeys(key_event);
        need_refresh = 1U;
    }

    BSP_Freq_Task();

    refresh_count++;
    if (refresh_count >= 20U) {
        refresh_count = 0U;
        need_refresh = 1U;
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
