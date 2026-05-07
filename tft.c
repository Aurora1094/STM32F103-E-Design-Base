#include "tft.h"

#include "bsp_gpio.h"
#include "delay.h"
#include "oledfont.h"

#define TFT_SPI                 SPI1
#define TFT_SPI_GPIO_PORT       GPIOA
#define TFT_SPI_SCK_PIN         5U
#define TFT_SPI_MOSI_PIN        7U

#define TFT_CTRL_GPIO_PORT      GPIOB
#define TFT_RES_PIN             5U
#define TFT_DC_PIN              6U
#define TFT_CS_PIN              7U
#define TFT_BLK_PIN             8U
#define TFT_RES_MASK            (1U << TFT_RES_PIN)
#define TFT_DC_MASK             (1U << TFT_DC_PIN)
#define TFT_CS_MASK             (1U << TFT_CS_PIN)
#define TFT_BLK_MASK            (1U << TFT_BLK_PIN)

#define TFT_X_OFFSET            0U
#define TFT_Y_OFFSET            0U
#define TFT_LINE_HEIGHT         16U

#define TFT_RES_HIGH()          BSP_GPIO_Set(TFT_CTRL_GPIO_PORT, TFT_RES_MASK)
#define TFT_RES_LOW()           BSP_GPIO_Reset(TFT_CTRL_GPIO_PORT, TFT_RES_MASK)
#define TFT_DC_HIGH()           BSP_GPIO_Set(TFT_CTRL_GPIO_PORT, TFT_DC_MASK)
#define TFT_DC_LOW()            BSP_GPIO_Reset(TFT_CTRL_GPIO_PORT, TFT_DC_MASK)
#define TFT_CS_HIGH()           BSP_GPIO_Set(TFT_CTRL_GPIO_PORT, TFT_CS_MASK)
#define TFT_CS_LOW()            BSP_GPIO_Reset(TFT_CTRL_GPIO_PORT, TFT_CS_MASK)
#define TFT_BLK_HIGH()          BSP_GPIO_Set(TFT_CTRL_GPIO_PORT, TFT_BLK_MASK)

static void TFT_SPI_SendByte(uint8_t byte)
{
    while ((TFT_SPI->SR & SPI_SR_TXE) == 0U) {
    }

    *(__IO uint8_t *)&TFT_SPI->DR = byte;

    while ((TFT_SPI->SR & SPI_SR_BSY) != 0U) {
    }
}

static void TFT_WriteCommand(uint8_t command)
{
    TFT_CS_LOW();
    TFT_DC_LOW();
    TFT_SPI_SendByte(command);
    TFT_CS_HIGH();
}

static void TFT_WriteData(uint8_t data)
{
    TFT_CS_LOW();
    TFT_DC_HIGH();
    TFT_SPI_SendByte(data);
    TFT_CS_HIGH();
}

static void TFT_WriteData16Repeat(uint16_t color, uint32_t count)
{
    uint8_t high;
    uint8_t low;

    high = (uint8_t)(color >> 8);
    low = (uint8_t)(color & 0xFFU);

    TFT_CS_LOW();
    TFT_DC_HIGH();

    while (count > 0UL) {
        TFT_SPI_SendByte(high);
        TFT_SPI_SendByte(low);
        count--;
    }

    TFT_CS_HIGH();
}

static void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 = (uint16_t)(x0 + TFT_X_OFFSET);
    x1 = (uint16_t)(x1 + TFT_X_OFFSET);
    y0 = (uint16_t)(y0 + TFT_Y_OFFSET);
    y1 = (uint16_t)(y1 + TFT_Y_OFFSET);

    TFT_WriteCommand(0x2AU);
    TFT_WriteData((uint8_t)(x0 >> 8));
    TFT_WriteData((uint8_t)(x0 & 0xFFU));
    TFT_WriteData((uint8_t)(x1 >> 8));
    TFT_WriteData((uint8_t)(x1 & 0xFFU));

    TFT_WriteCommand(0x2BU);
    TFT_WriteData((uint8_t)(y0 >> 8));
    TFT_WriteData((uint8_t)(y0 & 0xFFU));
    TFT_WriteData((uint8_t)(y1 >> 8));
    TFT_WriteData((uint8_t)(y1 & 0xFFU));

    TFT_WriteCommand(0x2CU);
}

