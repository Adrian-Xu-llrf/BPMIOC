# BPMIOC Mock库架构设计

> **阅读时间**: 45分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: [01-how-to-write-simulator.md](./01-how-to-write-simulator.md), [Part 4: 硬件函数](../part4-driver-layer/10-hardware-functions.md)

## 📋 本文目标

- 理解BPMIOC Mock库的整体架构
- 掌握50+硬件函数的分类和组织
- 学会设计大型模拟器的数据结构
- 了解状态管理和线程安全

## 🏗️ 整体架构

### 架构图

```
┌─────────────────────────────────────────────────────────┐
│              libbpm_mock.so (Mock库)                     │
│                                                          │
│  ┌────────────────────────────────────────────────────┐ │
│  │          公共接口层 (50+函数)                       │ │
│  │  SystemInit(), GetRFInfo(), GetXYPosition(), ...   │ │
│  └────────────────┬───────────────────────────────────┘ │
│                   ↓                                      │
│  ┌────────────────────────────────────────────────────┐ │
│  │          数据生成层                                 │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐         │ │
│  │  │ RF生成器  │  │ XY生成器  │  │Button生成 │         │ │
│  │  └──────────┘  └──────────┘  └──────────┘         │ │
│  └────────────────┬───────────────────────────────────┘ │
│                   ↓                                      │
│  ┌────────────────────────────────────────────────────┐ │
│  │          状态管理层                                 │ │
│  │  • 全局配置                                         │ │
│  │  • 时间管理                                         │ │
│  │  • 寄存器状态                                       │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 设计哲学

1. **分层设计**: 接口层、数据生成层、状态管理层分离
2. **模块化**: 每种数据类型独立模块（RF、XY、Button）
3. **可配置**: 通过配置文件或函数调整行为
4. **可测试**: 每个模块可独立测试

## 📂 文件结构

```c
// libbpm_mock.c - 主文件 (~1500行)
// ├─ 包含和宏定义 (1-100行)
// ├─ 数据结构定义 (101-200行)
// ├─ 全局变量 (201-300行)
// ├─ 辅助函数 (301-500行)
// ├─ 数据生成函数 (501-1000行)
// │  ├─ RF生成 (501-700行)
// │  ├─ XY生成 (701-850行)
// │  └─ Button生成 (851-1000行)
// ├─ 公共接口实现 (1001-1400行)
// └─ 初始化和清理 (1401-1500行)
```

## 🗂️ 数据结构设计

### 1. 全局配置结构

```c
// Mock库的核心配置
typedef struct {
    int initialized;              // 是否已初始化
    double simulation_time;       // 模拟时间（秒）
    double time_step;             // 时间步长（秒）
    int num_rf_channels;          // RF通道数（默认4：RF3-RF6）
    int num_xy_channels;          // XY通道数（默认4：X1,Y1,X2,Y2）
    int num_buttons;              // Button数量（默认8）
    int verbose;                  // 调试输出级别（0-3）
} MockConfig;

static MockConfig g_config = {
    .initialized = 0,
    .simulation_time = 0.0,
    .time_step = 0.1,        // 100ms
    .num_rf_channels = 4,
    .num_xy_channels = 4,
    .num_buttons = 8,
    .verbose = 1
};
```

### 2. RF通道配置

```c
// 每个RF通道的独立配置
typedef struct {
    // 基础参数
    double base_amplitude;        // 基础幅度（V）
    double base_phase;            // 基础相位（度）
    double center_frequency;      // 中心频率（MHz）

    // 变化参数
    double amp_variation_freq;    // 幅度变化频率（Hz）
    double amp_variation_depth;   // 幅度变化深度（%）
    double phase_drift_rate;      // 相位漂移速率（度/秒）

    // 噪声参数
    double amp_noise_level;       // 幅度噪声（%）
    double phase_noise_level;     // 相位噪声（度）

    // 状态
    int enabled;                  // 是否启用
} RfChannelConfig;

