# 辅助函数详解

> **阅读时间**: 25分钟
> **难度**: ⭐⭐⭐☆☆
> **前置知识**: C语言基础函数

## 📋 本文目标

- 理解driverWrapper.c中的辅助函数
- 掌握常用的工具函数
- 学会编写自己的辅助函数

## 🎯 辅助函数概览

### 定位

```
driverWrapper.c
├─ 核心函数 (60%)
│  ├─ InitDevice()
│  ├─ ReadData()
│  ├─ ReadWaveform()
│  └─ SetReg()
│
└─ 辅助函数 (40%)
   ├─ 初始化辅助
   ├─ 数据处理辅助
   ├─ 调试辅助
   └─ 工具辅助
```

**辅助函数的作用**:
- 代码复用
- 提高可读性
- 简化维护
- 方便调试

## 1. 初始化辅助函数

### 1.1 initAllBuffers() - 初始化所有缓冲区

```c
// driverWrapper.c line 1401-1450

static void initAllBuffers(void)
{
    printf("Initializing all buffers...\n");

    // ===== RF波形缓冲区 =====
    memset(rf3amp, 0, sizeof(rf3amp));
    memset(rf3phase, 0, sizeof(rf3phase));
    memset(rf4amp, 0, sizeof(rf4amp));
    memset(rf4phase, 0, sizeof(rf4phase));
    memset(rf5amp, 0, sizeof(rf5amp));
    memset(rf5phase, 0, sizeof(rf5phase));
    memset(rf6amp, 0, sizeof(rf6amp));
    memset(rf6phase, 0, sizeof(rf6phase));

    // ===== XY位置缓冲区 =====
    memset(wave_X1, 0, sizeof(wave_X1));
    memset(wave_Y1, 0, sizeof(wave_Y1));
    memset(wave_X2, 0, sizeof(wave_X2));
    memset(wave_Y2, 0, sizeof(wave_Y2));

    // ===== Button缓冲区 =====
    memset(wave_button1, 0, sizeof(wave_button1));
    memset(wave_button2, 0, sizeof(wave_button2));
    memset(wave_button3, 0, sizeof(wave_button3));
    memset(wave_button4, 0, sizeof(wave_button4));
    memset(wave_button5, 0, sizeof(wave_button5));
    memset(wave_button6, 0, sizeof(wave_button6));
    memset(wave_button7, 0, sizeof(wave_button7));
    memset(wave_button8, 0, sizeof(wave_button8));

    // ===== 历史缓冲区 =====
    memset(HistoryX1, 0, sizeof(HistoryX1));
    memset(HistoryY1, 0, sizeof(HistoryY1));
    memset(HistoryX2, 0, sizeof(HistoryX2));
    memset(HistoryY2, 0, sizeof(HistoryY2));

    // ===== 寄存器缓冲区 =====
    memset(Reg, 0, sizeof(Reg));

    printf("All buffers initialized (total: ~2.4 MB)\n");
}
```

**何时调用**: InitDevice()的第一步

### 1.2 initDefaultRegisters() - 初始化默认寄存器值

```c
static void initDefaultRegisters(void)
{
    printf("Setting default register values...\n");

    // 系统配置
    Reg[0] = 1;      // 系统运行
    Reg[1] = 100;    // 采样率 100kHz
    Reg[2] = 0;      // 触发模式: 软件触发

    // RF增益
    Reg[10] = 50;    // RF3增益
    Reg[11] = 50;    // RF4增益
    Reg[12] = 50;    // RF5增益
    Reg[13] = 50;    // RF6增益

    // XY校准
    Reg[20] = 0;     // X偏移
    Reg[21] = 0;     // Y偏移
    Reg[22] = 1000;  // X比例因子 (1.000)
    Reg[23] = 1000;  // Y比例因子

    // 触发和门控
    Reg[40] = 100;   // 触发延迟 (ns)
    Reg[41] = 0;     // 门控开始
    Reg[42] = 10000; // 门控结束

    printf("Default registers set\n");
}
```

## 2. 数据处理辅助函数

### 2.1 copyWaveData() - 拷贝波形数据

```c
static void copyWaveData(float *dest, const float *src, int len)
{
    if (dest == NULL || src == NULL) {
        fprintf(stderr, "ERROR: NULL pointer in copyWaveData\n");
        return;
    }

    memcpy(dest, src, len * sizeof(float));
}
```

