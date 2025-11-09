# 06: BPMIOC线程模型详解

> **难度**: ⭐⭐⭐⭐⭐
> **预计时间**: 80分钟
> **前置知识**: 01-architecture-overview.md, 02-data-flow.md, 04-memory-model.md

## 📋 本文目标

本文深入剖析BPMIOC的多线程模型，这是理解系统并发和同步的关键。

完成本文后，你将能够：
- ✅ 理解BPMIOC的线程架构
- ✅ 掌握I/O中断扫描机制
- ✅ 理解线程间的同步和通信
- ✅ 能够调试多线程问题

## 🧵 线程模型总览

BPMIOC是一个**多线程**系统，包含以下线程：

```
+---------------------------+
|   EPICS IOC 主线程         |  ← iocInit()创建
|   (epicsThreadCreate)     |
+---------------------------+
            │
            ├──> CA服务器线程（多个）     ← 处理客户端连接
            ├──> 扫描线程（多个）         ← 处理Record扫描
            ├──> 回调线程（多个）         ← 处理异步回调
            └──> 数据采集线程（1个）      ← pthread创建 ⭐重点
                       │
                       └──> 100ms周期轮询硬件
                               └──> scanIoRequest()触发I/O中断扫描
```

### 关键线程说明

| 线程名称 | 创建者 | 数量 | 作用 | 优先级 |
|---------|--------|------|------|--------|
| **IOC主线程** | iocInit() | 1 | 初始化系统，运行shell | 正常 |
| **CA服务器** | EPICS Base | 2+ | 监听CA请求，处理caget/caput | 正常 |
| **扫描线程** | EPICS Base | 10+ | 执行Record扫描（1s, 0.5s等） | 高 |
| **回调线程** | EPICS Base | 3 | 处理异步回调（低、中、高优先级） | 可变 |
| **数据采集线程** | pthread_create() | 1 | 从FPGA读取数据 ⭐ | 高 |

## ⭐ 数据采集线程详解

### 1. 线程创建

在`InitDevice()`函数中创建：

```c
// driverWrapper.c

long InitDevice()
{
    // 1. 初始化I/O扫描私有数据
    scanIoInit(&TriginScanPvt);
    scanIoInit(&TripBufferinScanPvt);
    scanIoInit(&ADCrawBufferinScanPvt);

    // 2. 加载硬件库（dlopen）
    void *handle = dlopen(dll_filename, RTLD_LAZY);
    // ... dlsym 获取函数指针

    // 3. 创建数据采集线程 ⭐
    pthread_t tidp1;
    int ret = pthread_create(&tidp1, NULL, pthread, NULL);
    if (ret == -1) {
        fprintf(stderr, "Create pthread error!\n");
        return -1;
    }

    // 4. 分离线程（不需要pthread_join）
    pthread_detach(tidp1);

    return 0;
}
```

### 2. 线程函数实现

```c
// driverWrapper.c

void *pthread(void *arg)
{
    while (1)  // 无限循环
    {
        // Step 1: 触发硬件采集
        // 通知FPGA准备好所有触发通道数据
        funcTriggerAllDataReached();

        // Step 2: 请求I/O中断扫描 ⭐核心！
        // 触发所有"I/O Intr"扫描的Record
        scanIoRequest(TriginScanPvt);

        // Step 3: 获取时间戳
        funcGetTimestampData(1, &TAISecond, &TAINanoSecond);

        // Step 4: 设置WR捕获触发
        funcSetWRCaputureDataTrigger();

        // Step 5: 休眠100ms（10 Hz采集频率）
        usleep(100000);  // 100ms = 100,000 微秒
    }

    return NULL;  // 永远不会执行到这里
}
```

### 3. 线程执行流程

```
数据采集线程 (每100ms)
    ↓
funcTriggerAllDataReached()  ← 通知FPGA准备数据
    ↓
scanIoRequest(TriginScanPvt) ← 触发I/O中断扫描 ⭐
    ↓
[EPICS扫描线程接管]
    ├──> 处理所有"I/O Intr"的Record
    │      ├──> read_ai()  ← 读取标量数据
    │      ├──> read_wf()  ← 读取波形数据
    │      └──> read_bo()  ← 读取状态
    │
    └──> Record处理完成
           └──> 通过CA发送给客户端
    ↓
[回到数据采集线程]
funcGetTimestampData()       ← 获取时间戳
    ↓
funcSetWRCaputureDataTrigger() ← 设置捕获触发
    ↓
usleep(100000)               ← 休眠100ms
    ↓
[循环重复]
```

