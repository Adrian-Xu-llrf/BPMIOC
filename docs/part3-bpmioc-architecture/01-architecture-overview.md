# 01 - 三层架构总览

> **目标**: 理解BPMIOC的三层架构设计及其原理
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 60分钟
> **前置知识**: Part 2完整内容

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解什么是分层架构以及为什么使用它
- ✅ 掌握BPMIOC三层的职责和边界
- ✅ 理解各层之间的接口和调用关系
- ✅ 能够判断某个功能应该在哪一层实现
- ✅ 理解这种架构的优势和局限

## 🏗️ 1. 什么是分层架构

### 软件分层的概念

**分层架构**是一种将复杂系统按职责划分为多个层次的设计模式：

```
+------------------+
|   表示层         |  ← 用户界面
+------------------+
|   业务逻辑层     |  ← 核心功能
+------------------+
|   数据访问层     |  ← 数据存储
+------------------+
```

**关键原则**:
1. **单向依赖**: 上层可以调用下层，下层不直接调用上层
2. **职责分离**: 每层只负责特定的工作
3. **接口隔离**: 层与层通过明确的接口通信
4. **可替换性**: 可以替换某一层而不影响其他层

### 为什么需要分层？

以汽车为例：

```
无分层设计（所有功能混在一起）:
❌ 换轮胎需要拆发动机
❌ 修音响需要懂变速箱
❌ 难以维护和升级

分层设计:
✅ 底盘系统 → 只关心行驶
✅ 动力系统 → 只关心动力
✅ 电气系统 → 只关心用电
✅ 各系统独立维护，通过接口交互
```

软件系统同理：
- **可维护性**: 修改某层不影响其他层
- **可测试性**: 可以独立测试每一层
- **可扩展性**: 容易添加新功能
- **可复用性**: 某一层可以在其他项目中复用

## 🎯 2. BPMIOC的三层架构

### 架构总览

BPMIOC采用经典的**三层架构**：

```
+------------------------------------------------+
|           数据库层 (Database Layer)            |
|                                                |
|  - BPMMonitor.db                              |
|  - 记录定义（ai, waveform, calc等）            |
|  - PV命名、扫描、报警配置                      |
|  - 用户可见的所有PV                            |
+------------------------------------------------+
                      ↕
        EPICS标准接口: dset结构体
                      ↕
+------------------------------------------------+
|        设备支持层 (Device Support Layer)        |
|                                                |
|  - devBPMMonitor.c                            |
|  - 实现dset接口                                |
|  - 解析INP/OUT字段                            |
|  - 管理offset系统                              |
|  - 数据格式转换                                |
+------------------------------------------------+
                      ↕
        自定义接口: 函数指针表
                      ↕
+------------------------------------------------+
|           驱动层 (Driver Layer)                |
|                                                |
|  - driverWrapper.c                            |
|  - 硬件抽象                                    |
|  - 动态库加载（dlopen/dlsym）                  |
|  - 线程管理（pthread）                         |
|  - 原始数据采集                                |
+------------------------------------------------+
                      ↕
                  硬件接口
                      ↕
+------------------------------------------------+
|              硬件 (Hardware)                   |
|                                                |
|  - libBPMboard14And15.so                      |
|  - VME总线通信                                 |
|  - BPM电子学板卡                               |
+------------------------------------------------+
```

### 数据流向

```
读取数据（从硬件到客户端）:
Hardware → Driver → Device Support → Database → Channel Access → Client

写入数据（从客户端到硬件）:
Client → Channel Access → Database → Device Support → Driver → Hardware
```

## 📚 3. 三层详解

### 3.1 数据库层 (Database Layer)

**位置**: `BPMmonitorApp/Db/BPMMonitor.db`

**职责**:
1. **定义PV**: 所有可访问的Process Variables
2. **配置扫描**: 何时读取数据（I/O Intr, Periodic等）
3. **设置报警**: 阈值和严重性
4. **记录链接**: PV之间的数据流
5. **数据处理**: 简单计算（使用calc记录）

