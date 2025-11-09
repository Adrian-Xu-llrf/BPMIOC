# 数据采集线程详解 (pthread)

> **阅读时间**: 45分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: POSIX线程、I/O Interrupt扫描、线程同步

## 📋 本文目标

- 理解pthread数据采集线程的作用
- 掌握100ms采集周期的设计
- 理解scanIoRequest()的触发机制
- 学会线程的创建和管理

## 🎯 为什么需要独立线程？

### EPICS的扫描机制

```
EPICS Record扫描方式:
├─ Passive: 被动扫描 (需要手动触发)
├─ Periodic: 周期扫描 (0.1s, 1s, 10s等)
└─ I/O Intr: I/O中断扫描 (硬件驱动)
              ↑
          需要外部触发
```

### BPMIOC的选择：I/O Intr + pthread

```
[pthread线程]  ──每100ms──→  scanIoRequest()  ──→  [EPICS Record处理]
     ↓                             ↓
采集硬件数据                 触发所有I/O Intr记录
更新全局缓冲区               Record从缓冲区读数据
```

**为什么不用Periodic扫描？**
- ❌ Periodic最小周期0.1s，不够灵活
- ❌ 无法精确控制采集时序
- ❌ 难以与硬件触发同步
- ✅ I/O Intr响应更快、更精确

## 📊 线程架构

### 整体数据流

```
┌────────────────────────────────────────────────────────┐
│                  pthread 数据采集线程                    │
│  ┌──────────────────────────────────────────────────┐  │
│  │  while (1) {                                     │  │
│  │    1. funcTriggerAllDataReached()  触发硬件采集  │  │
│  │    2. scanIoRequest(TriginScanPvt) 触发Record    │  │
│  │    3. usleep(100000)               等待100ms     │  │
│  │  }                                               │  │
│  └──────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
         ↓ scanIoRequest
┌────────────────────────────────────────────────────────┐
│              EPICS I/O Interrupt 机制                   │
│  TriginScanPvt ──→ 所有注册的I/O Intr Record           │
└────────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────────┐
│            Record处理 (devBPMMonitor.c)                 │
│  read_ai() ──→ ReadData() ──→ 读取全局缓冲区           │
└────────────────────────────────────────────────────────┘
```

## 1. 线程创建

### 1.1 全局变量声明

```c
// driverWrapper.c 全局变量区域

static pthread_t tidp1;              // 线程ID
static IOSCANPVT TriginScanPvt;      // I/O扫描私有数据
static void *pthread(void *arg);     // 线程函数声明
```

### 1.2 IOSCANPVT初始化

```c
// driverWrapper.c InitDevice() line 351-360

long InitDevice()
{
    // 1. 初始化I/O扫描私有数据
    scanIoInit(&TriginScanPvt);

    // ... dlopen/dlsym ...

    // ... 硬件初始化 ...

    // 5. 创建数据采集线程
    int ret = pthread_create(&tidp1, NULL, pthread, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to create thread: %d\n", ret);
        return -1;
    }

    printf("Data acquisition thread created: tid=%lu\n", tidp1);

    return 0;
}
```

**scanIoInit()作用**:
- 初始化IOSCANPVT结构
- 为I/O Interrupt扫描做准备
- 创建Record列表用于存储注册的Record

### 1.3 pthread_create()详解

```c
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg);
```

**参数**:
- `thread`: 输出线程ID
- `attr`: 线程属性 (NULL=默认)
- `start_routine`: 线程函数
- `arg`: 传递给线程函数的参数

**BPMIOC的使用**:
```c
pthread_create(&tidp1,    // 输出: 线程ID
               NULL,      // 默认属性
               pthread,   // 线程函数
               NULL);     // 无参数
```

## 2. 线程函数实现

### 2.1 完整代码

```c
// driverWrapper.c line 551-600

void *pthread(void *arg)
{
    printf("Data acquisition thread started\n");

    while (1) {
        // ===== Step 1: 触发硬件数据采集 =====
        if (funcTriggerAllDataReached != NULL) {
            int status = funcTriggerAllDataReached();
            if (status != 0) {
                fprintf(stderr, "Hardware trigger failed: %d\n", status);
            }
        }

        // ===== Step 2: 触发EPICS Record处理 =====
        scanIoRequest(TriginScanPvt);

        // ===== Step 3: 等待100ms =====
        usleep(100000);  // 100,000微秒 = 100毫秒
    }

    return NULL;  // 永远不会到达
}
```

