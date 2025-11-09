# Part 6.4: 扫描机制详解

> **目标**: 深入理解EPICS的扫描机制和性能优化
> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 90分钟

## 📖 什么是扫描（Scan）？

**扫描** = **Processing**（处理）

即调用Record的process()函数，执行：
1. 调用设备支持的read()/write()
2. 更新VAL字段
3. 检查报警
4. 触发monitors
5. 执行FLNK链接

**扫描的核心问题**: **何时**处理Record？

---

## 🎯 SCAN字段的值

### 完整列表

```c
field(SCAN, "...")

可选值：
  "Passive"          # 被动扫描（默认）
  ".1 second"        # 周期扫描（0.1秒）
  ".2 second"        # 0.2秒
  ".5 second"        # 0.5秒（BPMIOC常用）
  "1 second"         # 1秒
  "2 second"         # 2秒
  "5 second"         # 5秒
  "10 second"        # 10秒
  "I/O Intr"         # I/O中断扫描
  "Event"            # 事件扫描
```

---

## 🔄 扫描机制详解

### 1. Passive（被动扫描）

**特点**: 不会自动处理，只在以下情况触发：

1. **外部caput**
   ```bash
   caput Test:PV.PROC 1  # 强制处理
   ```

2. **FLNK触发**
   ```
   record(ai, "A") {
       field(SCAN, ".5 second")
       field(FLNK, "B")  # 触发B
   }
   record(calc, "B") {
       field(SCAN, "Passive")  # 被动扫描
   }
   ```

3. **数据库链接（PP或CP）**
   ```
   record(calc, "C") {
       field(SCAN, "Passive")
       field(INPA, "A PP")  # PP = Process Passive
   }
   ```

**使用场景**:
- calc Record（由其他Record触发）
- 输出Record（由上层控制系统写入）
- 中间计算节点

---

### 2. 周期扫描（Periodic Scan）

**工作原理**:

```
EPICS Scan Thread（每个周期一个线程）
    ↓
定时器触发（如每0.5秒）
    ↓
扫描该周期的所有Record
    ↓
    Record 1 → process()
    Record 2 → process()
    ...
    Record N → process()
```

**示例**:
```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")  # 每0.5秒处理
}
```

**内部实现**:

EPICS创建了多个扫描线程：
```c
Scan Thread 1: 处理".1 second"的所有Record
Scan Thread 2: 处理".2 second"的所有Record
Scan Thread 3: 处理".5 second"的所有Record
...
```

**性能考虑**:

| 扫描周期 | CPU占用 | 延迟 | 适用场景 |
|----------|---------|------|----------|
| `.1 second` | 高 | 0-100ms | 快速变化的信号 |
| `.5 second` | 中 | 0-500ms | 常规监控（**BPMIOC**） |
| `1 second` | 低 | 0-1s | 慢速数据 |
| `10 second` | 很低 | 0-10s | 很慢的数据 |

---

### 3. I/O中断扫描（I/O Intr）

**最重要的扫描机制！**

#### 工作原理

```
硬件事件（如触发信号）
    ↓
驱动层检测到事件
    ↓
scanIoRequest(ioscanpvt)  ← 驱动调用
    ↓
EPICS唤醒I/O中断扫描线程
    ↓
处理所有注册到该ioscanpvt的Record
```

**完整流程（BPMIOC）**:

```
1. Record配置
   record(ai, "$(P):RFIn_01_Amp") {
       field(SCAN, "I/O Intr")  ← 启用I/O中断扫描
   }

2. init_record时
   scanIoInit(&pPvt->ioscanpvt);  ← 初始化

3. get_ioint_info时
   *ppvt = pPvt->ioscanpvt;  ← 告诉EPICS用哪个ioscanpvt

4. 驱动层触发
   pthread (driverWrapper.c) {
       while (1) {
           wait_for_trigger();  ← 等待硬件触发
           scanIoRequest(TriginScanPvt);  ← 触发扫描！
       }
   }

5. EPICS处理
   I/O Intr Scan Thread → 处理所有SCAN="I/O Intr"的Record
```

---

#### BPMIOC的I/O中断实现

**driverWrapper.c中的触发线程**:

```c
// 全局ioscanpvt
IOSCANPVT TriginScanPvt;

// 初始化
int drvBPMmonitorInit(void)
{
    scanIoInit(&TriginScanPvt);  // 创建扫描结构

    // 创建线程等待触发
    pthread_create(&tid, NULL, BPM_Trigin_thread, NULL);
}

// 触发线程
void *BPM_Trigin_thread(void *arg)
{
    while (1) {
        // 等待硬件触发（例如：读取GPIO）
        wait_for_trigger();

        // 触发扫描！
        scanIoRequest(TriginScanPvt);
    }
}
```

**设备支持层连接**:

```c
// devBPMMonitor.c
static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 返回全局TriginScanPvt（定义在driverWrapper.c）
    *ppvt = TriginScanPvt;

    return 0;
}
```

---

#### I/O中断 vs. 周期扫描对比

**场景**: 读取100Hz的数据

**方案A: 周期扫描**
```
field(SCAN, ".01 second")  # 100Hz扫描

问题：
  ❌ CPU占用高（即使没有新数据也扫描）
  ❌ 延迟不确定（0-10ms）
  ❌ 可能错过数据（如果硬件触发不规律）
```

**方案B: I/O中断扫描**
```
field(SCAN, "I/O Intr")

优势：
  ✅ 只在有新数据时处理
  ✅ 延迟低（<1ms）
  ✅ 不会错过数据
  ✅ CPU占用低
```

---

### 4. Event扫描

**用途**: 软件事件触发

**配置**:
```
record(ai, "Test:PV")
{
    field(SCAN, "Event")
    field(EVNT, "1")  # 事件号
}
```

**触发**:
```c
// 在代码中
post_event(1);  // 触发所有EVNT=1的Record
```

**使用场景**:
- 软件状态机
- 同步多个Record
- 自定义触发逻辑

---

## 🔍 SCAN机制的内部实现

### EPICS的扫描线程

**IOC启动时**:

```c
iocInit()
  ↓
创建扫描线程：
  - 每个周期一个线程（.1s, .2s, .5s, ...）
  - I/O中断扫描线程（优先级最高）
  - Event扫描线程
```

**线程优先级**:

| 扫描类型 | 优先级 | 说明 |
|----------|--------|------|
| I/O Intr | 最高 | 实时响应硬件 |
| .1 second | 高 | 快速周期 |
| .5 second | 中 | 中速周期 |
| 1 second+ | 低 | 慢速周期 |

---

### Record处理顺序

**周期扫描线程**:

```c
// 伪代码
void scan_thread_05s()  // 0.5秒扫描线程
{
    while (1) {
        wait_for_timer(0.5);  // 等待0.5秒

        // 按Record添加顺序处理
        for (rec in scan_list_05s) {
            dbProcess(rec);  // 处理Record
        }
    }
}
```

**注意**: 同一周期内的Record是**顺序处理**的！

---

## ⚡ 性能优化

### 优化1: 合理选择扫描周期

```
# ❌ 不好：所有PV都用.1秒
record(ai, "SlowTemp") {
    field(SCAN, ".1 second")  # 温度变化慢，不需要这么快
}

# ✅ 好：根据信号速度选择
record(ai, "SlowTemp") {
    field(SCAN, "5 second")  # 温度用5秒
}
record(ai, "FastBeamPos") {
    field(SCAN, "I/O Intr")  # 束流位置用I/O中断
}
```

---

### 优化2: 使用I/O中断代替高频周期扫描

```
# ❌ 不好：高CPU占用
field(SCAN, ".01 second")  # 100Hz周期扫描

# ✅ 好：只在有数据时处理
field(SCAN, "I/O Intr")  # 硬件触发
```

---

### 优化3: 避免同步阻塞

**问题**: 如果一个Record处理很慢，会阻塞同周期的其他Record

```
record(ai, "SlowPV") {
    field(SCAN, ".5 second")
    # read()需要100ms → 阻塞其他.5秒扫描的Record
}
```

**解决**:
1. 异步设备支持（Asynchronous Device Support）
2. 移到更慢的扫描周期
3. 使用Passive + 独立线程

---

### 优化4: 监控扫描性能

**IOC Shell命令**:

```bash
# 查看扫描统计
scanppl

输出：
0.1 second scan thread:
  100 records
  Max process time: 0.005 seconds

0.5 second scan thread:
  250 records
  Max process time: 0.012 seconds

I/O Intr scan:
  50 records
```

**分析**:
- 如果Max process time接近扫描周期，说明**处理太慢**
- 需要优化或调整扫描周期

---

## 🎨 高级用法

### PHAS - 处理相位