**示例**:
```
record(ai, "iLinac_007:BPM14And15:RFIn_01_Amp")
{
    field(DESC, "RF Input 1 Amplitude")
    field(SCAN, "I/O Intr")              # ← 扫描类型
    field(DTYP, "BPMmonitor")            # ← 指向设备支持层
    field(INP,  "@AMP:0 ch=0")           # ← 传递给设备支持层
    field(PREC, "3")
    field(EGU,  "V")
    field(HOPR, "10")
    field(LOPR, "0")
    field(HIGH, "8")                     # ← 报警配置
    field(HIHI, "9.5")
}
```

**用户视角**:
- 这是用户唯一能看到的层
- 通过caget/caput访问PV
- 不需要知道底层实现

**设计特点**:
- ✅ 声明式配置（不需要编程）
- ✅ EPICS标准语法
- ✅ 易于修改和扩展
- ⚠️ 不能包含复杂逻辑

### 3.2 设备支持层 (Device Support Layer)

**位置**: `BPMmonitorApp/src/devBPMMonitor.c`

**职责**:
1. **实现EPICS接口**: 提供dset结构体
2. **解析配置**: 解析INP/OUT字段（如"@AMP:0 ch=0"）
3. **offset转换**: 将字符串类型转为数字offset
4. **数据转换**: 格式转换（如double ↔ float）
5. **调用驱动**: 通过函数指针调用驱动层
6. **错误处理**: 返回EPICS状态码

**核心结构**:
```c
// EPICS要求的设备支持接口
typedef struct {
    long      number;       // 函数个数
    DEVSUPFUN report;       // 报告函数
    DEVSUPFUN init;         // 初始化
    DEVSUPFUN init_record;  // 记录初始化
    DEVSUPFUN get_ioint_info; // I/O中断信息
    DEVSUPFUN read_ai;      // 读取ai记录
} dset;

// 对于ai记录，BPMIOC提供：
dset devAiBPMmonitor = {
    5,
    NULL,
    NULL,
    init_ai_record,    // ← 在此解析INP字段
    get_ioint_info,    // ← 在此注册I/O中断
    read_ai            // ← 在此读取数据
};
```

**init_ai_record示例**:
```c
static long init_ai_record(aiRecord *pRec)
{
    // 1. 解析INP字段: "@AMP:0 ch=0"
    //    提取: type_str="AMP", offset=0, channel=0

    // 2. 转换为offset编号
    //    "AMP" → 通过查表 → offsetEnum = 0

    // 3. 保存到DevPvt结构
    DevPvt *pPvt = malloc(sizeof(DevPvt));
    pPvt->offset = 0;
    pPvt->channel = 0;
    pRec->dpvt = pPvt;

    // 4. 返回状态
    return 0;  // 成功
}
```

**read_ai示例**:
```c
static long read_ai(aiRecord *pRec)
{
    DevPvt *pPvt = (DevPvt *)pRec->dpvt;

    // 调用驱动层函数（通过全局函数指针）
    int status = funcReadData(
        pPvt->offset,   // 0 (AMP:0)
        pPvt->channel,  // 0
        &value          // 输出
    );

    if (status == 0) {
        pRec->val = value;  // 设置记录值
        return 0;            // 成功
    } else {
        return -1;           // 失败
    }
}
```

**设计特点**:
- ✅ EPICS标准接口（dset）
- ✅ 与EPICS紧耦合
- ✅ 可移植性好（符合EPICS规范）
- ⚠️ 不直接操作硬件

### 3.3 驱动层 (Driver Layer)

**位置**: `BPMmonitorApp/src/driverWrapper.c`

