# PWMSwitch 项目设计文档

> STM32F103C8T6 平台 — 8通道RC接收机PWM输入 → 坦克混控 → 4路PWM输出 + 4路数字输出 + OLED显示

---

## 1. 项目概述

- **MCU**: STM32F103C8T6 (Blue Pill), HSI 8MHz
- **框架**: STM32Cube HAL (通过 PlatformIO)
- **功能**: 读取RC接收机8路PWM信号 → 应用坦克混控算法 → 输出4路PWM驱动电机 + 4路数字开关量 → 实时显示在128×64 OLED上
- **编程语言**: C (C99)

---

## 2. 代码架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         PWMSwitch 层次架构                               │
│                      STM32F103C8T6 (Blue Pill)                          │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                        main.c (主循环)                           │   │
│  │  ┌──────────────────────────────────────────────────────────┐   │   │
│  │  │  main():                                                  │   │   │
│  │  │    1. SystemCoreClock = 8MHz                              │   │   │
│  │  │    2. HAL_Init()                                          │   │   │
│  │  │    3. GPIO_Init()        ← PC13 LED                       │   │   │
│  │  │    4. Display_Init()     ← SSD1306 OLED                   │   │   │
│  │  │    5. Control_Init()     ← 初始化所有子系统                │   │   │
│  │  │    ┌─────────────────────┐                                 │   │   │
│  │  │    │ while(1) 主循环     │                                 │   │   │
│  │  │    │  Control_Update()   │ ← 周期执行 (10ms)               │   │   │
│  │  │    │  LED心跳指示        │ ← 根据油门大小闪烁               │   │   │
│  │  │    │  HAL_Delay(10)      │                                 │   │   │
│  │  │    └─────────────────────┘                                 │   │   │
│  │  └──────────────────────────────────────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                    │                                     │
│                                    ▼                                     │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                     control.c (控制逻辑)                          │   │
│  │                                                                   │   │
│  │  Control_Init() → 调用各模块的 Init                               │   │
│  │  Control_Update() → 一次完整的 读取→运算→写入 周期                │   │
│  │                                                                   │   │
│  │  ┌──────────────────────────────────────────────────────────────┐ │   │
│  │  │ Control_Update() 内部流程:                                   │ │   │
│  │  │  ① PWM_Input_GetPercent() × 8   ← 读取8路输入 (0~100%)      │ │   │
│  │  │  ② 坦克混控算法 (thr+steer)      ← 核心转换逻辑              │ │   │
│  │  │  ③ PWM_Output_Set() × 4        ← 写入PWM输出 (0~100%)       │ │   │
│  │  │  ④ Digital_Output_Set() × 4    ← 写入数字输出 (0/1)          │ │   │
│  │  │  ⑤ Display_Update()            ← 刷新OLED显示                │ │   │
│  │  └──────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│              │            │              │              │               │
│              ▼            ▼              ▼              ▼               │
│  ┌──────────────┐ ┌────────────┐ ┌──────────────┐ ┌──────────────┐    │
│  │  pwm_input   │ │ pwm_output │ │digital_output│ │   display    │    │
│  │  (PWM输入)   │ │ (PWM输出)  │ │ (数字输出)   │ │  (OLED显示)  │    │
│  └──────┬───────┘ └─────┬──────┘ └──────┬───────┘ └──────┬───────┘    │
│         │               │               │               │             │
│         ▼               ▼               ▼               ▼             │
│  ┌──────────────┐ ┌────────────┐ ┌──────────────┐ ┌──────────────┐    │
│  │  TIM2 + TIM3 │ │  TIM4     │ │  GPIO PB12~  │ │  ssd1306     │    │
│  │  输入捕获     │ │  PWM输出  │ │  PB15        │ │  I2C驱动     │    │
│  │  PA0~PA3     │ │  PB6~PB9  │ │  推挽输出     │ │  I2C2        │    │
│  │  PA6,PA7     │ │           │ │              │ │  PB10, PB11  │    │
│  │  PB0,PB1     │ │           │ │              │ │              │    │
│  └──────────────┘ └────────────┘ └──────────────┘ └──────────────┘    │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                     HAL 层 (stm32f1xx_hal)                       │   │
│  │    GPIO · TIM · I2C · NVIC · RCC                                │   │
│  └──────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. 模块详细说明