// RF3, RF4, RF5, RF6的配置
static RfChannelConfig g_rf_channels[4] = {
    // RF3 (channel 3)
    {
        .base_amplitude = 4.0,
        .base_phase = 0.0,
        .center_frequency = 499.8,
        .amp_variation_freq = 0.5,
        .amp_variation_depth = 0.1,
        .phase_drift_rate = 10.0,
        .amp_noise_level = 0.02,
        .phase_noise_level = 2.0,
        .enabled = 1
    },
    // RF4, RF5, RF6类似...
};
```

### 3. XY位置配置

```c
// XY位置的运动模拟
typedef struct {
    // 轨道参数
    double orbit_radius_x;        // X方向轨道半径（mm）
    double orbit_radius_y;        // Y方向轨道半径（mm）
    double orbit_frequency;       // 轨道频率（Hz）

    // 抖动参数
    double jitter_amplitude_x;    // X方向抖动幅度（mm）
    double jitter_amplitude_y;    // Y方向抖动幅度（mm）
    double jitter_frequency;      // 抖动频率（Hz）

    // 漂移参数
    double drift_rate_x;          // X方向漂移速率（mm/s）
    double drift_rate_y;          // Y方向漂移速率（mm/s）

    // 噪声
    double noise_level;           // 位置噪声（mm）

    // 状态
    double current_x;             // 当前X位置
    double current_y;             // 当前Y位置
} XYPositionConfig;

static XYPositionConfig g_xy_config[2] = {  // BPM1和BPM2
    // BPM1 (X1, Y1)
    {
        .orbit_radius_x = 2.0,
        .orbit_radius_y = 1.5,
        .orbit_frequency = 0.5,
        .jitter_amplitude_x = 0.1,
        .jitter_amplitude_y = 0.1,
        .jitter_frequency = 10.0,
        .drift_rate_x = 0.01,
        .drift_rate_y = 0.01,
        .noise_level = 0.05,
        .current_x = 0.0,
        .current_y = 0.0
    },
    // BPM2类似...
};
```

### 4. Button信号配置

```c
// Button信号（BPM的4个电极信号）
typedef struct {
    double base_signal;           // 基础信号强度
    double variation_freq;        // 变化频率
    double variation_depth;       // 变化深度
    double noise_level;           // 噪声水平
    double phase_offset;          // 相位偏移（用于4个button的差异）
} ButtonConfig;

static ButtonConfig g_buttons[8] = {
    // Button1-4是BPM1的4个电极
    {.base_signal = 50.0, .variation_freq = 0.5, .variation_depth = 0.1,
     .noise_level = 0.02, .phase_offset = 0.0},
    {.base_signal = 48.0, .variation_freq = 0.5, .variation_depth = 0.1,
     .noise_level = 0.02, .phase_offset = M_PI/2},
    {.base_signal = 52.0, .variation_freq = 0.5, .variation_depth = 0.1,
     .noise_level = 0.02, .phase_offset = M_PI},
    {.base_signal = 49.0, .variation_freq = 0.5, .variation_depth = 0.1,
     .noise_level = 0.02, .phase_offset = 3*M_PI/2},
    // Button5-8是BPM2的4个电极...
};
```

### 5. 寄存器状态

```c
// 模拟硬件寄存器（100个）
static int g_registers[100] = {0};

