# 日志系统完全指南

> **目标**: 掌握EPICS IOC日志系统
> **难度**: ⭐⭐⭐
> **预计时间**: 1-2天
> **前置知识**: C语言基础、EPICS基础

## 📋 本文档内容

1. EPICS日志系统概述
2. errlogPrintf使用
3. 日志级别和过滤
4. 日志文件配置
5. 实战案例
6. 最佳实践

## 🎯 为什么需要日志

日志是调试和监控IOC的重要工具：
- ✅ **问题定位**: 快速找到错误发生的位置
- ✅ **运行监控**: 了解IOC运行状态
- ✅ **性能分析**: 记录关键操作的时间
- ✅ **审计追踪**: 记录重要操作的历史
- ✅ **远程诊断**: 通过日志文件远程分析问题

## 1️⃣ EPICS日志系统概述

### EPICS日志架构

```
应用代码
  ↓ errlogPrintf()
errlog系统 (缓冲区)
  ↓
日志监听器
  ├→ 控制台输出
  ├→ 日志文件
  └→ 网络日志服务器
```

### 三种日志方式

| 方式 | 函数 | 优点 | 缺点 | 适用场景 |
|------|------|------|------|----------|
| **printf** | `printf()` | 简单直接 | 不带时间戳、线程不安全 | 开发调试 |
| **errlogPrintf** | `errlogPrintf()` | 线程安全、可重定向 | EPICS专用 | 生产环境 |
| **syslog** | `syslog()` | 系统级日志 | 需要配置 | 系统集成 |

## 2️⃣ errlogPrintf使用

### 基本用法

```c
#include <errlog.h>

// 简单消息
errlogPrintf("IOC started successfully\n");

// 格式化输出
errlogPrintf("InitDevice: Loaded library from %s\n", libpath);

// 多个参数
errlogPrintf("ReadData: offset=%d, channel=%d, value=%.3f\n",
             offset, channel, value);
```

### 在BPMIOC中添加日志

**示例1**: 在InitDevice中添加日志

```c
// driverWrapper.c
#include <errlog.h>

int InitDevice(void) {
    errlogPrintf("=== InitDevice: Starting device initialization ===\n");

    const char *libpath = getenv("BPM_DRIVER_LIB");
    if (libpath == NULL) {
        libpath = "/opt/BPMDriver/lib/libBPMDriver.so";
        errlogPrintf("InitDevice: Using default library path: %s\n", libpath);
    } else {
        errlogPrintf("InitDevice: Using library path from env: %s\n", libpath);
    }

    g_lib_handle = dlopen(libpath, RTLD_LAZY);
    if (!g_lib_handle) {
        errlogPrintf("ERROR: InitDevice: Failed to load library: %s\n",
                     dlerror());
        return -1;
    }

    errlogPrintf("InitDevice: Library loaded successfully\n");

    // 加载函数指针...
    BPM_RFIn_ReadADC = (BPM_RFIn_ReadADC_t)dlsym(g_lib_handle,
                                                  "BPM_RFIn_ReadADC");
    if (!BPM_RFIn_ReadADC) {
        errlogPrintf("ERROR: InitDevice: Failed to find BPM_RFIn_ReadADC: %s\n",
                     dlerror());
        return -1;
    }

    errlogPrintf("InitDevice: All function pointers loaded\n");
    errlogPrintf("=== InitDevice: Initialization completed successfully ===\n");

    return 0;
}
```

**示例2**: 在ReadData中添加调试日志

```c
float ReadData(int offset, int channel, int type) {
    float ret = 0.0;

    // 添加参数日志
    errlogPrintf("ReadData: offset=%d, channel=%d, type=%d\n",
                 offset, channel, type);

    // 边界检查
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        errlogPrintf("ERROR: ReadData: Invalid channel %d (max=%d)\n",
                     channel, MAX_RF_CHANNELS);
        return 0.0;
    }

    switch (offset) {
        case OFFSET_AMP:
            ret = g_data_buffer[offset][channel];
            errlogPrintf("ReadData: AMP[%d]=%.3f\n", channel, ret);
            break;

        case OFFSET_PHA:
            ret = g_data_buffer[offset][channel];
            errlogPrintf("ReadData: PHA[%d]=%.3f\n", channel, ret);
            break;

        // ...

        default:
            errlogPrintf("WARNING: ReadData: Unknown offset %d\n", offset);
            break;
    }

    return ret;
}
```