### 3.1 `pwm_input` — 8路PWM输入捕获

| 项目 | 说明 |
|------|------|
| **硬件定时器** | TIM2 (CH1~CH4) + TIM3 (CH1~CH4) |
| **时钟配置** | 8MHz → 预分频8分频 → 1MHz计数 → 1µs分辨率 |
| **GPIO映射** | CH1=PA0, CH2=PA1, CH3=PA2, CH4=PA3, CH5=PA6, CH6=PA7, CH7=PB0, CH8=PB1 |
| **测量范围** | 500~2500µs (标准RC脉冲 1000~2000µs, 额外余量) |
| **转换公式** | pulse 1000~2000µs → 0~100%, <1000→0%, >2000→100%, 无效→50%(中位) |
| **中断** | TIM2_IRQHandler / TIM3_IRQHandler, 优先级1 |
| **原理** | 双边沿捕获: 上升沿记录时间, 下降沿计算脉宽, 翻转极性交替捕获 |

### 3.2 `pwm_output` — 4路PWM输出

| 项目 | 说明 |
|------|------|
| **硬件定时器** | TIM4 (CH1~CH4) |
| **时钟配置** | 8MHz → 无预分频 → 125ns/tick |
| **GPIO映射** | CH1=PB6, CH2=PB7, CH3=PB8, CH4=PB9 |
| **默认频率** | 10kHz (可通过 `pwm_output_freq_hz` 动态调整) |
| **占空比** | 0~100% → 计算 CCR = percent × (ARR+1) / 100 |

### 3.3 `digital_output` — 8路数字输出

| 项目 | 说明 |
|------|------|
| **GPIO** | CH1~CH4: PB12~PB15, CH5: PA15, CH6~CH8: PB3~PB5 |
| **模式** | 推挽输出, 低电平初始 |
| **值域** | 0 = LOW, 非0 = HIGH |

### 3.4 `ssd1306` — OLED驱动

| 项目 | 说明 |
|------|------|
| **接口** | I2C2 (PB10=SCL, PB11=SDA), 400kHz |
| **地址** | 0x3C (7-bit) |
| **分辨率** | 128×64 |
| **架构** | 1024字节帧缓冲 → I2C批量刷新 |
| **字库** | 内置8×8 ASCII字库 (32~127) |

### 3.5 `display` — 应用层界面

- **左半屏** (x=0~63): 8路输入通道, 编号1~8, 条形图显示0~100%
- **右半屏** (x=64~127): 
  - 上方4行: PWM输出通道1~4, 条形图
  - 下方4行: 数字输出通道5~8, 显示 `ON`/`--`
- **中分线**: x=63 处垂直竖线

### 3.6 `main` — 入口与心跳

- SysTick_Handler → HAL_IncTick (1ms基准)
- 心跳LED (PC13) 根据油门(CH2)脉宽调速闪烁
  - 脉宽≤1000µs → 500ms周期 (慢闪)
  - 脉宽≥2000µs → 50ms周期 (快闪)
  - 中间线性插值
  - 无有效信号 → 自定义失败模式 (100ms周期闪两次)

### 3.7 `control` — 坦克混控算法

```
输入:
  CH1 = 转向 (steer)  ← 摇杆左右
  CH2 = 油门 (thr)   ← 摇杆上下

混控公式:
  thr   = CH2 - 50    →  -50 ~ +50
  steer = CH1 - 50    →  -50 ~ +50
  
  left  = thr + steer  →  左电机速度
  right = thr - steer  →  右电机速度

输出映射 (差速转向):
  left  ≥ 0 → out_ch_1 = left,  out_ch_2 = 0
  left  < 0 → out_ch_1 = 0,     out_ch_2 = -left
  right ≥ 0 → out_ch_3 = right, out_ch_4 = 0
  right < 0 → out_ch_3 = 0,     out_ch_4 = -right

  数字通道直通:
  out_ch_5 = (CH5 > 50) ? 1 : 0
  out_ch_6 = (CH6 > 50) ? 1 : 0
  out_ch_7 = (CH7 > 50) ? 1 : 0
  out_ch_8 = (CH8 > 50) ? 1 : 0
```

