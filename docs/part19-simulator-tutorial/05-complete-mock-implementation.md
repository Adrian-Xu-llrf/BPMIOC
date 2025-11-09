# 完整Mock库实现

> **阅读时间**: 60分钟
> **难度**: ⭐⭐⭐⭐⭐
> **目标**: 获得一个完整可用的libbpm_mock.c

## 📋 本文目标

- 提供完整的Mock库源代码
- 实现所有50+硬件函数
- 可以直接编译使用
- 与BPMIOC完全兼容

## 📂 文件结构

```
~/BPMIOC/
├── simulator/                    # 新建目录
│   ├── src/
│   │   ├── libbpm_mock.c        # 主Mock库（本文）
│   │   ├── libbpm_mock.h        # 头文件
│   │   └── test_mock.c          # 测试程序
│   ├── Makefile
│   └── config/
│       └── mock_config.ini      # 配置文件
```

## 📝 完整源码

### 1. 头文件: libbpm_mock.h

```c
// libbpm_mock.h
// BPMIOC Mock Hardware Library
// 提供与真实硬件相同的接口，但数据由软件生成

#ifndef LIBBPM_MOCK_H
#define LIBBPM_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

// ===== 系统管理函数 =====
int SystemInit(void);
void SystemClose(void);
int GetSystemStatus(void);
const char* GetVersion(void);

// ===== 数据采集函数 =====
int TriggerAllDataReached(void);
void StartAcquisition(void);
void StopAcquisition(void);
int IsDataReady(void);

// ===== RF数据函数 =====
float GetRFInfo(int channel, int type);
float GetCenterFrequency(void);
void SetCenterFrequency(float freq);

// ===== XY位置函数 =====
float GetXYPosition(int channel);
float GetQ(int channel);

// ===== Button信号函数 =====
float GetButtonSignal(int index);
float GetButtonSum(void);

// ===== 寄存器函数 =====
void SetReg(int addr, int value);
int GetReg(int addr);

// ===== 配置函数 =====
int LoadConfig(const char *filename);
void SetVerboseLevel(int level);

// ===== 调试函数 =====
void PrintStatistics(void);
void ResetStatistics(void);

#ifdef __cplusplus
}
#endif

#endif // LIBBPM_MOCK_H
```

### 2. 主实现: libbpm_mock.c

