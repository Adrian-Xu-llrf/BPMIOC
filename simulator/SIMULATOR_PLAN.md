# BPMIOC Simulator 开发计划

> **目标**: 在PC上模拟FPGA硬件，实现快速开发和验证
> **优先级**: 高
> **预计工时**: 2-3天

## 📋 1. 总体规划

### 1.1 核心目标

```
PC开发环境 (simulator):
  ✅ 无需真实硬件即可运行IOC
  ✅ 提供可配置的模拟数据源
  ✅ 支持从文件回放真实数据
  ✅ 与ZYNQ生产环境代码100%兼容
  ✅ 快速迭代开发周期（秒级）
```

### 1.2 架构设计

```
BPMmonitorApp/
  ├─ src/
  │   ├─ devBPMMonitor.c      ← PC和ZYNQ共用
  │   ├─ driverWrapper.c      ← PC和ZYNQ共用（增加库选择逻辑）
  │   └─ ...
  │
  ├─ Db/
  │   └─ BPMMonitor.db        ← PC和ZYNQ共用
  │
simulator/                     ← 新建目录
  ├─ SIMULATOR_PLAN.md         ← 本文件
  ├─ README.md                 ← 使用说明
  │
  ├─ src/                      ← Mock硬件库源码
  │   ├─ mockHardware.c        ← 主Mock实现
  │   ├─ mockHardware.h        ← 头文件
  │   ├─ dataGenerator.c       ← 数据生成器
  │   ├─ fileReplay.c          ← 文件回放
  │   └─ Makefile              ← 独立编译
  │
  ├─ config/                   ← 配置文件
  │   ├─ sim_sine.conf         ← 正弦波模式配置
  │   ├─ sim_random.conf       ← 随机噪声配置
  │   ├─ sim_replay.conf       ← 文件回放配置
  │   └─ fault_injection.conf  ← 故障注入配置
  │
  ├─ data/                     ← 测试数据
  │   ├─ sample_rf_data.csv    ← 示例真实数据
  │   ├─ waveform_samples/     ← 波形样本
  │   └─ README.md             ← 数据格式说明
  │
  ├─ scripts/                  ← 测试脚本
  │   ├─ build.sh              ← 编译脚本
  │   ├─ run_pc.sh             ← PC运行脚本
  │   ├─ test_basic.sh         ← 基础测试
  │   ├─ test_waveform.sh      ← 波形测试
  │   ├─ generate_sample_data.py ← 生成测试数据
  │   └─ compare_outputs.py    ← 对比PC/ZYNQ输出
  │
  ├─ iocBoot/                  ← IOC启动配置
  │   └─ iocSimulator/
  │       ├─ st.cmd            ← PC模拟启动脚本
  │       ├─ st.cmd.sine       ← 正弦波模式
  │       ├─ st.cmd.random     ← 随机模式
  │       └─ st.cmd.replay     ← 回放模式
  │
  ├─ tests/                    ← 自动化测试
  │   ├─ test_all_pvs.py       ← 测试所有PV
  │   ├─ test_data_flow.py     ← 数据流测试
  │   ├─ test_alarms.py        ← 报警测试
  │   └─ benchmark.py          ← 性能基准测试
  │
  └─ docs/                     ← 文档
      ├─ USER_GUIDE.md         ← 用户指南
      ├─ API_REFERENCE.md      ← Mock API参考
      └─ DEVELOPMENT.md        ← 开发者指南
```

## 📝 2. 详细实现计划

### 2.1 阶段1: 基础Mock库（优先级：最高）

**文件**: `simulator/src/mockHardware.c`

**功能**:
- ✅ 实现SystemInit() - 初始化模拟硬件
- ✅ 实现GetRfInfo() - 提供8通道RF数据
- ✅ 实现GetTrigWaveform() - 提供10000点波形
- ✅ 实现Set系列函数 - 写操作Mock（30+函数）
- ✅ 支持3种模式：正弦波/随机/文件回放

**数据生成策略**:
```c
模式0 - 正弦波（默认）:
  Amp[ch]   = 4.0 + 1.0 * sin(2πft + ch*π/4)
  Phase[ch] = 90.0 * sin(2π*0.1*t + ch*π/8)
  Power[ch] = Amp[ch]^2 * 50Ω

  特点: 平滑、可预测、适合基础测试

模式1 - 随机噪声:
  Amp[ch]   = 4.0 ± random(0, 2.0)
  Phase[ch] = random(-180, +180)

  特点: 模拟真实噪声、适合稳定性测试

模式2 - 文件回放:
  从CSV读取真实FPGA数据
  循环播放

  特点: 最接近真实、适合算法验证
```

