# 如何编写硬件模拟器（Simulator）教程

> **目标**: 从零开始学习如何编写硬件模拟器
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 3-4小时
> **前置知识**: C语言基础、指针、函数

## 📋 本教程内容

完成本教程后，你将能够：
- ✅ 理解硬件模拟器的基本原理
- ✅ 设计模拟器的架构
- ✅ 实现数据生成和管理
- ✅ 添加配置和控制功能
- ✅ 编写测试和调试代码

## 🎯 什么是硬件模拟器？

### 定义

**硬件模拟器（Simulator/Mock）** 是一个软件程序，它**模仿真实硬件的行为**，让你可以在没有实际硬件的情况下开发和测试软件。

### 类比理解

```
真实场景：
  你的程序 → 真实硬件（BPM板卡） → 物理信号

模拟器场景：
  你的程序 → 模拟器（软件） → 数学生成的信号

关键：你的程序"感觉"不到区别！
```

### 为什么需要模拟器？

| 问题 | 使用真实硬件 | 使用模拟器 |
|------|-------------|-----------|
| **成本** | 几万到几十万 | 免费 |
| **可用性** | 需要预约、配置 | 随时可用 |
| **开发速度** | 分钟级（部署、测试） | 秒级 |
| **调试** | 有限（难加断点） | 完整（GDB等） |
| **风险** | 可能损坏硬件 | 零风险 |
| **可重复性** | 信号可能变化 | 100%可重复 |

## 📐 模拟器设计原则

### 原则1：接口兼容

**关键思想**：模拟器必须提供与真实硬件**完全相同的接口**。

```c
// 真实硬件库（libBPMboard.so）提供的接口
int SystemInit(void);
int GetRfInfo(float *Amp, float *Phase, ...);
int SetPhaseOffset(int channel, float value);

// 模拟器库（libBPMboardMock.so）必须提供相同接口
int SystemInit(void);           // ← 相同函数签名
int GetRfInfo(float *Amp, ...); // ← 相同参数
int SetPhaseOffset(int ch, ...);// ← 相同返回值
```

**这样做的好处**：
```c
// 用户代码完全不需要修改！
#ifdef USE_REAL_HARDWARE
    dlopen("libBPMboard.so", ...);
#else
    dlopen("libBPMboardMock.so", ...);  // ← 只换库，代码不变
#endif

// 后续调用完全一样
SystemInit();
GetRfInfo(amp, phase, ...);
```

### 原则2：行为合理

模拟器不需要**完全精确**，但必须**合理**。

```c
// ❌ 不合理的模拟
int GetRfInfo(...) {
    Amp[0] = 3.14;  // 总是返回固定值
    Phase[0] = 0.0; // 数据不变化 → 无法测试时间相关逻辑
}

// ✅ 合理的模拟
int GetRfInfo(...) {
    static double time = 0.0;
    Amp[0] = 4.0 + sin(2*PI*0.5*time);  // 随时间变化
    Phase[0] = 90*sin(2*PI*0.1*time);   // 有意义的变化
    time += 0.1;
    return 0;
}
```

### 原则3：可配置

不同的测试场景需要不同的数据。

```c
// ❌ 硬编码
int GetRfInfo(...) {
    Amp[0] = 4.0;  // 无法改变
}

// ✅ 可配置
typedef struct {
    double amplitude;  // 可配置幅度
    double frequency;  // 可配置频率
    double noise;      // 可配置噪声
} ChannelConfig;

ChannelConfig g_config[8];  // 8个通道独立配置

int GetRfInfo(...) {
    for (int ch = 0; ch < 8; ch++) {
        Amp[ch] = g_config[ch].amplitude *
                  sin(2*PI * g_config[ch].frequency * time);
    }
}
```

## 🔨 实战：从零开始编写模拟器

### 第1步：最简单的模拟器

让我们从最简单的例子开始。

**目标**：模拟一个温度传感器

#### 1.1 定义接口

```c
// temp_sensor.h - 真实硬件的接口（已知）

/**
 * 初始化温度传感器
 * @return 0=成功, -1=失败
 */
int TempSensor_Init(void);

/**
 * 读取温度
 * @param temp [out] 温度值（摄氏度）
 * @return 0=成功, -1=失败
 */
int TempSensor_Read(float *temp);
```

