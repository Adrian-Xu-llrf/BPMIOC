# Lab 02: 修改扫描周期

> **目标**: 理解并修改BPMIOC的扫描机制
> **难度**: ⭐⭐☆☆☆
> **预计时间**: 45分钟
> **前置知识**: Lab 01, Part 2: 02-key-concepts.md

## 📋 实验目标

完成本实验后，你将能够：
- ✅ 理解I/O中断扫描与定时扫描的区别
- ✅ 修改驱动层的扫描频率
- ✅ 修改数据库层的扫描方式
- ✅ 观察扫描频率对性能的影响
- ✅ 理解scanIoRequest()的作用机制

## 🎯 背景知识

### EPICS扫描机制

BPMIOC使用两种主要扫描方式：

#### 1. I/O中断扫描 (I/O Intr)

```
硬件中断/定时器
    ↓
my_thread() 每100ms执行
    ↓
scanIoRequest(ioScanPvt)  ← 触发扫描请求
    ↓
所有 SCAN="I/O Intr" 的记录被处理
```

**优点**:
- 响应快速，事件驱动
- 多个记录共享一次扫描
- 减少CPU占用

**用于**: RF幅度、相位等实时数据

#### 2. 定时扫描 (Periodic)

```
EPICS扫描任务
    ↓
每隔指定时间（如"1 second"）
    ↓
处理该记录
```

**优点**:
- 简单、独立
- 周期可控

**用于**: 慢速数据、计算记录

### BPMIOC中的扫描配置

查看当前配置：

```bash
cd ~/BPMIOC
grep -n "SCAN" db/BPMMonitor.db | head -20
```

你会看到：
```
record(ai, "$(P)RF3Amp") {
    field(SCAN, "I/O Intr")  ← I/O中断扫描
    ...
}

record(calc, "$(P)RF3Power") {
    field(SCAN, "1 second")  ← 每秒扫描一次
    ...
}
```

## 🔬 实验一: 修改驱动层扫描频率

### 任务: 将扫描频率从100ms改为50ms

#### 步骤1: 查看当前扫描代码

```bash
vim BPMmonitorApp/src/driverWrapper.c
```

找到 `my_thread()` 函数中的睡眠语句（约在第230行左右）：

```c
static void my_thread(void *arg)
{
    static double sim_time = 0.0;

    while (1) {
        if (use_simulation) {
            // 模拟数据更新
            for (int i = 0; i < 8; i++) {
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);
            }
            sim_time += 0.1;
        } else {
            // 真实硬件
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }
        }

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.1);  ← 这里！100ms = 0.1秒
    }
}
```

#### 步骤2: 修改睡眠时间并添加调试输出

修改为：

```c
static void my_thread(void *arg)
{
    static double sim_time = 0.0;
    static int loop_count = 0;

    while (1) {
        if (use_simulation) {
            // 模拟数据更新
            for (int i = 0; i < 8; i++) {
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);
            }
            sim_time += 0.05;  // 对应50ms
        } else {
            // 真实硬件
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }
        }

        // 调试输出：每20次循环（1秒）打印一次
        if (loop_count % 20 == 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            printf("[DRIVER] my_thread: loop=%d, time=%ld.%03ld, Amp[0]=%.3f\n",
                   loop_count, ts.tv_sec, ts.tv_nsec/1000000, Amp[0]);
        }

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.05);  // 50ms = 0.05秒

        loop_count++;
    }
}
```

**注意**: 需要在文件开头添加头文件：

```c
#include <time.h>
```

#### 步骤3: 重新编译并运行

```bash
cd ~/BPMIOC
make clean
make

cd iocBoot/iocBPMmonitor
./st.cmd
```

#### 步骤4: 观察输出

你应该看到：

```
[DRIVER] my_thread: loop=0, time=1699410123.456, Amp[0]=1.234
[DRIVER] my_thread: loop=20, time=1699410124.456, Amp[0]=1.567
[DRIVER] my_thread: loop=40, time=1699410125.456, Amp[0]=1.890
```

每20次循环约等于1秒（20 × 50ms = 1000ms），验证了50ms的扫描周期。