## 🔔 I/O中断扫描机制

### 1. IOSCANPVT数据结构

```c
// EPICS Base定义（简化）
typedef struct IOSCANPVT {
    epicsMutex   *lock;        // 互斥锁
    epicsEvent   *event;       // 事件信号量
    ELLLIST       recordList;  // 等待扫描的Record链表
} *IOSCANPVT;
```

BPMIOC定义了3个I/O扫描私有数据：

```c
static IOSCANPVT TriginScanPvt;           // 触发数据扫描
static IOSCANPVT TripBufferinScanPvt;     // Trip缓冲扫描
static IOSCANPVT ADCrawBufferinScanPvt;   // ADC原始数据扫描
```

### 2. Record注册到IOSCANPVT

在`init_record()`时注册：

```c
// devBPMMonitor.c

long init_record_ai(aiRecord *prec)
{
    // ... 解析INP字段

    // 如果扫描机制是"I/O Intr"
    if (prec->scan == SCAN_IO_EVENT) {
        // 将这个Record添加到IOSCANPVT的链表
        prec->dpvt->ioscanpvt = devGetInTrigScanPvt();
    }

    return 0;
}

// get_ioint_info()提供IOSCANPVT
long get_ioint_info_ai(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    recordpara_t *priv = (recordpara_t *)prec->dpvt;

    // 返回这个Record关联的IOSCANPVT
    *ppvt = priv->ioscanpvt;

    return 0;
}
```

### 3. scanIoRequest()触发扫描

```c
// 数据采集线程调用
scanIoRequest(TriginScanPvt);
```

**内部工作原理**（EPICS Base实现）：

```c
void scanIoRequest(IOSCANPVT pvt)
{
    // 1. 获取锁
    epicsMutexLock(pvt->lock);

    // 2. 设置事件信号
    epicsEventSignal(pvt->event);

    // 3. 释放锁
    epicsMutexUnlock(pvt->lock);

    // ──> 唤醒等待的扫描线程
}
```

### 4. 扫描线程处理

```c
// EPICS Base扫描线程（简化）
void scanIoThread(IOSCANPVT pvt)
{
    while (1) {
        // 1. 等待事件信号
        epicsEventWait(pvt->event);

        // 2. 获取锁
        epicsMutexLock(pvt->lock);

        // 3. 遍历Record链表
        ELLNODE *node;
        for (node = ellFirst(&pvt->recordList); node; node = ellNext(node)) {
            dbCommon *prec = (dbCommon *)node->data;

            // 4. 处理Record
            dbScanLock(prec);
            dbProcess(prec);  // ──> 调用read函数
            dbScanUnlock(prec);
        }

        // 5. 释放锁
        epicsMutexUnlock(pvt->lock);
    }
}
```

## 🔒 线程同步机制

### 1. 全局数据的互斥保护

虽然当前代码没有显式使用互斥锁，但**应该添加**：

```c
// 推荐改进：添加互斥锁保护全局数组

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();

        // 加锁：保护全局数组写入
        pthread_mutex_lock(&data_mutex);

        // 从硬件读取数据到全局数组
        // （当前是funcTriggerAllDataReached内部完成）

        pthread_mutex_unlock(&data_mutex);

        // 触发扫描
        scanIoRequest(TriginScanPvt);

        usleep(100000);
    }
}

// read函数也需要加锁
static long read_ai(aiRecord *prec)
{
    float value;

    pthread_mutex_lock(&data_mutex);
    value = ReadData(priv->offset, priv->channel, priv->type);
    pthread_mutex_unlock(&data_mutex);

    prec->val = value;
    return 0;
}
```

### 2. EPICS的Record锁

EPICS Base为每个Record提供了锁：

```c
// Record处理时自动加锁
dbScanLock(prec);      // 获取Record锁
dbProcess(prec);       // 处理Record（调用read/write）
dbScanUnlock(prec);    // 释放Record锁
```

**作用**：
- 防止同一个Record被多个线程同时处理
- 确保Record字段的一致性

### 3. 线程间通信：事件机制

```
数据采集线程                EPICS扫描线程
     │                          │
     │  scanIoRequest()          │
     │ ─────────────────────────>│
     │  (epicsEventSignal)       │
     │                          │ epicsEventWait()解除阻塞
     │                          │
     │                          │ 处理所有"I/O Intr"的Record
     │                          │   ├──> read_ai()
     │                          │   ├──> read_wf()
     │                          │   └──> ...
     │                          │
     │  继续下一个周期            │ 处理完成，继续等待
     │                          │
```