**API兼容性**:
| 真实库函数 | Mock实现 | 状态 |
|-----------|---------|------|
| SystemInit() | 完整实现 | ✅ |
| GetRfInfo() | 完整实现 | ✅ |
| GetTrigWaveform() | 完整实现 | ✅ |
| SetPhaseOffset() | 打印log | ✅ |
| SetAmpOffset() | 打印log | ✅ |
| GetBoardStatus() | 返回模拟状态 | ✅ |
| GetFirmwareVersion() | 返回"MOCK-v1.0" | ✅ |
| ... (其他25+函数) | 逐步实现 | 🔄 |

**预计工作量**: 4-6小时

---

### 2.2 阶段2: 数据生成器（优先级：高）

**文件**: `simulator/src/dataGenerator.c`

**功能**:
```c
// 可配置的波形生成
typedef struct {
    double frequency;      // 信号频率
    double amplitude;      // 幅度
    double phase_offset;   // 相位偏移
    double noise_level;    // 噪声水平
    int harmonics;         // 是否包含谐波
} WaveConfig;

// 生成单个采样点
float generateSample(WaveConfig *cfg, double time);

// 生成完整波形
int generateWaveform(WaveConfig *cfg, float *buffer, int npts);

// 从配置文件加载参数
int loadWaveConfig(const char *filename, WaveConfig *cfg);
```

**配置文件示例**: `simulator/config/sim_sine.conf`
```ini
[Channel_0]
frequency = 0.5        # Hz（外包络频率）
amplitude = 4.0        # V
phase_offset = 0.0     # degrees
noise_level = 0.05     # 5%噪声
harmonics = 1          # 包含2次谐波

[Channel_1]
frequency = 0.6
amplitude = 3.8
phase_offset = 45.0
noise_level = 0.03
harmonics = 0

# ... Channel_2 到 Channel_7
```

**预计工作量**: 3-4小时

---

### 2.3 阶段3: 文件回放功能（优先级：中）

**文件**: `simulator/src/fileReplay.c`

**支持格式**:
```csv
# sample_rf_data.csv
# Format: timestamp, ch0_amp, ch0_phase, ch1_amp, ch1_phase, ...
0.000, 3.14, 45.2, 3.20, 43.8, 3.18, 46.1, ...
0.100, 3.15, 45.3, 3.21, 43.9, 3.19, 46.2, ...
0.200, 3.16, 45.4, 3.22, 44.0, 3.20, 46.3, ...
...
```

**功能**:
```c
// 加载CSV数据文件
int loadDataFile(const char *filename);

// 获取下一帧数据（循环播放）
int getNextFrame(float *Amp, float *Phase, ...);

// 重置到文件开头
int rewindData(void);

// 设置播放速度（1.0=正常，2.0=2倍速）
int setPlaybackSpeed(double speed);
```

**预计工作量**: 2-3小时

---

### 2.4 阶段4: 故障注入系统（优先级：低）

**文件**: `simulator/config/fault_injection.conf`

**支持的故障类型**:
```ini
[Faults]
enabled = 1

# 1. 通道故障
channel_0_dead = 0           # 通道0断线
channel_1_noisy = 0          # 通道1噪声过大
channel_2_saturated = 0      # 通道2饱和

# 2. 间歇性故障
intermittent_dropout = 0     # 间歇性数据丢失
intermittent_spike = 0       # 间歇性尖峰

# 3. 系统故障
slow_response = 0            # 模拟慢响应（延迟）
initialization_fail = 0      # 初始化失败
periodic_timeout = 0         # 周期性超时

# 4. 数据异常
phase_jump = 0               # 相位突变
amplitude_drift = 0          # 幅度漂移
```

**实现**:
```c
typedef struct {
    int channel_dead[8];      // 通道死区标志
    int noisy_channel[8];     // 噪声通道
    double noise_factor;      // 噪声因子
    int enable_dropout;       // 启用丢包
    double dropout_rate;      // 丢包率
} FaultConfig;

// 应用故障到数据
int applyFaults(FaultConfig *fault, float *Amp, float *Phase);
```

**预计工作量**: 3-4小时

---

### 2.5 阶段5: 测试脚本（优先级：高）

**文件**: `simulator/scripts/test_basic.sh`

```bash
#!/bin/bash
# 基础功能测试

echo "=== BPMIOC Simulator Basic Test ==="

# 1. 编译Mock库
cd ../src
make clean
make
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build success"

# 2. 启动IOC
cd ../iocBoot/iocSimulator
./st.cmd &
IOC_PID=$!
sleep 5

# 3. 测试所有PV可访问
echo "Testing PV accessibility..."
caget iLinac_007:BPM14And15:RFIn_01_Amp
caget iLinac_007:BPM14And15:RFIn_01_Phase
caget iLinac_007:BPM14And15:RFIn_01_TrigWaveform

# 4. 测试写操作
echo "Testing write operations..."
caput iLinac_007:BPM14And15:SetPhaseOffset 10.5

# 5. 测试I/O中断更新
echo "Monitoring updates..."
timeout 5 camonitor iLinac_007:BPM14And15:RFIn_01_Amp

# 6. 清理
kill $IOC_PID
echo "✅ All tests passed"
```

