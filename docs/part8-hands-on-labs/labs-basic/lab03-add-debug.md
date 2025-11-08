# Lab 03: 添加调试输出

> **目标**: 掌握BPMIOC系统的调试技巧
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 60分钟
> **前置知识**: Lab 01, Lab 02, Part 2: 05-scanning-basics.md

## 📋 实验目标

完成本实验后，你将能够：
- ✅ 在驱动层添加详细的调试输出
- ✅ 在设备支持层添加条件调试
- ✅ 使用EPICS内置的调试功能
- ✅ 实现可控的调试级别
- ✅ 分析和解决实际问题

## 🎯 背景知识

### 为什么需要调试

在开发和维护BPMIOC时，你会遇到：
- 数据流不清楚
- 性能问题
- 硬件通信失败
- 意外的数值
- 时序问题

**好的调试输出能帮助你**:
- 快速定位问题
- 理解数据流
- 验证修改
- 性能分析

### 调试输出的层次

```
第1层: 关键错误和警告
    ↓
第2层: 初始化和重要事件
    ↓
第3层: 数据流跟踪
    ↓
第4层: 详细的内部状态
    ↓
第5层: 每次调用的细节（性能影响大）
```

## 🔬 实验一: 实现调试级别系统

### 任务: 添加全局调试级别控制

#### 步骤1: 在driverWrapper.c中添加调试级别

```bash
vim BPMmonitorApp/src/driverWrapper.c
```

在文件开头添加：

```c
#include <stdio.h>
#include <time.h>

// 调试级别定义
static int debug_level = 1;  // 默认级别1

// 调试级别说明：
// 0 = 无调试输出
// 1 = 错误和警告
// 2 = 初始化和重要事件
// 3 = 数据流跟踪
// 4 = 详细状态
// 5 = 完整跟踪（慎用，影响性能）

// 调试宏
#define DEBUG_ERROR(fmt, ...)   if (debug_level >= 1) \
    printf("[ERROR] %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define DEBUG_WARN(fmt, ...)    if (debug_level >= 1) \
    printf("[WARN]  %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define DEBUG_INFO(fmt, ...)    if (debug_level >= 2) \
    printf("[INFO]  %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

#define DEBUG_TRACE(fmt, ...)   if (debug_level >= 3) \
    printf("[TRACE] %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

#define DEBUG_DETAIL(fmt, ...)  if (debug_level >= 4) \
    printf("[DETAIL] %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

#define DEBUG_FULL(fmt, ...)    if (debug_level >= 5) \
    printf("[FULL] %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

// 设置调试级别的函数
int SetDebugLevel(int level)
{
    if (level < 0 || level > 5) {
        DEBUG_ERROR("Invalid debug level %d (must be 0-5)", level);
        return -1;
    }

    int old_level = debug_level;
    debug_level = level;

    printf("=== Debug level changed: %d -> %d ===\n", old_level, level);

    const char *level_names[] = {
        "OFF", "ERROR/WARN", "INFO", "TRACE", "DETAIL", "FULL"
    };
    printf("    New level: %s\n", level_names[level]);

    return 0;
}
```

#### 步骤2: 在InitDevice()中添加调试

```c
int InitDevice()
{
    DEBUG_INFO("=== BPM Monitor Driver Initialization ===");

    handle = dlopen("/usr/lib/liblowlevel.so", RTLD_LAZY);

    if (!handle) {
        DEBUG_WARN("Cannot load liblowlevel.so: %s", dlerror());
        DEBUG_WARN("Using SIMULATION mode");
        use_simulation = 1;

        DEBUG_INFO("Initializing I/O interrupt scan structure");
        scanIoInit(&ioScanPvt);
        scanIoInit(&ioScanPvt_trip);
        scanIoInit(&ioScanPvt_adcraw);

        DEBUG_INFO("Creating BPM monitor thread");
        epicsThreadCreate("BPMMonitor", 50, 20000,
                          (EPICSTHREADFUNC)my_thread, NULL);

        DEBUG_INFO("Driver initialization complete (SIMULATION mode)");
        return 0;
    }

    // 真实硬件模式
    DEBUG_INFO("Loading hardware functions");

    getRfInfoFunc = (RfInfoFunc)dlsym(handle, "getRfInfo");
    if (!getRfInfoFunc) {
        DEBUG_ERROR("Failed to load getRfInfo: %s", dlerror());
        return -1;
    }
    DEBUG_INFO("  - getRfInfo: loaded");

    // ... 其他函数加载 ...

    DEBUG_INFO("Initializing hardware");
    (*InitFunc)();

    DEBUG_INFO("Driver initialization complete (HARDWARE mode)");
    return 0;
}
```

