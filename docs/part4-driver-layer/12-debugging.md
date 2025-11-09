# 驱动层调试技巧

> **阅读时间**: 30分钟
> **难度**: ⭐⭐⭐☆☆
> **前置知识**: gdb基础、printf调试

## 📋 本文目标

- 掌握驱动层常用调试方法
- 学会使用gdb调试IOC
- 理解常见问题的定位技巧

## 1. printf调试

### 1.1 基本调试输出

```c
// 在关键位置添加printf
long InitDevice()
{
    printf("[DEBUG] InitDevice: Starting...\n");

    scanIoInit(&TriginScanPvt);
    printf("[DEBUG] InitDevice: IOSCANPVT initialized\n");

    handle = dlopen(dll_filename, RTLD_LAZY);
    if (handle == NULL) {
        printf("[ERROR] dlopen failed: %s\n", dlerror());
        return -1;
    }
    printf("[DEBUG] InitDevice: Library loaded: %s\n", dll_filename);

    // ... 其他步骤 ...

    printf("[DEBUG] InitDevice: Complete\n");
    return 0;
}
```

### 1.2 条件调试输出

```c
// 添加调试级别控制
static int debug_level = 0;  // 通过Reg[99]控制

#define DEBUG_PRINT(level, fmt, ...) \
    do { \
        if (debug_level >= level) { \
            printf("[DEBUG-%d %s:%d] " fmt "\n", \
                   level, __FILE__, __LINE__, ##__VA_ARGS__); \
        } \
    } while(0)

// 使用
void *pthread(void *arg)
{
    while (1) {
        DEBUG_PRINT(2, "Triggering data acquisition");
        funcTriggerAllDataReached();

        DEBUG_PRINT(2, "Requesting I/O scan");
        scanIoRequest(TriginScanPvt);

        usleep(100000);
    }
}

// 在iocsh中控制
epics> caput LLRF:BPM:DebugLevel 2
```

### 1.3 数据追踪

```c
float ReadData(int offset, int channel, int type)
{
    float value;

    switch (offset) {
        case 0:  // RF信息
            value = funcGetRFInfo(channel, type);
            DEBUG_PRINT(3, "ReadData(offset=%d, ch=%d, type=%d) = %.3f",
                       offset, channel, type, value);
            break;
        // ...
    }

    return value;
}
```

## 2. gdb调试

### 2.1 启动gdb

```bash
# 编译时添加调试符号
cd ~/BPMIOC
make clean
make USR_CFLAGS="-g -O0"

# 启动gdb
cd iocBoot/iocBPMmonitor
gdb ./st.cmd

# 或attach到运行中的IOC
ps aux | grep st.cmd
gdb -p <PID>
```

### 2.2 常用断点

```gdb
# 启动gdb
(gdb) break InitDevice
(gdb) break ReadData
(gdb) break pthread

# 条件断点
(gdb) break ReadData if offset==0 && channel==3

# 运行
(gdb) run

# 当断点触发
(gdb) print offset
(gdb) print channel
(gdb) print funcGetRFInfo
```

### 2.3 查看数据

```gdb
# 查看全局缓冲区
(gdb) print rf3amp[0]@10
$1 = {105.234, 106.123, 104.567, ...}

# 查看寄存器
(gdb) print Reg[0]@10
$2 = {1, 100, 0, 0, 0, ...}

# 查看函数指针
(gdb) print funcGetRFInfo
$3 = (float (*)(int, int)) 0x7ffff7a12340

# 调用函数
(gdb) call funcGetRFInfo(3, 0)
$4 = 105.234
```

### 2.4 追踪调用栈

