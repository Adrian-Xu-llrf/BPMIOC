# RF数据模拟详解

> **阅读时间**: 40分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: RF信号基础、三角函数、随机数

## 📋 本文目标

- 理解RF信号的物理特性
- 掌握幅度和相位的生成算法
- 学会添加真实感的噪声和变化
- 实现BPMIOC的RF3-RF6模拟

## 🌊 RF信号基础

### 什么是RF信号？

RF (Radio Frequency) 信号是加速器中用于控制束流的高频信号。

```
物理意义:
RF信号 = 幅度 × cos(2πft + φ)
       ↓      ↓           ↓
     功率   频率       相位
```

**BPMIOC中的RF信号**:
- **RF3-RF6**: 4个RF通道
- **频率**: ~500 MHz (固定)
- **幅度**: 0-10V (变化)
- **相位**: -180°到+180° (缓慢漂移)

### 为什么需要模拟？

```
真实场景:
BPM板卡 → ADC采样 → 幅度/相位计算 → 传给IOC
  ↓
无法在PC上运行

模拟场景:
Mock库 → 数学公式生成 → 幅度/相位 → 传给IOC
  ↓
可以在PC上运行！
```

## 📊 RF数据结构

### 单个RF通道的完整参数

```c
typedef struct {
    // === 基础参数 ===
    double base_amplitude;        // 基础幅度 (V)
    double base_phase;            // 基础相位 (度)
    double center_frequency;      // 中心频率 (MHz)

    // === 幅度变化 ===
    double amp_variation_freq;    // 幅度变化频率 (Hz)
    double amp_variation_depth;   // 幅度变化深度 (0-1)
    double amp_drift_rate;        // 幅度漂移速率 (V/s)

    // === 相位变化 ===
    double phase_drift_rate;      // 相位漂移速率 (度/s)
    double phase_jitter_freq;     // 相位抖动频率 (Hz)
    double phase_jitter_amp;      // 相位抖动幅度 (度)

    // === 噪声 ===
    double amp_noise_level;       // 幅度噪声 (相对值，如0.02=2%)
    double phase_noise_level;     // 相位噪声 (度)

    // === 状态 ===
    int enabled;                  // 是否启用
    double last_amp;              // 上次的幅度（用于平滑）
    double last_phase;            // 上次的相位

} RfChannelConfig;
```

### BPMIOC的4个RF通道初始化

```c
// RF3-RF6的典型参数
static RfChannelConfig g_rf_channels[4] = {
    // RF3 (channel index 0)
    {
        .base_amplitude = 4.0,           // 4V
        .base_phase = 0.0,               // 0度
        .center_frequency = 499.8,       // 499.8 MHz

        .amp_variation_freq = 0.5,       // 0.5 Hz慢速变化
        .amp_variation_depth = 0.1,      // ±10%变化
        .amp_drift_rate = 0.01,          // 0.01 V/s漂移

        .phase_drift_rate = 10.0,        // 10度/秒漂移
        .phase_jitter_freq = 50.0,       // 50 Hz抖动
        .phase_jitter_amp = 5.0,         // ±5度抖动

        .amp_noise_level = 0.02,         // 2%噪声
        .phase_noise_level = 2.0,        // ±2度噪声

        .enabled = 1,
        .last_amp = 4.0,
        .last_phase = 0.0
    },

    // RF4 (channel index 1) - 略有不同的参数
    {
        .base_amplitude = 3.8,
        .base_phase = 45.0,              // 相位偏移45度
        .center_frequency = 499.8,

        .amp_variation_freq = 0.6,
        .amp_variation_depth = 0.12,
        .amp_drift_rate = 0.008,

        .phase_drift_rate = 12.0,
        .phase_jitter_freq = 45.0,
        .phase_jitter_amp = 4.5,

        .amp_noise_level = 0.025,
        .phase_noise_level = 2.5,

        .enabled = 1,
        .last_amp = 3.8,
        .last_phase = 45.0
    },

    // RF5, RF6类似...
};
```

## 🎨 幅度生成算法

### 算法1：分层生成（推荐）

