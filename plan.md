# 8 通道 RC 接收机 PWM 信号转换器

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (Cortex-M3) |
| Flash | 64 KB |
| RAM | 20 KB |
| 开发框架 | STM32Cube (HAL) |
| 构建工具 | PlatformIO |
| 调试/烧录 | J-Link SWD |
| 板载 LED | PC13 |

### 时钟配置

| 项目 | 值 |
|------|----|
| 时钟源 | HSI（内部 RC 振荡器） |
| HSI 频率 | 8 MHz |
| SYSCLK | 8 MHz（1 分频，不走 PLL） |
| HCLK | 8 MHz |
| APB1 (PCLK1) | 8 MHz |
| APB2 (PCLK2) | 8 MHz |
| TIM2/TIM3 时钟 | 8 MHz |

> **说明**：使用内部 HSI 而不用外部 HSE 晶振，因为常见 STM32F103C8T6 最小系统板可能未焊接 8MHz 外部晶振。不走 PLL 保持最大稳定性。

---

## RC 接收机信号规格

| 参数 | 范围 |
|------|------|
| 信号类型 | 标准航模 PWM |
| 周期 | ~20 ms（50 Hz） |
| 最小脉宽 | 1.0 ms（-100% 油门） |
| 中位脉宽 | 1.5 ms（0% 油门） |
| 最大脉宽 | 2.0 ms（+100% 油门） |

### 信号波形

```
         ┌──────────────────┐
         │   脉宽 1~2ms     │
    ─────┘                  └────────────────────
         │←────────── 周期 ≈20ms ──────────→│

    上升沿 → 开始计时
    下降沿 → 停止计时，脉宽 = 下降沿时间 - 上升沿时间
```

---

## 引脚分配

### 输入通道（8 路 RC 接收机）

```
接收机 CH1 ──→ PA0  (TIM2_CH1)
接收机 CH2 ──→ PA1  (TIM2_CH2)
接收机 CH3 ──→ PA2  (TIM2_CH3)
接收机 CH4 ──→ PA3  (TIM2_CH4)
接收机 CH5 ──→ PA6  (TIM3_CH1)
接收机 CH6 ──→ PA7  (TIM3_CH2)
接收机 CH7 ──→ PB0  (TIM3_CH3)
接收机 CH8 ──→ PB1  (TIM3_CH4)
```

| 通道 | 功能 | 定时器 | 引脚 | 定时器通道 |
|------|------|--------|------|------------|
| 1 | 输入捕获 | TIM2 | PA0 | CH1 (转向摇杆) |
| 2 | 输入捕获 | TIM2 | PA1 | CH2 (油门摇杆) |
| 3 | 输入捕获 | TIM2 | PA2 | CH3 |
| 4 | 输入捕获 | TIM2 | PA3 | CH4 |
| 5 | 输入捕获 | TIM3 | PA6 | CH1 |
| 6 | 输入捕获 | TIM3 | PA7 | CH2 |
| 7 | 输入捕获 | TIM3 | PB0 | CH3 |
| 8 | 输入捕获 | TIM3 | PB1 | CH4 |

### 输出通道（4 路 PWM + 4 路数字）

```
PWM 输出 CH1 ──→ PB6  (TIM4_CH1)
PWM 输出 CH2 ──→ PB7  (TIM4_CH2)
PWM 输出 CH3 ──→ PB8  (TIM4_CH3)
PWM 输出 CH4 ──→ PB9  (TIM4_CH4)

数字输出 CH5 ──→ PB12
数字输出 CH6 ──→ PB13
数字输出 CH7 ──→ PB14
数字输出 CH8 ──→ PB15
```

| 通道 | 功能 | 定时器 | 引脚 | 说明 |
|------|------|--------|------|------|
| 1 | PWM 输出 | TIM4 | PB6 | PWM 通道 1 |
| 2 | PWM 输出 | TIM4 | PB7 | PWM 通道 2 |
| 3 | PWM 输出 | TIM4 | PB8 | PWM 通道 3 |
| 4 | PWM 输出 | TIM4 | PB9 | PWM 通道 4 |
| 5 | 数字输出 | — | PB12 | 0/1, 阈值 1500µs |
| 6 | 数字输出 | — | PB13 | 0/1, 阈值 1500µs |
| 7 | 数字输出 | — | PB14 | 0/1, 阈值 1500µs |
| 8 | 数字输出 | — | PB15 | 0/1, 阈值 1500µs |