---

## 4. 运行流程图

### 4.1 主循环流程

```mermaid
flowchart TD
    A[上电/复位] --> B[SystemCoreClock = 8MHz]
    B --> C[HAL_Init]
    C --> D[GPIO_Init<br/>PC13 LED]
    D --> E[Display_Init<br/>SSD1306 OLED]
    E --> F[Control_Init]
    
    F --> F1[PWM_Input_Init<br/>TIM2+TIM3 输入捕获]
    F1 --> F2[PWM_Output_Init<br/>TIM4 PWM输出]
    F2 --> F3[Digital_Output_Init<br/>PB12~PB15]
    F3 --> G[进入主循环]
    
    G --> H[HAL_GetTick<br/>获取当前时间]
    H --> I{PWM_Input_IsValid(1)<br/>CH2有有效信号?}
    
    I -- 是 --> J[根据脉宽计算闪烁半周期<br/>pulse 1000~2000µs → 50~500ms]
    J --> K{now - last_tick<br/>>= half_period?}
    K -- 是 --> L[翻转PC13]
    L --> M[更新 last_tick]
    K -- 否 --> M
    
    I -- 否 --> N[失败模式: 100ms周期闪两次]
    N --> M
    
    M --> O[Control_Update]
    
    O --> P1[PWM_Input_GetPercent<br/>读取8路输入 0~100%]
    P1 --> P2[坦克混控算法<br/>thr+steer → left+right]
    P2 --> P3[PWM_Output_Set<br/>写入4路PWM输出]
    P3 --> P4[Digital_Output_Set<br/>写入4路数字输出]
    P4 --> P5[Display_Update<br/>刷新OLED]
    
    P5 --> Q[HAL_Delay(10)]
    Q --> H
```

### 4.2 PWM输入捕获 (中断) 流程

```mermaid
flowchart TD
    A[RC接收机<br/>输出PWM信号] --> B{TIM2/TIM3<br/>捕获中断触发}
    
    B --> C{当前边沿类型?}
    C -- 上升沿 --> D[记录 cap 到 pwm_rise[ch]]
    D --> E[设置 wait_rising = 0]
    E --> F[翻转极性 → 下次捕获下降沿]
    F --> Z[中断返回]
    
    C -- 下降沿 --> G[读取 cap]
    G --> H[计算脉宽 = cap - pwm_rise[ch]<br/>处理溢出]
    H --> I{脉宽在<br/>500~2500µs?}
    I -- 是 --> J[保存 pwm_pulse[ch]<br/>设置 pwm_valid[ch]=1]
    I -- 否 --> K[丢弃, 标记无效]
    J --> L[设置 wait_rising = 1]
    K --> L
    L --> M[翻转极性 → 下次捕获上升沿]
    M --> Z
```

### 4.3 数据流图

```mermaid
flowchart LR
    subgraph 输入
        RX[RC接收机<br/>8路PWM] --> TIM2[TIM2<br/>CH1~CH4<br/>PA0~PA3]
        RX --> TIM3[TIM3<br/>CH1~CH4<br/>PA6,PA7,PB0,PB1]
    end
    
    TIM2 --> ISR[输入捕获中断<br/>TIM2_IRQHandler<br/>TIM3_IRQHandler]
    TIM3 --> ISR
    
    ISR --> PULSE[pwm_pulse[0..7]<br/>脉宽 µs]
    
    PULSE --> CTL[Control_Update<br/>PWM_Input_GetPercent<br/>0~100%]
    
    CTL --> MIX[坦克混控<br/>thr+steer]
    
    MIX --> PWM_OUT[TIM4 PWM<br/>PB6~PB9<br/>out_ch_1~4]
    MIX --> DIG_OUT[GPIO 数字<br/>PB12~PB15<br/>out_ch_5~8]
    
    PWM_OUT --> M1[左电机前进]
    PWM_OUT --> M2[左电机后退]
    PWM_OUT --> M3[右电机前进]
    PWM_OUT --> M4[右电机后退]
    DIG_OUT --> D5[数字输出5]
    DIG_OUT --> D6[数字输出6]
    DIG_OUT --> D7[数字输出7]
    DIG_OUT --> D8[数字输出8]
    
    CTL --> DISP[Display_Update<br/>SSD1306 OLED<br/>128×64]
```