#### 步骤3: 在my_thread()中添加定期统计

```c
static void my_thread(void *arg)
{
    static double sim_time = 0.0;
    static int loop_count = 0;
    static time_t last_stat_time = 0;
    static int scan_count_since_stat = 0;

    DEBUG_INFO("BPM Monitor thread started");

    while (1) {
        // 更新数据
        if (use_simulation) {
            DEBUG_FULL("Generating simulated data");
            for (int i = 0; i < 8; i++) {
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);
            }
            DEBUG_DETAIL("Sim data: Amp[0]=%.3f, Phase[0]=%.1f",
                        Amp[0], Phase[0]);
            sim_time += 0.1;
        } else {
            DEBUG_FULL("Reading hardware data");
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }
        }

        // 触发I/O中断扫描
        DEBUG_FULL("Triggering I/O interrupt scan");
        scanIoRequest(ioScanPvt);
        scan_count_since_stat++;

        // 每10秒输出一次统计
        time_t now = time(NULL);
        if (debug_level >= 2 && (now - last_stat_time) >= 10) {
            double scan_rate = scan_count_since_stat / 10.0;
            DEBUG_INFO("=== Statistics (last 10s) ===");
            DEBUG_INFO("  Scan count: %d", scan_count_since_stat);
            DEBUG_INFO("  Scan rate:  %.1f Hz", scan_rate);
            DEBUG_INFO("  Amp[0]:     %.3f V", Amp[0]);
            DEBUG_INFO("  Phase[0]:   %.1f deg", Phase[0]);

            last_stat_time = now;
            scan_count_since_stat = 0;
        }

        epicsThreadSleep(0.1);
        loop_count++;
    }
}
```

#### 步骤4: 在ReadData()中添加调试

```c
float ReadData(int offset, int channel, int type)
{
    float value = 0.0;
    static int call_count = 0;
    call_count++;

    DEBUG_FULL("ReadData called: type=%s, offset=%d, ch=%d",
               type, offset, channel);

    if (strcmp(type, "AMP") == 0) {
        DEBUG_TRACE("Reading AMP: channel=%d", channel);
        if (channel >= 0 && channel < 10) {
            value = Amp[channel];
            DEBUG_DETAIL("  Amp[%d] = %.6f", channel, value);
        } else {
            DEBUG_ERROR("Invalid AMP channel: %d", channel);
        }
    }
    else if (strcmp(type, "PHASE") == 0) {
        DEBUG_TRACE("Reading PHASE: channel=%d", channel);
        if (channel >= 0 && channel < 10) {
            value = Phase[channel];
            DEBUG_DETAIL("  Phase[%d] = %.2f", channel, value);
        } else {
            DEBUG_ERROR("Invalid PHASE channel: %d", channel);
        }
    }
    else if (strcmp(type, "REG") == 0) {
        DEBUG_TRACE("Reading REG: offset=%d, ch=%d", offset, channel);

        switch(offset) {
            case 29:  // 温度
                value = 25.0 + (rand() % 100) / 100.0;  // 模拟温度
                DEBUG_DETAIL("  Temperature[%d] = %.1f C", channel, value);
                break;

            // ... 其他offset ...

            default:
                DEBUG_WARN("Unknown REG offset: %d", offset);
                break;
        }
    }
    else {
        DEBUG_ERROR("Unknown type: %s", type);
    }

    // 每1000次调用输出一次统计
    if (debug_level >= 3 && (call_count % 1000 == 0)) {
        DEBUG_INFO("ReadData called %d times", call_count);
    }

    return value;
}
```

