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
#define APP_SELECT_VPP         1U
#define APP_PWM_MIN_FREQ_HZ    100UL
#define APP_PWM_MAX_FREQ_HZ    1000UL
#define APP_PWM_FREQ_STEP_HZ   100UL
#define APP_VPP_MIN_V          1U
#define APP_VPP_MAX_V          10U
#define APP_VPP_DEFAULT_V      3U
#define APP_VPP_STEP_V         1U
#define APP_PWM_MAX_DUTY       1000U
#define APP_PWM_DEFAULT_DUTY   ((uint16_t)(((uint32_t)APP_VPP_DEFAULT_V * APP_PWM_MAX_DUTY) / APP_VPP_MAX_V))
#define APP_ADC_SAMPLE_COUNT   80U
#define APP_TFT_TEXT_Y_OFFSET  4U

static uint8_t s_selected_item = APP_SELECT_FREQ;
static uint32_t s_pwm_freq_hz = BSP_PWM_DEFAULT_FREQ_HZ;
static uint16_t s_pwm_duty_permille = APP_PWM_DEFAULT_DUTY;
static uint8_t s_target_vpp_v = APP_VPP_DEFAULT_V;
static uint16_t s_key_press_count = 0U;
static uint16_t s_adc_mv = 0U;
static uint16_t s_measured_vpp_mv = 0U;

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
    return APP_PWM_FREQ_STEP_HZ;
}

static uint16_t APP_ConvertVppToDutyPermille(uint8_t target_vpp_v)
{
    uint32_t duty_permille;

    if (target_vpp_v < APP_VPP_MIN_V) {
        target_vpp_v = APP_VPP_MIN_V;
    } else if (target_vpp_v > APP_VPP_MAX_V) {
        target_vpp_v = APP_VPP_MAX_V;
    }

    duty_permille = ((uint32_t)target_vpp_v * APP_PWM_MAX_DUTY) / APP_VPP_MAX_V;
    if (duty_permille > APP_PWM_MAX_DUTY) {
        duty_permille = APP_PWM_MAX_DUTY;
    }

    return (uint16_t)duty_permille;
}