#### 1.2 实现模拟器（版本1：最简单）

```c
// temp_sensor_mock.c - 我们的第一个模拟器

#include "temp_sensor.h"
#include <stdio.h>

static int initialized = 0;

int TempSensor_Init(void)
{
    printf("[Mock] TempSensor_Init called\n");
    initialized = 1;
    return 0;  // 成功
}

int TempSensor_Read(float *temp)
{
    if (!initialized) {
        printf("[Mock] Error: not initialized\n");
        return -1;  // 失败
    }

    if (temp == NULL) {
        printf("[Mock] Error: NULL pointer\n");
        return -1;
    }

    // 简单返回固定值
    *temp = 25.0;  // 室温25度

    printf("[Mock] TempSensor_Read: %.1f C\n", *temp);
    return 0;
}
```

#### 1.3 测试

```c
// test_temp.c

#include "temp_sensor.h"
#include <stdio.h>

int main(void)
{
    float temperature;

    // 测试1：未初始化时读取
    printf("Test 1: Read before init\n");
    int ret = TempSensor_Read(&temperature);
    // 期望：返回-1（失败）

    // 测试2：初始化
    printf("\nTest 2: Initialize\n");
    ret = TempSensor_Init();
    // 期望：返回0（成功）

    // 测试3：读取温度
    printf("\nTest 3: Read temperature\n");
    ret = TempSensor_Read(&temperature);
    printf("Temperature: %.1f C (ret=%d)\n", temperature, ret);
    // 期望：返回0，温度25.0

    return 0;
}
```

**编译和运行**：
```bash
gcc -c temp_sensor_mock.c -o temp_sensor_mock.o
gcc -c test_temp.c -o test_temp.o
gcc temp_sensor_mock.o test_temp.o -o test_temp
./test_temp
```

**输出**：
```
Test 1: Read before init
[Mock] Error: not initialized

Test 2: Initialize
[Mock] TempSensor_Init called

Test 3: Read temperature
[Mock] TempSensor_Read: 25.0 C
Temperature: 25.0 C (ret=0)
```

✅ **恭喜！** 你已经写出了第一个模拟器！

---

### 第2步：添加动态变化

固定值太简单，让温度随时间变化。

#### 2.1 添加时间模拟

```c
// temp_sensor_mock.c - 版本2：添加时间变化

#include "temp_sensor.h"
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int initialized = 0;
static double simulation_time = 0.0;  // 模拟时间（秒）

int TempSensor_Init(void)
{
    printf("[Mock] TempSensor_Init called\n");
    initialized = 1;
    simulation_time = 0.0;
    return 0;
}

int TempSensor_Read(float *temp)
{
    if (!initialized) {
        return -1;
    }

    if (temp == NULL) {
        return -1;
    }

    // 基础温度 + 正弦波变化（模拟昼夜温差）
    double base_temp = 25.0;              // 平均温度
    double variation = 5.0;               // 温度变化幅度（±5度）
    double period = 24.0;                 // 周期（24小时）

    *temp = base_temp + variation * sin(2.0 * M_PI * simulation_time / period);

    printf("[Mock] Time=%.1fs, Temp=%.1f C\n", simulation_time, *temp);

    // 推进时间（假设每次读取间隔1小时）
    simulation_time += 1.0;

    return 0;
}
```

#### 2.2 测试时间变化

```c
// test_temp_dynamic.c

#include "temp_sensor.h"
#include <stdio.h>

int main(void)
{
    TempSensor_Init();

    printf("Reading temperature over 24 hours:\n");
    printf("Time(h)  Temp(C)\n");
    printf("-------------------\n");

    for (int hour = 0; hour < 24; hour++) {
        float temp;
        TempSensor_Read(&temp);
        printf("%2d:00    %.1f\n", hour, temp);
    }

    return 0;
}
```

**输出**：
```
Reading temperature over 24 hours:
Time(h)  Temp(C)
-------------------
[Mock] Time=0.0s, Temp=25.0 C
 0:00    25.0
[Mock] Time=1.0s, Temp=26.3 C
 1:00    26.3
[Mock] Time=2.0s, Temp=27.5 C
 2:00    27.5
...
[Mock] Time=12.0s, Temp=25.0 C
12:00    25.0  ← 12小时后回到平均温度
...
```