---

## 5. 硬件引脚分配

| 功能 | 引脚 | 外设 | 说明 |
|------|------|------|------|
| **PWM输入 CH1** | PA0 | TIM2_CH1 | RC接收机信号 |
| **PWM输入 CH2** | PA1 | TIM2_CH2 | RC接收机信号 (油门) |
| **PWM输入 CH3** | PA2 | TIM2_CH3 | RC接收机信号 |
| **PWM输入 CH4** | PA3 | TIM2_CH4 | RC接收机信号 |
| **PWM输入 CH5** | PA6 | TIM3_CH1 | RC接收机信号 |
| **PWM输入 CH6** | PA7 | TIM3_CH2 | RC接收机信号 |
| **PWM输入 CH7** | PB0 | TIM3_CH3 | RC接收机信号 |
| **PWM输入 CH8** | PB1 | TIM3_CH4 | RC接收机信号 |
| **PWM输出 CH1** | PB6 | TIM4_CH1 | 左电机前进 |
| **PWM输出 CH2** | PB7 | TIM4_CH2 | 左电机后退 |
| **PWM输出 CH3** | PB8 | TIM4_CH3 | 右电机前进 |
| **PWM输出 CH4** | PB9 | TIM4_CH4 | 右电机后退 |
| **数字输出 CH1** | PB12 | GPIO | 开关量输出5 |
| **数字输出 CH2** | PB13 | GPIO | 开关量输出6 |
| **数字输出 CH3** | PB14 | GPIO | 开关量输出7 |
| **数字输出 CH4** | PB15 | GPIO | 开关量输出8 |
| **数字输出 CH5** | PA15 | GPIO | 新增开关量 (原 JTDI) |
| **数字输出 CH6** | PB3  | GPIO | 新增开关量 (原 JTDO) |
| **数字输出 CH7** | PB4  | GPIO | 新增开关量 (原 NJTRST) |
| **数字输出 CH8** | PB5  | GPIO | 新增开关量 |
| **OLED SCL** | PB10 | I2C2_SCL | SSD1306时钟 |
| **OLED SDA** | PB11 | I2C2_SDA | SSD1306数据 |
| **LED** | PC13 | GPIO | 心跳指示灯 (板载) |

---

## 6. 中断向量

| 中断 | 优先级 | 用途 |
|------|--------|------|
| SysTick_Handler | 默认(0) | HAL 1ms 时基 |
| TIM2_IRQHandler | 1 | CH1~CH4 PWM输入捕获 |
| TIM3_IRQHandler | 1 | CH5~CH8 PWM输入捕获 |

---

## 7. 文件依赖关系

```
main.c
 ├── stm32f1xx_hal.h        (HAL库)
 ├── control.h
 │    ├── pwm_input.h
 │    ├── pwm_output.h
 │    ├── digital_output.h
 │    └── display.h
 │         └── ssd1306.h
 └── (调用关系)

control.c
 ├── control.h
 ├── pwm_input.h
 ├── pwm_output.h
 ├── digital_output.h
 └── display.h

pwm_input.c
 ├── pwm_input.h
 └── stm32f1xx_hal.h

pwm_output.c
 ├── pwm_output.h
 └── stm32f1xx_hal.h

digital_output.c
 ├── digital_output.h
 └── stm32f1xx_hal.h

display.c
 ├── display.h
 └── ssd1306.h

ssd1306.c
 ├── ssd1306.h
 └── stm32f1xx_hal.h
```
