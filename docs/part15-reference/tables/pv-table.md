# PV 完整参考表

> **说明**: 本表列出BPMIOC的所有主要PV
> **前缀**: `iLinac_007:BPM14And15` (可通过宏配置)
> **总数**: 200+ PV

## 🔍 快速查找

- [RF监测PV](#rf监测pv) - RF3-RF10振幅、相位、功率
- [波形PV](#波形pv) - 触发波形、历史波形
- [波形分析PV](#波形分析pv) - 平均电压、背景设置
- [BPM位置PV](#bpm位置pv) - X/Y位置、Sum值
- [系统控制PV](#系统控制pv) - 触发、配置
- [状态监测PV](#状态监测pv) - LED、数字输入

## RF监测PV

### RF3 通道 (channel=0)

| PV名称 | 类型 | 单位 | 范围 | 扫描 | 描述 |
|--------|------|------|------|------|------|
| `$(P):RF3Amp` | ai | V | 0-2 | I/O Intr | RF3振幅 |
| `$(P):RF3Phase` | ai | 度 | 0-360 | I/O Intr | RF3相位 |
| `$(P):RF3Power` | ai | KW | 0-100 | 0.5s | RF3功率(计算) |
| `$(P):RF3TrigWaveform` | waveform | V | - | I/O Intr | RF3触发波形(10000点) |
| `$(P):RF3TripWaveform` | waveform | V | - | Passive | RF3历史波形 |
| `$(P):RF3AVGVoltage` | ai | V | - | I/O Intr | RF3平均电压 |
| `$(P):RF3AVGStart` | ao | - | 0-10000 | Passive | 平均起始位置 |
| `$(P):RF3AVGStop` | ao | - | 0-10000 | Passive | 平均结束位置 |
| `$(P):RF3BackGroundStart` | ao | - | 0-10000 | Passive | 背景起始位置 |
| `$(P):RF3BackGroundStop` | ao | - | 0-10000 | Passive | 背景结束位置 |

**数据流**:
```
FPGA → my_thread() → Amp[0] → ReadData(OFFSET_AMP, 0) → $(P):RF3Amp
```

### RF4-RF10 通道

**说明**: RF4-RF10 的PV结构与RF3完全相同，只是通道号不同

| 通道 | 前缀 | channel | 示例PV |
|------|------|---------|--------|
| RF4 | `$(P):RF4` | 1 | `$(P):RF4Amp` |
| RF5 | `$(P):RF5` | 2 | `$(P):RF5Amp` |
| RF6 | `$(P):RF6` | 3 | `$(P):RF6Amp` |
| RF7 | `$(P):RF7` | 4 | `$(P):RF7Amp` |
| RF8 | `$(P):RF8` | 5 | `$(P):RF8Amp` |
| RF9 | `$(P):RF9` | 6 | `$(P):RF9Amp` |
| RF10 | `$(P):RF10` | 7 | `$(P):RF10Amp` |

**总计**: 8个通道 × 10个PV/通道 = **80个RF监测PV**

## 波形PV

### 触发波形 (TrigWaveform)

| PV名称 | FTVL | NELM | 描述 |
|--------|------|------|------|
| `$(P):RF3TrigWaveform` | FLOAT | 10000 | RF3触发波形 |
| `$(P):RF4TrigWaveform` | FLOAT | 10000 | RF4触发波形 |
| `$(P):RF5TrigWaveform` | FLOAT | 10000 | RF5触发波形 |
| `$(P):RF6TrigWaveform` | FLOAT | 10000 | RF6触发波形 |
| `$(P):RF7TrigWaveform` | FLOAT | 10000 | RF7触发波形 |
| `$(P):RF8TrigWaveform` | FLOAT | 10000 | RF8触发波形 |
| `$(P):RF9TrigWaveform` | FLOAT | 10000 | RF9触发波形 |
| `$(P):RF10TrigWaveform` | FLOAT | 10000 | RF10触发波形 |

**读取示例**:
```bash
# 读取完整波形
caget -# 10000 iLinac_007:BPM14And15:RF3TrigWaveform

# Python读取
import epics
wf = epics.caget('iLinac_007:BPM14And15:RF3TrigWaveform')
print(f"波形长度: {len(wf)}")
```

### 历史波形 (TripWaveform)

| PV名称 | FTVL | NELM | 描述 |
|--------|------|------|------|
| `$(P):RF3TripWaveform` | FLOAT | 100000 | RF3历史波形 |
| ... | ... | ... | RF4-RF10类似 |

**总计**: 16个波形PV (8个触发 + 8个历史)

## 波形分析PV

### 平均电压计算 (最新功能)

| PV名称 | 类型 | 单位 | 描述 |
|--------|------|------|------|
| `$(P):RF3AVGVoltage` | ai | V | RF3平均电压(信号-背景) |
| `$(P):RF3AVGStart` | ao | - | 信号区域起始点 |
| `$(P):RF3AVGStop` | ao | - | 信号区域结束点 |
| `$(P):RF3BackGroundStart` | ao | - | 背景区域起始点 |
| `$(P):RF3BackGroundStop` | ao | - | 背景区域结束点 |

**使用示例**:
```bash
# 1. 设置信号区域 (假设信号在2000-3000点)
caput iLinac_007:BPM14And15:RF3AVGStart 2000
caput iLinac_007:BPM14And15:RF3AVGStop 3000

# 2. 设置背景区域 (假设背景在0-500点)
caput iLinac_007:BPM14And15:RF3BackGroundStart 0
caput iLinac_007:BPM14And15:RF3BackGroundStop 500

# 3. 读取平均电压
caget iLinac_007:BPM14And15:RF3AVGVoltage
```

**算法**:
```
AVG_Voltage = mean(waveform[AVGStart:AVGStop])
            - mean(waveform[BackGroundStart:BackGroundStop])
```

**总计**: 5个PV/通道 × 8通道 = **40个波形分析PV**

## BPM位置PV

### BPM14 ($(P1), channel=0)

| PV名称 | 类型 | 单位 | 范围 | 描述 |
|--------|------|------|------|------|
| `$(P1):VcValue` | ai | - | - | Vc值 |
| `$(P1):XPos` | ai | mm | ±10 | X位置 |
| `$(P1):YPos` | ai | mm | ±10 | Y位置 |
| `$(P1):SumValue` | ai | - | 0-200 | Sum值 |
| `$(P1):XTripWaveform` | waveform | mm | - | X位置历史(100000点) |
| `$(P1):YTripWaveform` | waveform | mm | - | Y位置历史(100000点) |
| `$(P1):ProtectStatus` | ai | - | 0/1 | 保护状态 |
| `$(P1):XProtectValue` | ai | mm | - | X保护阈值 |
| `$(P1):YProtectValue` | ai | mm | - | Y保护阈值 |

**数据流**:
```
FPGA BPM单元 → GetBPMX(0) → BPM_X[0] → ReadData(OFFSET_X_POS, 0) → $(P1):XPos
```

### BPM15 ($(P2), channel=1)

**说明**: BPM15的PV结构与BPM14完全相同

| 前缀 | channel | 示例PV |
|------|---------|--------|
| `$(P2)` | 1 | `$(P2):XPos`, `$(P2):YPos` |

**总计**: 2个BPM × 9个PV/BPM = **18个BPM位置PV**

## 系统控制PV

| PV名称 | 类型 | 描述 | 操作 |
|--------|------|------|------|
| `$(P):TripHistoryTrig` | ao | 触发历史数据采集 | 写入1触发 |
| `$(P):DataRatio` | ao | 数据比例设置 | 1-100 |
| `$(P):ParamRead` | ao | 触发参数读取 | 写入1触发 |
| `$(P):HistoryDataReady` | ai | 历史数据就绪标志 | 0=未就绪, 1=就绪 |

**使用示例**:
```bash
# 触发历史数据采集
caput iLinac_007:BPM14And15:TripHistoryTrig 1

# 等待数据就绪
camonitor iLinac_007:BPM14And15:HistoryDataReady

# 读取历史波形
caget -# 100000 iLinac_007:BPM14And15:RF3TripWaveform
```

**总计**: **4个系统控制PV**

## 状态监测PV

### LED状态

| PV名称 | 类型 | 描述 |
|--------|------|------|
| `$(P):LED1Status` | ai | LED1状态 (0=灭, 1=亮) |
| `$(P):LED2Status` | ai | LED2状态 |

### 数字输入

| PV名称 | 类型 | 描述 |
|--------|------|------|
| `$(P):DI0` | bi | 数字输入0 |
| `$(P):DI1` | bi | 数字输入1 |
| `$(P):DI2` | bi | 数字输入2 |
| `$(P):DI3` | bi | 数字输入3 |
| `$(P):DI4` | bi | 数字输入4 |
| `$(P):DI5` | bi | 数字输入5 |
| `$(P):DI6` | bi | 数字输入6 |
| `$(P):DI7` | bi | 数字输入7 |

**总计**: **10个状态监测PV**

## PV统计总结

| 类别 | 数量 | 说明 |
|------|------|------|
| RF监测 | 80 | 8通道 × 10 PV |
| 波形数据 | 16 | 8通道 × 2波形 |
| 波形分析 | 40 | 8通道 × 5 PV |
| BPM位置 | 18 | 2个BPM × 9 PV |
| 系统控制 | 4 | 触发、配置 |
| 状态监测 | 10 | LED、DI |
| **总计** | **168+** | 核心PV |

**注**: 实际PV数量超过200个，包含校准参数、配置参数等。

## PV命名规范

### 宏参数

| 宏 | 默认值 | 用途 | 示例 |
|----|--------|------|------|
| `$(P)` | `iLinac_007:BPM14And15` | 通用前缀 | `$(P):RF3Amp` |
| `$(P1)` | `iLinac_007:BPM14` | BPM1前缀 | `$(P1):XPos` |
| `$(P2)` | `iLinac_007:BPM15` | BPM2前缀 | `$(P2):XPos` |

### 命名模式

```
$(P):RF<N><Type>
  │   │  │  │
  │   │  │  └─ 类型: Amp, Phase, Power, etc.
  │   │  └──── 通道号: 3-10
  │   └─────── RF前缀
  └─────────── 系统前缀

$(P1):<Parameter>
  │     │
  │     └─────── BPM参数: XPos, YPos, etc.
  └───────────── BPM1前缀
```

## 使用示例

### 监控所有RF振幅

```bash
#!/bin/bash
for i in {3..10}; do
    echo "RF$i: $(caget -t iLinac_007:BPM14And15:RF${i}Amp)"
done
```

### Python批量读取

```python
import epics

prefix = "iLinac_007:BPM14And15"
channels = range(3, 11)  # RF3-RF10

# 读取所有振幅
amps = {}
for ch in channels:
    pv_name = f"{prefix}:RF{ch}Amp"
    amps[f"RF{ch}"] = epics.caget(pv_name)

print(amps)
```

### 波形数据分析

```python
import epics
import numpy as np
import matplotlib.pyplot as plt

# 读取波形
pv = "iLinac_007:BPM14And15:RF3TrigWaveform"
waveform = epics.caget(pv)

# 分析
print(f"Mean: {np.mean(waveform):.3f} V")
print(f"Std:  {np.std(waveform):.3f} V")
print(f"Max:  {np.max(waveform):.3f} V")
print(f"Min:  {np.min(waveform):.3f} V")

# 绘图
plt.plot(waveform)
plt.xlabel('Sample')
plt.ylabel('Voltage (V)')
plt.title('RF3 Trigger Waveform')
plt.show()
```

## 相关参考

- [Offset参考表](./offset-table.md) - Offset与PV的对应关系
- [数据类型参考](./data-type-reference.md) - 记录类型详解
- [数据库设计](../../part6-database-layer/) - 数据库详细文档

---

**提示**: 使用 `Ctrl+F` 快速搜索PV名称！