---

## 测量方案

### 技术选型

**定时器输入捕获（Timer Input Capture）**

- TIM2 提供 4 路输入捕获 → 通道 1~4
- TIM3 提供 4 路输入捕获 → 通道 5~8
- 每路配置为双沿捕获（上升沿 + 下降沿），中断驱动
- ISR 中自动切换捕获极性，计算脉宽

### 定时器参数

| 参数 | 值 | 说明 |
|------|----|------|
| 定时器时钟 | 8 MHz | 等于 HCLK（APB1 预分频器=1） |
| 预分频器 (PSC) | 7 | 8MHz ÷ 8 = **1 MHz** |
| 计数器精度 | **1 µs** | 每计数 = 1 微秒 |
| 自动重装载 (ARR) | 65535 (0xFFFF) | 最大测量周期 65.5ms |
| 输入滤波器 | 关闭 | RC 接收机信号足够干净 |

### 中断处理流程

```
TIMx_IRQHandler()
  │
  ├─ CC1IF? ──→ 读取 CCR1 ──→ 上升沿? ──→ 保存 rise_time, 切换到下降沿
  │                         └─→ 下降沿? ──→ pulse = fall - rise, 存入数组
  │
  ├─ CC2IF? ──→ (同上)
  ├─ CC3IF? ──→ (同上)
  └─ CC4IF? ──→ (同上)
```

### 计数器溢出处理

```c
if (fall_time < rise_time)
    fall_time += 65536;    // 处理 16-bit 计数器回绕
pulse = fall_time - rise_time;
```

### 信号有效性校验

- 脉宽范围：**500µs ~ 2500µs**
- 超出此范围的视为无效/噪声

---

## 默认映射逻辑（坦克混控演示）

项目内置一组坦克差速转向混控作为演示示例，展示如何将 CH1/CH2 摇杆输入映射为双电机控制。你可以自由替换为任意映射逻辑。

### 摇杆输入（CH1、CH2）

| 通道 | 功能 | 摇杆方向 | 脉宽范围 | 映射值 |
|------|------|----------|----------|--------|
| CH1 | 转向 | 左←→右 | 1000~1500~2000µs | -100←0→+100 |
| CH2 | 油门 | 后退←→前进 | 1000~1500~2000µs | -100←0→+100 |

摇杆默认居中（1500µs = 0%），CH3~CH8 预留。

### 坦克混控算法（演示）

```
left  = throttle + steering
right = throttle - steering
```

结果钳制到 ±100%。

混控举例：

| 操作 | throttle | steering | left | right | 车辆动作 |
|------|----------|----------|------|-------|----------|
| 前进 | +100 | 0 | +100 | +100 | 直线前进 |
| 后退 | -100 | 0 | -100 | -100 | 直线后退 |
| 原地右转 | 0 | +100 | +100 | -100 | 左前右后自旋 |
| 原地左转 | 0 | -100 | -100 | +100 | 左后右前自旋 |
| 前进行进中右转 | +100 | +50 | +150→+100 | +50 | 左快右慢右转 |

### 输出通道 CH1~CH4：PWM（TIM4）

每组输出两个 PWM 通道，配合使用可控制 H 桥的正转/反转：

| 输出 | 引脚 | 功能 |
|------|------|------|
| CH1 | PB6 (TIM4_CH1) | PWM 通道 1 |
| CH2 | PB7 (TIM4_CH2) | PWM 通道 2 |
| CH3 | PB8 (TIM4_CH3) | PWM 通道 3 |
| CH4 | PB9 (TIM4_CH4) | PWM 通道 4 |

**混控输出规则**（演示示例）：
- left ≥ 0 → CH1 = left%, CH2 = 0%
- left < 0 → CH1 = 0%, CH2 = |left|%
- right 同理映射到 CH3/CH4

### PWM 参数

| 参数 | 值 |
|------|----|
| 定时器 | TIM4 |
| 时钟 | 8 MHz（APB1） |
| 预分频器 | 0（不分频） |
| 默认频率 | **10 kHz**（周期 100µs） |
| ARR 寄存器 | 799（10kHz 时） |
| 占空比分辨率 | 800 步（~0.125%） |
| 频率配置 | 全局变量 `pwm_output_freq_hz`，运行时可变 |

