# 05 - 扫描机制基础

> **目标**: 深入理解EPICS的扫描机制
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 40分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解什么是记录扫描（Record Scanning）
- ✅ 掌握5种主要扫描类型
- ✅ 知道何时使用哪种扫描方式
- ✅ 理解I/O中断扫描的工作原理
- ✅ 能够为不同场景选择合适的扫描策略

## 🎯 什么是扫描

### 核心概念

**扫描（Scanning）** = 处理（Process）记录

当一个记录被"扫描"时，EPICS会：
1. 读取输入（如果有INP字段）
2. 执行记录特定的处理逻辑
3. 检查报警条件
4. 更新时间戳
5. 广播变化给监听的客户端
6. 处理输出链接（如果有）

### 为什么需要扫描

```
没有扫描:
    PV值永远不会更新
    客户端看不到数据变化
    记录的值一直是初始值

有了扫描:
    PV定期或按事件更新
    客户端可以监控变化
    数据实时反映硬件状态
```

## 📊 扫描类型概览

EPICS提供了多种扫描方式，通过记录的 `SCAN` 字段设置：

| 扫描类型 | SCAN字段值 | 触发方式 | 适用场景 |
|---------|-----------|---------|---------|
| **定时扫描** | "1 second", ".5 second" 等 | 定时器 | 慢速变化的数据 |
| **I/O中断扫描** | "I/O Intr" | 硬件事件/软件信号 | 快速响应，事件驱动 |
| **被动扫描** | "Passive" | 其他记录触发 | 计算记录，中间值 |
| **事件扫描** | "Event" | 命名事件 | 同步多个IOC |
| **主动扫描** | - | 不通过SCAN字段 | 特殊用途 |

## 🔢 1. 定时扫描（Periodic Scanning）

### 工作原理

```
EPICS扫描线程（每个周期一个线程）
    ↓
定时器触发（如每1秒）
    ↓
扫描该周期的所有记录
    ↓
等待下一个周期
```

### 支持的周期

EPICS Base定义了以下标准扫描周期：

```
".1 second"   - 每0.1秒（10 Hz）
".2 second"   - 每0.2秒（5 Hz）
".5 second"   - 每0.5秒（2 Hz）
"1 second"    - 每1秒（1 Hz）
"2 second"    - 每2秒
"5 second"    - 每5秒
"10 second"   - 每10秒
```

**注意**: 也可以在 `menuScan.dbd` 中自定义周期。

### 示例

**慢速温度监控**:
```
record(ai, "MY:SlowTemp")
{
    field(SCAN, "10 second")  # 每10秒读取一次
    field(DTYP, "...")
    field(INP,  "...")
}
```

**快速电压监控**:
```
record(ai, "MY:FastVoltage")
{
    field(SCAN, ".1 second")  # 每0.1秒读取一次
    field(DTYP, "...")
    field(INP,  "...")
}
```

### 优缺点

**优点**:
- ✅ 简单易用，容易理解
- ✅ 可预测的更新频率
- ✅ 不需要设备支持特殊功能

**缺点**:
- ❌ 可能错过快速变化
- ❌ 固定周期可能不是最优
- ❌ 多个记录独立扫描，效率低

### BPMIOC中的使用

```bash
cd ~/BPMIOC
grep "field(SCAN" BPMmonitorApp/Db/BPMMonitor.db | grep "second" | head -5
```

输出示例:
```
field(SCAN, ".5 second")
field(SCAN, "1 second")
```

---

## ⚡ 2. I/O中断扫描（I/O Interrupt Scanning）

### 工作原理

这是BPMIOC中最重要的扫描方式！

```
硬件事件（或软件定时器）
    ↓
驱动层调用 scanIoRequest(ioScanPvt)
    ↓
EPICS I/O中断扫描线程被唤醒
    ↓
处理所有连接到该ioScanPvt的记录（SCAN="I/O Intr"）
    ↓
所有记录几乎同时更新
```

### 关键组件

#### ioScanPvt（I/O Scan Private）

```c
// driverWrapper.c中定义
static IOSCANPVT ioScanPvt;

// 初始化
scanIoInit(&ioScanPvt);

// 在需要扫描时调用
scanIoRequest(ioScanPvt);
```

### BPMIOC中的实现

**初始化**（`driverWrapper.c`）:
```c
int InitDevice()
{
    // ... 其他初始化 ...

    scanIoInit(&ioScanPvt);  // 初始化扫描结构

    epicsThreadCreate("BPMMonitor", 50, 20000,
                      (EPICSTHREADFUNC)my_thread, NULL);

    return 0;
}
```

**触发扫描**（`driverWrapper.c` 中的 `my_thread`）:
```c
static void my_thread(void *arg)
{
    while (1) {
        // 更新数据（从硬件或模拟）
        for (int i = 0; i < 8; i++) {
            Amp[i] = ...;
            Phase[i] = ...;
        }

        scanIoRequest(ioScanPvt);  // ← 触发扫描！

        epicsThreadSleep(0.1);  // 100ms
    }
}
```