### 2.2 Step 1: 触发硬件数据采集

```c
int status = funcTriggerAllDataReached();
```

**funcTriggerAllDataReached()的作用**:
- 通知硬件/FPGA开始数据采集
- 采集所有通道的数据
- 更新全局缓冲区

**Mock实现**:
```c
// libbpm_mock.c
int TriggerAllDataReached(void)
{
    // 生成模拟数据并更新全局buffer
    for (int i = 0; i < buf_len; i++) {
        rf3amp[i] = 100.0 + sin(i * 0.01) * 10.0;
        rf3phase[i] = cos(i * 0.01) * 180.0;
        // ... 其他数据
    }

    return 0;  // 成功
}
```

**Real实现**:
```c
// libbpm_zynq.c
int TriggerAllDataReached(void)
{
    // 触发FPGA采集
    Xil_Out32(FPGA_TRIGGER_REG, 0x1);

    // 等待数据就绪 (轮询或中断)
    while ((Xil_In32(FPGA_STATUS_REG) & 0x1) == 0) {
        usleep(100);  // 等待100μs
    }

    // 从FPGA读取数据到全局buffer
    dma_read(FPGA_DATA_BASE, rf3amp, buf_len * sizeof(float));
    // ... 读取其他数据

    return 0;
}
```

### 2.3 Step 2: 触发EPICS Record处理

```c
scanIoRequest(TriginScanPvt);
```

**scanIoRequest()的作用**:
- 触发所有注册到`TriginScanPvt`的Record
- 这些Record的SCAN字段设置为`I/O Intr`
- EPICS会调用这些Record的process函数

**哪些Record会被触发？**
```
BPMMonitor.db中所有SCAN="I/O Intr"的Record:
├─ LLRF:BPM:RF3Amp
├─ LLRF:BPM:RF3Phase
├─ LLRF:BPM:X1
├─ LLRF:BPM:Y1
├─ ... (所有实时数据PV)
```

**触发流程**:
```
scanIoRequest(TriginScanPvt)
    ↓
EPICS扫描系统
    ↓
遍历TriginScanPvt的Record列表
    ↓
对每个Record调用 record->process()
    ↓
devBPMMonitor.c的read_ai()被调用
    ↓
调用ReadData()读取数据
    ↓
更新Record的VAL字段
    ↓
通过CA网络发送给客户端
```

### 2.4 Step 3: 等待100ms

```c
usleep(100000);  // 100,000微秒 = 100毫秒
```

**为什么是100ms？**
- **采样率**: 10 Hz (每秒10次)
- **数据量**: buf_len=10000，假设ADC采样率100kHz → 10000/100000 = 100ms
- **性能**: 10 Hz足够实时监控，不会过载CPU
- **网络**: CA网络可以轻松处理10 Hz更新

**时间精度**:
```c
// usleep不是精确定时
实际周期: 100ms ± 几毫秒
原因: Linux调度器不保证实时性

// 如需高精度，使用实时线程
struct sched_param param;
param.sched_priority = 50;
pthread_setschedparam(tidp1, SCHED_FIFO, &param);
```

## 3. I/O Interrupt机制详解

### 3.1 IOSCANPVT结构

```c
// EPICS内部结构 (简化)
typedef struct ioscanpvt {
    epicsMutexId lock;           // 互斥锁
    ELLLIST recordList;          // Record列表
    epicsEventId triggered;      // 触发事件
} *IOSCANPVT;
```

**作用**:
- `recordList`: 存储所有注册的Record
- `triggered`: 事件标志，scanIoRequest()时设置
- `lock`: 保护列表的并发访问

### 3.2 Record注册流程

```c
// devBPMMonitor.c 的 get_ioint_info()

static long get_ioint_info(int cmd, aiRecord *precord, IOSCANPVT *ppvt)
{
    // 返回TriginScanPvt给EPICS
    *ppvt = TriginScanPvt;

    return 0;
}
```

**EPICS的处理**:
1. IOC启动时，遍历所有SCAN="I/O Intr"的Record
2. 调用每个Record的get_ioint_info()
3. 获取IOSCANPVT指针
4. 将Record添加到IOSCANPVT的recordList中

**结果**:
```
TriginScanPvt.recordList:
├─ LLRF:BPM:RF3Amp的Record结构
├─ LLRF:BPM:RF3Phase的Record结构
├─ LLRF:BPM:X1的Record结构
└─ ... (所有I/O Intr Record)
```