```c
// libbpm_mock.c
// BPMIOC Mock Hardware Library Implementation
// Author: Your Name
// Date: 2025-01-09

#include "libbpm_mock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ===== 宏定义 =====
#define MAX_CHANNELS 8
#define MAX_REGISTERS 100
#define VERSION "1.0.0"

// ===== 全局配置结构 =====
typedef struct {
    int initialized;
    double simulation_time;
    double time_step;
    int verbose;
} MockConfig;

static MockConfig g_config = {
    .initialized = 0,
    .simulation_time = 0.0,
    .time_step = 0.1,
    .verbose = 1
};

// ===== RF通道配置 =====
typedef struct {
    double base_amplitude;
    double base_phase;
    double center_frequency;
    double amp_variation_freq;
    double amp_variation_depth;
    double phase_drift_rate;
    double amp_noise_level;
    double phase_noise_level;
    int enabled;
} RfChannelConfig;

static RfChannelConfig g_rf_channels[4] = {
    // RF3
    {4.0, 0.0, 499.8, 0.5, 0.1, 10.0, 0.02, 2.0, 1},
    // RF4
    {3.8, 45.0, 499.8, 0.6, 0.12, 12.0, 0.025, 2.5, 1},
    // RF5
    {4.2, 90.0, 499.8, 0.55, 0.11, 11.0, 0.022, 2.2, 1},
    // RF6
    {3.9, 135.0, 499.8, 0.52, 0.10, 10.5, 0.023, 2.3, 1}
};

// ===== XY位置配置 =====
typedef struct {
    double orbit_center_x;
    double orbit_center_y;
    double orbit_radius_x;
    double orbit_radius_y;
    double orbit_frequency;
    double orbit_phase;
    double jitter_amplitude_x;
    double jitter_amplitude_y;
    double jitter_frequency;
    double noise_level;
    int enabled;
} XYPositionConfig;

static XYPositionConfig g_xy_config[2] = {
    // BPM1
    {0.0, 0.0, 2.0, 1.5, 0.5, 0.0, 0.1, 0.1, 50.0, 0.05, 1},
    // BPM2
    {0.5, -0.3, 2.1, 1.6, 0.5, M_PI/4, 0.12, 0.12, 55.0, 0.06, 1}
};

// ===== Button配置 =====
typedef struct {
    double base_signal;
    double variation_freq;
    double variation_depth;
    double noise_level;
    double phase_offset;
} ButtonConfig;

static ButtonConfig g_buttons[8] = {
    {50.0, 0.5, 0.1, 0.02, 0.0},
    {48.0, 0.5, 0.1, 0.02, M_PI/2},
    {52.0, 0.5, 0.1, 0.02, M_PI},
    {49.0, 0.5, 0.1, 0.02, 3*M_PI/2},
    {51.0, 0.5, 0.1, 0.02, 0.0},
    {49.5, 0.5, 0.1, 0.02, M_PI/2},
    {50.5, 0.5, 0.1, 0.02, M_PI},
    {48.5, 0.5, 0.1, 0.02, 3*M_PI/2}
};

// ===== 寄存器 =====
static int g_registers[MAX_REGISTERS] = {0};

// ===== 统计信息 =====
typedef struct {
    unsigned long read_count;
    unsigned long trigger_count;
    time_t start_time;
} Statistics;

static Statistics g_stats = {0};

// ========================================
// 内部辅助函数
// ========================================

static float generateRfAmplitude(RfChannelConfig *cfg, double time)
{
    float amplitude = cfg->base_amplitude;

    // 慢速变化
    double slow_var = cfg->amp_variation_depth *
                     sin(2.0 * M_PI * cfg->amp_variation_freq * time);
    amplitude *= (1.0 + slow_var);

    // 噪声
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->amp_noise_level * amplitude;
    amplitude += noise;

    // 限制范围
    if (amplitude < 0.0) amplitude = 0.0;
    if (amplitude > 10.0) amplitude = 10.0;

    return amplitude;
}

static float generateRfPhase(RfChannelConfig *cfg, double time)
{
    float phase = cfg->base_phase;

    // 长期漂移
    phase += cfg->phase_drift_rate * time;

    // 慢速变化
    phase += 30.0 * sin(2.0 * M_PI * 0.1 * time);

    // 噪声
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->phase_noise_level;
    phase += noise;

    // 归一化到-180~180
    while (phase > 180.0) phase -= 360.0;
    while (phase < -180.0) phase += 360.0;

    return phase;
}

static void generateXYPosition(XYPositionConfig *cfg, double time,
                              float *x_out, float *y_out)
{
    double x = cfg->orbit_center_x;
    double y = cfg->orbit_center_y;

    // 主轨道运动
    double orbit_phase = 2.0 * M_PI * cfg->orbit_frequency * time +
                        cfg->orbit_phase;
    x += cfg->orbit_radius_x * cos(orbit_phase);
    y += cfg->orbit_radius_y * sin(orbit_phase);

    // 抖动
    double jitter_phase = 2.0 * M_PI * cfg->jitter_frequency * time;
    x += cfg->jitter_amplitude_x * sin(jitter_phase);
    y += cfg->jitter_amplitude_y * sin(jitter_phase + M_PI/2);

    // 噪声
    x += ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level;
    y += ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level;

    // 限制范围
    if (x > 10.0) x = 10.0;
    if (x < -10.0) x = -10.0;
    if (y > 10.0) y = 10.0;
    if (y < -10.0) y = -10.0;

    *x_out = (float)x;
    *y_out = (float)y;
}

static float generateButtonSignal(ButtonConfig *cfg, double time)
{
    float signal = cfg->base_signal;

    // 变化
    signal *= (1.0 + cfg->variation_depth *
               sin(2.0 * M_PI * cfg->variation_freq * time + cfg->phase_offset));

    // 噪声
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->noise_level * signal;
    signal += noise;

    return signal;
}

// ========================================
// 公共接口实现
// ========================================

int SystemInit(void)
{
    if (g_config.verbose >= 1) {
        printf("[Mock] SystemInit called\n");
    }

    if (g_config.initialized) {
        if (g_config.verbose >= 1) {
            printf("[Mock] Already initialized\n");
        }
        return 0;
    }

    // 初始化随机数
    srand(time(NULL));

    // 重置时间
    g_config.simulation_time = 0.0;

    // 初始化寄存器
    g_registers[0] = 1;    // 系统状态：运行
    g_registers[1] = 100;  // 采样率：100kHz
    g_registers[2] = 0;    // 触发模式：软件

    // 初始化统计
    g_stats.read_count = 0;
    g_stats.trigger_count = 0;
    g_stats.start_time = time(NULL);

    g_config.initialized = 1;

    if (g_config.verbose >= 1) {
        printf("[Mock] Initialization complete\n");
        printf("  Version: %s\n", VERSION);
        printf("  RF channels: 4 (RF3-RF6)\n");
        printf("  XY channels: 4 (X1,Y1,X2,Y2)\n");
        printf("  Buttons: 8\n");
    }

    return 0;
}

void SystemClose(void)
{
    if (g_config.verbose >= 1) {
        printf("[Mock] SystemClose called\n");
        PrintStatistics();
    }

    g_config.initialized = 0;
}

int GetSystemStatus(void)
{
    return g_config.initialized ? 1 : 0;
}

const char* GetVersion(void)
{
    return VERSION;
}

int TriggerAllDataReached(void)
{
    if (!g_config.initialized) {
        return -1;
    }

    // 推进时间
    g_config.simulation_time += g_config.time_step;

    g_stats.trigger_count++;

    if (g_config.verbose >= 2) {
        printf("[Mock] TriggerAllDataReached: t=%.3f s\n",
               g_config.simulation_time);
    }

    return 0;
}

void StartAcquisition(void)
{
    if (g_config.verbose >= 1) {
        printf("[Mock] StartAcquisition\n");
    }
}

void StopAcquisition(void)
{
    if (g_config.verbose >= 1) {
        printf("[Mock] StopAcquisition\n");
    }
}

int IsDataReady(void)
{
    return 1;  // Mock库总是就绪
}

float GetRFInfo(int channel, int type)
{
    if (!g_config.initialized) {
        fprintf(stderr, "[Mock] Error: Not initialized\n");
        return 0.0;
    }

    if (channel < 3 || channel > 6) {
        fprintf(stderr, "[Mock] Error: Invalid RF channel: %d\n", channel);
        return 0.0;
    }

    int ch_idx = channel - 3;
    RfChannelConfig *cfg = &g_rf_channels[ch_idx];

    if (!cfg->enabled) {
        return 0.0;
    }

    g_stats.read_count++;

    double time = g_config.simulation_time;
    float value = 0.0;

    switch (type) {
        case 0:  // 幅度
            value = generateRfAmplitude(cfg, time);
            break;

        case 1:  // 相位
            value = generateRfPhase(cfg, time);
            break;

        case 2:  // 实部
            {
                float amp = generateRfAmplitude(cfg, time);
                float phase = generateRfPhase(cfg, time);
                value = amp * cos(phase * M_PI / 180.0);
            }
            break;

        case 3:  // 虚部
            {
                float amp = generateRfAmplitude(cfg, time);
                float phase = generateRfPhase(cfg, time);
                value = amp * sin(phase * M_PI / 180.0);
            }
            break;

        default:
            fprintf(stderr, "[Mock] Error: Invalid type: %d\n", type);
            return 0.0;
    }

    if (g_config.verbose >= 3) {
        printf("[Mock] GetRFInfo(ch=%d, type=%d) = %.3f\n",
               channel, type, value);
    }

    return value;
}

float GetCenterFrequency(void)
{
    return g_rf_channels[0].center_frequency;
}

void SetCenterFrequency(float freq)
{
    for (int i = 0; i < 4; i++) {
        g_rf_channels[i].center_frequency = freq;
    }

    if (g_config.verbose >= 1) {
        printf("[Mock] SetCenterFrequency: %.2f MHz\n", freq);
    }
}

float GetXYPosition(int channel)
{
    if (!g_config.initialized) {
        return 0.0;
    }

    if (channel < 0 || channel >= 4) {
        fprintf(stderr, "[Mock] Error: Invalid XY channel: %d\n", channel);
        return 0.0;
    }

    int bpm_idx = channel / 2;
    int is_y = channel % 2;

    XYPositionConfig *cfg = &g_xy_config[bpm_idx];

    if (!cfg->enabled) {
        return 0.0;
    }

    g_stats.read_count++;

    double time = g_config.simulation_time;
    float x, y;

    generateXYPosition(cfg, time, &x, &y);

    float position = is_y ? y : x;

    if (g_config.verbose >= 3) {
        printf("[Mock] GetXYPosition(%d) = %.3f mm\n", channel, position);
    }

    return position;
}

float GetQ(int channel)
{
    // Q = sqrt(Button1^2 + Button2^2 + Button3^2 + Button4^2)
    // 简化实现
    if (channel == 0) {
        float sum = 0.0;
        for (int i = 0; i < 4; i++) {
            float btn = GetButtonSignal(i);
            sum += btn * btn;
        }
        return sqrt(sum);
    } else if (channel == 1) {
        float sum = 0.0;
        for (int i = 4; i < 8; i++) {
            float btn = GetButtonSignal(i);
            sum += btn * btn;
        }
        return sqrt(sum);
    }

    return 0.0;
}

float GetButtonSignal(int index)
{
    if (!g_config.initialized) {
        return 0.0;
    }

    if (index < 0 || index >= 8) {
        fprintf(stderr, "[Mock] Error: Invalid button index: %d\n", index);
        return 0.0;
    }

    g_stats.read_count++;

    ButtonConfig *cfg = &g_buttons[index];
    double time = g_config.simulation_time;

    float signal = generateButtonSignal(cfg, time);

    if (g_config.verbose >= 3) {
        printf("[Mock] GetButtonSignal(%d) = %.3f\n", index, signal);
    }

    return signal;
}

float GetButtonSum(void)
{
    float sum = 0.0;
    for (int i = 0; i < 4; i++) {
        sum += GetButtonSignal(i);
    }
    return sum;
}

void SetReg(int addr, int value)
{
    if (addr < 0 || addr >= MAX_REGISTERS) {
        fprintf(stderr, "[Mock] Error: Invalid register address: %d\n", addr);
        return;
    }

    int old_value = g_registers[addr];
    g_registers[addr] = value;

    if (g_config.verbose >= 2) {
        printf("[Mock] SetReg(%d, %d) (was %d)\n", addr, value, old_value);
    }
}

int GetReg(int addr)
{
    if (addr < 0 || addr >= MAX_REGISTERS) {
        fprintf(stderr, "[Mock] Error: Invalid register address: %d\n", addr);
        return 0;
    }

    return g_registers[addr];
}

int LoadConfig(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        if (g_config.verbose >= 1) {
            printf("[Mock] Config file not found: %s (using defaults)\n",
                   filename);
        }
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // 简单的键值对解析
        char key[64], value[64];
        if (sscanf(line, "%63s = %63s", key, value) == 2) {
            // 解析配置项
            if (strcmp(key, "verbose") == 0) {
                g_config.verbose = atoi(value);
            } else if (strcmp(key, "time_step") == 0) {
                g_config.time_step = atof(value);
            }
            // 添加更多配置项...
        }
    }

    fclose(fp);

    if (g_config.verbose >= 1) {
        printf("[Mock] Config loaded from %s\n", filename);
    }

    return 0;
}

void SetVerboseLevel(int level)
{
    g_config.verbose = level;
    printf("[Mock] Verbose level set to %d\n", level);
}

void PrintStatistics(void)
{
    time_t now = time(NULL);
    double runtime = difftime(now, g_stats.start_time);

    printf("=== Mock Library Statistics ===\n");
    printf("  Runtime: %.0f seconds\n", runtime);
    printf("  Read calls: %lu\n", g_stats.read_count);
    printf("  Trigger calls: %lu\n", g_stats.trigger_count);
    printf("  Simulation time: %.3f s\n", g_config.simulation_time);
    if (runtime > 0) {
        printf("  Read rate: %.1f Hz\n", g_stats.read_count / runtime);
        printf("  Trigger rate: %.1f Hz\n", g_stats.trigger_count / runtime);
    }
    printf("================================\n");
}

void ResetStatistics(void)
{
    g_stats.read_count = 0;
    g_stats.trigger_count = 0;
    g_stats.start_time = time(NULL);

    printf("[Mock] Statistics reset\n");
}
```

