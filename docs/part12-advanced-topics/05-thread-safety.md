# 线程安全

> **目标**: 编写线程安全的代码
> **难度**: ⭐⭐⭐⭐⭐
> **预计时间**: 1周

## EPICS线程原语

### epicsMutex

```c
#include <epicsMutex.h>

epicsMutexId g_mutex;

void InitMutex() {
    g_mutex = epicsMutexCreate();
}

void ThreadSafeFunction() {
    epicsMutexLock(g_mutex);
    
    // 临界区代码
    // ...
    
    epicsMutexUnlock(g_mutex);
}
```

### epicsEvent

```c
#include <epicsEvent.h>

epicsEventId g_event;

void Producer() {
    // 生产数据
    ProduceData();
    
    // 通知消费者
    epicsEventSignal(g_event);
}

void Consumer() {
    // 等待数据
    epicsEventWait(g_event);
    
    // 消费数据
    ConsumeData();
}
```

## Scan Lock

```c
// Record处理时自动加锁
static long read_ai(aiRecord *prec) {
    // IOC自动加锁，无需手动加锁
    DevPvt *pPvt = (DevPvt*)prec->dpvt;
    prec->val = ReadData(pPvt->offset, pPvt->channel, pPvt->type);
    return 0;
}
```

## 无锁编程

### Ring Buffer

```c
#include <epicsRingBytes.h>

epicsRingBytesId g_ring;

void InitRingBuffer() {
    g_ring = epicsRingBytesCreate(1024);
}

void Producer() {
    char data[128];
    epicsRingBytesPut(g_ring, data, sizeof(data));
}

void Consumer() {
    char data[128];
    epicsRingBytesGet(g_ring, data, sizeof(data));
}
```

## 常见竞态条件

### 检查-使用模式

```c
// 错误：检查和使用之间没有原子性
if (g_initialized == 0) {  // 线程1检查
    // 线程2可能在这里插入
    Initialize();          // 线程1和2都可能执行
    g_initialized = 1;
}

// 正确：使用互斥锁
epicsMutexLock(g_mutex);
if (g_initialized == 0) {
    Initialize();
    g_initialized = 1;
}
epicsMutexUnlock(g_mutex);
```

## 🔗 相关文档

- [06-asynchronous-io.md](./06-asynchronous-io.md)
- [01-performance-optimization.md](./01-performance-optimization.md)
