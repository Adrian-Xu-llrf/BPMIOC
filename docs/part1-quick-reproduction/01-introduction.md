# BPMIOC 项目简介

## 什么是 BPMIOC?

**BPMIOC** (Beam Position Monitor Input/Output Controller) 是一个基于 EPICS 的 IOC 应用程序,用于**加速器光束位置监测系统**。

### 核心功能

BPMIOC 监测和控制以下参数：

| 功能模块 | 描述 | 通道数 | 更新频率 |
|----------|------|--------|----------|
| **RF 监测** | 射频信号振幅和相位 | 8个通道 (RF3-RF10) | 100ms |
| **BPM 位置** | 光束X-Y位置计算 | 2个BPM单元 | 100ms |
| **功率转换** | RF振幅→功率(KW) | 8个通道 | 实时计算 |
| **波形采集** | 触发和历史波形 | 10000点/通道 | 触发时 |
| **波形分析** | 平均电压(含背景扣除) | 8个通道 | 按需计算 |
| **状态监控** | 系统状态和告警 | 多个状态位 | 实时 |

### 应用场景

- **加速器**: iLinac (直线加速器) 实验系统
- **设备**: BPM14 和 BPM15 两个光束位置监测器
- **平台**: ARM 架构 Linux 控制器
- **接口**: FPGA 硬件 + liblowlevel.so 抽象层

## 为什么要学习 BPMIOC?

### 1. 完整的工程实例
BPMIOC 是一个**真实的、正在运行的**加速器控制系统,不是玩具项目。它包含：
- ✅ 完整的三层架构
- ✅ 硬件抽象设计
- ✅ 实时数据采集
- ✅ 复杂的信号处理
- ✅ 工程化的代码组织

### 2. 典型的 EPICS IOC 设计
学习 BPMIOC 可以掌握：
- EPICS 三层架构（驱动→设备支持→数据库）
- I/O 中断扫描机制
- 多线程设计
- Channel Access 网络通信
- 数据库设计和 PV 管理

### 3. 可复现
通过模拟模式,你可以：
- 在没有硬件的情况下运行
- 修改代码立即看到效果
- 学习架构和数据流
- 作为模板开发类似系统

## 技术架构

### 三层设计

```
┌─────────────────────────────────────────────────────────┐
│                    数据库层                              │
│  BPMMonitor.db (1891行PV定义)                           │
│  - 200+ PV记录                                          │
│  - ai, ao, bi, bo, waveform类型                         │
│  - 宏参数化配置                                         │
└────────────────┬────────────────────────────────────────┘
                 │ INP/OUT字段 "@TYPE:offset ch=N"
                 ▼
┌─────────────────────────────────────────────────────────┐
│                 设备支持层                               │
│  devBPMMonitor.c (423行)                                │
│  - 7个DSET (设备支持表)                                 │
│  - 参数解析: 解析 "@AMP:0 ch=3"                         │
│  - 记录适配: ai, ao, waveform等                         │
│  - I/O中断: scanIoRequest触发                           │
└────────────────┬────────────────────────────────────────┘
                 │ 调用 ReadData(), SetReg()
                 ▼
┌─────────────────────────────────────────────────────────┐
│                    驱动层                                │
│  driverWrapper.c (1540行)                               │
│  - 硬件抽象: 50+个函数指针                              │
│  - 数据缓冲: Amp[], Phase[], TrigWaveform[]             │
│  - 后台线程: 100ms周期扫描                              │
│  - Offset映射: 34+种数据类型                            │
│  - 动态加载: dlopen("liblowlevel.so")                  │
└────────────────┬────────────────────────────────────────┘
                 │ dlsym获取函数地址
                 ▼
┌─────────────────────────────────────────────────────────┐
│                 硬件抽象层                               │
│  liblowlevel.so (外部库)                                │
│  - SystemInit(), GetRfInfo(), SetDO()                  │
│  - FPGA 寄存器访问                                      │
│  - 硬件厂商提供                                         │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
            [ FPGA 硬件 ]
```

### 关键设计模式

#### 1. 函数指针表（硬件抽象）
```c
typedef int (*GetRfInfoFunc)(int channel, double* amp, double* phase);
GetRfInfoFunc getRfInfoFunc = NULL;

// 运行时加载
*(void **)(&getRfInfoFunc) = dlsym(handle, "GetRfInfo");

// 调用
(*getRfInfoFunc)(0, &Amp[0], &Phase[0]);
```

#### 2. Offset 统一接口
```c
#define OFFSET_AMP      0
#define OFFSET_PHASE    1
#define OFFSET_POWER    17
// ... 34+ offsets

double ReadData(int offset, int channel, int type) {
    switch(offset) {
        case OFFSET_AMP: return Amp[channel];
        case OFFSET_PHASE: return Phase[channel];
        // ...
    }
}
```

#### 3. I/O 中断扫描
```c
// 后台线程更新数据
static void my_thread(void *arg) {
    while(1) {
        // 读取硬件数据到缓冲区
        updateHardwareData();

        // 触发EPICS记录扫描
        scanIoRequest(ioScanPvt);

        epicsThreadSleep(0.1);  // 100ms
    }
}
```

