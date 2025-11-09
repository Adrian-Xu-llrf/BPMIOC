# 04. LLRF系统概述

> **目标**: 理解LLRF系统架构和BPM在其中的作用
> **难度**: ⭐⭐
> **预计时间**: 20分钟

## 1. LLRF系统简介

### 1.1 什么是LLRF？

**LLRF = Low-Level Radio Frequency**

**作用**: 精确控制RF腔的电压幅度和相位，确保束流稳定加速

**类比**: 就像汽车的巡航控制系统
- 设定速度(目标电压/相位)
- 传感器测量实际速度(BPM, RF pickup)
- 控制器调整油门(RF功率)

### 1.2 为什么需要LLRF？

**问题**: 没有LLRF时：
- RF腔电压不稳定 → 束流能量漂移
- 相位漂移 → 束流丢失
- 负载变化 → 性能下降

**解决**: LLRF提供：
- 幅度稳定性: <0.01%
- 相位稳定性: <0.01°
- 响应时间: <1μs

## 2. LLRF系统架构

### 2.1 整体框图

```
┌───────────────────────────────────────────────────┐
│          Control Room (控制室)                    │
│  ┌──────────────┐        ┌──────────────┐        │
│  │ Operator GUI │ ◄────► │  EPICS IOC   │        │
│  │  (CS-Studio) │        │ (BPM, RF等)  │        │
│  └──────────────┘        └──────┬───────┘        │
└───────────────────────────────────┼───────────────┘
                                    ↕ (Ethernet, CA)
┌───────────────────────────────────┴───────────────┐
│             LLRF Controller Rack                  │
│  ┌────────────────────────────────────────────┐  │
│  │  LLRF Digital Controller (FPGA/μC)         │  │
│  │  ┌───────────┐  ┌──────────┐  ┌─────────┐ │  │
│  │  │ Amplitude │  │  Phase   │  │  Beam   │ │  │
│  │  │ Feedback  │  │ Feedback │  │ Loading │ │  │
│  │  └───────────┘  └──────────┘  └─────────┘ │  │
│  └────────────────────────────────────────────┘  │
│            ↑                          │          │
│     ┌──────┴──────┐          ┌───────▼───────┐  │
│     │  RF Pickup  │          │  RF Drive Amp │  │
│     │ (ADC采样)   │          │  (DAC输出)    │  │
│     └─────────────┘          └───────────────┘  │
└──────────────┬─────────────────────┬─────────────┘
               ↕                     ↕
┌──────────────┴─────────────────────┴─────────────┐
│           Accelerator Tunnel (隧道)              │
│  ┌────────┐      ┌─────────┐      ┌───────┐     │
│  │  BPM   │ ───► │ RF Cavity│ ◄─── │ Klystron│   │
│  │        │      │ (加速腔) │      │(功率放大)│   │
│  └────────┘      └─────────┘      └───────┘     │
│         ↑               │                        │
│         └───── 束流 ─────┘                        │
└───────────────────────────────────────────────────┘
```

### 2.2 信号流

**前向通道 (Forward Path)**:
```
设定值 → LLRF控制器 → DAC → 功率放大 → RF腔 → 束流加速
```

**反馈通道 (Feedback Path)**:
```
RF腔探头 → ADC → LLRF控制器 → 误差计算 → 修正信号
BPM信号  ↗
```

## 3. LLRF控制算法

### 3.1 幅度-相位反馈

**目标**: 使RF腔电压跟踪设定值

**控制框图**:

```
设定值     误差        控制量      实际值
Vref ───→ (Σ) ───→ [PI] ───→ (Plant) ───┐
          ↑ -                            │
          └────────── 反馈 ◄──────────────┘
```

**PI控制器**:

```
u(t) = Kp × e(t) + Ki × ∫e(t)dt

Kp = 比例增益
Ki = 积分增益
e(t) = Vref - Vmeasured
```

**数字实现**:

```c
// 离散PI控制器
float pi_controller(float error, float kp, float ki, float dt) {
    static float integral = 0.0;

    integral += error * dt;

    // 积分饱和限制
    if (integral > MAX_INTEGRAL) integral = MAX_INTEGRAL;
    if (integral < -MAX_INTEGRAL) integral = -MAX_INTEGRAL;

    float output = kp * error + ki * integral;

    return output;
}

// LLRF反馈循环
void llrf_feedback_loop() {
    // 读取设定值
    float v_ref_amp = GetSetpoint_Amplitude();
    float v_ref_phase = GetSetpoint_Phase();

    // 读取测量值
    float v_meas_amp, v_meas_phase;
    ReadRFPickup(&v_meas_amp, &v_meas_phase);

    // 计算误差
    float error_amp = v_ref_amp - v_meas_amp;
    float error_phase = v_ref_phase - v_meas_phase;

    // PI控制
    float ctrl_amp = pi_controller(error_amp, KP_AMP, KI_AMP, DT);
    float ctrl_phase = pi_controller(error_phase, KP_PHASE, KI_PHASE, DT);

    // 输出到DAC
    OutputRFDrive(ctrl_amp, ctrl_phase);
}
```

### 3.2 束流负载补偿

**问题**: 束流经过RF腔时会吸收能量
```
束流 → RF腔 → 腔压降低 → 加速效果下降
```

**解决**: 前馈补偿
```
测量束流强度 → 计算负载 → 预先增加RF功率
```

**算法**:

```c
float beam_loading_compensation(float beam_current) {
    // 束流负载导致的电压降
    float V_beam = BEAM_LOADING_COEFF * beam_current;

    // 补偿: 预先增加这么多电压
    float V_compensate = V_beam;

    return V_compensate;
}

// 集成到反馈循环
void llrf_with_beam_loading() {
    // 测量束流流强
    float i_beam = ReadBeamCurrent_FromBPM();

    // 前馈补偿
    float v_ff = beam_loading_compensation(i_beam);

    // 设定值 = 期望值 + 补偿
    float v_setpoint = V_NOMINAL + v_ff;

    // 正常反馈控制
    llrf_feedback_loop();
}
```

## 4. BPM在LLRF中的作用

### 4.1 三种应用模式

| 模式 | 用途 | 更新率 | 精度 |
|------|------|--------|------|
| **轨道反馈** | 位置修正 | 1-10kHz | <10μm |
| **相位参考** | RF同步检查 | 100Hz | <0.1° |
| **束流监控** | 流强测量 | 10Hz | <1% |

### 4.2 模式1: 轨道反馈

**流程**:

```
1. BPM测量位置 (x, y)
   ↓
2. 轨道控制器计算修正量
   Δθ = K × (x_measured - x_ref)
   ↓
3. 调整校正磁铁电流
   I_corrector += Δθ
   ↓
4. 束流位置回到参考轨道
```

**EPICS PV关联**:

```
# BPM位置PV
LLRF:BPM:01:PosX     (输入)
LLRF:BPM:01:PosY     (输入)

# 参考轨道PV
LLRF:Orbit:Ref:01:X  (设定值)

# 校正器PV
LLRF:Corrector:01:Current  (输出)
```

### 4.3 模式2: 相位同步

**目的**: 确保束流与RF腔相位同步

**检测方法**:

```
BPM的RF信号相位 vs. RF参考相位

Δφ = φ_BPM - φ_ref

|Δφ| < 阈值 → 同步OK
|Δφ| > 阈值 → 需要调整
```

**代码示例**:

```python
import epics

def check_rf_synchronization():
    """检查RF同步状态"""
    # 读取BPM RF相位
    pv_bpm_phase = epics.PV('LLRF:BPM:01:RF:Phase')
    phase_bpm = pv_bpm_phase.get()

    # 读取RF参考相位
    pv_ref_phase = epics.PV('LLRF:RF:Master:Phase')
    phase_ref = pv_ref_phase.get()

    # 计算相位差
    phase_error = phase_bpm - phase_ref

    # 归一化到±180°
    while phase_error > 180:
        phase_error -= 360
    while phase_error < -180:
        phase_error += 360

    print(f"相位误差: {phase_error:.2f}°")

    # 判断同步状态
    if abs(phase_error) < 5.0:
        print("✓ RF同步正常")
        return True
    else:
        print("✗ RF相位失步")
        return False
```

### 4.4 模式3: 束流流强监控

**原理**: BPM信号总和 ∝ 束流流强

```
I_beam = K × (V1 + V2 + V3 + V4)

K = 标定系数
```

**LLRF应用**: 束流负载补偿的输入