**设备支持层注册**（`devBPMMonitor.c`）:
```c
static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    *ppvt = devGetInTrigScanPvt();  // 返回ioScanPvt
    return 0;
}
```

**数据库配置**:
```
record(ai, "$(P):RF3Amp")
{
    field(SCAN, "I/O Intr")  # ← 使用I/O中断扫描
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
}
```

### 工作流程详解

```
第1步: IOC启动时
    ↓
    设备支持层的 get_ioint_info() 被调用
    ↓
    EPICS获取 ioScanPvt 指针
    ↓
    记录注册到 ioScanPvt 的扫描列表

第2步: 运行时（每100ms）
    ↓
    my_thread() 更新 Amp[] 和 Phase[]
    ↓
    调用 scanIoRequest(ioScanPvt)
    ↓
    I/O中断扫描线程被唤醒
    ↓
    处理所有 SCAN="I/O Intr" 的记录
    ↓
    每个记录调用 read_ai()
    ↓
    read_ai() 调用 ReadData() 获取值
    ↓
    PV更新，客户端收到通知
```

### 多个ioScanPvt

BPMIOC定义了3个独立的ioScanPvt：

```c
static IOSCANPVT ioScanPvt;           // 常规数据（Amp, Phase）
static IOSCANPVT ioScanPvt_trip;      // Trip波形
static IOSCANPVT ioScanPvt_adcraw;    // ADC原始数据
```

**为什么需要多个？**
- 不同数据有不同的更新频率
- 大数据（波形）不应和小数据混在一起
- 分离关注点，便于调试

### 优缺点

**优点**:
- ✅ 事件驱动，响应快速
- ✅ 多个记录共享一次扫描，效率高
- ✅ 数据一致性好（同一时刻的数据）
- ✅ 不浪费CPU（没事件就不扫描）

**缺点**:
- ❌ 需要设备支持层实现
- ❌ 调试相对复杂
- ❌ 如果事件频率太高，可能导致CPU过载

### 何时使用

- ✅ 快速变化的数据（RF信号）
- ✅ 多个相关PV需要同步更新
- ✅ 硬件提供中断信号
- ✅ 需要最小延迟

---

## 🔗 3. 被动扫描（Passive Scanning）

### 工作原理

```
记录初始值设置后，永不自动扫描
    ↓
只有在以下情况下才处理：
  - 另一个记录通过链接触发（如 CP）
  - 客户端显式执行 caput
  - dbpf/dbpr命令
```

### 示例

**计算记录**（常用Passive + CP链接）:
```
record(calc, "$(P):RF3Power")
{
    field(SCAN, "Passive")        # 被动扫描
    field(CALC, "A*A*50")
    field(INPA, "$(P):RF3Amp CP") # CP = 当Amp变化时处理本记录
}
```

**工作流程**:
```
RF3Amp被I/O中断更新
    ↓
EPICS检查哪些记录链接到RF3Amp（带CP标志）
    ↓
处理RF3Power（计算功率）
    ↓
RF3Power更新
```

### 链接选项

| 选项 | 全称 | 行为 |
|------|------|------|
| (无) | - | 初始化时读取一次 |
| `NPP` | No Process Passive | 不处理目标记录 |
| `PP` | Process Passive | 每次读取时处理目标记录 |
| `CP` | Change Process | 源变化时处理本记录 |
| `CPP` | Change Process Passive | 组合CP和PP |
| `MS` | Maximize Severity | 传播报警严重性 |

**常用组合**:
```
"PV_NAME CP"      # 最常用：PV变化时处理
"PV_NAME CP MS"   # 还传播报警
"PV_NAME NPP"     # 只读取，不处理
```

### 优缺点

**优点**:
- ✅ 不占用扫描资源
- ✅ 灵活，由链接控制
- ✅ 适合中间计算

**缺点**:
- ❌ 如果没有链接，永不更新
- ❌ 链接复杂时难以理解

---

## 📅 4. 事件扫描（Event Scanning）

### 工作原理

```
post_event(event_number)
    ↓
所有 SCAN="Event" 且事件号匹配的记录被处理
```

### 示例

**数据库**:
```
record(ai, "MY:SyncData")
{
    field(SCAN, "Event")
    field(EVNT, "1")  # 事件号1
}
```

**驱动层**:
```c
#include <eventDef.h>

// 在需要时触发事件
post_event(1);
```

### 何时使用

- 跨IOC同步
- 复杂触发逻辑
- 不规则事件

---

## 🔄 扫描类型对比

### 性能对比

| 特性 | 定时扫描 | I/O中断 | 被动 | 事件 |
|------|---------|---------|------|------|
| CPU占用 | 中等 | 低 | 最低 | 低 |
| 响应延迟 | 最高周期/2 | 最低 | 取决于链接 | 低 |
| 实现复杂度 | 简单 | 中等 | 简单 | 复杂 |
| 数据一致性 | 差 | 优秀 | 好 | 优秀 |

