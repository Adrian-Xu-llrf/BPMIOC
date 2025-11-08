# Lab 05: 波形数据读取与可视化

> **目标**: 掌握BPMIOC波形数据的读取和可视化
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 60分钟
> **前置知识**: Lab 01, Part 2: 08-ca-tools.md

## 📋 实验目标

完成本实验后，你将能够：
- ✅ 使用caget读取波形PV
- ✅ 使用Python pyepics库读取波形
- ✅ 使用matplotlib绘制波形图
- ✅ 实现实时波形监控
- ✅ 分析波形数据特征
- ✅ 保存和导出波形数据

## 🎯 背景知识

### BPMIOC的波形数据

BPMIOC提供三种类型的波形：

| 类型 | PV示例 | 数据点数 | 用途 |
|------|--------|---------|------|
| **TrigWaveform** | RFIn_01_TrigWaveform | 10,000 | 触发采集波形 |
| **TripWaveform** | RFIn_01_TripWaveform | 100,000 | Trip事件波形 |
| **BkgWaveform** | RFIn_01_BkgWaveform | 10,000 | 背景波形 |

### 数据库定义

```bash
cd ~/BPMIOC
grep -A 8 "TrigWaveform" BPMmonitorApp/Db/BPMMonitor.db | head -20
```

典型定义：
```
record(waveform, "$(P):RFIn_01_TrigWaveform")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@ARRAY:1")
    field(FTVL, "FLOAT")      # Field Type: FLOAT
    field(NELM, "10000")      # Number of Elements: 10000
    field(PREC, "3")
    field(EGU,  "V")
}
```

## 🔧 实验一: 使用caget读取波形

### 步骤1: 检查IOC状态

确保BPMIOC正在运行：

```bash
# 检查IOC是否运行
caget iLinac_007:BPM14And15:RFIn_01_Amp

# 如果未运行，启动IOC
cd ~/BPMIOC/iocBoot/iocBPMmonitor
./st.cmd &
```

### 步骤2: 读取波形数据

```bash
# 读取前10个点
caget -# 10 iLinac_007:BPM14And15:RFIn_01_TrigWaveform

# 输出示例:
# iLinac_007:BPM14And15:RFIn_01_TrigWaveform 10 1.234 1.235 1.236 1.237 ...
```

### 步骤3: 读取完整波形并保存

```bash
# 读取全部10000点并保存到文件
caget -# 10000 iLinac_007:BPM14And15:RFIn_01_TrigWaveform > waveform_raw.txt

# 查看文件
head -3 waveform_raw.txt
# iLinac_007:BPM14And15:RFIn_01_TrigWaveform 10000 1.234 1.235 1.236 ...
```

### 步骤4: 提取数值数据

```bash
# 提取纯数值（去掉PV名和元素数）
caget -# 10000 -n iLinac_007:BPM14And15:RFIn_01_TrigWaveform | \
    awk '{for(i=3;i<=NF;i++) print $i}' > waveform_data.txt

# 查看文件
head -5 waveform_data.txt
# 1.234
# 1.235
# 1.236
# 1.237
# 1.238
```

## 🐍 实验二: 使用Python读取波形

### 步骤1: 安装pyepics

```bash
# 安装pyepics库
pip3 install pyepics

# 或者使用系统包管理器
sudo apt-get install python3-epics  # Ubuntu/Debian
```

### 步骤2: 基本波形读取

创建 `read_waveform.py`:

```python
#!/usr/bin/env python3
import epics
import time

# 配置CA环境（如果需要）
import os
os.environ['EPICS_CA_ADDR_LIST'] = 'localhost'
os.environ['EPICS_CA_AUTO_ADDR_LIST'] = 'NO'

# 定义PV名称
PREFIX = "iLinac_007:BPM14And15"
waveform_pv = f"{PREFIX}:RFIn_01_TrigWaveform"

print(f"Reading waveform from {waveform_pv}...")

# 读取波形
waveform = epics.caget(waveform_pv)

if waveform is None:
    print("ERROR: Failed to read waveform!")
    exit(1)

print(f"Successfully read {len(waveform)} points")
print(f"First 10 points: {waveform[:10]}")
print(f"Last 10 points: {waveform[-10:]}")
print(f"Min value: {min(waveform):.3f} V")
print(f"Max value: {max(waveform):.3f} V")
print(f"Mean value: {sum(waveform)/len(waveform):.3f} V")
```

