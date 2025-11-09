# 01: 驱动层总览

> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 45分钟
> **前置知识**: Part 3完整内容

## 📋 本文目标

本文提供driverWrapper.c的整体概览，帮助你建立全局视图。

完成本文后，你将能够：
- ✅ 理解驱动层在三层架构中的位置
- ✅ 掌握驱动层的主要职责
- ✅ 了解driverWrapper.c的整体结构
- ✅ 知道从哪里开始深入学习

## 🎯 驱动层的定位

### 在三层架构中的位置

```
+-------------------+
|   数据库层（DB）   |  ← Record定义
+-------------------+
         ↕
    dset接口
         ↕
+-------------------+
|  设备支持层（Dev）  |  ← devBPMMonitor.c
+-------------------+
         ↕
    函数调用        ←─── 你现在在这里
         ↕
+-------------------+
|   驱动层（Driver） |  ← driverWrapper.c ⭐
+-------------------+
         ↕
    dlopen/dlsym
         ↕
+-------------------+
|  硬件库（Hardware）|  ← libBPMboard14And15.so
+-------------------+
```

### 驱动层的职责

| 职责 | 描述 | 实现方式 |
|------|------|----------|
| **硬件抽象** | 封装硬件细节，提供统一接口 | ReadData(), SetReg(), readWaveform() |
| **库加载** | 动态加载硬件库 | dlopen(), dlsym() |
| **数据采集** | 周期性从硬件读取数据 | pthread线程 + funcTriggerAllDataReached() |
| **数据缓存** | 缓存硬件数据供上层读取 | 全局数组 (rf3amp[], X1_avg, ...) |
| **事件通知** | 通知上层数据已准备好 | scanIoRequest() |
| **参数管理** | 管理配置参数 | Getparameters(), CSV文件读取 |

## 📊 driverWrapper.c文件统计

```
总行数: 1539行
  ├─ 头文件和宏定义: ~30行 (2%)
  ├─ 全局变量定义: ~170行 (11%)
  ├─ InitDevice()函数: ~100行 (6%)
  ├─ pthread()线程: ~20行 (1%)
  ├─ ReadData()函数: ~470行 (31%) ← 最大
  ├─ SetReg()函数: ~200行 (13%)
  ├─ readWaveform()函数: ~200行 (13%)
  ├─ 辅助函数: ~350行 (23%)
  └─ 其他: 行

关键函数数量:
  ├─ 导出函数: 7个 (InitDevice, ReadData, SetReg, ...)
  ├─ 静态辅助函数: ~20个
  └─ 硬件库函数指针: 50+个
```

## 🔑 核心接口（导出给设备支持层）

```c
// driverWrapper.h - 驱动层对外接口

// 1. 初始化
long InitDevice();

// 2. 获取I/O扫描私有数据（3个）
IOSCANPVT devGetInTrigScanPvt();
IOSCANPVT devGetInTripBufferScanPvt();
IOSCANPVT devGetInADCrawBufferScanPvt();

// 3. 数据读取（标量）
float ReadData(int offset, int channel, int type);

// 4. 数据写入
void SetReg(int offset, int channel, float val);

// 5. 波形读取
void readWaveform(int offset, int ch_N, unsigned int nelem,
                  float* data, long long *TAI_S, int *TAI_nS);

// 6. 参数管理
void Getparameters(int row, int column, double* data);

// 7. 辅助函数
double amp2power(float amp, int ch_N);
```

**7个函数**解决所有上层需求！这就是外观模式的威力。

## 🧩 主要数据结构

### 1. 全局缓冲区（见Part 3: 04-memory-model.md）

```c
// 正常波形缓冲区 (10000点)
static float rf3amp[buf_len];
static float rf4amp[buf_len];
// ... rf5-rf10

static float rf3phase[buf_len];
// ... 相位缓冲区

static float X1[buf_len], Y1[buf_len];
static float X2[buf_len], Y2[buf_len];

// 历史波形缓冲区 (100000点)
static float HistoryX1[trip_buf_len];
static float HistoryY1[trip_buf_len];
static float HistoryX2[trip_buf_len];
static float HistoryY2[trip_buf_len];

// 标量数据
static float X1_avg, Y1_avg;
static float X2_avg, Y2_avg;
static float ph_ch3, ph_offset3;
// ...
```

### 2. I/O扫描私有数据

```c
static IOSCANPVT TriginScanPvt;           // 触发数据
static IOSCANPVT TripBufferinScanPvt;     // Trip缓冲
static IOSCANPVT ADCrawBufferinScanPvt;   // ADC原始数据
```

### 3. 函数指针（50+个）

```c
// 系统初始化
static int (*funcSystemInit)(void);

// RF信息获取
static int (*funcGetRfInfo)(float *Amp, float *Phase, ...);

// BPM位置
static int (*funcGetxyPosition)(int channel);
static float (*funcGetBPMPhaseValue)(int channel);

// FPGA状态
static int (*funcGetFPGA_LED0_RBK)(void);
static int (*funcGetFPGA_LED1_RBK)(void);

// 数据触发
static int (*funcTriggerAllDataReached)(void);

// ... 还有40+个
```

