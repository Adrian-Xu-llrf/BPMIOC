# Week 2 - Device Support 开发

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 开发自己的 Device Support

---

## Day 1-2: Device Support 架构（4小时）

### Device Support 作用

```
Record (数据库) ↔ Device Support (抽象层) ↔ Driver Support (硬件层)
```

### DSET 结构

```c
typedef struct {
    long number;            // 函数数量
    DEVSUPFUN report;       // 报告函数
    DEVSUPFUN init;         // 初始化
    DEVSUPFUN init_record;  // Record 初始化
    DEVSUPFUN get_ioint_info; // I/O中断信息
    DEVSUPFUN read_write;   // 读/写函数
    DEVSUPFUN special_linconv; // 线性转换
} dset;
```

---

## Day 3-4: 编写 Device Support（4小时）

### devBPMMonitor.c 框架

```c
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "aiRecord.h"
#include "epicsExport.h"

/* 私有数据结构 */
typedef struct {
    int channel;
    int offset;
} DevicePrivate;

/* 初始化 Record */
static long init_record_ai(aiRecord *prec) {
    DevicePrivate *pPvt;

    /* 分配私有数据 */
    pPvt = (DevicePrivate *)malloc(sizeof(DevicePrivate));
    if (!pPvt) return S_db_noMemory;

    /* 解析 INP 字段 */
    /* 例如: "@AMP:0 ch=2" */
    sscanf(prec->inp.value.instio.string, 
           "@AMP:%d ch=%d", &pPvt->offset, &pPvt->channel);

    prec->dpvt = pPvt;
    return 0;
}

/* 读取数据 */
static long read_ai(aiRecord *prec) {
    DevicePrivate *pPvt = (DevicePrivate *)prec->dpvt;

    /* 从硬件读取 */
    float value = ReadHardware(pPvt->offset, pPvt->channel);

    prec->val = value;
    prec->udf = FALSE;

    return 2;  /* 不转换 */
}

/* Device Support 表 */
struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
} devAiBPM = {
    5,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai
};

epicsExportAddress(dset, devAiBPM);
```

---

## Day 5-6: I/O Intr 支持（4小时）

### 添加 I/O 中断

```c
#include "callback.h"
#include "dbScan.h"

static IOSCANPVT ioScanPvt;

/* I/O 中断信息 */
static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt) {
    *ppvt = ioScanPvt;
    return 0;
}

/* 初始化（在 Driver 中调用） */
void InitIOIntr() {
    scanIoInit(&ioScanPvt);
}

/* 触发 I/O 中断（在数据到达时调用） */
void TriggerIOIntr() {
    scanIoRequest(ioScanPvt);
}
```

---

## Day 7: 编译和测试（2小时）

### devBPMMonitor.dbd

```
device(ai, INST_IO, devAiBPM, "BPMmonitor")
```

### Makefile

```makefile
# 在 src/Makefile 中添加
PROD_IOC = BPMMonitor
DBD += BPMMonitor.dbd
BPMMonitor_DBD += base.dbd
BPMMonitor_DBD += devBPMMonitor.dbd

# 源文件
BPMMonitor_SRCS += devBPMMonitor.c
BPMMonitor_SRCS += BPMMonitor_registerRecordDeviceDriver.cpp

# 库
BPMMonitor_LIBS += $(EPICS_BASE_IOC_LIBS)
```

### 测试

```bash
$ make
$ cd iocBoot/iocBPMMonitor
$ ./st.cmd
epics> dbl
BPM:CH0:Amp
BPM:CH1:Amp
...
```

---

## 实战项目

基于 BPMIOC 项目的 `devBPMMonitor.c`，实现：

1. 4通道幅度读取
2. 4通道相位读取
3. I/O Intr 支持
4. 错误处理

参考代码：`BPMIOC/BPMmonitor/src/devBPMMonitor.c`

继续下周学习 Driver Support！