**示例3**: 在设备支持层添加日志

```c
// devBPMMonitor.c
static long init_record_ai(aiRecord *prec) {
    errlogPrintf("init_record_ai: Initializing record %s\n", prec->name);

    struct link *plink = &prec->inp;

    if (plink->type != INST_IO) {
        errlogPrintf("ERROR: init_record_ai: Invalid link type for %s\n",
                     prec->name);
        return S_db_badField;
    }

    DevPvt *pPvt = (DevPvt*)malloc(sizeof(DevPvt));
    if (!pPvt) {
        errlogPrintf("ERROR: init_record_ai: malloc failed for %s\n",
                     prec->name);
        return S_db_noMemory;
    }

    int nvals = sscanf(plink->value.instio.string, "@%d %d %d",
                       &pPvt->offset, &pPvt->channel, &pPvt->type);

    if (nvals != 3) {
        errlogPrintf("ERROR: init_record_ai: Invalid INP format for %s: '%s'\n",
                     prec->name, plink->value.instio.string);
        free(pPvt);
        return S_db_badField;
    }

    errlogPrintf("init_record_ai: %s configured with offset=%d, channel=%d, type=%d\n",
                 prec->name, pPvt->offset, pPvt->channel, pPvt->type);

    scanIoInit(&pPvt->ioscanpvt);
    prec->dpvt = pPvt;

    return 0;
}
```

### 日志级别

EPICS提供了日志严重性级别：

```c
#include <errlog.h>

// 致命错误
errlogSevFatal("System failed, shutting down\n");

// 主要错误
errlogSevMajor("Failed to read hardware\n");

// 次要错误
errlogSevMinor("Configuration warning\n");

// 信息
errlogMessage("IOC started\n");
```

严重性宏：

```c
#define errlogFatal     errlogSevPrintf(errlogFatal,
#define errlogMajor     errlogSevPrintf(errlogMajor,
#define errlogMinor     errlogSevPrintf(errlogMinor,
#define errlogInfo      errlogSevPrintf(errlogInfo,
```

使用示例：

```c
if (!g_lib_handle) {
    errlogSevPrintf(errlogFatal,
                    "InitDevice: Cannot load driver library\n");
    return -1;
}

if (channel >= MAX_RF_CHANNELS) {
    errlogSevPrintf(errlogMajor,
                    "ReadData: Channel %d out of range\n", channel);
    return 0.0;
}

if (value > THRESHOLD) {
    errlogSevPrintf(errlogMinor,
                    "ReadData: Value %.3f exceeds threshold\n", value);
}

errlogSevPrintf(errlogInfo,
                "InitDevice: Initialization completed\n");
```

## 3️⃣ 日志配置

### 在st.cmd中配置日志

```bash
# iocBoot/iocBPMmonitor/st.cmd

# 1. 设置日志输出到文件
eltc 0  # 禁用时间戳（可选）
iocLogInit

# 或者指定日志文件
epicsEnvSet("EPICS_IOC_LOG_FILE_NAME", "/var/log/BPMmonitor.log")

# 2. 设置日志缓冲区大小
errlogInit(5000)  # 5000行缓冲

# 3. 加载数据库和IOC初始化
dbLoadDatabase("dbd/BPMmonitor.dbd")
BPMmonitor_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("db/BPMmonitor.db")

iocInit()

# 4. 启动后日志
epicsThreadSleep(1.0)
date
```

### 日志文件轮转

创建日志管理脚本 `/etc/logrotate.d/bpmmonitor`:

```
/var/log/BPMmonitor.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 0644 epics epics
    postrotate
        /usr/bin/killall -HUP BPMmonitor
    endscript
}
```

### 实时查看日志

