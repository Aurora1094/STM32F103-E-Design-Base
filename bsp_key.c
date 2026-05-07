#include "bsp_key.h"

#include "bsp_gpio.h"
#include "delay.h"

#define KEY_GPIO_PORT     GPIOB
#define KEY1_GPIO_PIN     13U
#define KEY2_GPIO_PIN     14U
#define KEY3_GPIO_PIN     15U
#define KEYA_GPIO_PIN     4U
#define KEYB_GPIO_PIN     3U
#define KEYD_GPIO_PIN     9U

#define KEY1_GPIO_MASK    (1U << KEY1_GPIO_PIN)
#define KEY2_GPIO_MASK    (1U << KEY2_GPIO_PIN)
#define KEY3_GPIO_MASK    (1U << KEY3_GPIO_PIN)
#define KEYA_GPIO_MASK    (1U << KEYA_GPIO_PIN)
#define KEYB_GPIO_MASK    (1U << KEYB_GPIO_PIN)
#define KEYD_GPIO_MASK    (1U << KEYD_GPIO_PIN)

static uint8_t BSP_Key_ReadPressedMask(void)
{
    uint8_t mask;

    mask = 0U;

    if (BSP_GPIO_Read(KEY_GPIO_PORT, KEY1_GPIO_MASK) == 0U) {
        mask |= BSP_KEY1_EVENT;
    }

    if (BSP_GPIO_Read(KEY_GPIO_PORT, KEY2_GPIO_MASK) == 0U) {
        mask |= BSP_KEY2_EVENT;
    }

    if (BSP_GPIO_Read(KEY_GPIO_PORT, KEY3_GPIO_MASK) == 0U) {
        mask |= BSP_KEY3_EVENT;
    }

    if (BSP_GPIO_Read(KEY_GPIO_PORT, KEYA_GPIO_MASK) == 0U) {
        mask |= BSP_KEYA_EVENT;
    }

    if (BSP_GPIO_Read(KEY_GPIO_PORT, KEYB_GPIO_MASK) == 0U) {
        mask |= BSP_KEYB_EVENT;
    }

    if (BSP_GPIO_Read(KEY_GPIO_PORT, KEYD_GPIO_MASK) == 0U) {
        mask |= BSP_KEYD_EVENT;
    }

    return mask;
}

void BSP_Key_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

    BSP_GPIO_ConfigPin(KEY_GPIO_PORT, KEY1_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    BSP_GPIO_ConfigPin(KEY_GPIO_PORT, KEY2_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    BSP_GPIO_ConfigPin(KEY_GPIO_PORT, KEY3_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    BSP_GPIO_ConfigPin(KEY_GPIO_PORT, KEYA_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    BSP_GPIO_ConfigPin(KEY_GPIO_PORT, KEYB_GPIO_PIN, BSP_GPIO_PULL_INPUT);
    BSP_GPIO_ConfigPin(KEY_GPIO_PORT, KEYD_GPIO_PIN, BSP_GPIO_PULL_INPUT);

    KEY_GPIO_PORT->ODR |= KEY1_GPIO_MASK | KEY2_GPIO_MASK | KEY3_GPIO_MASK |
                          KEYA_GPIO_MASK | KEYB_GPIO_MASK | KEYD_GPIO_MASK;
}

uint8_t BSP_Key_Scan(void)
{
    static uint8_t last_stable_mask = 0U;
    static uint8_t pressed_lock = 0U;
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
