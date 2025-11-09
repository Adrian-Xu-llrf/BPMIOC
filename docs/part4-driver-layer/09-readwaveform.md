# ReadWaveform()函数详解

> **阅读时间**: 40分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: 全局缓冲区、waveform Record、memcpy

## 📋 本文目标

- 理解ReadWaveform()的核心作用
- 掌握波形数据的读取机制
- 学会如何添加新的波形PV
- 理解与ReadData()的区别

## 🎯 ReadWaveform()是什么？

### 定位

```
数据流: 硬件 → 全局缓冲区 → ReadWaveform() → waveform Record → CA
                               ↑
                         波形数据入口
```

**ReadWaveform()是波形数据的读取入口**，所有waveform Record的数据都通过它。

### 函数签名

```c
// driverWrapper.c line 1151-1300

long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    // offset: 波形类型 (0-29)
    // channel: 通道号 (0-7)
    // buf: 输出缓冲区 (EPICS提供)
    // len: 输出数据长度 (输出参数)

    switch (offset) {
        case 0:  // RF3Amp
            memcpy(buf, rf3amp, buf_len * sizeof(float));
            *len = buf_len;
            break;

        // ... 共20+个波形

        default:
            *len = 0;
            return -1;
    }

    return 0;  // 成功
}
```

### 与ReadData()的区别

```
ReadData()                 vs    ReadWaveform()
   ↓                                  ↓
返回单个float值                    拷贝float数组
用于ai Record                    用于waveform Record
每次1个值                         每次10000个值
~500行代码                        ~150行代码
29种offset                       ~25种offset
```

## 1. 四个参数详解

### 1.1 offset参数 (波形类型)

```c
// 波形offset通常与ReadData的offset对应，但有区别

波形offset分类:
├─ 0-7:   RF波形 (RF3-RF6的Amp/Phase)
├─ 8-11:  XY波形 (X1/Y1/X2/Y2)
├─ 12-19: Button波形 (Button1-8)
├─ 20-23: 历史波形 (HistoryX1/Y1/X2/Y2)
└─ 24-29: 其他特殊波形
```

**示例**:
- offset=0: RF3Amp波形 (10000点)
- offset=8: X1波形 (10000点)
- offset=20: HistoryX1 (100000点)

### 1.2 channel参数

```c
// channel在ReadWaveform中的用途较少
// 某些波形类型可能忽略channel

// 示例1: RF波形，channel已包含在offset中
ReadWaveform(0, 0, buf, &len);  // RF3Amp, channel忽略

// 示例2: Button波形，channel可能用于选择button
ReadWaveform(12, 0, buf, &len);  // Button1
ReadWaveform(12, 1, buf, &len);  // Button2
```

### 1.3 buf参数 (输出缓冲区)

```c
// EPICS提供的缓冲区，用于存储波形数据

// waveform Record定义
record(waveform, "LLRF:BPM:RF3AmpWave") {
    field(NELM, "10000")  # 最大元素数
    field(FTVL, "FLOAT")  # 元素类型: float
}

// EPICS分配的内部缓冲区
float epics_buffer[10000];

// ReadWaveform()将数据拷贝到这个缓冲区
ReadWaveform(0, 0, epics_buffer, &len);
```

**重要**:
- `buf`由EPICS分配，驱动层不能修改指针
- `buf`的大小由NELM字段决定
- 驱动层只负责填充数据

### 1.4 len参数 (输出长度)

```c
int len;
ReadWaveform(0, 0, buf, &len);
printf("Copied %d points\n", len);  // 输出: Copied 10000 points
```

**作用**:
- 告诉EPICS实际拷贝了多少数据
- 可以小于NELM (Record的最大长度)
- EPICS会更新Record的NORD字段 (实际数据点数)

## 2. 核心switch-case结构

### 2.1 整体结构

```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    // 参数验证
    if (buf == NULL || len == NULL) {
        fprintf(stderr, "ERROR: NULL pointer in ReadWaveform\n");
        return -1;
    }

    switch (offset) {
        case 0:   // RF3Amp
        case 1:   // RF3Phase
        // ... 共20+个波形

        default:
            fprintf(stderr, "ERROR: Unknown waveform offset: %d\n", offset);
            *len = 0;
            return -1;
    }

    return 0;  // 成功
}
```

### 2.2 典型case分析

#### Case 0: RF3Amp (普通波形)

```c
case 0:  // RF3Amp
    // 从全局缓冲区拷贝数据
    memcpy(buf, rf3amp, buf_len * sizeof(float));

    // 设置数据长度
    *len = buf_len;  // buf_len = 10000

    break;
```