static const uint8_t *TFT_FindAsciiData(char ch)
{
    uint8_t i;

    for (i = 0U; i < OLED_ASCII_6X8_COUNT; i++) {
        if (OLED_ASCII_6X8[i].ch == ch) {
            return OLED_ASCII_6X8[i].data;
        }
    }

    return OLED_ASCII_6X8[0].data;
}

static void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT)) {
        return;
    }

    TFT_SetAddressWindow(x, y, x, y);
    TFT_WriteData16Repeat(color, 1UL);
}

void TFT_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_SPI1EN;

    BSP_GPIO_ConfigPin(TFT_SPI_GPIO_PORT, TFT_SPI_SCK_PIN, BSP_GPIO_AF_PP_50MHZ);
    BSP_GPIO_ConfigPin(TFT_SPI_GPIO_PORT, TFT_SPI_MOSI_PIN, BSP_GPIO_AF_PP_50MHZ);
    BSP_GPIO_ConfigPin(TFT_CTRL_GPIO_PORT, TFT_RES_PIN, BSP_GPIO_OUTPUT_PP_50MHZ);
    BSP_GPIO_ConfigPin(TFT_CTRL_GPIO_PORT, TFT_DC_PIN, BSP_GPIO_OUTPUT_PP_50MHZ);
    BSP_GPIO_ConfigPin(TFT_CTRL_GPIO_PORT, TFT_CS_PIN, BSP_GPIO_OUTPUT_PP_50MHZ);
    BSP_GPIO_ConfigPin(TFT_CTRL_GPIO_PORT, TFT_BLK_PIN, BSP_GPIO_OUTPUT_PP_50MHZ);

    TFT_CS_HIGH();
    TFT_DC_HIGH();
    TFT_RES_HIGH();
    TFT_BLK_HIGH();

    TFT_SPI->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_1 | SPI_CR1_SSM | SPI_CR1_SSI |
                   SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    TFT_SPI->CR2 = 0U;
    TFT_SPI->CR1 |= SPI_CR1_SPE;

    TFT_RES_LOW();
    Delay_ms(20);
    TFT_RES_HIGH();
    Delay_ms(120);

    TFT_WriteCommand(0x01U);
    Delay_ms(150);

    TFT_WriteCommand(0x11U);
    Delay_ms(120);

    TFT_WriteCommand(0x3AU);
    TFT_WriteData(0x05U);

    TFT_WriteCommand(0x36U);
    TFT_WriteData(0x00U);

    TFT_WriteCommand(0xB1U);
    TFT_WriteData(0x01U);
    TFT_WriteData(0x2CU);
    TFT_WriteData(0x2DU);

    TFT_WriteCommand(0xB2U);
    TFT_WriteData(0x01U);
    TFT_WriteData(0x2CU);
    TFT_WriteData(0x2DU);

    TFT_WriteCommand(0xB3U);
    TFT_WriteData(0x01U);
    TFT_WriteData(0x2CU);
    TFT_WriteData(0x2DU);
    TFT_WriteData(0x01U);
    TFT_WriteData(0x2CU);
    TFT_WriteData(0x2DU);

    TFT_WriteCommand(0xB4U);
    TFT_WriteData(0x07U);

    TFT_WriteCommand(0xC0U);
    TFT_WriteData(0xA2U);
    TFT_WriteData(0x02U);
    TFT_WriteData(0x84U);

    TFT_WriteCommand(0xC1U);
    TFT_WriteData(0xC5U);

    TFT_WriteCommand(0xC2U);
    TFT_WriteData(0x0AU);
    TFT_WriteData(0x00U);

    TFT_WriteCommand(0xC3U);
    TFT_WriteData(0x8AU);
    TFT_WriteData(0x2AU);

    TFT_WriteCommand(0xC4U);
    TFT_WriteData(0x8AU);
    TFT_WriteData(0xEEU);

    TFT_WriteCommand(0xC5U);
    TFT_WriteData(0x0EU);

    TFT_WriteCommand(0xE0U);
    TFT_WriteData(0x02U);
    TFT_WriteData(0x1CU);
    TFT_WriteData(0x07U);
    TFT_WriteData(0x12U);
    TFT_WriteData(0x37U);
    TFT_WriteData(0x32U);
    TFT_WriteData(0x29U);
    TFT_WriteData(0x2DU);
    TFT_WriteData(0x29U);
    TFT_WriteData(0x25U);
    TFT_WriteData(0x2BU);
    TFT_WriteData(0x39U);
    TFT_WriteData(0x00U);
    TFT_WriteData(0x01U);
    TFT_WriteData(0x03U);
    TFT_WriteData(0x10U);

    TFT_WriteCommand(0xE1U);
    TFT_WriteData(0x03U);
    TFT_WriteData(0x1DU);
    TFT_WriteData(0x07U);
    TFT_WriteData(0x06U);
    TFT_WriteData(0x2EU);
    TFT_WriteData(0x2CU);
    TFT_WriteData(0x29U);
    TFT_WriteData(0x2DU);
    TFT_WriteData(0x2EU);
    TFT_WriteData(0x2EU);
    TFT_WriteData(0x37U);
    TFT_WriteData(0x3FU);
    TFT_WriteData(0x00U);
    TFT_WriteData(0x00U);
    TFT_WriteData(0x02U);
    TFT_WriteData(0x10U);

    TFT_WriteCommand(0x13U);
    TFT_WriteCommand(0x29U);
    Delay_ms(50);

    TFT_Clear();
}