**用途**: 封装memcpy，添加参数检查

**使用示例**:
```c
// 在ReadWaveform()中
case 0:
    copyWaveData(buf, rf3amp, buf_len);
    *len = buf_len;
    break;
```

### 2.2 calculateAverage() - 计算平均值

```c
static float calculateAverage(const float *data, int len)
{
    if (data == NULL || len == 0) {
        return 0.0;
    }

    float sum = 0.0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }

    return sum / len;
}
```

**用途**: 计算波形的平均值

**使用示例**:
```c
float avg_amp = calculateAverage(rf3amp, buf_len);
printf("Average RF3 amplitude: %.3f V\n", avg_amp);
```

### 2.3 calculateRMS() - 计算均方根

```c
static float calculateRMS(const float *data, int len)
{
    if (data == NULL || len == 0) {
        return 0.0;
    }

    float sum_sq = 0.0;
    for (int i = 0; i < len; i++) {
        sum_sq += data[i] * data[i];
    }

    return sqrt(sum_sq / len);
}
```

**用途**: 计算波形的RMS值

**使用示例**:
```c
float rms_amp = calculateRMS(rf3amp, buf_len);
printf("RMS RF3 amplitude: %.3f V\n", rms_amp);
```

### 2.4 findPeak() - 查找峰值

```c
static float findPeak(const float *data, int len, int *peak_index)
{
    if (data == NULL || len == 0) {
        if (peak_index != NULL) *peak_index = -1;
        return 0.0;
    }

    float max_value = data[0];
    int max_index = 0;

    for (int i = 1; i < len; i++) {
        if (data[i] > max_value) {
            max_value = data[i];
            max_index = i;
        }
    }

    if (peak_index != NULL) {
        *peak_index = max_index;
    }

    return max_value;
}
```

**使用示例**:
```c
int peak_idx;
float peak_amp = findPeak(rf3amp, buf_len, &peak_idx);
printf("Peak amplitude: %.3f V at index %d\n", peak_amp, peak_idx);
```

### 2.5 findMin() - 查找最小值

```c
static float findMin(const float *data, int len, int *min_index)
{
    if (data == NULL || len == 0) {
        if (min_index != NULL) *min_index = -1;
        return 0.0;
    }

    float min_value = data[0];
    int min_idx = 0;

    for (int i = 1; i < len; i++) {
        if (data[i] < min_value) {
            min_value = data[i];
            min_idx = i;
        }
    }

    if (min_index != NULL) {
        *min_index = min_idx;
    }

    return min_value;
}
```

## 3. 调试辅助函数

### 3.1 printDebugInfo() - 打印调试信息

```c
static int debug_level = 0;  // 0=关闭, 1=基本, 2=详细

static void printDebugInfo(const char *msg)
{
    if (debug_level >= 1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        printf("[DEBUG %ld.%06ld] %s\n",
               tv.tv_sec, tv.tv_usec, msg);
    }
}
```

**使用示例**:
```c
printDebugInfo("Starting data acquisition");
funcTriggerAllDataReached();
printDebugInfo("Data acquisition complete");
```

### 3.2 dumpBuffer() - 打印缓冲区内容

```c
static void dumpBuffer(const char *name, const float *buf, int len)
{
    printf("=== Buffer: %s (%d points) ===\n", name, len);

    // 只打印前10个和后10个
    int print_len = (len < 20) ? len : 10;

    for (int i = 0; i < print_len; i++) {
        printf("  [%5d] = %10.3f\n", i, buf[i]);
    }

    if (len > 20) {
        printf("  ...\n");
        for (int i = len - 10; i < len; i++) {
            printf("  [%5d] = %10.3f\n", i, buf[i]);
        }
    }

    printf("=============================\n");
}
```

**使用示例**:
```c
dumpBuffer("RF3Amp", rf3amp, buf_len);
```

### 3.3 analyzeBuffer() - 分析缓冲区