✅ 现在温度会**随时间变化**，更真实了！

---

### 第3步：添加噪声

真实传感器有噪声，我们也模拟一下。

#### 3.1 添加随机噪声

```c
// temp_sensor_mock.c - 版本3：添加噪声

#include <stdlib.h>
#include <time.h>

int TempSensor_Init(void)
{
    printf("[Mock] TempSensor_Init called\n");
    initialized = 1;
    simulation_time = 0.0;

    // 初始化随机数种子
    srand(time(NULL));

    return 0;
}

int TempSensor_Read(float *temp)
{
    // ... 前面代码相同 ...

    // 计算基础温度
    double value = base_temp + variation * sin(2.0 * M_PI * simulation_time / period);

    // 添加噪声（±0.5度的随机波动）
    double noise = ((double)rand() / RAND_MAX - 0.5) * 1.0;  // -0.5 ~ +0.5
    value += noise;

    *temp = value;

    printf("[Mock] Time=%.1fs, Temp=%.2f C (noise=%.2f)\n",
           simulation_time, *temp, noise);

    simulation_time += 1.0;
    return 0;
}
```

**输出**：
```
[Mock] Time=0.0s, Temp=25.23 C (noise=0.23)
[Mock] Time=1.0s, Temp=26.15 C (noise=-0.15)
[Mock] Time=2.0s, Temp=27.68 C (noise=0.18)
```

✅ 现在有了随机噪声，更接近真实传感器！

---

### 第4步：可配置参数

不同测试需要不同参数，我们让它可配置。

#### 4.1 添加配置结构

```c
// temp_sensor_mock.c - 版本4：可配置

typedef struct {
    double base_temperature;    // 基础温度
    double variation;           // 变化幅度
    double noise_level;         // 噪声水平
    double time_step;           // 时间步长
} SensorConfig;

// 全局配置（带默认值）
static SensorConfig g_config = {
    .base_temperature = 25.0,
    .variation = 5.0,
    .noise_level = 0.5,
    .time_step = 1.0
};

/**
 * 设置配置参数
 */
void TempSensor_SetConfig(double base_temp, double variation, double noise)
{
    g_config.base_temperature = base_temp;
    g_config.variation = variation;
    g_config.noise_level = noise;

    printf("[Mock] Config updated: base=%.1f, var=%.1f, noise=%.2f\n",
           base_temp, variation, noise);
}

int TempSensor_Read(float *temp)
{
    // ... 检查代码 ...

    // 使用配置参数
    double value = g_config.base_temperature +
                   g_config.variation * sin(2.0 * M_PI * simulation_time / 24.0);

    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * g_config.noise_level;
    value += noise;

    *temp = value;

    simulation_time += g_config.time_step;
    return 0;
}
```

#### 4.2 使用配置

```c
// test_config.c

int main(void)
{
    TempSensor_Init();

    // 场景1：正常室温
    printf("Scenario 1: Normal room temperature\n");
    TempSensor_SetConfig(25.0, 5.0, 0.5);
    for (int i = 0; i < 5; i++) {
        float temp;
        TempSensor_Read(&temp);
        printf("  Temp: %.2f C\n", temp);
    }

    // 场景2：冷冻室（低温，小波动，低噪声）
    printf("\nScenario 2: Freezer (-18C, small variation)\n");
    TempSensor_SetConfig(-18.0, 2.0, 0.1);
    for (int i = 0; i < 5; i++) {
        float temp;
        TempSensor_Read(&temp);
        printf("  Temp: %.2f C\n", temp);
    }

    // 场景3：沙漠（高温，大波动，高噪声）
    printf("\nScenario 3: Desert (40C, large variation)\n");
    TempSensor_SetConfig(40.0, 15.0, 2.0);
    for (int i = 0; i < 5; i++) {
        float temp;
        TempSensor_Read(&temp);
        printf("  Temp: %.2f C\n", temp);
    }

    return 0;
}
```

✅ 现在可以模拟**不同的环境条件**了！

---

### 第5步：从文件加载配置

硬编码配置不方便，让我们从文件读取。

#### 5.1 配置文件格式

创建 `sensor.conf`：
```ini
# Temperature Sensor Configuration

[Sensor]
base_temperature = 25.0
variation = 5.0
noise_level = 0.5
time_step = 1.0
```

