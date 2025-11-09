# 05. 束流测量技术

> **目标**: 理解束流诊断方法和测量需求
> **难度**: ⭐⭐
> **预计时间**: 20分钟

## 1. 束流诊断概述

### 1.1 为什么需要束流诊断？

```
优质束流 = 稳定的科学实验

需要测量的参数:
- 位置 (x, y)        → 轨道
- 能量 (E)           → 加速效果
- 发射度 (ε)         → 束流品质
- 流强 (I)           → 粒子数量
- 束团长度 (σt)      → 时间结构
```

### 1.2 束流诊断设备

| 设备 | 测量参数 | 原理 |
|------|----------|------|
| **BPM** | 位置 (x, y) | 电磁感应 |
| **BLM** | 束流损失 | 电离辐射 |
| **ICT** | 流强 (I) | 磁感应 |
| **屏幕站** | 束流剖面 | 荧光屏成像 |
| **光谱仪** | 能量 (E) | 磁偏转 |

**BPMIOC主要关注**: BPM和相关RF测量

## 2. 时域测量

### 2.1 Turn-by-Turn测量

**定义**: 逐圈测量束流位置

**目的**:
- 观察束流振荡
- 测量betatron频率
- 诊断不稳定性

**数据结构**:

```
圈数     X位置    Y位置
────────────────────────
  1      0.05mm   0.02mm
  2      0.08mm   0.01mm
  3      0.04mm   0.03mm
  ...
1000     0.06mm   0.02mm
```

**采样要求**:
- 速率: 每圈一次 (例如: 1MHz @ 1μs)
- 精度: <10μm
- 存储: 连续1000-10000圈

**EPICS IOC实现**:

```c
// Turn-by-Turn数据采集
#define MAX_TURNS 10000

typedef struct {
    int turn_number;
    float pos_x;
    float pos_y;
    uint64_t timestamp;
} TbTData;

static TbTData g_tbt_buffer[MAX_TURNS];
static int g_tbt_index = 0;
static int g_tbt_enabled = 0;

void TbT_AcquireOneTurn(float x, float y) {
    if (!g_tbt_enabled) return;
    if (g_tbt_index >= MAX_TURNS) return;

    g_tbt_buffer[g_tbt_index].turn_number = g_tbt_index;
    g_tbt_buffer[g_tbt_index].pos_x = x;
    g_tbt_buffer[g_tbt_index].pos_y = y;
    g_tbt_buffer[g_tbt_index].timestamp = GetTimestamp_ns();

    g_tbt_index++;

    // 采集完成
    if (g_tbt_index >= MAX_TURNS) {
        g_tbt_enabled = 0;
        printf("TbT acquisition complete\n");
    }
}

// 触发采集
void TbT_Start() {
    g_tbt_index = 0;
    g_tbt_enabled = 1;
}

// 导出数据
void TbT_ExportData(const char *filename) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "Turn,X(mm),Y(mm),Timestamp(ns)\n");

    for (int i = 0; i < g_tbt_index; i++) {
        fprintf(fp, "%d,%.6f,%.6f,%llu\n",
                g_tbt_buffer[i].turn_number,
                g_tbt_buffer[i].pos_x,
                g_tbt_buffer[i].pos_y,
                g_tbt_buffer[i].timestamp);
    }

    fclose(fp);
}
```

### 2.2 波形采集

**应用**: 单次注入、beam dump等瞬态过程

**数据示例**:

```python
import matplotlib.pyplot as plt
import numpy as np

# 模拟注入过程波形
t = np.linspace(0, 10, 10000)  # 10秒，10000点
x = 5.0 * np.exp(-t/2) * np.sin(2*np.pi*0.5*t)  # 衰减振荡

plt.plot(t, x)
plt.xlabel('Time (s)')
plt.ylabel('Position (mm)')
plt.title('Beam Injection Transient')
plt.grid(True)
plt.show()
```

## 3. 频域测量

### 3.1 Betatron频谱

**定义**: 束流横向振荡的频率成分