### 3.3 scanIoRequest()内部实现

```c
// EPICS源码 (简化)
void scanIoRequest(IOSCANPVT pvt)
{
    epicsMutexLock(pvt->lock);

    // 遍历所有注册的Record
    ELLNODE *node = ellFirst(&pvt->recordList);
    while (node != NULL) {
        dbCommon *precord = (dbCommon *)node;

        // 请求Record处理
        dbScanLock(precord);
        (*precord->rset->process)(precord);
        dbScanUnlock(precord);

        node = ellNext(node);
    }

    epicsMutexUnlock(pvt->lock);
}
```

**关键步骤**:
1. 加锁（保护Record列表）
2. 遍历Record列表
3. 对每个Record调用process()
4. 解锁

## 4. 时序分析

### 4.1 单次采集周期

```
时间轴 (100ms周期)
├─ t=0ms:    funcTriggerAllDataReached() 开始
├─ t=5ms:    硬件采集完成，更新缓冲区
├─ t=6ms:    scanIoRequest() 触发Record
├─ t=7ms:    Record处理开始
├─ t=8ms:    read_ai() → ReadData()
├─ t=9ms:    Record处理完成，CA发送
├─ t=10ms:   所有处理完成
├─ t=10-100ms: usleep等待
└─ t=100ms:  下一次循环开始
```

**实际耗时**:
- 硬件采集: ~5ms
- Record处理: ~5ms
- 空闲时间: ~90ms

**CPU占用率**: 10ms / 100ms = 10%

### 4.2 多Record并发处理

```
scanIoRequest() 触发后:

Record 1 (RF3Amp)      ────┐
Record 2 (RF3Phase)    ────┼─→ 并发处理
Record 3 (X1)          ────┼─→ (EPICS多线程)
Record 4 (Y1)          ────┘
...
```

**EPICS的处理方式**:
- 每个Record有自己的锁 (dbScanLock)
- 可以并发处理互不依赖的Record
- scanIoRequest()是异步的，不会阻塞pthread

## 5. 线程管理

### 5.1 线程生命周期

```
IOC启动
  ↓
InitDevice()
  ↓
pthread_create() ──→ [pthread线程开始]
  ↓                       ↓
主线程继续              while(1) {
  ↓                       采集数据
IOC运行                   触发Record
  ↓                       等待100ms
  ...                   }
  ↓                       ↑
IOC关闭                   │
  ↓                       │
(线程未正确退出)    (死循环，永不退出)
```

### 5.2 改进：优雅退出

```c
// 添加退出标志
static volatile int thread_should_exit = 0;

void *pthread(void *arg)
{
    printf("Thread started\n");

    while (!thread_should_exit) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }

    printf("Thread exiting\n");
    return NULL;
}

// 在IOC退出时调用
void ShutdownDevice(void)
{
    printf("Shutting down...\n");

    thread_should_exit = 1;

    // 等待线程退出
    pthread_join(tidp1, NULL);

    // 关闭硬件
    if (funcSystemClose != NULL) {
        funcSystemClose();
    }

    // 关闭动态库
    if (handle != NULL) {
        dlclose(handle);
    }

    printf("Shutdown complete\n");
}
```

### 5.3 线程优先级设置

```c
// 提高线程优先级 (需要root权限)
long InitDevice()
{
    // ... 创建线程 ...

    struct sched_param param;
    param.sched_priority = 50;  // 0-99，值越大优先级越高

    int ret = pthread_setschedparam(tidp1, SCHED_FIFO, &param);
    if (ret != 0) {
        fprintf(stderr, "Failed to set thread priority: %d\n", ret);
        // 继续运行，但使用默认优先级
    } else {
        printf("Thread priority set to %d\n", param.sched_priority);
    }

    return 0;
}
```

**何时需要实时优先级？**
- 采集频率 > 100 Hz
- 对时间抖动要求严格 (< 1ms)
- 系统负载很高

**BPMIOC的选择**: 10 Hz不需要实时优先级

## 6. 错误处理

### 6.1 硬件采集失败

```c
void *pthread(void *arg)
{
    int consecutive_errors = 0;

    while (1) {
        int status = funcTriggerAllDataReached();

        if (status != 0) {
            consecutive_errors++;
            fprintf(stderr, "Hardware trigger failed (%d consecutive)\n",
                    consecutive_errors);

            if (consecutive_errors > 10) {
                fprintf(stderr, "Too many errors, stopping thread\n");
                break;  // 退出线程
            }
        } else {
            consecutive_errors = 0;  // 重置错误计数
        }

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }

    return NULL;
}
```

