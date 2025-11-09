# Part 19.6: 高级模拟技巧

> **目标**: 学习高级模拟器功能 - 故障注入、场景回放、性能优化
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 1.5小时
> **前置**: 已完成05-complete-mock-implementation.md

## 📖 内容概览

本文档介绍Mock库的高级功能：
- 故障注入（Fault Injection）
- 场景回放（Scenario Replay）
- 性能优化（Performance Optimization）
- 调试辅助功能

这些功能让你的Mock库更强大、更实用。

---

## 1. 故障注入（Fault Injection）

### 1.1 为什么需要故障注入？

在实际系统中，硬件会出现各种故障：
- ❌ 通信超时
- ❌ 数据损坏
- ❌ 信号丢失
- ❌ 硬件故障

**你的IOC必须能正确处理这些故障！**

故障注入让你在PC上测试这些情况。

---

### 1.2 设计故障注入系统

#### 核心思想

```c
// 故障类型
typedef enum {
    FAULT_NONE = 0,           // 无故障
    FAULT_TIMEOUT,            // 通信超时
    FAULT_DATA_CORRUPTION,    // 数据损坏
    FAULT_SIGNAL_LOSS,        // 信号丢失
    FAULT_OUT_OF_RANGE,       // 数值超范围
    FAULT_INTERMITTENT        // 间歇性故障
} FaultType;

// 故障配置
typedef struct {
    FaultType type;
    double probability;       // 故障概率 (0.0 - 1.0)
    int trigger_count;        // 调用多少次后触发
    int duration;             // 持续多少次
    int current_count;        // 当前计数器
    int active;               // 是否激活
} FaultConfig;

// 全局故障配置
static FaultConfig g_faults[10] = {0};
```

---

### 1.3 实现故障注入

#### 添加故障控制函数

```c
// 启用故障注入
int EnableFaultInjection(int fault_id, int fault_type,
                        double probability, int duration) {
    if (fault_id < 0 || fault_id >= 10) {
        return -1;
    }

    g_faults[fault_id].type = fault_type;
    g_faults[fault_id].probability = probability;
    g_faults[fault_id].duration = duration;
    g_faults[fault_id].current_count = 0;
    g_faults[fault_id].active = 1;

    printf("Fault injection enabled: ID=%d, Type=%d, Prob=%.2f\n",
           fault_id, fault_type, probability);

    return 0;
}

// 禁用故障注入
int DisableFaultInjection(int fault_id) {
    if (fault_id < 0 || fault_id >= 10) {
        return -1;
    }

    g_faults[fault_id].active = 0;
    printf("Fault injection disabled: ID=%d\n", fault_id);

    return 0;
}
```

#### 检查是否应触发故障

```c
// 检查故障
static int checkFault(int fault_id) {
    if (fault_id < 0 || fault_id >= 10) {
        return 0;
    }

    FaultConfig *fault = &g_faults[fault_id];

    if (!fault->active) {
        return 0;
    }

    // 增加计数器
    fault->current_count++;

    // 检查是否应该结束故障
    if (fault->duration > 0 &&
        fault->current_count >= fault->duration) {
        fault->active = 0;
        printf("Fault ID=%d ended after %d calls\n",
               fault_id, fault->current_count);
        return 0;
    }

    // 基于概率决定是否触发
    double rand_val = (double)rand() / RAND_MAX;

    if (rand_val < fault->probability) {
        return 1; // 触发故障
    }

    return 0; // 不触发
}
```

---

### 1.4 在数据生成中应用故障

#### 修改GetRFInfo函数

```c
float GetRFInfo(int channel, int type) {
    // 检查RF通信超时故障
    if (checkFault(0)) {
        printf("FAULT: RF communication timeout!\n");
        return -999.0f; // 错误值
    }

    int ch_idx = channel - 3;
    if (ch_idx < 0 || ch_idx >= 4) {
        return 0.0f;
    }

    double time = g_config.simulation_time;
    RfChannelConfig *cfg = &g_rf_channels[ch_idx];

    // 检查数据损坏故障
    if (checkFault(1)) {
        printf("FAULT: RF data corruption!\n");
        return 1e10f; // 异常大的值
    }

    float value;
    switch (type) {
        case 0: // Amplitude
            value = generateRfAmplitude(cfg, time);

            // 检查信号丢失故障
            if (checkFault(2)) {
                printf("FAULT: RF signal loss!\n");
                return 0.0f;
            }

            break;

        case 1: // Phase
            value = generateRfPhase(cfg, time);
            break;

        // ... 其他类型

        default:
            return 0.0f;
    }

    return value;
}
```