```c
float estimate_beam_current_from_bpm() {
    // 读取BPM 4个button信号
    float v1, v2, v3, v4;
    ReadBPMButtons(&v1, &v2, &v3, &v4);

    // 信号总和
    float sum = v1 + v2 + v3 + v4;

    // 转换为电流（需要标定）
    float i_beam = BPM_CURRENT_CALIB * sum;

    return i_beam;
}
```

## 5. LLRF系统性能指标

### 5.1 关键指标

| 参数 | 典型值 | 物理意义 |
|------|--------|----------|
| **幅度稳定性** | 0.01% | RF腔电压波动 |
| **相位稳定性** | 0.01° | RF相位抖动 |
| **响应带宽** | 1-10kHz | 反馈速度 |
| **动态范围** | 60dB | 适应束流变化 |

### 5.2 性能测试

**Python测试脚本**:

```python
import epics
import numpy as np
import time

def test_amplitude_stability(pv_name, duration=60):
    """测试幅度稳定性"""
    pv = epics.PV(pv_name)

    samples = []
    start_time = time.time()

    while time.time() - start_time < duration:
        samples.append(pv.get())
        time.sleep(0.01)  # 100Hz采样

    samples = np.array(samples)

    mean = np.mean(samples)
    std = np.std(samples)
    stability = (std / mean) * 100  # 百分比

    print(f"幅度稳定性测试 ({duration}s):")
    print(f"  均值: {mean:.4f}")
    print(f"  标准差: {std:.4f}")
    print(f"  稳定性: {stability:.4f}%")

    if stability < 0.01:
        print("  ✓ 满足要求 (<0.01%)")
    else:
        print("  ✗ 不满足要求")

    return stability

# 测试
stability = test_amplitude_stability('LLRF:RF:Cavity01:Amp', duration=60)
```

## 6. 故障诊断

### 6.1 常见问题

| 现象 | 可能原因 | 排查方法 |
|------|----------|----------|
| **幅度振荡** | 增益过高 | 降低Kp |
| **相位失锁** | 参考信号丢失 | 检查RF Master |
| **响应慢** | 增益过低 | 增加Ki |
| **饱和** | 设定值过高 | 检查限幅 |

### 6.2 诊断工具

```python
def diagnose_llrf_system(prefix):
    """LLRF系统诊断"""
    import epics

    print("=== LLRF系统诊断 ===\n")

    # 1. 检查设定值
    sp_amp = epics.caget(f'{prefix}:Setpoint:Amp')
    sp_phase = epics.caget(f'{prefix}:Setpoint:Phase')
    print(f"设定值: Amp={sp_amp:.2f}, Phase={sp_phase:.2f}°")

    # 2. 检查测量值
    meas_amp = epics.caget(f'{prefix}:Measured:Amp')
    meas_phase = epics.caget(f'{prefix}:Measured:Phase')
    print(f"测量值: Amp={meas_amp:.2f}, Phase={meas_phase:.2f}°")

    # 3. 计算误差
    error_amp = sp_amp - meas_amp
    error_phase = sp_phase - meas_phase
    print(f"误差: Amp={error_amp:.4f}, Phase={error_phase:.4f}°")

    # 4. 检查反馈状态
    fb_enabled = epics.caget(f'{prefix}:Feedback:Enable')
    print(f"反馈状态: {'启用' if fb_enabled else '禁用'}")

    # 5. 判断
    if abs(error_amp/sp_amp) < 0.001 and abs(error_phase) < 0.1:
        print("\n✓ LLRF系统运行正常")
    else:
        print("\n✗ LLRF系统异常，需要检查")

# 使用
diagnose_llrf_system('LLRF:RF:Cavity01')
```

## 7. 小结

### 关键要点

1. **LLRF = 精密RF控制系统**
   - 幅度/相位反馈
   - 束流负载补偿
   - 高稳定性要求

2. **BPM在LLRF中的三种作用**
   - 轨道反馈: 位置测量
   - 相位同步: RF同步检查
   - 流强监控: 束流负载估算

3. **软件开发关注点**
   - 实时性: kHz级反馈
   - 数据同步: 多PV关联
   - 故障诊断: 监控和报警

4. **性能指标**
   - 幅度: <0.01%
   - 相位: <0.01°
   - 带宽: 1-10kHz

### 下一步

继续学习 [05-束流测量技术](./05-beam-measurement.md) 了解更多测量方法。

## 📚 延伸阅读

- [LLRF System Design](https://accelconf.web.cern.ch/)
- [Digital LLRF Control](https://ieeexplore.ieee.org/)
