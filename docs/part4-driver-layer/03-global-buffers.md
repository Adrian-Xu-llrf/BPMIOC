# 全局缓冲区设计详解

> **阅读时间**: 40分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: C语言数组、指针、内存管理

## 📋 本文目标

- 理解为什么使用全局缓冲区
- 掌握缓冲区的设计和内存布局
- 学会如何添加新的缓冲区
- 理解缓冲区的线程安全性

## 🎯 为什么需要全局缓冲区？

### EPICS Record的特点

```c
// EPICS Record每次处理只能返回一个值
// 对于ai (analog input) record:
float value = ReadData(offset, channel, type);

// 对于waveform record:
long status = ReadWaveform(offset, channel, buffer, &length);
```

### 数据流特点

```
硬件 → 采集 → 缓存 → EPICS Record → CA网络
     (100ms)  (全局)   (随时访问)    (异步)
```

**关键需求**:
1. 数据需要**持久化**：Record可能随时访问
2. 数据需要**共享**：多个Record访问同一份数据
3. 性能要求**高**：避免频繁内存分配

## 📊 缓冲区总览

### 缓冲区分类

```
全局缓冲区
├─ 标量缓冲区 (隐式在函数调用中)
│  └─ 通过硬件函数实时获取
│
├─ 波形缓冲区 (10000点)
│  ├─ RF信息 (8通道)
│  ├─ XY位置 (4通道)
│  ├─ Button信号 (8通道)
│  └─ 其他波形 (~20个)
│
├─ 历史波形缓冲区 (100000点)
│  ├─ HistoryX1
│  ├─ HistoryY1
│  ├─ HistoryX2
│  └─ HistoryY2
│
└─ 寄存器缓冲区 (100个)
   └─ Reg[REG_NUM]
```

## 1. 波形缓冲区详解

### 1.1 RF信息缓冲区 (8通道)

```c
// driverWrapper.c line 51-90

// RF3 (通道3)
static float rf3amp[buf_len];      // 幅度
static float rf3phase[buf_len];    // 相位

// RF4 (通道4)
static float rf4amp[buf_len];
static float rf4phase[buf_len];

// RF5 (通道5)
static float rf5amp[buf_len];
static float rf5phase[buf_len];

// RF6 (通道6)
static float rf6amp[buf_len];
static float rf6phase[buf_len];
```

**内存计算**:
```
每个buffer大小: buf_len × sizeof(float)
              = 10000 × 4字节
              = 40 KB

RF信息总大小: 8个buffer × 40KB = 320 KB
```

**更新机制**:
```c
void *pthread(void *arg)
{
    while (1) {
        // 触发硬件采集
        funcTriggerAllDataReached();

        // 从硬件获取数据并填充buffer
        // (实际通过硬件库内部填充，或通过funcGetAllWaveData)

        usleep(100000);  // 100ms周期
    }
}
```

**访问方式**:
```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    switch (offset) {
        case 0:  // RF3Amp
            memcpy(buf, rf3amp, buf_len * sizeof(float));
            *len = buf_len;
            break;
        case 1:  // RF3Phase
            memcpy(buf, rf3phase, buf_len * sizeof(float));
            *len = buf_len;
            break;
        // ...
    }
}
```

### 1.2 XY位置缓冲区 (4通道)

```c
// X1, Y1: BPM 1的位置
static float wave_X1[buf_len];
static float wave_Y1[buf_len];

// X2, Y2: BPM 2的位置
static float wave_X2[buf_len];
static float wave_Y2[buf_len];
```

**内存计算**:
```
4个buffer × 40KB = 160 KB
```

**物理意义**:
```
BPM (Beam Position Monitor)
├─ X1, Y1: 第一个探头的位置
│  └─ 10000个采样点 = 100ms × 100kHz
│
└─ X2, Y2: 第二个探头的位置
   └─ 用于束流轨迹计算
```

### 1.3 Button信号缓冲区 (8通道)

```c
// 8个button信号
static float wave_button1[buf_len];
static float wave_button2[buf_len];
static float wave_button3[buf_len];
static float wave_button4[buf_len];
static float wave_button5[buf_len];
static float wave_button6[buf_len];
static float wave_button7[buf_len];
static float wave_button8[buf_len];
```