---

### 1.5 故障注入使用示例

```c
// test_fault_injection.c
#include <stdio.h>
#include "libbpm_mock.h"

int main() {
    // 初始化Mock库
    SystemInit();

    printf("=== Normal Operation ===\n");
    for (int i = 0; i < 5; i++) {
        float amp = GetRFInfo(3, 0);
        printf("RF3 Amplitude: %.2f\n", amp);
    }

    // 启用故障：30%概率超时，持续10次
    printf("\n=== Enable Timeout Fault ===\n");
    EnableFaultInjection(0, FAULT_TIMEOUT, 0.3, 10);

    for (int i = 0; i < 15; i++) {
        float amp = GetRFInfo(3, 0);
        printf("RF3 Amplitude: %.2f\n", amp);
    }

    // 启用数据损坏：20%概率
    printf("\n=== Enable Data Corruption ===\n");
    EnableFaultInjection(1, FAULT_DATA_CORRUPTION, 0.2, 5);

    for (int i = 0; i < 10; i++) {
        float amp = GetRFInfo(3, 0);
        printf("RF3 Amplitude: %.2f\n", amp);
    }

    SystemClose();
    return 0;
}
```

**输出示例**:
```
=== Normal Operation ===
RF3 Amplitude: 1.02
RF3 Amplitude: 1.01
RF3 Amplitude: 1.03

=== Enable Timeout Fault ===
Fault injection enabled: ID=0, Type=1, Prob=0.30
RF3 Amplitude: 1.02
FAULT: RF communication timeout!
RF3 Amplitude: -999.00
RF3 Amplitude: 1.01
FAULT: RF communication timeout!
RF3 Amplitude: -999.00
```

---

## 2. 场景回放（Scenario Replay）

### 2.1 为什么需要场景回放？

有时你需要：
- ✅ 重现特定的硬件行为
- ✅ 回放真实采集的数据
- ✅ 测试特定的边界情况

场景回放让你从文件加载预定义的数据序列。

---

### 2.2 设计场景系统

#### 数据格式

```ini
# scenario_rf_ramp.txt
# 时间(s), RF3_Amp, RF3_Phase, RF4_Amp, RF4_Phase, ...
0.0, 1.00, 0.0, 1.00, 0.0, 1.00, 0.0, 1.00, 0.0
0.1, 1.02, 0.1, 1.02, 0.1, 1.02, 0.1, 1.02, 0.1
0.2, 1.05, 0.2, 1.05, 0.2, 1.05, 0.2, 1.05, 0.2
0.3, 1.10, 0.3, 1.10, 0.3, 1.10, 0.3, 1.10, 0.3
# ... 更多数据
```

#### 数据结构

```c
// 场景数据点
typedef struct {
    double time;
    float rf_amp[4];      // RF3-RF6 幅度
    float rf_phase[4];    // RF3-RF6 相位
    float xy_pos[8];      // XY1-XY4 位置
} ScenarioDataPoint;

// 场景配置
typedef struct {
    char filename[256];
    int num_points;
    int current_index;
    ScenarioDataPoint *data;
    int loop;             // 是否循环播放
    int active;
} ScenarioConfig;

static ScenarioConfig g_scenario = {0};
```

---

### 2.3 实现场景加载

```c
// 加载场景文件
int LoadScenario(const char *filename, int loop) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("ERROR: Cannot open scenario file: %s\n", filename);
        return -1;
    }

    // 计算数据点数量
    int count = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue; // 跳过注释和空行
        }
        count++;
    }

    if (count == 0) {
        printf("ERROR: No data in scenario file\n");
        fclose(fp);
        return -1;
    }

    // 分配内存
    g_scenario.data = malloc(count * sizeof(ScenarioDataPoint));
    if (!g_scenario.data) {
        printf("ERROR: Cannot allocate memory for scenario\n");
        fclose(fp);
        return -1;
    }

    // 读取数据
    rewind(fp);
    int idx = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        ScenarioDataPoint *pt = &g_scenario.data[idx];

        sscanf(line, "%lf, %f, %f, %f, %f, %f, %f, %f, %f, "
                     "%f, %f, %f, %f, %f, %f, %f, %f",
               &pt->time,
               &pt->rf_amp[0], &pt->rf_phase[0],
               &pt->rf_amp[1], &pt->rf_phase[1],
               &pt->rf_amp[2], &pt->rf_phase[2],
               &pt->rf_amp[3], &pt->rf_phase[3],
               &pt->xy_pos[0], &pt->xy_pos[1],
               &pt->xy_pos[2], &pt->xy_pos[3],
               &pt->xy_pos[4], &pt->xy_pos[5],
               &pt->xy_pos[6], &pt->xy_pos[7]);

        idx++;
    }

    fclose(fp);

    strncpy(g_scenario.filename, filename, sizeof(g_scenario.filename)-1);
    g_scenario.num_points = count;
    g_scenario.current_index = 0;
    g_scenario.loop = loop;
    g_scenario.active = 1;

    printf("Scenario loaded: %s (%d points)\n", filename, count);

    return 0;
}

// 停止场景回放
int StopScenario(void) {
    if (g_scenario.data) {
        free(g_scenario.data);
        g_scenario.data = NULL;
    }
    g_scenario.active = 0;
    printf("Scenario stopped\n");
    return 0;
}
```

