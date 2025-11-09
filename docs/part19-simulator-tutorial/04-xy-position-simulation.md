# XY位置数据模拟详解

> **阅读时间**: 35分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: 束流物理基础、二维运动、噪声模型

## 📋 本文目标

- 理解束流位置的物理意义
- 掌握轨迹生成算法
- 学会模拟抖动和漂移
- 实现X1, Y1, X2, Y2的完整模拟

## 🎯 XY位置的物理意义

### 什么是束流位置？

```
加速器中的束流在磁场作用下沿着设计轨道运动

    Y ↑
      │      ● ← 束流质心
      │     ╱ ╲
      │    │   │ ← 束流横截面
      │     ╲ ╱
      └──────────→ X
```

**BPM (Beam Position Monitor)** 测量束流质心的XY坐标。

### BPMIOC的XY通道

```
BPM1 (第一个探测器)
├─ X1: 水平位置 (-10mm ~ +10mm)
└─ Y1: 垂直位置 (-10mm ~ +10mm)

BPM2 (第二个探测器)
├─ X2: 水平位置
└─ Y2: 垂直位置

用途:
- 监控束流轨迹
- 束流位置反馈控制
- 轨道校正
```

### 需要模拟的现象

```
真实束流的位置变化:
1. 设计轨道: 圆形或椭圆
2. 束流抖动: 高频小幅度振荡
3. 轨道漂移: 低频缓慢移动
4. 测量噪声: 随机噪声
```

## 📐 数据结构设计

### XY位置配置结构

```c
typedef struct {
    // === 轨道参数 ===
    double orbit_center_x;        // 轨道中心X坐标 (mm)
    double orbit_center_y;        // 轨道中心Y坐标 (mm)
    double orbit_radius_x;        // X方向半径 (mm)
    double orbit_radius_y;        // Y方向半径 (mm)
    double orbit_frequency;       // 轨道频率 (Hz)
    double orbit_phase;           // 轨道相位 (rad)

    // === 抖动参数 ===
    double jitter_amplitude_x;    // X抖动幅度 (mm)
    double jitter_amplitude_y;    // Y抖动幅度 (mm)
    double jitter_frequency;      // 抖动频率 (Hz)
    double jitter_phase_x;        // X抖动相位
    double jitter_phase_y;        // Y抖动相位

    // === 漂移参数 ===
    double drift_rate_x;          // X漂移速率 (mm/s)
    double drift_rate_y;          // Y漂移速率 (mm/s)
    double drift_amplitude_x;     // X漂移幅度 (mm)
    double drift_amplitude_y;     // Y漂移幅度 (mm)
    double drift_frequency;       // 漂移频率 (Hz)

    // === 噪声 ===
    double noise_level;           // 测量噪声 (mm)

    // === 状态 ===
    double current_x;             // 当前X位置 (mm)
    double current_y;             // 当前Y位置 (mm)
    int enabled;                  // 是否启用

} XYPositionConfig;
```

### BPMIOC的两个BPM初始化

```c
static XYPositionConfig g_xy_config[2] = {
    // BPM1 (X1, Y1)
    {
        // 轨道参数
        .orbit_center_x = 0.0,
        .orbit_center_y = 0.0,
        .orbit_radius_x = 2.0,        // ±2mm
        .orbit_radius_y = 1.5,        // ±1.5mm (椭圆轨道)
        .orbit_frequency = 0.5,       // 0.5 Hz (2秒一圈)
        .orbit_phase = 0.0,

        // 抖动参数
        .jitter_amplitude_x = 0.1,    // ±0.1mm
        .jitter_amplitude_y = 0.1,
        .jitter_frequency = 50.0,     // 50 Hz
        .jitter_phase_x = 0.0,
        .jitter_phase_y = M_PI / 2,   // Y相位差90度

        // 漂移参数
        .drift_rate_x = 0.01,         // 0.01 mm/s
        .drift_rate_y = 0.01,
        .drift_amplitude_x = 0.5,     // ±0.5mm
        .drift_amplitude_y = 0.5,
        .drift_frequency = 0.01,      // 0.01 Hz (100秒周期)

        // 噪声
        .noise_level = 0.05,          // ±0.05mm

        // 状态
        .current_x = 0.0,
        .current_y = 0.0,
        .enabled = 1
    },

    // BPM2 (X2, Y2) - 略有不同参数
    {
        .orbit_center_x = 0.5,        // 轨道中心略有偏移
        .orbit_center_y = -0.3,
        .orbit_radius_x = 2.1,
        .orbit_radius_y = 1.6,
        .orbit_frequency = 0.5,
        .orbit_phase = M_PI / 4,      // 相位差45度

        .jitter_amplitude_x = 0.12,
        .jitter_amplitude_y = 0.12,
        .jitter_frequency = 55.0,
        .jitter_phase_x = 0.0,
        .jitter_phase_y = M_PI / 2,

        .drift_rate_x = 0.008,
        .drift_rate_y = 0.012,
        .drift_amplitude_x = 0.4,
        .drift_amplitude_y = 0.6,
        .drift_frequency = 0.012,

        .noise_level = 0.06,

        .current_x = 0.5,
        .current_y = -0.3,
        .enabled = 1
    }
};
```