### 6.2 监控线程健康状态

```c
// 添加心跳计数器
static volatile unsigned long heartbeat = 0;

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        heartbeat++;  // 心跳递增

        usleep(100000);
    }
}

// 定期检查心跳 (可以在另一个线程或EPICS Record中)
unsigned long last_heartbeat = 0;

void checkThreadHealth(void)
{
    if (heartbeat == last_heartbeat) {
        fprintf(stderr, "WARNING: Thread may be stuck!\n");
    } else {
        last_heartbeat = heartbeat;
    }
}
```

## 7. 性能优化

### 7.1 减少系统调用

```c
// ❌ 每次调用usleep
while (1) {
    // ...
    usleep(100000);
}

// ✅ 使用nanosleep (更精确)
struct timespec req = {0, 100000000};  // 0秒 + 100,000,000纳秒
while (1) {
    // ...
    nanosleep(&req, NULL);
}
```

### 7.2 批量处理

```c
void *pthread(void *arg)
{
    while (1) {
        // 一次性触发所有数据采集
        funcTriggerAllDataReached();

        // 可选：再次触发确保数据最新
        // usleep(1000);  // 等待1ms
        // funcTriggerAllDataReached();

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

### 7.3 使用条件变量 (高级)

```c
// 代替usleep，使用条件变量可以立即唤醒
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *pthread(void *arg)
{
    struct timespec ts;

    while (1) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        // 等待100ms或被唤醒
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000;  // +100ms
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        pthread_mutex_lock(&mutex);
        pthread_cond_timedwait(&cond, &mutex, &ts);
        pthread_mutex_unlock(&mutex);
    }
}

// 可以从外部立即触发采集
void triggerImmediately(void)
{
    pthread_cond_signal(&cond);
}
```

## ❓ 常见问题

### Q1: 为什么用pthread而不是EPICS线程？
**A**:
- EPICS线程(epicsThread)是跨平台的，但功能有限
- pthread是Linux标准，功能强大
- BPMIOC主要运行在Linux，pthread足够

### Q2: 100ms周期可以改吗？
**A**:
```c
// 修改周期
#define ACQUISITION_PERIOD_US 50000  // 50ms = 20 Hz

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);
        usleep(ACQUISITION_PERIOD_US);
    }
}
```

### Q3: 如何确认线程在运行？
**A**:
```bash
# 查看IOC进程的线程
ps -eLf | grep st.cmd

# 输出示例:
# user  1234  1233  1234  0    2 10:00 pts/0  00:00:00 ./st.cmd
# user  1234  1233  1235  0    2 10:00 pts/0  00:00:01 ./st.cmd
#                    ↑
#                  线程ID不同

# 查看线程栈
cat /proc/1234/task/1235/stack
```

### Q4: 能否有多个采集线程？
**A**: 可以，但要注意：
```c
static pthread_t tidp1, tidp2;
static IOSCANPVT TriginScanPvt1, TriginScanPvt2;

// 线程1: 采集RF数据
void *pthread1(void *arg) {
    while (1) {
        funcTriggerRFData();
        scanIoRequest(TriginScanPvt1);
        usleep(50000);  // 50ms
    }
}

// 线程2: 采集XY数据
void *pthread2(void *arg) {
    while (1) {
        funcTriggerXYData();
        scanIoRequest(TriginScanPvt2);
        usleep(100000);  // 100ms
    }
}
```

## 📚 延伸阅读

- [Part 5: 02-iointr-scan.md](../part5-device-support-layer/02-iointr-scan.md) - I/O Interrupt扫描详解
- `man pthread_create` - pthread文档
- `man usleep` - 延时函数文档
- EPICS Application Developer's Guide - Chapter 6: Database Scanning

## 🎓 本章总结

- ✅ pthread线程负责周期性数据采集
- ✅ 100ms周期 = 10 Hz采样率
- ✅ funcTriggerAllDataReached()触发硬件
- ✅ scanIoRequest()触发EPICS Record
- ✅ I/O Interrupt机制实现事件驱动

**核心流程**: 采集 → 触发 → 等待 (循环)

**下一步**: 阅读 [07-readdata.md](./07-readdata.md) 学习ReadData()函数详解

---

**实验任务**: 修改采集周期为50ms，观察CA更新频率变化
