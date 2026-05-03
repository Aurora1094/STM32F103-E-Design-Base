# STM32F103C8T6 最小系统测试工程代码

这套文件用于 Keil5 + STM32 标准外设库工程，主控为 STM32F103C8T6，不使用 HAL。

## 文件作用

- `main.c`：程序入口，调用应用初始化和主循环。
- `app.c` / `app.h`：应用层逻辑，负责按键计数、ADC 采样和 OLED 内容刷新。
- `bsp_key.c` / `bsp_key.h`：PB13/PB14/PB15 按键驱动，上拉输入，低电平按下，20ms 消抖，单次按下只返回一次事件。
- `bsp_adc.c` / `bsp_adc.h`：ADC1_IN3，也就是 PA3 电压采样，按 0~3.3V 换算毫伏值。
- `oled.c` / `oled.h` / `oledfont.h`：SSD1306 128x64 OLED 驱动，PB10/PB11 软件 I2C，内置 “南开电赛” 16x16 点阵和本例所需 ASCII 点阵。
- `delay.c` / `delay.h`：基于 SysTick 的阻塞延时，供软件 I2C 和按键消抖使用。

## 接线说明

| 模块 | 信号 | STM32F103C8T6 引脚 | 说明 |
| --- | --- | --- | --- |
| OLED SSD1306 | SCL | PB10 | 软件 I2C 时钟 |
| OLED SSD1306 | SDA | PB11 | 软件 I2C 数据 |
| OLED SSD1306 | VCC | 3.3V | 建议使用 3.3V 供电 |
| OLED SSD1306 | GND | GND | 共地 |
| KEY1 | GPIO | PB13 | 按键另一端接 GND，内部上拉，按下为低 |
| KEY2 | GPIO | PB14 | 按键另一端接 GND，内部上拉，按下为低 |
| KEY3 | GPIO | PB15 | 按键另一端接 GND，内部上拉，按下为低 |
| ADC 输入 | ADC1_IN3 | PA3 | 采集 0~3.3V 直流电压，禁止超过 VDD |

如果 OLED 模块本身没有 I2C 上拉电阻，建议在 SCL、SDA 上外接 4.7kΩ 上拉到 3.3V。

## OLED 显示布局

- 第 1 行：南开电赛
- 第 2 行：`KEY1: 计数`
- 第 3 行：`KEY2: 计数`
- 第 4 行：`KEY3: 计数`
- 第 5 行：`ADC: x.xxV`

## Keil5 标准库工程使用说明

1. 新建或打开 STM32F103C8T6 标准外设库工程。
2. 将本目录下的 `.c` 文件加入 Keil 工程的用户代码分组。
3. 将本目录加入 Include Paths。
4. 工程中需要已有 CMSIS 和 STM32F10x 标准外设库，并确保加入这些标准库源文件：
   - `stm32f10x_gpio.c`
   - `stm32f10x_rcc.c`
   - `stm32f10x_adc.c`
   - `system_stm32f10x.c`
5. 工程宏定义建议包含：
   - `USE_STDPERIPH_DRIVER`
   - `STM32F10X_MD`
6. 使用中密度启动文件：
   - `startup_stm32f10x_md.s`

## 预留接口

- `PA2`：预留为 `PWM_OUT`，当前代码未初始化，后续可用 TIM2_CH3 或其他方案实现 PWM 输出。
- `PA6`：预留为 `FREQ_IN`，当前代码未初始化，后续可用 TIM3_CH1 输入捕获或计数模式实现频率测量。

这两个引脚目前不要接入会影响启动或下载的外设，后续扩展时再在对应 BSP 模块中初始化。
