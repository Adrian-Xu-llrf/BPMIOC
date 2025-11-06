# BPMIOC 技术文档

## 目录
1. [项目概述](#项目概述)
2. [系统架构](#系统架构)
3. [核心组件详解](#核心组件详解)
4. [EPICS基础知识](#epics基础知识)
5. [数据库设计](#数据库设计)
6. [构建系统](#构建系统)
7. [运行与调试](#运行与调试)
8. [开发指南](#开发指南)

---

## 项目概述

### 项目定位
**BPMIOC** 是一个用于加速器物理实验的 **EPICS IOC (Input/Output Controller)** 应用程序,专门用于 **BPM (Beam Position Monitor)** 光束位置监测系统。

### 应用场景
- **系统**: iLinac 加速器控制系统
- **监测对象**: BPM14 和 BPM15 两个光束位置监测单元
- **目标平台**: linux-arm (ARM 架构的 Linux 系统)

### 核心功能
| 功能模块 | 描述 | 技术特点 |
|---------|------|---------|
| **RF监测** | 监测 RF3-RF10 共8个通道的振幅和相位 | 实时采集,0.5秒更新 |
| **光束位置计算** | 基于 BPM 探测器数据计算 X-Y 坐标 | FPGA 硬件计算 |
| **功率转换** | RF 振幅转换为功率(KW) | CSV 校准文件 |
| **波形采集** | 触发和历史波形数据采集 | 10000-100000点缓冲 |
| **波形分析** | 平均电压计算(含背景扣除) | 最新添加功能 |
| **实时监控** | FPGA 状态、告警监控 | I/O 中断驱动 |

---

## 系统架构

### 三层架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    EPICS 数据库层                        │
│  BPMMonitor.db (1891行) + BPMCal.db (155行)            │
│  - PV记录定义 (ai, ao, bi, bo, waveform)               │
│  - 扫描机制配置                                         │
│  - 参数化宏定义 $(P), $(P1), $(P2)                      │
└────────────────┬────────────────────────────────────────┘
                 │ record → device support
                 ▼
┌─────────────────────────────────────────────────────────┐
│                 EPICS 设备支持层                         │
│  devBPMMonitor.c (423行)                                │
│  - 7个设备支持表 (DSET)                                 │
│  - 记录类型适配 (ai, ao, bi, bo, waveform)             │
│  - 参数解析: "TYPE:offset ch=channel"                  │
│  - I/O 中断扫描                                         │
└────────────────┬────────────────────────────────────────┘
                 │ DSET函数调用
                 ▼
┌─────────────────────────────────────────────────────────┐
│                    驱动层                                │
│  driverWrapper.c (1540行)                               │
│  - 硬件抽象接口                                         │
│  - 数据读取: ReadData(offset, channel, type)           │
│  - 寄存器写入: SetReg(offset, channel, value)          │
│  - 波形采集: readWaveform()                            │
│  - 平均电压: calculateAvgVoltage()                     │
│  - 功率转换: amp2power()                               │
└────────────────┬────────────────────────────────────────┘
                 │ dlopen/dlsym动态加载
                 ▼
┌─────────────────────────────────────────────────────────┐
│                 硬件抽象层                               │
│  liblowlevel.so (外部库)                                │
│  - SystemInit(), GetRfInfo(), SetDO()                  │
│  - GetTriggerAllData(), GetHistoryChannelData()        │
│  - BPM计算函数集                                        │
│  - 50+ 底层硬件函数                                     │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
         [ FPGA 硬件 / 加速器控制器 ]
```

### 数据流向

#### 读取流程 (以 RF 振幅为例)
```
FPGA 寄存器
   ↓
liblowlevel.so::GetRfInfo(channel, &amp, &phase)
   ↓
driverWrapper.c::ReadData(OFFSET_AMP, channel, TYPE_AMP)
   ↓
devBPMMonitor.c::read_ai() → DSET::read_ai
   ↓
EPICS Database: ai记录更新
   ↓
Channel Access (CA) → 客户端 (CSS, Python, ...)
```

#### 写入流程 (以寄存器设置为例)
```
客户端 (CSS/Python) → 设置 ao记录
   ↓
EPICS Database: process ao记录
   ↓
devBPMMonitor.c::write_ao() → DSET::write_ao
   ↓
driverWrapper.c::SetReg(offset, channel, value)
   ↓
liblowlevel.so::SetXXX(channel, value)
   ↓
FPGA 寄存器写入
```

---

## 核心组件详解

### 1. 驱动层 (driverWrapper.c)

#### 1.1 初始化流程

```c
// 文件: BPMmonitorApp/src/driverWrapper.c

// 系统初始化入口
int InitDevice()
{
    // 1. 动态加载底层库
    handle = dlopen("/usr/lib/liblowlevel.so", RTLD_LAZY);

    // 2. 获取函数指针 (50+ 个函数)
    *(void **)(&systemInitFunc) = dlsym(handle, "SystemInit");
    *(void **)(&getRfInfoFunc) = dlsym(handle, "GetRfInfo");
    // ... 更多函数指针

    // 3. 初始化 FPGA 系统
    (*systemInitFunc)();

    // 4. 加载校准参数
    Amp2PowerInit();     // 从 CSV 加载功率转换表
    LoadParam();         // 加载 BPM 校准参数

    // 5. 创建后台扫描线程
    epicsThreadCreate("BPMMonitor", 50, 20000,
                      (EPICSTHREADFUNC)my_thread, NULL);

    return 0;
}
```

**关键点**:
- 使用 `dlopen()` 实现动态链接,避免编译时依赖
- 函数指针保存在全局变量中,供其他函数调用
- `epicsThreadCreate()` 创建 EPICS 线程,执行周期性扫描

#### 1.2 数据读取接口

```c
// 核心读取函数
// offset: 数据类型偏移 (34种不同的数据类型)
// channel: 通道号 (0-7 for RF, 0-1 for BPM)
// type: 数据类型 (AMP=1, PHASE=2, REG=0, ...)
double ReadData(int offset, int channel, int type)
{
    double value = 0.0;

    switch(offset) {
        case OFFSET_AMP:  // RF 振幅
            if (channel >= 0 && channel < 8) {
                value = Amp[channel];  // 从全局缓冲读取
            }
            break;

        case OFFSET_PHASE:  // RF 相位
            value = Phase[channel];
            break;

        case OFFSET_VC_VALUE:  // BPM Vc 值
            value = (*getVcValueFunc)(channel);  // 调用底层库
            break;

        case OFFSET_POWER:  // 功率转换
            value = amp2power(Amp[channel], channel);
            break;

        case OFFSET_AVG_VOLTAGE:  // 平均电压 (新功能)
            value = AVG_Voltage[channel];
            break;

        // ... 30+ 其他数据类型
    }

    return value;
}
```

**Offset 定义示例**:
```c
#define OFFSET_AMP              0   // RF 振幅
#define OFFSET_PHASE            1   // RF 相位
#define OFFSET_DI               2   // 数字输入
#define OFFSET_VC_VALUE         9   // BPM Vc 值
#define OFFSET_X_POS            10  // X 位置
#define OFFSET_Y_POS            11  // Y 位置
#define OFFSET_POWER            17  // 功率
#define OFFSET_AVG_VOLTAGE      34  // 平均电压
// ... 更多定义见 driverWrapper.h
```

#### 1.3 波形平均电压计算 (最新功能)

```c
// 计算波形平均电压,扣除背景
// channel: RF 通道号 (0-7)
void calculateAvgVoltage(int channel)
{
    if (channel < 0 || channel >= 8) return;

    double sum_signal = 0.0;
    double sum_background = 0.0;
    int count_signal = 0;
    int count_background = 0;

    // 1. 获取波形数据
    float *waveform = TrigWaveform[channel];  // 10000点

    // 2. 计算信号区域平均值
    for (int i = AVGStart[channel]; i <= AVGStop[channel]; i++) {
        sum_signal += waveform[i];
        count_signal++;
    }

    // 3. 计算背景区域平均值
    for (int i = BackGroundStart[channel]; i <= BackGroundStop[channel]; i++) {
        sum_background += waveform[i];
        count_background++;
    }

    // 4. 扣除背景
    double avg_signal = (count_signal > 0) ? (sum_signal / count_signal) : 0.0;
    double avg_background = (count_background > 0) ? (sum_background / count_background) : 0.0;

    AVG_Voltage[channel] = avg_signal - avg_background;
}
```

**使用场景**: 用于分析触发波形,提取有效信号幅度并扣除噪声本底。

#### 1.4 后台扫描线程

```c
static void my_thread(void *arg)
{
    while (1) {
        // 1. 读取 RF 信息 (所有通道)
        for (int i = 0; i < 8; i++) {
            (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
        }

        // 2. 更新时间戳
        (*getTimeStampFunc)(&TAIsec, &TAInsec);

        // 3. 触发 I/O 中断扫描
        scanIoRequest(ioScanPvt);

        // 4. 休眠 100ms
        epicsThreadSleep(0.1);
    }
}
```

**工作原理**:
- 每 100ms 从 FPGA 读取最新数据到全局缓冲区
- 调用 `scanIoRequest()` 触发 EPICS 记录扫描
- 使用 I/O 中断机制,避免轮询

---

### 2. 设备支持层 (devBPMMonitor.c)

#### 2.1 设备支持表 (DSET)

EPICS 使用 DSET (Device Support Entry Table) 结构定义设备驱动接口:

```c
// 模拟输入 (ai) 设备支持
typedef struct {
    long number;           // 函数数量
    DEVSUPFUN report;      // 设备报告
    DEVSUPFUN init;        // 驱动初始化
    DEVSUPFUN init_record; // 记录初始化
    DEVSUPFUN get_ioint_info; // I/O中断信息
    DEVSUPFUN read_ai;     // 读取函数 ★核心★
    DEVSUPFUN special_linconv; // 线性转换
} AI_DSET;

// 定义 ai 设备支持
AI_DSET devAiBPMMonitor = {
    6,
    NULL,
    NULL,
    init_ai_record,
    get_ioint_info,
    read_ai,
    NULL
};
```

**导出到 EPICS 数据库**:
```c
// 文件: devBPMMonitor.dbd
device(ai, INST_IO, devAiBPMMonitor, "BPMMonitor")
device(ao, INST_IO, devAoBPMMonitor, "BPMMonitor")
device(bi, INST_IO, devBiBPMMonitor, "BPMMonitor")
device(bo, INST_IO, devBoBPMMonitor, "BPMMonitor")
device(waveform, INST_IO, devTrigWaveformBPMMonitor, "BPMmonitorTrigWave")
device(waveform, INST_IO, devHistoryWaveformBPMMonitor, "BPMmonitorTripWave")
device(waveform, INST_IO, devADCRawDataWaveformBPMMonitor, "BPMmonitorADCWave")
```

#### 2.2 记录初始化

```c
// ai 记录初始化
static long init_ai_record(struct aiRecord *prec)
{
    struct instio *pinstio;
    BPMMonitorPvt *pPvt;

    // 1. 分配私有数据结构
    pPvt = (BPMMonitorPvt *)malloc(sizeof(BPMMonitorPvt));
    prec->dpvt = pPvt;

    // 2. 解析 INP 字段
    // 格式: "TYPE:offset ch=channel"
    // 示例: "AMP:0 ch=3" → TYPE=AMP, offset=0, channel=3
    pinstio = (struct instio *)&(prec->inp.value);
    char *params = pinstio->string;

    sscanf(params, "%[^:]:%d ch=%d",
           pPvt->type_str, &pPvt->offset, &pPvt->channel);

    // 3. 确定数据类型
    if (strcmp(pPvt->type_str, "AMP") == 0)
        pPvt->type = TYPE_AMP;
    else if (strcmp(pPvt->type_str, "PHASE") == 0)
        pPvt->type = TYPE_PHASE;
    // ... 其他类型

    return 0;
}
```

#### 2.3 读取函数

```c
// ai 记录读取
static long read_ai(struct aiRecord *prec)
{
    BPMMonitorPvt *pPvt = (BPMMonitorPvt *)prec->dpvt;

    // 调用驱动层读取数据
    double value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 更新记录值
    prec->val = value;
    prec->udf = FALSE;

    return 2; // 返回2表示不需要转换
}
```

#### 2.4 I/O 中断支持

```c
extern IOSCANPVT ioScanPvt;  // 定义在 driverWrapper.c

// 提供 I/O 中断信息
static long get_ioint_info(int cmd, struct aiRecord *prec, IOSCANPVT *ppvt)
{
    *ppvt = ioScanPvt;
    return 0;
}
```

**工作流程**:
1. 后台线程调用 `scanIoRequest(ioScanPvt)`
2. EPICS 扫描器触发所有使用该 IOSCANPVT 的记录
3. 调用 `read_ai()` 更新记录值

---

### 3. 数据库层 (BPMMonitor.db)

#### 3.1 参数化宏

数据库使用 3 个宏变量实现多实例配置:

```
$(P)  - 通用 PV 前缀     → iLinac_007:BPM14And15
$(P1) - BPM1 通道前缀    → iLinac_007:BPM14
$(P2) - BPM2 通道前缀    → iLinac_007:BPM15
```

#### 3.2 典型记录示例

**RF 振幅监测**:
```
record(ai, "$(P):RF3Amp") {
    field(DTYP, "BPMMonitor")          # 设备类型
    field(INP,  "@AMP:0 ch=0")         # 输入参数
    field(SCAN, "I/O Intr")            # I/O 中断扫描
    field(PREC, "3")                   # 精度 3 位小数
    field(EGU,  "V")                   # 工程单位: 伏特
}
```

**参数解析**:
- `@AMP:0 ch=0` → type=AMP, offset=0, channel=0
- 调用 `ReadData(0, 0, TYPE_AMP)` → 返回 RF3 振幅

**RF 功率监测**:
```
record(ai, "$(P):RF3Power") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@POWER:17 ch=0")      # offset=17, channel=0
    field(SCAN, ".5 second")           # 0.5秒扫描
    field(PREC, "3")
    field(EGU,  "KW")                  # 千瓦
}
```

**波形平均电压** (新功能):
```
record(ai, "$(P):RF3AVGVoltage") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@AVG_VOLTAGE:34 ch=0")
    field(SCAN, "I/O Intr")
    field(PREC, "3")
    field(EGU,  "V")
}
```

**平均电压范围设置**:
```
record(ao, "$(P):RF3AVGStart") {
    field(DTYP, "BPMMonitor")
    field(OUT,  "@REG:44 ch=0")        # 设置起始位置
    field(PREC, "0")
    field(DRVL, "0")                   # 下限 0
    field(DRVH, "10000")               # 上限 10000
}

record(ao, "$(P):RF3AVGStop") {
    field(DTYP, "BPMMonitor")
    field(OUT,  "@REG:45 ch=0")        # 设置结束位置
    field(PREC, "0")
    field(DRVL, "0")
    field(DRVH, "10000")
}
```

**触发波形采集**:
```
record(waveform, "$(P):RF3TrigWaveform") {
    field(DTYP, "BPMmonitorTrigWave")  # 专用设备类型
    field(INP,  "@0 ch=0")             # offset=0, channel=0
    field(SCAN, "I/O Intr")
    field(FTVL, "FLOAT")               # 浮点数组
    field(NELM, "10000")               # 10000 个元素
}
```

#### 3.3 完整的 RF 通道配置

8 个 RF 通道 (RF3-RF10) 每个包含:
- 振幅 (Amp)
- 相位 (Phase)
- 功率 (Power)
- 触发波形 (TrigWaveform)
- 历史波形 (TripWaveform)
- 平均电压 (AVGVoltage)
- 平均范围设置 (AVGStart, AVGStop)
- 背景范围设置 (BackGroundStart, BackGroundStop)

**总计**: 每通道 ~9 条记录 × 8 通道 = 72+ 条核心记录

---

## EPICS 基础知识

### 1. EPICS 是什么?

**EPICS** (Experimental Physics and Industrial Control System) 是一个用于构建大型分布式控制系统的开源软件工具集。

**核心组件**:
- **IOC** (Input/Output Controller): 实时数据采集和控制
- **Channel Access** (CA): 网络通信协议
- **OPI** (Operator Interface): 操作界面 (CSS, EDM, etc.)

### 2. EPICS 记录类型

| 记录类型 | 简称 | 用途 | 示例 |
|---------|------|------|------|
| Analog Input | ai | 模拟输入 | 温度、电压、电流 |
| Analog Output | ao | 模拟输出 | 设定值、控制参数 |
| Binary Input | bi | 二进制输入 | 开关状态、告警 |
| Binary Output | bo | 二进制输出 | 开关控制 |
| Waveform | waveform | 波形数组 | 示波器数据 |
| Calc | calc | 计算记录 | 公式计算 |
| String Input | stringin | 字符串输入 | 设备名称 |

### 3. 扫描机制

| 扫描类型 | 描述 | 示例 |
|---------|------|------|
| Passive | 被动扫描,仅在被其他记录触发时处理 | `SCAN: Passive` |
| I/O Intr | I/O 中断,由设备驱动触发 | `SCAN: I/O Intr` |
| 定时扫描 | 周期性扫描 | `SCAN: .5 second` |
| Event | 事件驱动 | `SCAN: Event` |

### 4. 设备支持 (Device Support)

设备支持是连接 EPICS 记录和硬件的桥梁:

```
EPICS Record
   ↓
Record Support (通用,由 EPICS Base 提供)
   ↓
Device Support (硬件特定,用户实现) ← 本项目实现
   ↓
Driver Layer (可选)
   ↓
Hardware
```

---

## 数据库设计

### 完整 PV 清单

以 BPM14And15 系统为例,主要 PV 包括:

#### 1. RF 监测 (8 通道 × 每通道 9+ PV)

**RF3 通道示例**:
```
iLinac_007:BPM14And15:RF3Amp             # 振幅
iLinac_007:BPM14And15:RF3Phase           # 相位
iLinac_007:BPM14And15:RF3Power           # 功率
iLinac_007:BPM14And15:RF3TrigWaveform    # 触发波形
iLinac_007:BPM14And15:RF3TripWaveform    # 历史波形
iLinac_007:BPM14And15:RF3AVGVoltage      # 平均电压
iLinac_007:BPM14And15:RF3AVGStart        # 平均起始
iLinac_007:BPM14And15:RF3AVGStop         # 平均结束
iLinac_007:BPM14And15:RF3BackGroundStart # 背景起始
iLinac_007:BPM14And15:RF3BackGroundStop  # 背景结束
```

**其他 RF 通道**: RF4, RF5, RF6, RF7, RF8, RF9, RF10

#### 2. BPM 位置监测 (2 个 BPM × 每个 10+ PV)

**BPM14 示例**:
```
iLinac_007:BPM14:VcValue                 # Vc 值
iLinac_007:BPM14:XPos                    # X 位置
iLinac_007:BPM14:YPos                    # Y 位置
iLinac_007:BPM14:SumValue                # Sum 值
iLinac_007:BPM14:XTripWaveform           # X 历史波形
iLinac_007:BPM14:YTripWaveform           # Y 历史波形
iLinac_007:BPM14:ProtectStatus           # 保护状态
# ... 更多 BPM 参数
```

#### 3. 系统控制

```
iLinac_007:BPM14And15:TripHistoryTrig    # 触发历史数据
iLinac_007:BPM14And15:DataRatio          # 数据比例
iLinac_007:BPM14And15:ParamRead          # 参数读取
iLinac_007:BPM14And15:LED1Status         # LED1 状态
iLinac_007:BPM14And15:LED2Status         # LED2 状态
iLinac_007:BPM14And15:HistoryDataReady   # 历史数据就绪
```

**总计**: 约 **200+ PV**

---

## 构建系统

### 目录结构

```
BPMIOC/
├── configure/              # 构建配置
│   ├── CONFIG              # 架构和编译器配置
│   ├── CONFIG_SITE         # 站点特定设置
│   ├── RELEASE             # 模块依赖
│   ├── RULES*              # Makefile 规则
│   └── Makefile
├── BPMmonitorApp/          # 应用程序
│   ├── src/                # 源代码
│   │   ├── Makefile        # 编译规则
│   │   ├── *.c, *.h        # C 源文件
│   │   ├── *.cpp           # C++ 源文件
│   │   └── *.dbd           # 数据库定义
│   ├── Db/                 # 数据库文件
│   │   ├── Makefile
│   │   └── *.db            # 数据库实例
│   └── Makefile
├── iocBoot/                # 启动脚本
│   ├── iocBPMmonitor/
│   │   ├── st.cmd          # 启动命令
│   │   └── Makefile
│   └── Makefile
└── Makefile                # 顶层 Makefile
```

### 构建流程

#### 1. 配置依赖 (configure/RELEASE)

```makefile
# EPICS 基础路径
EPICS_BASE=/home/llrf/epics/base-3.15.6

# 如果依赖其他模块,在此添加:
# ASYN=$(EPICS_BASE)/../asyn-4-35
# STREAM=$(EPICS_BASE)/../stream-2-8
```

#### 2. 应用程序构建 (BPMmonitorApp/src/Makefile)

```makefile
TOP=../..
include $(TOP)/configure/CONFIG

# 1. 生成可执行文件
PROD_IOC = BPMmonitor

# 2. 数据库定义文件
DBD += BPMmonitor.dbd

# 3. 包含基础 DBD
BPMmonitor_DBD += base.dbd
BPMmonitor_DBD += devBPMMonitor.dbd

# 4. 源文件列表
BPMmonitor_SRCS += BPMmonitor_registerRecordDeviceDriver.cpp
BPMmonitor_SRCS += BPMmonitorMain.cpp
BPMmonitor_SRCS += driverWrapper.c
BPMmonitor_SRCS += devBPMMonitor.c

# 5. 链接库
BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)
BPMmonitor_SYS_LIBS += dl pthread

include $(TOP)/configure/RULES
```

#### 3. 数据库安装 (BPMmonitorApp/Db/Makefile)

```makefile
TOP=../..
include $(TOP)/configure/CONFIG

# 安装数据库文件
DB += BPMMonitor.db
DB += BPMCal.db

include $(TOP)/configure/RULES
```

### 编译命令

```bash
# 清理
make clean

# 完整构建
make

# 仅构建应用
cd BPMmonitorApp
make
```

**输出位置**:
- 可执行文件: `bin/linux-arm/BPMmonitor`
- 数据库文件: `db/BPMMonitor.db`, `db/BPMCal.db`
- DBD 文件: `dbd/BPMmonitor.dbd`

---

## 运行与调试

### 启动 IOC

```bash
cd iocBoot/iocBPMmonitor
chmod +x st.cmd
./st.cmd
```

**启动脚本内容**:
```bash
#!/../../bin/linux-arm/BPMmonitor

# 进入 IOC shell
< envPaths

cd "${TOP}"

# 加载 DBD
dbLoadDatabase("dbd/BPMmonitor.dbd")
BPMmonitor_registerRecordDeviceDriver(pdbbase)

# 加载数据库
dbLoadRecords("db/BPMMonitor.db",
  "P=iLinac_007:BPM14And15,P1=iLinac_007:BPM14,P2=iLinac_007:BPM15")
dbLoadRecords("db/BPMCal.db",
  "P=iLinac_007:BPM14And15,P1=iLinac_007:BPM14,P2=iLinac_007:BPM15")

# 初始化
iocInit()

# 可选: 打印所有 PV
dbl
```

### IOC Shell 常用命令

启动后进入交互式 IOC Shell:

```bash
# 1. 列出所有记录
epics> dbl

# 2. 查看记录详细信息
epics> dbpr "iLinac_007:BPM14And15:RF3Amp", 2

# 3. 读取 PV 值
epics> dbgf "iLinac_007:BPM14And15:RF3Amp"

# 4. 设置 PV 值
epics> dbpf "iLinac_007:BPM14And15:RF3AVGStart" "100"

# 5. 查看驱动信息
epics> drvReport 2

# 6. 查看线程状态
epics> epicsThreadShowAll

# 7. 退出
epics> exit
```

### 远程访问 PV

使用 EPICS 客户端工具:

#### 1. caget (读取)
```bash
caget iLinac_007:BPM14And15:RF3Amp
```

#### 2. caput (写入)
```bash
caput iLinac_007:BPM14And15:RF3AVGStart 100
```

#### 3. camonitor (监控)
```bash
camonitor iLinac_007:BPM14And15:RF3Amp
```

#### 4. Python (pyepics)
```python
import epics

# 读取
amp = epics.caget('iLinac_007:BPM14And15:RF3Amp')
print(f"RF3 Amplitude: {amp} V")

# 写入
epics.caput('iLinac_007:BPM14And15:RF3AVGStart', 100)

# 监控
def on_change(pvname=None, value=None, **kws):
    print(f"{pvname} = {value}")

pv = epics.PV('iLinac_007:BPM14And15:RF3Amp', callback=on_change)
```

### 调试技巧

#### 1. 添加调试输出

在 `driverWrapper.c` 中:
```c
#include <stdio.h>

double ReadData(int offset, int channel, int type)
{
    double value = 0.0;
    // ...

    // 调试输出
    printf("DEBUG: ReadData(offset=%d, ch=%d, type=%d) = %f\n",
           offset, channel, type, value);

    return value;
}
```

#### 2. 查看实时日志

```bash
# 运行时重定向到文件
./st.cmd 2>&1 | tee ioc.log
```

#### 3. 使用 GDB 调试

```bash
gdb ../../bin/linux-arm/BPMmonitor
(gdb) run < st.cmd
(gdb) break ReadData
(gdb) continue
```

#### 4. 检查库加载

```bash
# 查看动态库依赖
ldd ../../bin/linux-arm/BPMmonitor

# 检查 liblowlevel.so 是否存在
ls -l /usr/lib/liblowlevel.so
```

---

## 开发指南

### 添加新功能的步骤

以 "添加新的 RF 参数监测" 为例:

#### Step 1: 驱动层添加支持 (driverWrapper.c/h)

```c
// driverWrapper.h
#define OFFSET_NEW_PARAM    35  // 新的 offset

// driverWrapper.c
double ReadData(int offset, int channel, int type)
{
    // ...
    case OFFSET_NEW_PARAM:
        // 调用底层库获取新参数
        value = (*getNewParamFunc)(channel);
        break;
    // ...
}
```

#### Step 2: 数据库添加记录 (BPMMonitor.db)

```
record(ai, "$(P):RF3NewParam") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@NEW_PARAM:35 ch=0")
    field(SCAN, "I/O Intr")
    field(PREC, "3")
    field(EGU,  "unit")
}
```

#### Step 3: 设备支持层支持类型解析 (devBPMMonitor.c)

```c
// init_ai_record() 中添加:
else if (strcmp(pPvt->type_str, "NEW_PARAM") == 0)
    pPvt->type = TYPE_NEW_PARAM;
```

#### Step 4: 重新编译

```bash
make clean
make
```

#### Step 5: 测试

```bash
cd iocBoot/iocBPMmonitor
./st.cmd
epics> dbgf "iLinac_007:BPM14And15:RF3NewParam"
```

### 修改波形分析算法

假设要修改平均电压计算方式:

```c
// driverWrapper.c

void calculateAvgVoltage(int channel)
{
    // 原算法: 简单平均
    // 新算法: 加权平均

    double weighted_sum = 0.0;
    double weight_total = 0.0;

    for (int i = AVGStart[channel]; i <= AVGStop[channel]; i++) {
        double weight = gaussian_weight(i);  // 高斯权重
        weighted_sum += TrigWaveform[channel][i] * weight;
        weight_total += weight;
    }

    double avg_signal = weighted_sum / weight_total;

    // 背景扣除保持不变
    // ...
}
```

### 添加新的校准参数

#### 1. 创建校准文件

```csv
# /path/to/new_calibration.csv
channel,param1,param2,param3
0,1.23,4.56,7.89
1,2.34,5.67,8.90
...
```

#### 2. 驱动层加载

```c
// driverWrapper.c

double NewCalibParam[8][3];  // 8 通道 × 3 参数

void LoadNewCalibration()
{
    FILE *fp = fopen("/path/to/new_calibration.csv", "r");
    if (!fp) return;

    char line[256];
    fgets(line, sizeof(line), fp);  // 跳过表头

    int ch;
    while (fscanf(fp, "%d,%lf,%lf,%lf",
                  &ch,
                  &NewCalibParam[ch][0],
                  &NewCalibParam[ch][1],
                  &NewCalibParam[ch][2]) == 4) {
        // 成功读取
    }

    fclose(fp);
}

// 在 InitDevice() 中调用
int InitDevice()
{
    // ...
    LoadNewCalibration();
    // ...
}
```

### 性能优化建议

#### 1. 减少锁竞争

如果添加共享数据,使用 epicsMutex:

```c
#include <epicsMutex.h>

epicsMutexId dataLock;

// 初始化
dataLock = epicsMutexCreate();

// 读取时
epicsMutexLock(dataLock);
value = shared_data[channel];
epicsMutexUnlock(dataLock);

// 写入时
epicsMutexLock(dataLock);
shared_data[channel] = new_value;
epicsMutexUnlock(dataLock);
```

#### 2. 波形数据优化

对于大波形,避免频繁拷贝:

```c
// 不好: 每次都拷贝
memcpy(dest, TrigWaveform[ch], 10000 * sizeof(float));

// 好: 使用指针
float *waveform_ptr = TrigWaveform[ch];
```

#### 3. 扫描周期调整

根据数据变化频率选择合适扫描周期:
- 快速变化 (RF 振幅): I/O Intr
- 慢速变化 (温度): 5 second
- 配置参数: Passive (仅在写入时处理)

---

## 常见问题与解决方案

### Q1: IOC 启动失败,提示找不到 liblowlevel.so

**解决**:
```bash
# 检查库文件路径
ls -l /usr/lib/liblowlevel.so

# 如果不存在,修改 driverWrapper.c 中的路径
handle = dlopen("/actual/path/to/liblowlevel.so", RTLD_LAZY);

# 或设置 LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH
```

### Q2: PV 值始终为 0

**排查步骤**:
1. 检查设备初始化是否成功
   ```bash
   epics> var BPMMonitorDebug 1  # 开启调试
   ```

2. 检查 INP 字段配置
   ```bash
   epics> dbpr "iLinac_007:BPM14And15:RF3Amp", 4
   ```

3. 检查扫描是否正常
   ```bash
   epics> scanppl
   ```

### Q3: 波形数据未更新

**检查**:
1. I/O 中断是否触发
   ```c
   printf("scanIoRequest called\n");
   scanIoRequest(ioScanPvt);
   ```

2. 波形记录 SCAN 字段
   ```
   field(SCAN, "I/O Intr")  # 确保正确
   ```

### Q4: 编译错误: undefined reference

**解决**:
```makefile
# 确保链接了必要的库
BPMmonitor_SYS_LIBS += dl pthread

# 检查库顺序 (dl 和 pthread 在最后)
```

---

## 总结

### 核心要点

1. **三层架构**: 驱动 → 设备支持 → 数据库,职责清晰
2. **动态加载**: 使用 dlopen 实现硬件抽象
3. **I/O 中断**: 高效的数据更新机制
4. **参数化设计**: 通过宏实现灵活配置
5. **EPICS 标准**: 遵循 EPICS 规范,易于集成

### 学习路径

1. **C 语言**: 指针、结构体、函数指针、动态库
2. **EPICS 基础**: 记录类型、设备支持、Channel Access
3. **系统编程**: 多线程、动态链接、文件 I/O
4. **加速器物理**: BPM 原理、RF 系统、光束诊断

### 参考资料

- **EPICS 官方文档**: https://epics-controls.org/
- **Application Developer's Guide**: https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide.pdf
- **Device Support Reference**: https://epics.anl.gov/base/R3-15/6-docs/README.html

---

**文档版本**: 1.0
**更新日期**: 2025-11-06
**适用版本**: EPICS Base 3.15.6