## 🔄 完整工作流程

### 启动流程

```
1. IOC启动
   └─> iocInit()
       └─> InitDevice()  ← driverWrapper.c
           ├─> scanIoInit() × 3
           ├─> dlopen("libBPMboard14And15.so")
           ├─> dlsym() × 50+  // 获取所有函数指针
           ├─> funcSystemInit()  // 初始化硬件
           └─> pthread_create()  // 创建数据采集线程
               └─> pthread() 开始运行
```

### 运行时数据流

```
数据采集线程（100ms周期）
    ↓
funcTriggerAllDataReached()  // 通知硬件准备数据
    ↓
scanIoRequest(TriginScanPvt)  // 触发I/O中断扫描
    ↓
[EPICS扫描线程]
    ↓
read_ai(prec)  ← devBPMMonitor.c
    ↓
ReadData(offset, channel, type)  ← driverWrapper.c
    ↓
switch(offset)
    case 0: GetRFInfo() → return rf3amp[0]
    case 7: GetXYPosition() → return X1_avg
    ...
    ↓
返回数据给Record
    ↓
prec->val = value
    ↓
通过CA发送给客户端
```

## 📖 阅读源码的建议顺序

### 第一遍：宏观理解

```c
1. 查看头文件和宏定义 (1-32行)
   └─ 了解依赖的库和常量定义

2. 浏览全局变量 (33-200行)
   └─ 了解有哪些数据缓冲区

3. 看函数声明 (driverWrapper.h)
   └─ 了解对外接口

4. 看InitDevice() (250-350行)
   └─ 理解初始化流程

5. 看pthread() (393-410行)
   └─ 理解数据采集流程
```

### 第二遍：深入细节

```c
6. 分析ReadData() (426-900行)
   └─ 理解每个offset的含义

7. 分析SetReg() (901-1100行)
   └─ 理解写操作

8. 分析readWaveform() (1101-1300行)
   └─ 理解波形读取

9. 分析辅助函数 (1301-1539行)
   └─ 理解具体实现
```

### 第三遍：实践验证

```c
10. 添加printf调试
11. 用gdb单步调试
12. 修改参数观察变化
13. 添加新功能
```

## 🎓 学习要点

### 必须理解的概念

1. **动态库加载**
   - dlopen()如何工作
   - dlsym()如何获取函数地址
   - 为什么使用动态加载而不是静态链接

2. **多线程**
   - pthread线程的生命周期
   - 为什么需要单独的数据采集线程
   - 线程同步问题

3. **I/O中断扫描**
   - IOSCANPVT是什么
   - scanIoRequest()的作用
   - 如何触发Record处理

4. **Offset系统**（见Part 3: 05-offset-system.md）
   - 为什么使用offset而不是函数名
   - 如何添加新的offset
   - offset、channel、type的区别

### 可选理解的细节

- CSV参数文件解析
- 时间戳处理（TAI）
- 特定硬件的数据格式
- 历史数据管理

## ✅ 学习检查点

完成本文后，你应该能够回答：

1. **架构理解**：
   - [ ] 驱动层在三层架构中的位置？
   - [ ] 驱动层的主要职责有哪些？
   - [ ] 导出了哪些接口给设备支持层？

2. **代码结构**：
   - [ ] driverWrapper.c有多少行代码？
   - [ ] 有哪些主要函数？
   - [ ] 哪个函数最复杂（代码行数最多）？

3. **工作流程**：
   - [ ] 系统启动时驱动层做了什么？
   - [ ] 数据采集线程的周期是多少？
   - [ ] 数据如何从硬件流向Record？

4. **下一步**：
   - [ ] 应该先学习哪个文档？
   - [ ] 最关键的函数是哪几个？

## 🔗 相关文档

**Part 3回顾**：
- [Part 3: 01-architecture-overview.md](../part3-bpmioc-architecture/01-architecture-overview.md) - 三层架构
- [Part 3: 04-memory-model.md](../part3-bpmioc-architecture/04-memory-model.md) - 内存模型
- [Part 3: 05-offset-system.md](../part3-bpmioc-architecture/05-offset-system.md) - Offset系统
- [Part 3: 06-thread-model.md](../part3-bpmioc-architecture/06-thread-model.md) - 线程模型

**Part 4深入**：
- [02-file-structure.md](./02-file-structure.md) - 文件结构分析（下一篇）
- [04-initdevice.md](./04-initdevice.md) - 初始化详解
- [07-readdata.md](./07-readdata.md) - 数据读取详解

## 📚 扩展阅读

- [EPICS Device Support Guide](https://epics.anl.gov/base/R3-15/6-docs/DeviceSupport.html)
- [Linux dlopen(3) Manual](https://man7.org/linux/man-pages/man3/dlopen.3.html)
- [POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)

---

**下一篇**: [02-file-structure.md](./02-file-structure.md) - 详细分析文件结构

**实践建议**:
1. 打开driverWrapper.c，对照本文档快速浏览一遍
2. 数一数真的有50+个函数指针吗？
3. 找到InitDevice()函数，大概看看做了什么
4. 找到pthread()函数，理解100ms周期