**内存计算**:
```
8个buffer × 40KB = 320 KB
```

**物理意义**:
- Button1-4: BPM探头的4个电极信号
- Button5-8: 可能是另一个BPM的信号或预留

### 1.4 普通波形缓冲区总计

```
RF信息:     320 KB
XY位置:     160 KB
Button信号: 320 KB
-----------------------
总计:       800 KB
```

## 2. 历史波形缓冲区详解

### 2.1 设计目的

```
历史波形 vs 普通波形
├─ 普通波形: 10000点 = 100ms数据
│  └─ 用于实时监控
│
└─ 历史波形: 100000点 = 1秒数据
   └─ 用于故障分析（trip buffer）
```

### 2.2 缓冲区定义

```c
// 历史X1, Y1: 1秒的位置数据
static float HistoryX1[trip_buf_len];  // trip_buf_len = 100000
static float HistoryY1[trip_buf_len];

// 历史X2, Y2
static float HistoryX2[trip_buf_len];
static float HistoryY2[trip_buf_len];
```

**内存计算**:
```
每个buffer: 100000 × 4字节 = 400 KB
总计:      4 × 400KB = 1.6 MB
```

### 2.3 环形缓冲区实现

```c
static int history_index = 0;  // 当前写入位置

void updateHistoryBuffer(void)
{
    // 写入新数据
    HistoryX1[history_index] = current_X1;
    HistoryY1[history_index] = current_Y1;
    HistoryX2[history_index] = current_X2;
    HistoryY2[history_index] = current_Y2;

    // 循环索引
    history_index = (history_index + 1) % trip_buf_len;
}
```

**环形缓冲区示意**:
```
[0]───[1]───[2]───...───[99999]
 ↑                        ↓
 └────────循环写入──────┘

当索引到达99999时，下一次写入到0
始终保持最近1秒的数据
```

### 2.4 读取历史数据

```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    switch (offset) {
        case 20:  // HistoryX1
            // 从history_index开始读取，保证时间连续性
            int start = history_index;
            for (int i = 0; i < trip_buf_len; i++) {
                int idx = (start + i) % trip_buf_len;
                buf[i] = HistoryX1[idx];
            }
            *len = trip_buf_len;
            break;
        // ...
    }
}
```

**时间连续性**:
```
假设history_index = 5000
读取顺序: [5000, 5001, ..., 99999, 0, 1, ..., 4999]
         └────── 新数据 ──────┘ └──── 旧数据 ────┘
保证数据按时间顺序排列
```

## 3. 寄存器缓冲区详解

### 3.1 设计目的

```c
// 存储硬件配置参数
static int Reg[REG_NUM];  // REG_NUM = 100
```

**用途**:
- Reg[0]: 系统状态
- Reg[1]: 采样率设置
- Reg[2]: 触发模式
- Reg[3]: 增益配置
- Reg[4-99]: 其他参数

### 3.2 读写操作

```c
// 写入寄存器
long SetReg(int addr, int value)
{
    if (addr < 0 || addr >= REG_NUM) {
        return -1;
    }

    // 更新本地缓存
    Reg[addr] = value;

    // 写入硬件
    if (funcSetReg != NULL) {
        funcSetReg(addr, value);
    }

    return 0;
}

// 读取寄存器
float ReadData(int offset, int channel, int type)
{
    if (offset == 28) {  // Reg offset
        if (channel >= 0 && channel < REG_NUM) {
            return (float)Reg[channel];
        }
    }
    // ...
}
```

### 3.3 软硬件同步

```
                SetReg(addr, value)
                       ↓
    ┌──────────────────┴────────────────────┐
    │                                        │
    ↓                                        ↓
Reg[addr] = value                  funcSetReg(addr, value)
(软件缓存)                         (写入硬件)
    │                                        │
    │                                        │
    └──────────────→ 保持同步 ←──────────────┘
```

## 4. 内存布局总览

### 4.1 总内存占用

