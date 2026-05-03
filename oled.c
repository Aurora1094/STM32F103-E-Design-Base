#include "oled.h"

#include "delay.h"
#include "oledfont.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define OLED_I2C_GPIO_PORT      GPIOB
#define OLED_I2C_GPIO_RCC       RCC_APB2Periph_GPIOB
#define OLED_I2C_SCL_PIN        GPIO_Pin_10
#define OLED_I2C_SDA_PIN        GPIO_Pin_11
#define OLED_I2C_ADDR           0x78U

#define OLED_SCL_HIGH()         GPIO_SetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN)
#define OLED_SCL_LOW()          GPIO_ResetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN)
#define OLED_SDA_HIGH()         GPIO_SetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN)
#define OLED_SDA_LOW()          GPIO_ResetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN)

static void OLED_I2C_Delay(void)
{
    Delay_us(2);
}

static void OLED_I2C_Start(void)
{
    OLED_SDA_HIGH();
    OLED_SCL_HIGH();
    OLED_I2C_Delay();
    OLED_SDA_LOW();
    OLED_I2C_Delay();
    OLED_SCL_LOW();
}

static void OLED_I2C_Stop(void)
{
    OLED_SDA_LOW();
    OLED_SCL_HIGH();
    OLED_I2C_Delay();
    OLED_SDA_HIGH();
    OLED_I2C_Delay();
}

static void OLED_I2C_WaitAck(void)
{
    uint8_t timeout;

    timeout = 0;
    OLED_SDA_HIGH();
    OLED_I2C_Delay();
    OLED_SCL_HIGH();
    OLED_I2C_Delay();

    while (GPIO_ReadInputDataBit(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN) == Bit_SET) {
        timeout++;
        if (timeout > 250U) {
            break;
        }
    }

    OLED_SCL_LOW();
    OLED_I2C_Delay();
}

static void OLED_I2C_SendByte(uint8_t byte)
{
    uint8_t i;

    for (i = 0; i < 8U; i++) {
        if ((byte & 0x80U) != 0U) {
            OLED_SDA_HIGH();
        } else {
            OLED_SDA_LOW();
        }

        OLED_I2C_Delay();
        OLED_SCL_HIGH();
        OLED_I2C_Delay();
        OLED_SCL_LOW();
        byte <<= 1;
    }

    OLED_I2C_WaitAck();
}

static void OLED_WriteByte(uint8_t byte, uint8_t control)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_I2C_ADDR);
    OLED_I2C_SendByte(control);
    OLED_I2C_SendByte(byte);
    OLED_I2C_Stop();
}

static void OLED_WriteCommand(uint8_t command)
{
    OLED_WriteByte(command, 0x00U);
}

static void OLED_WriteDataStream(const uint8_t *data, uint16_t length)
{
    uint16_t i;

    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_I2C_ADDR);
    OLED_I2C_SendByte(0x40U);

    for (i = 0; i < length; i++) {
        OLED_I2C_SendByte(data[i]);
    }

    OLED_I2C_Stop();
}

static void OLED_WriteRepeatData(uint8_t data, uint16_t length)
{
    uint16_t i;

    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_I2C_ADDR);
    OLED_I2C_SendByte(0x40U);

    for (i = 0; i < length; i++) {
        OLED_I2C_SendByte(data);
    }

    OLED_I2C_Stop();
}

static void OLED_SetPos(uint8_t x, uint8_t page)
{
    OLED_WriteCommand((uint8_t)(0xB0U + page));
    OLED_WriteCommand((uint8_t)(0x10U | ((x & 0xF0U) >> 4U)));
    OLED_WriteCommand((uint8_t)(x & 0x0FU));
}

static const uint8_t *OLED_FindAsciiData(char ch)
{
    uint8_t i;

    for (i = 0; i < OLED_ASCII_6X8_COUNT; i++) {
        if (OLED_ASCII_6X8[i].ch == ch) {
            return OLED_ASCII_6X8[i].data;
        }
    }

    return OLED_ASCII_6X8[0].data;
}

void OLED_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(OLED_I2C_GPIO_RCC, ENABLE);

    gpio.GPIO_Pin = OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_I2C_GPIO_PORT, &gpio);

    OLED_SCL_HIGH();
    OLED_SDA_HIGH();

    Delay_ms(100);

    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0x20);
    OLED_WriteCommand(0x02);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x10);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);
}

void OLED_Clear(void)
{
    uint8_t page;

    for (page = 0; page < 8U; page++) {
        OLED_SetPos(0, page);
        OLED_WriteRepeatData(0x00, OLED_WIDTH);
    }
}

void OLED_ClearLine(uint8_t page)
{
    if (page >= 8U) {
        return;
    }

    OLED_SetPos(0, page);
    OLED_WriteRepeatData(0x00, OLED_WIDTH);
}

void OLED_ShowChar(uint8_t x, uint8_t page, char ch)
{
    const uint8_t *data;

    if ((x > (OLED_WIDTH - 6U)) || (page >= 8U)) {
        return;
    }

    data = OLED_FindAsciiData(ch);
    OLED_SetPos(x, page);
    OLED_WriteDataStream(data, 6U);
}

void OLED_ShowString(uint8_t x, uint8_t page, const char *str)
{
    while ((*str != '\0') && (x <= (OLED_WIDTH - 6U)) && (page < 8U)) {
        OLED_ShowChar(x, page, *str);
        x = (uint8_t)(x + 6U);
        str++;
    }
}

void OLED_ShowChinese(uint8_t x, uint8_t page, uint8_t index)
{
    if ((index >= OLED_CHINESE_16X16_COUNT) || (x > (OLED_WIDTH - 16U)) || (page > 6U)) {
        return;
    }

    OLED_SetPos(x, page);
    OLED_WriteDataStream(&OLED_CHINESE_16X16[index].data[0], 16U);

    OLED_SetPos(x, (uint8_t)(page + 1U));
    OLED_WriteDataStream(&OLED_CHINESE_16X16[index].data[16], 16U);
}