---

### 2.4 在数据生成中使用场景

```c
float GetRFInfo(int channel, int type) {
    // 如果场景激活，从场景获取数据
    if (g_scenario.active) {
        int ch_idx = channel - 3;
        ScenarioDataPoint *pt = &g_scenario.data[g_scenario.current_index];

        float value;
        if (type == 0) {
            value = pt->rf_amp[ch_idx];
        } else if (type == 1) {
            value = pt->rf_phase[ch_idx];
        } else {
            value = 0.0f;
        }

        // 所有通道读取完后，推进到下一个数据点
        static int read_count = 0;
        read_count++;
        if (read_count >= 8) { // 4通道 × 2类型
            read_count = 0;
            g_scenario.current_index++;

            if (g_scenario.current_index >= g_scenario.num_points) {
                if (g_scenario.loop) {
                    g_scenario.current_index = 0;
                    printf("Scenario loop restarted\n");
                } else {
                    g_scenario.active = 0;
                    printf("Scenario ended\n");
                }
            }
        }

        return value;
    }

    // 否则使用正常的生成逻辑
    // ... (原来的代码)
}
```

---

### 2.5 场景回放使用示例

```c
// test_scenario.c
#include <stdio.h>
#include "libbpm_mock.h"

int main() {
    SystemInit();

    // 加载场景（循环播放）
    if (LoadScenario("scenario_rf_ramp.txt", 1) == 0) {
        printf("=== Scenario Replay ===\n");

        for (int i = 0; i < 20; i++) {
            float amp = GetRFInfo(3, 0);
            float phase = GetRFInfo(3, 1);
            printf("Point %2d: RF3 Amp=%.2f, Phase=%.2f\n",
                   i, amp, phase);
        }

        StopScenario();
    }

    SystemClose();
    return 0;
}
```

---

## 3. 性能优化

### 3.1 测量性能

#### 添加性能统计

```c
// 性能统计结构
typedef struct {
    unsigned long call_count;
    double total_time_us;
    double min_time_us;
    double max_time_us;
} PerfStats;

static PerfStats g_perf_stats[100] = {0};

// 获取微秒级时间
static double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

// 记录函数调用
#define PERF_START(id) \
    double start_time_##id = get_time_us();

#define PERF_END(id) \
    do { \
        double elapsed = get_time_us() - start_time_##id; \
        g_perf_stats[id].call_count++; \
        g_perf_stats[id].total_time_us += elapsed; \
        if (elapsed < g_perf_stats[id].min_time_us || \
            g_perf_stats[id].min_time_us == 0) { \
            g_perf_stats[id].min_time_us = elapsed; \
        } \
        if (elapsed > g_perf_stats[id].max_time_us) { \
            g_perf_stats[id].max_time_us = elapsed; \
        } \
    } while(0)
```

#### 在函数中使用

```c
float GetRFInfo(int channel, int type) {
    PERF_START(0);

    // ... 原有代码

    PERF_END(0);
    return value;
}
```

#### 打印性能报告

```c
void PrintPerfStats(void) {
    printf("\n=== Performance Statistics ===\n");
    printf("Function       Calls      Total(ms)  Avg(us)   Min(us)   Max(us)\n");
    printf("---------------------------------------------------------------\n");

    const char *func_names[] = {
        "GetRFInfo", "GetXYPosition", "GetButton",
        "ReadWaveform", "SetReg", "GetReg"
    };

    for (int i = 0; i < 6; i++) {
        if (g_perf_stats[i].call_count > 0) {
            double avg = g_perf_stats[i].total_time_us /
                        g_perf_stats[i].call_count;

            printf("%-14s %6lu %12.2f %9.2f %9.2f %9.2f\n",
                   func_names[i],
                   g_perf_stats[i].call_count,
                   g_perf_stats[i].total_time_us / 1000.0,
                   avg,
                   g_perf_stats[i].min_time_us,
                   g_perf_stats[i].max_time_us);
        }
    }
}
```