```gdb
# 查看调用栈
(gdb) backtrace
#0  ReadData (offset=0, channel=3, type=0) at driverWrapper.c:650
#1  0x00007ffff7b45678 in read_ai (precord=0x...) at devBPMMonitor.c:123
#2  0x00007ffff7c56789 in process (precord=0x...) at recAi.c:456
#3  0x00007ffff7d67890 in dbScanOnce (precord=0x...) at dbScan.c:789

# 切换栈帧
(gdb) frame 1
(gdb) print precord->name
$5 = "LLRF:BPM:RF3Amp"
```

### 2.5 watchpoint监视变量

```gdb
# 监视Reg[0]的变化
(gdb) watch Reg[0]
Hardware watchpoint 1: Reg[0]

(gdb) continue
Hardware watchpoint 1: Reg[0]

Old value = 1
New value = 0
SetReg (addr=0, value=0) at driverWrapper.c:1120

# 查看是谁修改的
(gdb) backtrace
```

## 3. 调试工具函数

### 3.1 调试寄存器

```c
// 添加调试寄存器 Reg[99]
// bit 0: 打印ReadData调用
// bit 1: 打印ReadWaveform调用
// bit 2: 打印SetReg调用
// bit 3: 打印pthread循环

float ReadData(int offset, int channel, int type)
{
    if (Reg[99] & 0x1) {
        printf("[ReadData] offset=%d, ch=%d, type=%d\n",
               offset, channel, type);
    }

    // ... 正常处理 ...
}

// 使用
epics> caput LLRF:BPM:DebugReg 1   # 打开ReadData调试
epics> caput LLRF:BPM:DebugReg 15  # 打开所有调试
epics> caput LLRF:BPM:DebugReg 0   # 关闭调试
```

### 3.2 性能计时

```c
#include <sys/time.h>

#define TIME_START(name) \
    struct timeval tv_start_##name, tv_end_##name; \
    gettimeofday(&tv_start_##name, NULL)

#define TIME_END(name) \
    do { \
        gettimeofday(&tv_end_##name, NULL); \
        long us = (tv_end_##name.tv_sec - tv_start_##name.tv_sec) * 1000000 + \
                  (tv_end_##name.tv_usec - tv_start_##name.tv_usec); \
        printf("[PERF] %s took %ld us (%.3f ms)\n", #name, us, us/1000.0); \
    } while(0)

// 使用
void *pthread(void *arg)
{
    while (1) {
        TIME_START(acquisition);
        funcTriggerAllDataReached();
        TIME_END(acquisition);

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

### 3.3 内存使用监控

```c
#include <malloc.h>

void printMemoryUsage(void)
{
    struct mallinfo mi = mallinfo();

    printf("=== Memory Usage ===\n");
    printf("  Total allocated: %d KB\n", mi.uordblks / 1024);
    printf("  Total free:      %d KB\n", mi.fordblks / 1024);
    printf("  Global buffers:  ~2400 KB\n");
    printf("====================\n");
}
```

## 4. 日志系统

### 4.1 简单日志

```c
static FILE *log_file = NULL;

void initLogging(const char *filename)
{
    log_file = fopen(filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "ERROR: Cannot open log file: %s\n", filename);
        return;
    }

    fprintf(log_file, "\n=== IOC Started: %s ===\n", getCurrentTime());
    fflush(log_file);
}

void logMessage(const char *level, const char *fmt, ...)
{
    if (log_file == NULL) return;

    va_list args;
    fprintf(log_file, "[%s] [%s] ", getCurrentTime(), level);

    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
}

// 使用
logMessage("INFO", "System initialized");
logMessage("ERROR", "Data acquisition timeout");
logMessage("DEBUG", "ReadData(offset=%d, ch=%d) = %.3f", 0, 3, value);
```

### 4.2 滚动日志

```c
#define MAX_LOG_SIZE (10 * 1024 * 1024)  // 10MB