#### 5.2 解析配置文件

```c
// temp_sensor_mock.c - 版本5：从文件加载配置

#include <string.h>

int TempSensor_LoadConfig(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("[Mock] Failed to open config file: %s\n", filename);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[') {
            continue;
        }

        // 解析 key = value
        char key[64], value[64];
        if (sscanf(line, "%63s = %63s", key, value) == 2) {
            if (strcmp(key, "base_temperature") == 0) {
                g_config.base_temperature = atof(value);
            } else if (strcmp(key, "variation") == 0) {
                g_config.variation = atof(value);
            } else if (strcmp(key, "noise_level") == 0) {
                g_config.noise_level = atof(value);
            } else if (strcmp(key, "time_step") == 0) {
                g_config.time_step = atof(value);
            }
        }
    }

    fclose(fp);

    printf("[Mock] Config loaded from %s\n", filename);
    printf("  base_temp=%.1f, var=%.1f, noise=%.2f\n",
           g_config.base_temperature, g_config.variation, g_config.noise_level);

    return 0;
}
```

#### 5.3 使用

```c
int main(void)
{
    TempSensor_Init();

    // 从文件加载配置
    TempSensor_LoadConfig("sensor.conf");

    // 使用配置读取
    for (int i = 0; i < 10; i++) {
        float temp;
        TempSensor_Read(&temp);
        printf("Temp: %.2f C\n", temp);
    }

    return 0;
}
```

✅ 现在可以通过**修改配置文件**来改变行为，无需重新编译！

---

## 🎓 进阶技巧

### 技巧1：多种模拟模式

```c
typedef enum {
    MODE_CONSTANT,      // 恒定值
    MODE_SINE,          // 正弦波
    MODE_RANDOM,        // 随机
    MODE_FILE_REPLAY    // 文件回放
} SimulationMode;

static SimulationMode g_mode = MODE_SINE;

void TempSensor_SetMode(SimulationMode mode)
{
    g_mode = mode;
    printf("[Mock] Mode set to %d\n", mode);
}

int TempSensor_Read(float *temp)
{
    switch (g_mode) {
        case MODE_CONSTANT:
            *temp = g_config.base_temperature;
            break;

        case MODE_SINE:
            *temp = g_config.base_temperature +
                    g_config.variation * sin(2.0 * M_PI * simulation_time / 24.0);
            break;

        case MODE_RANDOM:
            *temp = g_config.base_temperature +
                    ((double)rand() / RAND_MAX - 0.5) * 2.0 * g_config.variation;
            break;

        case MODE_FILE_REPLAY:
            // 从文件读取（后续实现）
            *temp = 25.0;
            break;
    }

    simulation_time += g_config.time_step;
    return 0;
}
```

### 技巧2：故障注入

```c
typedef enum {
    FAULT_NONE = 0,
    FAULT_SENSOR_DEAD,       // 传感器死掉（返回0）
    FAULT_SENSOR_STUCK,      // 传感器卡死（值不变）
    FAULT_OVER_RANGE,        // 超量程
    FAULT_NOISE_BURST        // 噪声突发
} FaultType;

static FaultType g_fault = FAULT_NONE;
static float g_stuck_value = 0.0;

void TempSensor_InjectFault(FaultType fault)
{
    g_fault = fault;
    printf("[Mock] Fault injected: %d\n", fault);
}

int TempSensor_Read(float *temp)
{
    // 先正常计算值
    float value = calculate_temperature();

    // 应用故障
    switch (g_fault) {
        case FAULT_SENSOR_DEAD:
            value = 0.0;
            break;

        case FAULT_SENSOR_STUCK:
            value = g_stuck_value;  // 返回上次的值
            break;

        case FAULT_OVER_RANGE:
            value = 999.9;  // 超量程错误码
            break;

        case FAULT_NOISE_BURST:
            value += ((double)rand() / RAND_MAX - 0.5) * 50.0;  // 大噪声
            break;

        default:
            break;
    }

    g_stuck_value = value;  // 保存供STUCK模式使用
    *temp = value;
    return 0;
}
```