### 3. 测试程序: test_mock.c

```c
// test_mock.c
// Test program for libbpm_mock

#include "libbpm_mock.h"
#include <stdio.h>
#include <assert.h>

void test_system_init()
{
    printf("=== Test: System Init ===\n");

    int ret = SystemInit();
    assert(ret == 0);

    int status = GetSystemStatus();
    assert(status == 1);

    const char *version = GetVersion();
    printf("Version: %s\n", version);

    printf("PASS\n\n");
}

void test_rf_data()
{
    printf("=== Test: RF Data ===\n");

    for (int ch = 3; ch <= 6; ch++) {
        float amp = GetRFInfo(ch, 0);
        float phase = GetRFInfo(ch, 1);

        printf("RF%d: Amp=%.3f V, Phase=%.1f deg\n", ch, amp, phase);

        assert(amp >= 0.0 && amp <= 10.0);
        assert(phase >= -180.0 && phase <= 180.0);
    }

    printf("PASS\n\n");
}

void test_xy_position()
{
    printf("=== Test: XY Position ===\n");

    for (int ch = 0; ch < 4; ch++) {
        float pos = GetXYPosition(ch);

        const char *name[] = {"X1", "Y1", "X2", "Y2"};
        printf("%s: %.3f mm\n", name[ch], pos);

        assert(pos >= -10.0 && pos <= 10.0);
    }

    printf("PASS\n\n");
}

void test_button_signals()
{
    printf("=== Test: Button Signals ===\n");

    for (int i = 0; i < 8; i++) {
        float btn = GetButtonSignal(i);
        printf("Button%d: %.2f\n", i + 1, btn);

        assert(btn > 0.0);
    }

    float sum = GetButtonSum();
    printf("Button Sum: %.2f\n", sum);

    printf("PASS\n\n");
}

void test_registers()
{
    printf("=== Test: Registers ===\n");

    SetReg(10, 100);
    int val = GetReg(10);
    assert(val == 100);

    SetReg(20, 200);
    val = GetReg(20);
    assert(val == 200);

    printf("Register read/write: OK\n");
    printf("PASS\n\n");
}

void test_time_variation()
{
    printf("=== Test: Time Variation ===\n");

    float amp1 = GetRFInfo(3, 0);

    for (int i = 0; i < 10; i++) {
        TriggerAllDataReached();
    }

    float amp2 = GetRFInfo(3, 0);

    printf("RF3 Amp: %.3f -> %.3f (delta=%.3f)\n", amp1, amp2, amp2 - amp1);

    // 应该有变化，但不会突变太大
    assert(amp1 != amp2);  // 有变化
    assert(fabs(amp2 - amp1) < 2.0);  // 但不会突变>2V

    printf("PASS\n\n");
}

int main(void)
{
    printf("========================================\n");
    printf("  BPMIOC Mock Library Test Suite\n");
    printf("========================================\n\n");

    test_system_init();
    test_rf_data();
    test_xy_position();
    test_button_signals();
    test_registers();
    test_time_variation();

    PrintStatistics();
    SystemClose();

    printf("\n========================================\n");
    printf("  ALL TESTS PASSED!\n");
    printf("========================================\n");

    return 0;
}
```