```bash
# 查看实时日志
tail -f /var/log/BPMmonitor.log

# 过滤ERROR
tail -f /var/log/BPMmonitor.log | grep ERROR

# 过滤特定函数
tail -f /var/log/BPMmonitor.log | grep ReadData
```

## 4️⃣ 日志最佳实践

### ✅ 好的日志

```c
// 1. 包含上下文信息
errlogPrintf("ReadData: offset=%d, channel=%d, value=%.3f\n",
             offset, channel, value);

// 2. 明确错误原因
errlogPrintf("ERROR: InitDevice: dlopen failed: %s\n", dlerror());

// 3. 记录关键操作
errlogPrintf("InitDevice: Starting thread BPM_Trigin_thread\n");
pthread_create(&trigin_tid, NULL, BPM_Trigin_thread, NULL);
errlogPrintf("InitDevice: Thread created with TID=%lu\n", trigin_tid);

// 4. 使用适当的日志级别
if (critical_error) {
    errlogSevPrintf(errlogFatal, "System halted\n");
} else if (recoverable_error) {
    errlogSevPrintf(errlogMajor, "Operation failed, retrying\n");
} else {
    errlogSevPrintf(errlogInfo, "Normal operation\n");
}
```

### ❌ 不好的日志

```c
// 1. 无用的日志
errlogPrintf("here\n");  // 在哪里？
errlogPrintf("debug\n"); // 调试什么？

// 2. 缺少上下文
errlogPrintf("Error\n");  // 什么错误？

// 3. 日志过多
for (int i = 0; i < 1000000; i++) {
    errlogPrintf("Loop %d\n", i);  // 会淹没日志！
}

// 4. 敏感信息
errlogPrintf("Password: %s\n", password);  // 不要记录密码！
```

### 日志级别使用指南

| 级别 | 使用场景 | 示例 |
|------|----------|------|
| **Fatal** | 系统无法继续运行 | 驱动库加载失败 |
| **Major** | 重要功能失败 | 硬件通信失败 |
| **Minor** | 非关键问题 | 配置警告 |
| **Info** | 正常操作信息 | 初始化完成 |

### 条件编译调试日志

对于开发时的详细日志，使用条件编译：

```c
// driverWrapper.c

#ifdef DEBUG_READDATA
#define DEBUG_LOG(...) errlogPrintf(__VA_ARGS__)
#else
#define DEBUG_LOG(...) do {} while(0)
#endif

float ReadData(int offset, int channel, int type) {
    DEBUG_LOG("ReadData: ENTER offset=%d, channel=%d, type=%d\n",
              offset, channel, type);

    float ret = g_data_buffer[offset][channel];

    DEBUG_LOG("ReadData: EXIT ret=%.3f\n", ret);

    return ret;
}
```

编译时启用：

```makefile
# Makefile
ifdef DEBUG
USR_CFLAGS += -DDEBUG_READDATA
endif
```

使用：

```bash
# 开发时启用调试日志
make DEBUG=1

# 生产环境不启用
make
```

## 5️⃣ 实战案例

### 案例1: 追踪初始化过程

**目标**: 记录IOC启动的每一步