## 🔬 实验二: 在设备支持层添加调试

### 步骤1: 在devBPMMonitor.c中添加调试宏

```bash
vim BPMmonitorApp/src/devBPMMonitor.c
```

添加：

```c
// 设备支持层调试（默认关闭，避免过多输出）
static int dev_debug = 0;

#define DEV_DEBUG(level, fmt, ...) \
    if (dev_debug >= level) \
        printf("[DEV] %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

// 设置设备支持层调试级别
int SetDevDebugLevel(int level)
{
    dev_debug = level;
    printf("Device support debug level set to %d\n", level);
    return 0;
}
```

### 步骤2: 在init_ai_record()中添加调试

```c
static long init_ai_record(aiRecord *prec)
{
    DevPvt *pPvt = malloc(sizeof(DevPvt));

    DEV_DEBUG(2, "Initializing ai record: %s", prec->name);

    if (!pPvt) {
        DEV_DEBUG(1, "ERROR: malloc failed for %s", prec->name);
        return S_dev_noMemory;
    }

    // 解析INP字段
    char *pchar = prec->inp.value.instio.string;
    DEV_DEBUG(3, "  Parsing INP: %s", pchar);

    // 跳过@
    pchar++;

    // 解析类型
    char *type_end = strchr(pchar, ':');
    if (type_end) {
        int type_len = type_end - pchar;
        strncpy(pPvt->type_str, pchar, type_len);
        pPvt->type_str[type_len] = '\0';
        DEV_DEBUG(3, "  Type: %s", pPvt->type_str);
    }

    // 解析offset
    pchar = type_end + 1;
    pPvt->offset = strtol(pchar, &pchar, 0);
    DEV_DEBUG(3, "  Offset: %d", pPvt->offset);

    // 解析channel
    if (strstr(pchar, "ch=")) {
        pchar = strstr(pchar, "ch=") + 3;
        pPvt->channel = strtol(pchar, &pchar, 0);
        DEV_DEBUG(3, "  Channel: %d", pPvt->channel);
    }

    prec->dpvt = pPvt;

    DEV_DEBUG(2, "  Initialization complete: type=%s, offset=%d, ch=%d",
              pPvt->type_str, pPvt->offset, pPvt->channel);

    return 0;
}
```

### 步骤3: 在read_ai()中添加选择性调试

```c
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    static int read_count = 0;
    read_count++;

    // 只为特定PV或定期输出
    int should_debug = 0;

    // 总是调试RF3Amp（用于验证）
    if (strcmp(prec->name, "iLinac_007:BPM14And15:RFIn_01_Amp") == 0) {
        should_debug = (read_count % 10 == 0);  // 每10次
    }

    // 其他PV根据调试级别
    if (dev_debug >= 4) {
        should_debug = 1;
    }

    if (should_debug) {
        DEV_DEBUG(3, "Reading %s", prec->name);
    }

    float value = ReadData(pPvt->offset, pPvt->channel, pPvt->type_str);

    prec->val = value;
    prec->udf = 0;

    if (should_debug) {
        DEV_DEBUG(3, "  Value: %.6f %s", value, prec->egu);
    }

    return 0;
}
```

## 🔬 实验三: 添加PV控制调试级别

### 步骤1: 导出调试函数

在 `driverWrapper.h` 中添加：

```c
int SetDebugLevel(int level);
int SetDevDebugLevel(int level);
```

### 步骤2: 在数据库中添加控制PV

在 `BPMmonitor.db` 中添加：

```
# 驱动层调试级别控制
record(longout, "$(P):DebugLevel")
{
    field(DESC, "Driver Debug Level (0-5)")
    field(DTYP, "BPMmonitor")
    field(OUT,  "@DEBUG:0")
    field(VAL,  "1")
    field(DRVH, "5")
    field(DRVL, "0")
    field(HOPR, "5")
    field(LOPR, "0")
}

# 设备支持层调试级别控制
record(longout, "$(P):DevDebugLevel")
{
    field(DESC, "Device Support Debug Level")
    field(DTYP, "BPMmonitor")
    field(OUT,  "@DEVDEBUG:0")
    field(VAL,  "0")
    field(DRVH, "5")
    field(DRVL, "0")
}

# 调试帮助信息
record(stringin, "$(P):DebugHelp")
{
    field(DESC, "Debug Level Help")
    field(VAL,  "0=OFF 1=ERR 2=INFO 3=TRACE 4=DETAIL 5=FULL")
}
```