### 使用场景对比

| 场景 | 推荐扫描方式 | 原因 |
|------|------------|------|
| 温度监控 | 定时（1-10秒） | 变化慢 |
| RF信号 | I/O中断 | 快速，需同步 |
| 功率计算 | 被动+CP | 派生值 |
| 束流触发 | I/O中断或事件 | 实时响应 |
| 用户设置 | 被动 | 只在caput时更新 |

---

## 🧪 实践示例

### 示例1: 混合扫描策略

```
# 硬件数据 - I/O中断
record(ai, "BPM:RF1:Amp")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
}

# 派生计算 - 被动+CP
record(calc, "BPM:RF1:Power")
{
    field(SCAN, "Passive")
    field(CALC, "A*A*50")
    field(INPA, "BPM:RF1:Amp CP")
}

# 慢速状态 - 定时
record(ai, "BPM:Temperature")
{
    field(SCAN, "10 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
}

# 用户设置 - 被动
record(ao, "BPM:SetThreshold")
{
    field(SCAN, "Passive")
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:0 ch=0")
}
```

### 示例2: 扫描链

```
AI记录（I/O Intr）
    ↓ [CP链接]
CALC记录1（Passive）
    ↓ [CP链接]
CALC记录2（Passive）
    ↓ [CP链接]
BO记录（Passive，执行动作）
```

一次I/O中断可以触发整条链的处理！

---

## 🔍 调试扫描问题

### 常见问题1: 记录不更新

**症状**: `caget` 总是返回相同的值

**检查**:
```bash
# 1. 检查SCAN字段
dbpr "MY:PV" 2

# 2. 如果是I/O Intr，检查get_ioint_info
# （在IOC启动日志中查看）

# 3. 如果是Passive，检查链接
dbpr "MY:PV" | grep INP
```

### 常见问题2: 更新太慢

**症状**: 数据有明显延迟

**检查**:
```
- 定时扫描周期是否太长？
- I/O中断是否真的被触发？（添加printf调试）
- 链接是否使用了NPP而不是CP？
```

### 常见问题3: CPU占用过高

**症状**: IOC进程CPU使用率很高

**检查**:
```
- 是否有太多记录使用快速扫描（.1 second）？
- I/O中断频率是否过高？
- 是否有扫描死循环？（A触发B，B触发A）
```

### 调试工具

```bash
# IOC Shell命令

# 显示扫描线程状态
scanppl

# 显示I/O中断扫描列表
scanpiol

# 显示记录详细信息
dbpr "PV_NAME" 4
```

---

## 📊 BPMIOC扫描配置总结

### RF数据（快速，I/O中断）

```
8个RF通道 × 10个参数 = 80个ai记录
全部使用 SCAN="I/O Intr"
共享一个 ioScanPvt
每100ms触发一次
```

### 功率计算（派生，被动）

```
8个calc记录
SCAN="Passive"
INPA="...Amp CP"
当幅度变化时自动计算
```

### 配置寄存器（慢速，定时）

```
版本、温度等
SCAN="1 second" 或更慢
不需要高频更新
```

### 用户设置（被动）

```
所有ao记录（阈值、配置等）
SCAN="Passive"
只在用户caput时处理
```

---

## 🔗 相关文档

- [Part 4: 03-thread-and-scanning.md](../../part4-driver-layer/03-thread-and-scanning.md) - 驱动层线程实现
- [Part 5: 06-io-interrupt.md](../../part5-device-support-layer/06-io-interrupt.md) - I/O中断详细实现
- [Part 8: lab02](../part8-hands-on-labs/labs-basic/lab02-modify-scan.md) - 修改扫描周期实验
- [EPICS Application Developer's Guide - Chapter 5: Database Scanning](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/DatabaseScanning.html)

---

## 📝 总结

### 关键要点

1. **扫描 = 处理记录**: 更新值、检查报警、广播变化
2. **I/O中断最高效**: 多记录共享一次扫描，BPMIOC核心机制
3. **被动+CP常用于计算**: 避免重复计算
4. **选择合适的扫描方式**: 根据数据特性（快/慢，主/从）

### 选择指南

```
数据快速变化 + 多个相关PV → I/O中断
数据慢速变化 → 定时扫描
派生计算 → 被动 + CP链接
用户输入 → 被动
跨IOC同步 → 事件扫描
```

### 下一步

- [06-links-and-forwarding.md](./06-links-and-forwarding.md) - 记录链接详解
- [07-alarms-and-archive.md](./07-alarms-and-archive.md) - 报警和归档
- [Part 8: lab02](../part8-hands-on-labs/labs-basic/lab02-modify-scan.md) - 动手实验

---

**🎉 恭喜！** 你已经掌握了EPICS扫描机制的核心知识，这是理解BPMIOC运行的关键！
