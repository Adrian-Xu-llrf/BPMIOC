# 异步I/O

> **目标**: 实现高性能异步设备支持
> **难度**: ⭐⭐⭐⭐⭐
> **预计时间**: 1-2周

## 异步设备支持

### 异步read_ai

```c
typedef struct {
    CALLBACK callback;
    aiRecord *prec;
    int request_id;
} AsyncRequest;

static long read_ai(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)prec->dpvt;
    
    // 启动异步请求
    int request_id = StartAsyncRead(pPvt->channel);
    
    if (request_id < 0) {
        return -1;  // 同步错误
    }
    
    // 创建异步请求结构
    AsyncRequest *req = malloc(sizeof(AsyncRequest));
    req->prec = prec;
    req->request_id = request_id;
    
    // 设置回调
    callbackSetCallback(async_callback, &req->callback);
    callbackSetUser(req, &req->callback);
    callbackSetPriority(priorityMedium, &req->callback);
    
    // 返回异步标志
    return 0;  // 实际应该返回特殊值表示异步
}

static void async_callback(CALLBACK *pcallback) {
    AsyncRequest *req;
    callbackGetUser(req, pcallback);
    
    aiRecord *prec = req->prec;
    
    // 获取结果
    float value = GetAsyncResult(req->request_id);
    prec->val = value;
    
    // 完成Record处理
    dbScanLock((dbCommon*)prec);
    (prec->rset->process)(prec);
    dbScanUnlock((dbCommon*)prec);
    
    free(req);
}
```

## asynDriver框架

### 使用asynDriver

```c
#include <asynDriver.h>
#include <asynInt32.h>

static asynStatus readInt32(void *drvPvt, asynUser *pasynUser,
                             epicsInt32 *value) {
    // 异步读取
    *value = ReadHardware();
    return asynSuccess;
}

static asynInt32 myInt32 = {
    writeInt32,
    readInt32,
    getBounds,
    NULL,
    NULL
};

int myDriverInit() {
    asynInterface *pinterface = pasynInt32->find(drvPvt, "asyn");
    pasynManager->registerPort("myPort", ASYN_MULTIDEVICE, 1, 0, 0);
    pasynManager->registerInterface("myPort", &myInt32);
}
```

## 请求队列

```c
#define MAX_QUEUE_SIZE 128

typedef struct {
    int requests[MAX_QUEUE_SIZE];
    int head;
    int tail;
    epicsMutexId mutex;
    epicsEventId event;
} RequestQueue;

void queue_push(RequestQueue *q, int request) {
    epicsMutexLock(q->mutex);
    q->requests[q->tail] = request;
    q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
    epicsMutexUnlock(q->mutex);
    epicsEventSignal(q->event);
}

int queue_pop(RequestQueue *q) {
    epicsEventWait(q->event);
    epicsMutexLock(q->mutex);
    int request = q->requests[q->head];
    q->head = (q->head + 1) % MAX_QUEUE_SIZE;
    epicsMutexUnlock(q->mutex);
    return request;
}
```

## 🔗 相关文档

- [05-thread-safety.md](./05-thread-safety.md)
- [01-performance-optimization.md](./01-performance-optimization.md)