```c
// driverWrapper.c
int InitDevice(void) {
    errlogPrintf("========================================\n");
    errlogPrintf("InitDevice: BEGIN\n");
    errlogPrintf("========================================\n");

    // 步骤1: 加载库
    errlogPrintf("[1/5] Loading driver library...\n");
    const char *libpath = getenv("BPM_DRIVER_LIB");
    if (!libpath) {
        libpath = DEFAULT_DRIVER_LIB;
        errlogPrintf("      Using default: %s\n", libpath);
    }

    g_lib_handle = dlopen(libpath, RTLD_LAZY);
    if (!g_lib_handle) {
        errlogSevPrintf(errlogFatal, "      FAILED: %s\n", dlerror());
        return -1;
    }
    errlogPrintf("      SUCCESS\n");

    // 步骤2: 加载函数指针
    errlogPrintf("[2/5] Loading function pointers...\n");
    // ... (类似地记录每个函数)
    errlogPrintf("      SUCCESS: All %d functions loaded\n", num_functions);

    // 步骤3: 初始化缓冲区
    errlogPrintf("[3/5] Initializing data buffers...\n");
    memset(g_data_buffer, 0, sizeof(g_data_buffer));
    errlogPrintf("      SUCCESS: Buffer size = %lu bytes\n",
                 sizeof(g_data_buffer));

    // 步骤4: 初始化硬件
    errlogPrintf("[4/5] Initializing hardware...\n");
    if (BPM_DeviceInit() != 0) {
        errlogSevPrintf(errlogFatal, "      FAILED\n");
        return -1;
    }
    errlogPrintf("      SUCCESS\n");

    // 步骤5: 启动线程
    errlogPrintf("[5/5] Starting acquisition thread...\n");
    pthread_create(&trigin_tid, NULL, BPM_Trigin_thread, NULL);
    errlogPrintf("      SUCCESS: Thread TID=%lu\n", trigin_tid);

    errlogPrintf("========================================\n");
    errlogPrintf("InitDevice: COMPLETED SUCCESSFULLY\n");
    errlogPrintf("========================================\n");

    return 0;
}
```

**输出示例**:

```
========================================
InitDevice: BEGIN
========================================
[1/5] Loading driver library...
      Using default: /opt/BPMDriver/lib/libBPMDriver.so
      SUCCESS
[2/5] Loading function pointers...
      SUCCESS: All 15 functions loaded
[3/5] Initializing data buffers...
      SUCCESS: Buffer size = 2560 bytes
[4/5] Initializing hardware...
      SUCCESS
[5/5] Starting acquisition thread...
      SUCCESS: Thread TID=12345
========================================
InitDevice: COMPLETED SUCCESSFULLY
========================================
```

### 案例2: 性能日志

记录关键操作的时间：

```c
#include <time.h>
#include <sys/time.h>

// 辅助函数：获取微秒时间戳
static double get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void *BPM_Trigin_thread(void *arg) {
    double start_time, end_time, elapsed;

    while (1) {
        start_time = get_timestamp_ms();

        // 等待触发
        BPM_WaitForTrig();

        // 读取数据
        for (int ch = 0; ch < MAX_RF_CHANNELS; ch++) {
            g_data_buffer[OFFSET_AMP][ch] = BPM_RFIn_ReadADC(ch, 0);
            g_data_buffer[OFFSET_PHA][ch] = BPM_RFIn_ReadADC(ch, 1);
            // ...
        }

        // 触发扫描
        scanIoRequest(TriginScanPvt);

        end_time = get_timestamp_ms();
        elapsed = end_time - start_time;

        // 记录性能
        errlogPrintf("BPM_Trigin_thread: Acquisition took %.3f ms\n", elapsed);

        // 性能警告
        if (elapsed > 10.0) {
            errlogSevPrintf(errlogMinor,
                            "BPM_Trigin_thread: Slow acquisition: %.3f ms\n",
                            elapsed);
        }
    }

    return NULL;
}
```

### 案例3: 错误统计

```c
// 全局错误计数器
static unsigned long g_error_count = 0;
static unsigned long g_total_reads = 0;

float ReadData(int offset, int channel, int type) {
    g_total_reads++;

    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        g_error_count++;
        errlogPrintf("ERROR #%lu: ReadData: Invalid channel %d (total reads: %lu)\n",
                     g_error_count, channel, g_total_reads);

        // 每100个错误报告一次统计
        if (g_error_count % 100 == 0) {
            double error_rate = 100.0 * g_error_count / g_total_reads;
            errlogPrintf("STATS: Error rate: %.2f%% (%lu/%lu)\n",
                         error_rate, g_error_count, g_total_reads);
        }

        return 0.0;
    }

    // 正常逻辑...
}
```

## 6️⃣ 日志分析工具

### 使用grep过滤日志

```bash
# 查看所有错误
grep ERROR /var/log/BPMmonitor.log

# 查看特定函数的日志
grep "ReadData:" /var/log/BPMmonitor.log

# 查看时间范围
grep "2025-11-09 14:" /var/log/BPMmonitor.log

# 统计错误次数
grep -c ERROR /var/log/BPMmonitor.log

# 查看错误上下文（前后5行）
grep -A 5 -B 5 ERROR /var/log/BPMmonitor.log
```