#### 步骤5: 测量更新频率

**在新终端**，使用camonitor测量实际更新频率：

```bash
camonitor -# 100 iLinac_007:BPM14And15:RF3Amp
```

观察时间戳，计算两次更新之间的间隔，应该约为50ms。

## 🔬 实验二: 修改数据库层扫描方式

### 任务: 将calc记录从定时扫描改为I/O中断扫描

#### 背景

当前RF功率计算记录使用定时扫描：

```bash
grep -A 5 "RF3Power" db/BPMMonitor.db
```

输出：
```
record(calc, "$(P)RF3Power") {
    field(SCAN, "1 second")
    field(CALC, "A*A*50")
    field(INPA, "$(P)RF3Amp")
    ...
}
```

这意味着功率计算每秒只执行一次，即使幅度数据每50ms更新一次。

#### 步骤1: 修改为I/O中断扫描

```bash
vim db/BPMMonitor.db
```

找到所有RF功率计算记录（RF3Power到RF10Power），修改SCAN字段：

```
record(calc, "$(P)RF3Power") {
    field(SCAN, "I/O Intr")  ← 从"1 second"改为"I/O Intr"
    field(SDIS, "$(P)RF3Amp.SEVR")  ← 可选：当Amp严重报警时禁用
    field(CALC, "A*A*50")
    field(INPA, "$(P)RF3Amp CP")  ← 添加CP（Change Process）
    field(PREC, "2")
    field(EGU,  "W")
}
```

**关键点**:
- `SCAN="I/O Intr"`: 使用I/O中断扫描
- `INPA="$(P)RF3Amp CP"`: CP表示"Change Process"，当RF3Amp变化时处理本记录

但是等等！这里有个问题：calc记录的I/O中断扫描需要特殊处理。

#### 步骤2: 正确的方法 - 使用CP链接

实际上，更好的方法是保持SCAN为Passive，但使用CP链接：

```
record(calc, "$(P)RF3Power") {
    field(SCAN, "Passive")    ← 被动扫描
    field(CALC, "A*A*50")
    field(INPA, "$(P)RF3Amp CP MS")  ← CP MS = Change Process, Maximize Severity
    field(PREC, "2")
    field(EGU,  "W")
}
```

**CP（Change Process）的工作原理**:

```
RF3Amp记录被I/O中断更新
    ↓
EPICS检查哪些记录链接到RF3Amp（带CP标志）
    ↓
处理RF3Power记录（执行CALC）
    ↓
RF3Power更新
```

#### 步骤3: 修改8个RF通道的功率记录

对RF3Power到RF10Power都应用相同的修改：

```bash
# 使用sed批量修改
cd ~/BPMIOC
sed -i 's/\(field(INPA, "$(P)RF[0-9]*Amp\)"/\1 CP MS"/g' db/BPMMonitor.db
```

验证修改：

```bash
grep -A 4 "RF.*Power" db/BPMMonitor.db | grep -E "(record|INPA)"
```

应该看到所有INPA字段都包含"CP MS"。

#### 步骤4: 重新编译并测试

```bash
make clean
make

cd iocBoot/iocBPMmonitor
./st.cmd
```

#### 步骤5: 验证效果

**在新终端**，同时监控幅度和功率：

```bash
camonitor iLinac_007:BPM14And15:RF3Amp iLinac_007:BPM14And15:RF3Power
```

你应该看到：
```
iLinac_007:BPM14And15:RF3Amp  2025-11-08 10:00:00.100  1.234
iLinac_007:BPM14And15:RF3Power 2025-11-08 10:00:00.101  76.13  # 立即更新！
iLinac_007:BPM14And15:RF3Amp  2025-11-08 10:00:00.150  1.567
iLinac_007:BPM14And15:RF3Power 2025-11-08 10:00:00.151  122.81  # 再次立即更新！
```

功率现在每次幅度变化时都会更新，而不是每秒一次！

## 🔬 实验三: 性能对比测试

### 任务: 比较不同扫描频率的CPU使用率

#### 步骤1: 测试50ms扫描

当前配置（50ms扫描），在新终端监控CPU：