### 步骤3: 在设备支持层处理调试PV

在 `devBPMMonitor.c` 的 `write_longout()` 中添加：

```c
static long write_longout(longoutRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    if (strcmp(pPvt->type_str, "DEBUG") == 0) {
        SetDebugLevel(prec->val);
        return 0;
    }

    if (strcmp(pPvt->type_str, "DEVDEBUG") == 0) {
        SetDevDebugLevel(prec->val);
        return 0;
    }

    // ... 其他处理 ...
}
```

## 🧪 实验四: 测试和使用调试系统

### 步骤1: 编译并运行

```bash
cd ~/BPMIOC
make clean
make

cd iocBoot/iocBPMmonitor
./st.cmd
```

### 步骤2: 测试不同调试级别

**级别0（无输出）**:
```bash
caput iLinac_007:BPM14And15:DebugLevel 0
# 应该看不到任何调试输出
```

**级别1（错误和警告）**:
```bash
caput iLinac_007:BPM14And15:DebugLevel 1
# 只看到错误和警告
```

**级别2（信息）**:
```bash
caput iLinac_007:BPM14And15:DebugLevel 2
# 看到初始化信息和10秒统计
```

输出示例：
```
=== Debug level changed: 1 -> 2 ===
    New level: INFO
[INFO]  my_thread: === Statistics (last 10s) ===
[INFO]  my_thread:   Scan count: 100
[INFO]  my_thread:   Scan rate:  10.0 Hz
[INFO]  my_thread:   Amp[0]:     1.234 V
[INFO]  my_thread:   Phase[0]:   45.6 deg
```

**级别3（跟踪）**:
```bash
caput iLinac_007:BPM14And15:DebugLevel 3
# 看到数据流跟踪
```

输出示例：
```
[TRACE] ReadData: Reading AMP: channel=0
[TRACE] ReadData: Reading PHASE: channel=0
```

**级别4（详细）**:
```bash
caput iLinac_007:BPM14And15:DebugLevel 4
# 看到详细的内部状态
```

输出示例：
```
[DETAIL] ReadData:   Amp[0] = 1.234567
[DETAIL] my_thread: Sim data: Amp[0]=1.235, Phase[0]=45.7
```

**级别5（完整，注意性能影响）**:
```bash
caput iLinac_007:BPM14And15:DebugLevel 5
# 看到每次函数调用
```

### 步骤3: 测试设备支持层调试

```bash
# 开启设备支持层调试
caput iLinac_007:BPM14And15:DevDebugLevel 3

# 监控RF3Amp，观察调试输出
camonitor iLinac_007:BPM14And15:RFIn_01_Amp
```

### 步骤4: 查看调试帮助

```bash
caget iLinac_007:BPM14And15:DebugHelp
# iLinac_007:BPM14And15:DebugHelp    0=OFF 1=ERR 2=INFO 3=TRACE 4=DETAIL 5=FULL
```

## 🔬 实验五: 调试实际问题

### 场景1: 数据不更新

**问题**: RF3Amp的值从不变化

**调试步骤**:
```bash
# 1. 开启INFO级别
caput iLinac_007:BPM14And15:DebugLevel 2

# 2. 查看是否有扫描
# 应该看到每10秒的统计，确认scan_rate正常

# 3. 开启TRACE级别
caput iLinac_007:BPM14And15:DebugLevel 3

# 4. 监控PV
camonitor iLinac_007:BPM14And15:RFIn_01_Amp

# 5. 查看是否有ReadData调用
# 应该看到 [TRACE] ReadData: Reading AMP: channel=0
```

### 场景2: 性能问题

**问题**: IOC CPU占用过高