### 日志分析脚本

创建 `analyze_log.sh`:

```bash
#!/bin/bash

LOG_FILE="/var/log/BPMmonitor.log"

echo "=== BPMmonitor Log Analysis ==="
echo ""

echo "Total lines: $(wc -l < $LOG_FILE)"
echo "Error count: $(grep -c ERROR $LOG_FILE)"
echo "Warning count: $(grep -c WARNING $LOG_FILE)"
echo ""

echo "=== Top 10 Error Messages ==="
grep ERROR $LOG_FILE | sort | uniq -c | sort -rn | head -10
echo ""

echo "=== Recent Errors (last 5) ==="
grep ERROR $LOG_FILE | tail -5
```

### Python日志分析

创建 `analyze_log.py`:

```python
#!/usr/bin/env python3
import re
from collections import Counter

def analyze_log(filename):
    errors = []
    performance = []

    with open(filename, 'r') as f:
        for line in f:
            # 提取错误
            if 'ERROR' in line:
                errors.append(line.strip())

            # 提取性能数据
            match = re.search(r'Acquisition took ([\d.]+) ms', line)
            if match:
                performance.append(float(match.group(1)))

    # 分析错误
    print(f"Total errors: {len(errors)}")
    error_types = Counter([e.split(':')[0] for e in errors])
    print("\nTop error types:")
    for error_type, count in error_types.most_common(5):
        print(f"  {error_type}: {count}")

    # 分析性能
    if performance:
        avg = sum(performance) / len(performance)
        max_time = max(performance)
        min_time = min(performance)
        print(f"\nPerformance statistics:")
        print(f"  Average: {avg:.3f} ms")
        print(f"  Min: {min_time:.3f} ms")
        print(f"  Max: {max_time:.3f} ms")

if __name__ == '__main__':
    analyze_log('/var/log/BPMmonitor.log')
```

## 📝 练习任务

### 练习1: 添加日志

在BPMIOC的以下位置添加日志：
1. `InitDevice()` - 记录初始化步骤
2. `ReadData()` - 记录每次读取
3. `init_record_ai()` - 记录Record初始化

### 练习2: 日志级别

修改日志，使用适当的严重性级别：
- Fatal: 驱动库加载失败
- Major: 硬件读取失败
- Minor: 参数超出范围
- Info: 正常操作

### 练习3: 性能日志

添加性能日志，记录：
- 数据采集耗时
- Record处理耗时
- 慢操作警告（>10ms）

### 练习4: 日志分析

编写脚本分析日志文件：
- 统计错误次数
- 找出最频繁的错误
- 计算平均性能

## 🔍 调试技巧

### 临时启用详细日志

```bash
# 在IOC Shell中动态调整日志级别
epics> var devBPMMonitorDebug 1

# 或在st.cmd中
var devBPMMonitorDebug 1
```

在代码中：

```c
// devBPMMonitor.c
int devBPMMonitorDebug = 0;  // 全局变量

static long read_ai(aiRecord *prec) {
    if (devBPMMonitorDebug) {
        errlogPrintf("read_ai: %s, val=%.3f\n", prec->name, prec->val);
    }
    // ...
}
```

## 📚 参考资源

- **EPICS errlog**: https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/node9.html
- **日志最佳实践**: https://www.codeproject.com/Articles/42354/The-Art-of-Logging
- **EPICS IOC日志**: https://epics-controls.org/resources-and-support/documents/appdev/

## 🔗 相关文档

- **[01-gdb-debugging.md](./01-gdb-debugging.md)** - GDB调试
- **[03-performance-tools.md](./03-performance-tools.md)** - 性能分析
- **[Part 3: 07-error-handling.md](../part3-bpmioc-architecture/07-error-handling.md)** - 错误处理

---

**下一步**: 学习 [性能分析工具](./03-performance-tools.md)，优化IOC性能！
