# IO 引脚分配表

> STM32F103C6T6 | 更新于 2026-07-11

## 完整引脚表

| 引脚 | 最初 C8T6 | 当前 C6T6 | 变动 |
|---|---|---|---|
| PA0 | RC 输入 CH1 (TIM2_CH1) | — 同 — | |
| PA1 | RC 输入 CH2 (TIM2_CH2) | — 同 — | |
| PA2 | RC 输入 CH3 (TIM2_CH3) | — 同 — | |
| PA3 | RC 输入 CH4 (TIM2_CH4) | — 同 — | |
| PA4 | 空闲 | 空闲 | |
| PA5 | 空闲 | 空闲 | |
| PA6 | RC 输入 CH5 (TIM3_CH1) | — 同 — | |
| PA7 | RC 输入 CH6 (TIM3_CH2) | — 同 — | |
| **PA8** | 空闲 | **PWM 输出 CH1** | C6 迁移：TIM4→TIM1 |
| **PA9** | 空闲 | **PWM 输出 CH2** | C6 迁移：TIM4→TIM1 |
| **PA10** | 空闲 | **PWM 输出 CH3** | C6 迁移：TIM4→TIM1 |
| **PA11** | 空闲 | **PWM 输出 CH4** | C6 迁移：TIM4→TIM1 |
| PA12 | 空闲 | 空闲 | |
| PA13 | SWDIO | SWDIO | |
| PA14 | SWCLK | SWCLK | |
| PA15 | 数字输出 CH5 | 数字输出 CH1 | 编码器占用 PB12~15，数字通道重编号 |
| PB0 | RC 输入 CH7 (TIM3_CH3) | — 同 — | |
| PB1 | RC 输入 CH8 (TIM3_CH4) | — 同 — | |
| PB2 | BOOT1 | BOOT1 | |
| PB3 | 数字输出 CH6 | 数字输出 CH2 | 重编号 |
| PB4 | 数字输出 CH7 | 数字输出 CH3 | 重编号 |
| PB5 | 数字输出 CH8 | 数字输出 CH4 | 重编号 |
| **PB6** | **PWM 输出 CH1** (TIM4) | **I2C1 SCL** | C6 迁移：I2C2→I2C1 |
| **PB7** | **PWM 输出 CH2** (TIM4) | **I2C1 SDA** | C6 迁移：I2C2→I2C1 |
| **PB8** | **PWM 输出 CH3** (TIM4) | **空闲** | C6 迁移：PWM 迁走 |
| **PB9** | **PWM 输出 CH4** (TIM4) | **空闲** | C6 迁移：PWM 迁走 |
| **PB10** | **I2C2 SCL** | **空闲** | C6 迁移：I2C 迁走 |
| **PB11** | **I2C2 SDA** | **空闲** | C6 迁移：I2C 迁走 |
| **PB12** | **数字输出 CH1** | **编码器 左A** | 编码器占用 |
| **PB13** | **数字输出 CH2** | **编码器 左B** | 编码器占用 |
| **PB14** | **数字输出 CH3** | **编码器 右A** | 编码器占用 |
| **PB15** | **数字输出 CH4** | **编码器 右B** | 编码器占用 |
| PC13 | LED | LED | |

## 功能分组

### RC 接收机输入 (8ch)
| 通道 | 引脚 | 定时器 |
|---|---|---|
| CH1 | PA0 | TIM2_CH1 |
| CH2 | PA1 | TIM2_CH2 |
| CH3 | PA2 | TIM2_CH3 |
| CH4 | PA3 | TIM2_CH4 |
| CH5 | PA6 | TIM3_CH1 |
| CH6 | PA7 | TIM3_CH2 |
| CH7 | PB0 | TIM3_CH3 |
| CH8 | PB1 | TIM3_CH4 |

### PWM 电机输出 (4ch, 1.8kHz)
| 通道 | 引脚 | 定时器 | 功能 |
|---|---|---|---|
| CH1 | PA8 | TIM1_CH1 | 左电机反转 |
| CH2 | PA9 | TIM1_CH2 | 左电机正转 |
| CH3 | PA10 | TIM1_CH3 | 右电机反转 |
| CH4 | PA11 | TIM1_CH4 | 右电机正转 |

### 数字输出 (4ch)
| 通道 | 引脚 |
|---|---|
| CH1 | PA15 |
| CH2 | PB3 |
| CH3 | PB4 |
| CH4 | PB5 |

### 编码器测速 (2 电机, 正交解码)
| 信号 | 引脚 | 说明 |
|---|---|---|
| 左 A | PB12 | EXTI12 |
| 左 B | PB13 | EXTI13 |
| 右 A | PB14 | EXTI14 |
| 右 B | PB15 | EXTI15 |

### I2C OLED
| 信号 | 引脚 |
|---|---|
| SCL | PB6 (I2C1) |
| SDA | PB7 (I2C1) |

### 其他
| 功能 | 引脚 |
|---|---|
| LED | PC13 |
| SWDIO | PA13 |
| SWCLK | PA14 |
| BOOT1 | PB2 |

## 空闲引脚

PA4, PA5, PA12, PB8, PB9, PB10, PB11 (共 7 个)

## 变动原因

1. **STM32F103C8T6 → C6T6**：C6T6 无 TIM4、I2C2，PWM 迁至 TIM1 (PA8~11)，I2C 迁至 I2C1 (PB6/PB7)
2. **编码器测速**：PB12~PB15 由数字输出改为正交编码器输入 (EXTI 软件解码)
3. **数字输出缩减**：8ch → 4ch (PA15, PB3~PB5)，通道重新编号