void TFT_Clear(void)
{
    TFT_FillScreen(TFT_BLACK);
}

void TFT_FillScreen(uint16_t color)
{
    TFT_FillRect(0U, 0U, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t x1;
    uint16_t y1;
    uint32_t count;

    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT) || (width == 0U) || (height == 0U)) {
        return;
    }

    if ((x + width) > TFT_WIDTH) {
        width = (uint16_t)(TFT_WIDTH - x);
    }

    if ((y + height) > TFT_HEIGHT) {
        height = (uint16_t)(TFT_HEIGHT - y);
    }

    x1 = (uint16_t)(x + width - 1U);
    y1 = (uint16_t)(y + height - 1U);
    count = (uint32_t)width * (uint32_t)height;

    TFT_SetAddressWindow(x, y, x1, y1);
    TFT_WriteData16Repeat(color, count);
}

void TFT_ClearLine(uint8_t line)
{
    uint16_t y;

    y = (uint16_t)line * TFT_LINE_HEIGHT;
    TFT_FillRect(0U, y, TFT_WIDTH, TFT_LINE_HEIGHT, TFT_BLACK);
}

void TFT_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg, uint8_t scale)
{
    const uint8_t *data;
    uint8_t col;
    uint8_t row;
    uint8_t bits;
    uint16_t draw_color;
    uint16_t px;
    uint16_t py;

    if (scale == 0U) {
        scale = 1U;
    }

    data = TFT_FindAsciiData(ch);

    for (col = 0U; col < 6U; col++) {
        bits = data[col];
        for (row = 0U; row < 8U; row++) {
            if ((bits & (uint8_t)(1U << row)) != 0U) {
                draw_color = color;
            } else {
                draw_color = bg;
            }

            px = (uint16_t)(x + ((uint16_t)col * scale));
            py = (uint16_t)(y + ((uint16_t)row * scale));

            if (scale == 1U) {
                TFT_DrawPixel(px, py, draw_color);
            } else {
                TFT_FillRect(px, py, scale, scale, draw_color);
            }
        }
    }
}

void TFT_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale)
{
    uint16_t step;

    if (scale == 0U) {
        scale = 1U;
    }

    step = (uint16_t)(6U * scale);

    while ((*str != '\0') && ((x + step) <= TFT_WIDTH)) {
        TFT_ShowChar(x, y, *str, color, bg, scale);
        x = (uint16_t)(x + step);
        str++;
    }
}