## 🌀 位置生成算法

### 算法：分层生成

```c
void generateXYPosition(XYPositionConfig *cfg, double time,
                       float *x_out, float *y_out)
{
    double x = 0.0, y = 0.0;

    // ===== Layer 1: 轨道中心 =====
    x += cfg->orbit_center_x;
    y += cfg->orbit_center_y;

    // ===== Layer 2: 主轨道运动（椭圆） =====
    double orbit_phase = 2.0 * M_PI * cfg->orbit_frequency * time + cfg->orbit_phase;

    x += cfg->orbit_radius_x * cos(orbit_phase);
    y += cfg->orbit_radius_y * sin(orbit_phase);

    // ===== Layer 3: 束流抖动（betatron oscillation模拟） =====
    double jitter_phase_x = 2.0 * M_PI * cfg->jitter_frequency * time +
                           cfg->jitter_phase_x;
    double jitter_phase_y = 2.0 * M_PI * cfg->jitter_frequency * time +
                           cfg->jitter_phase_y;

    x += cfg->jitter_amplitude_x * sin(jitter_phase_x);
    y += cfg->jitter_amplitude_y * sin(jitter_phase_y);

    // ===== Layer 4: 轨道漂移（长期变化） =====
    // 组合线性漂移和周期性漂移
    double drift_phase = 2.0 * M_PI * cfg->drift_frequency * time;

    x += cfg->drift_rate_x * time;  // 线性漂移
    x += cfg->drift_amplitude_x * sin(drift_phase);  // 周期性漂移

    y += cfg->drift_rate_y * time;
    y += cfg->drift_amplitude_y * cos(drift_phase);  // Y用cos以形成李萨如图形

    // ===== Layer 5: 测量噪声 =====
    double noise_x = ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level;
    double noise_y = ((double)rand() / RAND_MAX - 0.5) * 2.0 * cfg->noise_level;

    x += noise_x;
    y += noise_y;

    // ===== 限制范围（物理限制） =====
    const double MAX_POSITION = 10.0;  // ±10mm
    if (x > MAX_POSITION) x = MAX_POSITION;
    if (x < -MAX_POSITION) x = -MAX_POSITION;
    if (y > MAX_POSITION) y = MAX_POSITION;
    if (y < -MAX_POSITION) y = -MAX_POSITION;

    // ===== 平滑处理 =====
    const float alpha = 0.9;
    x = alpha * cfg->current_x + (1.0 - alpha) * x;
    y = alpha * cfg->current_y + (1.0 - alpha) * y;

    // ===== 更新状态 =====
    cfg->current_x = x;
    cfg->current_y = y;

    *x_out = (float)x;
    *y_out = (float)y;
}
```

## 🎯 GetXYPosition()实现

```c
float GetXYPosition(int channel)
{
    // channel: 0=X1, 1=Y1, 2=X2, 3=Y2

    // ===== 参数验证 =====
    if (!g_config.initialized) {
        fprintf(stderr, "[Mock] Error: Not initialized\n");
        return 0.0;
    }

    if (channel < 0 || channel >= 4) {
        fprintf(stderr, "[Mock] Error: Invalid XY channel: %d\n", channel);
        return 0.0;
    }

    // ===== 确定BPM和坐标 =====
    int bpm_idx = channel / 2;  // 0,1→BPM1; 2,3→BPM2
    int is_y = channel % 2;     // 0→X; 1→Y

    XYPositionConfig *cfg = &g_xy_config[bpm_idx];

    if (!cfg->enabled) {
        return 0.0;
    }

    // ===== 生成XY位置 =====
    double time = g_config.simulation_time;
    float x, y;

    generateXYPosition(cfg, time, &x, &y);

    // ===== 返回对应坐标 =====
    float position = is_y ? y : x;

    // ===== 调试输出 =====
    if (g_config.verbose >= 3) {
        const char *coord = is_y ? "Y" : "X";
        int bpm_num = bpm_idx + 1;
        printf("[Mock] Get%s%d = %.3f mm\n", coord, bpm_num, position);
    }

    return position;
}
```

## 🎨 物理模型和变体

### 变体1：圆形轨道

```c
// 简单的圆形轨道
void generateCircularOrbit(double time, float *x, float *y)
{
    const double radius = 2.0;
    const double freq = 0.5;

    double angle = 2.0 * M_PI * freq * time;

    *x = radius * cos(angle);
    *y = radius * sin(angle);
}
```

