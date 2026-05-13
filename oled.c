#include "oled.h"

#include "bsp_gpio.h"
#include "delay.h"
#include "oledfont.h"

#define OLED_I2C_GPIO_PORT          GPIOB
#define OLED_I2C_DEFAULT_SCL_PIN    10U
#define OLED_I2C_DEFAULT_SDA_PIN    11U
#define OLED_I2C_ADDR_3C        0x78U
#define OLED_I2C_ADDR_3D        0x7AU

#define OLED_SCL_HIGH()         OLED_I2C_ReleaseScl()
#define OLED_SCL_LOW()          OLED_I2C_PullSclLow()
#define OLED_SDA_HIGH()         OLED_I2C_ReleaseSda()
#define OLED_SDA_LOW()          OLED_I2C_PullSdaLow()

static uint8_t s_oled_i2c_addr = OLED_I2C_ADDR_3C;
static GPIO_TypeDef *s_oled_i2c_port = OLED_I2C_GPIO_PORT;
static uint8_t s_oled_i2c_scl_pin = OLED_I2C_DEFAULT_SCL_PIN;
static uint8_t s_oled_i2c_sda_pin = OLED_I2C_DEFAULT_SDA_PIN;
static uint16_t s_oled_i2c_scl_mask = (uint16_t)(1U << OLED_I2C_DEFAULT_SCL_PIN);
static uint16_t s_oled_i2c_sda_mask = (uint16_t)(1U << OLED_I2C_DEFAULT_SDA_PIN);

static void OLED_I2C_ReleaseScl(void);
static void OLED_I2C_PullSclLow(void);
static void OLED_I2C_ReleaseSda(void);
static void OLED_I2C_PullSdaLow(void);

static void OLED_I2C_SelectPins(uint8_t scl_pin, uint8_t sda_pin)
{
    s_oled_i2c_port = OLED_I2C_GPIO_PORT;
    s_oled_i2c_scl_pin = scl_pin;
    s_oled_i2c_sda_pin = sda_pin;
    s_oled_i2c_scl_mask = (uint16_t)(1U << scl_pin);
    s_oled_i2c_sda_mask = (uint16_t)(1U << sda_pin);
}

static void OLED_I2C_ReleaseScl(void)
{
    BSP_GPIO_Set(s_oled_i2c_port, s_oled_i2c_scl_mask);
    BSP_GPIO_ConfigPin(s_oled_i2c_port, s_oled_i2c_scl_pin, BSP_GPIO_PULL_INPUT);
}

static void OLED_I2C_PullSclLow(void)
{
    BSP_GPIO_Reset(s_oled_i2c_port, s_oled_i2c_scl_mask);
    BSP_GPIO_ConfigPin(s_oled_i2c_port, s_oled_i2c_scl_pin, BSP_GPIO_OUTPUT_OD_50MHZ);
    BSP_GPIO_Reset(s_oled_i2c_port, s_oled_i2c_scl_mask);
}

static void OLED_I2C_ReleaseSda(void)
{
    BSP_GPIO_Set(s_oled_i2c_port, s_oled_i2c_sda_mask);
    BSP_GPIO_ConfigPin(s_oled_i2c_port, s_oled_i2c_sda_pin, BSP_GPIO_PULL_INPUT);
}

static void OLED_I2C_PullSdaLow(void)
{
    BSP_GPIO_Reset(s_oled_i2c_port, s_oled_i2c_sda_mask);
    BSP_GPIO_ConfigPin(s_oled_i2c_port, s_oled_i2c_sda_pin, BSP_GPIO_OUTPUT_OD_50MHZ);
    BSP_GPIO_Reset(s_oled_i2c_port, s_oled_i2c_sda_mask);
}

static void OLED_I2C_ConfigPins(void)
{
    OLED_SCL_HIGH();
    OLED_SDA_HIGH();
}

static void OLED_I2C_Delay(void)
{
    Delay_us(10);
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

static uint8_t OLED_I2C_WaitAck(void)
{
    uint8_t timeout;
    uint8_t ack;

    timeout = 0;
    OLED_SDA_HIGH();
    OLED_I2C_Delay();
    OLED_SCL_HIGH();
    OLED_I2C_Delay();

    while (BSP_GPIO_Read(s_oled_i2c_port, s_oled_i2c_sda_mask) != 0U) {
        timeout++;
        if (timeout > 250U) {
            break;
        }
        OLED_I2C_Delay();
    }

    ack = (BSP_GPIO_Read(s_oled_i2c_port, s_oled_i2c_sda_mask) == 0U) ? 1U : 0U;
    OLED_SCL_LOW();
    OLED_I2C_Delay();

    return ack;
}

static uint8_t OLED_I2C_SendByte(uint8_t byte)
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

    return OLED_I2C_WaitAck();
}

static uint8_t OLED_I2C_CheckAddress(uint8_t address)
{
    uint8_t ack;

    OLED_I2C_Start();
    ack = OLED_I2C_SendByte(address);
    OLED_I2C_Stop();

    return ack;
}

static uint8_t OLED_I2C_DetectBus(void)
{
    OLED_I2C_SelectPins(OLED_I2C_DEFAULT_SCL_PIN, OLED_I2C_DEFAULT_SDA_PIN);
    OLED_I2C_ConfigPins();

    if (OLED_I2C_CheckAddress(OLED_I2C_ADDR_3C) != 0U) {
        s_oled_i2c_addr = OLED_I2C_ADDR_3C;
        return 1U;
    }

    if (OLED_I2C_CheckAddress(OLED_I2C_ADDR_3D) != 0U) {
        s_oled_i2c_addr = OLED_I2C_ADDR_3D;
        return 1U;
    }

    s_oled_i2c_addr = OLED_I2C_ADDR_3C;
    return 0U;
}

static void OLED_WriteByte(uint8_t byte, uint8_t control)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(s_oled_i2c_addr);
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
    OLED_I2C_SendByte(s_oled_i2c_addr);
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
    OLED_I2C_SendByte(s_oled_i2c_addr);
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
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    Delay_ms(100);
    (void)OLED_I2C_DetectBus();

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
    OLED_Fill(0x00);
}

void OLED_Fill(uint8_t data)
{
    uint8_t page;

    for (page = 0; page < 8U; page++) {
        OLED_SetPos(0, page);
        OLED_WriteRepeatData(data, OLED_WIDTH);
    }
}

void OLED_EntireDisplayOn(uint8_t enable)
{
    OLED_WriteCommand((enable != 0U) ? 0xA5U : 0xA4U);
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