```bash
# 找到IOC进程ID
ps aux | grep BPMmonitor

# 监控CPU使用率
top -p <PID>
```

记录稳定后的CPU使用率（例如：5%）。

#### 步骤2: 测试10ms扫描

修改 `driverWrapper.c` 中的 `epicsThreadSleep(0.05)` 为：

```c
epicsThreadSleep(0.01);  // 10ms
```

重新编译运行，再次记录CPU使用率（例如：15%）。

#### 步骤3: 测试200ms扫描

修改为：

```c
epicsThreadSleep(0.2);  // 200ms
```

重新编译运行，记录CPU使用率（例如：2%）。

#### 步骤4: 分析结果

填写下表：

| 扫描周期 | 每秒扫描次数 | CPU使用率 | 响应延迟 | 适用场景 |
|---------|-------------|----------|----------|---------|
| 10ms    | 100次       | ~15%     | 最低     | 快速变化的信号 |
| 50ms    | 20次        | ~5%      | 低       | 一般RF监控 |
| 100ms   | 10次        | ~3%      | 中等     | 默认配置 |
| 200ms   | 5次         | ~2%      | 较高     | 慢速变化 |
| 1秒     | 1次         | ~1%      | 高       | 状态监控 |

**结论**: 扫描频率是性能和响应性的权衡。

## 🧪 实验四: 添加动态扫描周期控制

### 任务: 通过PV动态调整扫描周期

#### 步骤1: 在driverWrapper.c中添加全局变量

```c
static double scan_period = 0.1;  // 默认100ms
```

#### 步骤2: 添加设置函数

```c
int SetScanPeriod(double period)
{
    if (period < 0.001 || period > 10.0) {
        printf("ERROR: Invalid scan period %.3f (must be 0.001-10.0)\n", period);
        return -1;
    }

    printf("INFO: Changing scan period from %.3f to %.3f seconds\n",
           scan_period, period);
    scan_period = period;
    return 0;
}
```

#### 步骤3: 修改my_thread使用变量

```c
static void my_thread(void *arg)
{
    // ... 现有代码 ...

    scanIoRequest(ioScanPvt);
    epicsThreadSleep(scan_period);  // 使用变量

    loop_count++;
}
```

#### 步骤4: 在driverWrapper.h中声明

```c
int SetScanPeriod(double period);
```

#### 步骤5: 添加数据库记录

在 `db/BPMMonitor.db` 中添加：

```
# 扫描周期控制
record(ao, "$(P)ScanPeriod") {
    field(DESC, "Scan Period Control")
    field(DTYP, "BPMMonitor")
    field(OUT,  "@SCANPERIOD")
    field(VAL,  "0.1")
    field(PREC, "3")
    field(EGU,  "seconds")
    field(DRVH, "10.0")
    field(DRVL, "0.001")
    field(HOPR, "1.0")
    field(LOPR, "0.01")
}

# 实际扫描频率（只读）
record(calc, "$(P)ScanRate") {
    field(DESC, "Scan Rate (Hz)")
    field(SCAN, "1 second")
    field(CALC, "1/A")
    field(INPA, "$(P)ScanPeriod")
    field(PREC, "2")
    field(EGU,  "Hz")
}
```

#### 步骤6: 在devBPMMonitor.c中添加支持

在 `write_ao()` 函数中添加：

```c
static long write_ao(aoRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    if (strcmp(pPvt->type_str, "SCANPERIOD") == 0) {
        if (SetScanPeriod(prec->val) == 0) {
            return 0;
        } else {
            return -1;
        }
    }

    // ... 原有代码 ...
}
```

#### 步骤7: 更新Offset定义

在 `driverWrapper.h` 中添加：

```c
#define OFFSET_SCANPERIOD  100
```

#### 步骤8: 测试动态调整

重新编译运行后：