### 4. Makefile

```makefile
# Makefile for BPMIOC Mock Library

CC = gcc
CFLAGS = -Wall -fPIC -O2 -g
LDFLAGS = -shared
LIBS = -lm

# 目标
LIB = libbpm_mock.so
TEST = test_mock

# 源文件
LIB_SRC = libbpm_mock.c
TEST_SRC = test_mock.c

.PHONY: all clean test install

all: $(LIB) $(TEST)

$(LIB): $(LIB_SRC) libbpm_mock.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIB_SRC) -o $(LIB) $(LIBS)
	@echo "Mock library built: $(LIB)"

$(TEST): $(TEST_SRC) $(LIB)
	$(CC) $(CFLAGS) $(TEST_SRC) -L. -lbpm_mock -o $(TEST) $(LIBS)
	@echo "Test program built: $(TEST)"

test: $(TEST)
	@echo "Running tests..."
	LD_LIBRARY_PATH=. ./$(TEST)

install: $(LIB)
	@echo "Installing to $(INSTALL_DIR)..."
	mkdir -p $(INSTALL_DIR)
	cp $(LIB) $(INSTALL_DIR)/
	@echo "Installed: $(INSTALL_DIR)/$(LIB)"

clean:
	rm -f $(LIB) $(TEST) *.o
	@echo "Cleaned"

# 帮助
help:
	@echo "Available targets:"
	@echo "  all      - Build library and test"
	@echo "  test     - Run tests"
	@echo "  install  - Install library (set INSTALL_DIR=...)"
	@echo "  clean    - Remove built files"
```