**职责**:
1. **硬件抽象**: 隐藏硬件实现细节
2. **动态加载**: 运行时加载硬件库（dlopen）
3. **数据采集**: 周期性读取硬件数据
4. **线程管理**: 创建和管理数据采集线程
5. **I/O中断**: 触发EPICS记录处理（scanIoRequest）
6. **缓冲管理**: 维护全局数据数组（Amp[], Phase[]等）

**核心函数**:
```c
// 初始化设备
int InitDevice(int type)
{
    // 1. 动态加载硬件库
    void *handle = dlopen("libBPMboard14And15.so", RTLD_LAZY);

    // 2. 加载所有函数指针
    funcSystemInit = dlsym(handle, "SystemInit");
    funcGetRfInfo = dlsym(handle, "GetRfInfo");
    // ... 加载30+个函数

    // 3. 初始化硬件
    funcSystemInit();

    // 4. 分配全局数组内存
    Amp = (float *)malloc(8 * sizeof(float));
    Phase = (float *)malloc(8 * sizeof(float));
    // ... 更多数组

    // 5. 创建采集线程
    pthread_create(&tid, NULL, DataAcquisitionThread, NULL);

    // 6. 初始化I/O中断扫描
    scanIoInit(&ioScanPvt);

    return 0;
}
```

**数据采集线程**:
```c
void *DataAcquisitionThread(void *arg)
{
    while (1) {
        // 1. 调用硬件函数读取数据
        funcGetRfInfo(Amp, Phase, Power, VBPM, IBPM, VCRI, ICRI, PSET);

        // 2. 触发I/O中断扫描
        //    这会导致所有SCAN="I/O Intr"的记录被处理
        scanIoRequest(ioScanPvt);

        // 3. 延时100ms
        epicsThreadSleep(0.1);
    }
}
```

**读取数据**:
```c
int ReadValue(int offset, int channel, void *pValue)
{
    switch (offset) {
        case 0:  // AMP:0
            *(double *)pValue = (double)Amp[channel];
            break;
        case 1:  // PHASE:1
            *(double *)pValue = (double)Phase[channel];
            break;
        // ... 更多offset类型
    }
    return 0;
}
```

**设计特点**:
- ✅ 硬件无关（通过动态库）
- ✅ 可在运行时切换硬件
- ✅ 独立的线程模型
- ⚠️ 不了解EPICS记录

## 🔄 4. 三层交互

### 4.1 初始化流程

```
IOC启动
  |
  ├─→ 加载数据库（iocInit）
  |     |
  |     └─→ 对每个记录调用 init_record()
  |           |
  |           └─→ devBPMMonitor::init_ai_record()
  |                 |
  |                 ├─→ 第一次调用？
  |                 |     YES → 调用 InitDevice()
  |                 |              |
  |                 |              ├─→ dlopen加载库
  |                 |              ├─→ funcSystemInit()
  |                 |              ├─→ malloc分配数组
  |                 |              ├─→ pthread_create()
  |                 |              └─→ scanIoInit()
  |                 |
  |                 ├─→ 解析INP字段
  |                 ├─→ 分配DevPvt
  |                 └─→ 返回成功
  |
  └─→ 启动扫描线程
        |
        └─→ 驱动层线程开始运行
              |
              └─→ 周期性触发scanIoRequest()
```

### 4.2 读取数据流程（I/O Interrupt）

```
硬件数据可用
  ↓
DataAcquisitionThread线程被唤醒
  ↓
调用 funcGetRfInfo(Amp, Phase, ...)
  ↓
更新全局数组: Amp[0]=3.14, Phase[0]=45.6, ...
  ↓
调用 scanIoRequest(ioScanPvt)  ← 驱动层
  ↓
EPICS扫描线程被触发
  ↓
对所有 SCAN="I/O Intr" 的记录:
  ↓
调用 devBPMMonitor::read_ai(pRec)  ← 设备支持层
  ↓
  ├─→ 从pRec->dpvt获取DevPvt
  ├─→ 调用 ReadValue(offset, channel, &value)  ← 驱动层
  ├─→ 设置 pRec->val = value
  └─→ 返回成功
  ↓
EPICS处理记录
  ↓
  ├─→ 应用ASLO/AOFF转换
  ├─→ 检查报警条件
  ├─→ 更新时间戳
  ├─→ 处理FLNK链接
  └─→ 通知CA客户端
  ↓
客户端收到更新（camonitor显示新值）
```