运行：
```bash
chmod +x read_waveform.py
./read_waveform.py
```

**输出示例**:
```
Reading waveform from iLinac_007:BPM14And15:RFIn_01_TrigWaveform...
Successfully read 10000 points
First 10 points: [1.234, 1.235, 1.236, ...]
Last 10 points: [1.240, 1.241, 1.242, ...]
Min value: 0.856 V
Max value: 1.678 V
Mean value: 1.234 V
```

### 步骤3: 读取多个波形

创建 `read_all_rf.py`:

```python
#!/usr/bin/env python3
import epics

PREFIX = "iLinac_007:BPM14And15"

# 读取所有8个RF通道的波形
rf_channels = range(1, 9)  # RF1 到 RF8
waveforms = {}

print("Reading RF waveforms...")
for ch in rf_channels:
    pv_name = f"{PREFIX}:RFIn_{ch:02d}_TrigWaveform"
    print(f"  Reading {pv_name}...")

    wf = epics.caget(pv_name)

    if wf is not None:
        waveforms[f"RF{ch}"] = wf
        print(f"    ✓ Got {len(wf)} points")
    else:
        print(f"    ✗ Failed to read")

print(f"\nTotal channels read: {len(waveforms)}")

# 统计信息
print("\nStatistics:")
for name, wf in waveforms.items():
    print(f"{name}: min={min(wf):.3f}, max={max(wf):.3f}, mean={sum(wf)/len(wf):.3f} V")
```

## 📊 实验三: 使用matplotlib绘图

### 步骤1: 安装matplotlib

```bash
pip3 install matplotlib numpy
```

### 步骤2: 绘制单个波形

创建 `plot_waveform.py`:

```python
#!/usr/bin/env python3
import epics
import matplotlib.pyplot as plt
import numpy as np

PREFIX = "iLinac_007:BPM14And15"
pv_name = f"{PREFIX}:RFIn_01_TrigWaveform"

print(f"Reading and plotting {pv_name}...")

# 读取波形
waveform = epics.caget(pv_name)

if waveform is None:
    print("ERROR: Failed to read waveform")
    exit(1)

# 创建时间轴（假设采样率）
# 实际采样率需要从PV读取
sampling_rate = 100e6  # 100 MHz (示例)
time_us = np.arange(len(waveform)) / sampling_rate * 1e6  # 微秒

# 绘图
plt.figure(figsize=(12, 6))

# 完整波形
plt.subplot(2, 1, 1)
plt.plot(time_us, waveform, linewidth=0.5)
plt.xlabel('Time (μs)')
plt.ylabel('Amplitude (V)')
plt.title(f'{pv_name} - Full Waveform ({len(waveform)} points)')
plt.grid(True, alpha=0.3)

# 放大前1000个点
plt.subplot(2, 1, 2)
plt.plot(time_us[:1000], waveform[:1000], linewidth=1)
plt.xlabel('Time (μs)')
plt.ylabel('Amplitude (V)')
plt.title('Zoomed View (First 1000 points)')
plt.grid(True, alpha=0.3)

plt.tight_layout()

# 保存图片
output_file = 'waveform_plot.png'
plt.savefig(output_file, dpi=150)
print(f"Plot saved to {output_file}")

# 显示图片（如果有显示）
# plt.show()
```

运行：
```bash
./plot_waveform.py
```

生成的图片 `waveform_plot.png` 包含两个子图：
- 上图：完整10000点波形
- 下图：前1000点的放大视图