使用：
```c
// 测试传感器死机
TempSensor_InjectFault(FAULT_SENSOR_DEAD);
TempSensor_Read(&temp);  // 返回0.0

// 测试噪声突发
TempSensor_InjectFault(FAULT_NOISE_BURST);
for (int i = 0; i < 10; i++) {
    TempSensor_Read(&temp);
    printf("Temp with noise burst: %.2f\n", temp);
}
```

### 技巧3：统计和调试信息

```c
typedef struct {
    int read_count;          // 读取次数
    float min_temp;          // 最小温度
    float max_temp;          // 最大温度
    float avg_temp;          // 平均温度
    double total_time;       // 总运行时间
} Statistics;

static Statistics g_stats = {0};

int TempSensor_Read(float *temp)
{
    // ... 计算温度 ...

    // 更新统计
    g_stats.read_count++;
    if (*temp < g_stats.min_temp || g_stats.read_count == 1) {
        g_stats.min_temp = *temp;
    }
    if (*temp > g_stats.max_temp || g_stats.read_count == 1) {
        g_stats.max_temp = *temp;
    }
    g_stats.avg_temp = (g_stats.avg_temp * (g_stats.read_count - 1) + *temp) /
                       g_stats.read_count;
    g_stats.total_time = simulation_time;

    return 0;
}

void TempSensor_PrintStatistics(void)
{
    printf("=== Temperature Sensor Statistics ===\n");
    printf("  Read count: %d\n", g_stats.read_count);
    printf("  Min temp:   %.2f C\n", g_stats.min_temp);
    printf("  Max temp:   %.2f C\n", g_stats.max_temp);
    printf("  Avg temp:   %.2f C\n", g_stats.avg_temp);
    printf("  Total time: %.1f s\n", g_stats.total_time);
    printf("=====================================\n");
}
```

---

## 📚 完整示例：RF通道模拟器

现在让我们看一个更复杂的例子，模拟BPMIOC的RF通道。

### 接口定义

```c
// rf_channel.h

typedef struct {
    float amplitude;   // 幅度 (V)
    float phase;       // 相位 (度)
    float power;       // 功率 (W)
} RfData;

int RfChannel_Init(int num_channels);
int RfChannel_Read(int channel, RfData *data);
int RfChannel_SetConfig(int channel, double freq, double amp);
```

### 模拟器实现

```c
// rf_channel_mock.c

#include "rf_channel.h"
#include <stdlib.h>
#include <math.h>

#define MAX_CHANNELS 8
#define M_PI 3.14159265358979323846

typedef struct {
    double frequency;    // 变化频率 (Hz)
    double amplitude;    // 幅度基准 (V)
    double phase_offset; // 相位偏移 (rad)
    double noise_level;  // 噪声水平
} ChannelConfig;

static ChannelConfig g_channels[MAX_CHANNELS];
static int g_num_channels = 0;
static int g_initialized = 0;
static double g_time = 0.0;

int RfChannel_Init(int num_channels)
{
    if (num_channels < 1 || num_channels > MAX_CHANNELS) {
        return -1;
    }

    g_num_channels = num_channels;

    // 初始化每个通道（默认配置）
    for (int ch = 0; ch < num_channels; ch++) {
        g_channels[ch].frequency = 0.5 + ch * 0.05;      // 0.5~0.85 Hz
        g_channels[ch].amplitude = 4.0;
        g_channels[ch].phase_offset = ch * M_PI / 4.0;  // 0, 45, 90...度
        g_channels[ch].noise_level = 0.02;               // 2%噪声
    }

    g_initialized = 1;
    g_time = 0.0;

    printf("[Mock] RF Channels initialized: %d channels\n", num_channels);
    return 0;
}

int RfChannel_Read(int channel, RfData *data)
{
    if (!g_initialized) {
        return -1;
    }

    if (channel < 0 || channel >= g_num_channels) {
        return -1;
    }

    if (data == NULL) {
        return -1;
    }

    ChannelConfig *cfg = &g_channels[channel];

    // 计算幅度（正弦波 + 噪声）
    double amp = cfg->amplitude * (1.0 + 0.2 * sin(2.0 * M_PI * cfg->frequency * g_time));
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level * amp;
    data->amplitude = amp + noise;

    // 计算相位（慢速漂移）
    data->phase = 90.0 * sin(2.0 * M_PI * 0.1 * g_time + cfg->phase_offset);

    // 计算功率 (P = A^2 * R, 假设50欧姆)
    data->power = data->amplitude * data->amplitude * 50.0;

    return 0;
}

int RfChannel_SetConfig(int channel, double freq, double amp)
{
    if (!g_initialized || channel < 0 || channel >= g_num_channels) {
        return -1;
    }

    g_channels[channel].frequency = freq;
    g_channels[channel].amplitude = amp;

    printf("[Mock] Channel %d config: freq=%.2f Hz, amp=%.2f V\n",
           channel, freq, amp);

    return 0;
}

void RfChannel_UpdateTime(double delta_time)
{
    g_time += delta_time;
}
```

