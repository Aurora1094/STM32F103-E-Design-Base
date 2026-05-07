# STM32F103C8T6 波形信号发生器测试代码

这套源码按当前原理图整理，用于 STM32F103C8T6 工程。底层只依赖 CMSIS 设备头文件 `stm32f1xx.h`，不再依赖 `stm32f10x.h` 或 STM32F10x 标准外设库。当前代码主要用于焊板后的联调：PWM 输出、频率输入测量、ADC 前端电压换算、按键调参，并且 OLED/TFT 双屏同步显示。

## 已实现功能

- `PA2 / TIM2_CH3`：PWM 输出，默认 1 kHz、50% 占空比。
- `PA6 / TIM3_CH1`：比较器输出频率测量，输入捕获方式，界面显示 Hz。
- `PA3 / ADC1_IN3`：ADC 采样，同时按原理图 `Uadc=(5-Vin)/2` 换算 `Vin`。
- `PB13/PB14/PB15`：三颗独立按键，低电平按下。
- `PB4/PB3/PB9`：五向开关预留按键，低电平按下；初始化时关闭 JTAG、保留 SWD。
- `PB10/PB11`：SSD1306 OLED 软件 I2C。
- `PA5/PA7 + PB5/PB6/PB7/PB8`：ST7735 1.8 寸 TFT SPI 屏。

## 界面和按键

OLED 和 TFT 显示同一组信息：

- `WAVE GEN`
- `SEL FREQ` / `SEL DUTY`
- `PWM xxxxHZ`
- `DUTY xxx%`
- `FIN xxxxHZ`
- `ADC x.xxV`
- `VIN x.xxV`
- `K1SEL K2+ K3-`

按键逻辑：

- `KEY1(PB13)` 或 `KEYA(PB4)`：切换调节项，频率/占空比。
- `KEY2(PB14)` 或 `KEYB(PB3)`：增加当前调节项。
- `KEY3(PB15)` 或 `KEYD(PB9)`：减少当前调节项。

PWM 频率范围为 1 Hz 到 200 kHz。占空比步进为 5%。

## Keil 工程需要加入的用户源码

- `main.c`
- `app.c`
- `delay.c`
- `bsp_adc.c`
- `bsp_key.c`
- `bsp_gpio.h`
- `bsp_pwm.c`
- `bsp_freq.c`
- `oled.c`
- `tft.c`

头文件目录加入本目录即可。

## include 要求

CubeIDE/CubeMX 工程一般已经具备这些 include path：

- `Core/Inc`
- `Drivers/CMSIS/Include`
- `Drivers/CMSIS/Device/ST/STM32F1xx/Include`

需要能 include 到：

- `stm32f1xx.h`
- `core_cm3.h`
- `system_stm32f1xx.c`
- 对应芯片启动文件，例如 `startup_stm32f103xb.s`

工程宏定义需要包含你的具体芯片型号。STM32F103C8T6 通常使用：

- `STM32F103xB`

如果你的工程是 CubeMX 生成的，这些通常已经配置好了。

## Cube 工程注意事项

本代码的微秒延时使用 Cortex-M3 的 DWT 计数器，不会占用 SysTick。

`bsp_freq.c` 里提供了弱定义的 `TIM3_IRQHandler`。如果你的 `Core/Src/stm32f1xx_it.c` 里已经有 `TIM3_IRQHandler`，请在那个函数里调用：

```c
BSP_Freq_TIM3_IRQHandler();
```

## 上板测试建议

先不接外部模拟输入时，确认双屏能亮、PA2 有默认 1 kHz PWM。再把 PA2 的 PWM 临时跳到 PA6 的频率输入，界面上的 `FIN` 应接近 PWM 设置值。模拟前端调试时，`ADC` 是 PA3 实测电压，`VIN` 是按 `5V - 2*ADC` 换算出来的原输入电压。