```c
float generateRfAmplitude(RfChannelConfig *cfg, double time)
{
    // ===== Layer 1: 基础值 =====
    float amplitude = cfg->base_amplitude;

    // ===== Layer 2: 长期漂移 =====
    // 模拟功率源的缓慢变化
    amplitude += cfg->amp_drift_rate * time;

    // ===== Layer 3: 慢速周期变化 =====
    // 模拟功率波动（例如电网电压波动）
    double slow_variation = cfg->amp_variation_depth *
                           sin(2.0 * M_PI * cfg->amp_variation_freq * time);
    amplitude *= (1.0 + slow_variation);

    // ===== Layer 4: 快速调制 =====
    // 模拟RF系统的快速调制（可选）
    double fast_modulation = 0.02 * sin(2.0 * M_PI * 10.0 * time);
    amplitude *= (1.0 + fast_modulation);

    // ===== Layer 5: 白噪声 =====
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->amp_noise_level * amplitude;
    amplitude += noise;

    // ===== 限制范围 =====
    if (amplitude < 0.0) amplitude = 0.0;
    if (amplitude > 10.0) amplitude = 10.0;  // 假设最大10V

    // ===== 平滑处理（可选）=====
    // 避免突变
    float alpha = 0.9;  // 平滑系数
    amplitude = alpha * cfg->last_amp + (1.0 - alpha) * amplitude;
    cfg->last_amp = amplitude;

    return amplitude;
}
```

### 示例：RF3的幅度随时间变化

```c
// 模拟10秒的RF3幅度
void demo_rf3_amplitude()
{
    RfChannelConfig *rf3 = &g_rf_channels[0];

    printf("Time(s)  Amplitude(V)  Components\n");
    printf("--------------------------------------------\n");

    for (double t = 0; t < 10.0; t += 0.1) {
        // 分解各个成分
        float base = rf3->base_amplitude;
        float drift = rf3->amp_drift_rate * t;
        float slow = rf3->amp_variation_depth *
                    sin(2.0 * M_PI * rf3->amp_variation_freq * t);
        float noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                     rf3->amp_noise_level * base;

        float total = base * (1.0 + slow) + drift + noise;

        printf("%.1f     %.3f        base=%.2f drift=%.3f slow=%.3f noise=%.3f\n",
               t, total, base, drift, slow, noise);
    }
}

/* 输出示例:
Time(s)  Amplitude(V)  Components
--------------------------------------------
0.0     4.023        base=4.00 drift=0.000 slow=0.000 noise=0.023
0.1     4.027        base=4.00 drift=0.001 slow=0.012 noise=0.014
0.2     4.051        base=4.00 drift=0.002 slow=0.024 noise=0.025
...
1.0     4.387        base=4.00 drift=0.010 slow=0.362 noise=0.015
...
*/
```

## 🔄 相位生成算法

### 算法2：相位模拟

```c
float generateRfPhase(RfChannelConfig *cfg, double time)
{
    // ===== Layer 1: 基础相位 =====
    float phase = cfg->base_phase;

    // ===== Layer 2: 长期漂移 =====
    // 模拟温度变化等导致的缓慢相位漂移
    phase += cfg->phase_drift_rate * time;

    // ===== Layer 3: 慢速周期变化 =====
    // 模拟系统调谐等引起的相位变化
    double slow_change = 30.0 * sin(2.0 * M_PI * 0.1 * time);
    phase += slow_change;

    // ===== Layer 4: 快速抖动 =====
    // 模拟电源噪声等引起的相位抖动
    double jitter = cfg->phase_jitter_amp *
                   sin(2.0 * M_PI * cfg->phase_jitter_freq * time);
    phase += jitter;

    // ===== Layer 5: 白噪声 =====
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->phase_noise_level;
    phase += noise;

    // ===== 归一化到 -180 到 +180 度 =====
    while (phase > 180.0) phase -= 360.0;
    while (phase < -180.0) phase += 360.0;

    // ===== 平滑处理 =====
    float alpha = 0.95;
    phase = alpha * cfg->last_phase + (1.0 - alpha) * phase;
    cfg->last_phase = phase;

    return phase;
}
```

### 相位的物理意义

```c
/*
相位的物理意义:

相位 = 0°    → RF信号与参考信号同相
相位 = 90°   → RF信号超前90度
相位 = 180°  → RF信号反相
相位 = -90°  → RF信号滞后90度

在BPMIOC中:
- 相位漂移: 温度变化、电缆长度变化
- 相位抖动: 电源噪声、时钟抖动
- 相位调整: 通过SetPhaseOffset()控制
*/
```

## 🎯 完整的GetRFInfo()实现

### 主函数