## 主要 PV 示例

### RF 监测 PV
```
iLinac_007:BPM14And15:RF3Amp           # RF3 振幅 (V)
iLinac_007:BPM14And15:RF3Phase         # RF3 相位 (度)
iLinac_007:BPM14And15:RF3Power         # RF3 功率 (KW)
iLinac_007:BPM14And15:RF3TrigWaveform  # RF3 触发波形 (10000点)
iLinac_007:BPM14And15:RF3AVGVoltage    # RF3 平均电压 (V)
```

### BPM 位置 PV
```
iLinac_007:BPM14:XPos                  # BPM14 X位置
iLinac_007:BPM14:YPos                  # BPM14 Y位置
iLinac_007:BPM14:SumValue              # BPM14 Sum值
```

### 控制 PV
```
iLinac_007:BPM14And15:RF3AVGStart      # 平均电压起始点
iLinac_007:BPM14And15:RF3AVGStop       # 平均电压结束点
iLinac_007:BPM14And15:TripHistoryTrig  # 触发历史数据采集
```

## 项目统计

| 指标 | 数值 |
|------|------|
| 总代码行数 | ~4000行 |
| 驱动层代码 | 1540行 (driverWrapper.c) |
| 设备支持层 | 423行 (devBPMMonitor.c) |
| 数据库记录 | 1891行 (BPMMonitor.db) |
| PV 总数 | 200+ |
| 支持的记录类型 | 7种 (ai, ao, bi, bo, waveform, calc, etc.) |
| 后台线程数 | 1个主扫描线程 |
| 扫描周期 | 100ms |
| 波形缓冲 | 10000点/通道 |

## 最新功能

基于 Git 历史,最近添加的功能：

### 波形平均电压计算 (最新)
```c
// 计算波形平均电压,扣除背景
void calculateAvgVoltage(int channel) {
    // 1. 计算信号区域平均
    double signal_avg = average(waveform, AVGStart, AVGStop);

    // 2. 计算背景区域平均
    double bg_avg = average(waveform, BackGroundStart, BackGroundStop);

    // 3. 扣除背景
    AVG_Voltage[channel] = signal_avg - bg_avg;
}
```

**相关 Commits**:
- `cc1ea99`: 新增波形平均电压计算功能
- `23e45d0`: 修正函数名拼写错误
- `2eb03c1`: 修复 Makefile 配置

## 依赖关系

### 软件依赖
- **EPICS Base** 3.15.6
- **gcc/g++** 4.8+
- **GNU Make**
- **动态库支持** (libdl, libpthread)

### 硬件依赖 (真实部署时)
- **liblowlevel.so**: 底层硬件库
- **FPGA 控制器**: BPM 硬件接口
- **ARM Linux**: 目标运行平台

### 模拟模式 (学习时)
- ❌ 不需要 liblowlevel.so
- ❌ 不需要 FPGA 硬件
- ✅ 仅需 EPICS Base
- ✅ 可在任何 Linux 系统运行

## 学习路径

学习 BPMIOC 的三个层次：

### 层次1: 能运行 (1-2周)
- 编译并运行 IOC
- 访问 PV
- 理解基本概念

### 层次2: 能修改 (3-6周)
- 理解三层架构
- 追踪数据流
- 添加新功能

### 层次3: 能创建 (7-8周)
- 从零设计类似系统
- 独立开发 IOC
- 部署到硬件

## 为什么选择 BPMIOC 作为学习项目?

### ✅ 优势
1. **真实项目**: 不是教学demo,是实际运行的系统
2. **完整架构**: 三层设计清晰,职责分明
3. **适度复杂**: 不太简单也不太复杂,适合学习
4. **可模拟**: 无硬件也能运行和测试
5. **有文档**: 你现在正在看的这套文档
6. **有代码**: 完整的开源代码
7. **有注释**: 关键部分有详细注释
8. **有历史**: Git历史记录了演进过程

### 🎯 适合人群
- 博士生: 需要修改或扩展 BPM 系统
- 工程师: 需要开发类似的设备 IOC
- 学生: 学习 EPICS IOC 开发
- 研究者: 了解加速器控制系统

## 对比其他学习资源

| 资源 | 优势 | 劣势 |
|------|------|------|
| **EPICS 官方教程** | 全面,权威 | 偏理论,缺少完整项目 |
| **简单示例** | 容易上手 | 太简单,不够实用 |
| **BPMIOC** ⭐ | 真实,完整,可模拟 | 需要一定学习曲线 |

## 下一步

现在你已经了解了 BPMIOC 是什么,接下来：

1. **检查前置条件** → [02-prerequisites.md](./02-prerequisites.md)
2. **安装 EPICS Base** → [03-install-epics.md](./03-install-epics.md)
3. **编译 BPMIOC** → [04-clone-and-compile.md](./04-clone-and-compile.md)
4. **运行 IOC** → [06-first-run.md](./06-first-run.md)

或者直接跳到 [QUICK_START.md](../QUICK_START.md) 快速开始！

---

**准备好深入学习了吗？** 让我们开始吧！🚀