// 寄存器的语义定义
#define REG_SYSTEM_STATUS   0    // 系统状态
#define REG_SAMPLING_RATE   1    // 采样率
#define REG_TRIGGER_MODE    2    // 触发模式
#define REG_RF3_GAIN        10   // RF3增益
#define REG_RF4_GAIN        11   // RF4增益
// ... 更多定义
```

## 🔧 核心函数实现

### 1. SystemInit() - 系统初始化

```c
int SystemInit(void)
{
    printf("[Mock] SystemInit called\n");

    if (g_config.initialized) {
        printf("[Mock] Already initialized\n");
        return 0;
    }

    // 1. 初始化随机数
    srand(time(NULL));

    // 2. 重置时间
    g_config.simulation_time = 0.0;

    // 3. 初始化寄存器默认值
    g_registers[REG_SYSTEM_STATUS] = 1;  // 运行
    g_registers[REG_SAMPLING_RATE] = 100;  // 100kHz
    g_registers[REG_TRIGGER_MODE] = 0;   // 软件触发

    // 4. 初始化RF通道
    for (int ch = 0; ch < 4; ch++) {
        g_rf_channels[ch].enabled = 1;
    }

    // 5. 初始化XY位置
    for (int bpm = 0; bpm < 2; bpm++) {
        g_xy_config[bpm].current_x = 0.0;
        g_xy_config[bpm].current_y = 0.0;
    }

    // 6. 加载配置文件（如果存在）
    loadConfigFile("mock_config.ini");

    g_config.initialized = 1;

    printf("[Mock] Initialization complete\n");
    printf("  RF channels: %d\n", g_config.num_rf_channels);
    printf("  XY channels: %d\n", g_config.num_xy_channels);
    printf("  Buttons: %d\n", g_config.num_buttons);

    return 0;
}
```

### 2. GetRFInfo() - RF数据获取

```c
float GetRFInfo(int channel, int type)
{
    // channel: 3-6 (RF3-RF6)
    // type: 0=幅度, 1=相位

    if (!g_config.initialized) {
        fprintf(stderr, "[Mock] Error: Not initialized\n");
        return 0.0;
    }

    // 转换channel (3-6) 到索引 (0-3)
    int ch_idx = channel - 3;
    if (ch_idx < 0 || ch_idx >= 4) {
        fprintf(stderr, "[Mock] Error: Invalid RF channel: %d\n", channel);
        return 0.0;
    }

    RfChannelConfig *cfg = &g_rf_channels[ch_idx];

    if (!cfg->enabled) {
        return 0.0;
    }

    double t = g_config.simulation_time;
    float value;

    if (type == 0) {  // 幅度
        // 基础幅度
        value = cfg->base_amplitude;

        // 添加慢速变化（模拟功率波动）
        value *= (1.0 + cfg->amp_variation_depth *
                  sin(2.0 * M_PI * cfg->amp_variation_freq * t));

        // 添加噪声
        double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                       cfg->amp_noise_level * value;
        value += noise;

    } else if (type == 1) {  // 相位
        // 基础相位
        value = cfg->base_phase;

        // 添加漂移
        value += cfg->phase_drift_rate * t;

        // 添加慢速变化
        value += 90.0 * sin(2.0 * M_PI * 0.1 * t);

        // 添加噪声
        double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                       cfg->phase_noise_level;
        value += noise;

        // 限制在-180到180度
        while (value > 180.0) value -= 360.0;
        while (value < -180.0) value += 360.0;

    } else {
        fprintf(stderr, "[Mock] Error: Invalid type: %d\n", type);
        return 0.0;
    }

    if (g_config.verbose >= 3) {
        printf("[Mock] GetRFInfo(ch=%d, type=%d) = %.3f\n",
               channel, type, value);
    }

    return value;
}
```

### 3. GetXYPosition() - 位置数据获取

```c
float GetXYPosition(int channel)
{
    // channel: 0=X1, 1=Y1, 2=X2, 3=Y2

    if (!g_config.initialized) {
        return 0.0;
    }

    if (channel < 0 || channel >= 4) {
        fprintf(stderr, "[Mock] Error: Invalid XY channel: %d\n", channel);
        return 0.0;
    }

    int bpm_idx = channel / 2;  // 0-1 → BPM1, 2-3 → BPM2
    int is_y = channel % 2;     // 0=X, 1=Y

    XYPositionConfig *cfg = &g_xy_config[bpm_idx];
    double t = g_config.simulation_time;

    double position;

    if (is_y == 0) {  // X位置
        // 主轨道运动
        position = cfg->orbit_radius_x * sin(2.0 * M_PI * cfg->orbit_frequency * t);

        // 添加抖动
        position += cfg->jitter_amplitude_x *
                    sin(2.0 * M_PI * cfg->jitter_frequency * t);

        // 添加漂移
        position += cfg->drift_rate_x * t;

        // 添加噪声
        position += ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level;

    } else {  // Y位置
        // 主轨道运动（相位差90度）
        position = cfg->orbit_radius_y *
                   cos(2.0 * M_PI * cfg->orbit_frequency * t);

        // 添加抖动
        position += cfg->jitter_amplitude_y *
                    sin(2.0 * M_PI * cfg->jitter_frequency * t + M_PI/4);

        // 添加漂移
        position += cfg->drift_rate_y * t;

        // 添加噪声
        position += ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level;
    }

    // 更新当前位置
    if (is_y == 0) {
        cfg->current_x = position;
    } else {
        cfg->current_y = position;
    }

    if (g_config.verbose >= 3) {
        printf("[Mock] GetXYPosition(ch=%d) = %.3f mm\n", channel, position);
    }

    return position;
}
```

### 4. GetButtonSignal() - Button信号获取

```c
float GetButtonSignal(int index)
{
    // index: 0-7 (8个button)

    if (!g_config.initialized) {
        return 0.0;
    }

    if (index < 0 || index >= 8) {
        fprintf(stderr, "[Mock] Error: Invalid button index: %d\n", index);
        return 0.0;
    }

    ButtonConfig *cfg = &g_buttons[index];
    double t = g_config.simulation_time;

    // 基础信号
    float signal = cfg->base_signal;

    // 添加变化（包含相位偏移）
    signal *= (1.0 + cfg->variation_depth *
               sin(2.0 * M_PI * cfg->variation_freq * t + cfg->phase_offset));

    // 添加噪声
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->noise_level * signal;
    signal += noise;

    if (g_config.verbose >= 3) {
        printf("[Mock] GetButtonSignal(idx=%d) = %.3f\n", index, signal);
    }

    return signal;
}
```

### 5. TriggerAllDataReached() - 触发数据更新

```c
int TriggerAllDataReached(void)
{
    if (!g_config.initialized) {
        return -1;
    }

    // 推进模拟时间
    g_config.simulation_time += g_config.time_step;

    if (g_config.verbose >= 2) {
        printf("[Mock] TriggerAllDataReached: t=%.3f s\n",
               g_config.simulation_time);
    }

    // 这里不需要实际生成数据
    // 数据在GetRFInfo()、GetXYPosition()等函数被调用时实时生成

    return 0;
}
```

### 6. SetReg() / GetReg() - 寄存器操作

```c
void SetReg(int addr, int value)
{
    if (addr < 0 || addr >= 100) {
        fprintf(stderr, "[Mock] Error: Invalid register address: %d\n", addr);
        return;
    }

    int old_value = g_registers[addr];
    g_registers[addr] = value;

    if (g_config.verbose >= 2) {
        printf("[Mock] SetReg(%d, %d) (was %d)\n", addr, value, old_value);
    }

    // 某些寄存器有副作用
    switch (addr) {
        case REG_SAMPLING_RATE:
            // 采样率改变，可能需要调整时间步长
            printf("[Mock] Sampling rate changed to %d kHz\n", value);
            break;

        case REG_TRIGGER_MODE:
            printf("[Mock] Trigger mode changed to %d\n", value);
            break;

        default:
            break;
    }
}