```c
float GetRFInfo(int channel, int type)
{
    // channel: 3-6 (RF3-RF6)
    // type: 0=幅度, 1=相位, 2=实部, 3=虚部

    // ===== 参数验证 =====
    if (!g_config.initialized) {
        fprintf(stderr, "[Mock] Error: Not initialized\n");
        return 0.0;
    }

    if (channel < 3 || channel > 6) {
        fprintf(stderr, "[Mock] Error: Invalid RF channel: %d\n", channel);
        return 0.0;
    }

    // ===== 获取配置 =====
    int ch_idx = channel - 3;  // 3→0, 4→1, 5→2, 6→3
    RfChannelConfig *cfg = &g_rf_channels[ch_idx];

    if (!cfg->enabled) {
        return 0.0;
    }

    double time = g_config.simulation_time;
    float value = 0.0;

    // ===== 根据类型生成数据 =====
    switch (type) {
        case 0:  // 幅度
            value = generateRfAmplitude(cfg, time);
            break;

        case 1:  // 相位
            value = generateRfPhase(cfg, time);
            break;

        case 2:  // 实部 (Real)
            {
                float amp = generateRfAmplitude(cfg, time);
                float phase = generateRfPhase(cfg, time);
                value = amp * cos(phase * M_PI / 180.0);
            }
            break;

        case 3:  // 虚部 (Imaginary)
            {
                float amp = generateRfAmplitude(cfg, time);
                float phase = generateRfPhase(cfg, time);
                value = amp * sin(phase * M_PI / 180.0);
            }
            break;

        default:
            fprintf(stderr, "[Mock] Error: Invalid type: %d\n", type);
            return 0.0;
    }

    // ===== 调试输出 =====
    if (g_config.verbose >= 3) {
        printf("[Mock] GetRFInfo(ch=%d, type=%d) = %.3f\n",
               channel, type, value);
    }

    return value;
}
```

## 📈 高级技巧

### 技巧1：相关性模拟

RF3和RF4应该有一定的相关性（因为来自同一个RF源）：

```c
// 共享的全局相位漂移
static double global_phase_drift = 0.0;

float generateRfPhase(RfChannelConfig *cfg, double time)
{
    // 个体相位
    float phase = cfg->base_phase;

    // 添加全局漂移（所有RF通道共享）
    phase += global_phase_drift;

    // 添加个体漂移
    phase += cfg->phase_drift_rate * time;

    // ... 其他成分

    return phase;
}

// 在TriggerAllDataReached()中更新全局漂移
void TriggerAllDataReached()
{
    global_phase_drift += 0.1 * ((double)rand() / RAND_MAX - 0.5);
    g_config.simulation_time += g_config.time_step;
}
```

### 技巧2：物理模型

使用简单的物理模型增加真实感：

```c
// 温度影响模型
static double temperature = 25.0;  // 环境温度

void updateTemperature(double time)
{
    // 温度缓慢变化
    temperature = 25.0 + 5.0 * sin(2.0 * M_PI * time / 3600.0);
}

float generateRfAmplitude(RfChannelConfig *cfg, double time)
{
    updateTemperature(time);

    float amplitude = cfg->base_amplitude;

    // 温度影响（每度0.1%）
    float temp_effect = 1.0 + (temperature - 25.0) * 0.001;
    amplitude *= temp_effect;

    // ... 其他成分

    return amplitude;
}
```

### 技巧3：故障注入

```c
typedef enum {
    RF_FAULT_NONE = 0,
    RF_FAULT_POWER_LOSS,      // 功率丢失
    RF_FAULT_PHASE_JUMP,      // 相位跳变
    RF_FAULT_INSTABILITY,     // 不稳定
    RF_FAULT_SATURATION       // 饱和
} RfFaultType;

static RfFaultType g_rf_fault = RF_FAULT_NONE;
static int g_fault_channel = -1;

void InjectRfFault(int channel, RfFaultType fault)
{
    g_fault_channel = channel - 3;
    g_rf_fault = fault;
    printf("[Mock] Fault injected: channel=%d, type=%d\n", channel, fault);
}

float generateRfAmplitude(RfChannelConfig *cfg, double time)
{
    float amplitude = /* 正常计算 */;

    // 应用故障
    if (g_fault_channel == (cfg - g_rf_channels)) {
        switch (g_rf_fault) {
            case RF_FAULT_POWER_LOSS:
                amplitude *= 0.1;  // 功率降到10%
                break;

            case RF_FAULT_INSTABILITY:
                amplitude += 0.5 * sin(2.0 * M_PI * 100.0 * time);  // 快速振荡
                break;

            case RF_FAULT_SATURATION:
                if (amplitude > 8.0) amplitude = 8.0;  // 饱和
                break;

            default:
                break;
        }
    }

    return amplitude;
}
```

## 📊 测试和验证

### 测试1：基本功能

