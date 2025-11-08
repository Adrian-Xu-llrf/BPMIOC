# Simulator Test Data

本目录包含用于模拟器回放功能的测试数据文件。

## 文件格式

### RF数据文件 (.csv)

```csv
# timestamp(s), ch0_amp, ch0_phase, ch1_amp, ch1_phase, ..., ch7_amp, ch7_phase
0.000, 3.14, 45.2, 3.20, 43.8, 3.18, 46.1, 3.22, 44.5, 3.16, 45.8, 3.19, 44.2, 3.21, 45.5, 3.17, 44.9
0.100, 3.15, 45.3, 3.21, 43.9, 3.19, 46.2, 3.23, 44.6, 3.17, 45.9, 3.20, 44.3, 3.22, 45.6, 3.18, 45.0
0.200, 3.16, 45.4, 3.22, 44.0, 3.20, 46.3, 3.24, 44.7, 3.18, 46.0, 3.21, 44.4, 3.23, 45.7, 3.19, 45.1
...
```

**列定义**:
- 第1列: 时间戳 (秒)
- 第2-3列: 通道0幅度(V)、相位(度)
- 第4-5列: 通道1幅度、相位
- ...以此类推

### 波形数据文件 (.wfm)

```csv
# channel, timestamp, npts, [data points...]
0, 0.000, 10000, 0.12, -0.34, 0.56, -0.78, ...
1, 0.000, 10000, 0.23, -0.45, 0.67, -0.89, ...
...
```

## 示例数据

### sample_rf_data.csv

示例RF数据文件（待创建）。

包含:
- 8个通道
- 100秒数据
- 100ms采样间隔
- 共1000个数据点

### waveform_samples/

波形数据目录（待创建）。

## 从真实硬件录制数据

### 方法1：使用EPICS Archive

```bash
# 使用Channel Access Archiver记录数据
# （需要配置Archive Appliance）
```

### 方法2：使用Python脚本

```python
#!/usr/bin/env python3
import epics
import time
import csv

# PV列表
pvs = [f'iLinac_007:BPM14And15:RFIn_0{i+1}_Amp' for i in range(8)]
pvs += [f'iLinac_007:BPM14And15:RFIn_0{i+1}_Phase' for i in range(8)]

# 打开CSV文件
with open('recorded_data.csv', 'w', newline='') as f:
    writer = csv.writer(f)

    # 写入标题
    header = ['timestamp']
    for i in range(8):
        header += [f'ch{i}_amp', f'ch{i}_phase']
    writer.writerow(header)

    # 记录100秒，每100ms一个点
    start_time = time.time()
    for _ in range(1000):
        row = [time.time() - start_time]

        # 读取所有PV
        for pv_name in pvs:
            value = epics.caget(pv_name)
            row.append(value)

        writer.writerow(row)
        time.sleep(0.1)

print("Recording complete!")
```

### 方法3：使用caget循环

```bash
#!/bin/bash
# record_data.sh

OUTPUT=recorded_data.csv

# 写入标题
echo "timestamp,ch0_amp,ch0_phase,ch1_amp,ch1_phase,..." > $OUTPUT

# 记录100秒
for i in {1..1000}; do
    TIMESTAMP=$(date +%s.%N)

    # 读取所有通道
    AMP0=$(caget -t iLinac_007:BPM14And15:RFIn_01_Amp)
    PHASE0=$(caget -t iLinac_007:BPM14And15:RFIn_01_Phase)
    # ... 读取其他通道

    echo "$TIMESTAMP,$AMP0,$PHASE0,..." >> $OUTPUT

    sleep 0.1
done
```

## 使用录制的数据

1. 将录制的数据文件放入此目录

2. 设置环境变量：
```bash
export BPMIOC_SIM_MODE=2
export BPMIOC_DATA_FILE=./simulator/data/recorded_data.csv
```

3. 启动IOC：
```bash
./st.cmd
```

## 数据质量要求

✅ **良好的数据**:
- 连续时间戳（无大跳变）
- 所有通道数据完整
- 值在合理范围内（Amp: 0-10V, Phase: -180~+180度）

❌ **问题数据**:
- 时间戳乱序
- 缺失通道数据
- 包含NaN或Inf值

## 数据预处理工具

### validate_data.py

验证数据文件格式和质量（待实现）。

### convert_data.py

转换其他格式到标准CSV（待实现）。

## 注意事项

⚠️ **文件大小** - 长时间录制会产生大文件，注意磁盘空间

⚠️ **采样率** - 确保采样间隔与真实硬件匹配（通常100ms）

⚠️ **同步** - 多通道数据应同时采集，避免时间偏移

⚠️ **回放速度** - 可通过配置调整回放速度（1.0=正常，2.0=2倍速）