---

### 3.2 优化技巧

#### 1. 缓存三角函数计算

```c
// 预计算sin/cos表
#define SIN_TABLE_SIZE 1000
static float g_sin_table[SIN_TABLE_SIZE];
static int g_sin_table_initialized = 0;

void initSinTable(void) {
    for (int i = 0; i < SIN_TABLE_SIZE; i++) {
        double angle = 2.0 * M_PI * i / SIN_TABLE_SIZE;
        g_sin_table[i] = sin(angle);
    }
    g_sin_table_initialized = 1;
}

float fastSin(double angle) {
    // 归一化到 [0, 2π)
    while (angle < 0) angle += 2.0 * M_PI;
    while (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;

    int index = (int)(angle / (2.0 * M_PI) * SIN_TABLE_SIZE);
    return g_sin_table[index];
}
```

#### 2. 减少重复计算

```c
// 不好：每次都计算
float GetRFInfo(int channel, int type) {
    double time = g_config.simulation_time;
    double base = g_rf_channels[ch_idx].base_amplitude;
    double drift = 0.001 * sin(2.0 * M_PI * 0.01 * time);  // 重复
    double variation = 0.01 * sin(2.0 * M_PI * 0.1 * time); // 重复
    // ...
}

// 好：缓存计算结果
static double g_cached_drift = 0.0;
static double g_cached_variation = 0.0;
static double g_last_update_time = -1.0;

void updateCache(double time) {
    if (time != g_last_update_time) {
        g_cached_drift = 0.001 * sin(2.0 * M_PI * 0.01 * time);
        g_cached_variation = 0.01 * sin(2.0 * M_PI * 0.1 * time);
        g_last_update_time = time;
    }
}

float GetRFInfo(int channel, int type) {
    double time = g_config.simulation_time;
    updateCache(time);  // 只在时间改变时计算
    // 使用 g_cached_drift 和 g_cached_variation
}
```

---

## 4. 调试辅助功能

### 4.1 数据记录

```c
// 启用数据记录
static FILE *g_log_file = NULL;

int EnableDataLogging(const char *filename) {
    g_log_file = fopen(filename, "w");
    if (!g_log_file) {
        printf("ERROR: Cannot open log file: %s\n", filename);
        return -1;
    }

    fprintf(g_log_file, "# Time, RF3_Amp, RF3_Phase, XY1_X, XY1_Y\n");
    printf("Data logging enabled: %s\n", filename);

    return 0;
}

void DisableDataLogging(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
        printf("Data logging disabled\n");
    }
}

// 在数据生成时记录
float GetRFInfo(int channel, int type) {
    // ... 生成数据

    if (g_log_file && channel == 3 && type == 0) {
        fprintf(g_log_file, "%.3f, %.6f\n",
                g_config.simulation_time, value);
    }

    return value;
}
```

---

### 4.2 调试输出控制

```c
// 调试级别
typedef enum {
    DEBUG_NONE = 0,
    DEBUG_ERROR = 1,
    DEBUG_WARN = 2,
    DEBUG_INFO = 3,
    DEBUG_VERBOSE = 4
} DebugLevel;

static DebugLevel g_debug_level = DEBUG_ERROR;

void SetDebugLevel(int level) {
    g_debug_level = level;
    printf("Debug level set to: %d\n", level);
}

// 调试宏
#define DEBUG_PRINT(level, fmt, ...) \
    do { \
        if (g_debug_level >= level) { \
            printf("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__); \
        } \
    } while(0)

// 使用示例
float GetRFInfo(int channel, int type) {
    DEBUG_PRINT(DEBUG_VERBOSE, "Called with channel=%d, type=%d\n",
                channel, type);

    if (channel < 3 || channel > 6) {
        DEBUG_PRINT(DEBUG_ERROR, "Invalid channel: %d\n", channel);
        return 0.0f;
    }

    // ...
}
```

---

## 5. 完整示例：高级Mock库

### 5.1 头文件扩展