**关键点**:
- 使用memcpy快速拷贝
- 数据源是全局数组rf3amp
- 长度固定为buf_len (10000)

#### Case 20: HistoryX1 (历史波形)

```c
case 20:  // HistoryX1
    // 从环形缓冲区拷贝数据，保证时间连续性
    int start_idx = history_index;  // 当前写入位置

    for (int i = 0; i < trip_buf_len; i++) {
        int idx = (start_idx + i) % trip_buf_len;
        buf[i] = HistoryX1[idx];
    }

    *len = trip_buf_len;  // trip_buf_len = 100000

    break;
```

**关键点**:
- 环形缓冲区需要重新排序
- 从history_index开始读取，保证时间连续
- 长度是trip_buf_len (100000)

#### Case with channel (Button波形)

```c
case 12:  // Button波形
    // 根据channel选择不同的button
    switch (channel) {
        case 0:
            memcpy(buf, wave_button1, buf_len * sizeof(float));
            break;
        case 1:
            memcpy(buf, wave_button2, buf_len * sizeof(float));
            break;
        case 2:
            memcpy(buf, wave_button3, buf_len * sizeof(float));
            break;
        // ... button4-8
        default:
            fprintf(stderr, "ERROR: Invalid button channel: %d\n", channel);
            *len = 0;
            return -1;
    }

    *len = buf_len;
    break;
```

## 3. 完整波形列表

### 3.1 RF波形 (8个)

```c
// offset=0-7: RF3-RF6的Amp和Phase

case 0:  // RF3Amp
    memcpy(buf, rf3amp, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 1:  // RF3Phase
    memcpy(buf, rf3phase, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 2:  // RF4Amp
    memcpy(buf, rf4amp, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 3:  // RF4Phase
    memcpy(buf, rf4phase, buf_len * sizeof(float));
    *len = buf_len;
    break;

// ... RF5, RF6 类似
```

**数据源**: 全局数组rf3amp, rf3phase, rf4amp, ...
**长度**: 10000点

### 3.2 XY位置波形 (4个)

```c
// offset=8-11: X1, Y1, X2, Y2

case 8:  // X1
    memcpy(buf, wave_X1, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 9:  // Y1
    memcpy(buf, wave_Y1, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 10:  // X2
    memcpy(buf, wave_X2, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 11:  // Y2
    memcpy(buf, wave_Y2, buf_len * sizeof(float));
    *len = buf_len;
    break;
```

**数据源**: wave_X1, wave_Y1, wave_X2, wave_Y2
**长度**: 10000点

### 3.3 Button波形 (8个)

```c
// offset=12-19: Button1-8

case 12:  // Button1
    memcpy(buf, wave_button1, buf_len * sizeof(float));
    *len = buf_len;
    break;

case 13:  // Button2
    memcpy(buf, wave_button2, buf_len * sizeof(float));
    *len = buf_len;
    break;

// ... Button3-8 类似
```

**数据源**: wave_button1, wave_button2, ...
**长度**: 10000点

### 3.4 历史波形 (4个)

```c
// offset=20-23: HistoryX1, HistoryY1, HistoryX2, HistoryY2

case 20:  // HistoryX1
    // 环形缓冲区，需要重新排序
    for (int i = 0; i < trip_buf_len; i++) {
        int idx = (history_index + i) % trip_buf_len;
        buf[i] = HistoryX1[idx];
    }
    *len = trip_buf_len;
    break;

// ... HistoryY1, HistoryX2, HistoryY2 类似
```

**数据源**: HistoryX1, HistoryY1, HistoryX2, HistoryY2
**长度**: 100000点

## 4. 调用链详解

### 4.1 完整数据流

```
1. CA客户端
   caget LLRF:BPM:RF3AmpWave
         ↓

2. CA网络
   请求到达IOC
         ↓

3. EPICS Record
   waveform Record处理
   precord->rset->process(precord)
         ↓

4. 设备支持层
   devBPMMonitor.c: read_waveform()
   DevPvt *pPvt = (DevPvt *)precord->dpvt;
         ↓

5. 驱动层
   ReadWaveform(pPvt->offset, pPvt->channel,
                precord->bptr, &nElements)
   ReadWaveform(0, 0, epics_buffer, &len)
         ↓

6. 全局缓冲区
   memcpy(epics_buffer, rf3amp, 10000 * 4)
         ↓

7. Record更新
   precord->nord = len;  // 实际数据点数
         ↓

8. CA网络
   发送10000个float给客户端
```

### 4.2 代码示例