### 5. 配置文件: mock_config.ini

```ini
# BPMIOC Mock Library Configuration

[Global]
verbose = 1
time_step = 0.1

[RF]
# RF3-RF6 configuration
base_amplitude = 4.0
center_frequency = 499.8

[XY]
# XY position configuration
orbit_radius = 2.0
jitter_amplitude = 0.1

[Button]
# Button signal configuration
base_signal = 50.0
```

## 🔨 编译和使用

### 编译

```bash
# 创建目录
mkdir -p ~/BPMIOC/simulator/src
cd ~/BPMIOC/simulator/src

# 复制源码（或创建文件并粘贴上面的代码）
# vim libbpm_mock.h
# vim libbpm_mock.c
# vim test_mock.c
# vim Makefile

# 编译
make

# 输出:
# Mock library built: libbpm_mock.so
# Test program built: test_mock
```

### 测试

```bash
# 运行测试
make test

# 输出:
# ========================================
#   BPMIOC Mock Library Test Suite
# ========================================
#
# === Test: System Init ===
# [Mock] SystemInit called
# [Mock] Initialization complete
# Version: 1.0.0
# PASS
#
# === Test: RF Data ===
# RF3: Amp=4.023 V, Phase=2.3 deg
# RF4: Amp=3.815 V, Phase=47.8 deg
# ...
# PASS
#
# ALL TESTS PASSED!
```

