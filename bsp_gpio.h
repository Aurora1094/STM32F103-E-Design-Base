#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "stm32f1xx.h"

#define BSP_GPIO_ANALOG_INPUT        0x0U
#define BSP_GPIO_FLOATING_INPUT      0x4U
#define BSP_GPIO_PULL_INPUT          0x8U
#define BSP_GPIO_OUTPUT_PP_50MHZ     0x3U
#define BSP_GPIO_OUTPUT_OD_50MHZ     0x7U
#define BSP_GPIO_AF_PP_50MHZ         0xBU

static inline void BSP_GPIO_ConfigPin(GPIO_TypeDef *port, uint8_t pin, uint32_t mode_cnf)
{
    volatile uint32_t *config_reg;
    uint32_t shift;

    if (pin < 8U) {
        config_reg = &port->CRL;
        shift = (uint32_t)pin * 4UL;
    } else {
        config_reg = &port->CRH;
        shift = ((uint32_t)pin - 8UL) * 4UL;
    }

    *config_reg = (*config_reg & ~(0xFUL << shift)) | ((mode_cnf & 0xFUL) << shift);
}

static inline void BSP_GPIO_Set(GPIO_TypeDef *port, uint16_t pin_mask)
{
    port->BSRR = pin_mask;
}

static inline void BSP_GPIO_Reset(GPIO_TypeDef *port, uint16_t pin_mask)
{
    port->BRR = pin_mask;
}

static inline uint8_t BSP_GPIO_Read(GPIO_TypeDef *port, uint16_t pin_mask)
{
    return ((port->IDR & pin_mask) != 0U) ? 1U : 0U;
}

#endif
