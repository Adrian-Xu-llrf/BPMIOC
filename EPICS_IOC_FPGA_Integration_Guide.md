# EPICS IOC 与 FPGA 交互完整技术指南

**作者**: 技术文档
**版本**: 1.0
**日期**: 2025-11-18
**基于项目**: BPMIOC - BPM 监控系统

---

## 目录

- [第一部分：完整的系统架构分析](#第一部分完整的系统架构分析)
  - [1.1 EPICS IOC 内部结构详解](#11-epics-ioc-内部结构详解)
  - [1.2 IOC 与 FPGA 通信链路](#12-ioc-与-fpga-通信链路)
  - [1.3 数据流动路径分析](#13-数据流动路径分析)

---

## 第一部分：完整的系统架构分析

### 1.1 EPICS IOC 内部结构详解

EPICS IOC（Input/Output Controller）采用分层架构设计，从上到下分为以下几层：

```
┌─────────────────────────────────────────────────────────────┐
│                    Channel Access Network                    │
│                  (CA Protocol / PVAccess)                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                     Record Layer                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ ai/ao    │  │ bi/bo    │  │ waveform │  │  calc    │   │
│  │ record   │  │ record   │  │ record   │  │  record  │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│       │             │               │             │          │
│  ┌────▼─────────────▼───────────────▼─────────────▼─────┐   │
│  │        Record Support Functions (rsup)               │   │
│  │    - init_record, process, get_units, etc.           │   │
│  └──────────────────────┬────────────────────────────────┘   │
└─────────────────────────┼────────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────────┐
│                    Device Support Layer                       │
│  ┌──────────────────────────────────────────────────────┐    │
│  │           Device Support Entry Table (dset)          │    │
│  │  ┌────────────────────────────────────────────────┐  │    │
│  │  │  struct {                                       │  │    │
│  │  │    long      number;                            │  │    │
│  │  │    DEVSUPFUN report;                            │  │    │
│  │  │    DEVSUPFUN init;                              │  │    │
│  │  │    DEVSUPFUN init_record;    // 记录初始化     │  │    │
│  │  │    DEVSUPFUN get_ioint_info; // I/O Intr 支持  │  │    │
│  │  │    DEVSUPFUN read/write;     // 读写操作       │  │    │
│  │  │    DEVSUPFUN special_linconv;                   │  │    │
│  │  │  } devAi, devAo, devBi, devBo, devWaveform;    │  │    │
│  │  └────────────────────────────────────────────────┘  │    │
│  └────────────────────┬─────────────────────────────────┘    │
│                       │                                       │
│  ┌────────────────────▼─────────────────────────────────┐    │
│  │    devIoParse()  - 解析 INP/OUT 字段参数            │    │
│  │    recordpara_t  - 存储解析后的参数 (dpvt)          │    │
│  └────────────────────┬─────────────────────────────────┘    │
└─────────────────────────┼──────────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────────┐
│                     Driver Support Layer                      │
│  ┌──────────────────────────────────────────────────────┐    │
│  │           Driver Support Entry Table (drvet)         │    │
│  │  struct {                                             │    │
│  │    long number;                                       │    │
│  │    DRVSUPFUN report;                                  │    │
│  │    DRVSUPFUN init;  // InitDevice() - 初始化硬件     │    │
│  │  } drWrapper;                                         │    │
│  └────────────────────┬─────────────────────────────────┘    │
│                       │                                       │
│  ┌────────────────────▼─────────────────────────────────┐    │
│  │  驱动层函数（封装硬件访问逻辑）                       │    │
│  │  - ReadData()     - 从 FPGA 读取数据                │    │
│  │  - SetReg()       - 向 FPGA 写入寄存器              │    │
│  │  - readWaveform() - 读取波形数据                     │    │
│  │  - pthread()      - 后台线程（I/O Intr 扫描）       │    │
│  └────────────────────┬─────────────────────────────────┘    │
└─────────────────────────┼──────────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────────┐
│                  Hardware Abstraction Layer                   │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  动态库加载 (dlopen/dlsym)                           │    │
│  │  liblowlevel.so - FPGA 访问库                        │    │
│  │  ┌────────────────────────────────────────────────┐  │    │
│  │  │  函数指针映射：                                 │  │    │
│  │  │  - funcGetRfInfo()     -> GetRfInfo()          │  │    │
│  │  │  - funcSetDO()         -> SetDO()              │  │    │
│  │  │  - funcGetVcValue()    -> GetVcValue()         │  │    │
│  │  │  - funcGetTriggerAllData() -> 数据采集         │  │    │
│  │  └────────────────────────────────────────────────┘  │    │
│  └────────────────────┬─────────────────────────────────┘    │
└─────────────────────────┼──────────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────────┐
│                    Physical Hardware Layer                    │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  FPGA 寄存器访问（内存映射 I/O）                     │    │
│  │  - /dev/mem 或 /dev/uio* 设备文件                   │    │
│  │  - mmap() 映射物理地址到用户空间                    │    │
│  │  - volatile uint32_t* 指针访问寄存器                │    │
│  └────────────────────┬─────────────────────────────────┘    │
└─────────────────────────┼──────────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────────┐
│                         FPGA Logic                            │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  寄存器模块 (Register Bank)                          │    │
│  │  - 控制寄存器 (Control Registers)                    │    │
│  │  - 状态寄存器 (Status Registers)                     │    │
│  │  - 数据缓冲区 (Data Buffers / FIFOs)                │    │
│  │  - 中断生成逻辑 (Interrupt Generation)               │    │
│  └──────────────────────────────────────────────────────┘    │
└───────────────────────────────────────────────────────────────┘
```

#### 1.1.1 Record Layer（记录层）

**作用**: 提供数据的高层抽象，定义业务逻辑和数据处理。

**主要记录类型**（在 BPMIOC 中使用）:

```c
// BPMmonitorApp/Db/BPMMonitor.db 示例
record(ai, "$(P):RFIn_03_Amp")
{
    field(SCAN, ".5 second")         // 扫描周期：每 0.5 秒
    field(DTYP, "BPMmonitor")        // 设备类型：关联到 Device Support
    field(INP,  "@AMP:0 ch=2")       // 输入参数：类型=AMP, offset=0, channel=2
    field(EGU,  "mV")                // 工程单位
    field(PREC, "3")                 // 精度
}

record(ao, "$(P):BPM1_K1_Set")
{
    field(SCAN, "Passive")           // 被动扫描（通过 caput 触发）
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:10 ch=0")      // 输出参数：offset=10, channel=0
    field(VAL,  "1.0")               // 初始值
}

record(waveform, "$(P):RFIn_03_AmpWave")
{
    field(SCAN, "I/O Intr")          // I/O 中断扫描（事件驱动）
    field(DTYP, "BPMmonitorTrigWave")
    field(INP,  "@ARRAY:11 ch=0")
    field(NELM, "10000")             // 数组长度
    field(FTVL, "FLOAT")             // 元素类型
}
```

**关键字段说明**:
- `SCAN`: 扫描机制（周期扫描 / I/O Intr / Passive）
- `DTYP`: 设备类型，对应 DBD 中定义的 device support
- `INP/OUT`: 硬件地址字符串，传递给 Device Support 解析
- `VAL`: 记录值
- `dpvt`: 私有数据指针，存储解析后的参数

#### 1.1.2 DBD (Database Definition)

**作用**: 定义记录类型与设备支持的绑定关系。

**文件位置**: `BPMmonitorApp/src/devBPMMonitor.dbd`

```dbd
# devBPMMonitor.dbd
device(ai,         INST_IO, devAi,         "BPMmonitor")
device(ao,         INST_IO, devAo,         "BPMmonitor")
device(bi,         INST_IO, devBi,         "BPMmonitor")
device(bo,         INST_IO, devBo,         "BPMmonitor")
device(waveform,   INST_IO, devTrigWaveform,   "BPMmonitorTrigWave")
device(waveform,   INST_IO, devHistoryWaveform, "BPMmonitorTripWave")
device(waveform,   INST_IO, devADCRawDataWaveform, "BPMmonitorADCWave")
driver(drWrapper)
```

**解释**:
- `device(记录类型, 链接类型, dset名称, DTYP字符串)`
- 当 record 的 `DTYP="BPMmonitor"` 时，IOC 会调用 `devAi` 的函数表
- `driver(drWrapper)` 注册驱动层，在 IOC 启动时调用 `InitDevice()`

#### 1.1.3 Device Support Layer（设备支持层）

**作用**: 提供硬件抽象接口，解析 INP/OUT 字段，调用驱动层函数。

**文件位置**: `BPMmonitorApp/src/devBPMMonitor.c`

**核心数据结构**:

```c
// 私有数据结构（存储在 record->dpvt）
typedef enum strtype {
    NONE = 0,
    REG,      // 寄存器访问
    STRING,
    AMP,      // 幅度
    PHASE,    // 相位
    POWER,    // 功率
    ARRAY     // 数组
} strtype_t;

typedef struct {
    strtype_t type;          // 数据类型
    unsigned short offset;   // 寄存器偏移
    unsigned short channel;  // 通道号
} recordpara_t;
```

**Device Support Entry Table (dset)**:

```c
// AI 记录的设备支持函数表
struct {
    long      number;          // 函数数量
    DEVSUPFUN report;          // 报告函数（通常为 NULL）
    DEVSUPFUN init;            // 全局初始化（通常为 NULL）
    DEVSUPFUN init_record;     // 记录初始化 -> init_record_ai()
    DEVSUPFUN get_ioint_info;  // I/O Intr 信息 -> NULL 或 devGetInTrigInfo()
    DEVSUPFUN read;            // 读取函数 -> read_ai()
    DEVSUPFUN special_linconv; // 线性转换（通常为 NULL）
} devAi = {
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai,
    NULL
};
epicsExportAddress(dset, devAi);  // 导出符号供 IOC 加载
```

**关键函数实现**:

```c
// 1. 记录初始化函数
static long init_record_ai(aiRecord *record)
{
    recordpara_t *priv;
    // 分配私有数据结构
    priv = (recordpara_t *)callocMustSucceed(1, sizeof(recordpara_t),
                                              "init_record_ai");
    // 解析 INP 字段（例如 "@AMP:0 ch=2"）
    devIoParse(record->inp.value.instio.string, priv);

    // 将私有数据挂载到 record
    record->dpvt = priv;
    return 0;
}

// 2. INP/OUT 字段解析函数
static int devIoParse(char *string, recordpara_t *recordpara)
{
    // 输入格式: "TYPE:offset ch=channel"
    // 例如: "@AMP:0 ch=2" 或 "@REG:10 ch=0"

    char typeName[10] = "";
    char *pchar = string;

    // 解析类型名称
    int nchar = strcspn(pchar, ":");
    strncpy(typeName, pchar, nchar);
    typeName[nchar] = '\0';
    pchar += nchar + 1;  // 跳过 ':'

    if (strcmp(typeName, "REG") == 0)
        recordpara->type = REG;
    else if (strcmp(typeName, "AMP") == 0)
        recordpara->type = AMP;
    else if (strcmp(typeName, "PHASE") == 0)
        recordpara->type = PHASE;
    else if (strcmp(typeName, "POWER") == 0)
        recordpara->type = POWER;
    else if (strcmp(typeName, "ARRAY") == 0)
        recordpara->type = ARRAY;

    // 解析 offset
    recordpara->offset = strtol(pchar, &pchar, 0);

    // 解析 channel（格式: "ch=2"）
    pchar = strstr(pchar, "ch=");
    if (pchar) {
        recordpara->channel = strtol(pchar + 3, NULL, 0);
    }

    return 0;
}

// 3. 读取函数
static long read_ai(aiRecord *record)
{
    recordpara_t *priv = (recordpara_t *)record->dpvt;
    float value;

    switch (priv->type) {
        case POWER:
            // 读取幅度并转换为功率
            float amp = ReadData(priv->offset, priv->channel, priv->type);
            value = amp2power(amp, priv->channel + 1);
            break;
        case REG:
        case AMP:
        case PHASE:
            // 直接读取寄存器/幅度/相位
            value = ReadData(priv->offset, priv->channel, priv->type);
            break;
        default:
            value = 0;
            break;
    }

    record->val = value;
    record->udf = FALSE;  // 标记数据有效
    return 2;  // 不进行单位转换
}

// 4. 写入函数（AO 记录）
static long write_ao(aoRecord *record)
{
    recordpara_t *priv = (recordpara_t *)record->dpvt;
    // 调用驱动层写入函数
    SetReg(priv->offset, priv->channel, record->val);
    return 0;
}

// 5. I/O Intr 支持函数
static long devGetInTrigInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt)
{
    // 返回扫描句柄，用于事件驱动
    *ppvt = devGetInTrigScanPvt();
    return 0;
}
```

**I/O Intr 扫描机制**:

对于 `SCAN="I/O Intr"` 的记录，当硬件事件发生时（如触发器到达、数据准备好），驱动层通过 `scanIoRequest(scanPvt)` 触发所有关联记录的处理。

```c
// 驱动层后台线程
void *pthread()
{
    while(1)
    {
        // 等待 FPGA 数据准备好
        funcTriggerAllDataReached();

        // 触发 I/O Intr 扫描
        scanIoRequest(TriginScanPvt);

        // 获取时间戳
        funcGetTimestampData(1, &TAISecond, &TAINanoSecond);
        funcSetWRCaputureDataTrigger();

        usleep(100000);  // 100ms
    }
}
```

#### 1.1.4 Driver Support Layer（驱动支持层）

**作用**: 封装硬件访问逻辑，加载底层硬件库，提供统一的硬件接口。

**文件位置**: `BPMmonitorApp/src/driverWrapper.c`

**Driver Support Entry Table (drvet)**:

```c
static long InitDevice();

struct {
    long number;
    DRVSUPFUN report;
    DRVSUPFUN init;  // IOC 启动时调用
} drWrapper = {
    2,
    NULL,
    InitDevice
};
epicsExportAddress(drvet, drWrapper);
```

**关键函数实现**:

```c
// 驱动初始化函数
static long InitDevice()
{
    printf("## 7100-10ADC RK BPM IOC_20250830\n");

    void *handle;
    int (*funcOpen)();

    // 1. 动态加载底层硬件库
    handle = dlopen("liblowlevel.so", RTLD_NOW);
    if (handle == NULL) {
        fprintf(stderr, "Failed to open library: %s\n", dlerror());
        return -1;
    }

    // 2. 初始化硬件
    funcOpen = dlsym(handle, "SystemInit");
    int result = funcOpen();
    if(result == 0) {
        printf("Open System success!\n");
    }

    // 3. 映射硬件函数指针
    funcGetRfInfo = dlsym(handle, "GetRfInfo");
    funcGetDI = dlsym(handle, "GetDI");
    funcSetDO = dlsym(handle, "SetDO");
    funcGetVcValue = dlsym(handle, "GetVcValue");
    funcSetBPMk1 = dlsym(handle, "SetBPMk1");
    funcGetTriggerAllData = dlsym(handle, "GetTriggerAllData");
    // ... 映射更多函数 ...

    // 4. 创建后台线程（用于 I/O Intr 扫描）
    pthread_t tidp1;
    if(pthread_create(&tidp1, NULL, pthread, NULL) == -1) {
        printf("create thread error!\n");
        return -1;
    }

    // 5. 初始化 I/O Intr 扫描句柄
    scanIoInit(&TriginScanPvt);
    scanIoInit(&TripBufferinScanPvt);
    scanIoInit(&ADCrawBufferinScanPvt);

    return 0;
}

// ReadData() - 驱动层读取函数
float ReadData(int offset, int channel, int type)
{
    switch(offset)
    {
        case 0:  // RF 信息
            if(type == AMP)
                return GetRFInfo(channel, 0);  // 幅度
            else if(type == PHASE)
                return GetRFInfo(channel, 1);  // 相位
            break;

        case 1:  // 数字输入
            return GetDI(channel);

        case 5:  // BPM 电压值
            return funcGetVcValue(channel);

        case 7:  // XY 位置
            return funcGetxyPosition(channel);

        case 8:  // 电压和
            return funcGetVcSumValue(channel);

        case 10:  // 差分电压 (A-C)
            return (funcGetVcValue(0) - funcGetVcValue(2));

        case 29:  // XY 位置（单位转换）
            if(pulseMode == 0)
                return ((float)funcGetxyPosition(channel) / 1E+6);
            else {
                // 脉冲模式：返回平均值
                if(channel == 0) return X1_avg / 1000;
                if(channel == 1) return Y1_avg / 1000;
                if(channel == 2) return X2_avg / 1000;
                if(channel == 3) return Y2_avg / 1000;
            }
            break;

        default:
            printf("Unknown offset: %d\n", offset);
            return 0;
    }
}

// SetReg() - 驱动层写入函数
void SetReg(int offset, int channel, float val)
{
    int val_tmp = (int)val;

    switch(offset)
    {
        case 0:  // 数字输出
            funcSetDO(channel, val_tmp);
            break;

        case 2:  // 内部触发使能
            funcSetInnerTrigEn(val_tmp);
            break;

        case 10:  // BPM K1 系数
            funcSetBPMk1(channel, val);
            break;

        case 11:  // BPM K2 系数
            funcSetBPMk2(channel, val);
            break;

        case 13:  // BPM 相位偏移
            funcSetBPMPhaseOffset(channel, val);
            break;

        case 14:  // BPM Kxy 系数
            funcSetBPMkxy(channel, (int)(val * 1E+6));
            break;

        default:
            printf("Unknown offset: %d\n", offset);
            break;
    }
}

// readWaveform() - 读取波形数据
void readWaveform(int offset, int ch_N, unsigned int nelem,
                  float* data, long long *TAI_S, int *TAI_nS)
{
    // 转换时间戳（TAI -> EPICS epoch）
    *TAI_S = (TAISecond - 631152000 - 8*60*60);
    *TAI_nS = (TAINanoSecond * 16);

    switch(offset)
    {
        case 1:  // 触发波形 - 原始数据
            funcGetTriggerAllData(0, 0, data);
            break;

        case 11:  // RF3 幅度波形（带单位转换）
            copyArray(rf3amp, data, 0, nelem);
            calculateAvgVoltage(data, 0, nelem);  // 计算平均电压
            break;

        case 21:  // RF3 相位波形
            copyPhArray(rf3phase, data, 1, nelem);
            break;

        case 61:  // X1 位置波形
            copyXYArray(X1, data, 16, nelem);
            break;

        default:
            printf("Unknown waveform offset: %d\n", offset);
            break;
    }
}

// 单位转换辅助函数
static void copyArray(float *dmaBuf, float *wfBuf, int ch_N, int length)
{
    // 从 FPGA 获取原始数据
    funcGetTriggerAllData(1, ch_N, dmaBuf);

    // 单位转换：原始值 -> 电压 (RMS)
    for(int i = 0; i < length; ++i) {
        wfBuf[i] = ((float)dmaBuf[i] / 1.28E+6) * sqrt(2);
    }
}
```

### 1.2 IOC 与 FPGA 通信链路

#### 1.2.1 物理层通信

**BPMIOC 使用的通信方式**: **Zynq SoC 内存映射 I/O**

```
┌─────────────────────────────────────────────────────────────┐
│                    Xilinx Zynq SoC                           │
│                                                               │
│  ┌────────────────┐         ┌────────────────────────────┐  │
│  │   ARM Cortex   │  AXI    │      FPGA (PL)             │  │
│  │   (PS 侧)      │◄───────►│  ┌──────────────────────┐  │  │
│  │                │  Bus    │  │  BPM 处理逻辑        │  │  │
│  │  - Linux OS    │         │  │  - ADC 接口          │  │  │
│  │  - EPICS IOC   │         │  │  - IQ 解调           │  │  │
│  │  - liblowlevel │         │  │  - 波形缓冲          │  │  │
│  │                │         │  │  - 触发逻辑          │  │  │
│  └────────────────┘         │  └──────────────────────┘  │  │
│         │                   │           │                 │  │
│         │ mmap()            │           │                 │  │
│         ▼                   │           ▼                 │  │
│  ┌────────────────┐         │  ┌──────────────────────┐  │  │
│  │   /dev/mem     │         │  │   寄存器接口          │  │  │
│  │   /dev/uio0    │◄────────┼──┤   (AXI-Lite Slave)   │  │  │
│  └────────────────┘         │  │  - 控制寄存器        │  │  │
│                              │  │  - 状态寄存器        │  │  │
│                              │  │  - 数据 FIFO         │  │  │
│                              │  └──────────────────────┘  │  │
│                              └────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

**物理层特点**:
- **ARM 与 FPGA 集成在同一芯片**: 无需外部总线，低延迟
- **AXI 总线**: ARM 通过 AXI 互连访问 FPGA 寄存器
- **内存映射**: FPGA 寄存器映射到 ARM 物理地址空间
- **设备文件**: `/dev/mem` 或 `/dev/uio*` 提供用户态访问

#### 1.2.2 软件层通信

```
┌─────────────────────────────────────────────────────────────┐
│                      EPICS IOC 进程                          │
│                                                               │
│  ┌────────────────────────────────────────────────────────┐  │
│  │            driverWrapper.c (驱动层)                    │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  InitDevice() {                                   │  │  │
│  │  │    handle = dlopen("liblowlevel.so", RTLD_NOW);  │  │  │
│  │  │    funcGetRfInfo = dlsym(handle, "GetRfInfo");   │  │  │
│  │  │    funcSetDO = dlsym(handle, "SetDO");           │  │  │
│  │  │    // ... 更多函数映射 ...                        │  │  │
│  │  │  }                                                 │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  │                        │                                │  │
│  │                        │ dlopen/dlsym                   │  │
│  │                        ▼                                │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  函数指针表:                                      │  │  │
│  │  │  static void (*funcGetRfInfo)(...);              │  │  │
│  │  │  static void (*funcSetDO)(...);                  │  │  │
│  │  │  static int  (*funcGetVcValue)(...);             │  │  │
│  │  │  static void (*funcGetTriggerAllData)(...);      │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
│                        │                                     │
│                        │ 函数调用                            │
│                        ▼                                     │
│  ┌────────────────────────────────────────────────────────┐  │
│  │           liblowlevel.so (硬件抽象库)                  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  int SystemInit() {                               │  │  │
│  │  │    fd = open("/dev/uio0", O_RDWR);               │  │  │
│  │  │    fpga_base = mmap(NULL, MAP_SIZE,              │  │  │
│  │  │                     PROT_READ|PROT_WRITE,         │  │  │
│  │  │                     MAP_SHARED, fd, 0);           │  │  │
│  │  │    return 0;                                      │  │  │
│  │  │  }                                                 │  │  │
│  │  │                                                    │  │  │
│  │  │  void GetRfInfo(int ch, float *amp, float *ph) { │  │  │
│  │  │    volatile uint32_t *reg = fpga_base + offset;  │  │  │
│  │  │    uint32_t raw_amp = reg[ch * 2];               │  │  │
│  │  │    uint32_t raw_ph  = reg[ch * 2 + 1];           │  │  │
│  │  │    *amp = raw_amp / 1.28e6 * sqrt(2);            │  │  │
│  │  │    *ph  = raw_ph * 360.0 / 65536;                │  │  │
│  │  │  }                                                 │  │  │
│  │  │                                                    │  │  │
│  │  │  void SetDO(int ch, int val) {                   │  │  │
│  │  │    volatile uint32_t *reg = fpga_base + DO_ADDR; │  │  │
│  │  │    reg[ch] = val;                                 │  │  │
│  │  │  }                                                 │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
                          │
                          │ mmap() / read() / write()
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   Linux Kernel Space                         │
│  ┌────────────────────────────────────────────────────────┐  │
│  │            UIO Driver (uio_pdrv_genirq)                │  │
│  │  - /dev/uio0 -> 物理地址 0xXXXXXXXX                   │  │
│  │  - 提供 mmap() 接口                                    │  │
│  │  - 提供中断通知机制                                     │  │
│  └────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
                          │
                          │ 物理地址映射
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    FPGA 寄存器空间                           │
│  基地址: 0x43C00000 (示例)                                  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  偏移     │  寄存器名称        │  读/写  │  说明      │  │
│  ├──────────────────────────────────────────────────────┤  │
│  │  0x0000   │  CTRL_REG          │  R/W    │  控制寄存器│  │
│  │  0x0004   │  STATUS_REG        │  R      │  状态寄存器│  │
│  │  0x0010   │  CH0_AMP           │  R      │  通道0幅度 │  │
│  │  0x0014   │  CH0_PHASE         │  R      │  通道0相位 │  │
│  │  0x0100   │  DO_REG            │  R/W    │  数字输出  │  │
│  │  0x0200   │  BPM_K1_CH0        │  R/W    │  K1 系数   │  │
│  │  0x1000   │  WAVEFORM_FIFO     │  R      │  波形FIFO  │  │
│  │  ...      │  ...               │  ...    │  ...       │  │
│  └──────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

### 1.3 数据流动路径分析

#### 1.3.1 读取路径 (FPGA → IOC → Channel Access)

**场景**: 用户通过 `caget` 读取 BPM 幅度值

```
Step 1: 用户命令
$ caget BPM:RFIn_03_Amp

Step 2: Channel Access 网络层
CA Server 收到请求 → 查找 PV → 触发 record 处理

Step 3: Record 处理 (ai record)
┌─────────────────────────────────────────┐
│  record(ai, "BPM:RFIn_03_Amp") {        │
│    field(SCAN, ".5 second")             │
│    field(DTYP, "BPMmonitor")            │
│    field(INP,  "@AMP:0 ch=2")           │
│  }                                       │
└─────────────────────────────────────────┘
         │
         │ dbProcess(precord)
         ▼
┌─────────────────────────────────────────┐
│  Record Support (rsup)                  │
│  - 检查 PACT 标志                        │
│  - 调用 device support read 函数        │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Device Support: devAi.read()           │
│  函数: read_ai()                         │
│                                          │
│  recordpara_t *priv = record->dpvt;     │
│  // priv->type = AMP                    │
│  // priv->offset = 0                    │
│  // priv->channel = 2                   │
│                                          │
│  value = ReadData(0, 2, AMP);           │
│  record->val = value;                   │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Driver Support: ReadData()             │
│  函数位置: driverWrapper.c:426          │
│                                          │
│  switch(offset) {                       │
│    case 0:  // RF 信息                  │
│      if(type == AMP)                    │
│        return GetRFInfo(channel, 0);    │
│  }                                       │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Driver 内部函数: GetRFInfo()           │
│                                          │
│  float amp, phase;                      │
│  funcGetRfInfo(channel, &amp, &phase);  │
│  return amp;                            │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  硬件抽象层: funcGetRfInfo()            │
│  (liblowlevel.so)                       │
│                                          │
│  volatile uint32_t *reg =               │
│      fpga_base + RF_AMP_BASE;           │
│  uint32_t raw_amp = reg[channel * 2];  │
│  *amp = raw_amp / 1.28e6 * sqrt(2);    │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  FPGA 寄存器                             │
│  物理地址: 0x43C00010 + (channel * 8)  │
│  读取 32-bit 值                         │
└─────────────────────────────────────────┘
```

**数据流动 ASCII 图**:

```
User                CA Network       Record Layer      Device Support      Driver Layer       HW Layer         FPGA
 │                      │                  │                  │                 │                │              │
 ├─ caget BPM:...─────►│                  │                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      ├─ dbProcess() ───►│                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      │                  ├─ read_ai() ─────►│                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  ├─ ReadData() ───►│                │              │
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  │                 ├─ GetRFInfo() ─►│              │
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  │                 │                ├─ reg read ──►│
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  │                 │                │◄─ 32bit ─────┤
 │                      │                  │                  │                 │                │   data       │
 │                      │                  │                  │                 │◄─ float ───────┤              │
 │                      │                  │                  │◄─ float ────────┤   value        │              │
 │                      │                  │◄─ record->val ───┤                 │                │              │
 │                      │◄─ CA response ───┤                  │                 │                │              │
 │◄─ value ─────────────┤                  │                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
```

#### 1.3.2 写入路径 (Channel Access → IOC → FPGA)

**场景**: 用户通过 `caput` 设置 BPM K1 系数

```
Step 1: 用户命令
$ caput BPM:BPM1_K1_Set 1.234

Step 2: Channel Access 网络层
CA Server 收到请求 → 查找 PV → 设置 record->val → 触发处理

Step 3: Record 处理 (ao record)
┌─────────────────────────────────────────┐
│  record(ao, "BPM:BPM1_K1_Set") {        │
│    field(SCAN, "Passive")               │
│    field(DTYP, "BPMmonitor")            │
│    field(OUT,  "@REG:10 ch=0")          │
│  }                                       │
│  record->val = 1.234                    │
└─────────────────────────────────────────┘
         │
         │ dbProcess(precord)
         ▼
┌─────────────────────────────────────────┐
│  Record Support (rsup)                  │
│  - 调用 device support write 函数       │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Device Support: devAo.write()          │
│  函数: write_ao()                        │
│                                          │
│  recordpara_t *priv = record->dpvt;     │
│  // priv->offset = 10                   │
│  // priv->channel = 0                   │
│                                          │
│  SetReg(10, 0, record->val);            │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Driver Support: SetReg()               │
│  函数位置: driverWrapper.c:597          │
│                                          │
│  switch(offset) {                       │
│    case 10:  // BPM K1 系数             │
│      SetBPMk1(channel, val);            │
│      break;                             │
│  }                                       │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Driver 内部函数: SetBPMk1()            │
│                                          │
│  funcSetBPMk1(channel, value);          │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  硬件抽象层: funcSetBPMk1()             │
│  (liblowlevel.so)                       │
│                                          │
│  volatile uint32_t *reg =               │
│      fpga_base + BPM_K1_BASE;           │
│  int32_t fixed_point = value * 32767;  │
│  reg[channel] = fixed_point;            │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  FPGA 寄存器                             │
│  物理地址: 0x43C00200 + (channel * 4)  │
│  写入 32-bit 定点数                     │
└─────────────────────────────────────────┘
```

**数据流动 ASCII 图**:

```
User                CA Network       Record Layer      Device Support      Driver Layer       HW Layer         FPGA
 │                      │                  │                  │                 │                │              │
 ├─ caput val=1.234 ──►│                  │                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      ├─ set val ───────►│                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      ├─ dbProcess() ───►│                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      │                  ├─ write_ao() ────►│                 │                │              │
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  ├─ SetReg() ─────►│                │              │
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  │                 ├─ SetBPMk1() ──►│              │
 │                      │                  │                  │                 │                │              │
 │                      │                  │                  │                 │                ├─ reg write ─►│
 │                      │                  │                  │                 │                │   (0x4D2)    │
 │                      │                  │                  │                 │                │              │
 │                      │◄─ CA ack ────────┼──────────────────┼─────────────────┤                │              │
 │◄─ OK ────────────────┤                  │                  │                 │                │              │
 │                      │                  │                  │                 │                │              │
```

#### 1.3.3 I/O Intr 扫描机制（事件驱动）

**场景**: FPGA 触发器到达，自动采集波形数据

```
FPGA 事件                Driver Thread        I/O Intr Scan        Record Processing
    │                         │                     │                       │
    ├─ Trigger arrives ───────►│                     │                       │
    │   (硬件事件)              │                     │                       │
    │                         │                     │                       │
    │                         ├─ funcTriggerAll    │                       │
    │                         │   DataReached()     │                       │
    │                         │   (轮询检测)         │                       │
    │                         │                     │                       │
    │                         ├─ scanIoRequest() ──►│                       │
    │                         │   (TriginScanPvt)   │                       │
    │                         │                     │                       │
    │                         │                     ├─ 扫描所有 I/O Intr  ─►│
    │                         │                     │   关联的 record        │
    │                         │                     │                       │
    │                         │                     │                       ├─ read_wf()
    │                         │                     │                       │   (waveform record)
    │                         │                     │                       │
    │                         │                     │                       ├─ readWaveform()
    │                         │◄────────────────────┼───────────────────────┤   (driver call)
    │                         │                     │                       │
    │                         ├─ funcGetTrigger    │                       │
    │                         │   AllData()         │                       │
    │                         │   (读取波形)         │                       │
    │◄─ 读取 FIFO ─────────────┤                     │                       │
    │                         │                     │                       │
    │                         ├─ 返回数据 ──────────┼───────────────────────►│
    │                         │                     │                       │
    │                         │                     │                       ├─ record->bptr
    │                         │                     │                       │   (波形数据)
    │                         │                     │                       │
    │                         │                     │   CA monitors 通知 ──►│ Clients
    │                         │                     │                       │
    │                         ├─ usleep(100ms) ─────┤                       │
    │                         │                     │                       │
    │                         └─ 循环 ──────────────┘                       │
```

**后台线程代码**:

```c
// driverWrapper.c:393
void *pthread()
{
    while(1)
    {
        // 1. 等待 FPGA 触发数据准备好
        funcTriggerAllDataReached();

        // 2. 触发 I/O Intr 扫描（所有 SCAN="I/O Intr" 的 record 都会被处理）
        scanIoRequest(TriginScanPvt);

        // 3. 获取时间戳（White Rabbit）
        funcGetTimestampData(1, &TAISecond, &TAINanoSecond);
        funcSetWRCaputureDataTrigger();

        // 4. 等待 100ms
        usleep(100000);
    }
}
```

**I/O Intr 记录定义**:

```
record(waveform, "$(P):RFIn_03_AmpWave")
{
    field(SCAN, "I/O Intr")              # 关键：事件驱动扫描
    field(DTYP, "BPMmonitorTrigWave")    # 关联到 devTrigWaveform
    field(INP,  "@ARRAY:11 ch=0")
    field(NELM, "10000")
    field(FTVL, "FLOAT")
}
```

**Device Support 中的 I/O Intr 绑定**:

```c
// devBPMMonitor.c:153
struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;  // ← 关键：绑定 I/O Intr
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devTrigWaveform = {
    6,
    NULL,
    NULL,
    init_record_wf,
    devGetInTrigInfo,  // ← 返回 TriginScanPvt
    read_wf,
    NULL
};

static long devGetInTrigInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt)
{
    *ppvt = devGetInTrigScanPvt();  // 返回驱动层的扫描句柄
    return 0;
}
```

#### 1.3.4 时间戳同步（White Rabbit）

BPMIOC 使用 White Rabbit 时序系统进行时间戳同步：

```
White Rabbit          FPGA                Driver              Record
Network               Timing              Thread              Timestamp
   │                    │                    │                    │
   ├─ PPS + UTC ───────►│                    │                    │
   │   (每秒同步)         │                    │                    │
   │                    │                    │                    │
   │                    ├─ 时间戳寄存器       │                    │
   │                    │   - TAI_SEC        │                    │
   │                    │   - TAI_NSEC       │                    │
   │                    │                    │                    │
   │                    │◄─ 读取请求 ─────────┤                    │
   │                    │   funcGetTimestamp │                    │
   │                    │   Data()           │                    │
   │                    │                    │                    │
   │                    ├─ 返回时间戳 ────────►│                    │
   │                    │   TAISecond        │                    │
   │                    │   TAINanoSecond    │                    │
   │                    │                    │                    │
   │                    │                    ├─ 转换为 EPICS ─────►│
   │                    │                    │   epoch            │
   │                    │                    │   (减去偏移)        │
   │                    │                    │                    │
   │                    │                    │                    ├─ record->time
   │                    │                    │                    │   .secPastEpoch
   │                    │                    │                    │   .nsec
```

**时间戳转换代码**:

```c
// driverWrapper.c:722
void readWaveform(int offset, int ch_N, unsigned int nelem,
                  float* data, long long *TAI_S, int *TAI_nS)
{
    // TAI 转 EPICS epoch (1990-01-01 00:00:00)
    // 631152000 = 1990-01-01 相对于 1970-01-01 的秒数
    // 8*60*60 = UTC+8 时区偏移
    *TAI_S = (TAISecond - 631152000 - 8*60*60);
    *TAI_nS = (TAINanoSecond * 16);  // FPGA 时间戳单位转换

    // ... 读取波形数据 ...
}

// devBPMMonitor.c:417
static long read_wf(waveformRecord *record)
{
    long long TaiSec = 0;
    int TaiNSec = 0;
    recordpara_t *priv = (recordpara_t *)record->dpvt;

    readWaveform(priv->offset, priv->channel, record->nelm,
                 record->bptr, &TaiSec, &TaiNSec);

    // 设置记录时间戳
    record->time.secPastEpoch = (epicsUInt32)TaiSec;
    record->time.nsec = (epicsUInt32)TaiNSec;

    record->nord = record->nelm;
    return 0;
}
```

---

**第一部分总结**:

1. **EPICS IOC 采用分层架构**: Record → Device Support → Driver Support → Hardware Abstraction → FPGA
2. **数据流路径清晰**: PV 读写通过层层调用最终到达 FPGA 寄存器
3. **I/O Intr 机制**: 实现事件驱动的数据采集，避免轮询
4. **时间戳同步**: 通过 White Rabbit 实现高精度时间同步

接下来，我将继续编写第二部分：常见 EPICS-FPGA 交互接口详解...

---

## 第二部分：常见 EPICS-FPGA 交互接口详解

作为 IOC 开发者，理解不同的硬件接口类型对于选择合适的技术方案至关重要。本节详细介绍各种常见的 EPICS-FPGA 交互接口。

### 2.1 PCIe (PCI Express) 接口

#### 2.1.1 工作原理

PCIe 是现代计算机中最常用的高速串行总线标准，广泛用于连接 FPGA 加速卡。

**架构图**:

```
┌─────────────────────────────────────────────────────────────┐
│                     Host Computer (x86/ARM)                  │
│  ┌────────────────────────────────────────────────────────┐  │
│  │             EPICS IOC Process                          │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Device Support / Driver Support                  │  │  │
│  │  └────────────────────┬─────────────────────────────┘  │  │
│  └───────────────────────┼────────────────────────────────┘  │
│                          │                                   │
│  ┌───────────────────────▼────────────────────────────────┐  │
│  │          Kernel Driver (PCIE_DRIVER.ko)                │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  - BAR (Base Address Register) 映射              │  │  │
│  │  │  - DMA Engine 控制                               │  │  │
│  │  │  - 中断处理 (MSI/MSI-X)                          │  │  │
│  │  │  - mmap() 接口                                   │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └───────────────────────┬────────────────────────────────┘  │
└──────────────────────────┼─────────────────────────────────────┘
                           │
                           │ PCIe Transaction Layer
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                   FPGA PCIe Endpoint                         │
│  ┌────────────────────────────────────────────────────────┐  │
│  │           PCIe Hard IP / Soft IP Core                  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  - Configuration Space                            │  │  │
│  │  │  - BAR0: 寄存器空间 (1MB)                         │  │  │
│  │  │  - BAR1: DMA 缓冲区 (256MB)                       │  │  │
│  │  │  - MSI Interrupt Vector                           │  │  │
│  │  └──────────────────┬───────────────────────────────┘  │  │
│  └───────────────────────┼────────────────────────────────┘  │
│                          │ AXI/Avalon Bus                    │
│  ┌───────────────────────▼────────────────────────────────┐  │
│  │            User Logic (Application)                    │  │
│  │  - 控制/状态寄存器                                      │  │
│  │  - DMA Descriptor Ring                                 │  │
│  │  - Data FIFOs                                          │  │
│  └────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

**数据传输方式**:

1. **PIO (Programmed I/O)** - 寄存器访问
   ```c
   // 用户空间代码
   volatile uint32_t *bar0 = mmap(NULL, bar0_size, PROT_READ|PROT_WRITE,
                                   MAP_SHARED, fd, 0);
   
   // 写入控制寄存器
   bar0[CTRL_REG_OFFSET] = 0x00000001;  // 启动采集
   
   // 读取状态寄存器
   uint32_t status = bar0[STATUS_REG_OFFSET];
   ```

2. **DMA (Direct Memory Access)** - 批量数据传输
   ```c
   // 配置 DMA 传输
   struct dma_desc {
       uint64_t src_addr;   // FPGA 内存地址
       uint64_t dst_addr;   // 主机内存地址
       uint32_t length;     // 传输字节数
       uint32_t flags;      // 控制标志
   };
   
   // 提交 DMA 描述符
   ioctl(fd, DMA_SUBMIT, &desc);
   
   // 等待 DMA 完成（通过中断）
   read(fd, &event, sizeof(event));  // 阻塞等待中断
   ```

3. **中断机制 (MSI/MSI-X)**
   ```c
   // 内核驱动注册中断处理
   pci_enable_msi(pdev);
   request_irq(pdev->irq, fpga_interrupt_handler, 0, "fpga", dev);
   
   // 中断处理函数
   static irqreturn_t fpga_interrupt_handler(int irq, void *dev_id)
   {
       // 读取中断状态
       uint32_t int_status = readl(bar0 + INT_STATUS_REG);
       
       if (int_status & DMA_DONE_INT) {
           // DMA 完成
           wake_up_interruptible(&dma_wait_queue);
       }
       
       if (int_status & DATA_READY_INT) {
           // 数据准备好
           scanIoRequest(dataScanPvt);  // EPICS I/O Intr
       }
       
       return IRQ_HANDLED;
   }
   ```

#### 2.1.2 优缺点

**优点**:
- ✅ **极高带宽**: PCIe Gen3 x8 可达 ~8 GB/s
- ✅ **低延迟**: 寄存器访问 < 1 μs
- ✅ **成熟生态**: 大量 IP 核和驱动框架
- ✅ **DMA 支持**: 大数据传输无需 CPU 干预
- ✅ **中断机制**: 高效的事件通知

**缺点**:
- ❌ **硬件复杂**: 需要 PCIe Endpoint IP（Xilinx/Intel 提供）
- ❌ **驱动开发**: 需要编写内核驱动（或使用 XDMA 等框架）
- ❌ **物理限制**: 需要 PCIe 插槽，不适合嵌入式系统
- ❌ **调试困难**: PCIe 协议复杂，问题定位需要专业工具

#### 2.1.3 适用场景

- 高性能数据采集系统（如高速 ADC/DAC）
- 实时数据处理（如粒子加速器 BPM 系统）
- 需要大量数据传输的应用（> 100 MB/s）
- 桌面/服务器环境

#### 2.1.4 EPICS 集成示例

```c
// Device Support 示例
static long init_record_ai(aiRecord *record)
{
    pcie_priv_t *priv = malloc(sizeof(pcie_priv_t));
    
    // 打开 PCIe 设备
    priv->fd = open("/dev/fpga_pcie0", O_RDWR);
    
    // 映射 BAR0（寄存器空间）
    priv->bar0 = mmap(NULL, BAR0_SIZE, PROT_READ|PROT_WRITE,
                      MAP_SHARED, priv->fd, 0);
    
    record->dpvt = priv;
    return 0;
}

static long read_ai(aiRecord *record)
{
    pcie_priv_t *priv = record->dpvt;
    recordpara_t *para = parse_inp(record->inp);
    
    // 直接读取 FPGA 寄存器
    uint32_t raw_value = priv->bar0[para->offset];
    
    // 单位转换
    record->val = raw_value * para->scale + para->offset;
    return 0;
}
```

---

### 2.2 Zynq/AXI-Lite 内存映射接口

#### 2.2.1 工作原理

**这是 BPMIOC 项目使用的方案！**

Xilinx Zynq SoC 集成了 ARM 处理器和 FPGA，通过 AXI 总线进行通信。

**架构图**:

```
┌─────────────────────────────────────────────────────────────┐
│                    Xilinx Zynq SoC                           │
│                                                               │
│  ┌────────────────────────┬────────────────────────────┐    │
│  │   Processing System    │    Programmable Logic      │    │
│  │       (PS 侧)          │         (PL 侧)            │    │
│  │                        │                            │    │
│  │  ┌──────────────────┐  │  ┌──────────────────────┐  │    │
│  │  │  ARM Cortex-A9   │  │  │   BPM Processing     │  │    │
│  │  │  Dual Core       │  │  │   ┌────────────────┐ │  │    │
│  │  │  - Linux OS      │  │  │   │ IQ Demodulator │ │  │    │
│  │  │  - EPICS IOC     │  │  │   └────────────────┘ │  │    │
│  │  └──────────────────┘  │  │   ┌────────────────┐ │  │    │
│  │                        │  │   │ Waveform FIFO  │ │  │    │
│  │  ┌──────────────────┐  │  │   └────────────────┘ │  │    │
│  │  │   DDR3 Memory    │  │  │   ┌────────────────┐ │  │    │
│  │  │   (Shared)       │  │  │   │ Trigger Logic  │ │  │    │
│  │  └──────────────────┘  │  │   └────────────────┘ │  │    │
│  │                        │  └──────────────────────┘  │    │
│  │          │             │             │               │    │
│  │          │             │             │               │    │
│  │  ┌───────▼─────────────▼─────────────▼────────────┐ │    │
│  │  │          AXI Interconnect                       │ │    │
│  │  │  ┌───────────────┐  ┌───────────────────────┐  │ │    │
│  │  │  │ AXI GP0 Port  │  │ AXI HP0 Port (DMA)    │  │ │    │
│  │  │  │ (寄存器访问)   │  │ (高性能数据传输)       │  │ │    │
│  │  │  └───────────────┘  └───────────────────────┘  │ │    │
│  │  └──────────────────────────────────────────────────┘ │    │
│  │                                                        │    │
│  │  物理地址映射:                                         │    │
│  │  0x43C00000 - 0x43C00FFF: 控制寄存器                  │    │
│  │  0x43C01000 - 0x43C01FFF: 状态寄存器                  │    │
│  │  0x43C10000 - 0x43CFFFFF: 数据缓冲区                  │    │
│  └────────────────────────────────────────────────────────┘    │
└───────────────────────────────────────────────────────────────┘

         │
         │ Linux /dev/uio0
         ▼

┌─────────────────────────────────────────────────────────────┐
│              User Space (EPICS IOC)                          │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  liblowlevel.so                                        │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  int SystemInit() {                               │  │  │
│  │  │    fd = open("/dev/uio0", O_RDWR);               │  │  │
│  │  │    fpga_base = mmap(NULL, 0x100000,              │  │  │
│  │  │                     PROT_READ|PROT_WRITE,         │  │  │
│  │  │                     MAP_SHARED, fd, 0);           │  │  │
│  │  │    // fpga_base -> 0x43C00000                    │  │  │
│  │  │  }                                                 │  │  │
│  │  │                                                    │  │  │
│  │  │  void SetDO(int ch, int val) {                   │  │  │
│  │  │    volatile uint32_t *reg =                      │  │  │
│  │  │        (uint32_t*)(fpga_base + 0x100);           │  │  │
│  │  │    reg[ch] = val;  // 写入 FPGA 寄存器           │  │  │
│  │  │  }                                                 │  │  │
│  │  │                                                    │  │  │
│  │  │  int GetVcValue(int ch) {                        │  │  │
│  │  │    volatile uint32_t *reg =                      │  │  │
│  │  │        (uint32_t*)(fpga_base + 0x200);           │  │  │
│  │  │    return reg[ch];  // 读取 FPGA 寄存器          │  │  │
│  │  │  }                                                 │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

**AXI-Lite 时序**:

```
主机写操作 (SetDO):
     ____      ____      ____      ____      ____
CLK _|    |____|    |____|    |____|    |____|    |____

AWVALID ________/‾‾‾‾‾‾‾‾‾\___________________________
AWREADY ________________/‾‾‾‾‾‾‾‾‾\___________________
AWADDR  ========X 0x100 X=============================

WVALID  ________/‾‾‾‾‾‾‾‾‾\___________________________
WREADY  ________________/‾‾‾‾‾‾‾‾‾\___________________
WDATA   ========X  0x01  X=============================

BVALID  ________________________/‾‾‾‾‾‾‾‾‾\___________
BREADY  ________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\___________

写延迟: ~2-3 个时钟周期 (@100MHz = 20-30ns)

主机读操作 (GetVcValue):
     ____      ____      ____      ____      ____
CLK _|    |____|    |____|    |____|    |____|    |____

ARVALID ________/‾‾‾‾‾‾‾‾‾\___________________________
ARREADY ________________/‾‾‾‾‾‾‾‾‾\___________________
ARADDR  ========X 0x200 X=============================

RVALID  ________________________/‾‾‾‾‾‾‾‾‾\___________
RREADY  ________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\___________
RDATA   ========================X 0x1234 X===========

读延迟: ~2-3 个时钟周期
```

#### 2.2.2 UIO (Userspace I/O) 驱动

**Device Tree 配置** (设备树定义):

```dts
/ {
    amba_pl: amba_pl {
        #address-cells = <1>;
        #size-cells = <1>;
        compatible = "simple-bus";
        ranges;

        bpm_regs: bpm_regs@43c00000 {
            compatible = "generic-uio";
            reg = <0x43c00000 0x10000>;  // 基地址 + 大小
            interrupt-parent = <&intc>;
            interrupts = <0 29 4>;       // 中断号
        };
    };
};
```

**加载 UIO 驱动**:

```bash
# 检查 UIO 设备
$ ls /dev/uio*
/dev/uio0  /dev/uio1

# 查看 UIO 信息
$ cat /sys/class/uio/uio0/name
bpm_regs

$ cat /sys/class/uio/uio0/maps/map0/addr
0x43c00000

$ cat /sys/class/uio/uio0/maps/map0/size
0x10000
```

**liblowlevel.so 实现示例**:

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#define UIO_DEVICE "/dev/uio0"
#define MAP_SIZE 0x10000

static int fd;
static volatile void *fpga_base = NULL;

// 寄存器偏移定义（FPGA 团队提供）
#define RF_AMP_BASE     0x0010
#define RF_PHASE_BASE   0x0014
#define DO_REG          0x0100
#define DI_REG          0x0104
#define BPM_VC_BASE     0x0200
#define BPM_K1_BASE     0x0300
#define CTRL_REG        0x0000
#define STATUS_REG      0x0004
#define FIFO_DATA       0x1000

// 系统初始化
int SystemInit(void)
{
    fd = open(UIO_DEVICE, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Failed to open UIO device");
        return -1;
    }

    fpga_base = mmap(NULL, MAP_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);

    if (fpga_base == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }

    printf("FPGA mapped to %p\n", fpga_base);
    return 0;
}

// 读取 RF 信息
void GetRfInfo(int channel, float *amp, float *phase)
{
    volatile uint32_t *reg_amp = (uint32_t*)(fpga_base + RF_AMP_BASE);
    volatile uint32_t *reg_phase = (uint32_t*)(fpga_base + RF_PHASE_BASE);

    // 读取原始值（每个通道占 2 个寄存器）
    uint32_t raw_amp = reg_amp[channel * 2];
    uint32_t raw_phase = reg_phase[channel * 2];

    // 单位转换（FPGA 团队提供公式）
    *amp = (float)raw_amp / 1.28e6 * sqrt(2);  // -> RMS 电压
    *phase = (float)raw_phase * 360.0 / 65536; // -> 度数
}

// 设置数字输出
void SetDO(int channel, int value)
{
    volatile uint32_t *reg = (uint32_t*)(fpga_base + DO_REG);
    reg[channel] = value;
}

// 获取数字输入
void GetDI(int channel, int *value)
{
    volatile uint32_t *reg = (uint32_t*)(fpga_base + DI_REG);
    *value = reg[channel];
}

// 获取 BPM 电压值
int GetVcValue(int channel)
{
    volatile uint32_t *reg = (uint32_t*)(fpga_base + BPM_VC_BASE);
    return (int)reg[channel];
}

// 设置 BPM K1 系数
void SetBPMk1(int channel, float value)
{
    volatile uint32_t *reg = (uint32_t*)(fpga_base + BPM_K1_BASE);
    
    // 浮点 -> 定点转换 (Q15 格式)
    int32_t fixed_point = (int32_t)(value * 32767);
    reg[channel] = (uint32_t)fixed_point;
}

// 读取触发数据（批量）
void GetTriggerAllData(int sel, int channel, float *data)
{
    volatile uint32_t *fifo = (uint32_t*)(fpga_base + FIFO_DATA);
    volatile uint32_t *ctrl = (uint32_t*)(fpga_base + CTRL_REG);
    
    // 配置读取通道
    *ctrl = (sel << 8) | channel;
    
    // 等待数据准备好
    volatile uint32_t *status = (uint32_t*)(fpga_base + STATUS_REG);
    while ((*status & 0x01) == 0) {
        usleep(10);
    }
    
    // 读取 10000 个数据点
    for (int i = 0; i < 10000; i++) {
        data[i] = (float)fifo[0];  // FIFO 自动递增
    }
}

// 等待触发数据准备好
int TriggerAllDataReached(void)
{
    volatile uint32_t *status = (uint32_t*)(fpga_base + STATUS_REG);
    return (*status & 0x02) ? 1 : 0;
}

// 系统清理
void SystemCleanup(void)
{
    if (fpga_base != NULL) {
        munmap((void*)fpga_base, MAP_SIZE);
    }
    if (fd >= 0) {
        close(fd);
    }
}
```

#### 2.2.3 优缺点

**优点**:
- ✅ **ARM + FPGA 紧密集成**: 低延迟 (~20ns)
- ✅ **简单易用**: UIO 驱动无需内核编程
- ✅ **用户态访问**: mmap() 直接访问寄存器
- ✅ **成本低**: 单芯片方案
- ✅ **功耗低**: 适合嵌入式应用
- ✅ **丰富的 IP 核**: Xilinx 提供完整生态

**缺点**:
- ❌ **平台绑定**: 依赖 Xilinx Zynq 平台
- ❌ **带宽限制**: AXI GP 端口 ~1 GB/s（不如 PCIe）
- ❌ **内存共享**: DDR 被 ARM 和 FPGA 共享，可能有冲突
- ❌ **调试工具**: 不如 PCIe 成熟

#### 2.2.4 适用场景

- **嵌入式 BPM 系统**（如 BPMIOC）
- 中等数据量的实时控制（< 500 MB/s）
- 需要低功耗的应用
- 要求 ARM + FPGA 紧密协作的场景

#### 2.2.5 BPMIOC 实际应用

在 BPMIOC 中，`liblowlevel.so` 封装了所有寄存器访问，EPICS IOC 通过 `driverWrapper.c` 调用：

```c
// driverWrapper.c 中的动态加载
static long InitDevice()
{
    void *handle = dlopen("liblowlevel.so", RTLD_NOW);
    
    // 映射硬件函数
    funcGetRfInfo = dlsym(handle, "GetRfInfo");
    funcGetVcValue = dlsym(handle, "GetVcValue");
    funcSetBPMk1 = dlsym(handle, "SetBPMk1");
    // ...
    
    // 初始化硬件
    int (*funcOpen)() = dlsym(handle, "SystemInit");
    funcOpen();
    
    return 0;
}

// 驱动层调用硬件函数
float ReadData(int offset, int channel, int type)
{
    switch(offset) {
        case 0:  // RF 信息
            float amp, phase;
            funcGetRfInfo(channel, &amp, &phase);
            return (type == AMP) ? amp : phase;
        
        case 5:  // BPM 电压
            return funcGetVcValue(channel);
        
        // ...
    }
}
```

---

### 2.3 Etherbone / Wishbone 接口

#### 2.3.1 工作原理

Etherbone 是一种通过以太网访问 Wishbone 总线的协议，常用于远程访问 FPGA。

**架构图**:

```
┌─────────────────────────────────────────────────────────────┐
│                      IOC Host (任意平台)                     │
│  ┌────────────────────────────────────────────────────────┐  │
│  │            EPICS IOC Process                           │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Device/Driver Support                            │  │  │
│  │  └────────────────────┬─────────────────────────────┘  │  │
│  └───────────────────────┼────────────────────────────────┘  │
│                          │                                   │
│  ┌───────────────────────▼────────────────────────────────┐  │
│  │        libetherbone.so (用户态库)                      │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  eb_device_t dev;                                 │  │  │
│  │  │  eb_open(dev, "udp/192.168.1.100");              │  │  │
│  │  │  eb_read(dev, addr, &value);  // 读寄存器        │  │  │
│  │  │  eb_write(dev, addr, value);  // 写寄存器        │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └───────────────────────┬────────────────────────────────┘  │
└──────────────────────────┼─────────────────────────────────────┘
                           │
                           │ UDP/IP (端口 20000)
                           │ Etherbone 协议封装
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                    FPGA (with Ethernet)                      │
│  ┌────────────────────────────────────────────────────────┐  │
│  │           Ethernet MAC + IP/UDP Stack                  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Etherbone Core (Wishbone Master)                 │  │  │
│  │  │  - 解析 Etherbone 包                              │  │  │
│  │  │  - 转换为 Wishbone 事务                           │  │  │
│  │  └────────────────────┬─────────────────────────────┘  │  │
│  └───────────────────────┼────────────────────────────────┘  │
│                          │ Wishbone Bus                      │
│  ┌───────────────────────▼────────────────────────────────┐  │
│  │         Wishbone Crossbar / Interconnect               │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │  │
│  │  │ Slave 0:     │  │ Slave 1:     │  │ Slave 2:     │  │  │
│  │  │ 控制寄存器    │  │ ADC 数据     │  │ FIFO         │  │  │
│  │  │ 0x00000000   │  │ 0x00010000   │  │ 0x00020000   │  │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

**Etherbone 包格式**:

```
UDP Packet:
┌──────────────────────────────────────────────────────────┐
│  Ethernet Header (14 bytes)                              │
├──────────────────────────────────────────────────────────┤
│  IP Header (20 bytes)                                    │
├──────────────────────────────────────────────────────────┤
│  UDP Header (8 bytes)                                    │
│  - Source Port: 随机                                     │
│  - Dest Port: 20000                                      │
├──────────────────────────────────────────────────────────┤
│  Etherbone Header (8 bytes)                              │
│  - Magic: 0x4E6F                                         │
│  - Version: 1                                            │
│  - Flags: Read/Write                                     │
├──────────────────────────────────────────────────────────┤
│  Wishbone Record (可变长度)                              │
│  ┌────────────────────────────────────────────────────┐  │
│  │  Write Cycle:                                      │  │
│  │  - Address: 0x00001000                             │  │
│  │  - Data:    0x12345678                             │  │
│  └────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────┐  │
│  │  Read Cycle:                                       │  │
│  │  - Address: 0x00001004                             │  │
│  │  - (Data 在响应包中)                               │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

#### 2.3.2 EPICS 集成示例

```c
#include <etherbone.h>

static eb_device_t eb_device;
static eb_socket_t eb_socket;

// 初始化
static long init_device(void)
{
    eb_status_t status;
    
    // 打开 Etherbone socket
    status = eb_socket_open(EB_ABI_CODE, 0, EB_ADDR32|EB_DATA32, &eb_socket);
    if (status != EB_OK) {
        printf("Failed to open Etherbone socket\n");
        return -1;
    }
    
    // 连接到 FPGA
    status = eb_device_open(eb_socket, "udp/192.168.1.100", EB_ADDR32|EB_DATA32,
                            3, &eb_device);  // 3 次重试
    if (status != EB_OK) {
        printf("Failed to connect to FPGA\n");
        return -1;
    }
    
    printf("Connected to FPGA via Etherbone\n");
    return 0;
}

// 读取寄存器
static long read_ai(aiRecord *record)
{
    recordpara_t *priv = record->dpvt;
    eb_data_t data;
    eb_cycle_t cycle;
    
    // 开始一个读周期
    eb_cycle_open(eb_device, 0, NULL, &cycle);
    eb_cycle_read(cycle, priv->offset, EB_DATA32, &data);
    eb_cycle_close(cycle);
    
    // 单位转换
    record->val = (float)data / 1.28e6 * sqrt(2);
    return 0;
}

// 写入寄存器
static long write_ao(aoRecord *record)
{
    recordpara_t *priv = record->dpvt;
    eb_cycle_t cycle;
    
    // 浮点 -> 定点
    int32_t fixed_val = (int32_t)(record->val * 32767);
    
    // 开始一个写周期
    eb_cycle_open(eb_device, 0, NULL, &cycle);
    eb_cycle_write(cycle, priv->offset, EB_DATA32, (eb_data_t)fixed_val);
    eb_cycle_close(cycle);
    
    return 0;
}

// 批量读取（优化性能）
void read_waveform(int offset, float *data, int nelem)
{
    eb_cycle_t cycle;
    eb_data_t buffer[10000];
    
    // 批量读取
    eb_cycle_open(eb_device, 0, NULL, &cycle);
    for (int i = 0; i < nelem; i++) {
        eb_cycle_read(cycle, offset + i*4, EB_DATA32, &buffer[i]);
    }
    eb_cycle_close(cycle);
    
    // 单位转换
    for (int i = 0; i < nelem; i++) {
        data[i] = (float)buffer[i] / 1.28e6 * sqrt(2);
    }
}
```

#### 2.3.3 优缺点

**优点**:
- ✅ **远程访问**: 通过网络访问 FPGA，无需物理接触
- ✅ **平台无关**: 任何支持以太网的主机都可以使用
- ✅ **简单集成**: libetherbone 库易于使用
- ✅ **标准化**: Wishbone 是开放标准
- ✅ **低成本**: 只需以太网接口

**缺点**:
- ❌ **带宽受限**: 千兆以太网 ~100 MB/s
- ❌ **延迟高**: 网络延迟 ~100 μs（不适合实时控制）
- ❌ **不可靠**: UDP 可能丢包
- ❌ **FPGA 资源**: 需要 Ethernet MAC + IP/UDP 栈

#### 2.3.4 适用场景

- 远程监控和配置
- 非实时数据采集
- 分布式系统（FPGA 在机房，IOC 在控制室）
- 测试和调试

---

### 2.4 SPI / I2C / UART 自定义驱动

#### 2.4.1 工作原理

这些低速串行接口常用于配置 FPGA 内部的慢速外设（如 ADC、DAC、传感器）。

**典型应用场景**:

```
┌─────────────────────────────────────────────────────────────┐
│                        FPGA                                  │
│  ┌────────────────────────────────────────────────────────┐  │
│  │           Main Logic (高速数据处理)                    │  │
│  └──────────────┬─────────────────────────────────────────┘  │
│                 │                                             │
│  ┌──────────────▼──────────────────┐                         │
│  │  Configuration Registers        │                         │
│  │  (通过 AXI/Wishbone 访问)       │                         │
│  └──────────────┬──────────────────┘                         │
│                 │                                             │
│  ┌──────────────▼──────────────────┐                         │
│  │  SPI Master Controller          │                         │
│  │  - SCLK, MOSI, MISO, CS        │                         │
│  └──────────────┬──────────────────┘                         │
└─────────────────┼────────────────────────────────────────────┘
                  │ SPI Bus
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                   External ADC Chip                          │
│  (例如: AD9467, 16-bit, 250 MSPS)                           │
│  - 配置寄存器: 采样率、输入范围、测试模式                   │
│  - SPI Slave 接口                                            │
└───────────────────────────────────────────────────────────────┘
```

**IOC 访问流程**:

```
EPICS IOC                  FPGA SPI Master            ADC Chip
   │                             │                        │
   ├─ caput ADC_SampleRate ────►│                        │
   │   (设置采样率=200MHz)        │                        │
   │                             │                        │
   │                             ├─ SPI Transaction ─────►│
   │                             │   Addr: 0x01           │
   │                             │   Data: 0x08           │
   │                             │   (200MHz code)        │
   │                             │                        │
   │                             │◄─ SPI ACK ─────────────┤
   │◄─ OK ───────────────────────┤                        │
   │                             │                        │
```

#### 2.4.2 Linux 驱动方式

**方法 1: 通过 FPGA 寄存器位操作（最常用）**

```c
// FPGA 提供 SPI 控制器寄存器
#define SPI_CTRL_REG    0x1000  // 控制寄存器
#define SPI_DATA_REG    0x1004  // 数据寄存器
#define SPI_STATUS_REG  0x1008  // 状态寄存器

// 位定义
#define SPI_START       (1 << 0)
#define SPI_BUSY        (1 << 0)
#define SPI_DONE        (1 << 1)

// IOC 驱动层函数
void spi_write_adc_reg(uint8_t addr, uint8_t data)
{
    volatile uint32_t *ctrl = fpga_base + SPI_CTRL_REG;
    volatile uint32_t *data_reg = fpga_base + SPI_DATA_REG;
    volatile uint32_t *status = fpga_base + SPI_STATUS_REG;
    
    // 配置 SPI 事务（地址 + 数据）
    *data_reg = (addr << 8) | data;
    
    // 启动 SPI 传输
    *ctrl = SPI_START;
    
    // 等待完成
    while ((*status & SPI_BUSY) != 0) {
        usleep(10);
    }
    
    // 检查状态
    if ((*status & SPI_DONE) == 0) {
        printf("SPI write failed!\n");
    }
}

// EPICS AO record 调用
static long write_ao(aoRecord *record)
{
    recordpara_t *priv = record->dpvt;
    
    if (priv->type == ADC_CONFIG) {
        uint8_t reg_val = (uint8_t)record->val;
        spi_write_adc_reg(priv->offset, reg_val);
    }
    
    return 0;
}
```

**方法 2: 使用 Linux SPI 子系统**

```c
#include <linux/spi/spidev.h>

static int spi_fd;

// 初始化
int spi_init(void)
{
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 1000000;  // 1 MHz
    
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("Failed to open SPI device");
        return -1;
    }
    
    // 配置 SPI 模式
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    
    return 0;
}

// SPI 传输
int spi_transfer(uint8_t *tx_buf, uint8_t *rx_buf, int len)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = len,
        .speed_hz = 1000000,
        .bits_per_word = 8,
    };
    
    return ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

// 写 ADC 寄存器
void write_adc_reg(uint8_t addr, uint8_t data)
{
    uint8_t tx[2] = {addr, data};
    uint8_t rx[2] = {0};
    
    spi_transfer(tx, rx, 2);
}
```

#### 2.4.3 I2C 示例

```c
#include <linux/i2c-dev.h>

static int i2c_fd;

// 初始化
int i2c_init(void)
{
    i2c_fd = open("/dev/i2c-0", O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open I2C device");
        return -1;
    }
    
    // 设置从机地址
    if (ioctl(i2c_fd, I2C_SLAVE, 0x50) < 0) {  // EEPROM 地址
        perror("Failed to set I2C slave address");
        return -1;
    }
    
    return 0;
}

// 读 EEPROM
int eeprom_read(uint16_t addr, uint8_t *data, int len)
{
    uint8_t addr_buf[2] = {(addr >> 8) & 0xFF, addr & 0xFF};
    
    // 写地址
    if (write(i2c_fd, addr_buf, 2) != 2) {
        perror("Failed to write address");
        return -1;
    }
    
    // 读数据
    if (read(i2c_fd, data, len) != len) {
        perror("Failed to read data");
        return -1;
    }
    
    return 0;
}

// 写 EEPROM
int eeprom_write(uint16_t addr, uint8_t *data, int len)
{
    uint8_t buf[258];  // 地址 (2) + 数据 (最多 256)
    
    buf[0] = (addr >> 8) & 0xFF;
    buf[1] = addr & 0xFF;
    memcpy(&buf[2], data, len);
    
    if (write(i2c_fd, buf, len + 2) != len + 2) {
        perror("Failed to write");
        return -1;
    }
    
    usleep(5000);  // EEPROM 写入时间
    return 0;
}
```

#### 2.4.4 优缺点

**优点**:
- ✅ **简单**: 协议简单，易于实现
- ✅ **低成本**: 只需少量 I/O 引脚
- ✅ **低功耗**: 适合配置和慢速传感器
- ✅ **标准化**: Linux 提供标准驱动

**缺点**:
- ❌ **速度慢**: SPI ~10 MHz, I2C ~400 kHz
- ❌ **不适合高速数据**: 只能用于控制和配置
- ❌ **距离限制**: 通常 < 1 米

#### 2.4.5 适用场景

- FPGA 内部 ADC/DAC 配置
- EEPROM 读写（存储校准数据）
- 温度传感器、电源监控
- 前面板 LED/按钮控制

---

### 2.5 纯内存映射寄存器访问 (/dev/uio, /dev/mem)

#### 2.5.1 /dev/mem vs /dev/uio 对比

**(/dev/mem** - 传统方法):

```c
#include <fcntl.h>
#include <sys/mman.h>

#define FPGA_BASE_ADDR  0x43C00000
#define MAP_SIZE        0x10000

int fd = open("/dev/mem", O_RDWR | O_SYNC);
if (fd < 0) {
    perror("Cannot open /dev/mem");
    exit(1);
}

// 映射物理地址
void *fpga_base = mmap(NULL, MAP_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd,
                       FPGA_BASE_ADDR);  // 物理地址

if (fpga_base == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
}

// 直接访问寄存器
volatile uint32_t *reg = (uint32_t*)fpga_base;
reg[0] = 0x12345678;  // 写入
uint32_t val = reg[1];  // 读取
```

**问题**:
- 需要 root 权限
- 不安全（可以访问任意物理地址）
- 现代 Linux 内核可能禁用 `/dev/mem`

**(/dev/uio** - 推荐方法):

```c
// 见前面 2.2.2 节的 UIO 示例
```

**优势**:
- 不需要 root 权限（配置 udev 规则）
- 安全（只能访问指定设备）
- 支持中断通知
- 设备树自动管理

#### 2.5.2 性能对比

| 接口类型           | 读延迟    | 写延迟    | 带宽      | 适用场景           |
|--------------------|----------|----------|----------|-------------------|
| PCIe (PIO)         | ~500ns   | ~500ns   | 1 GB/s   | 高性能系统         |
| PCIe (DMA)         | ~10μs    | ~10μs    | 8 GB/s   | 大数据传输         |
| Zynq AXI-Lite      | ~20ns    | ~20ns    | 1 GB/s   | 嵌入式实时控制     |
| Zynq AXI-HP (DMA)  | ~1μs     | ~1μs     | 2 GB/s   | 嵌入式数据采集     |
| Etherbone (1GbE)   | ~100μs   | ~100μs   | 100 MB/s | 远程访问           |
| SPI                | ~10μs    | ~10μs    | 1 MB/s   | 配置/慢速外设      |
| I2C                | ~100μs   | ~100μs   | 50 KB/s  | EEPROM/传感器      |

---

## 第二部分总结

1. **PCIe**: 高性能，适合桌面/服务器，需要内核驱动
2. **Zynq/AXI**: BPMIOC 使用方案，ARM + FPGA 集成，UIO 简化开发
3. **Etherbone**: 远程访问，适合分布式系统
4. **SPI/I2C**: 慢速配置接口，不适合数据采集
5. **UIO vs /dev/mem**: UIO 更安全，推荐使用

**选择建议**:

| 需求                     | 推荐方案              |
|-------------------------|----------------------|
| 高性能数据采集 (> 1 GB/s) | PCIe                 |
| 嵌入式实时控制           | Zynq AXI             |
| 远程监控配置             | Etherbone            |
| ADC/DAC 配置            | SPI                  |
| 传感器读取               | I2C                  |

---