int GetReg(int addr)
{
    if (addr < 0 || addr >= 100) {
        fprintf(stderr, "[Mock] Error: Invalid register address: %d\n", addr);
        return 0;
    }

    return g_registers[addr];
}
```

## 🎨 设计模式和技巧

### 1. 分层生成 (Layered Generation)

```c
// 分层生成更真实的数据

float generateRfAmplitude(RfChannelConfig *cfg, double time)
{
    // Layer 1: 基础值
    float value = cfg->base_amplitude;

    // Layer 2: 慢速变化（功率波动）
    value *= (1.0 + cfg->amp_variation_depth *
              sin(2.0 * M_PI * cfg->amp_variation_freq * time));

    // Layer 3: 中速变化（模拟调制）
    value *= (1.0 + 0.05 * sin(2.0 * M_PI * 5.0 * time));

    // Layer 4: 噪声
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 *
                   cfg->amp_noise_level * value;
    value += noise;

    return value;
}
```

### 2. 状态管理

```c
// 使用静态变量保持状态

float generatePositionWithMemory(double time)
{
    static double last_position = 0.0;
    static double velocity = 0.0;

    // 使用物理模型（加速度 → 速度 → 位置）
    double accel = -0.1 * last_position;  // 弹簧力
    velocity += accel * 0.1;  // 更新速度
    last_position += velocity * 0.1;  // 更新位置

    return last_position;
}
```

### 3. 可配置的数据生成模式

```c
typedef enum {
    GEN_MODE_SINE,       // 正弦波
    GEN_MODE_RANDOM,     // 随机
    GEN_MODE_CONSTANT,   // 恒定
    GEN_MODE_FILE        // 从文件读取
} GenMode;