**物理意义**:
```
Betatron tune (Q) = 每圈振荡次数

例如: Q = 6.25
     → 每圈振荡6.25次
     → 4圈回到初始相位
```

**FFT分析**:

```python
import numpy as np
from scipy import signal

def analyze_betatron_spectrum(tbt_data):
    """分析betatron频谱"""
    # TbT数据 (1000圈)
    x_pos = tbt_data['x']
    n_turns = len(x_pos)

    # FFT
    fft_result = np.fft.fft(x_pos)
    freq = np.fft.fftfreq(n_turns, d=1.0)  # d=1 turn

    # 功率谱
    power = np.abs(fft_result)**2

    # 查找峰值频率
    peak_idx = np.argmax(power[1:n_turns//2]) + 1
    betatron_tune = freq[peak_idx]

    print(f"Betatron Tune: {betatron_tune:.4f}")

    # 绘图
    plt.figure(figsize=(10, 4))

    plt.subplot(121)
    plt.plot(x_pos)
    plt.xlabel('Turn')
    plt.ylabel('Position (mm)')
    plt.title('Time Domain')

    plt.subplot(122)
    plt.plot(freq[:n_turns//2], power[:n_turns//2])
    plt.xlabel('Tune')
    plt.ylabel('Power')
    plt.title('Frequency Domain')
    plt.axvline(betatron_tune, color='r', linestyle='--',
                label=f'Q={betatron_tune:.4f}')
    plt.legend()

    plt.tight_layout()
    plt.show()

    return betatron_tune

# 示例数据
tbt_data = {
    'x': 5.0 * np.exp(-np.arange(1000)/200) *
         np.sin(2*np.pi*6.25*np.arange(1000)/1000)
}

tune = analyze_betatron_spectrum(tbt_data)
```

### 3.2 RF频谱分析

**目的**: 检查RF纯度、寄生模式

```python
def analyze_rf_spectrum(rf_signal, fs):
    """RF信号频谱分析"""
    # 窗函数
    window = signal.hann(len(rf_signal))
    windowed = rf_signal * window

    # FFT
    freq, psd = signal.welch(windowed, fs=fs, nperseg=1024)

    # 查找主峰
    peak_idx = np.argmax(psd)
    rf_freq = freq[peak_idx]

    print(f"主频率: {rf_freq/1e6:.3f} MHz")

    # 查找谐波
    harmonics = []
    for n in [2, 3, 4]:
        harmonic_freq = n * rf_freq
        harmonic_idx = np.argmin(np.abs(freq - harmonic_freq))
        harmonic_power = psd[harmonic_idx]

        harmonic_ratio = harmonic_power / psd[peak_idx]
        print(f"{n}次谐波: {harmonic_ratio*100:.2f}%")

        if harmonic_ratio > 0.01:  # >1%
            print(f"  ⚠ 警告: {n}次谐波过高")

    # 绘图
    plt.semilogy(freq/1e6, psd)
    plt.xlabel('Frequency (MHz)')
    plt.ylabel('PSD')
    plt.title('RF Spectrum')
    plt.grid(True)
    plt.show()
```

## 4. 精度需求

### 4.1 不同应用的精度要求

| 应用 | 位置精度 | 更新率 | 动态范围 |
|------|----------|--------|----------|
| **慢轨道反馈** | 10μm | 10Hz | ±5mm |
| **快轨道反馈** | 50μm | 1kHz | ±2mm |
| **束流研究** | 1μm | 100Hz | ±10mm |
| **机器保护** | 100μm | 100Hz | ±20mm |

### 4.2 精度与SNR的关系

```
位置精度 σ_x ≈ d / (2√2 × SNR)

d = BPM孔径
SNR = 信噪比

例子:
d = 40mm
SNR = 100 (20dB)

σ_x = 40 / (2√2 × 100) = 0.14mm = 140μm
```

**提高精度的方法**:

```python
def improve_position_accuracy(raw_positions, method='average'):
    """提高位置测量精度"""

    if method == 'average':
        # 方法1: 多次平均
        # σ_avg = σ_single / √N
        smoothed = np.convolve(raw_positions,
                              np.ones(10)/10,
                              mode='same')
        return smoothed

    elif method == 'kalman':
        # 方法2: Kalman滤波
        from pykalman import KalmanFilter

        kf = KalmanFilter(
            transition_matrices=[1],
            observation_matrices=[1],
            initial_state_mean=raw_positions[0],
            initial_state_covariance=1,
            observation_covariance=1,
            transition_covariance=0.01
        )

        smoothed, _ = kf.filter(raw_positions)
        return smoothed.flatten()

    elif method == 'savgol':
        # 方法3: Savitzky-Golay滤波
        from scipy.signal import savgol_filter
        smoothed = savgol_filter(raw_positions,
                                window_length=11,
                                polyorder=3)
        return smoothed
```

## 5. 数据采集策略

### 5.1 连续采集 vs. 触发采集

| 模式 | 优点 | 缺点 | 应用 |
|------|------|------|------|
| **连续** | 不丢数据 | 数据量大 | 轨道反馈 |
| **触发** | 节省存储 | 可能丢失 | 故障诊断 |

**触发采集示例**:

```c
// 触发条件: 位置超限
#define TRIGGER_THRESHOLD 1.0  // mm

void CheckTriggerCondition(float x, float y) {
    static int triggered = 0;

    if (!triggered) {
        // 检查触发条件
        if (fabs(x) > TRIGGER_THRESHOLD ||
            fabs(y) > TRIGGER_THRESHOLD) {

            printf("Trigger! x=%.3f, y=%.3f\n", x, y);

            // 启动波形采集
            StartWaveformAcquisition();
            triggered = 1;
        }
    }
}
```

### 5.2 数据压缩

**问题**: 高速采集产生大量数据

**方法1**: 降采样

```python
def decimate_data(data, factor=10):
    """降采样"""
    return data[::factor]

# 1kHz → 100Hz
high_rate_data = np.random.randn(10000)  # 10s @ 1kHz
low_rate_data = decimate_data(high_rate_data, factor=10)
```

**方法2**: Delta压缩

```python
def delta_compress(data, threshold=0.01):
    """Delta压缩: 仅存储变化超过阈值的数据"""
    compressed = [data[0]]  # 第一个点
    indices = [0]

    for i in range(1, len(data)):
        if abs(data[i] - compressed[-1]) > threshold:
            compressed.append(data[i])
            indices.append(i)

    print(f"压缩率: {len(compressed)/len(data)*100:.1f}%")

    return np.array(compressed), np.array(indices)

# 示例
data = 10.0 + 0.01 * np.random.randn(10000)
compressed, indices = delta_compress(data, threshold=0.05)
```

## 6. 实时监控

### 6.1 在线数据质量监控

```python
import epics
import time
import numpy as np

class BeamMonitor:
    """实时束流监控"""

    def __init__(self, pv_prefix):
        self.pv_x = epics.PV(f'{pv_prefix}:PosX')
        self.pv_y = epics.PV(f'{pv_prefix}:PosY')

        self.history_x = []
        self.history_y = []

    def check_orbit_stability(self, duration=60):
        """检查轨道稳定性"""
        print(f"监控{duration}秒...")

        self.history_x = []
        self.history_y = []

        start_time = time.time()
        while time.time() - start_time < duration:
            self.history_x.append(self.pv_x.get())
            self.history_y.append(self.pv_y.get())
            time.sleep(0.1)  # 10Hz

        # 统计分析
        x_arr = np.array(self.history_x)
        y_arr = np.array(self.history_y)

        x_mean = np.mean(x_arr)
        x_std = np.std(x_arr)
        x_rms = np.sqrt(np.mean(x_arr**2))

        print(f"\nX轴统计:")
        print(f"  均值: {x_mean:.4f} mm")
        print(f"  标准差: {x_std:.4f} mm")
        print(f"  RMS: {x_rms:.4f} mm")

        # 判断稳定性
        if x_std < 0.01:  # <10μm
            print("  ✓ 稳定性良好")
        else:
            print("  ✗ 稳定性较差")

        return x_std

# 使用
monitor = BeamMonitor('LLRF:BPM:01')
stability = monitor.check_orbit_stability(duration=60)
```