```c
// 设备支持层 (devBPMMonitor.c)
static long read_waveform(waveformRecord *precord)
{
    DevPvt *pPvt = (DevPvt *)precord->dpvt;
    int nElements;

    // 调用ReadWaveform
    long status = ReadWaveform(pPvt->offset,
                               pPvt->channel,
                               (float *)precord->bptr,
                               &nElements);

    if (status != 0) {
        // 读取失败
        recGblSetSevr(precord, READ_ALARM, INVALID_ALARM);
        precord->nord = 0;
        return -1;
    }

    // 更新实际数据点数
    precord->nord = nElements;

    return 0;
}
```

## 5. 如何添加新波形

### 示例：添加RF7Amp波形

#### Step 1: 声明全局缓冲区

```c
// driverWrapper.c 全局变量区域
static float rf7amp[buf_len];
static float rf7phase[buf_len];
```

#### Step 2: 初始化缓冲区

```c
static void initAllBuffers(void)
{
    // ... 现有buffer初始化 ...

    memset(rf7amp, 0, sizeof(rf7amp));
    memset(rf7phase, 0, sizeof(rf7phase));
}
```

#### Step 3: 在ReadWaveform()中添加case

```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    switch (offset) {
        // ... 现有case 0-23 ...

        case 24:  // RF7Amp
            memcpy(buf, rf7amp, buf_len * sizeof(float));
            *len = buf_len;
            break;

        case 25:  // RF7Phase
            memcpy(buf, rf7phase, buf_len * sizeof(float));
            *len = buf_len;
            break;

        default:
            *len = 0;
            return -1;
    }

    return 0;
}
```

#### Step 4: 在pthread中更新数据

```c
void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();

        // 更新RF7数据 (假设硬件库提供funcGetRF7Wave)
        if (funcGetRF7WaveAmp != NULL) {
            for (int i = 0; i < buf_len; i++) {
                rf7amp[i] = funcGetRF7WaveAmp(i);
                rf7phase[i] = funcGetRF7WavePhase(i);
            }
        }

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

#### Step 5: 在数据库中创建Record

```
# BPMMonitor.db
record(waveform, "LLRF:BPM:RF7AmpWave") {
    field(DESC, "RF7 Amplitude Waveform")
    field(DTYP, "BPMMonitor")
    field(INP,  "@24")         # offset=24
    field(SCAN, "I/O Intr")
    field(NELM, "10000")       # 10000个元素
    field(FTVL, "FLOAT")       # float类型
    field(EGU,  "V")
}

record(waveform, "LLRF:BPM:RF7PhaseWave") {
    field(DESC, "RF7 Phase Waveform")
    field(DTYP, "BPMMonitor")
    field(INP,  "@25")
    field(SCAN, "I/O Intr")
    field(NELM, "10000")
    field(FTVL, "FLOAT")
    field(EGU,  "deg")
}
```

## 6. 性能优化

### 6.1 memcpy性能

```c
// memcpy是高度优化的，通常使用SIMD指令

// 10000个float的拷贝时间
Size:  10000 × 4字节 = 40 KB
Time:  ~1-2 μs (现代CPU)
```

**最佳实践**: 总是使用memcpy，不要手写循环

### 6.2 避免不必要的拷贝

```c
// ❌ 不推荐：两次拷贝
float temp_buffer[buf_len];
memcpy(temp_buffer, rf3amp, buf_len * sizeof(float));
memcpy(buf, temp_buffer, buf_len * sizeof(float));

// ✅ 推荐：一次拷贝
memcpy(buf, rf3amp, buf_len * sizeof(float));
```

### 6.3 大波形的处理

```c
// 对于历史波形 (100000点 = 400KB)
case 20:  // HistoryX1
    // 如果不需要重新排序，直接memcpy
    if (history_index == 0) {
        // 数据已经是连续的
        memcpy(buf, HistoryX1, trip_buf_len * sizeof(float));
    } else {
        // 需要重新排序 (~100μs)
        for (int i = 0; i < trip_buf_len; i++) {
            int idx = (history_index + i) % trip_buf_len;
            buf[i] = HistoryX1[idx];
        }
    }
    *len = trip_buf_len;
    break;