```c
void test_rf_basic()
{
    printf("=== Test 1: Basic RF Generation ===\n");

    SystemInit();

    for (int ch = 3; ch <= 6; ch++) {
        float amp = GetRFInfo(ch, 0);
        float phase = GetRFInfo(ch, 1);

        printf("RF%d: Amp=%.3f V, Phase=%.1f deg\n", ch, amp, phase);

        // 验证范围
        assert(amp >= 0.0 && amp <= 10.0);
        assert(phase >= -180.0 && phase <= 180.0);
    }

    printf("PASS\n\n");
}
```

### 测试2：时间变化

```c
void test_rf_time_variation()
{
    printf("=== Test 2: Time Variation ===\n");

    SystemInit();

    float amp_prev = GetRFInfo(3, 0);

    for (int i = 0; i < 10; i++) {
        TriggerAllDataReached();  // 推进时间

        float amp = GetRFInfo(3, 0);
        printf("t=%.1fs: Amp=%.3f V (delta=%.3f)\n",
               g_config.simulation_time, amp, amp - amp_prev);

        // 验证变化合理
        assert(fabs(amp - amp_prev) < 1.0);  // 不会突变太大

        amp_prev = amp;
    }

    printf("PASS\n\n");
}
```

### 测试3：噪声水平

```c
void test_rf_noise()
{
    printf("=== Test 3: Noise Level ===\n");

    SystemInit();

    const int N = 100;
    float samples[N];

    for (int i = 0; i < N; i++) {
        samples[i] = GetRFInfo(3, 0);
        TriggerAllDataReached();
    }

    // 计算均值和标准差
    float mean = 0.0, std = 0.0;
    for (int i = 0; i < N; i++) {
        mean += samples[i];
    }
    mean /= N;

    for (int i = 0; i < N; i++) {
        std += (samples[i] - mean) * (samples[i] - mean);
    }
    std = sqrt(std / N);

    printf("Mean: %.3f V\n", mean);
    printf("Std:  %.3f V (%.1f%%)\n", std, std/mean*100);

    // 验证噪声水平合理（应该在2-5%范围）
    assert(std/mean > 0.01 && std/mean < 0.10);

    printf("PASS\n\n");
}
```

## 🎨 可视化

### 生成CSV数据用于绘图

```c
void export_rf_data_to_csv(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "Time,RF3_Amp,RF3_Phase,RF4_Amp,RF4_Phase\n");

    SystemInit();

    for (int i = 0; i < 1000; i++) {
        double t = g_config.simulation_time;

        fprintf(fp, "%.3f,%.3f,%.1f,%.3f,%.1f\n",
                t,
                GetRFInfo(3, 0),
                GetRFInfo(3, 1),
                GetRFInfo(4, 0),
                GetRFInfo(4, 1));

        TriggerAllDataReached();
    }

    fclose(fp);
    printf("Data exported to %s\n", filename);
}
```

然后用Python绘图：

```python
import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv('rf_data.csv')

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

# 幅度
ax1.plot(data['Time'], data['RF3_Amp'], label='RF3')
ax1.plot(data['Time'], data['RF4_Amp'], label='RF4')
ax1.set_ylabel('Amplitude (V)')
ax1.set_title('RF Amplitude vs Time')
ax1.legend()
ax1.grid(True)

# 相位
ax2.plot(data['Time'], data['RF3_Phase'], label='RF3')
ax2.plot(data['Time'], data['RF4_Phase'], label='RF4')
ax2.set_xlabel('Time (s)')
ax2.set_ylabel('Phase (deg)')
ax2.set_title('RF Phase vs Time')
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.savefig('rf_simulation.png')
plt.show()
```

## ❓ 常见问题

### Q1: 幅度和相位的典型范围是多少？
**A**:
- 幅度: 0-10V（BPMIOC的ADC范围）
- 相位: -180°到+180°（标准相位范围）

### Q2: 噪声应该加多少？
**A**:
- 幅度噪声: 2-5%（太少不真实，太多影响测试）
- 相位噪声: 1-3°

### Q3: 如何让数据更真实？
**A**:
1. 添加多层变化（长期漂移+周期变化+噪声）
2. 模拟物理过程（温度、电源）
3. 添加通道间的相关性
4. 参考真实数据调整参数

### Q4: 性能会有问题吗？
**A**:
不会。每次调用只需要几次sin()和随机数，耗时<1μs，远快于100ms的更新周期。

## 📚 下一步

现在你掌握了RF数据模拟，接下来：

1. [04-xy-position-simulation.md](./04-xy-position-simulation.md) - XY位置模拟
2. [05-complete-mock-implementation.md](./05-complete-mock-implementation.md) - 完整实现

---

**记住**: 好的模拟 = 分层生成 + 合理范围 + 适量噪声 🎯