```c
// libbpm_mock_advanced.h
#ifndef LIBBPM_MOCK_ADVANCED_H
#define LIBBPM_MOCK_ADVANCED_H

// 故障注入
int EnableFaultInjection(int fault_id, int fault_type,
                        double probability, int duration);
int DisableFaultInjection(int fault_id);

// 场景回放
int LoadScenario(const char *filename, int loop);
int StopScenario(void);

// 性能统计
void PrintPerfStats(void);
void ResetPerfStats(void);

// 数据记录
int EnableDataLogging(const char *filename);
void DisableDataLogging(void);

// 调试
void SetDebugLevel(int level);

#endif
```

---

### 5.2 测试程序

```c
// test_advanced.c
#include <stdio.h>
#include "libbpm_mock.h"
#include "libbpm_mock_advanced.h"

int main() {
    printf("=== Advanced Mock Library Test ===\n\n");

    SystemInit();
    SetDebugLevel(3); // INFO level

    // 测试1: 性能测试
    printf("=== Test 1: Performance ===\n");
    for (int i = 0; i < 1000; i++) {
        GetRFInfo(3, 0);
        GetRFInfo(3, 1);
        GetXYPosition(0);
    }
    PrintPerfStats();

    // 测试2: 故障注入
    printf("\n=== Test 2: Fault Injection ===\n");
    EnableFaultInjection(0, FAULT_TIMEOUT, 0.5, 5);

    for (int i = 0; i < 10; i++) {
        float val = GetRFInfo(3, 0);
        printf("RF3[%d] = %.2f\n", i, val);
    }

    // 测试3: 数据记录
    printf("\n=== Test 3: Data Logging ===\n");
    EnableDataLogging("mock_data.csv");

    for (int i = 0; i < 100; i++) {
        TriggerAllDataReached();
        GetRFInfo(3, 0);
        usleep(10000); // 10ms
    }

    DisableDataLogging();
    printf("Data logged to mock_data.csv\n");

    // 测试4: 场景回放
    printf("\n=== Test 4: Scenario Replay ===\n");
    if (LoadScenario("test_scenario.txt", 0) == 0) {
        for (int i = 0; i < 50; i++) {
            float amp = GetRFInfo(3, 0);
            printf("Scenario[%d] = %.2f\n", i, amp);
        }
        StopScenario();
    }

    SystemClose();
    return 0;
}
```

---

## 6. Makefile更新

```makefile
# 添加高级功能的编译
CC = gcc
CFLAGS = -fPIC -Wall -O2 -g
LDFLAGS = -shared -lm -lpthread

# 目标文件
OBJS = libbpm_mock.o libbpm_mock_advanced.o

all: libbpm_mock.so test_advanced

libbpm_mock.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

libbpm_mock_advanced.o: libbpm_mock_advanced.c libbpm_mock_advanced.h
	$(CC) $(CFLAGS) -c $<

test_advanced: test_advanced.c libbpm_mock.so
	$(CC) -o $@ $< -L. -lbpm_mock -Wl,-rpath,.

clean:
	rm -f *.o *.so test_advanced mock_data.csv
```

---

## 7. 实践练习

### 练习1: 实现故障恢复

**任务**: 添加自动故障恢复功能

```c
// 提示：
// 1. 检测连续故障次数
// 2. 超过阈值后自动恢复
// 3. 记录恢复次数
```

### 练习2: 场景编辑器

**任务**: 编写工具生成场景文件

```c
// 提示：
// 1. 从真实数据转换
// 2. 生成测试场景（ramp, step, spike）
// 3. 验证场景文件格式
```

### 练习3: 实时性能监控

**任务**: 添加实时性能显示

```c
// 提示：
// 1. 每秒打印性能统计
// 2. 检测性能下降
// 3. 自动优化建议
```

---

## 8. 总结

### 你学到了什么？

✅ **故障注入系统**
- 设计故障类型和配置
- 实现概率性故障触发
- 在数据生成中集成故障

✅ **场景回放**
- 设计场景数据格式
- 加载和解析场景文件
- 回放和循环控制

✅ **性能优化**
- 测量函数性能
- 缓存计算结果
- 优化热点代码

✅ **调试辅助**
- 数据记录和分析
- 分级调试输出
- 性能报告

---

### 下一步

现在你已经掌握了高级Mock库技巧！

继续学习：
- **[07-build-and-test.md](./07-build-and-test.md)** - 详细编译和测试指南
- **[08-debugging-mock.md](./08-debugging-mock.md)** - Mock库调试技巧
- **[09-integration-with-ioc.md](./09-integration-with-ioc.md)** - 与BPMIOC IOC集成

---

**🎯 重要提示**: 这些高级功能不是必需的，但它们会让你的开发效率大幅提升！先确保基本的Mock库工作正常，再逐步添加这些功能。
