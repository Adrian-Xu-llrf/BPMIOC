# 03. RF信号基础

> **目标**: 理解RF信号的幅度、相位等参数的物理意义
> **难度**: ⭐⭐
> **预计时间**: 25分钟

## 1. 什么是RF信号？

### 1.1 RF = Radio Frequency

**定义**: 射频信号，通常指几MHz到几GHz的高频电磁波

**加速器中的RF**:
- 频率: 通常100MHz - 3GHz
- 作用: 加速带电粒子
- 例子: 500MHz, 1.3GHz (XFEL), 2.856GHz (SLAC)

### 1.2 为什么用RF？

```
直流电场 vs. RF电场

直流:
E ───────→  只能加速一次

RF:
E ∿∿∿∿∿∿→  可以重复加速（同步）
```

**优点**:
- 粒子可以一圈圈加速
- 通过相位控制实现同步
- 高功率密度

## 2. RF信号的数学表示

### 2.1 时域表示

```
V(t) = A × cos(2πf×t + φ)

A = 幅度 (Amplitude)
f = 频率 (Frequency)
φ = 相位 (Phase)
t = 时间
```

**图示**:

```
幅度 A
  ↑
  │    ╱‾╲     ╱‾╲
  │   ╱   ╲   ╱   ╲
  │  ╱     ╲ ╱     ╲
  └──────────────────→ 时间
  │ ╲     ╱ ╲     ╱
  │  ╲   ╱   ╲   ╱
  │   ╲_╱     ╲_╱
  │
  ↓ -A

  ◄───────►  周期 T = 1/f
```

### 2.2 复数表示

```
V(t) = A × e^(j(2πft + φ))
     = A × e^(jφ) × e^(j2πft)
       ↑           ↑
     复幅度      旋转因子
```

## 3. 关键参数详解

### 3.1 幅度 (Amplitude)

**物理意义**: 信号强度，与束流强度和能量相关

**单位**:
- 电压: V (伏特)
- 功率: dBm (相对于1mW的分贝数)

**转换**:
```
P(dBm) = 10 × log10(P(mW))

例子:
1mW = 0dBm
10mW = 10dBm
100mW = 20dBm
```

**BPMIOC中的幅度**:

```c
// PV: LLRF:BPM:RFIn_01_Amp
// 单位: dBm
// 典型值: 0 - 20 dBm

float amp_dbm = 10.5;  // dBm

// 转换为mW
float amp_mw = pow(10.0, amp_dbm / 10.0);
// amp_mw = 11.22 mW

// 转换为V (假设50Ω系统)
float amp_v = sqrt(amp_mw * 0.001 * 50.0);
// amp_v = 0.75 V
```

### 3.2 相位 (Phase)

**物理意义**: 信号的时间偏移，决定加速/减速

**单位**: 度 (°) 或弧度 (rad)

**关键概念**:
- 0°: 同相 (最大加速)
- 90°: 正交
- 180°: 反相 (减速)

**图示**:

```
        信号1 (0°)
         ╱‾╲
        ╱   ╲
       ╱     ╲
──────────────────
       ╲     ╱
        ╲   ╱
         ╲_╱

        信号2 (90°)
           ╱‾╲
──────────╱───╲────
         ╲     ╱
          ╲   ╱
           ╲_╱
```

**相位差的意义**:

```
Δφ = φ_beam - φ_ref

Δφ = 0°:   束流与RF同步
Δφ > 0°:   束流提前
Δφ < 0°:   束流滞后
```

**BPMIOC中的相位**:

```c
// PV: LLRF:BPM:RFIn_01_Phase
// 单位: degree
// 范围: -180° to +180°

float phase_deg = 45.0;  // 度

// 转换为弧度
float phase_rad = phase_deg * M_PI / 180.0;

// 判断同步状态
if (fabs(phase_deg) < 5.0) {
    printf("Beam in sync\n");
} else {
    printf("Beam phase error: %.2f deg\n", phase_deg);
}
```

### 3.3 频率 (Frequency)

**加速器RF频率选择**:

| 加速器 | RF频率 | 原因 |
|--------|--------|------|
| SLAC | 2.856 GHz | 高加速梯度 |
| European XFEL | 1.3 GHz | 超导腔 |
| 同步光源 | 500 MHz | 长脉冲稳定 |

**频率稳定性要求**:

```
Δf / f < 10^-6

例如: f = 500 MHz
      Δf < 500 Hz
```

## 4. I/Q解调

### 4.1 什么是I/Q？

**I = In-phase (同相分量)**
**Q = Quadrature (正交分量)**

**数学关系**:

```
V(t) = A cos(2πft + φ)
     = I × cos(2πft) - Q × sin(2πft)

其中:
I = A × cos(φ)  (同相分量)
Q = A × sin(φ)  (正交分量)

幅度: A = √(I² + Q²)
相位: φ = arctan(Q / I)
```

### 4.2 I/Q图解

```
        Q (正交)
        ↑
        │
        │  ╱ A (幅度)
        │╱
        │╲ φ (相位)
        │  ╲
────────┼────────→ I (同相)
        │
        │
        │
```

**例子**:

```
I = 3.0
Q = 4.0

A = √(3² + 4²) = 5.0
φ = arctan(4/3) = 53.13°
```

### 4.3 为什么需要I/Q？

**优点**:
1. 完整描述RF信号（幅度+相位）
2. 便于数字处理
3. 解决相位模糊问题

**硬件实现**:

```
RF信号
  ↓
混频器 ← RF参考信号
  ↓
╔═══════════╦═══════════╗
║  cos支路  ║  sin支路  ║
║ (I通道)   ║ (Q通道)   ║
╚═══╤═══════╩═══╤═══════╝
    ↓           ↓
   ADC         ADC
    ↓           ↓
   I数据       Q数据
```

**FPGA处理**:

```vhdl
-- 伪代码：I/Q解调
signal i_data, q_data : signed(15 downto 0);
signal amplitude, phase : signed(31 downto 0);

-- 幅度计算 (CORDIC算法)
amplitude <= sqrt(i_data * i_data + q_data * q_data);

-- 相位计算 (CORDIC算法)
phase <= atan2(q_data, i_data);
```

## 5. 信噪比 (SNR)

### 5.1 定义

```
SNR = 信号功率 / 噪声功率

或（dB形式）:
SNR(dB) = 10 × log10(P_signal / P_noise)
```

**物理意义**: 信号质量指标

### 5.2 SNR对测量精度的影响

```
位置精度 ∝ 1 / SNR

例子:
SNR = 100 (20dB) → 位置精度 ~10μm
SNR = 10  (10dB) → 位置精度 ~100μm
```

### 5.3 提高SNR的方法

| 方法 | 原理 | 代码示例 |
|------|------|----------|
| **平均** | 降低随机噪声 | `avg = sum(samples) / N` |
| **滤波** | 去除带外噪声 | FIR/IIR滤波器 |
| **提高信号** | 增加束流强度 | 硬件调整 |

**代码示例**:

```python
import numpy as np

def calculate_snr(signal, noise_samples):
    """计算信噪比"""
    signal_power = signal ** 2
    noise_power = np.mean(noise_samples ** 2)

    snr_linear = signal_power / noise_power
    snr_db = 10 * np.log10(snr_linear)

    return snr_db

# 例子
signal = 1.0  # V
noise = np.random.normal(0, 0.01, 1000)  # 噪声标准差0.01V

snr = calculate_snr(signal, noise)
print(f"SNR = {snr:.1f} dB")  # ~40dB
```

## 6. 品质因子 (Q值)

### 6.1 定义

```
Q = f0 / Δf

f0 = 谐振频率
Δf = 3dB带宽
```

**物理意义**: 谐振器的"尖锐度"

### 6.2 高Q vs. 低Q

```
        高Q (Q=1000)
        │
     ▲  │  ▲
    ││  │  ││
    ││  │  ││
────┴┴──┴──┴┴──── 频率
        │
     ◄─►  Δf很窄


        低Q (Q=10)
        │
     ╱‾‾│‾‾╲
    ╱   │   ╲
───╱────┴────╲─── 频率
        │
     ◄────►  Δf较宽
```

**应用**:
- RF腔: 高Q (10^4 - 10^6) → 能量存储
- BPM: 适中Q (10-100) → 带宽vs选择性

### 6.3 BPMIOC中的Q值

```c
// PV: LLRF:BPM:RFIn_01_Q
// 谐振腔或BPM的品质因子

float calculate_q_factor(float f0, float f_low, float f_high) {
    // f_low, f_high: -3dB频率点
    float bandwidth = f_high - f_low;
    return f0 / bandwidth;
}

// 例子
float q = calculate_q_factor(500e6, 499.9e6, 500.1e6);
// q = 2500
```

