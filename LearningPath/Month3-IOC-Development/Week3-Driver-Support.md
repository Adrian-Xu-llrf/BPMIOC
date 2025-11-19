# Week 3 - Driver Support 开发

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 开发 Driver Support，实现硬件接口

---

## Day 1-2: Driver Support 架构（4小时）

### Driver vs Device Support

```
Device Support:  Record ↔ DSET (通用接口)
Driver Support:  DSET ↔ DRVET ↔ 硬件
```

### DRVET 结构

```c
typedef struct {
    long number;
    long (*report)(int level);
    long (*init)(void);
} drvet;
```

---

## Day 3-4: 实现 Driver Support（4小时）

### drvBPMWrapper.c

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>

#include "drvSup.h"
#include "epicsExport.h"
#include "dbScan.h"

/* 硬件寄存器地址 */
#define BPM_BASE_ADDR 0x43C00000

/* 动态库句柄 */
static void *g_libHandle = NULL;

/* 函数指针 */
static float (*funcGetAmplitude)(int ch) = NULL;
static float (*funcGetPhase)(int ch) = NULL;

/* I/O中断扫描 */
extern IOSCANPVT g_ioScanPvt;

/* 初始化驱动 */
static long init_driver(void) {
    printf("Initializing BPM Driver...\n");

    /* 加载动态库 */
    g_libHandle = dlopen("libbpm_hw.so", RTLD_NOW);
    if (!g_libHandle) {
        printf("Error: Failed to load libbpm_hw.so\n");
        return -1;
    }

    /* 获取函数指针 */
    funcGetAmplitude = dlsym(g_libHandle, "GetAmplitude");
    funcGetPhase = dlsym(g_libHandle, "GetPhase");

    if (!funcGetAmplitude || !funcGetPhase) {
        printf("Error: Failed to load functions\n");
        return -1;
    }

    /* 初始化 I/O 中断 */
    scanIoInit(&g_ioScanPvt);

    /* 启动数据采集线程 */
    pthread_t tid;
    pthread_create(&tid, NULL, data_acquisition_thread, NULL);

    printf("BPM Driver initialized successfully\n");
    return 0;
}

/* 报告函数 */
static long report_driver(int level) {
    printf("BPM Driver Report:\n");
    printf("  Library: %s\n", g_libHandle ? "Loaded" : "Not loaded");
    return 0;
}

/* 数据采集线程 */
static void* data_acquisition_thread(void *arg) {
    while (1) {
        /* 等待硬件中断或定时触发 */
        usleep(10000);  // 10ms

        /* 触发 I/O 中断扫描 */
        scanIoRequest(g_ioScanPvt);
    }
    return NULL;
}

/* 读取硬件数据 */
float ReadAmplitude(int channel) {
    if (funcGetAmplitude) {
        return funcGetAmplitude(channel);
    }
    return 0.0;
}

float ReadPhase(int channel) {
    if (funcGetPhase) {
        return funcGetPhase(channel);
    }
    return 0.0;
}

/* Driver Support 表 */
static drvet drvBPM = {
    2,
    report_driver,
    init_driver
};

epicsExportAddress(drvet, drvBPM);
```

---

## Day 5-6: 硬件接口实现（4小时）

### 硬件访问库 bpm_hw.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define BPM_BASE_ADDR 0x43C00000
#define BPM_SIZE 0x10000

static volatile unsigned int *g_bpm_regs = NULL;

/* 初始化硬件访问 */
int InitHardware(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return -1;
    }

    g_bpm_regs = (unsigned int *)mmap(
        NULL, 
        BPM_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        BPM_BASE_ADDR
    );

    close(fd);

    if (g_bpm_regs == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    printf("Hardware initialized at 0x%08X\n", BPM_BASE_ADDR);
    return 0;
}

/* 读取幅度 */
float GetAmplitude(int channel) {
    if (channel < 0 || channel > 3) return 0.0;

    unsigned int raw = g_bpm_regs[channel];
    return (float)raw / 65536.0;  // 转换为浮点数
}

/* 读取相位 */
float GetPhase(int channel) {
    if (channel < 0 || channel > 3) return 0.0;

    unsigned int raw = g_bpm_regs[4 + channel];
    return (float)raw * 360.0 / 65536.0;  // 转换为角度
}

/* 写入寄存器 */
void WriteRegister(int offset, unsigned int value) {
    g_bpm_regs[offset] = value;
}
```

---

## Day 7: 集成和测试（2小时）

### 编译硬件库

```makefile
# libbpm_hw.so
libbpm_hw.so: bpm_hw.c
	gcc -shared -fPIC -o libbpm_hw.so bpm_hw.c
```

### drvBPMWrapper.dbd

```
driver(drvBPM)
```

### Makefile 集成

```makefile
# Driver
BPMMonitor_SRCS += drvBPMWrapper.c
BPMMonitor_DBD += drvBPMWrapper.dbd

# 链接动态加载库
BPMMonitor_LIBS += dl
```

### st.cmd 加载

```bash
dbLoadDatabase "dbd/BPMMonitor.dbd"
BPMMonitor_registerRecordDeviceDriver pdbbase

# Driver 会自动初始化
# 加载数据库
dbLoadRecords("db/BPM.db")

iocInit
```

---

## 实战项目

参考 BPMIOC 项目：

1. 研究 `driverWrapper.c` 的实现
2. 理解动态库加载机制
3. 实现自己的硬件访问库
4. 测试 I/O 中断机制

关键文件：
- `BPMIOC/BPMmonitor/src/driverWrapper.c`
- `BPMIOC/BPMmonitor/src/devBPMMonitor.c`

继续下周完成完整项目！