```

## 7. 调试技巧

### 7.1 打印波形统计信息

```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    // ... switch-case ...

    #ifdef DEBUG_WAVEFORM
        // 计算统计信息
        float min = buf[0], max = buf[0], sum = 0.0;
        for (int i = 0; i < *len; i++) {
            if (buf[i] < min) min = buf[i];
            if (buf[i] > max) max = buf[i];
            sum += buf[i];
        }
        float avg = sum / (*len);

        printf("ReadWaveform(offset=%d): len=%d, min=%.3f, max=%.3f, avg=%.3f\n",
               offset, *len, min, max, avg);
    #endif

    return 0;
}
```

### 7.2 保存波形到文件

```c
void saveWaveformToFile(const char *filename, const float *buf, int len)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", filename);
        return;
    }

    for (int i = 0; i < len; i++) {
        fprintf(fp, "%d\t%.6f\n", i, buf[i]);
    }

    fclose(fp);
    printf("Waveform saved to %s (%d points)\n", filename, len);
}

// 使用
case 0:  // RF3Amp
    memcpy(buf, rf3amp, buf_len * sizeof(float));
    *len = buf_len;

    #ifdef SAVE_WAVEFORM
        saveWaveformToFile("/tmp/rf3amp.txt", buf, *len);
    #endif

    break;
```

### 7.3 检查数据完整性

```c
void checkWaveformIntegrity(const float *buf, int len)
{
    int nan_count = 0;
    int inf_count = 0;

    for (int i = 0; i < len; i++) {
        if (isnan(buf[i])) {
            nan_count++;
        }
        if (isinf(buf[i])) {
            inf_count++;
        }
    }

    if (nan_count > 0 || inf_count > 0) {
        printf("WARNING: Waveform contains %d NaN, %d Inf\n",
               nan_count, inf_count);
    }
}
```

## 8. 网络传输考虑

### 8.1 数据量计算

```c
// 单个普通波形
Size: 10000点 × 4字节 = 40 KB

// CA网络开销
Header: ~100字节
Total:  ~40.1 KB

// 传输时间 (假设100 Mbps网络)
Time:   40 KB × 8 / 100 Mbps = 3.2 ms

// 历史波形
Size:   100000点 × 4字节 = 400 KB
Time:   400 KB × 8 / 100 Mbps = 32 ms
```

**注意**: 大波形会占用网络带宽

### 8.2 按需读取策略

```c
// 只在需要时读取历史波形
record(waveform, "LLRF:BPM:HistoryX1") {
    field(SCAN, "Passive")  # 不是I/O Intr
}

# 客户端按需触发
caput LLRF:BPM:HistoryX1.PROC 1
caget LLRF:BPM:HistoryX1
```

## ❓ 常见问题

### Q1: ReadWaveform会被并发调用吗？
**A**:
- 是的，多个waveform Record可能并发调用
- 但每个Record有自己的dbScanLock
- 只要不修改全局状态，就是安全的

### Q2: 可以返回小于NELM的数据吗？
**A**:
```c
case 0:
    // 只返回前5000个点
    memcpy(buf, rf3amp, 5000 * sizeof(float));
    *len = 5000;  // EPICS会设置NORD=5000
    break;
```

### Q3: 如何处理可变长度波形？
**A**:
```c
case 26:  // 可变长度波形
    int actual_len = funcGetActualDataLength();

    if (actual_len > buf_len) {
        actual_len = buf_len;  // 截断
    }

    memcpy(buf, variable_wave, actual_len * sizeof(float));
    *len = actual_len;
    break;
```

### Q4: 能返回int或double类型的波形吗？
**A**:
- EPICS waveform支持多种FTVL (Field Type of Value)
- 但BPMIOC统一使用FLOAT
- 如需其他类型，在ReadWaveform中转换:
```c
// 如果Record是FTVL="SHORT"
case 27:
    short *short_buf = (short *)buf;
    for (int i = 0; i < buf_len; i++) {
        short_buf[i] = (short)rf3amp[i];
    }
    *len = buf_len;
    break;
```

## 📚 延伸阅读

- [03-global-buffers.md](./03-global-buffers.md) - 全局缓冲区设计
- [07-readdata.md](./07-readdata.md) - ReadData函数对比
- [Part 5: 04-read-waveform.md](../part5-device-support-layer/04-read-waveform.md) - 设备支持层调用

## 🎓 本章总结

- ✅ ReadWaveform()是波形数据读取入口
- ✅ 使用memcpy快速拷贝数组数据
- ✅ 支持10000点普通波形和100000点历史波形
- ✅ 通过offset路由到不同的全局缓冲区
- ✅ 添加新波形需要5个步骤

**核心操作**: memcpy(EPICS缓冲区, 全局缓冲区, 长度)

**下一步**: 阅读 [10-hardware-functions.md](./10-hardware-functions.md) 学习硬件函数详解

---

**实验任务**: 添加一个新波形PV，保存前100个点到文件，用gnuplot绘图