```
组成部分                    大小        占比
──────────────────────────────────────────
普通波形缓冲区              800 KB      33%
历史波形缓冲区              1.6 MB      67%
寄存器缓冲区                400 B       <1%
──────────────────────────────────────────
总计                        ~2.4 MB     100%
```

### 4.2 内存对齐

```c
// 现代CPU喜欢对齐的内存
static float rf3amp[buf_len] __attribute__((aligned(64)));
                                          ↑
                                    64字节对齐
                                    (CPU cache line)
```

**优势**:
- 减少cache miss
- 提高memcpy性能
- 适合SIMD指令

## 5. 线程安全性分析

### 5.1 读写模式

```
[pthread数据采集线程]  ──写入→  [全局缓冲区]  ←读取──  [EPICS Record处理]
     (生产者)                      ↓                    (消费者)
                                  数据
```

**特点**:
- 生产者: 单线程（pthread）
- 消费者: 多线程（多个Record并发读）
- 模式: Single Producer, Multiple Readers

### 5.2 当前实现 (无锁)

```c
// pthread: 写入
void *pthread(void *arg)
{
    while (1) {
        // 写入buffer (无锁)
        updateAllBuffers();

        usleep(100000);
    }
}

// Record: 读取
long ReadWaveform(...)
{
    // 读取buffer (无锁)
    memcpy(buf, rf3amp, buf_len * sizeof(float));
}
```

**为什么无锁是安全的？**

1. **写入频率低**: 100ms = 10 Hz
2. **读取时间短**: memcpy很快 (~1μs)
3. **碰撞概率低**: 写入占用 < 1%时间
4. **数据不关键**: 偶尔读到不一致数据可以接受

### 5.3 如果需要线程安全

```c
// 使用EPICS互斥锁
static epicsMutexId bufferLock;

// 初始化
void InitDevice()
{
    bufferLock = epicsMutexCreate();
}

// 写入时加锁
void updateAllBuffers()
{
    epicsMutexLock(bufferLock);
    // ... 更新buffer
    epicsMutexUnlock(bufferLock);
}

// 读取时加锁
long ReadWaveform(...)
{
    epicsMutexLock(bufferLock);
    memcpy(buf, rf3amp, buf_len * sizeof(float));
    epicsMutexUnlock(bufferLock);
}
```

**代价**:
- 性能开销: 加锁/解锁 (~100ns)
- 可能阻塞: Record处理可能等待

**BPMIOC的选择**: 性能优先，不加锁

## 6. 如何添加新缓冲区

### 示例：添加RF7Amp波形

#### Step 1: 声明全局变量

```c
// driverWrapper.c 全局变量区域
static float rf7amp[buf_len];
static float rf7phase[buf_len];
```

#### Step 2: 初始化

```c
static void initAllBuffers(void)
{
    memset(rf3amp, 0, sizeof(rf3amp));
    memset(rf3phase, 0, sizeof(rf3phase));
    // ... 其他buffer

    // 新增
    memset(rf7amp, 0, sizeof(rf7amp));
    memset(rf7phase, 0, sizeof(rf7phase));
}
```

#### Step 3: 在ReadWaveform中添加case

```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    switch (offset) {
        // ... 现有case

        case 30:  // RF7Amp (选择一个未使用的offset)
            memcpy(buf, rf7amp, buf_len * sizeof(float));
            *len = buf_len;
            break;

        case 31:  // RF7Phase
            memcpy(buf, rf7phase, buf_len * sizeof(float));
            *len = buf_len;
            break;
    }
}
```

#### Step 4: 更新数据

```c
void *pthread(void *arg)
{
    while (1) {
        // 触发采集
        funcTriggerAllDataReached();

        // 获取RF7数据 (需要硬件库支持)
        for (int i = 0; i < buf_len; i++) {
            rf7amp[i] = funcGetRFWaveData(7, i, AMP);
            rf7phase[i] = funcGetRFWaveData(7, i, PHASE);
        }

        usleep(100000);
    }
}
```

#### Step 5: 计算新内存占用

```
原有内存: 2.4 MB
新增RF7: 2 × 40KB = 80KB
总内存:  2.48 MB
```