## ⏱️ 时序分析

### 1. 典型时序图

```
时间轴 ──────────────────────────────────────────>

数据采集线程:
    ├─ funcTrigger... ─┤ scanIoRequest ├─ usleep(100ms) ─┤
    │  (~1ms)           │   (~1μs)       │                 │
                                         └──> 下一个周期

EPICS扫描线程:
                        ├─ 处理Record ─────┤
                        │  (~10ms, 取决于Record数量)

客户端感知:
                                ├─ 收到新数据 ─┤
                                   (CA传输延迟 ~1ms)

总延迟: ~12ms (从硬件触发到客户端收到)
```

### 2. 性能指标

| 指标 | 数值 | 说明 |
|------|------|------|
| **采集周期** | 100ms | `usleep(100000)` |
| **采集频率** | 10 Hz | 1 / 0.1s |
| **硬件触发延迟** | ~1ms | `funcTriggerAllDataReached()` |
| **扫描触发延迟** | ~1μs | `scanIoRequest()` |
| **Record处理时间** | ~10ms | 取决于Record数量 |
| **CA传输延迟** | ~1ms | 网络延迟 |
| **总端到端延迟** | ~12ms | 从触发到客户端收到 |

## 🐛 多线程调试技巧

### 1. 查看线程信息

```bash
# 方法1：使用ps查看线程
ps -T -p $(pidof st.cmd)

# 输出示例：
  PID  SPID TTY      STAT   TIME COMMAND
 1234  1234 pts/0    Sl+    0:01 /opt/epics/base/bin/linux-x86_64/softIoc st.cmd
 1234  1235 pts/0    Sl+    0:00  \_ [CA server]
 1234  1236 pts/0    Sl+    0:00  \_ [CA server]
 1234  1237 pts/0    Sl+    0:00  \_ [scanOnce]
 1234  1238 pts/0    Sl+    0:00  \_ [scan-1.0]
 1234  1239 pts/0    Sl+    0:00  \_ [pthread]       ← 数据采集线程

# 方法2：使用top查看线程
top -H -p $(pidof st.cmd)
```

### 2. 使用gdb调试多线程

```bash
# 启动IOC
./st.cmd &

# 附加到进程
gdb -p $(pidof st.cmd)

# 查看所有线程
(gdb) info threads

# 输出示例：
  Id   Target Id         Frame
  1    Thread 0x1234     main () at iocsh.c:123
  2    Thread 0x1235     ca_server_thread () at cas.c:456
  3    Thread 0x1236     scanOnce_thread () at scan.c:789
* 4    Thread 0x1237     pthread () at driverWrapper.c:393

# 切换到线程4（数据采集线程）
(gdb) thread 4

# 查看当前位置
(gdb) backtrace

# 查看线程正在执行的代码
(gdb) list

# 设置断点
(gdb) break pthread
(gdb) break scanIoRequest

# 继续执行
(gdb) continue
```

### 3. 添加调试输出

```c
// driverWrapper.c

void *pthread(void *arg)
{
    int count = 0;

    while (1) {
        count++;

        // 调试输出：线程ID和循环计数
        if (count % 100 == 0) {  // 每10秒打印一次
            printf("[pthread] tid=%ld, count=%d\n",
                   pthread_self(), count);
        }

        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        // 调试：Record处理时间
        struct timeval tv1, tv2;
        gettimeofday(&tv1, NULL);

        usleep(100000);

        gettimeofday(&tv2, NULL);
        long elapsed = (tv2.tv_sec - tv1.tv_sec) * 1000000
                     + (tv2.tv_usec - tv1.tv_usec);

        if (elapsed > 110000) {  // 超过110ms（理论100ms）
            printf("[pthread] WARNING: cycle took %ld us\n", elapsed);
        }
    }
}
```

## 🚨 常见多线程问题

### 问题1：数据竞争（Data Race）

**症状**：PV值偶尔出现异常、波形数据混乱

**原因**：数据采集线程和EPICS扫描线程同时访问全局数组

```c
// 错误示例（无保护）
void *pthread() {
    while (1) {
        rf3amp[0] = GetRFInfo(3, 0);  // 写入
        scanIoRequest(...);
        usleep(100000);
    }
}

long read_ai(aiRecord *prec) {
    float value = rf3amp[0];  // 读取 ← 可能与写入冲突！
    prec->val = value;
}
```