### 4.3 周期扫描流程（Periodic Scan）

```
EPICS扫描线程（.5 second定时器）
  ↓
时间到 → 触发记录处理
  ↓
调用 devBPMMonitor::read_ai(pRec)  ← 设备支持层
  ↓
调用 ReadValue(offset, channel, &value)  ← 驱动层
  ↓
从全局数组读取: value = Amp[channel]
  ↓
返回到设备支持层
  ↓
设置 pRec->val = value
  ↓
EPICS处理记录（同上）
```

**关键区别**:
- **I/O Intr**: 数据驱动，硬件有新数据时立即处理
- **Periodic**: 时间驱动，定时读取当前缓存的数据

## 🎓 5. 层次边界和接口

### 5.1 数据库层 ↔ 设备支持层

**接口**: EPICS标准dset结构体

```c
// 数据库层 → 设备支持层
field(DTYP, "BPMmonitor")  // 指定使用哪个dset
field(INP,  "@AMP:0 ch=0") // 传递配置字符串

// 设备支持层提供的接口
dset devAiBPMmonitor = {
    5,
    NULL,
    NULL,
    init_ai_record,     // 初始化时调用
    get_ioint_info,     // 注册I/O中断时调用
    read_ai             // 读取时调用
};
```

**数据传递**:
- **输入**: INP字段字符串（如"@AMP:0 ch=0"）
- **输出**: 记录的val字段（double类型）
- **返回值**: EPICS状态码（0=成功, -1=失败）

**职责分工**:
- 数据库层：声明**要什么**（PV名、扫描类型、报警阈值）
- 设备支持层：实现**怎么拿**（解析配置、调用驱动、格式转换）

### 5.2 设备支持层 ↔ 驱动层

**接口**: 自定义函数指针（全局变量）

```c
// 驱动层提供的接口（全局函数指针）
extern int (*funcReadData)(int offset, int channel, void *pValue);
extern int (*funcWriteData)(int offset, int channel, void *pValue);

// 设备支持层调用
static long read_ai(aiRecord *pRec)
{
    DevPvt *pPvt = pRec->dpvt;
    double value;

    // 调用驱动层
    int status = funcReadData(pPvt->offset, pPvt->channel, &value);

    if (status == 0) {
        pRec->val = value;
    }
    return status;
}
```

**数据传递**:
- **输入**: offset（数据类型编号）, channel（通道号）
- **输出**: void *指针（指向value缓冲区）
- **返回值**: 0=成功, 非0=失败

**职责分工**:
- 设备支持层：解析配置，知道**读什么**（offset+channel）
- 驱动层：实现读取，知道**从哪读**（全局数组）

### 5.3 驱动层 ↔ 硬件

**接口**: 动态库导出的函数

```c
// 硬件库导出的函数（libBPMboard14And15.so）
int SystemInit(void);
int GetRfInfo(float *Amp, float *Phase, float *Power, ...);
int SetXxxParam(int channel, float value);
// ... 30+个函数

// 驱动层动态加载
void *handle = dlopen("libBPMboard14And15.so", RTLD_LAZY);
funcSystemInit = dlsym(handle, "SystemInit");
funcGetRfInfo = dlsym(handle, "GetRfInfo");

// 调用
funcSystemInit();
funcGetRfInfo(Amp, Phase, Power, ...);
```

**数据传递**:
- **输入**: 命令参数（通道号、设置值等）
- **输出**: 硬件数据（通过指针传递数组）
- **返回值**: 0=成功, 非0=失败