**用途**: 控制同一扫描周期内的处理顺序

```
record(ai, "A") {
    field(SCAN, ".5 second")
    field(PHAS, "0")  # 第一批处理
}
record(calc, "B") {
    field(SCAN, ".5 second")
    field(PHAS, "1")  # 第二批处理（A之后）
    field(INPA, "A")
}
```

**处理顺序**:
```
.5秒扫描线程:
  1. 处理所有PHAS=0的Record
  2. 处理所有PHAS=1的Record
  3. ...
```

---

### SDIS - 禁用扫描

**用途**: 条件性禁用Record处理

```
record(ai, "Sensor") {
    field(SCAN, ".5 second")
    field(SDIS, "System:Enable")  # 禁用链接
    field(DISV, "0")  # 当Enable=0时禁用
}

record(bi, "System:Enable") {
    field(VAL, "1")  # 1=启用，0=禁用
}
```

**工作原理**:
```
每次扫描时:
1. 读取SDIS指向的PV（System:Enable）
2. 如果值 == DISV (0)，跳过处理
3. 否则正常处理
```

---

### PRIO - 处理优先级

**用途**: 设置Record的调度优先级

```
record(ai, "HighPrio") {
    field(PRIO, "HIGH")  # 高优先级
}
record(ai, "MediumPrio") {
    field(PRIO, "MEDIUM")  # 中优先级（默认）
}
record(ai, "LowPrio") {
    field(PRIO, "LOW")  # 低优先级
}
```

**注意**: 这影响Callback任务的优先级，不影响扫描线程。

---

## 🔧 调试扫描问题

### 问题1: Record不更新

**检查**:
```bash
# 查看Record的SCAN字段
dbgf Test:PV.SCAN

# 查看上次处理时间
dbgf Test:PV.TIME

# 强制处理一次
caput Test:PV.PROC 1
```

**常见原因**:
- SCAN="Passive"且无触发源
- SDIS禁用了扫描
- 设备支持read()返回错误

---

### 问题2: I/O中断不工作

**检查**:
```c
// 设备支持层
static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    // ✅ 确保正确返回ioscanpvt
    *ppvt = TriginScanPvt;
    return 0;
}

// 驱动层
// ✅ 确保调用了scanIoRequest()
scanIoRequest(TriginScanPvt);
```

**调试**:
```c
// 添加调试信息
printf("scanIoRequest called\n");
```

---

### 问题3: 扫描太慢

**分析**:
```bash
# 查看扫描统计
scanppl

# 找出慢的Record
# 在设备支持中添加计时
static long read_ai(aiRecord *prec)
{
    epicsTimeStamp t1, t2;
    epicsTimeGetCurrent(&t1);

    // ... read操作 ...

    epicsTimeGetCurrent(&t2);
    double dt = epicsTimeDiffInSeconds(&t2, &t1);
    if (dt > 0.01) {
        printf("%s: read took %.3f ms\n", prec->name, dt*1000);
    }
}
```

---

## ✅ 学习检查点

- [ ] 理解Passive、周期扫描、I/O中断的区别
- [ ] 理解I/O中断扫描的完整流程
- [ ] 知道如何选择合适的扫描机制
- [ ] 理解扫描性能优化方法
- [ ] 能够调试扫描问题

---

## 🎯 总结

### 扫描机制的本质

**三个核心问题**:
1. **何时处理？** → SCAN字段
2. **如何触发？** → 周期/中断/事件
3. **顺序如何？** → PHAS字段

### 最佳实践

| 数据类型 | 推荐SCAN | 原因 |
|----------|----------|------|
| 快速硬件事件 | `I/O Intr` | 低延迟，低CPU |
| 中速监控 | `.5 second` | 平衡性能和更新率 |
| 慢速数据 | `5 second` | 节省CPU |
| 计算节点 | `Passive` | 由输入触发 |
| 输出控制 | `Passive` | 由上层控制 |

### BPMIOC的选择

```
BPMIOC使用".5 second"周期扫描
原因：
  ✅ 简单可靠
  ✅ 0.5秒延迟可接受
  ✅ CPU占用合理

如果需要更低延迟：
  → 改用"I/O Intr"（需要硬件触发支持）
```

**下一步**: 学习Part 6.5 - st.cmd启动脚本详解，了解IOC的启动过程！

---

**关键理解**: 扫描机制是EPICS的"心跳" - 选择合适的机制对系统性能至关重要！💡