### 变体2：李萨如图形

```c
// 两个频率不同的正弦波组成的李萨如图形
void generateLissajous(double time, float *x, float *y)
{
    const double amp_x = 2.0;
    const double amp_y = 1.5;
    const double freq_x = 0.5;
    const double freq_y = 0.7;  // 频率比不是整数

    *x = amp_x * sin(2.0 * M_PI * freq_x * time);
    *y = amp_y * sin(2.0 * M_PI * freq_y * time + M_PI / 2);
}
```

### 变体3：阻尼振荡

```c
// 模拟束流注入后的阻尼过程
void generateDampedOscillation(double time, float *x, float *y)
{
    const double init_amp_x = 5.0;  // 初始振幅
    const double init_amp_y = 4.0;
    const double freq = 2.0;        // 振荡频率
    const double damping = 0.5;     // 阻尼系数

    double amp_x = init_amp_x * exp(-damping * time);
    double amp_y = init_amp_y * exp(-damping * time);

    *x = amp_x * sin(2.0 * M_PI * freq * time);
    *y = amp_y * cos(2.0 * M_PI * freq * time);
}
```

### 变体4：随机游走

```c
// Brownian motion / 随机游走
static double walk_x = 0.0;
static double walk_y = 0.0;

void generateRandomWalk(double dt, float *x, float *y)
{
    const double step_size = 0.1;  // mm

    // 随机步长
    double dx = ((double)rand() / RAND_MAX - 0.5) * 2.0 * step_size;
    double dy = ((double)rand() / RAND_MAX - 0.5) * 2.0 * step_size;

    walk_x += dx;
    walk_y += dy;

    // 添加恢复力（防止走太远）
    walk_x *= 0.99;
    walk_y *= 0.99;

    *x = walk_x;
    *y = walk_y;
}
```

## 🔧 高级功能

### 功能1：束流损失模拟

```c
static int beam_present = 1;

void simulateBeamLoss(double time)
{
    // 模拟束流在某个时刻丢失
    const double loss_time = 30.0;  // 30秒时丢失
    const double restore_time = 35.0;  // 35秒时恢复

    if (time > loss_time && time < restore_time) {
        beam_present = 0;
    } else {
        beam_present = 1;
    }
}

float GetXYPosition(int channel)
{
    // ...

    if (!beam_present) {
        return 0.0;  // 无束流时返回0
    }

    // ... 正常位置生成
}
```

### 功能2：两个BPM的相关性

```c
// BPM2的位置应该和BPM1相关（束流是连续的）
void generateCorrelatedPosition(double time)
{
    float x1, y1, x2, y2;

    // 生成BPM1位置
    generateXYPosition(&g_xy_config[0], time, &x1, &y1);

    // BPM2位置 = BPM1位置 + 微小差异
    x2 = x1 + 0.5 * sin(2.0 * M_PI * 0.3 * time);
    y2 = y1 + 0.3 * cos(2.0 * M_PI * 0.4 * time);

    g_xy_config[0].current_x = x1;
    g_xy_config[0].current_y = y1;
    g_xy_config[1].current_x = x2;
    g_xy_config[1].current_y = y2;
}
```

### 功能3：故障注入

```c
typedef enum {
    XY_FAULT_NONE = 0,
    XY_FAULT_OFFSET,      // 位置偏移
    XY_FAULT_STUCK,       // 位置卡死
    XY_FAULT_NOISE_BURST, // 噪声突发
    XY_FAULT_BEAM_LOSS    // 束流丢失
} XYFaultType;

static XYFaultType g_xy_fault = XY_FAULT_NONE;

void InjectXYFault(XYFaultType fault)
{
    g_xy_fault = fault;
}

void applyFault(float *x, float *y)
{
    switch (g_xy_fault) {
        case XY_FAULT_OFFSET:
            *x += 5.0;  // 5mm偏移
            *y += 3.0;
            break;

        case XY_FAULT_STUCK:
            // 位置不变
            static float stuck_x = 0.0, stuck_y = 0.0;
            *x = stuck_x;
            *y = stuck_y;
            break;

        case XY_FAULT_NOISE_BURST:
            *x += ((double)rand() / RAND_MAX - 0.5) * 4.0;  // ±2mm大噪声
            *y += ((double)rand() / RAND_MAX - 0.5) * 4.0;
            break;

        case XY_FAULT_BEAM_LOSS:
            *x = 0.0;
            *y = 0.0;
            break;

        default:
            break;
    }
}
```

## 📊 测试和验证

### 测试1：轨道形状