**职责分工**:
- 驱动层：管理数据缓冲、线程、I/O中断
- 硬件层：VME通信、寄存器读写、硬件控制

## 💡 6. 架构优势

### 6.1 模块化

```
示例：更换硬件
❌ 无分层: 需要修改所有代码
✅ 有分层: 只需替换驱动层的.so文件

示例：添加新PV
❌ 无分层: 需要修改硬件驱动代码
✅ 有分层: 只需在.db文件中添加记录
```

### 6.2 可测试性

```
测试数据库层:
  → 可以mock设备支持层
  → 测试记录链接、报警逻辑

测试设备支持层:
  → 可以mock驱动层
  → 测试INP解析、offset转换

测试驱动层:
  → 可以mock硬件库
  → 测试线程、缓冲管理
```

### 6.3 可维护性

```
修改报警阈值:
  → 只需修改.db文件
  → 不需要重新编译

优化数据采集频率:
  → 只需修改driverWrapper.c
  → 不影响数据库层和设备支持层

添加新的offset类型:
  → 修改devBPMMonitor.c和driverWrapper.c
  → .db文件自动支持新类型
```

### 6.4 可扩展性

```
添加新功能的难度:

数据库层（最容易）:
  - 添加calc记录做数据处理
  - 添加新PV监控新参数
  - 修改扫描周期

设备支持层（中等）:
  - 支持新的记录类型（如waveform）
  - 添加新的offset类型
  - 优化数据转换逻辑

驱动层（较难）:
  - 支持新硬件
  - 修改线程模型
  - 添加新的数据采集方式
```

## ⚠️ 7. 架构局限

### 7.1 性能开销

```
数据必须经过三层:
Hardware → Driver → Device Support → Database → CA → Client

每一层都有开销:
  - 函数调用
  - 数据复制
  - 格式转换

对于高吞吐量应用可能成为瓶颈
```

### 7.2 复杂性

```
简单功能也需要修改三层:
  1. 修改.db添加PV
  2. 修改devBPMMonitor.c添加offset解析
  3. 修改driverWrapper.c添加数据读取

学习曲线陡峭
```

### 7.3 调试困难

```
问题可能出现在任何一层:
  - 数据库层：配置错误
  - 设备支持层：offset映射错误
  - 驱动层：线程同步问题
  - 硬件层：VME通信故障

需要理解整个调用链才能有效调试
```

## 🔍 8. 实际示例：完整追踪

### 场景：读取RF3的幅度值

**步骤1: 客户端请求**
```bash
$ caget iLinac_007:BPM14And15:RFIn_01_Amp
```

**步骤2: 数据库层**
```
record(ai, "iLinac_007:BPM14And15:RFIn_01_Amp")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")  # ← 使用devAiBPMmonitor
    field(INP,  "@AMP:0 ch=0") # ← 配置参数
}
```

**步骤3: 设备支持层 (devBPMMonitor.c)**
```c
// init_ai_record时已经解析了INP:
// DevPvt { offset=0, channel=0 }

// read_ai被调用:
static long read_ai(aiRecord *pRec)
{
    DevPvt *pPvt = pRec->dpvt;  // offset=0, channel=0
    double value;

    // 调用驱动层
    ReadValue(0, 0, &value);

    pRec->val = value;
    return 0;
}
```

**步骤4: 驱动层 (driverWrapper.c)**
```c
int ReadValue(int offset, int channel, void *pValue)
{
    switch (offset) {
        case 0:  // AMP:0
            *(double *)pValue = (double)Amp[channel];
            //                           Amp[0] = 3.14
            break;
    }
    return 0;
}
```

**步骤5: 全局数组**
```c
// Amp数组由DataAcquisitionThread线程定期更新
float Amp[8];  // Amp[0] = 3.14 (最近一次采集的值)
```