### 步骤3: 绘制多通道对比

创建 `plot_all_rf.py`:

```python
#!/usr/bin/env python3
import epics
import matplotlib.pyplot as plt
import numpy as np

PREFIX = "iLinac_007:BPM14And15"

# 读取所有RF通道
rf_channels = [1, 2, 3, 4, 5, 6, 7, 8]
waveforms = {}

print("Reading all RF channels...")
for ch in rf_channels:
    pv_name = f"{PREFIX}:RFIn_{ch:02d}_TrigWaveform"
    wf = epics.caget(pv_name)
    if wf is not None:
        waveforms[f"RF{ch}"] = wf
        print(f"  RF{ch}: ✓")

# 绘制4x2子图
fig, axes = plt.subplots(4, 2, figsize=(14, 10))
fig.suptitle('RF Channels Comparison', fontsize=16)

for idx, (name, wf) in enumerate(waveforms.items()):
    row = idx // 2
    col = idx % 2
    ax = axes[row, col]

    # 只绘制前1000点以提高性能
    time_ms = np.arange(1000) * 0.01  # 假设采样间隔10us
    ax.plot(time_ms, wf[:1000], linewidth=0.8)
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('Amplitude (V)')
    ax.set_title(name)
    ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('all_rf_channels.png', dpi=150)
print("Plot saved to all_rf_channels.png")
```

## 🔄 实验四: 实时波形监控

### 步骤1: 创建实时更新脚本

创建 `monitor_waveform.py`:

```python
#!/usr/bin/env python3
import epics
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np

PREFIX = "iLinac_007:BPM14And15"
pv_name = f"{PREFIX}:RFIn_01_TrigWaveform"

# 创建PV对象
pv = epics.PV(pv_name)

# 准备绘图
fig, ax = plt.subplots(figsize=(12, 6))
line, = ax.plot([], [], linewidth=0.8)

ax.set_xlabel('Sample Index')
ax.set_ylabel('Amplitude (V)')
ax.set_title(f'Real-time Waveform: {pv_name}')
ax.grid(True, alpha=0.3)

# 初始化函数
def init():
    line.set_data([], [])
    return line,

# 更新函数
def update(frame):
    # 读取最新波形
    wf = pv.get()

    if wf is not None:
        # 只显示前2000点
        x = np.arange(min(2000, len(wf)))
        y = wf[:len(x)]

        line.set_data(x, y)

        # 自动调整y轴范围
        if len(y) > 0:
            margin = 0.1 * (max(y) - min(y))
            ax.set_ylim(min(y) - margin, max(y) + margin)

        ax.set_xlim(0, len(x))

    return line,

# 创建动画（每500ms更新一次）
ani = animation.FuncAnimation(fig, update, init_func=init,
                             interval=500, blit=True)

print(f"Monitoring {pv_name}")
print("Close the plot window to stop")
plt.show()
```

运行：
```bash
./monitor_waveform.py
```

这会打开一个窗口，每500ms更新一次波形显示。

### 步骤2: 优化的实时监控（使用回调）

创建 `monitor_callback.py`:

```python
#!/usr/bin/env python3
import epics
import numpy as np
from collections import deque
import time

PREFIX = "iLinac_007:BPM14And15"
pv_name = f"{PREFIX}:RFIn_01_TrigWaveform"

# 存储最近的统计信息
stats_history = deque(maxlen=10)  # 保留最近10次

def waveform_callback(pvname=None, value=None, **kws):
    """波形更新时的回调函数"""

    if value is None or len(value) == 0:
        return

    # 计算统计信息
    stats = {
        'time': time.time(),
        'min': float(np.min(value)),
        'max': float(np.max(value)),
        'mean': float(np.mean(value)),
        'std': float(np.std(value)),
        'points': len(value)
    }

    stats_history.append(stats)

    # 打印最新统计
    print(f"\r[{time.strftime('%H:%M:%S')}] "
          f"Min: {stats['min']:6.3f}V  "
          f"Max: {stats['max']:6.3f}V  "
          f"Mean: {stats['mean']:6.3f}V  "
          f"Std: {stats['std']:6.3f}V  "
          f"Points: {stats['points']:5d}", end='')

# 创建PV并设置回调
print(f"Monitoring {pv_name}")
print("Press Ctrl+C to stop\n")

pv = epics.PV(pv_name, callback=waveform_callback, auto_monitor=True)

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\n\nStopped by user")

    # 打印统计摘要
    if len(stats_history) > 0:
        print("\nLast 10 updates summary:")
        print("  Time       Min     Max     Mean    Std")
        for s in stats_history:
            t = time.strftime('%H:%M:%S', time.localtime(s['time']))
            print(f"  {t}  {s['min']:6.3f}  {s['max']:6.3f}  "
                  f"{s['mean']:6.3f}  {s['std']:6.3f}")
```

## 📈 实验五: 波形分析

### 步骤1: FFT频谱分析

创建 `analyze_spectrum.py`:

```python
#!/usr/bin/env python3
import epics
import numpy as np
import matplotlib.pyplot as plt

PREFIX = "iLinac_007:BPM14And15"
pv_name = f"{PREFIX}:RFIn_01_TrigWaveform"

# 读取波形
print(f"Reading {pv_name}...")
waveform = epics.caget(pv_name)

if waveform is None:
    print("ERROR: Failed to read waveform")
    exit(1)

# 假设采样率
fs = 100e6  # 100 MHz

# 计算FFT
fft_result = np.fft.fft(waveform)
freqs = np.fft.fftfreq(len(waveform), 1/fs)

# 只取正频率部分
positive_freqs = freqs[:len(freqs)//2]
magnitude = np.abs(fft_result[:len(fft_result)//2])

# 转换为dB
magnitude_db = 20 * np.log10(magnitude + 1e-10)

# 绘图
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

# 时域
ax1.plot(waveform[:1000])
ax1.set_xlabel('Sample')
ax1.set_ylabel('Amplitude (V)')
ax1.set_title('Time Domain (First 1000 samples)')
ax1.grid(True, alpha=0.3)

# 频域
ax2.plot(positive_freqs/1e6, magnitude_db)  # MHz
ax2.set_xlabel('Frequency (MHz)')
ax2.set_ylabel('Magnitude (dB)')
ax2.set_title('Frequency Spectrum')
ax2.set_xlim(0, fs/2/1e6)
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('spectrum_analysis.png', dpi=150)
print("Spectrum analysis saved to spectrum_analysis.png")
```

### 步骤2: 统计分析

创建 `analyze_stats.py`:

```python
#!/usr/bin/env python3
import epics
import numpy as np
import matplotlib.pyplot as plt

PREFIX = "iLinac_007:BPM14And15"

# 读取多个通道进行对比分析
channels = [1, 2, 3, 4, 5, 6, 7, 8]
stats = []

for ch in channels:
    pv_name = f"{PREFIX}:RFIn_{ch:02d}_TrigWaveform"
    wf = epics.caget(pv_name)

    if wf is not None:
        stats.append({
            'channel': f'RF{ch}',
            'mean': np.mean(wf),
            'std': np.std(wf),
            'min': np.min(wf),
            'max': np.max(wf),
            'rms': np.sqrt(np.mean(wf**2))
        })

# 打印统计表
print("\nRF Channels Statistics:")
print("Channel  Mean(V)  Std(V)   Min(V)   Max(V)   RMS(V)")
print("-" * 60)
for s in stats:
    print(f"{s['channel']:7s}  {s['mean']:6.3f}  {s['std']:6.3f}  "
          f"{s['min']:6.3f}  {s['max']:6.3f}  {s['rms']:6.3f}")

# 绘制统计对比图
fig, axes = plt.subplots(2, 2, figsize=(12, 8))

channels_list = [s['channel'] for s in stats]

# Mean
axes[0, 0].bar(channels_list, [s['mean'] for s in stats])
axes[0, 0].set_ylabel('Mean (V)')
axes[0, 0].set_title('Mean Values')
axes[0, 0].grid(True, alpha=0.3)

# Std
axes[0, 1].bar(channels_list, [s['std'] for s in stats])
axes[0, 1].set_ylabel('Std (V)')
axes[0, 1].set_title('Standard Deviation')
axes[0, 1].grid(True, alpha=0.3)

# Min/Max Range
axes[1, 0].bar(channels_list, [s['max'] - s['min'] for s in stats])
axes[1, 0].set_ylabel('Range (V)')
axes[1, 0].set_title('Min-Max Range')
axes[1, 0].grid(True, alpha=0.3)

# RMS
axes[1, 1].bar(channels_list, [s['rms'] for s in stats])
axes[1, 1].set_ylabel('RMS (V)')
axes[1, 1].set_title('RMS Values')
axes[1, 1].grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('stats_comparison.png', dpi=150)
print("\nStatistics comparison saved to stats_comparison.png")
```