### 输出通道 CH5~CH8：数字输出（GPIO）

| 参数 | 值 |
|------|----|
| 输出引脚 | PB12, PB13, PB14, PB15 |
| 输出模式 | 推挽输出 (Push-Pull) |
| 逻辑阈值 | 输入脉宽 > 1500µs → HIGH，否则 LOW |

---

## 软件架构

### 文件结构

```
PWMSwitch/
├── platformio.ini              # PlatformIO 项目配置
├── plan.md                     # 本文件
├── src/
│   ├── main.c                  # 主程序：输入→输出映射 + LED 心跳
│   ├── pwm_input.h             # PWM 输入捕获 API（TIM2+TIM3）
│   ├── pwm_input.c             # 输入捕获实现
│   ├── pwm_output.h            # PWM 输出 API（TIM4）
│   ├── pwm_output.c            # PWM 输出实现
│   ├── digital_output.h        # 数字输出 API
│   └── digital_output.c        # 数字输出实现 (PB12~15)
├── include/
├── lib/
└── test/
```

### 模块 API

#### pwm_input

```c
void     PWM_Input_Init(void);
uint32_t PWM_Input_Read(uint8_t channel);       // 返回脉宽 (µs)
uint8_t  PWM_Input_IsValid(uint8_t channel);    // 信号是否有效
```

#### pwm_output

```c
extern uint32_t pwm_output_freq_hz;             // 全局频率变量，默认 10000
void PWM_Output_Init(void);                     // 初始化 TIM4 4-ch PWM
void PWM_Output_ApplyFreq(void);                // 修改频率后调用
void PWM_Output_Set(uint8_t channel, uint8_t pct); // 0~100%
```

#### digital_output

```c
void Digital_Output_Init(void);                  // 初始化 PB12~PB15
void Digital_Output_Set(uint8_t channel, uint8_t val); // 0=LOW, 1=HIGH
```

### main.c 行为

| 输入 | 输出 | 映射逻辑 |
|------|------|----------|
| CH1 (转向) | PWM CH1~CH4 | 默认映射（steering） |
| CH2 (油门) | PWM CH1~CH4 | 默认映射（throttle） |
| CH3~CH4 有效 | — | 读取但暂未使用 |
| CH5 有效 | DO CH5 | 脉宽 > 1500µs → HIGH |
| CH6 有效 | DO CH6 | 同上 |
| CH7 有效 | DO CH7 | 同上 |
| CH8 有效 | DO CH8 | 同上 |
| 任意输入无效 | 对应输出 | 关断（0% 或 LOW） |

**LED 指示**：
- 有信号：闪烁频率随 CH2 油门变化（0%→1Hz, 100%→10Hz）
- 无信号：双闪模式，每秒快闪两次（亮50ms-灭50ms-亮50ms-灭850ms）

---

## 烧录与调试

### 烧录命令

```bash
pio run --target upload
```

或在 VSCode 中按 `Ctrl+Alt+U`

### 调试

```bash
pio debug
```

或在 VSCode 中按 `F5` 启动 J-Link SWD 调试

### 关键断点位置

- `pwm_input.c` → `capture_isr()`：观察上升/下降沿捕获过程
- `main.c` → `while(1)` 循环内：观察 `ch[]` 数组中的脉宽值

---

## 验证步骤

1. 按引脚表将接收机 8 路信号线接到 STM32
2. 接收机和 STM32 共地（GND 必须接通）
3. `pio run --target upload` 烧录固件
4. 观察 LED：连接 CH1 并推油门，LED 闪烁速率应随之变化
5. 用 J-Link 调试断点检查 `pwm_values[]` 数组，确认数值在 1000~2000 范围
6. 依次验证 8 个通道

---

## 资源占用

| 资源 | 使用量 | 占比 |
|------|--------|------|
| RAM | 356 字节 | 1.7% |
| Flash | 4652 字节 | 7.1% |
| 定时器 | TIM2, TIM3, TIM4 | — |
| 中断 | TIM2_IRQn, TIM3_IRQn | — |
| GPIO 输入 | PA0~PA3, PA6~PA7, PB0~PB1 | 8 路 |
| GPIO 输出 | PB6~PB9, PB12~PB15, PC13 | 9 路 |
 