void checkLogSize(void)
{
    if (log_file == NULL) return;

    long size = ftell(log_file);
    if (size > MAX_LOG_SIZE) {
        fclose(log_file);

        // 重命名旧日志
        rename("bpm.log", "bpm.log.old");

        // 创建新日志
        log_file = fopen("bpm.log", "w");
        logMessage("INFO", "Log file rotated");
    }
}
```

## 5. 常见问题定位

### 5.1 IOC启动失败

```bash
# 检查动态库是否存在
ls -l libbpm_mock.so

# 检查符号
nm -D libbpm_mock.so | grep SystemInit

# 查看依赖
ldd libbpm_mock.so

# 查看错误信息
./st.cmd 2>&1 | grep -i error
```

### 5.2 数据全是0

```c
// 添加健康检查
void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();

        // 检查数据
        if (checkBufferHealth(rf3amp, buf_len) == 0) {
            printf("WARNING: rf3amp buffer unhealthy\n");
            analyzeBuffer("rf3amp", rf3amp, buf_len);
        }

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

### 5.3 PV无法访问

```bash
# 检查IOC是否运行
ps aux | grep st.cmd

# 检查PV是否存在
epics> dbl | grep RF3Amp

# 检查PV详细信息
epics> dbpr LLRF:BPM:RF3Amp 3

# 从外部访问
caget LLRF:BPM:RF3Amp
```

### 5.4 数据更新慢

```c
// 添加统计
static unsigned long scan_count = 0;
static time_t last_report = 0;

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        scan_count++;

        // 每10秒报告一次
        time_t now = time(NULL);
        if (now - last_report >= 10) {
            printf("Scan rate: %.1f Hz\n", scan_count / 10.0);
            scan_count = 0;
            last_report = now;
        }

        usleep(100000);
    }
}
```

## 6. Valgrind内存检查

```bash
# 编译
make clean
make USR_CFLAGS="-g -O0"

# 运行valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind.log \
         ./st.cmd

# 查看报告
less valgrind.log
```

## 7. strace系统调用追踪

```bash
# 追踪系统调用
strace -o strace.log ./st.cmd

# 只追踪文件操作
strace -e trace=open,read,write,close ./st.cmd

# attach到运行中的进程
strace -p <PID>
```

## 8. 远程调试

### 8.1 gdbserver (ZYNQ)

```bash
# 在ZYNQ上启动gdbserver
gdbserver :2345 ./st.cmd

# 在PC上连接
arm-linux-gnueabihf-gdb ./st.cmd
(gdb) target remote 192.168.1.100:2345
(gdb) break InitDevice
(gdb) continue
```

## ❓ 常见问题

### Q1: 如何调试dlopen失败？
**A**:
```c
handle = dlopen("libbpm_mock.so", RTLD_LAZY);
if (handle == NULL) {
    printf("ERROR: %s\n", dlerror());

    // 尝试绝对路径
    handle = dlopen("/full/path/to/libbpm_mock.so", RTLD_LAZY);

    // 检查LD_LIBRARY_PATH
    printf("LD_LIBRARY_PATH=%s\n", getenv("LD_LIBRARY_PATH"));
}
```

### Q2: 如何调试I/O Interrupt不触发？
**A**:
```c
void *pthread(void *arg)
{
    while (1) {
        printf("Before scanIoRequest\n");
        scanIoRequest(TriginScanPvt);
        printf("After scanIoRequest\n");

        usleep(100000);
    }
}

// 检查Record是否注册
(gdb) print TriginScanPvt
(gdb) print TriginScanPvt->recordList
```

## 📚 延伸阅读

- [13-troubleshooting.md](./13-troubleshooting.md) - 问题排查
- gdb手册: `man gdb`
- Valgrind文档: https://valgrind.org/docs/

## 🎓 本章总结

- ✅ 使用printf进行基本调试
- ✅ 使用gdb进行深度调试
- ✅ 添加调试寄存器控制输出
- ✅ 使用日志系统记录运行信息
- ✅ 使用Valgrind检查内存问题

---

**实验任务**: 使用gdb追踪一次完整的数据采集流程