```bash
# 读取当前周期
caget iLinac_007:BPM14And15:ScanPeriod
# iLinac_007:BPM14And15:ScanPeriod  0.1

# 读取扫描频率
caget iLinac_007:BPM14And15:ScanRate
# iLinac_007:BPM14And15:ScanRate    10

# 改为50ms（20Hz）
caput iLinac_007:BPM14And15:ScanPeriod 0.05
# Old : 0.1
# New : 0.05

caget iLinac_007:BPM14And15:ScanRate
# iLinac_007:BPM14And15:ScanRate    20

# 改为500ms（2Hz）
caput iLinac_007:BPM14And15:ScanPeriod 0.5

# 监控RF3Amp，观察更新频率变化
camonitor iLinac_007:BPM14And15:RF3Amp
```

## 📊 验证理解

回答以下问题：

### Q1: I/O中断扫描与定时扫描的主要区别是什么？

<details>
<summary>答案</summary>

**I/O中断扫描**:
- 事件驱动，由scanIoRequest()触发
- 多个记录共享一次扫描
- 适合快速响应的场景
- CPU效率高

**定时扫描**:
- 时间驱动，按固定周期扫描
- 每个记录独立
- 适合周期性任务
- 实现简单
</details>

### Q2: scanIoRequest()在哪里调用？作用是什么？

<details>
<summary>答案</summary>

**调用位置**: `driverWrapper.c` 的 `my_thread()` 函数中

**作用**: 触发I/O中断扫描请求，通知EPICS处理所有 `SCAN="I/O Intr"` 且连接到 `ioScanPvt` 的记录

**工作流程**:
```c
scanIoRequest(ioScanPvt);
    ↓
EPICS扫描线程被唤醒
    ↓
查找所有SCAN="I/O Intr"的记录
    ↓
调用每个记录的设备支持函数
    ↓
更新记录值
    ↓
通过CA广播给客户端
```
</details>

### Q3: CP链接的含义和作用？

<details>
<summary>答案</summary>

**CP = Change Process**（变化时处理）

**作用**: 当输入PV变化时，自动处理（扫描）本记录

**示例**:
```
field(INPA, "$(P)RF3Amp CP")
```

当RF3Amp更新时，自动计算本记录（例如功率计算）

**MS = Maximize Severity**: 同时传播报警严重性
</details>

### Q4: 如何选择合适的扫描周期？

<details>
<summary>答案</summary>

考虑因素：

1. **信号变化速度**: 快速变化需要短周期
2. **CPU资源**: 周期越短，CPU占用越高
3. **网络带宽**: 频繁更新会增加网络流量
4. **应用需求**: 控制需要快速响应，监控可以较慢

**经验法则**:
- 控制回路: 1-10ms
- 快速监控: 10-100ms
- 一般监控: 100ms-1s
- 状态检查: 1-10s
</details>

## 🚀 扩展练习

1. **练习1**: 添加扫描次数统计PV
   - 提示：在my_thread()中维护计数器
   - 添加longin记录显示总扫描次数

2. **练习2**: 实现自适应扫描周期
   - 当数值变化大时自动加快扫描
   - 当数值稳定时自动减慢扫描

3. **练习3**: 添加性能监控
   - 记录每次扫描的实际耗时
   - 计算扫描抖动（jitter）

## 📝 实验报告模板

```markdown
# Lab 02 实验报告

## 实验一：修改扫描频率
- 原始频率：100ms
- 修改后频率：50ms
- CPU使用率变化：3% → 5%
- 观察到的现象：更新更快，数据更平滑

## 实验二：修改扫描方式
- 修改的记录：RF3Power
- 原始SCAN：1 second
- 修改后SCAN：Passive with CP
- 效果：功率立即跟随幅度变化

## 实验三：性能对比
[填写性能测试表格]

## 实验四：动态控制
- 实现的功能：通过PV调整扫描周期
- 测试范围：10ms - 1s
- 遇到的问题：无

## 收获和体会
...
```

## 🔗 相关文档

- [Part 2: 05-scanning-basics.md](../../part2-understanding-basics/05-scanning-basics.md) - 扫描机制详解
- [Part 4: 03-thread-and-scanning.md](../../part4-driver-layer/03-thread-and-scanning.md) - 驱动层线程
- [Part 5: 06-io-interrupt.md](../../part5-device-support-layer/06-io-interrupt.md) - I/O中断实现

---

**🎉 恭喜完成实验！** 你已经掌握了EPICS扫描机制的核心知识。