static void APP_ApplyTargetVpp(void)
{
    s_pwm_duty_permille = APP_ConvertVppToDutyPermille(s_target_vpp_v);

#if !APP_SAFE_DISPLAY_ONLY
    BSP_PWM_SetDutyPermille(s_pwm_duty_permille);
#endif
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

static void APP_DrawTitle(void)
{
    uint8_t i;

    OLED_ClearLine(0U);
    OLED_ClearLine(1U);
    OLED_ShowString(0U, 0U, "WAVE");
    OLED_ShowString(0U, 1U, "GEN");

    for (i = 0U; i < 4U; i++) {
        OLED_ShowChinese((uint8_t)(48U + (i * 16U)), 0U, i);
    }
}

static void APP_SampleMeasurement(void)
{
    uint8_t i;
    uint16_t adc_mv;
    int16_t input_mv;
    int16_t min_mv;
    int16_t max_mv;

    if (BSP_ADC_IsReady() == 0U) {
        s_adc_mv = 0U;
        s_measured_vpp_mv = 0U;
        return;
    }

    adc_mv = BSP_ADC_ReadMilliVoltSafe(0U);
    input_mv = BSP_ADC_ConvertToInputSignedMilliVolt(adc_mv);
    min_mv = input_mv;
    max_mv = input_mv;

    for (i = 1U; i < APP_ADC_SAMPLE_COUNT; i++) {
        adc_mv = BSP_ADC_ReadMilliVoltSafe(adc_mv);
        input_mv = BSP_ADC_ConvertToInputSignedMilliVolt(adc_mv);

        if (input_mv < min_mv) {
            min_mv = input_mv;
        }

        if (input_mv > max_mv) {
            max_mv = input_mv;
        }

        Delay_us(150U);
    }

    s_adc_mv = adc_mv;
    s_measured_vpp_mv = (uint16_t)(max_mv - min_mv);
}

static void APP_AdjustSelected(int8_t direction)
{
    uint32_t freq_step;

    if (s_selected_item == APP_SELECT_FREQ) {
        freq_step = APP_GetFrequencyStep();

        if (direction > 0) {
            if ((APP_PWM_MAX_FREQ_HZ - s_pwm_freq_hz) < freq_step) {
                s_pwm_freq_hz = APP_PWM_MAX_FREQ_HZ;
            } else {
                s_pwm_freq_hz += freq_step;
            }
        } else {
            if ((s_pwm_freq_hz - APP_PWM_MIN_FREQ_HZ) < freq_step) {
                s_pwm_freq_hz = APP_PWM_MIN_FREQ_HZ;
            } else {
                s_pwm_freq_hz -= freq_step;
            }
        }

        BSP_PWM_SetFrequency(s_pwm_freq_hz);
    } else {
        if (direction > 0) {
            if ((APP_VPP_MAX_V - s_target_vpp_v) < APP_VPP_STEP_V) {
                s_target_vpp_v = APP_VPP_MAX_V;
            } else {
                s_target_vpp_v = (uint8_t)(s_target_vpp_v + APP_VPP_STEP_V);
            }
        } else {
            if ((s_target_vpp_v - APP_VPP_MIN_V) < APP_VPP_STEP_V) {
                s_target_vpp_v = APP_VPP_MIN_V;
            } else {
                s_target_vpp_v = (uint8_t)(s_target_vpp_v - APP_VPP_STEP_V);
            }
        }

        APP_ApplyTargetVpp();
    }
}

static void APP_HandleKeys(uint8_t key_event)
{
    if (key_event != 0U) {
        s_key_press_count++;
    }

    if ((key_event & (BSP_KEY1_EVENT | BSP_KEYA_EVENT)) != 0U) {
        if (s_selected_item == APP_SELECT_FREQ) {
            s_selected_item = APP_SELECT_VPP;
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
#if !APP_SAFE_DISPLAY_ONLY
    uint32_t freq_hz;
#endif
    uint16_t duty_percent;

    APP_DrawTitle();

    if (s_selected_item == APP_SELECT_FREQ) {
        sprintf(line, "SEL F K%03u", (unsigned int)s_key_press_count);
    } else {
        sprintf(line, "SEL V K%03u", (unsigned int)s_key_press_count);
    }
    APP_DisplayLine(2U, line);

    sprintf(line, "FSET %luHZ", (unsigned long)s_pwm_freq_hz);
    APP_DisplayLine(3U, line);

    duty_percent = (uint16_t)((s_pwm_duty_permille + 5U) / 10U);
    sprintf(line, "VSET %02uV D%03u%%", (unsigned int)s_target_vpp_v, (unsigned int)duty_percent);
    APP_DisplayLine(4U, line);

#if APP_SAFE_DISPLAY_ONLY
    APP_DisplayLine(5U, "FIN ----HZ");
    APP_DisplayLine(6U, "VPP --.--V");
#else
    APP_SampleMeasurement();
    BSP_Freq_Task();
    freq_hz = BSP_Freq_GetHz();
    sprintf(line, "FIN %luHZ", (unsigned long)freq_hz);
    APP_DisplayLine(5U, line);

    APP_FormatVoltage(line, "VPP", s_measured_vpp_mv);
    APP_DisplayLine(6U, line);

    APP_FormatVoltage(line, "ADC", s_adc_mv);
    APP_DisplayLine(7U, line);
#endif

#if APP_SAFE_DISPLAY_ONLY
    sprintf(line, "KEY %03u", (unsigned int)s_key_press_count);
    APP_DisplayLine(7U, line);
#endif
}

void APP_Init(void)
{
    Delay_Init();
    OLED_Init();

    OLED_Clear();
    OLED_EntireDisplayOn(1U);
    Delay_ms(500);
    OLED_EntireDisplayOn(0U);
    OLED_Fill(0xFF);
    Delay_ms(300);
    OLED_Clear();
    OLED_ShowString(0U, 0U, "OLED OK");

#if !APP_SAFE_DISPLAY_ONLY
    OLED_ShowString(0U, 1U, "KEY INIT");
    BSP_Key_Init();
    OLED_ShowString(0U, 2U, "ADC INIT");
    BSP_ADC_Init();
    OLED_ShowString(0U, 3U, "PWM INIT");
    BSP_PWM_Init();
    APP_ApplyTargetVpp();
    OLED_ShowString(0U, 4U, "FREQ INIT");
    BSP_Freq_Init();
#endif

#if APP_USE_TFT
    TFT_Init();
    TFT_Clear();
#endif

    APP_ShowRuntimeInfo();
}

void APP_Task(void)
{
#if APP_SAFE_DISPLAY_ONLY
    OLED_EntireDisplayOn(1U);
    Delay_ms(120);
    OLED_EntireDisplayOn(0U);
    OLED_Clear();
    APP_ShowRuntimeInfo();
    Delay_ms(1000);
#else
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
#endif
}

void APP_Run(void)
{
    while (1) {
        APP_Task();
    }
}