**调试步骤**:
```bash
# 1. 开启INFO级别，查看扫描频率
caput iLinac_007:BPM14And15:DebugLevel 2

# 2. 观察统计输出
# 如果scan_rate > 预期（如 >20 Hz），可能扫描太频繁

# 3. 检查是否有意外的扫描循环
caput iLinac_007:BPM14And15:DebugLevel 4

# 4. 监控特定PV，计算更新频率
```

### 场景3: 数值异常

**问题**: Amp值偶尔出现NaN或无穷大

**调试步骤**:
```bash
# 1. 添加特殊检查代码
```

在 `ReadData()` 中添加：
```c
if (strcmp(type, "AMP") == 0) {
    value = Amp[channel];

    // 检查异常值
    if (isnan(value)) {
        DEBUG_ERROR("NaN detected in Amp[%d]!", channel);
    }
    if (isinf(value)) {
        DEBUG_ERROR("Infinity detected in Amp[%d]!", channel);
    }
    if (value < 0 || value > 10) {
        DEBUG_WARN("Out-of-range Amp[%d] = %.3f", channel, value);
    }
}
```

## 📊 性能影响分析

### 不同调试级别的CPU影响

| 调试级别 | CPU增加 | 适用场景 |
|---------|--------|---------|
| 0 (OFF) | 0% | 生产环境 |
| 1 (ERR) | <0.1% | 生产环境（推荐） |
| 2 (INFO) | ~0.5% | 日常开发 |
| 3 (TRACE) | ~2% | 调试数据流 |
| 4 (DETAIL) | ~5% | 深度调试 |
| 5 (FULL) | ~10-20% | 短期诊断 |

**建议**:
- 生产环境: 级别1
- 开发环境: 级别2
- 调试问题: 临时提升到3-5，解决后降回2

## 🎯 调试最佳实践

### 1. 分层调试

```
驱动层调试(DebugLevel) + 设备支持层调试(DevDebugLevel)
可以独立控制，定位问题更快
```

### 2. 条件输出

```c
// 不好：总是输出
printf("Value: %.3f\n", value);

// 好：条件输出
DEBUG_TRACE("Value: %.3f", value);

// 更好：只为关键PV输出
if (channel == 0 && read_count % 10 == 0) {
    DEBUG_INFO("Amp[0] = %.3f", value);
}
```

### 3. 有意义的消息

```c
// 不好
DEBUG_INFO("x=%d", x);

// 好
DEBUG_INFO("Processing channel %d, offset=%d", channel, offset);

// 更好
DEBUG_INFO("Reading RF3 Amp: ch=%d, offset=%d, expected=1-2V",
           channel, offset);
```

### 4. 包含上下文

```c
DEBUG_ERROR("Invalid channel: %d (valid range: 0-7, offset=%d, type=%s)",
            channel, offset, type);
```

## 📝 实验报告模板

```markdown
# Lab 03 实验报告

## 实验一：调试级别系统
- 实现的调试级别数：5
- 默认级别：1（ERROR/WARN）
- PV控制：已实现 (DebugLevel PV)

## 实验二：设备支持层调试
- 添加的调试点数：3（init, read, write）
- 选择性调试：已实现（RF3Amp每10次输出）

## 实验三：实际调试案例
### 案例1: [描述问题]
- 调试级别使用：3
- 发现的原因：...
- 解决方法：...

### 案例2: [描述问题]
- 调试级别使用：4
- 发现的原因：...
- 解决方法：...

## 性能影响测试
- 级别0 CPU: X%
- 级别2 CPU: X%
- 级别5 CPU: X%

## 收获和体会
...
```

## 🔗 相关文档

- [Part 10: 01-debugging-basics.md](../../part10-debugging-testing/01-debugging-basics.md) - 调试基础
- [Part 10: 02-using-gdb.md](../../part10-debugging-testing/02-using-gdb.md) - 使用GDB
- [Part 10: 05-performance-profiling.md](../../part10-debugging-testing/05-performance-profiling.md) - 性能分析

---

**🎉 恭喜完成实验！** 你已经掌握了BPMIOC的调试技巧，这对开发和维护至关重要！