## 💾 实验六: 数据导出

### 步骤1: 导出为CSV

创建 `export_csv.py`:

```python
#!/usr/bin/env python3
import epics
import csv
import time

PREFIX = "iLinac_007:BPM14And15"
channels = range(1, 9)

# 读取所有通道
print("Reading all RF channels...")
data = {}

for ch in channels:
    pv_name = f"{PREFIX}:RFIn_{ch:02d}_TrigWaveform"
    wf = epics.caget(pv_name)
    if wf is not None:
        data[f'RF{ch}'] = wf
        print(f"  RF{ch}: {len(wf)} points")

# 导出为CSV
filename = f"waveform_export_{time.strftime('%Y%m%d_%H%M%S')}.csv"

with open(filename, 'w', newline='') as csvfile:
    # 写入头部
    fieldnames = ['Sample'] + list(data.keys())
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()

    # 写入数据
    max_len = max(len(wf) for wf in data.values())
    for i in range(max_len):
        row = {'Sample': i}
        for name, wf in data.items():
            if i < len(wf):
                row[name] = f"{wf[i]:.6f}"
        writer.writerow(row)

print(f"\nData exported to {filename}")
print(f"File size: {os.path.getsize(filename) / 1024:.1f} KB")
```

### 步骤2: 导出为NumPy格式

创建 `export_numpy.py`:

```python
#!/usr/bin/env python3
import epics
import numpy as np
import time

PREFIX = "iLinac_007:BPM14And15"
channels = range(1, 9)

# 读取所有通道
waveforms = []
names = []

for ch in channels:
    pv_name = f"{PREFIX}:RFIn_{ch:02d}_TrigWaveform"
    wf = epics.caget(pv_name)
    if wf is not None:
        waveforms.append(wf)
        names.append(f'RF{ch}')

# 转换为NumPy数组
data_array = np.array(waveforms)

# 保存为.npz文件（压缩格式）
filename = f"waveform_{time.strftime('%Y%m%d_%H%M%S')}.npz"
np.savez_compressed(filename,
                    data=data_array,
                    channels=names,
                    timestamp=time.time())

print(f"Data saved to {filename}")
print(f"Shape: {data_array.shape}")
print(f"Channels: {names}")

# 验证读取
loaded = np.load(filename)
print(f"\nVerification:")
print(f"  Loaded data shape: {loaded['data'].shape}")
print(f"  Channels: {list(loaded['channels'])}")
```

## 🎯 综合练习

### 练习1: 创建完整的波形采集和分析工具

要求：
1. 读取所有8个RF通道波形
2. 计算每个通道的统计信息
3. 绘制对比图
4. 导出数据为CSV
5. 生成PDF报告

<details>
<summary>提示</summary>

可以组合之前的脚本，并添加：
- argparse用于命令行参数
- reportlab生成PDF
- 多线程加速数据读取
</details>

### 练习2: 波形异常检测