## 7. 性能优化技巧

### 7.1 使用memcpy而非循环

```c
// ❌ 慢速方式
for (int i = 0; i < buf_len; i++) {
    buf[i] = rf3amp[i];
}

// ✅ 快速方式
memcpy(buf, rf3amp, buf_len * sizeof(float));
```

**性能差异**:
- 循环: ~100μs
- memcpy: ~1μs (100倍速度提升)

### 7.2 内存对齐

```c
static float rf3amp[buf_len] __attribute__((aligned(64)));
```

**效果**:
- 对齐到cache line边界
- 减少cache miss
- 提高访问速度

### 7.3 预分配vs动态分配

```c
// ✅ 全局预分配 (BPMIOC的选择)
static float rf3amp[buf_len];
// 优点: 无分配开销，访问快
// 缺点: 占用固定内存

// ❌ 动态分配
float *rf3amp = (float *)malloc(buf_len * sizeof(float));
// 优点: 灵活
// 缺点: malloc/free开销，可能碎片化
```

## 8. 调试技巧

### 8.1 打印缓冲区内容

```c
void dumpBuffer(const char *name, const float *buf, int len)
{
    printf("Buffer %s (%d points):\n", name, len);
    for (int i = 0; i < (len < 10 ? len : 10); i++) {
        printf("  [%d] = %.3f\n", i, buf[i]);
    }
    if (len > 10) {
        printf("  ...\n");
    }
}

// 使用
dumpBuffer("RF3Amp", rf3amp, buf_len);
```

### 8.2 统计缓冲区数据

```c
void analyzeBuffer(const float *buf, int len)
{
    float min = buf[0], max = buf[0], sum = 0.0;

    for (int i = 0; i < len; i++) {
        if (buf[i] < min) min = buf[i];
        if (buf[i] > max) max = buf[i];
        sum += buf[i];
    }

    float avg = sum / len;

    printf("Min: %.3f, Max: %.3f, Avg: %.3f\n", min, max, avg);
}
```

### 8.3 检查数据更新

```c
static int update_counter = 0;

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();

        update_counter++;
        printf("Buffer updated: %d times\n", update_counter);

        usleep(100000);
    }
}
```

## ❓ 常见问题

### Q1: 为什么不用动态分配？
**A**:
- 避免malloc/free开销
- 避免内存碎片
- 简化内存管理
- 提高访问速度

### Q2: 2.4MB内存会不会太大？
**A**:
- 现代系统内存充足
- 相比数据价值，内存成本可忽略
- 实时性比内存占用更重要

### Q3: 如何知道缓冲区有没有溢出？
**A**:
```c
// 使用guard bytes
#define GUARD_PATTERN 0xDEADBEEF
static uint32_t guard_start = GUARD_PATTERN;
static float rf3amp[buf_len];
static uint32_t guard_end = GUARD_PATTERN;

// 定期检查
void checkBufferIntegrity()
{
    if (guard_start != GUARD_PATTERN || guard_end != GUARD_PATTERN) {
        printf("ERROR: Buffer overflow detected!\n");
    }
}
```

### Q4: 能否在运行时动态改变buf_len？
**A**:
- 静态数组不支持
- 需要改用动态分配
- 但会增加复杂度和性能开销
- BPMIOC选择固定大小

## 📚 延伸阅读

- [06-pthread.md](./06-pthread.md) - 数据采集线程详解
- [09-readwaveform.md](./09-readwaveform.md) - ReadWaveform函数详解
- [Part 3: 04-memory-model.md](../part3-bpmioc-architecture/04-memory-model.md) - 内存模型总览

## 🎓 本章总结

- ✅ 全局缓冲区用于持久化存储数据
- ✅ 分为波形、历史、寄存器三类
- ✅ 总内存约2.4MB
- ✅ 采用无锁设计，性能优先
- ✅ 添加新缓冲区需要4个步骤

**下一步**: 阅读 [05-dlopen-dlsym.md](./05-dlopen-dlsym.md) 学习动态库加载机制

---

**实验任务**: 使用 `dumpBuffer()` 在IOC中打印rf3amp的前10个点