```c
static void analyzeBuffer(const char *name, const float *buf, int len)
{
    if (buf == NULL || len == 0) {
        printf("Buffer %s is empty\n", name);
        return;
    }

    // 统计信息
    float min, max, sum = 0.0, sum_sq = 0.0;
    int min_idx, max_idx;

    min = max = buf[0];
    min_idx = max_idx = 0;

    for (int i = 0; i < len; i++) {
        float val = buf[i];

        if (val < min) {
            min = val;
            min_idx = i;
        }
        if (val > max) {
            max = val;
            max_idx = i;
        }

        sum += val;
        sum_sq += val * val;
    }

    float avg = sum / len;
    float rms = sqrt(sum_sq / len);
    float std_dev = sqrt((sum_sq / len) - (avg * avg));

    printf("=== Analysis: %s (%d points) ===\n", name, len);
    printf("  Min:    %10.3f  @ [%d]\n", min, min_idx);
    printf("  Max:    %10.3f  @ [%d]\n", max, max_idx);
    printf("  Avg:    %10.3f\n", avg);
    printf("  RMS:    %10.3f\n", rms);
    printf("  StdDev: %10.3f\n", std_dev);
    printf("  Range:  %10.3f\n", max - min);
    printf("===============================\n");
}
```

**使用示例**:
```c
analyzeBuffer("RF3Amp", rf3amp, buf_len);
```

**输出示例**:
```
=== Analysis: RF3Amp (10000 points) ===
  Min:     90.234  @ [4567]
  Max:    109.876  @ [2341]
  Avg:    100.123
  RMS:    100.456
  StdDev:   5.789
  Range:   19.642
===============================
```

### 3.4 checkBufferHealth() - 检查缓冲区健康状态

```c
static int checkBufferHealth(const float *buf, int len)
{
    int nan_count = 0;
    int inf_count = 0;
    int zero_count = 0;

    for (int i = 0; i < len; i++) {
        if (isnan(buf[i])) {
            nan_count++;
        } else if (isinf(buf[i])) {
            inf_count++;
        } else if (buf[i] == 0.0) {
            zero_count++;
        }
    }

    if (nan_count > 0) {
        printf("WARNING: %d NaN values detected\n", nan_count);
    }
    if (inf_count > 0) {
        printf("WARNING: %d Inf values detected\n", inf_count);
    }
    if (zero_count == len) {
        printf("WARNING: All values are zero\n");
    }

    return (nan_count == 0 && inf_count == 0);
}
```

## 4. 工具辅助函数

### 4.1 getCurrentTime() - 获取当前时间字符串

```c
static const char* getCurrentTime(void)
{
    static char time_buf[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    return time_buf;
}
```

**使用示例**:
```c
printf("[%s] System initialized\n", getCurrentTime());
```

### 4.2 getElapsedTime() - 计算经过的时间

```c
static struct timeval start_time;

static void startTimer(void)
{
    gettimeofday(&start_time, NULL);
}

static double getElapsedTime(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    return (now.tv_sec - start_time.tv_sec) +
           (now.tv_usec - start_time.tv_usec) / 1000000.0;
}
```

**使用示例**:
```c
startTimer();
funcTriggerAllDataReached();
double elapsed = getElapsedTime();
printf("Data acquisition took %.3f ms\n", elapsed * 1000);
```

### 4.3 saveBufferToFile() - 保存缓冲区到文件

```c
static int saveBufferToFile(const char *filename,
                             const float *buf,
                             int len)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", filename);
        return -1;
    }

    // 写入头部
    fprintf(fp, "# Data saved at: %s\n", getCurrentTime());
    fprintf(fp, "# Number of points: %d\n", len);
    fprintf(fp, "# Index\tValue\n");

    // 写入数据
    for (int i = 0; i < len; i++) {
        fprintf(fp, "%d\t%.6f\n", i, buf[i]);
    }

    fclose(fp);

    printf("Buffer saved to %s (%d points)\n", filename, len);

    return 0;
}
```

**使用示例**:
```c
saveBufferToFile("/tmp/rf3amp.dat", rf3amp, buf_len);
```

### 4.4 loadBufferFromFile() - 从文件加载缓冲区

```c
static int loadBufferFromFile(const char *filename,
                               float *buf,
                               int max_len)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", filename);
        return -1;
    }

    int count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL && count < max_len) {
        // 跳过注释行
        if (line[0] == '#') continue;

        // 解析数据
        int index;
        float value;
        if (sscanf(line, "%d\t%f", &index, &value) == 2) {
            buf[count++] = value;
        }
    }

    fclose(fp);

    printf("Loaded %d points from %s\n", count, filename);

    return count;
}
```

## 5. 编写自定义辅助函数