## 7. 数字信号处理

### 7.1 采样定理

```
采样率 f_s ≥ 2 × f_max

例子:
RF信号频率 = 500MHz
采样率 ≥ 1GHz (实际常用2-4 GSPS)
```

**欠采样技术**:

```
使用混频下变频后再采样:

500MHz RF → 混频 → 10MHz IF → 采样(50MSPS)
```

### 7.2 数字滤波

**FIR滤波器示例**:

```python
from scipy import signal

# 设计低通滤波器
fs = 100e6  # 100MHz采样率
fc = 10e6   # 10MHz截止频率
N = 64      # 阶数

# FIR滤波器系数
taps = signal.firwin(N, fc, fs=fs)

# 应用滤波
filtered_signal = signal.lfilter(taps, 1.0, input_signal)
```

## 8. 实际应用

### 8.1 RF测量链路

```
┌────────┐     ┌────────┐     ┌────────┐     ┌────────┐
│  BPM   │ ──→ │ 放大器 │ ──→ │ 混频器 │ ──→ │  ADC   │
│ pickup │     │ (LNA)  │     │ (I/Q)  │     │(14bit) │
└────────┘     └────────┘     └────────┘     └────────┘
  500MHz         Gain=30dB     ↓ LO=490MHz      采样100MS/s
                              10MHz IF

      ↓
┌──────────────────────────────────────┐
│          FPGA数字处理                │
│  ┌────────┐  ┌─────────┐  ┌────────┐│
│  │ DDC    │→ │数字滤波 │→ │ I/Q分离││
│  └────────┘  └─────────┘  └────────┘│
└──────────────────────────────────────┘
      ↓
┌──────────────────────────────────────┐
│            BPM IOC                   │
│  ┌────────┐  ┌─────────┐            │
│  │ 读取   │→ │ PV发布  │            │
│  │ I/Q数据│  │ 幅度相位 │            │
│  └────────┘  └─────────┘            │
└──────────────────────────────────────┘
```

### 8.2 常见问题排查

| 问题 | 可能原因 | 检查方法 |
|------|----------|----------|
| **幅度过低** | 束流弱、增益低 | 检查束流、放大器 |
| **相位跳变** | PLL失锁 | 检查参考信号 |
| **SNR低** | 噪声大 | 检查接地、屏蔽 |
| **Q值异常** | 谐振漂移 | 检查温度、调谐 |

**Python诊断工具**:

```python
import epics

def diagnose_rf_signal(pv_prefix):
    """诊断RF信号质量"""
    amp_pv = epics.PV(f'{pv_prefix}:Amp')
    phase_pv = epics.PV(f'{pv_prefix}:Phase')

    amp = amp_pv.get()
    phase = phase_pv.get()

    print(f"幅度: {amp:.2f} dBm")
    print(f"相位: {phase:.2f} deg")

    # 检查幅度
    if amp < 5.0:
        print("⚠ 警告: 幅度过低")
    elif amp > 25.0:
        print("⚠ 警告: 幅度过高，可能饱和")
    else:
        print("✓ 幅度正常")

    # 检查相位
    if abs(phase) > 30:
        print("⚠ 警告: 相位偏离大")
    else:
        print("✓ 相位正常")

# 使用
diagnose_rf_signal('LLRF:BPM:RFIn_01')
```

## 9. 小结

### 关键要点

1. **RF信号 = A × cos(2πft + φ)**
   - 幅度 A: 信号强度
   - 相位 φ: 时间偏移
   - 频率 f: 振荡速度

2. **I/Q解调**
   - I = A cos(φ), Q = A sin(φ)
   - 完整描述幅度和相位

3. **质量指标**
   - SNR: 信噪比，影响测量精度
   - Q值: 品质因子，表征谐振器

4. **软件开发关注点**
   - 理解I/Q数据的物理意义
   - 实现数字滤波和信号处理
   - 数据质量监控（SNR、幅度范围）

### 下一步

继续学习 [04-LLRF系统概述](./04-llrf-system-overview.md) 了解LLRF系统的整体架构。

## 📚 延伸阅读

- [I/Q Data for Dummies](https://www.tek.com/en/documents/primer/iq-basics)
- [Digital Signal Processing in LLRF](https://accelconf.web.cern.ch/)