static GenMode g_gen_mode = GEN_MODE_SINE;

void SetGenerationMode(GenMode mode)
{
    g_gen_mode = mode;
}

float generateData(double time)
{
    switch (g_gen_mode) {
        case GEN_MODE_SINE:
            return sin(2.0 * M_PI * time);

        case GEN_MODE_RANDOM:
            return (double)rand() / RAND_MAX;

        case GEN_MODE_CONSTANT:
            return 1.0;

        case GEN_MODE_FILE:
            return readFromFile(time);

        default:
            return 0.0;
    }
}
```

## 📊 性能考虑

### 计算复杂度

```c
// 每次GetRFInfo()调用的操作数
// - 几次浮点数乘法和加法
// - 1-2次sin()调用
// - 1次随机数生成
// 总计: < 1μs（现代CPU）

// BPMIOC需求: 10 Hz更新，50个PV
// 总CPU时间: 50 × 1μs × 10 Hz = 0.5 ms/s = 0.05% CPU
// 结论: 性能完全不是问题
```

### 优化技巧

```c
// 如果需要优化（通常不需要）：

// 1. 预计算sin表
static float sin_table[360];

void init_sin_table()
{
    for (int i = 0; i < 360; i++) {
        sin_table[i] = sin(i * M_PI / 180.0);
    }
}

float fast_sin(float angle_deg)
{
    int idx = ((int)angle_deg) % 360;
    return sin_table[idx];
}

// 2. 缓存结果（如果同一时刻多次调用）
static float cached_value;
static double cached_time = -1.0;

float getCachedData(double time)
{
    if (time != cached_time) {
        cached_value = expensiveCalculation(time);
        cached_time = time;
    }
    return cached_value;
}
```

## ❓ 常见问题

### Q1: Mock库需要多复杂？
**A**:
- 基本版本：固定值 + 简单正弦波（30分钟）
- 实用版本：多层变化 + 噪声（2小时）
- 完整版本：配置文件 + 故障注入（4小时）

从简单开始，逐步完善！

### Q2: 如何保证数据真实感？
**A**:
1. 正确的数值范围（查阅物理参数）
2. 合理的变化速度（不要太快或太慢）
3. 适量的噪声（2-5%）
4. 正确的相关性（如4个button的相位关系）

### Q3: 是否需要线程安全？
**A**:
Mock库通常单线程使用（在InitDevice()和pthread中调用），不需要加锁。但可以添加检查：

```c
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

float GetRFInfo(int channel, int type)
{
    pthread_mutex_lock(&g_lock);
    float value = /* 计算 */;
    pthread_mutex_unlock(&g_lock);
    return value;
}
```

## 📚 下一步

现在你理解了Mock库的架构，接下来：

1. [03-rf-data-simulation.md](./03-rf-data-simulation.md) - 深入RF数据生成
2. [04-xy-position-simulation.md](./04-xy-position-simulation.md) - XY位置模拟详解
3. [05-complete-mock-implementation.md](./05-complete-mock-implementation.md) - 完整实现

---

**记住**: 好的架构 = 清晰的分层 + 合理的数据结构 + 简单的接口 🎯