要求：
1. 持续监控波形
2. 检测异常值（超过3个标准差）
3. 发送报警通知
4. 记录异常事件

### 练习3: 波形对比工具

要求：
1. 读取两个不同时间的波形
2. 计算差异
3. 可视化变化
4. 生成对比报告

## 📚 常用工具函数库

创建 `waveform_utils.py`:

```python
"""BPMIOC波形工具函数库"""
import epics
import numpy as np

def read_waveform(pv_name, timeout=5.0):
    """读取单个波形PV"""
    return epics.caget(pv_name, timeout=timeout)

def read_all_rf_waveforms(prefix, channels=range(1, 9)):
    """读取所有RF通道波形"""
    waveforms = {}
    for ch in channels:
        pv_name = f"{prefix}:RFIn_{ch:02d}_TrigWaveform"
        wf = read_waveform(pv_name)
        if wf is not None:
            waveforms[f'RF{ch}'] = wf
    return waveforms

def calc_stats(waveform):
    """计算波形统计信息"""
    return {
        'min': float(np.min(waveform)),
        'max': float(np.max(waveform)),
        'mean': float(np.mean(waveform)),
        'std': float(np.std(waveform)),
        'rms': float(np.sqrt(np.mean(waveform**2))),
        'points': len(waveform)
    }

def detect_outliers(waveform, threshold=3.0):
    """检测异常点（超过N个标准差）"""
    mean = np.mean(waveform)
    std = np.std(waveform)
    outliers = np.abs(waveform - mean) > threshold * std
    return np.where(outliers)[0]

# 使用示例
if __name__ == '__main__':
    PREFIX = "iLinac_007:BPM14And15"
    waveforms = read_all_rf_waveforms(PREFIX)

    for name, wf in waveforms.items():
        stats = calc_stats(wf)
        outliers = detect_outliers(wf)
        print(f"{name}: mean={stats['mean']:.3f}V, "
              f"std={stats['std']:.3f}V, outliers={len(outliers)}")
```

## 🔗 相关文档

- [Part 2: 04-record-types.md](../../part2-understanding-basics/04-record-types.md) - waveform记录类型
- [Part 2: 08-ca-tools.md](../../part2-understanding-basics/08-ca-tools.md) - CA工具
- [Part 6: 09-waveform-records.md](../../part6-database-layer/09-waveform-records.md) - 波形记录详解

## 📝 实验报告模板

```markdown
# Lab 05 实验报告

## 实验一：caget读取
- 成功读取的波形PV：RFIn_01_TrigWaveform
- 数据点数：10000
- 数据范围：0.856 - 1.678 V

## 实验二：Python读取
- 使用的库：pyepics
- 读取的通道数：8
- 遇到的问题：无

## 实验三：matplotlib绘图
- 生成的图片数量：3
- 图片类型：时域波形、频谱分析、多通道对比
- [贴上你的图片]

## 实验四：实时监控
- 监控时长：5分钟
- 更新频率：每500ms
- 观察到的变化：[描述]

## 实验五：波形分析
- FFT峰值频率：XX MHz
- 平均幅度：XX V
- 标准差：XX V

## 收获和体会
...
```

## 📊 总结

### 关键要点

1. **读取方法**:
   - caget: 命令行快速查看
   - pyepics: Python编程接口

2. **可视化**:
   - matplotlib: 静态图和动画
   - 实时监控: 回调函数

3. **数据分析**:
   - 统计分析: mean/std/min/max
   - FFT频谱分析
   - 异常检测

4. **数据导出**:
   - CSV: 通用格式
   - NumPy: 高效存储

### 最佳实践

- ✅ 读取大波形时设置合理的超时
- ✅ 使用回调函数实现高效监控
- ✅ 只绘制必要的数据点以提高性能
- ✅ 导出数据时添加时间戳和元数据

---

**🎉 恭喜完成实验！** 你已经掌握了BPMIOC波形数据的读取、可视化和分析！