```c
void test_orbit_shape()
{
    printf("=== Test: Orbit Shape ===\n");

    FILE *fp = fopen("orbit.csv", "w");
    fprintf(fp, "X,Y\n");

    SystemInit();

    for (int i = 0; i < 200; i++) {
        float x1 = GetXYPosition(0);  // X1
        float y1 = GetXYPosition(1);  // Y1

        fprintf(fp, "%.3f,%.3f\n", x1, y1);

        TriggerAllDataReached();
    }

    fclose(fp);
    printf("Orbit data saved to orbit.csv\n");
    printf("Use: plot 'orbit.csv' using 1:2 with lines\n");
}
```

### 测试2：抖动频率

```c
void test_jitter_frequency()
{
    printf("=== Test: Jitter Frequency ===\n");

    const int N = 1000;
    float samples[N];

    SystemInit();

    // 采集数据
    for (int i = 0; i < N; i++) {
        samples[i] = GetXYPosition(0);
        TriggerAllDataReached();
    }

    // 简单FFT分析（伪代码）
    // analyze_frequency_spectrum(samples, N);

    printf("Check for peak at jitter_frequency\n");
}
```

### 测试3：范围检查

```c
void test_position_range()
{
    printf("=== Test: Position Range ===\n");

    SystemInit();

    float min_x = 10.0, max_x = -10.0;
    float min_y = 10.0, max_y = -10.0;

    for (int i = 0; i < 10000; i++) {
        float x = GetXYPosition(0);
        float y = GetXYPosition(1);

        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;

        TriggerAllDataReached();
    }

    printf("X range: [%.2f, %.2f] mm\n", min_x, max_x);
    printf("Y range: [%.2f, %.2f] mm\n", min_y, max_y);

    // 验证在合理范围内
    assert(min_x >= -10.0 && max_x <= 10.0);
    assert(min_y >= -10.0 && max_y <= 10.0);

    printf("PASS\n");
}
```

## 🎨 可视化

### Python绘制轨迹

```python
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 读取数据
data = pd.read_csv('orbit.csv')

# 创建图形
fig, axes = plt.subplots(2, 2, figsize=(12, 10))

# 1. XY轨迹图
ax = axes[0, 0]
ax.plot(data['X'], data['Y'], 'b-', linewidth=0.5)
ax.plot(data['X'].iloc[0], data['Y'].iloc[0], 'go', label='Start')
ax.plot(data['X'].iloc[-1], data['Y'].iloc[-1], 'ro', label='End')
ax.set_xlabel('X (mm)')
ax.set_ylabel('Y (mm)')
ax.set_title('Beam Orbit Trajectory')
ax.axis('equal')
ax.grid(True)
ax.legend()

# 2. X vs Time
ax = axes[0, 1]
time = np.arange(len(data)) * 0.1  # 0.1s间隔
ax.plot(time, data['X'], 'b-')
ax.set_xlabel('Time (s)')
ax.set_ylabel('X Position (mm)')
ax.set_title('X Position vs Time')
ax.grid(True)

# 3. Y vs Time
ax = axes[1, 0]
ax.plot(time, data['Y'], 'r-')
ax.set_xlabel('Time (s)')
ax.set_ylabel('Y Position (mm)')
ax.set_title('Y Position vs Time')
ax.grid(True)

# 4. Radius vs Time
ax = axes[1, 1]
radius = np.sqrt(data['X']**2 + data['Y']**2)
ax.plot(time, radius, 'g-')
ax.set_xlabel('Time (s)')
ax.set_ylabel('Radial Position (mm)')
ax.set_title('Radial Distance vs Time')
ax.grid(True)

plt.tight_layout()
plt.savefig('xy_simulation.png', dpi=150)
plt.show()
```

## ❓ 常见问题

### Q1: 轨道半径多大合适？
**A**:
- 典型值: 1-3mm（根据你的加速器设计）
- 太小: 看不出轨道形状
- 太大: 可能超出±10mm物理限制

### Q2: 抖动频率应该是多少？
**A**:
- Betatron频率: 通常几十Hz
- BPMIOC采样10Hz，抖动频率应<5Hz才能正确采样
- 或者用50Hz模拟欠采样效果

### Q3: 如何模拟真实的束流轨迹？
**A**:
1. 如果有真实数据，从文件回放
2. 否则用椭圆轨道+抖动+漂移已经很真实
3. 添加BPM间的相关性

### Q4: 两个BPM的位置应该有关系吗？
**A**:
是的！它们测量的是同一束流，应该有相关性。但可以有小的差异（束流在BPM间的轨迹变化）。

## 📚 下一步

位置模拟掌握后，接下来：

1. [05-complete-mock-implementation.md](./05-complete-mock-implementation.md) - 完整Mock库
2. 实现Button信号模拟
3. 集成所有组件

---

**记住**: 好的轨迹模拟 = 物理模型 + 多层变化 + 合理相关性 🎯