**步骤6: 硬件读取**
```c
void *DataAcquisitionThread(void *arg)
{
    while (1) {
        // 调用硬件库
        funcGetRfInfo(Amp, Phase, ...);
        // 硬件库内部通过VME总线读取BPM板卡寄存器
        // 更新Amp[0] = 3.14

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.1);
    }
}
```

**步骤7: 返回客户端**
```bash
iLinac_007:BPM14And15:RFIn_01_Amp    3.14
```

**完整调用链**:
```
caget
  ↓
Channel Access网络协议
  ↓
EPICS数据库: aiRecord->val
  ↓
devAiBPMmonitor::read_ai()
  ↓
ReadValue(offset=0, channel=0)
  ↓
Amp[0]
  ↑
funcGetRfInfo()
  ↑
libBPMboard14And15.so
  ↑
VME总线
  ↑
BPM硬件
```

## 📊 9. 对比其他架构

### 单层架构（所有代码在一起）

```
优点:
  ✅ 简单直接
  ✅ 性能好
  ✅ 易于理解

缺点:
  ❌ 难以维护
  ❌ 难以测试
  ❌ 不可重用
  ❌ 硬件变化需要大量修改
```

### 两层架构（数据库+驱动，无设备支持层）

```
优点:
  ✅ 比三层简单
  ✅ 性能稍好

缺点:
  ❌ 数据库直接调用驱动（耦合）
  ❌ 不符合EPICS规范
  ❌ 难以支持多种记录类型
```

### 四层架构（增加业务逻辑层）

```
优点:
  ✅ 更清晰的职责分离
  ✅ 复杂逻辑独立管理

缺点:
  ❌ 更复杂
  ❌ 性能开销更大
  ❌ 对于BPMIOC过度设计
```

### BPMIOC选择三层的原因

1. **符合EPICS规范**: 标准的dset接口
2. **平衡复杂度和灵活性**: 不太简单也不太复杂
3. **硬件抽象**: 通过动态库实现硬件独立
4. **可维护性**: 大部分修改只涉及一层

## ✅ 学习检查点

完成本节后，你应该能够回答：

### 基础理解
- [ ] 什么是分层架构？为什么要用分层？
- [ ] BPMIOC的三层分别是哪三层？
- [ ] 每一层的主要职责是什么？

### 接口理解
- [ ] 数据库层如何调用设备支持层？（提示：dset）
- [ ] 设备支持层如何调用驱动层？（提示：函数指针）
- [ ] 驱动层如何访问硬件？（提示：dlopen/dlsym）

### 数据流理解
- [ ] 从硬件到客户端，数据经过哪些步骤？
- [ ] I/O中断扫描是在哪一层触发的？
- [ ] 全局数组Amp[]在哪一层定义？谁更新它？

### 设计理解
- [ ] 为什么不直接在数据库层访问硬件？
- [ ] 如果要更换硬件，需要修改哪些文件？
- [ ] DevPvt结构的作用是什么？

## 🚀 下一步

现在你已经理解了三层架构的总体设计，接下来：

👉 [02-data-flow.md](./02-data-flow.md) - 详细追踪数据在三层中的流动

或者深入某一层：
- [Part 4: Driver Layer](../part4-driver-layer/) - 驱动层详解
- [Part 5: Device Support Layer](../part5-device-support-layer/) - 设备支持层详解
- [Part 6: Database Layer](../part6-database-layer/) - 数据库层详解

---

**💡 思考题**:

1. 如果要添加一个新的PV监控CPU温度，需要修改哪些文件？
2. 假设硬件从VME总线换成以太网，哪些层需要修改？
3. 为什么全局数组Amp[]定义在驱动层而不是设备支持层？

**⏱️ 建议**: 花30分钟重新阅读BPMIOC的三个核心文件（BPMMonitor.db, devBPMMonitor.c, driverWrapper.c），尝试识别每个文件属于哪一层，以及它们如何交互。