### 测试

```c
// test_rf.c

#include "rf_channel.h"
#include <stdio.h>

int main(void)
{
    // 初始化8个通道
    RfChannel_Init(8);

    printf("Time(s)  Ch0_Amp  Ch0_Phase  Ch0_Power\n");
    printf("-------------------------------------------\n");

    // 读取100次（模拟10秒）
    for (int i = 0; i < 100; i++) {
        RfData data;
        RfChannel_Read(0, &data);

        printf("%.1f     %.3f    %.1f       %.1f\n",
               i * 0.1, data.amplitude, data.phase, data.power);

        RfChannel_UpdateTime(0.1);  // 推进100ms
    }

    return 0;
}
```

---

## ✅ 总结：编写模拟器的步骤

### 步骤清单

1. **分析真实硬件接口**
   - [ ] 列出所有函数
   - [ ] 记录参数和返回值
   - [ ] 理解数据格式

2. **设计模拟器架构**
   - [ ] 决定数据生成方式（固定/随机/计算）
   - [ ] 设计配置结构
   - [ ] 规划状态管理

3. **实现基础功能**
   - [ ] 初始化函数
   - [ ] 数据读取函数
   - [ ] 错误处理

4. **添加真实感**
   - [ ] 时间变化
   - [ ] 噪声
   - [ ] 合理的数值范围

5. **增加灵活性**
   - [ ] 可配置参数
   - [ ] 多种模式
   - [ ] 从文件加载

6. **测试和调试**
   - [ ] 单元测试
   - [ ] 边界条件测试
   - [ ] 性能测试

### 常见陷阱

❌ **陷阱1**: 返回固定值
```c
// 太简单，无法测试时间相关逻辑
int Read() { return 3.14; }
```

❌ **陷阱2**: 忘记错误处理
```c
// 真实硬件会失败，模拟器也应该能模拟失败
int Read(float *data) {
    *data = 3.14;  // 没检查NULL指针！
    return 0;      // 永远成功？
}
```

❌ **陷阱3**: 硬编码magic numbers
```c
// 3.14是什么？为什么是3.14？
*temp = 3.14 + 1.5 * sin(0.628 * time);
```

✅ **正确做法**:
```c
const double BASE_TEMP = 25.0;  // 室温
const double VARIATION = 5.0;    // ±5度
const double PERIOD = 24.0;      // 24小时周期

*temp = BASE_TEMP + VARIATION * sin(2*PI*time/PERIOD);
```

---

## 🚀 下一步

现在你已经学会了编写模拟器的基础知识！

### 实践练习

试着为这些硬件编写模拟器：

1. **LED控制器**
   - `LED_SetBrightness(int led, int brightness)`
   - `LED_GetStatus(int led, int *status)`

2. **电机控制**
   - `Motor_SetSpeed(int motor, float speed)`
   - `Motor_GetPosition(int motor, float *position)`

3. **ADC转换器**
   - `ADC_Init(int channels)`
   - `ADC_Read(int channel, uint16_t *value)`

### 参考资料

- [BPMIOC Simulator源码](../../simulator/src/) - 完整的生产级实现
- [mockHardware.c](../../simulator/src/mockHardware.c) - 参考实现
- [测试代码](../../simulator/src/test_mock.c) - 测试示例

---

**🎓 恭喜！** 你现在知道如何编写硬件模拟器了！

记住核心原则：
1. **接口兼容** - 完全相同的函数签名
2. **行为合理** - 数据有意义、会变化
3. **可配置** - 灵活应对不同测试场景

开始动手写你自己的模拟器吧！💪