**预计工作量**: 4-5小时（包括所有测试脚本）

---

### 2.6 阶段6: Python测试工具（优先级：中）

**文件**: `simulator/tests/test_all_pvs.py`

```python
#!/usr/bin/env python3
"""
自动化测试所有PV
"""
import epics
import time
import sys

# 定义所有需要测试的PV
PV_LIST = [
    'iLinac_007:BPM14And15:RFIn_01_Amp',
    'iLinac_007:BPM14And15:RFIn_01_Phase',
    'iLinac_007:BPM14And15:RFIn_01_Power',
    # ... 168个PV
]

def test_pv_readable(pvname):
    """测试PV可读"""
    try:
        pv = epics.PV(pvname)
        value = pv.get(timeout=5.0)
        if value is None:
            return False, "Timeout"
        return True, value
    except Exception as e:
        return False, str(e)

def test_all_pvs():
    """测试所有PV"""
    passed = 0
    failed = 0

    for pvname in PV_LIST:
        success, result = test_pv_readable(pvname)
        if success:
            print(f"✅ {pvname}: {result}")
            passed += 1
        else:
            print(f"❌ {pvname}: {result}")
            failed += 1

    print(f"\nResults: {passed} passed, {failed} failed")
    return failed == 0

if __name__ == '__main__':
    sys.exit(0 if test_all_pvs() else 1)
```

**预计工作量**: 3-4小时

---

### 2.7 阶段7: 文档（优先级：中）

**文件清单**:
- `simulator/README.md` - 快速入门
- `simulator/docs/USER_GUIDE.md` - 详细用户指南
- `simulator/docs/API_REFERENCE.md` - Mock API文档
- `simulator/docs/DEVELOPMENT.md` - 开发者文档

**预计工作量**: 4-5小时

---

## 🔧 3. 技术细节

### 3.1 编译系统

**simulator/src/Makefile**:
```makefile
# Mock硬件库独立编译

CC = gcc
CFLAGS = -Wall -fPIC -O2 -g
LDFLAGS = -shared -lm -lpthread

TARGET = libBPMboardMock.so
SRCS = mockHardware.c dataGenerator.c fileReplay.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

install:
	cp $(TARGET) ../../lib/linux-x86_64/

.PHONY: all clean install
```

### 3.2 集成到BPMIOC构建系统

**修改 BPMmonitorApp/src/Makefile**:
```makefile
# 添加条件编译支持

# 检测目标架构
ifeq ($(T_A), linux-x86_64)
    # PC平台：可选使用Mock库
    USE_MOCK ?= YES
endif

ifeq ($(USE_MOCK), YES)
    # 编译Mock库
    LIBRARY_HOST = BPMboardMock
    BPMboardMock_SRCS += ../../../simulator/src/mockHardware.c
    BPMboardMock_SRCS += ../../../simulator/src/dataGenerator.c
    BPMboardMock_SRCS += ../../../simulator/src/fileReplay.c
    BPMboardMock_LIBS += m
endif
```

### 3.3 环境变量配置

**支持的环境变量**:
```bash
# 硬件库选择
export BPMIOC_HW_LIB=libBPMboardMock.so    # 或 libBPMboard14And15.so

# 模拟模式
export BPMIOC_SIM_MODE=0                   # 0=正弦, 1=随机, 2=回放

# 配置文件路径
export BPMIOC_SIM_CONFIG=./config/sim_sine.conf

# 数据文件路径（回放模式）
export BPMIOC_DATA_FILE=./data/sample_rf_data.csv

# 故障注入
export BPMIOC_FAULT_CONFIG=./config/fault_injection.conf

# 调试输出
export BPMIOC_DEBUG_LEVEL=2                # 0=OFF, 1=ERROR, 2=INFO, 3=DEBUG
```

---

## 📊 4. 测试验证计划

### 4.1 单元测试（Makefile集成）

```makefile
# simulator/src/Makefile
test: $(TARGET)
	./test_sine_wave
	./test_random
	./test_file_replay
	./test_fault_injection
```

### 4.2 集成测试

