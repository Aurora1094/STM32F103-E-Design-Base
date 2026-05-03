#include "bsp_key.h"

#include "delay.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define KEY_GPIO_PORT     GPIOB
#define KEY_GPIO_RCC      RCC_APB2Periph_GPIOB
#define KEY1_GPIO_PIN     GPIO_Pin_13
#define KEY2_GPIO_PIN     GPIO_Pin_14
#define KEY3_GPIO_PIN     GPIO_Pin_15

static uint8_t BSP_Key_ReadPressedMask(void)
{
    uint8_t mask;

    mask = 0;

    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY1_GPIO_PIN) == Bit_RESET) {
        mask |= BSP_KEY1_EVENT;
    }

    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY2_GPIO_PIN) == Bit_RESET) {
        mask |= BSP_KEY2_EVENT;
    }

    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY3_GPIO_PIN) == Bit_RESET) {
        mask |= BSP_KEY3_EVENT;
    }

    return mask;
}

void BSP_Key_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(KEY_GPIO_RCC, ENABLE);

    gpio.GPIO_Pin = KEY1_GPIO_PIN | KEY2_GPIO_PIN | KEY3_GPIO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT, &gpio);
}

uint8_t BSP_Key_Scan(void)
{
    static uint8_t last_stable_mask = 0;
    static uint8_t pressed_lock = 0;
    uint8_t raw_mask;
    uint8_t stable_mask;
    uint8_t event_mask;

    raw_mask = BSP_Key_ReadPressedMask();
    stable_mask = raw_mask;

    if (raw_mask != last_stable_mask) {
        Delay_ms(20);
        stable_mask = BSP_Key_ReadPressedMask();
        last_stable_mask = stable_mask;
    }

    event_mask = (uint8_t)(stable_mask & (uint8_t)(~pressed_lock));
    pressed_lock = stable_mask;

    return event_mask;
}