### 6.2 异常检测

```python
def detect_anomalies(data, threshold=3.0):
    """异常检测 (3-sigma方法)"""
    mean = np.mean(data)
    std = np.std(data)

    # 异常: 超过3倍标准差
    anomalies = np.abs(data - mean) > threshold * std

    n_anomalies = np.sum(anomalies)
    print(f"检测到 {n_anomalies} 个异常点 ({n_anomalies/len(data)*100:.2f}%)")

    return anomalies

# 应用到实时监控
def monitor_with_anomaly_detection(pv_name, duration=60):
    pv = epics.PV(pv_name)

    data = []
    for _ in range(int(duration*10)):  # 10Hz
        data.append(pv.get())
        time.sleep(0.1)

    data = np.array(data)
    anomalies = detect_anomalies(data, threshold=3.0)

    # 绘图
    plt.plot(data, label='Data')
    plt.plot(np.where(anomalies)[0], data[anomalies], 'ro',
            label='Anomalies')
    plt.xlabel('Sample')
    plt.ylabel('Position (mm)')
    plt.legend()
    plt.grid(True)
    plt.show()
```

## 7. 数据归档与分析

### 7.1 长期数据存储

**Archiver Appliance集成**:

```python
import requests
import datetime

def retrieve_historical_data(pv_name, start_time, end_time):
    """从Archiver检索历史数据"""
    url = "http://archiver.example.com:17668/retrieval/data/getData.json"

    params = {
        'pv': pv_name,
        'from': start_time.isoformat(),
        'to': end_time.isoformat()
    }

    response = requests.get(url, params=params)
    data = response.json()

    # 解析数据
    timestamps = []
    values = []

    for point in data[0]['data']:
        timestamps.append(point['secs'])
        values.append(point['val'])

    return np.array(timestamps), np.array(values)

# 示例: 检索过去24小时的数据
end = datetime.datetime.now()
start = end - datetime.timedelta(days=1)

timestamps, values = retrieve_historical_data(
    'LLRF:BPM:01:PosX', start, end
)

# 分析轨道漂移
drift = values[-100:].mean() - values[:100].mean()
print(f"24小时轨道漂移: {drift:.4f} mm")
```

### 7.2 机器学习应用

```python
from sklearn.ensemble import IsolationForest

def ml_anomaly_detection(historical_data):
    """使用机器学习检测异常"""
    # 训练Isolation Forest模型
    model = IsolationForest(contamination=0.01)  # 1%异常

    # 特征: 位置及其统计量
    features = []
    for i in range(len(historical_data) - 10):
        window = historical_data[i:i+10]
        features.append([
            np.mean(window),
            np.std(window),
            np.max(window) - np.min(window)
        ])

    features = np.array(features)
    model.fit(features)

    # 预测
    predictions = model.predict(features)
    anomalies = predictions == -1

    print(f"ML检测到 {np.sum(anomalies)} 个异常")

    return anomalies
```

## 8. 小结

### 关键要点

1. **束流测量模式**
   - 时域: Turn-by-Turn, 波形
   - 频域: Betatron谱, RF谱

2. **精度需求**
   - 位置: 1-100μm (应用相关)
   - 更新率: 10Hz-1kHz
   - SNR决定精度

3. **数据处理**
   - 滤波: 平均、Kalman、Savgol
   - 压缩: 降采样、Delta
   - 异常检测: 3-sigma, ML

4. **软件实现**
   - 实时采集和监控
   - 历史数据归档
   - 数据质量检查

### 总结

恭喜完成Part 17 - 物理背景！现在你应该理解了：
- BPM系统的物理基础
- 测量参数的物理意义
- 精度需求的来源
- 数据处理方法

这些知识将帮助你更好地设计和优化EPICS IOC软件。

## 📚 延伸阅读

- [Beam Diagnostics (CERN School)](https://cas.web.cern.ch/)
- [Beam Instrumentation Handbook](https://www.springer.com/)