**解决方案**：添加互斥锁

```c
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

void *pthread() {
    while (1) {
        pthread_mutex_lock(&data_mutex);
        rf3amp[0] = GetRFInfo(3, 0);
        pthread_mutex_unlock(&data_mutex);

        scanIoRequest(...);
        usleep(100000);
    }
}

long read_ai(aiRecord *prec) {
    pthread_mutex_lock(&data_mutex);
    float value = rf3amp[0];
    pthread_mutex_unlock(&data_mutex);

    prec->val = value;
}
```

### 问题2：死锁（Deadlock）

**症状**：IOC卡住，无响应

**原因**：两个锁的获取顺序不一致

```c
// 错误示例
// 线程A
pthread_mutex_lock(&lock1);
pthread_mutex_lock(&lock2);  // ← 等待lock2

// 线程B
pthread_mutex_lock(&lock2);
pthread_mutex_lock(&lock1);  // ← 等待lock1

// 结果：死锁！
```

**解决方案**：
1. 总是按相同顺序获取锁
2. 使用`pthread_mutex_trylock()`避免阻塞
3. 减少锁的数量

### 问题3：优先级反转

**症状**：高优先级线程被低优先级线程阻塞

**解决方案**：使用优先级继承互斥锁

```c
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

pthread_mutex_init(&data_mutex, &attr);
```

## 💡 线程模型优化建议

### 1. 添加互斥锁保护

```c
// driverWrapper.c

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// 所有全局数组访问都加锁
float ReadData(int offset, int channel, int type)
{
    float result;

    pthread_mutex_lock(&data_mutex);

    switch (offset) {
        // ... 读取全局数组
    }

    pthread_mutex_unlock(&data_mutex);

    return result;
}
```

### 2. 可配置的采集周期

```c
// 允许通过PV修改采集周期
static int acquisition_period_us = 100000;  // 默认100ms

record(longout, "LLRF:BPM:AcqPeriod")
{
    field(DTYP, "Soft Channel")
    field(VAL,  "100000")
}

void *pthread() {
    while (1) {
        // ...
        usleep(acquisition_period_us);  // 使用可配置周期
    }
}
```

### 3. 线程优先级设置

```c
int InitDevice()
{
    pthread_t tidp1;
    pthread_attr_t attr;

    // 设置线程属性
    pthread_attr_init(&attr);

    // 设置为实时优先级
    struct sched_param param;
    param.sched_priority = 50;  // 0-99
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);

    // 创建线程
    pthread_create(&tidp1, &attr, pthread, NULL);

    pthread_attr_destroy(&attr);
}
```

## ✅ 学习检查点

完成本文后，你应该能够回答：

1. **线程架构**：
   - [ ] BPMIOC有哪些线程？各自的作用？
   - [ ] 数据采集线程的执行周期？
   - [ ] 为什么需要数据采集线程？

2. **I/O中断扫描**：
   - [ ] `scanIoRequest()`的作用？
   - [ ] IOSCANPVT是什么？
   - [ ] Record如何注册到IOSCANPVT？

3. **线程同步**：
   - [ ] 为什么需要互斥锁保护全局数组？
   - [ ] EPICS的Record锁保护什么？
   - [ ] 如何避免死锁？

4. **调试**：
   - [ ] 如何查看IOC的线程列表？
   - [ ] 如何用gdb调试多线程？
   - [ ] 数据竞争的症状和解决方案？

## 🔗 相关文档

- **[02-data-flow.md](./02-data-flow.md)** - 理解数据流动
- **[04-memory-model.md](./04-memory-model.md)** - 内存模型
- **[08-performance-analysis.md](./08-performance-analysis.md)** - 性能分析（下一步）

## 📚 扩展阅读

### POSIX Threads
- 《POSIX Threads Programming》
- [pthread手册](https://man7.org/linux/man-pages/man7/pthreads.7.html)

### EPICS多线程
- [EPICS Application Developer's Guide - Thread Safety](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/ThreadSafety.html)
- [EPICS IOC Core Locking](https://epics.anl.gov/tech-talk/2010/msg00123.php)

---

**下一篇**: [07-error-handling.md](./07-error-handling.md) - 错误处理策略

**实践练习**:
1. 使用`ps -T`查看IOC的线程，识别数据采集线程
2. 修改采集周期为200ms，观察PV更新频率的变化
3. 添加互斥锁保护全局数组，编译测试
4. 使用gdb附加到IOC，切换到数据采集线程并设置断点