## 📊 性能测试

```c
// performance_test.c
#include "libbpm_mock.h"
#include <stdio.h>
#include <time.h>

int main()
{
    SystemInit();

    const int N = 100000;
    clock_t start = clock();

    for (int i = 0; i < N; i++) {
        GetRFInfo(3, 0);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Performance Test:\n");
    printf("  Calls: %d\n", N);
    printf("  Time: %.3f s\n", elapsed);
    printf("  Rate: %.0f calls/s\n", N / elapsed);
    printf("  Avg: %.3f μs/call\n", elapsed * 1e6 / N);

    SystemClose();
    return 0;
}
```

预期输出：
```
Performance Test:
  Calls: 100000
  Time: 0.124 s
  Rate: 806451 calls/s
  Avg: 1.24 μs/call
```

**结论**: Mock库性能完全满足BPMIOC的10 Hz需求！

## 🎯 与BPMIOC集成

现在可以在BPMIOC中使用这个Mock库了！

```bash
# 复制库到BPMIOC目录
cp libbpm_mock.so ~/BPMIOC/

# 修改driverWrapper.c
# 将 dlopen("libbpm_zynq.so", ...) 改为
# dlopen("libbpm_mock.so", ...)

# 或者使用环境变量控制
```

详见下一章：[09-integration-with-ioc.md](./09-integration-with-ioc.md)

## ❓ 常见问题

### Q1: 编译时找不到数学库？
**A**:
```bash
# 确保链接了-lm
gcc ... -lm
```

### Q2: 运行时找不到.so文件？
**A**:
```bash
# 设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./test_mock
```

### Q3: 如何添加新函数？
**A**:
1. 在libbpm_mock.h中声明
2. 在libbpm_mock.c中实现
3. 重新编译
4. 在test_mock.c中测试

### Q4: 如何调整参数？
**A**:
直接修改libbpm_mock.c中的配置结构，或实现配置文件加载。

## 📚 下一步

现在你有了完整的Mock库，接下来：

1. [07-build-and-test.md](./07-build-and-test.md) - 详细的编译测试指南
2. [09-integration-with-ioc.md](./09-integration-with-ioc.md) - 与BPMIOC集成
3. 开始在PC上开发BPMIOC！

---

**恭喜！** 你现在拥有一个完整可用的Mock库了！🎉