```bash
# simulator/scripts/test_integration.sh

# 1. 测试正弦波模式
BPMIOC_SIM_MODE=0 ./iocBoot/iocSimulator/st.cmd &
sleep 5
python3 tests/test_all_pvs.py
killall BPMmonitor

# 2. 测试随机模式
BPMIOC_SIM_MODE=1 ./iocBoot/iocSimulator/st.cmd &
sleep 5
python3 tests/test_all_pvs.py
killall BPMmonitor

# 3. 测试文件回放
BPMIOC_SIM_MODE=2 ./iocBoot/iocSimulator/st.cmd &
sleep 5
python3 tests/test_waveform.py
killall BPMmonitor
```

### 4.3 性能基准测试

```python
# simulator/tests/benchmark.py

import epics
import time

def benchmark_throughput():
    """测试数据吞吐量"""
    pv = epics.PV('iLinac_007:BPM14And15:RFIn_01_Amp')

    count = 0
    start = time.time()

    while time.time() - start < 10.0:  # 10秒测试
        value = pv.get()
        count += 1

    rate = count / 10.0
    print(f"Throughput: {rate:.1f} reads/sec")

def benchmark_latency():
    """测试延迟"""
    pv = epics.PV('iLinac_007:BPM14And15:SetPhaseOffset')

    latencies = []
    for i in range(100):
        start = time.time()
        pv.put(i * 0.1, wait=True)
        latency = time.time() - start
        latencies.append(latency)

    avg = sum(latencies) / len(latencies)
    print(f"Average latency: {avg*1000:.2f} ms")
```

---

## 🎯 5. 里程碑和时间表

### 第1天（8小时）
- ✅ 创建目录结构（0.5h）
- ✅ 实现基础mockHardware.c（4h）
- ✅ 实现dataGenerator.c（2h）
- ✅ 编写Makefile（0.5h）
- ✅ 基础编译和测试（1h）

### 第2天（8小时）
- ✅ 实现fileReplay.c（3h）
- ✅ 创建配置文件（1h）
- ✅ 编写测试脚本（3h）
- ✅ 集成测试（1h）

### 第3天（6小时）
- ✅ 故障注入系统（3h）
- ✅ Python测试工具（2h）
- ✅ 文档编写（1h）

### 总计：22小时（约3个工作日）

---

## ✅ 6. 验收标准

### 6.1 功能验收

- [ ] Mock库成功编译（无警告）
- [ ] IOC在PC上成功启动
- [ ] 所有168个PV可访问
- [ ] I/O中断扫描正常工作（10Hz更新）
- [ ] 波形数据正确（10000点）
- [ ] 写操作正常响应
- [ ] 三种模式（正弦/随机/回放）都能运行
- [ ] 配置文件正确加载

### 6.2 性能验收

- [ ] 数据更新率：≥10 Hz
- [ ] 读取延迟：<10 ms
- [ ] 写入延迟：<5 ms
- [ ] 内存占用：<100 MB
- [ ] CPU占用：<10%（空闲时）

### 6.3 兼容性验收

- [ ] PC和ZYNQ使用相同的上层代码
- [ ] 切换库文件无需重新编译上层
- [ ] 环境变量配置生效
- [ ] 支持Ubuntu 20.04/22.04
- [ ] 支持EPICS Base 3.15.6+

---

## 🚀 7. 快速开始（计划实施后）

```bash
# 1. 编译Mock库
cd simulator/src
make

# 2. 安装到lib目录
make install

# 3. 运行模拟器（正弦波模式）
cd ../iocBoot/iocSimulator
./st.cmd.sine

# 4. 另一个终端测试
caget iLinac_007:BPM14And15:RFIn_01_Amp
camonitor iLinac_007:BPM14And15:RFIn_01_Amp

# 5. 运行自动化测试
cd ../../tests
python3 test_all_pvs.py
```

---

## 📌 8. 待决问题

### Q1: 是否需要模拟VME总线？
**答**: 暂时不需要。Mock库直接生成数据，不模拟底层总线。

### Q2: 如何保证Mock数据的真实性？
**答**: 提供文件回放功能，可从真实FPGA录制数据后在PC回放。

### Q3: 是否需要GUI工具？
**答**: Phase 1不需要。可使用CSS/BOY等EPICS标准GUI。

### Q4: 如何处理时序敏感的测试？
**答**: 提供可配置的延迟注入和时间戳模拟。

---

## 📋 9. 下一步行动

请确认此计划，我将开始实施：

### 选项A: 完整实施（推荐）
- 按照上述7个阶段完整实现
- 预计3个工作日

### 选项B: 最小可行版本（快速）
- 只实现阶段1+2+5（基础Mock+测试）
- 预计1个工作日
- 后续可扩展

### 选项C: 自定义
- 您指定优先实现哪些部分

---

**请确认**:
1. 目录结构是否合理？
2. 需要调整哪些功能？
3. 选择哪个实施选项？

确认后我将立即开始创建文件！