### 5.1 设计原则

```c
// ✅ 好的辅助函数
// 1. 单一职责
static float calculateAverage(const float *data, int len);

// 2. 参数检查
static float calculateAverage(const float *data, int len)
{
    if (data == NULL || len == 0) {
        return 0.0;  // 安全返回
    }
    // ...
}

// 3. 清晰命名
static void initAllBuffers(void);     // 清晰
static void copyWaveData(...);        // 清晰

// ❌ 不好的辅助函数
// 1. 职责不清
static void doStuff(void);  // 做什么？

// 2. 无参数检查
static float avg(float *d, int l) {
    return sum(d) / l;  // 没检查NULL和0
}

// 3. 命名不清
static void proc(void);  // 不知道做什么
```

### 5.2 示例：编写calculateSNR()

```c
/**
 * 计算信噪比
 * @param signal: 信号缓冲区
 * @param noise_start: 噪声区间开始索引
 * @param noise_end: 噪声区间结束索引
 * @param len: 数据长度
 * @return: SNR (dB)
 */
static float calculateSNR(const float *signal,
                          int noise_start,
                          int noise_end,
                          int len)
{
    // 参数检查
    if (signal == NULL || len == 0) {
        fprintf(stderr, "ERROR: Invalid parameters for calculateSNR\n");
        return 0.0;
    }

    if (noise_start < 0 || noise_end >= len || noise_start >= noise_end) {
        fprintf(stderr, "ERROR: Invalid noise region\n");
        return 0.0;
    }

    // 计算信号功率 (整个波形的RMS)
    float signal_power = calculateRMS(signal, len);

    // 计算噪声功率 (噪声区间的RMS)
    int noise_len = noise_end - noise_start;
    float noise_power = calculateRMS(signal + noise_start, noise_len);

    // 计算SNR (dB)
    if (noise_power == 0.0) {
        return 100.0;  // 无限大SNR，返回一个大值
    }

    float snr_db = 20.0 * log10(signal_power / noise_power);

    return snr_db;
}
```

**使用示例**:
```c
// 假设前100个点是噪声区域
float snr = calculateSNR(rf3amp, 0, 100, buf_len);
printf("RF3 SNR: %.1f dB\n", snr);
```

## 6. 性能考虑

### 6.1 避免重复计算

```c
// ❌ 慢速
for (int i = 0; i < 100; i++) {
    float avg = calculateAverage(rf3amp, buf_len);  // 重复计算
    printf("Average: %.3f\n", avg);
}

// ✅ 快速
float avg = calculateAverage(rf3amp, buf_len);  // 只计算一次
for (int i = 0; i < 100; i++) {
    printf("Average: %.3f\n", avg);
}
```

### 6.2 使用适当的数据类型

```c
// ❌ 不必要的double
static double calculateAverage(const double *data, int len);

// ✅ float足够
static float calculateAverage(const float *data, int len);
```

## ❓ 常见问题

### Q1: 辅助函数应该声明为static吗？
**A**: 是的，辅助函数通常是内部使用，声明为static可以:
- 避免命名冲突
- 提高性能（编译器优化）
- 明确作用域

### Q2: 什么时候应该创建辅助函数？
**A**: 当满足以下任一条件时:
- 代码重复出现 >= 2次
- 逻辑复杂需要封装
- 便于测试和调试
- 提高代码可读性

### Q3: 辅助函数应该放在文件的哪里？
**A**:
```c
// 推荐顺序
// 1. 头文件包含
// 2. 宏定义
// 3. 全局变量
// 4. 辅助函数声明（可选）
// 5. 核心函数实现
// 6. 辅助函数实现
```

## 📚 延伸阅读

- [02-file-structure.md](./02-file-structure.md) - 文件结构分析
- [12-debugging.md](./12-debugging.md) - 调试技巧
- C Programming Best Practices

## 🎓 本章总结

- ✅ 辅助函数提高代码复用性和可读性
- ✅ 分为初始化、数据处理、调试、工具四类
- ✅ 遵循单一职责、参数检查、清晰命名原则
- ✅ 声明为static限制作用域

**核心思想**: 封装复杂逻辑，提供简洁接口

**下一步**: 阅读 [12-debugging.md](./12-debugging.md) 学习调试技巧

---

**实验任务**: 编写calculateJitter()函数计算波形抖动
