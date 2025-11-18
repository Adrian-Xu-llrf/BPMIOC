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

## 第三部分：IOC 开发者必备知识

作为 IOC 开发者（不编写 FPGA 代码），你需要理解 FPGA 侧提供的接口、如何验证硬件、以及如何在 IOC 中正确使用这些接口。

### 3.1 FPGA 团队必须提供的文档

#### 3.1.1 寄存器地址映射表 (Register Map)

这是最关键的文档！必须包含每个寄存器的详细信息。

**标准寄存器表格式**:

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         BPM FPGA Register Map v2.0                                  │
│                         基地址: 0x43C00000                                          │
├──────────┬──────────────────────┬─────┬────────┬────────────────────────────────────┤
│ 偏移地址  │  寄存器名称           │ 位宽 │ 读/写  │  说明                              │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0000   │ CTRL_REG             │ 32  │ R/W    │ 控制寄存器                          │
│          │  [0]    START        │     │        │   1: 启动采集                      │
│          │  [1]    STOP         │     │        │   1: 停止采集                      │
│          │  [2]    RESET        │     │        │   1: 复位（自清零）                │
│          │  [7:4]  MODE         │     │        │   采集模式 (0=连续, 1=单次, ...)    │
│          │  [31:8] RESERVED     │     │        │   保留                             │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0004   │ STATUS_REG           │ 32  │ R      │ 状态寄存器                          │
│          │  [0]    BUSY         │     │        │   1: 正在采集                      │
│          │  [1]    DATA_READY   │     │        │   1: 数据准备好                    │
│          │  [2]    ERROR        │     │        │   1: 发生错误                      │
│          │  [3]    FIFO_FULL    │     │        │   1: FIFO 满                       │
│          │  [4]    FIFO_EMPTY   │     │        │   1: FIFO 空                       │
│          │  [31:5] RESERVED     │     │        │   保留                             │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0008   │ VERSION_REG          │ 32  │ R      │ 固件版本                            │
│          │  [15:0]  MINOR       │     │        │   次版本号                         │
│          │  [31:16] MAJOR       │     │        │   主版本号                         │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0010   │ CH0_AMP              │ 32  │ R      │ 通道 0 幅度（有符号 Q20.12）        │
│          │                      │     │        │   范围: -1.0 ~ +1.0                │
│          │                      │     │        │   公式: amp = reg / 2^20           │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0014   │ CH0_PHASE            │ 32  │ R      │ 通道 0 相位（无符号 Q16）           │
│          │                      │     │        │   范围: 0 ~ 360 度                 │
│          │                      │     │        │   公式: phase = reg * 360 / 2^16   │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0018   │ CH1_AMP              │ 32  │ R      │ 通道 1 幅度                         │
│ 0x001C   │ CH1_PHASE            │ 32  │ R      │ 通道 1 相位                         │
│   ...    │   ...                │ ... │  ...   │  ...                               │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0100   │ DO_REG               │ 32  │ R/W    │ 数字输出                            │
│          │  [7:0]   DO_VALUE    │     │        │   输出值（每位控制一个 DO）         │
│          │  [31:8]  RESERVED    │     │        │   保留                             │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0104   │ DI_REG               │ 32  │ R      │ 数字输入                            │
│          │  [7:0]   DI_VALUE    │     │        │   输入值（只读）                   │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0200   │ BPM_VC_CH0           │ 32  │ R      │ BPM 电压值（通道 0）                │
│          │                      │     │        │   有符号整数，单位: LSB            │
│ 0x0204   │ BPM_VC_CH1           │ 32  │ R      │ BPM 电压值（通道 1）                │
│   ...    │   ...                │ ... │  ...   │  ...                               │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x0300   │ BPM_K1_CH0           │ 32  │ R/W    │ BPM K1 系数（通道 0）               │
│          │                      │     │        │   有符号定点 Q15 格式              │
│          │                      │     │        │   范围: -1.0 ~ +1.0                │
│          │                      │     │        │   公式: reg = k1 * 32767           │
│ 0x0304   │ BPM_K1_CH1           │ 32  │ R/W    │ BPM K1 系数（通道 1）               │
│   ...    │   ...                │ ... │  ...   │  ...                               │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x1000   │ WAVEFORM_FIFO        │ 32  │ R      │ 波形数据 FIFO                       │
│          │                      │     │        │   读取后自动递增到下一个数据       │
│          │                      │     │        │   深度: 10000 samples              │
├──────────┼──────────────────────┼─────┼────────┼────────────────────────────────────┤
│ 0x2000   │ TIMESTAMP_SEC        │ 64  │ R      │ White Rabbit 时间戳（秒）           │
│ 0x2008   │ TIMESTAMP_NSEC       │ 32  │ R      │ White Rabbit 时间戳（纳秒）         │
└──────────┴──────────────────────┴─────┴────────┴────────────────────────────────────┘
```

**关键信息**:
- ✅ 基地址 (Base Address)
- ✅ 每个寄存器的偏移地址
- ✅ 寄存器名称和功能
- ✅ 位宽和位域定义
- ✅ 读/写属性
- ✅ 数据格式（有符号/无符号、定点/浮点、单位）
- ✅ 转换公式

#### 3.1.2 寄存器位域定义 (Bit Fields)

对于复杂的寄存器，需要详细的位域说明：

```c
// CTRL_REG (0x0000) - 控制寄存器
// ┌──────────────────────────────────────────────────────────┐
// │ 31        24 │ 23        16 │ 15         8 │ 7          0 │
// ├──────────────┼──────────────┼──────────────┼──────────────┤
// │   RESERVED   │   RESERVED   │   RESERVED   │  MODE │S│S│S │
// │   (0x00)     │   (0x00)     │   (0x00)     │       │T│P│T │
// │              │              │              │       │A│ │R │
// │              │              │              │       │R│  │T │
// └──────────────┴──────────────┴──────────────┴──────────────┘
//                                                 [7:4] [2][1][0]

#define CTRL_START_BIT      0   // 启动采集
#define CTRL_STOP_BIT       1   // 停止采集
#define CTRL_RESET_BIT      2   // 复位
#define CTRL_MODE_SHIFT     4   // 模式位偏移
#define CTRL_MODE_MASK      0xF // 模式位掩码

// 模式定义
#define MODE_CONTINUOUS     0x0 // 连续采集
#define MODE_SINGLE_SHOT    0x1 // 单次采集
#define MODE_TRIGGERED      0x2 // 触发采集

// 使用示例
uint32_t ctrl_val = 0;
ctrl_val |= (1 << CTRL_START_BIT);              // 启动
ctrl_val |= (MODE_TRIGGERED << CTRL_MODE_SHIFT); // 触发模式
reg[CTRL_REG_OFFSET] = ctrl_val;
```

#### 3.1.3 读写时序要求

**关键时序参数**:

```
寄存器写操作时序:
─────────────────────────────────────────────────────────────
参数                  │ 最小值  │ 典型值  │ 最大值  │ 单位
─────────────────────────────────────────────────────────────
写建立时间 (tWS)      │ 10      │ 20      │ -       │ ns
写保持时间 (tWH)      │ 10      │ 20      │ -       │ ns
写周期时间 (tWC)      │ 40      │ 50      │ -       │ ns
写后延迟 (tWD)        │ 0       │ 0       │ 100     │ ns
─────────────────────────────────────────────────────────────

寄存器读操作时序:
─────────────────────────────────────────────────────────────
参数                  │ 最小值  │ 典型值  │ 最大值  │ 单位
─────────────────────────────────────────────────────────────
读建立时间 (tRS)      │ 10      │ 20      │ -       │ ns
读访问时间 (tRA)      │ -       │ 30      │ 50      │ ns
读保持时间 (tRH)      │ 5       │ 10      │ -       │ ns
─────────────────────────────────────────────────────────────

特殊时序要求:
─────────────────────────────────────────────────────────────
FIFO 读取间隔         │ 40      │ 50      │ -       │ ns
CTRL_REG 写后等待     │ 100     │ -       │ -       │ ns
STATUS_REG 轮询间隔   │ 1       │ 10      │ -       │ us
─────────────────────────────────────────────────────────────
```

**时序图示例**:

```
写操作时序:
         ______        ______        ______
CLK  ___|      |______|      |______|      |______
        ◄─tWC─►
              ┌───────────────┐
ADDR  ────────X  Valid Addr   X──────────────────
              ◄tWS►     ◄tWH►
                   ┌──────────┐
DATA  ─────────────X   Data   X──────────────────
                              ◄────tWD────►
                              ┌──────────────────
WE    ────────────────────────┘

读操作时序:
         ______        ______        ______
CLK  ___|      |______|      |______|      |______
              ┌───────────────┐
ADDR  ────────X  Valid Addr   X──────────────────
              ◄tRS►
                        ◄tRA► ┌──────────┐
DATA  ────────────────────────X   Data   X───────
                              ◄tRH►
              ┌───────────────┐
RE    ────────┘               └──────────────────
```

#### 3.1.4 握手协议与状态机

**FIFO 读取握手协议**:

```
步骤 1: 检查数据准备好
┌─────────────┐
│ IOC         │
│             ├──── 读 STATUS_REG ────────┐
│             │                            │
│             │◄─── DATA_READY = 1 ────────┤
└─────────────┘                            │
                                           │
步骤 2: 配置读取参数                       ▼
┌─────────────┐                    ┌──────────────┐
│ IOC         │                    │ FPGA         │
│             ├──── 写 CTRL_REG ───►│              │
│             │    (SELECT_CH=0)   │ 配置FIFO读取 │
│             │                    │ 指针到通道0  │
└─────────────┘                    └──────────────┘

步骤 3: 读取数据
┌─────────────┐                    ┌──────────────┐
│ IOC         │                    │ FPGA FIFO    │
│             ├──── 读 FIFO[0] ────►│  └─►[sample0]│
│ data[0]◄────┤◄─── sample0 ────────┤     [sample1]│
│             │                    │     [sample2]│
│             ├──── 读 FIFO[0] ────►│     [...]    │
│ data[1]◄────┤◄─── sample1 ────────┤              │
│             │    (自动递增)       │              │
│  ...        │                    │              │
└─────────────┘                    └──────────────┘

步骤 4: 完成读取
┌─────────────┐                    ┌──────────────┐
│ IOC         │                    │ FPGA         │
│             ├──── 写 CTRL_REG ───►│              │
│             │    (FIFO_RESET)    │ 复位FIFO指针 │
└─────────────┘                    └──────────────┘
```

**状态机定义**:

```
FPGA 采集状态机:
┌──────────┐
│  IDLE    │  初始状态
└────┬─────┘
     │ START = 1
     ▼
┌──────────┐
│  CONFIG  │  配置采集参数
└────┬─────┘
     │ 配置完成
     ▼
┌──────────┐
│  WAIT    │  等待触发
│  TRIGGER │
└────┬─────┘
     │ 触发到达
     ▼
┌──────────┐
│ACQUIRING │  正在采集
│          │  STATUS.BUSY = 1
└────┬─────┘
     │ 采集完成
     ▼
┌──────────┐
│  READY   │  数据准备好
│          │  STATUS.DATA_READY = 1
└────┬─────┘
     │ 读取完成 或 超时
     ▼
┌──────────┐
│  IDLE    │  返回初始状态
└──────────┘
```

#### 3.1.5 中断机制

**中断源定义**:

```
中断寄存器 (INT_REG, 0x0008):
┌─────────────────────────────────────────────────────────┐
│ Bit │ 名称              │ 说明                          │
├─────┼───────────────────┼───────────────────────────────┤
│  0  │ DATA_READY_INT    │ 数据准备好中断                │
│  1  │ FIFO_FULL_INT     │ FIFO 满中断                   │
│  2  │ ERROR_INT         │ 错误中断                      │
│  3  │ TRIGGER_INT       │ 触发到达中断                  │
│  4  │ WR_PPS_INT        │ White Rabbit PPS 中断         │
│ ... │ ...               │ ...                           │
└─────┴───────────────────┴───────────────────────────────┘

中断使能寄存器 (INT_ENABLE, 0x000C):
- 对应位置 1 使能该中断
- 默认全部禁用 (0x00000000)

中断处理流程:
1. FPGA 产生中断 → 设置 INT_REG 对应位
2. CPU 接收中断 → 调用中断处理函数
3. 读取 INT_REG → 确定中断源
4. 处理中断 → 执行相应操作
5. 清除中断 → 写 1 到 INT_REG 对应位（W1C: Write-1-to-Clear）
```

#### 3.1.6 DMA 描述符格式

**DMA 描述符结构**:

```c
// DMA 描述符 (64 bytes, 缓存行对齐)
struct dma_descriptor {
    uint64_t src_addr;      // 源地址（FPGA 内存）
    uint64_t dst_addr;      // 目标地址（主机内存）
    uint32_t length;        // 传输长度（字节）
    uint32_t flags;         // 控制标志
    uint32_t next_desc;     // 下一个描述符地址（链式 DMA）
    uint32_t reserved[9];   // 保留，对齐到 64 字节
} __attribute__((aligned(64)));

// flags 位定义
#define DMA_FLAG_ENABLE     (1 << 0)  // 使能 DMA
#define DMA_FLAG_INT_EN     (1 << 1)  // 使能完成中断
#define DMA_FLAG_LAST       (1 << 2)  // 最后一个描述符
#define DMA_FLAG_SRC_INC    (1 << 3)  // 源地址递增
#define DMA_FLAG_DST_INC    (1 << 4)  // 目标地址递增

// 示例：配置 DMA 传输
void setup_dma_transfer(void *src, void *dst, size_t len)
{
    struct dma_descriptor *desc = dma_desc_pool;
    
    desc->src_addr = (uint64_t)src;
    desc->dst_addr = (uint64_t)dst;
    desc->length = len;
    desc->flags = DMA_FLAG_ENABLE | DMA_FLAG_INT_EN | 
                  DMA_FLAG_LAST | DMA_FLAG_SRC_INC | DMA_FLAG_DST_INC;
    desc->next_desc = 0;  // 无下一个描述符
    
    // 写入描述符地址到 FPGA
    volatile uint32_t *dma_ctrl = fpga_base + DMA_CTRL_REG;
    dma_ctrl[0] = (uint32_t)((uint64_t)desc & 0xFFFFFFFF);
    dma_ctrl[1] = (uint32_t)((uint64_t)desc >> 32);
    
    // 启动 DMA
    dma_ctrl[2] = DMA_START;
}
```

### 3.2 如何确认 FPGA 接口可用

在开始 IOC 开发前，必须验证 FPGA 硬件正常工作。

#### 3.2.1 硬件自检程序

**编写独立的测试程序** (test_fpga.c):

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define UIO_DEVICE "/dev/uio0"
#define MAP_SIZE 0x10000

#define CTRL_REG        0x0000
#define STATUS_REG      0x0004
#define VERSION_REG     0x0008
#define CH0_AMP         0x0010
#define DO_REG          0x0100

int main(int argc, char **argv)
{
    int fd;
    volatile void *fpga_base;
    volatile uint32_t *reg;
    
    printf("=== FPGA Hardware Test ===\n\n");
    
    // 1. 打开 UIO 设备
    printf("Step 1: Opening UIO device %s...\n", UIO_DEVICE);
    fd = open(UIO_DEVICE, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Failed to open UIO device");
        return -1;
    }
    printf("  [OK] Device opened, fd=%d\n\n", fd);
    
    // 2. 映射内存
    printf("Step 2: Mapping FPGA memory...\n");
    fpga_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (fpga_base == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }
    printf("  [OK] Memory mapped to %p\n\n", fpga_base);
    
    reg = (uint32_t*)fpga_base;
    
    // 3. 读取版本号
    printf("Step 3: Reading firmware version...\n");
    uint32_t version = reg[VERSION_REG / 4];
    uint16_t major = (version >> 16) & 0xFFFF;
    uint16_t minor = version & 0xFFFF;
    printf("  Firmware Version: %d.%d\n", major, minor);
    if (version == 0 || version == 0xFFFFFFFF) {
        printf("  [ERROR] Invalid version! FPGA may not be configured.\n");
        goto cleanup;
    }
    printf("  [OK] Version read successfully\n\n");
    
    // 4. 测试读写（使用 DO 寄存器，因为它是 R/W）
    printf("Step 4: Testing register read/write...\n");
    uint32_t test_patterns[] = {0x00, 0xFF, 0xAA, 0x55};
    for (int i = 0; i < 4; i++) {
        reg[DO_REG / 4] = test_patterns[i];
        usleep(1000);  // 等待 1ms
        uint32_t readback = reg[DO_REG / 4];
        printf("  Write: 0x%02X, Read: 0x%02X ... ", 
               test_patterns[i], readback);
        if (readback == test_patterns[i]) {
            printf("[OK]\n");
        } else {
            printf("[FAIL]\n");
            goto cleanup;
        }
    }
    printf("  [OK] Register read/write test passed\n\n");
    
    // 5. 测试状态寄存器
    printf("Step 5: Reading status register...\n");
    uint32_t status = reg[STATUS_REG / 4];
    printf("  STATUS = 0x%08X\n", status);
    printf("    BUSY       = %d\n", (status >> 0) & 1);
    printf("    DATA_READY = %d\n", (status >> 1) & 1);
    printf("    ERROR      = %d\n", (status >> 2) & 1);
    printf("    FIFO_FULL  = %d\n", (status >> 3) & 1);
    printf("    FIFO_EMPTY = %d\n", (status >> 4) & 1);
    printf("  [OK] Status read successfully\n\n");
    
    // 6. 测试数据采集
    printf("Step 6: Testing data acquisition...\n");
    reg[CTRL_REG / 4] = 0x01;  // 启动采集
    printf("  Started acquisition\n");
    
    int timeout = 100;  // 100ms 超时
    while (timeout-- > 0) {
        status = reg[STATUS_REG / 4];
        if (status & 0x02) {  // DATA_READY
            printf("  [OK] Data ready after %d ms\n", 100 - timeout);
            break;
        }
        usleep(1000);
    }
    if (timeout <= 0) {
        printf("  [ERROR] Timeout waiting for data\n");
        goto cleanup;
    }
    
    // 读取一些数据
    uint32_t amp = reg[CH0_AMP / 4];
    printf("  CH0 Amplitude = 0x%08X (%d)\n", amp, amp);
    printf("  [OK] Data acquisition test passed\n\n");
    
    printf("=== All tests PASSED ===\n");
    
cleanup:
    munmap((void*)fpga_base, MAP_SIZE);
    close(fd);
    return 0;
}
```

**编译和运行**:

```bash
# 编译
$ gcc -o test_fpga test_fpga.c

# 运行（可能需要 sudo）
$ sudo ./test_fpga

# 预期输出
=== FPGA Hardware Test ===

Step 1: Opening UIO device /dev/uio0...
  [OK] Device opened, fd=3

Step 2: Mapping FPGA memory...
  [OK] Memory mapped to 0x7f8a3c000000

Step 3: Reading firmware version...
  Firmware Version: 2.0
  [OK] Version read successfully

Step 4: Testing register read/write...
  Write: 0x00, Read: 0x00 ... [OK]
  Write: 0xFF, Read: 0xFF ... [OK]
  Write: 0xAA, Read: 0xAA ... [OK]
  Write: 0x55, Read: 0x55 ... [OK]
  [OK] Register read/write test passed

Step 5: Reading status register...
  STATUS = 0x00000010
    BUSY       = 0
    DATA_READY = 0
    ERROR      = 0
    FIFO_FULL  = 0
    FIFO_EMPTY = 1
  [OK] Status read successfully

Step 6: Testing data acquisition...
  Started acquisition
  [OK] Data ready after 12 ms
  CH0 Amplitude = 0x00123456 (1193046)
  [OK] Data acquisition test passed

=== All tests PASSED ===
```

#### 3.2.2 寄存器读写验证脚本

**Python 脚本** (verify_registers.py):

```python
#!/usr/bin/env python3
import mmap
import struct
import time

UIO_DEVICE = "/dev/uio0"
MAP_SIZE = 0x10000

# 寄存器定义
REGS = {
    'CTRL':    0x0000,
    'STATUS':  0x0004,
    'VERSION': 0x0008,
    'CH0_AMP': 0x0010,
    'DO':      0x0100,
}

def read_reg(mm, offset):
    """读取 32-bit 寄存器"""
    mm.seek(offset)
    return struct.unpack('I', mm.read(4))[0]

def write_reg(mm, offset, value):
    """写入 32-bit 寄存器"""
    mm.seek(offset)
    mm.write(struct.pack('I', value))

def main():
    with open(UIO_DEVICE, "r+b") as f:
        mm = mmap.mmap(f.fileno(), MAP_SIZE)
        
        print("=== Register Verification ===\n")
        
        # 读取版本
        version = read_reg(mm, REGS['VERSION'])
        major = (version >> 16) & 0xFFFF
        minor = version & 0xFFFF
        print(f"Firmware Version: {major}.{minor}")
        
        # 读取状态
        status = read_reg(mm, REGS['STATUS'])
        print(f"Status: 0x{status:08X}")
        print(f"  BUSY:       {(status >> 0) & 1}")
        print(f"  DATA_READY: {(status >> 1) & 1}")
        
        # 测试读写
        print("\nTesting DO register:")
        for val in [0x00, 0xFF, 0xAA, 0x55]:
            write_reg(mm, REGS['DO'], val)
            time.sleep(0.001)
            readback = read_reg(mm, REGS['DO'])
            status = "OK" if readback == val else "FAIL"
            print(f"  Write: 0x{val:02X}, Read: 0x{readback:02X} ... [{status}]")
        
        # 读取采集数据
        print("\nReading CH0 amplitude:")
        amp = read_reg(mm, REGS['CH0_AMP'])
        print(f"  Raw value: 0x{amp:08X} ({amp})")
        
        mm.close()

if __name__ == "__main__":
    main()
```

#### 3.2.3 回环测试

**数字 I/O 回环测试**:

```c
// 硬件连接: DO[0] -> DI[0], DO[1] -> DI[1], ...

int test_digital_loopback(volatile uint32_t *reg)
{
    printf("Digital I/O Loopback Test:\n");
    
    uint8_t test_patterns[] = {0x00, 0xFF, 0xAA, 0x55, 0x0F, 0xF0};
    
    for (int i = 0; i < 6; i++) {
        // 写入 DO
        reg[DO_REG / 4] = test_patterns[i];
        usleep(1000);  // 等待稳定
        
        // 读取 DI
        uint8_t di_value = reg[DI_REG / 4] & 0xFF;
        
        printf("  Pattern 0x%02X -> DI: 0x%02X ... ", 
               test_patterns[i], di_value);
        
        if (di_value == test_patterns[i]) {
            printf("[PASS]\n");
        } else {
            printf("[FAIL]\n");
            return -1;
        }
    }
    
    printf("  [OK] All loopback tests passed\n");
    return 0;
}
```

**ADC 数据范围检查**:

```c
int test_adc_data_range(volatile uint32_t *reg)
{
    printf("ADC Data Range Test:\n");
    
    // 启动采集
    reg[CTRL_REG / 4] = 0x01;
    
    // 等待数据准备好
    int timeout = 100;
    while (timeout-- > 0) {
        if (reg[STATUS_REG / 4] & 0x02) break;
        usleep(1000);
    }
    
    if (timeout <= 0) {
        printf("  [ERROR] Timeout\n");
        return -1;
    }
    
    // 读取多个通道
    for (int ch = 0; ch < 8; ch++) {
        int32_t amp = reg[(CH0_AMP + ch * 8) / 4];
        
        // 检查范围（假设是有符号 20-bit）
        if (amp < -(1 << 19) || amp > (1 << 19)) {
            printf("  CH%d: Out of range! (0x%08X)\n", ch, amp);
            return -1;
        }
        
        printf("  CH%d: 0x%08X (%d) ... [OK]\n", ch, amp, amp);
    }
    
    return 0;
}
```

#### 3.2.4 时序验证工具

**使用示波器/逻辑分析仪**:

如果有硬件调试工具，可以监控：
- AXI 总线时序（AWVALID, WVALID, RVALID 等）
- 中断信号时序
- 数字 I/O 电平

**软件时序测试**:

```c
#include <time.h>

// 测量寄存器访问延迟
void measure_register_latency(volatile uint32_t *reg)
{
    struct timespec start, end;
    int iterations = 10000;
    
    // 测量读延迟
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        volatile uint32_t dummy = reg[STATUS_REG / 4];
        (void)dummy;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long long ns = (end.tv_sec - start.tv_sec) * 1000000000LL +
                   (end.tv_nsec - start.tv_nsec);
    printf("Average read latency: %lld ns\n", ns / iterations);
    
    // 测量写延迟
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        reg[DO_REG / 4] = i;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    ns = (end.tv_sec - start.tv_sec) * 1000000000LL +
         (end.tv_nsec - start.tv_nsec);
    printf("Average write latency: %lld ns\n", ns / iterations);
}
```

### 3.3 如何在 IOC 中绑定寄存器地址

#### 3.3.1 使用 offset 映射方案（BPMIOC 方法）

**优点**:
- 灵活：可以轻松更改映射关系
- 集中管理：所有映射逻辑在驱动层
- 易于维护：修改不需要重新编译数据库

**offset 定义表**:

```c
// driverWrapper.c 中定义 offset 映射
/*
 * Offset 映射表:
 * ┌────────┬──────────────────────┬─────────────────────────┐
 * │ Offset │ 功能                  │ 对应的 FPGA 寄存器       │
 * ├────────┼──────────────────────┼─────────────────────────┤
 * │   0    │ RF 信息 (幅度/相位)   │ CH*_AMP, CH*_PHASE      │
 * │   1    │ 数字输入              │ DI_REG                  │
 * │   2    │ FPGA LED 0           │ LED0_REG                │
 * │   5    │ BPM 电压值            │ BPM_VC_CH*              │
 * │   7    │ XY 位置               │ XY_POS_CH*              │
 * │   10   │ 设置 BPM K1          │ BPM_K1_CH*              │
 * │   11   │ 设置 BPM K2          │ BPM_K2_CH*              │
 * │   ...  │ ...                  │ ...                     │
 * └────────┴──────────────────────┴─────────────────────────┘
 */

float ReadData(int offset, int channel, int type)
{
    switch(offset)
    {
        case 0:  // RF 信息
            if(type == AMP)
                return GetRFInfo(channel, 0);  // 读取幅度
            else if(type == PHASE)
                return GetRFInfo(channel, 1);  // 读取相位
            break;
            
        case 1:  // 数字输入
            return GetDI(channel);
            
        case 5:  // BPM 电压值
            // 直接调用硬件函数
            return funcGetVcValue(channel);
            
        case 7:  // XY 位置
            return funcGetxyPosition(channel);
            
        // ... 更多 offset ...
    }
}

void SetReg(int offset, int channel, float val)
{
    switch(offset)
    {
        case 0:  // 数字输出
            funcSetDO(channel, (int)val);
            break;
            
        case 10:  // BPM K1 系数
            funcSetBPMk1(channel, val);
            break;
            
        case 11:  // BPM K2 系数
            funcSetBPMk2(channel, val);
            break;
            
        // ... 更多 offset ...
    }
}
```

**数据库中使用**:

```
# BPMMonitor.db
record(ai, "$(P):RFIn_03_Amp")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=2")    # offset=0, channel=2, type=AMP
    field(SCAN, ".5 second")
}

record(ao, "$(P):BPM1_K1_Set")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:10 ch=0")   # offset=10, channel=0
    field(SCAN, "Passive")
}
```

#### 3.3.2 直接地址映射方案

**优点**:
- 直观：INP 字段直接对应物理地址
- 性能：减少一层 switch 判断

**缺点**:
- 不灵活：更改地址需要修改数据库
- 容易出错：手动计算地址偏移

**实现方式**:

```c
// Device Support 中直接使用地址
static long read_ai(aiRecord *record)
{
    recordpara_t *priv = record->dpvt;
    volatile uint32_t *reg = fpga_base + priv->offset;
    
    // 直接读取寄存器
    uint32_t raw_value = reg[priv->channel];
    
    // 单位转换
    record->val = (float)raw_value / 1.28e6 * sqrt(2);
    return 0;
}
```

**数据库中使用**:

```
# 直接使用寄存器偏移地址
record(ai, "$(P):CH0_Amp")
{
    field(DTYP, "DirectReg")
    field(INP,  "@0x0010 ch=0")   # 地址 0x0010, 通道 0
    field(SCAN, ".5 second")
}
```

#### 3.3.3 符号名映射方案

**使用字符串名称代替数字**:

```c
// 定义符号名到地址的映射表
typedef struct {
    const char *name;
    uint32_t addr;
    int channels;
} reg_map_t;

static const reg_map_t reg_map[] = {
    {"CH_AMP",    0x0010, 10},  // 10 个通道
    {"CH_PHASE",  0x0014, 10},
    {"BPM_VC",    0x0200, 8},
    {"BPM_K1",    0x0300, 8},
    {"DO",        0x0100, 8},
    {NULL, 0, 0}
};

// 解析符号名
static uint32_t resolve_register(const char *name, int channel)
{
    for (int i = 0; reg_map[i].name != NULL; i++) {
        if (strcmp(name, reg_map[i].name) == 0) {
            if (channel >= reg_map[i].channels) {
                printf("Error: channel %d out of range for %s\n", 
                       channel, name);
                return 0;
            }
            return reg_map[i].addr + channel * 4;
        }
    }
    printf("Error: unknown register name '%s'\n", name);
    return 0;
}
```

**数据库中使用**:

```
record(ai, "$(P):CH0_Amp")
{
    field(DTYP, "SymbolReg")
    field(INP,  "@CH_AMP ch=0")   # 使用符号名
    field(SCAN, ".5 second")
}
```

### 3.4 解析状态、发出控制命令

#### 3.4.1 状态解析

**位域解析示例**:

```c
// 解析 STATUS 寄存器
typedef struct {
    uint8_t busy        : 1;  // bit 0
    uint8_t data_ready  : 1;  // bit 1
    uint8_t error       : 1;  // bit 2
    uint8_t fifo_full   : 1;  // bit 3
    uint8_t fifo_empty  : 1;  // bit 4
    uint32_t reserved   : 27; // bit 31:5
} status_reg_t;

// 使用方式 1: 位域结构
status_reg_t *status = (status_reg_t*)&reg[STATUS_REG / 4];
if (status->data_ready) {
    printf("Data is ready!\n");
}

// 使用方式 2: 位掩码
uint32_t status_val = reg[STATUS_REG / 4];
#define STATUS_BUSY        (1 << 0)
#define STATUS_DATA_READY  (1 << 1)
#define STATUS_ERROR       (1 << 2)

if (status_val & STATUS_DATA_READY) {
    printf("Data is ready!\n");
}

// 使用方式 3: 提取多位字段
#define STATUS_MODE_SHIFT  8
#define STATUS_MODE_MASK   0xF
uint8_t mode = (status_val >> STATUS_MODE_SHIFT) & STATUS_MODE_MASK;
```

**在 Device Support 中使用**:

```c
static long read_bi(biRecord *record)
{
    recordpara_t *priv = record->dpvt;
    
    if (priv->offset == STATUS_OFFSET) {
        uint32_t status = funcGetStatus();
        
        // 根据 channel 返回不同的状态位
        switch (priv->channel) {
            case 0:  // BUSY
                record->rval = (status & STATUS_BUSY) ? 1 : 0;
                break;
            case 1:  // DATA_READY
                record->rval = (status & STATUS_DATA_READY) ? 1 : 0;
                break;
            case 2:  // ERROR
                record->rval = (status & STATUS_ERROR) ? 1 : 0;
                break;
        }
    }
    
    return 0;
}
```

**数据库定义**:

```
record(bi, "$(P):Status_Busy")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@STATUS:0 ch=0")  # STATUS 寄存器, bit 0
    field(SCAN, ".1 second")
    field(ZNAM, "Idle")
    field(ONAM, "Busy")
}

record(bi, "$(P):Status_DataReady")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@STATUS:0 ch=1")  # STATUS 寄存器, bit 1
    field(SCAN, "I/O Intr")         # 中断驱动
    field(ZNAM, "NotReady")
    field(ONAM, "Ready")
}
```

#### 3.4.2 控制命令序列

**复杂的控制流程**:

```c
// 示例：启动数据采集的完整流程
int start_acquisition(void)
{
    // 1. 检查状态
    uint32_t status = reg[STATUS_REG / 4];
    if (status & STATUS_BUSY) {
        printf("Error: Already busy!\n");
        return -1;
    }
    
    // 2. 复位 FIFO
    reg[CTRL_REG / 4] = CTRL_FIFO_RESET;
    usleep(100);  // 等待复位完成
    
    // 3. 配置采集参数
    reg[CONFIG_REG / 4] = (MODE_TRIGGERED << 4) | CHANNEL_SELECT_ALL;
    
    // 4. 使能中断
    reg[INT_ENABLE_REG / 4] = INT_DATA_READY | INT_ERROR;
    
    // 5. 启动采集
    reg[CTRL_REG / 4] = CTRL_START;
    
    printf("Acquisition started\n");
    return 0;
}

// 在 Device Support 中封装为 BO record
static long write_bo(boRecord *record)
{
    recordpara_t *priv = record->dpvt;
    
    if (priv->offset == START_ACQ_CMD) {
        if (record->val == 1) {
            int ret = start_acquisition();
            if (ret < 0) {
                recGblSetSevr(record, WRITE_ALARM, MAJOR_ALARM);
                return -1;
            }
        } else {
            stop_acquisition();
        }
    }
    
    return 0;
}
```

**数据库定义**:

```
record(bo, "$(P):StartAcq")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@CMD:100")  # 命令 offset
    field(ZNAM, "Stop")
    field(ONAM, "Start")
}
```

---

## 第三部分总结

作为 IOC 开发者，你需要：

1. **获取完整的硬件文档**:
   - 寄存器地址映射表
   - 位域定义
   - 时序要求
   - 握手协议
   - 中断机制
   - DMA 描述符格式

2. **验证硬件接口**:
   - 编写独立测试程序
   - 验证寄存器读写
   - 回环测试
   - 时序测量

3. **设计 IOC 映射**:
   - 选择合适的映射方案（offset / 直接地址 / 符号名）
   - 解析状态寄存器
   - 封装控制命令序列

4. **与 FPGA 团队协作**:
   - 明确接口规范
   - 及时沟通问题
   - 共同测试验证

---

## 第四部分：从零搭建完整指南

本节提供从零开始搭建 EPICS IOC 与 FPGA 集成系统的完整步骤。

### 4.1 从 FPGA 团队获取资料并验证

#### 4.1.1 需求清单

在开始开发前，向 FPGA 团队索取以下资料：

**必需文档**:
- [ ] 寄存器地址映射表 (Excel/PDF)
- [ ] 固件版本号和更新日志
- [ ] 硬件接口类型（PCIe/Zynq/Etherbone 等）
- [ ] 物理地址范围（基地址 + 大小）
- [ ] 数据格式说明（定点/浮点、单位、量程）
- [ ] 时序要求文档

**可选但推荐**:
- [ ] FPGA 设计框图
- [ ] 测试程序或示例代码
- [ ] Vivado/Quartus 工程文件（用于查看地址分配）
- [ ] 仿真波形文件

**验证资料**:
- [ ] 设备树文件 (Zynq)
- [ ] UIO 设备号分配
- [ ] 中断号分配

#### 4.1.2 验证步骤

**步骤 1: 检查硬件连接**

```bash
# Zynq 平台
# 检查 UIO 设备
$ ls -l /dev/uio*
crw-rw---- 1 root root 247, 0 Nov 18 10:00 /dev/uio0
crw-rw---- 1 root root 247, 1 Nov 18 10:00 /dev/uio1

# 检查 UIO 设备信息
$ cat /sys/class/uio/uio0/name
bpm_regs

$ cat /sys/class/uio/uio0/maps/map0/addr
0x43c00000

$ cat /sys/class/uio/uio0/maps/map0/size
0x10000

# PCIe 平台
# 检查 PCIe 设备
$ lspci | grep -i fpga
01:00.0 Memory controller: Xilinx Corporation Device 7024

$ lspci -v -s 01:00.0
01:00.0 Memory controller: Xilinx Corporation Device 7024
        Subsystem: Xilinx Corporation Device 0007
        Flags: bus master, fast devsel, latency 0, IRQ 16
        Memory at f0000000 (64-bit, prefetchable) [size=256M]
        Memory at e0000000 (64-bit, prefetchable) [size=256M]
        Capabilities: [40] Power Management version 3
        Capabilities: [48] MSI: Enable+ Count=1/4 Maskable- 64bit+
        Capabilities: [60] Express Endpoint, MSI 00
        Kernel driver in use: xdma
```

**步骤 2: 运行硬件测试程序**

使用前面 3.2.1 节的 `test_fpga.c` 程序：

```bash
$ gcc -o test_fpga test_fpga.c
$ sudo ./test_fpga

# 预期所有测试都通过
# 如果失败，记录错误信息并反馈给 FPGA 团队
```

**步骤 3: 验证寄存器映射**

创建寄存器验证表：

```python
#!/usr/bin/env python3
# verify_reg_map.py

import mmap
import struct

# 从 FPGA 文档中获取的寄存器表
REG_MAP = {
    'VERSION':    {'addr': 0x0008, 'rw': 'R',   'expected': 0x00020000},  # v2.0
    'STATUS':     {'addr': 0x0004, 'rw': 'R',   'expected': None},
    'DO':         {'addr': 0x0100, 'rw': 'R/W', 'expected': None},
    'CH0_AMP':    {'addr': 0x0010, 'rw': 'R',   'expected': None},
}

def verify_registers():
    with open("/dev/uio0", "r+b") as f:
        mm = mmap.mmap(f.fileno(), 0x10000)
        
        print("Register Verification Report")
        print("=" * 60)
        
        for name, info in REG_MAP.items():
            mm.seek(info['addr'])
            value = struct.unpack('I', mm.read(4))[0]
            
            status = "OK"
            if info['expected'] is not None:
                if value != info['expected']:
                    status = f"FAIL (expected 0x{info['expected']:08X})"
            
            print(f"{name:12s} @ 0x{info['addr']:04X}: 0x{value:08X} [{status}]")
        
        mm.close()

if __name__ == "__main__":
    verify_registers()
```

**步骤 4: 数据一致性检查**

```c
// 验证数据读取的一致性
void test_data_consistency(volatile uint32_t *reg)
{
    printf("Data Consistency Test:\n");
    
    for (int ch = 0; ch < 8; ch++) {
        uint32_t readings[100];
        
        // 快速连续读取 100 次
        for (int i = 0; i < 100; i++) {
            readings[i] = reg[(CH0_AMP + ch * 8) / 4];
        }
        
        // 计算标准差
        double mean = 0;
        for (int i = 0; i < 100; i++) {
            mean += readings[i];
        }
        mean /= 100;
        
        double variance = 0;
        for (int i = 0; i < 100; i++) {
            variance += (readings[i] - mean) * (readings[i] - mean);
        }
        variance /= 100;
        double stddev = sqrt(variance);
        
        printf("  CH%d: mean=%.0f, stddev=%.2f\n", ch, mean, stddev);
        
        // 如果标准差太大，可能有问题
        if (stddev > mean * 0.1) {  // 10% 阈值
            printf("    [WARNING] High variation!\n");
        }
    }
}
```

### 4.2 设计 Device Support 和 Driver Support

#### 4.2.1 项目目录结构

创建标准的 EPICS IOC 项目结构：

```bash
$ mkdir -p MyBPMIOC
$ cd MyBPMIOC

# 使用 EPICS makeBaseApp 创建模板
$ makeBaseApp.pl -t ioc MyBPM
$ makeBaseApp.pl -i -t ioc MyBPM
Using target architecture linux-arm.
The following applications are available:
    MyBPM
What application should the IOC(s) boot?
The default uses the IOC's name, even if not listed above.
Application name? MyBPM

# 生成的目录结构
MyBPMIOC/
├── configure/
│   ├── CONFIG
│   ├── CONFIG_SITE
│   ├── Makefile
│   └── RELEASE
├── MyBPMApp/
│   ├── Db/
│   │   └── Makefile
│   └── src/
│       ├── Makefile
│       └── MyBPMMain.cpp
├── iocBoot/
│   └── iocMyBPM/
│       ├── Makefile
│       └── st.cmd
└── Makefile
```

**手动创建设备支持文件**:

```bash
$ cd MyBPMApp/src/
$ touch devMyBPM.h devMyBPM.c driverWrapper.h driverWrapper.c devMyBPM.dbd
```

最终结构：

```
MyBPMIOC/
├── MyBPMApp/
│   ├── Db/
│   │   ├── MyBPM.db           # PV 定义
│   │   ├── Makefile
│   │   └── MyBPM.substitutions
│   └── src/
│       ├── devMyBPM.h         # Device Support 头文件
│       ├── devMyBPM.c         # Device Support 实现
│       ├── driverWrapper.h    # Driver Support 头文件
│       ├── driverWrapper.c    # Driver Support 实现
│       ├── devMyBPM.dbd       # DBD 定义
│       ├── MyBPMMain.cpp      # IOC 主程序
│       └── Makefile
├── iocBoot/
│   └── iocMyBPM/
│       ├── st.cmd             # IOC 启动脚本
│       └── envPaths
└── configure/
    └── RELEASE                # EPICS Base 路径
```

#### 4.2.2 编写 DBD 文件

**MyBPMApp/src/devMyBPM.dbd**:

```dbd
# Device Support 定义
device(ai,         INST_IO, devAi,         "MyBPM")
device(ao,         INST_IO, devAo,         "MyBPM")
device(bi,         INST_IO, devBi,         "MyBPM")
device(bo,         INST_IO, devBo,         "MyBPM")
device(longin,     INST_IO, devLi,         "MyBPM")
device(longout,    INST_IO, devLo,         "MyBPM")
device(waveform,   INST_IO, devWaveform,   "MyBPM")

# Driver Support 定义
driver(drMyBPM)

# 如果需要注册函数供 IOC shell 使用
registrar(MyBPMRegistrar)
```

#### 4.2.3 编写 Device Support 头文件

**MyBPMApp/src/devMyBPM.h**:

```c
#ifndef _devMyBPM_H
#define _devMyBPM_H

#include <dbScan.h>

/* 数据类型枚举 */
typedef enum {
    TYPE_NONE = 0,
    TYPE_REG,      /* 通用寄存器 */
    TYPE_AMP,      /* 幅度 */
    TYPE_PHASE,    /* 相位 */
    TYPE_VOLTAGE,  /* 电压 */
    TYPE_POSITION, /* 位置 */
    TYPE_STATUS    /* 状态 */
} data_type_t;

/* Record 私有数据结构 */
typedef struct {
    data_type_t type;        /* 数据类型 */
    unsigned short offset;   /* 寄存器偏移 */
    unsigned short channel;  /* 通道号 */
    float scale;             /* 缩放因子 */
    float offset_val;        /* 偏移值 */
} record_priv_t;

/* I/O Intr 支持函数声明 */
static long devGetIoIntInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt);

/* INP/OUT 字段解析函数 */
static int devIoParse(char *string, record_priv_t *priv);

#endif /* _devMyBPM_H */
```

#### 4.2.4 编写 Device Support 实现

**MyBPMApp/src/devMyBPM.c** (简化版本):

```c
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <cantProceed.h>
#include <epicsExport.h>

#include <aiRecord.h>
#include <aoRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <waveformRecord.h>

#include "devMyBPM.h"
#include "driverWrapper.h"

/* ============================================================
 * AI Record Support
 * ============================================================ */
static long init_record_ai(aiRecord *record);
static long read_ai(aiRecord *record);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devAi = {
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,  /* 如果需要 I/O Intr，改为 devGetIoIntInfo */
    read_ai,
    NULL
};
epicsExportAddress(dset, devAi);

static long init_record_ai(aiRecord *record)
{
    record_priv_t *priv;
    
    /* 分配私有数据 */
    priv = (record_priv_t *)callocMustSucceed(1, sizeof(record_priv_t),
                                               "init_record_ai");
    
    /* 解析 INP 字段 */
    if (devIoParse(record->inp.value.instio.string, priv) < 0) {
        printf("Error parsing INP field: %s\n", 
               record->inp.value.instio.string);
        free(priv);
        return -1;
    }
    
    /* 保存私有数据 */
    record->dpvt = priv;
    
    return 0;
}

static long read_ai(aiRecord *record)
{
    record_priv_t *priv = (record_priv_t *)record->dpvt;
    float value;
    
    /* 调用驱动层读取函数 */
    value = ReadData(priv->offset, priv->channel, priv->type);
    
    /* 应用缩放和偏移 */
    record->val = value * priv->scale + priv->offset_val;
    record->udf = FALSE;
    
    return 2;  /* 不进行线性转换 */
}

/* ============================================================
 * AO Record Support
 * ============================================================ */
static long init_record_ao(aoRecord *record);
static long write_ao(aoRecord *record);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
    DEVSUPFUN special_linconv;
} devAo = {
    6,
    NULL,
    NULL,
    init_record_ao,
    NULL,
    write_ao,
    NULL
};
epicsExportAddress(dset, devAo);

static long init_record_ao(aoRecord *record)
{
    record_priv_t *priv;
    
    priv = (record_priv_t *)callocMustSucceed(1, sizeof(record_priv_t),
                                               "init_record_ao");
    
    if (devIoParse(record->out.value.instio.string, priv) < 0) {
        printf("Error parsing OUT field: %s\n",
               record->out.value.instio.string);
        free(priv);
        return -1;
    }
    
    record->dpvt = priv;
    
    return 2;  /* 保留 VAL 字段的值 */
}

static long write_ao(aoRecord *record)
{
    record_priv_t *priv = (record_priv_t *)record->dpvt;
    
    /* 应用反向缩放和偏移 */
    float hw_value = (record->val - priv->offset_val) / priv->scale;
    
    /* 调用驱动层写入函数 */
    WriteData(priv->offset, priv->channel, hw_value);
    
    return 0;
}

/* ============================================================
 * Waveform Record Support (简化版本)
 * ============================================================ */
static long init_record_wf(waveformRecord *record);
static long read_wf(waveformRecord *record);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devWaveform = {
    6,
    NULL,
    NULL,
    init_record_wf,
    devGetIoIntInfo,  /* I/O Intr 支持 */
    read_wf,
    NULL
};
epicsExportAddress(dset, devWaveform);

static long init_record_wf(waveformRecord *record)
{
    record_priv_t *priv;
    
    priv = (record_priv_t *)callocMustSucceed(1, sizeof(record_priv_t),
                                               "init_record_wf");
    
    if (devIoParse(record->inp.value.instio.string, priv) < 0) {
        printf("Error parsing INP field: %s\n",
               record->inp.value.instio.string);
        free(priv);
        return -1;
    }
    
    record->dpvt = priv;
    
    return 0;
}

static long read_wf(waveformRecord *record)
{
    record_priv_t *priv = (record_priv_t *)record->dpvt;
    
    /* 调用驱动层读取波形 */
    ReadWaveform(priv->offset, priv->channel, record->nelm, record->bptr);
    
    record->nord = record->nelm;
    
    return 0;
}

/* ============================================================
 * I/O Intr Support
 * ============================================================ */
static long devGetIoIntInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt)
{
    *ppvt = GetIoIntScanPvt();
    return 0;
}

/* ============================================================
 * INP/OUT 字段解析
 * ============================================================ */
static int devIoParse(char *string, record_priv_t *priv)
{
    /*
     * 支持的格式:
     *   @TYPE:offset ch=channel scale=scale offset=offset
     * 
     * 示例:
     *   @AMP:0 ch=2
     *   @REG:10 ch=0 scale=1.0 offset=0.0
     *   @VOLTAGE:5 ch=1 scale=0.001
     */
    
    char typeName[32] = "";
    char *pchar = string;
    
    /* 跳过 '@' */
    if (*pchar == '@') pchar++;
    
    /* 解析类型名 */
    int nchar = strcspn(pchar, ":");
    strncpy(typeName, pchar, nchar);
    typeName[nchar] = '\0';
    pchar += nchar + 1;
    
    /* 确定类型 */
    if (strcmp(typeName, "REG") == 0)
        priv->type = TYPE_REG;
    else if (strcmp(typeName, "AMP") == 0)
        priv->type = TYPE_AMP;
    else if (strcmp(typeName, "PHASE") == 0)
        priv->type = TYPE_PHASE;
    else if (strcmp(typeName, "VOLTAGE") == 0)
        priv->type = TYPE_VOLTAGE;
    else if (strcmp(typeName, "POSITION") == 0)
        priv->type = TYPE_POSITION;
    else if (strcmp(typeName, "STATUS") == 0)
        priv->type = TYPE_STATUS;
    else {
        printf("Unknown type: %s\n", typeName);
        return -1;
    }
    
    /* 解析 offset */
    priv->offset = strtol(pchar, &pchar, 0);
    
    /* 默认值 */
    priv->channel = 0;
    priv->scale = 1.0;
    priv->offset_val = 0.0;
    
    /* 解析可选参数 */
    while (*pchar != '\0') {
        /* 跳过空格 */
        while (*pchar == ' ' || *pchar == '\t') pchar++;
        
        if (strncmp(pchar, "ch=", 3) == 0) {
            pchar += 3;
            priv->channel = strtol(pchar, &pchar, 0);
        } else if (strncmp(pchar, "scale=", 6) == 0) {
            pchar += 6;
            priv->scale = strtof(pchar, &pchar);
        } else if (strncmp(pchar, "offset=", 7) == 0) {
            pchar += 7;
            priv->offset_val = strtof(pchar, &pchar);
        } else {
            pchar++;
        }
    }
    
    return 0;
}
```

#### 4.2.5 编写 Driver Support 头文件

**MyBPMApp/src/driverWrapper.h**:

```c
#ifndef _driverWrapper_H
#define _driverWrapper_H

#include <dbScan.h>

/* 驱动层接口函数（供 Device Support 调用） */

/* 读取数据 */
float ReadData(int offset, int channel, int type);

/* 写入数据 */
void WriteData(int offset, int channel, float value);

/* 读取波形 */
void ReadWaveform(int offset, int channel, unsigned int nelem, float *data);

/* 获取 I/O Intr 扫描句柄 */
IOSCANPVT GetIoIntScanPvt(void);

#endif /* _driverWrapper_H */
```

#### 4.2.6 编写 Driver Support 实现

**MyBPMApp/src/driverWrapper.c** (简化版本):

```c
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>

#include <drvSup.h>
#include <epicsExport.h>
#include <dbScan.h>

#include "driverWrapper.h"

/* 硬件访问库文件名 */
#define HW_LIB_NAME "liblowlevel.so"

/* I/O Intr 扫描句柄 */
static IOSCANPVT IoIntScanPvt;

/* 硬件函数指针 */
static int (*hwInit)(void);
static uint32_t (*hwReadReg)(uint32_t addr);
static void (*hwWriteReg)(uint32_t addr, uint32_t value);
static void (*hwReadArray)(uint32_t addr, int ch, float *data, int len);

/* 驱动初始化函数 */
static long InitDriver(void);

/* Driver Support Entry Table */
struct {
    long number;
    DRVSUPFUN report;
    DRVSUPFUN init;
} drMyBPM = {
    2,
    NULL,
    InitDriver
};
epicsExportAddress(drvet, drMyBPM);

/* 后台线程 */
static void *background_thread(void *arg);

/* ============================================================
 * 驱动初始化
 * ============================================================ */
static long InitDriver(void)
{
    void *handle;
    
    printf("=== MyBPM Driver Initializing ===\n");
    
    /* 1. 加载硬件库 */
    printf("Loading hardware library: %s\n", HW_LIB_NAME);
    handle = dlopen(HW_LIB_NAME, RTLD_NOW);
    if (handle == NULL) {
        fprintf(stderr, "Failed to load %s: %s\n", HW_LIB_NAME, dlerror());
        return -1;
    }
    
    /* 2. 映射硬件函数 */
    hwInit = dlsym(handle, "HardwareInit");
    hwReadReg = dlsym(handle, "ReadRegister");
    hwWriteReg = dlsym(handle, "WriteRegister");
    hwReadArray = dlsym(handle, "ReadArray");
    
    if (!hwInit || !hwReadReg || !hwWriteReg || !hwReadArray) {
        fprintf(stderr, "Failed to resolve hardware functions\n");
        dlclose(handle);
        return -1;
    }
    
    /* 3. 初始化硬件 */
    printf("Initializing hardware...\n");
    if (hwInit() < 0) {
        fprintf(stderr, "Hardware initialization failed\n");
        return -1;
    }
    printf("Hardware initialized successfully\n");
    
    /* 4. 初始化 I/O Intr 扫描 */
    scanIoInit(&IoIntScanPvt);
    
    /* 5. 创建后台线程（用于 I/O Intr） */
    pthread_t tid;
    if (pthread_create(&tid, NULL, background_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create background thread\n");
        return -1;
    }
    
    printf("=== MyBPM Driver Initialized ===\n");
    return 0;
}

/* ============================================================
 * 数据读取函数
 * ============================================================ */
float ReadData(int offset, int channel, int type)
{
    uint32_t raw_value;
    float value = 0.0;
    
    switch (offset) {
        case 0:  /* RF 幅度/相位 */
            if (type == TYPE_AMP) {
                raw_value = hwReadReg(0x0010 + channel * 8);
                /* 转换: Q20.12 定点 -> 浮点 */
                value = (float)((int32_t)raw_value) / (1 << 20);
            } else if (type == TYPE_PHASE) {
                raw_value = hwReadReg(0x0014 + channel * 8);
                /* 转换: 16-bit -> 360 度 */
                value = (float)raw_value * 360.0 / 65536.0;
            }
            break;
            
        case 5:  /* 电压值 */
            raw_value = hwReadReg(0x0200 + channel * 4);
            value = (float)((int32_t)raw_value);
            break;
            
        case 10:  /* XY 位置 */
            raw_value = hwReadReg(0x0400 + channel * 4);
            /* 转换: um 单位 */
            value = (float)((int32_t)raw_value) / 1000.0;
            break;
            
        default:
            printf("Unknown offset: %d\n", offset);
            break;
    }
    
    return value;
}

/* ============================================================
 * 数据写入函数
 * ============================================================ */
void WriteData(int offset, int channel, float value)
{
    uint32_t raw_value;
    
    switch (offset) {
        case 0:  /* 数字输出 */
            raw_value = (uint32_t)value;
            hwWriteReg(0x0100 + channel * 4, raw_value);
            break;
            
        case 20:  /* BPM K1 系数 */
            /* 浮点 -> Q15 定点 */
            raw_value = (uint32_t)((int32_t)(value * 32767));
            hwWriteReg(0x0300 + channel * 4, raw_value);
            printf("Set BPM K1[%d] = %.3f (0x%08X)\n", 
                   channel, value, raw_value);
            break;
            
        default:
            printf("Unknown offset: %d\n", offset);
            break;
    }
}

/* ============================================================
 * 波形读取函数
 * ============================================================ */
void ReadWaveform(int offset, int channel, unsigned int nelem, float *data)
{
    /* 调用硬件库读取数组 */
    hwReadArray(offset, channel, data, nelem);
}

/* ============================================================
 * I/O Intr 支持
 * ============================================================ */
IOSCANPVT GetIoIntScanPvt(void)
{
    return IoIntScanPvt;
}

/* 后台线程：轮询硬件状态并触发 I/O Intr */
static void *background_thread(void *arg)
{
    printf("Background thread started\n");
    
    while (1) {
        /* 检查硬件是否有新数据 */
        uint32_t status = hwReadReg(0x0004);  /* STATUS_REG */
        
        if (status & 0x02) {  /* DATA_READY bit */
            /* 触发 I/O Intr 扫描 */
            scanIoRequest(IoIntScanPvt);
        }
        
        usleep(100000);  /* 100ms */
    }
    
    return NULL;
}
```

### 4.3 PV 定义与寄存器映射

#### 4.3.1 编写数据库文件

**MyBPMApp/Db/MyBPM.db**:

```
# ============================================================
# 版本和状态
# ============================================================
record(longin, "$(P):Version")
{
    field(DTYP, "MyBPM")
    field(INP,  "@REG:8 ch=0")
    field(SCAN, "I/O Intr")
    field(DESC, "Firmware version")
}

record(bi, "$(P):Status:Busy")
{
    field(DTYP, "MyBPM")
    field(INP,  "@STATUS:4 ch=0")
    field(SCAN, ".1 second")
    field(ZNAM, "Idle")
    field(ONAM, "Busy")
}

record(bi, "$(P):Status:DataReady")
{
    field(DTYP, "MyBPM")
    field(INP,  "@STATUS:4 ch=1")
    field(SCAN, "I/O Intr")
    field(ZNAM, "Not Ready")
    field(ONAM, "Ready")
}

# ============================================================
# RF 幅度和相位（多通道）
# ============================================================
record(ai, "$(P):CH$(CH):Amp")
{
    field(DTYP, "MyBPM")
    field(INP,  "@AMP:0 ch=$(CH)")
    field(SCAN, ".5 second")
    field(EGU,  "V")
    field(PREC, "4")
    field(DESC, "Channel $(CH) amplitude")
}

record(ai, "$(P):CH$(CH):Phase")
{
    field(DTYP, "MyBPM")
    field(INP,  "@PHASE:0 ch=$(CH)")
    field(SCAN, ".5 second")
    field(EGU,  "deg")
    field(PREC, "2")
    field(DESC, "Channel $(CH) phase")
}

# ============================================================
# BPM 电压值
# ============================================================
record(ai, "$(P):BPM$(BPM):Voltage$(CH)")
{
    field(DTYP, "MyBPM")
    field(INP,  "@VOLTAGE:5 ch=$(CH)")
    field(SCAN, ".1 second")
    field(EGU,  "mV")
    field(PREC, "3")
    field(DESC, "BPM$(BPM) voltage channel $(CH)")
}

# ============================================================
# BPM 位置
# ============================================================
record(ai, "$(P):BPM$(BPM):PosX")
{
    field(DTYP, "MyBPM")
    field(INP,  "@POSITION:10 ch=0")
    field(SCAN, ".1 second")
    field(EGU,  "mm")
    field(PREC, "3")
    field(DESC, "BPM$(BPM) X position")
}

record(ai, "$(P):BPM$(BPM):PosY")
{
    field(DTYP, "MyBPM")
    field(INP,  "@POSITION:10 ch=1")
    field(SCAN, ".1 second")
    field(EGU,  "mm")
    field(PREC, "3")
    field(DESC, "BPM$(BPM) Y position")
}

# ============================================================
# 控制参数（BPM K1 系数）
# ============================================================
record(ao, "$(P):BPM$(BPM):K1:CH$(CH):Set")
{
    field(DTYP, "MyBPM")
    field(OUT,  "@REG:20 ch=$(CH)")
    field(SCAN, "Passive")
    field(PREC, "6")
    field(DESC, "BPM K1 coefficient")
    field(VAL,  "1.0")
    field(DRVH, "2.0")
    field(DRVL, "-2.0")
}

record(ai, "$(P):BPM$(BPM):K1:CH$(CH):RBV")
{
    field(DTYP, "MyBPM")
    field(INP,  "@REG:20 ch=$(CH)")
    field(SCAN, "1 second")
    field(PREC, "6")
    field(DESC, "BPM K1 readback")
}

# ============================================================
# 波形记录
# ============================================================
record(waveform, "$(P):CH$(CH):AmpWave")
{
    field(DTYP, "MyBPM")
    field(INP,  "@AMP:0 ch=$(CH)")
    field(SCAN, "I/O Intr")
    field(NELM, "10000")
    field(FTVL, "FLOAT")
    field(EGU,  "V")
    field(DESC, "Channel $(CH) amplitude waveform")
}

record(waveform, "$(P):CH$(CH):PhaseWave")
{
    field(DTYP, "MyBPM")
    field(INP,  "@PHASE:0 ch=$(CH)")
    field(SCAN, "I/O Intr")
    field(NELM, "10000")
    field(FTVL, "FLOAT")
    field(EGU,  "deg")
    field(DESC, "Channel $(CH) phase waveform")
}

# ============================================================
# 数字 I/O
# ============================================================
record(bo, "$(P):DO$(CH):Set")
{
    field(DTYP, "MyBPM")
    field(OUT,  "@REG:0 ch=$(CH)")
    field(SCAN, "Passive")
    field(ZNAM, "Off")
    field(ONAM, "On")
}

record(bi, "$(P):DI$(CH):RBV")
{
    field(DTYP, "MyBPM")
    field(INP,  "@REG:1 ch=$(CH)")
    field(SCAN, ".1 second")
    field(ZNAM, "Low")
    field(ONAM, "High")
}
```

#### 4.3.2 使用 Substitution 文件

**MyBPMApp/Db/MyBPM.substitutions**:

```
# 为多个通道生成 PV
file "MyBPM.db"
{
    pattern { P,         CH }
            { "BPM1",    "0"  }
            { "BPM1",    "1"  }
            { "BPM1",    "2"  }
            { "BPM1",    "3"  }
            { "BPM1",    "4"  }
            { "BPM1",    "5"  }
            { "BPM1",    "6"  }
            { "BPM1",    "7"  }
}

# 生成 BPM 位置 PV
file "MyBPM.db"
{
    pattern { P,         BPM }
            { "Sector1", "1"  }
            { "Sector1", "2"  }
}
```

生成后的 PV 列表示例：
```
BPM1:CH0:Amp
BPM1:CH0:Phase
BPM1:CH0:AmpWave
BPM1:CH0:PhaseWave
BPM1:CH1:Amp
BPM1:CH1:Phase
...
Sector1:BPM1:PosX
Sector1:BPM1:PosY
Sector1:BPM2:PosX
Sector1:BPM2:PosY
```

---

*(第四部分继续...)*

### 4.4 Makefile 配置

#### 4.4.1 src/Makefile

**MyBPMApp/src/Makefile**:

```makefile
TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = MyBPM
# MyBPM.dbd will be created and installed
DBD += MyBPM.dbd

# MyBPM.dbd will be made up from these files:
MyBPM_DBD += base.dbd
MyBPM_DBD += devMyBPM.dbd

# Include dbd files from all support applications:
#MyBPM_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#MyBPM_LIBS += xxx

# MyBPM_registerRecordDeviceDriver.cpp derives from MyBPM.dbd
MyBPM_SRCS += MyBPM_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
MyBPM_SRCS_DEFAULT += MyBPMMain.cpp
MyBPM_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#MyBPM_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
MyBPM_LIBS += $(EPICS_BASE_IOC_LIBS)

# 添加 Device Support 和 Driver Support 源文件
MyBPM_SRCS += devMyBPM.c
MyBPM_SRCS += driverWrapper.c

# 链接数学库和动态加载库
MyBPM_SYS_LIBS_Linux += m
MyBPM_SYS_LIBS_Linux += dl
MyBPM_SYS_LIBS_Linux += pthread

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

```

#### 4.4.2 Db/Makefile

**MyBPMApp/Db/Makefile**:

```makefile
TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
# Create and install (or just install) into <top>/db
# databases, templates, substitutions like this
DB += MyBPM.db
DB += MyBPM.substitutions

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_template = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
```

#### 4.4.3 configure/RELEASE

**configure/RELEASE**:

```makefile
# RELEASE - Location of external support modules
#
# EPICS_BASE usually appears last so other apps can override stuff:

# EPICS Base 路径（根据实际安装路径修改）
EPICS_BASE=/opt/epics/base

# 如果使用其他支持模块，在此添加
# ASYN=/opt/epics/modules/asyn
# AUTOSAVE=/opt/epics/modules/autosave
```

### 4.5 IOC 启动脚本

#### 4.5.1 st.cmd

**iocBoot/iocMyBPM/st.cmd**:

```bash
#!../../bin/linux-arm/MyBPM

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/MyBPM.dbd"
MyBPM_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate "db/MyBPM.substitutions"
# 或者直接加载单个数据库
# dbLoadRecords("db/MyBPM.db","P=BPM1,CH=0")

## Set IOC name (用于 CA server)
epicsEnvSet("EPICS_IOC_LOG_INET", "127.0.0.1")

## 启动 IOC
cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=root"
```

#### 4.5.2 envPaths

**iocBoot/iocMyBPM/envPaths** (自动生成):

```bash
epicsEnvSet("IOC","iocMyBPM")
epicsEnvSet("TOP","/home/user/MyBPMIOC")
epicsEnvSet("EPICS_BASE","/opt/epics/base")
```

### 4.6 编译 IOC

#### 4.6.1 编译步骤

```bash
$ cd /home/user/MyBPMIOC

# 清理之前的编译
$ make clean

# 编译整个项目
$ make

# 预期输出
make -C ./configure install
make[1]: Entering directory '/home/user/MyBPMIOC/configure'
...
make[1]: Leaving directory '/home/user/MyBPMIOC/configure'
make -C ./MyBPMApp install
make[1]: Entering directory '/home/user/MyBPMIOC/MyBPMApp'
make -C ./src install
make[2]: Entering directory '/home/user/MyBPMIOC/MyBPMApp/src'
...
Installing dbd file ../../dbd/MyBPM.dbd
Installing dbd file ../../dbd/devMyBPM.dbd
...
Installing created library ../../lib/linux-arm/libMyBPM.so
Installing executable ../../bin/linux-arm/MyBPM
make[2]: Leaving directory '/home/user/MyBPMIOC/MyBPMApp/src'
make -C ./Db install
make[2]: Entering directory '/home/user/MyBPMIOC/MyBPMApp/Db'
Installing db file ../../db/MyBPM.db
Installing db file ../../db/MyBPM.substitutions
make[2]: Leaving directory '/home/user/MyBPMIOC/MyBPMApp/Db'
make[1]: Leaving directory '/home/user/MyBPMIOC/MyBPMApp'
make -C ./iocBoot install
...
```

#### 4.6.2 检查编译结果

```bash
$ ls -l bin/linux-arm/
total 123
-rwxr-xr-x 1 user user 125896 Nov 18 10:30 MyBPM

$ ls -l lib/linux-arm/
total 45
-rwxr-xr-x 1 user user 45678 Nov 18 10:30 libMyBPM.so

$ ls -l dbd/
total 8
-rw-r--r-- 1 user user 1234 Nov 18 10:30 MyBPM.dbd
-rw-r--r-- 1 user user  567 Nov 18 10:30 devMyBPM.dbd

$ ls -l db/
total 12
-rw-r--r-- 1 user user 5678 Nov 18 10:30 MyBPM.db
-rw-r--r-- 1 user user  234 Nov 18 10:30 MyBPM.substitutions
```

### 4.7 调试与验证

#### 4.7.1 启动 IOC

```bash
$ cd iocBoot/iocMyBPM
$ chmod +x st.cmd
$ sudo ./st.cmd

# 预期输出
#!../../bin/linux-arm/MyBPM
< envPaths
cd "/home/user/MyBPMIOC"
dbLoadDatabase "dbd/MyBPM.dbd"
MyBPM_registerRecordDeviceDriver pdbbase

=== MyBPM Driver Initializing ===
Loading hardware library: liblowlevel.so
Initializing hardware...
Hardware initialized successfully
Background thread started
=== MyBPM Driver Initialized ===

dbLoadTemplate "db/MyBPM.substitutions"
cd "/home/user/MyBPMIOC/iocBoot/iocMyBPM"
iocInit
Starting iocInit
############################################################################
## EPICS R7.0.3.1
## Rev. 2021-04-15
############################################################################
iocRun: All initialization complete

# IOC Shell 提示符
epics>
```

#### 4.7.2 测试 PV 访问

**在 IOC Shell 中测试**:

```bash
epics> dbl
BPM1:CH0:Amp
BPM1:CH0:Phase
BPM1:CH0:AmpWave
BPM1:CH0:PhaseWave
BPM1:Status:Busy
BPM1:Status:DataReady
BPM1:Version
... (更多 PV)

epics> dbpr BPM1:CH0:Amp
Record: BPM1:CH0:Amp
  DTYP: MyBPM
  INP:  INST_IO @AMP:0 ch=0
  SCAN: .5 second
  VAL:  0.12345
  ...

epics> dbgf BPM1:CH0:Amp
DBR_DOUBLE:         0.12345

epics> dbpf BPM1:BPM1:K1:CH0:Set 1.234
DBR_DOUBLE:         1.234
```

**使用 CA 客户端测试**:

```bash
# 在另一个终端

# 1. 测试 caget
$ caget BPM1:CH0:Amp
BPM1:CH0:Amp               0.12345

$ caget BPM1:CH0:Phase
BPM1:CH0:Phase             45.67

# 2. 测试 caput
$ caput BPM1:BPM1:K1:CH0:Set 1.5
Old : BPM1:BPM1:K1:CH0:Set   1.234
New : BPM1:BPM1:K1:CH0:Set   1.5

# 3. 测试 camonitor（监控值的变化）
$ camonitor BPM1:CH0:Amp
BPM1:CH0:Amp               2024-11-18 10:35:12.123456 0.12345
BPM1:CH0:Amp               2024-11-18 10:35:12.623456 0.12347
BPM1:CH0:Amp               2024-11-18 10:35:13.123456 0.12349
^C

# 4. 测试波形数据
$ caget -a BPM1:CH0:AmpWave
BPM1:CH0:AmpWave 10000 0.1 0.11 0.12 0.13 ... (10000 个值)

# 5. 测试 I/O Intr
$ camonitor BPM1:Status:DataReady
BPM1:Status:DataReady      2024-11-18 10:36:00.000000 Not Ready
BPM1:Status:DataReady      2024-11-18 10:36:05.123456 Ready  # 触发
BPM1:Status:DataReady      2024-11-18 10:36:10.456789 Not Ready
```

#### 4.7.3 调试工具

**1. IOC Shell 调试命令**:

```bash
# 列出所有 PV
epics> dbl

# 列出特定模式的 PV
epics> dbl "BPM1:CH*"

# 显示 record 详细信息
epics> dbpr BPM1:CH0:Amp 2

# 设置/获取字段值
epics> dbgf BPM1:CH0:Amp.VAL
epics> dbpf BPM1:CH0:Amp.SCAN "1 second"

# 显示 I/O Intr 信息
epics> scanppl

# 显示驱动信息
epics> dbior
```

**2. 日志输出**:

在驱动代码中添加详细日志：

```c
#define DEBUG 1

#if DEBUG
#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

float ReadData(int offset, int channel, int type)
{
    LOG("offset=%d, channel=%d, type=%d", offset, channel, type);
    
    // ... 实现 ...
    
    LOG("return value=%.4f", value);
    return value;
}
```

**3. 性能分析**:

```c
#include <time.h>

void measure_performance(void)
{
    struct timespec start, end;
    int iterations = 10000;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        float val = ReadData(0, 0, TYPE_AMP);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long long ns = (end.tv_sec - start.tv_sec) * 1000000000LL +
                   (end.tv_nsec - start.tv_nsec);
    
    printf("ReadData performance: %lld ns/call\n", ns / iterations);
}
```

#### 4.7.4 常见问题排查

**问题 1: IOC 启动失败，找不到 liblowlevel.so**

```
Error: Failed to load liblowlevel.so: cannot open shared object file
```

**解决**:
```bash
# 1. 检查库文件位置
$ ls -l /usr/local/lib/liblowlevel.so

# 2. 添加到库搜索路径
$ export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# 3. 或者修改 st.cmd
epicsEnvSet("LD_LIBRARY_PATH", "/usr/local/lib:${LD_LIBRARY_PATH}")

# 4. 更新 ldconfig
$ sudo ldconfig
```

**问题 2: 读取的值总是 0**

```
epics> dbgf BPM1:CH0:Amp
DBR_DOUBLE:         0.00000
```

**排查步骤**:
```bash
# 1. 检查硬件是否正常
$ sudo ./test_fpga

# 2. 在驱动中添加日志
printf("ReadData: offset=%d, channel=%d, raw=0x%08X\n", 
       offset, channel, raw_value);

# 3. 检查单位转换是否正确
printf("Raw value: 0x%08X (%d), Converted: %.6f\n", 
       raw_value, raw_value, converted_value);

# 4. 验证 INP 字段解析
epics> dbpr BPM1:CH0:Amp
  INP:  INST_IO @AMP:0 ch=0  # 检查是否正确
```

**问题 3: I/O Intr 不触发**

```
# camonitor 没有更新
```

**排查步骤**:
```bash
# 1. 检查后台线程是否运行
epics> 
# 在驱动代码中添加:
printf("Background thread checking status...\n");

# 2. 检查 get_ioint_info 是否正确
# 在 Device Support 中添加:
static long devGetIoIntInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt)
{
    printf("devGetIoIntInfo called for %s\n", record->name);
    *ppvt = GetIoIntScanPvt();
    return 0;
}

# 3. 手动触发测试
epics> 
# 在驱动中添加 IOC Shell 命令:
static void triggerIoIntr(void)
{
    printf("Manually triggering I/O Intr\n");
    scanIoRequest(IoIntScanPvt);
}

# 注册命令
static const iocshFuncDef triggerDef = {"triggerIoIntr", 0, NULL};
static void triggerCall(const iocshArgBuf *args) { triggerIoIntr(); }
void MyBPMRegistrar(void)
{
    iocshRegister(&triggerDef, triggerCall);
}

# 测试
epics> triggerIoIntr
```

**问题 4: 编译错误**

```
error: 'DEVSUPFUN' undeclared
```

**解决**:
```c
// 检查头文件包含
#include <devSup.h>  // ← 必须包含

// 检查 DBD 文件
device(ai, INST_IO, devAi, "MyBPM")  // ← 检查语法
```

### 4.8 文档编写

#### 4.8.1 寄存器映射文档

**docs/RegisterMap.md**:

```markdown
# MyBPM FPGA Register Map

## 基本信息
- **基地址**: 0x43C00000
- **地址空间**: 64KB (0x10000)
- **固件版本**: 2.0

## 寄存器列表

| 偏移     | 名称           | 位宽 | 访问 | 说明                 |
|---------|---------------|------|------|---------------------|
| 0x0000  | CTRL_REG      | 32   | R/W  | 控制寄存器           |
| 0x0004  | STATUS_REG    | 32   | R    | 状态寄存器           |
| 0x0008  | VERSION_REG   | 32   | R    | 版本寄存器           |
| 0x0010  | CH0_AMP       | 32   | R    | 通道 0 幅度         |
| ...     | ...           | ...  | ...  | ...                 |

## CTRL_REG (0x0000)

| 位     | 名称       | 访问 | 默认值 | 说明                    |
|--------|-----------|------|--------|------------------------|
| 0      | START     | R/W  | 0      | 1: 启动采集             |
| 1      | STOP      | R/W  | 0      | 1: 停止采集             |
| 2      | RESET     | W    | 0      | 1: 复位（自清零）        |
| 7:4    | MODE      | R/W  | 0      | 采集模式                |
| 31:8   | RESERVED  | -    | 0      | 保留                   |

## 数据格式

### CH*_AMP 寄存器
- **格式**: 有符号定点数 Q20.12
- **范围**: -1.0 ~ +1.0
- **转换公式**: `amplitude = (int32_t)register_value / (1 << 20)`

### CH*_PHASE 寄存器
- **格式**: 无符号整数
- **范围**: 0 ~ 65535 对应 0 ~ 360 度
- **转换公式**: `phase_deg = register_value * 360.0 / 65536`
```

#### 4.8.2 PV 列表文档

**docs/PVList.md**:

```markdown
# MyBPM PV List

## 版本和状态

| PV 名称                  | 类型 | 访问 | 单位 | 说明           |
|-------------------------|------|------|------|----------------|
| BPM1:Version            | LI   | R    | -    | 固件版本       |
| BPM1:Status:Busy        | BI   | R    | -    | 采集忙状态     |
| BPM1:Status:DataReady   | BI   | R    | -    | 数据准备好     |

## RF 数据

| PV 名称                  | 类型 | 访问 | 单位 | 说明               |
|-------------------------|------|------|------|--------------------|
| BPM1:CH0:Amp            | AI   | R    | V    | 通道 0 幅度        |
| BPM1:CH0:Phase          | AI   | R    | deg  | 通道 0 相位        |
| BPM1:CH0:AmpWave        | WF   | R    | V    | 通道 0 幅度波形    |
| BPM1:CH0:PhaseWave      | WF   | R    | deg  | 通道 0 相位波形    |

## BPM 位置

| PV 名称                  | 类型 | 访问 | 单位 | 说明           |
|-------------------------|------|------|------|----------------|
| Sector1:BPM1:PosX       | AI   | R    | mm   | X 位置         |
| Sector1:BPM1:PosY       | AI   | R    | mm   | Y 位置         |

## 控制参数

| PV 名称                      | 类型 | 访问 | 单位 | 说明            |
|-----------------------------|------|------|------|-----------------|
| BPM1:BPM1:K1:CH0:Set        | AO   | W    | -    | K1 系数设置     |
| BPM1:BPM1:K1:CH0:RBV        | AI   | R    | -    | K1 系数回读     |
```

#### 4.8.3 用户手册

**docs/UserGuide.md**:

```markdown
# MyBPM IOC User Guide

## 1. 启动 IOC

```bash
cd /home/user/MyBPMIOC/iocBoot/iocMyBPM
sudo ./st.cmd
```

## 2. 查看数据

### 使用 caget
```bash
# 读取单个 PV
caget BPM1:CH0:Amp

# 读取多个 PV
caget BPM1:CH0:Amp BPM1:CH0:Phase

# 读取波形
caget -a BPM1:CH0:AmpWave
```

### 使用 camonitor
```bash
# 监控 PV 变化
camonitor BPM1:CH0:Amp BPM1:CH0:Phase
```

## 3. 配置参数

```bash
# 设置 BPM K1 系数
caput BPM1:BPM1:K1:CH0:Set 1.234

# 验证设置
caget BPM1:BPM1:K1:CH0:RBV
```

## 4. 故障排查

### 问题: 读取的数据全是 0
1. 检查 FPGA 是否配置正确
2. 运行硬件测试程序 `test_fpga`
3. 检查 IOC 日志输出

### 问题: IOC 启动失败
1. 检查 `liblowlevel.so` 是否存在
2. 确认 `/dev/uio0` 设备可访问
3. 检查权限（可能需要 sudo）
```

---

## 第四部分总结

从零搭建 EPICS IOC 与 FPGA 集成系统的完整流程：

1. **获取资料并验证**:
   - 向 FPGA 团队索取寄存器映射文档
   - 编写独立测试程序验证硬件
   - 确认数据格式和时序要求

2. **设计代码结构**:
   - 使用标准 EPICS IOC 目录结构
   - 分离 Device Support 和 Driver Support
   - 定义清晰的接口函数

3. **实现功能**:
   - Device Support: 解析 INP/OUT、调用驱动函数
   - Driver Support: 加载硬件库、封装寄存器访问
   - 支持 I/O Intr 事件驱动

4. **配置数据库**:
   - 定义 PV 与寄存器的映射关系
   - 使用 substitution 文件生成多通道 PV
   - 配置扫描周期和单位

5. **编译和测试**:
   - 配置 Makefile
   - 编译 IOC
   - 使用 CA 工具测试功能

6. **调试和优化**:
   - 添加日志输出
   - 性能分析
   - 常见问题排查

7. **文档编写**:
   - 寄存器映射文档
   - PV 列表
   - 用户手册

---

## 第五部分：职责边界与协作流程

明确的职责划分是软硬件协同开发成功的关键。

### 5.1 FPGA 开发者职责

#### 5.1.1 硬件设计与实现

**核心职责**:
- ✅ 实现 FPGA 逻辑功能（ADC 接口、IQ 解调、数据处理等）
- ✅ 设计寄存器接口（AXI-Lite / Wishbone Slave）
- ✅ 实现 DMA 传输逻辑（如果需要高速数据传输）
- ✅ 实现中断生成逻辑
- ✅ 时序收敛和资源优化

**输出文档**:
```
1. 寄存器地址映射表（Register Map）
   ├── 基地址
   ├── 每个寄存器的偏移、位宽、读写属性
   ├── 位域定义
   └── 数据格式（定点/浮点、量程、单位）

2. 接口时序文档
   ├── 读写时序要求
   ├── 握手协议
   └── 状态机定义

3. 中断机制文档
   ├── 中断源定义
   ├── 中断清除方式
   └── 中断优先级

4. DMA 描述符格式（如果使用 DMA）

5. 测试报告
   ├── 仿真波形
   ├── 实际测试数据
   └── 性能指标
```

**示例：寄存器地址映射表模板**

```
┌─────────────────────────────────────────────────────────────┐
│                  XXX FPGA Register Map v1.0                  │
│                  基地址: 0x43C00000                          │
├──────────┬──────────────────┬─────┬────────┬────────────────┤
│ 偏移地址  │  寄存器名称       │ 位宽 │ 读/写  │  说明          │
├──────────┼──────────────────┼─────┼────────┼────────────────┤
│ 0x0000   │ CTRL_REG         │ 32  │ R/W    │ 控制寄存器     │
│          │  [0]    START    │     │        │   1: 启动      │
│          │  [1]    STOP     │     │        │   1: 停止      │
│          │  [2]    RESET    │     │        │   1: 复位      │
│          │  [31:3] RESERVED │     │        │   保留         │
├──────────┼──────────────────┼─────┼────────┼────────────────┤
│ 0x0004   │ STATUS_REG       │ 32  │ R      │ 状态寄存器     │
│          │  [0]    BUSY     │     │        │   1: 忙        │
│          │  [1]    READY    │     │        │   1: 准备好    │
│          │  [31:2] RESERVED │     │        │   保留         │
├──────────┼──────────────────┼─────┼────────┼────────────────┤
│ 0x0010   │ DATA_REG         │ 32  │ R      │ 数据寄存器     │
│          │                  │     │        │   有符号Q15    │
│          │                  │     │        │   范围:-1~+1   │
│          │                  │     │        │   公式见下方   │
└──────────┴──────────────────┴─────┴────────┴────────────────┘

数据格式转换公式:
  DATA_REG (Q15格式):
    float_value = (int16_t)(register_value >> 16) / 32768.0
```

#### 5.1.2 硬件抽象层实现

**FPGA 团队应提供**:
- ✅ 底层驱动库（如 `liblowlevel.so`）
- ✅ 封装寄存器访问函数
- ✅ UIO/PCIe 驱动（或者使用通用驱动）

**示例：硬件库接口定义**

```c
/* liblowlevel.h */

/**
 * @brief 初始化硬件
 * @return 0: 成功, -1: 失败
 */
int HardwareInit(void);

/**
 * @brief 读取寄存器
 * @param addr 寄存器地址（相对于基地址的偏移）
 * @return 32-bit 寄存器值
 */
uint32_t ReadRegister(uint32_t addr);

/**
 * @brief 写入寄存器
 * @param addr 寄存器地址
 * @param value 要写入的值
 */
void WriteRegister(uint32_t addr, uint32_t value);

/**
 * @brief 读取数组数据（从 FIFO）
 * @param addr FIFO 地址
 * @param channel 通道号
 * @param data 输出缓冲区
 * @param len 数据长度
 */
void ReadArray(uint32_t addr, int channel, float *data, int len);

/**
 * @brief 等待数据准备好
 * @return 1: 准备好, 0: 未准备好
 */
int WaitDataReady(void);

/**
 * @brief 触发采集
 */
void TriggerAcquisition(void);

/**
 * @brief 清理资源
 */
void HardwareCleanup(void);
```

#### 5.1.3 测试与验证

**FPGA 团队负责**:
- ✅ 提供硬件自检程序
- ✅ 提供寄存器读写测试脚本
- ✅ 提供数据采集示例程序

**示例：硬件自检程序**

```c
/* fpga_selftest.c - FPGA 团队提供 */

#include "liblowlevel.h"

int main()
{
    // 1. 初始化
    if (HardwareInit() < 0) {
        printf("ERROR: Hardware init failed\n");
        return -1;
    }
    
    // 2. 检查版本
    uint32_t version = ReadRegister(0x0008);
    printf("Firmware Version: %d.%d\n", 
           (version >> 16) & 0xFFFF, version & 0xFFFF);
    
    // 3. 测试寄存器读写
    WriteRegister(0x0100, 0xAA);
    uint32_t readback = ReadRegister(0x0100);
    if (readback != 0xAA) {
        printf("ERROR: Register R/W test failed\n");
        return -1;
    }
    
    // 4. 测试数据采集
    TriggerAcquisition();
    if (!WaitDataReady()) {
        printf("ERROR: Data not ready\n");
        return -1;
    }
    
    float data[10];
    ReadArray(0x1000, 0, data, 10);
    printf("Sample data: %.4f, %.4f, ...\n", data[0], data[1]);
    
    printf("FPGA self-test PASSED\n");
    HardwareCleanup();
    return 0;
}
```

---

### 5.2 IOC 开发者职责

#### 5.2.1 EPICS 软件开发

**核心职责**:
- ✅ 设计 Device Support 和 Driver Support
- ✅ 定义 PV 数据库
- ✅ 实现数据转换和单位换算
- ✅ 实现 I/O Intr 扫描机制
- ✅ 编写 IOC 启动脚本

**不负责**:
- ❌ FPGA 逻辑设计
- ❌ 硬件寄存器地址分配
- ❌ 底层驱动开发（可以使用 FPGA 团队提供的库）

#### 5.2.2 接口适配与集成

**IOC 开发者的工作**:

```c
/* driverWrapper.c - IOC 开发者编写 */

#include <dlfcn.h>
#include "liblowlevel.h"  // FPGA 团队提供

// 动态加载硬件库
static long InitDriver(void)
{
    void *handle = dlopen("liblowlevel.so", RTLD_NOW);
    
    // 映射 FPGA 团队提供的函数
    hwInit = dlsym(handle, "HardwareInit");
    hwReadReg = dlsym(handle, "ReadRegister");
    hwWriteReg = dlsym(handle, "WriteRegister");
    
    // 调用初始化
    hwInit();
    
    return 0;
}

// 封装为 EPICS 接口
float ReadData(int offset, int channel, int type)
{
    uint32_t raw_value;
    
    // 根据 offset 确定寄存器地址
    // （这是 IOC 开发者设计的映射逻辑）
    switch (offset) {
        case 0:  // RF 幅度
            raw_value = hwReadReg(0x0010 + channel * 8);
            return convert_to_voltage(raw_value);
        
        case 5:  // BPM 电压
            raw_value = hwReadReg(0x0200 + channel * 4);
            return (float)raw_value;
        
        // ... 更多映射 ...
    }
}
```

#### 5.2.3 测试与文档

**IOC 开发者负责**:
- ✅ 测试所有 PV 的读写功能
- ✅ 验证数据正确性
- ✅ 编写 PV 列表文档
- ✅ 编写用户手册

---

### 5.3 协作流程

#### 5.3.1 项目启动阶段

```
Week 1: 需求确认
┌─────────────────────────────────────────────────────────┐
│ FPGA 团队        │ IOC 团队                             │
├──────────────────┼──────────────────────────────────────┤
│ 确认功能需求     │ 确认控制需求                         │
│ 确认性能指标     │ 确认 PV 数量和类型                   │
│ 确认硬件平台     │ 确认扫描周期                         │
└──────────────────┴──────────────────────────────────────┘
                        │
                        ▼
                 共同制定接口规范
                        │
                        ▼
Week 2: 接口设计
┌─────────────────────────────────────────────────────────┐
│ FPGA 团队                │ IOC 团队                     │
├──────────────────────────┼──────────────────────────────┤
│ 设计寄存器地址映射表      │ 审核寄存器映射表             │
│ 定义数据格式              │ 确认数据格式是否满足需求     │
│ 定义中断机制              │ 确认 I/O Intr 需求           │
└──────────────────────────┴──────────────────────────────┘
                        │
                        ▼
              双方签字确认接口文档
```

#### 5.3.2 开发阶段

```
Week 3-6: 并行开发
┌─────────────────────────────────────────────────────────┐
│ FPGA 团队                │ IOC 团队                     │
├──────────────────────────┼──────────────────────────────┤
│ 实现 FPGA 逻辑           │ 实现 Device/Driver Support   │
│ 编写硬件库 liblowlevel.so│ 定义 PV 数据库               │
│ 提供自检程序              │ 编写测试脚本                 │
└──────────────────────────┴──────────────────────────────┘
                        │
                        ▼
              定期同步进度（每周例会）
                        │
                        ▼
Week 7: 集成测试
┌─────────────────────────────────────────────────────────┐
│ FPGA 团队                │ IOC 团队                     │
├──────────────────────────┼──────────────────────────────┤
│ 提供测试固件              │ 运行 IOC                     │
│ 监控 FPGA 状态           │ 测试 PV 读写                 │
│ 提供 ILA 调试数据         │ 提供 CA 测试结果             │
└──────────────────────────┴──────────────────────────────┘
                        │
                        ▼
              发现问题 → 定位问题 → 修复问题
                        │
                        ▼
Week 8: 性能优化与文档
```

#### 5.3.3 联调阶段

**问题定位流程**:

```
问题出现
    │
    ├─ 是否能读取寄存器？
    │   ├─ 否 → FPGA 团队检查硬件连接、UIO 配置
    │   └─ 是 ↓
    │
    ├─ 读取的值是否正确？
    │   ├─ 否 → FPGA 团队检查数据格式、逻辑实现
    │   └─ 是 ↓
    │
    ├─ IOC 转换后的值是否正确？
    │   ├─ 否 → IOC 团队检查单位转换公式
    │   └─ 是 ↓
    │
    ├─ PV 是否能通过 CA 访问？
    │   ├─ 否 → IOC 团队检查 DBD、数据库定义
    │   └─ 是 ↓
    │
    └─ 功能正常！
```

**日志记录规范**:

```c
// FPGA 团队：在硬件库中添加日志
void WriteRegister(uint32_t addr, uint32_t value)
{
    printf("[HW] Write: addr=0x%04X, value=0x%08X\n", addr, value);
    // ... 实际写操作 ...
}

// IOC 团队：在驱动层添加日志
void WriteData(int offset, int channel, float value)
{
    printf("[IOC] WriteData: offset=%d, ch=%d, val=%.4f\n", 
           offset, channel, value);
    // ... 调用硬件库 ...
}

// 联调时开启日志，可以清楚看到数据流动：
// [IOC] WriteData: offset=20, ch=0, val=1.2340
// [HW] Write: addr=0x0300, value=0x00009EBA
```

#### 5.3.4 交付阶段

**FPGA 团队交付清单**:
- [ ] 固件二进制文件（.bit / .rbf）
- [ ] 硬件库源码和编译后的 .so 文件
- [ ] 完整的寄存器映射文档
- [ ] 硬件自检程序
- [ ] 测试报告

**IOC 团队交付清单**:
- [ ] IOC 源码
- [ ] 编译后的 IOC 可执行文件
- [ ] 数据库文件（.db / .substitutions）
- [ ] 启动脚本（st.cmd）
- [ ] PV 列表文档
- [ ] 用户手册

---

### 5.4 接口变更管理

#### 5.4.1 变更流程

```
变更请求
    │
    ├─ 谁提出？
    │   ├─ FPGA 团队：需要修改寄存器地址
    │   └─ IOC 团队：需要新增控制功能
    │
    ▼
评估影响
    │
    ├─ 是否影响接口？
    │   ├─ 是 → 双方讨论并更新接口文档
    │   └─ 否 → 只需一方修改
    │
    ▼
更新文档
    │
    ├─ 更新寄存器映射表
    ├─ 更新版本号
    └─ 记录变更日志
    │
    ▼
重新测试
    │
    ├─ FPGA 自检
    ├─ IOC 功能测试
    └─ 集成测试
    │
    ▼
发布新版本
```

#### 5.4.2 版本管理

**固件版本号规范**:
```
版本号格式: MAJOR.MINOR

MAJOR: 主版本号（接口不兼容变更）
MINOR: 次版本号（接口兼容的功能新增）

示例:
  v1.0 → v1.1: 新增寄存器，兼容
  v1.1 → v2.0: 修改寄存器地址，不兼容
```

**IOC 版本号**:
```
IOC 版本号应与固件版本号对应

IOC v1.0 支持固件 v1.x
IOC v2.0 支持固件 v2.x
```

---

### 5.5 沟通机制

#### 5.5.1 定期会议

**每周例会**:
- 时间：每周一 10:00
- 参与者：FPGA 团队负责人 + IOC 团队负责人
- 内容：
  - 进度汇报
  - 问题讨论
  - 下周计划

#### 5.5.2 问题跟踪

**使用 Issue Tracker**:

```
Issue #12: CH0 读取的幅度值全是 0

标签: bug, FPGA
优先级: High
负责人: FPGA Team

描述:
  IOC 读取 CH0_AMP 寄存器 (0x0010)，返回值始终为 0。
  其他通道正常。

测试结果:
  - 硬件自检程序读取：正常，值为 0x12345678
  - IOC 读取：0x00000000

时间线:
  2024-11-18 10:00 - IOC 团队发现问题
  2024-11-18 11:00 - FPGA 团队确认是 AXI 地址映射错误
  2024-11-18 14:00 - 修复并发布新固件 v1.1
  2024-11-18 15:00 - IOC 团队验证通过，关闭 Issue
```

#### 5.5.3 文档共享

**推荐使用 Git 管理文档**:

```
project-docs/
├── interface/
│   ├── register_map.md          # 寄存器映射（共同维护）
│   ├── data_format.md           # 数据格式说明
│   └── version_history.md       # 版本历史
├── fpga/
│   ├── design_spec.md           # FPGA 设计文档（FPGA 团队）
│   └── test_report.md           # 测试报告
└── ioc/
    ├── pv_list.md               # PV 列表（IOC 团队）
    └── user_guide.md            # 用户手册
```

---

## 第五部分总结

成功的软硬件协同开发需要：

1. **明确职责**:
   - FPGA 团队：硬件实现 + 底层库 + 接口文档
   - IOC 团队：EPICS 软件 + 数据适配 + 用户界面

2. **规范接口**:
   - 详细的寄存器映射文档
   - 清晰的数据格式定义
   - 版本管理和变更控制

3. **高效协作**:
   - 定期同步进度
   - 问题快速定位和修复
   - 文档共享和及时更新

4. **联调流程**:
   - 先各自测试，再集成测试
   - 使用日志记录数据流动
   - 建立问题跟踪机制

---

## 第六部分：从零重建系统的可执行路线图

本节提供一个完整的、按时间顺序的路线图，让你能够独立搭建 EPICS IOC 与 FPGA 的集成系统。

### 6.1 环境搭建（第 1-2 天）

#### Day 1: 安装 EPICS Base

**目标**: 搭建基本的 EPICS 开发环境

**步骤**:

```bash
# 1. 安装依赖
sudo apt-get update
sudo apt-get install build-essential gcc g++ make perl \
                     libreadline-dev re2c

# 2. 下载 EPICS Base
cd /opt
sudo git clone --recursive https://github.com/epics-base/epics-base.git
cd epics-base
sudo git checkout R7.0.7  # 或最新稳定版本

# 3. 编译 EPICS Base（ARM 平台）
sudo make -j4

# 4. 设置环境变量
echo 'export EPICS_BASE=/opt/epics-base' >> ~/.bashrc
echo 'export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)' >> ~/.bashrc
echo 'export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}' >> ~/.bashrc
source ~/.bashrc

# 5. 验证安装
caget -h
# 应该显示 caget 的帮助信息
```

**验证**:
```bash
$ which caget
/opt/epics-base/bin/linux-arm/caget

$ echo $EPICS_BASE
/opt/epics-base
```

#### Day 2: 验证硬件环境

**目标**: 确认 FPGA 硬件可访问

**步骤**:

```bash
# 1. 检查 UIO 设备（Zynq 平台）
ls -l /dev/uio*

# 如果没有，检查设备树和内核驱动
dmesg | grep uio

# 2. 检查 UIO 映射信息
cat /sys/class/uio/uio0/name
cat /sys/class/uio/uio0/maps/map0/addr
cat /sys/class/uio/uio0/maps/map0/size

# 3. 编译并运行硬件测试程序（FPGA 团队提供）
gcc -o test_fpga test_fpga.c
sudo ./test_fpga

# 预期输出：所有测试通过
```

**检查清单**:
- [ ] UIO 设备存在且可访问
- [ ] 能读取固件版本号
- [ ] 寄存器读写测试通过
- [ ] 数据采集测试通过

---

### 6.2 项目创建（第 3 天）

#### Step 1: 创建 IOC 项目

```bash
cd ~
makeBaseApp.pl -t ioc MyBPM
makeBaseApp.pl -i -t ioc MyBPM
# 选择 Application name: MyBPM
```

#### Step 2: 创建源文件

```bash
cd MyBPMIOC/MyBPMApp/src/

# 创建头文件
cat > devMyBPM.h << 'HEADER_EOF'
#ifndef _devMyBPM_H
#define _devMyBPM_H

typedef enum {
    TYPE_REG = 0,
    TYPE_AMP,
    TYPE_PHASE
} data_type_t;

typedef struct {
    data_type_t type;
    unsigned short offset;
    unsigned short channel;
} record_priv_t;

#endif
HEADER_EOF

cat > driverWrapper.h << 'DRIVER_HEADER_EOF'
#ifndef _driverWrapper_H
#define _driverWrapper_H

#include <dbScan.h>

float ReadData(int offset, int channel, int type);
void WriteData(int offset, int channel, float value);
IOSCANPVT GetIoIntScanPvt(void);

#endif
DRIVER_HEADER_EOF

# 创建 DBD 文件
cat > devMyBPM.dbd << 'DBD_EOF'
device(ai, INST_IO, devAi, "MyBPM")
device(ao, INST_IO, devAo, "MyBPM")
driver(drMyBPM)
DBD_EOF
```

#### Step 3: 实现核心代码

参考第四部分的代码，实现：
- `devMyBPM.c`
- `driverWrapper.c`

#### Step 4: 配置 Makefile

参考第四部分 4.4 节的 Makefile 配置。

---

### 6.3 实现最小功能（第 4-5 天）

#### Day 4: 实现基本读写

**目标**: 能读取一个 AI record 和写入一个 AO record

**步骤**:

1. **实现简化版 Device Support**:

```c
// devMyBPM.c - 最小实现

static long read_ai(aiRecord *record)
{
    record_priv_t *priv = (record_priv_t *)record->dpvt;
    
    // 直接调用驱动函数
    float value = ReadData(priv->offset, priv->channel, priv->type);
    
    record->val = value;
    record->udf = FALSE;
    return 2;
}
```

2. **实现简化版 Driver Support**:

```c
// driverWrapper.c - 最小实现

static void *fpga_base = NULL;

static long InitDriver(void)
{
    int fd = open("/dev/uio0", O_RDWR | O_SYNC);
    fpga_base = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    printf("FPGA base: %p\n", fpga_base);
    return 0;
}

float ReadData(int offset, int channel, int type)
{
    volatile uint32_t *reg = (uint32_t*)(fpga_base + offset);
    uint32_t raw = reg[channel];
    
    printf("Read: offset=0x%04X, channel=%d, raw=0x%08X\n", 
           offset, channel, raw);
    
    // 简单的单位转换
    return (float)raw / 1000.0;
}
```

3. **创建测试数据库**:

```bash
cat > MyBPMApp/Db/test.db << 'DB_EOF'
record(ai, "TEST:Amp")
{
    field(DTYP, "MyBPM")
    field(INP,  "@AMP:16 ch=0")   # 偏移 0x10, 通道 0
    field(SCAN, "1 second")
    field(PREC, "4")
}

record(ao, "TEST:Ctrl")
{
    field(DTYP, "MyBPM")
    field(OUT,  "@REG:256 ch=0")  # 偏移 0x100, 通道 0
    field(SCAN, "Passive")
}
DB_EOF
```

4. **编译并测试**:

```bash
cd ~/MyBPMIOC
make clean && make

cd iocBoot/iocMyBPM
# 修改 st.cmd，加载 test.db
# dbLoadRecords("db/test.db")

sudo ./st.cmd

# 在 IOC shell 中测试
epics> dbgf TEST:Amp
epics> dbpf TEST:Ctrl 123
```

**验证**:
- [ ] IOC 能启动
- [ ] 能读取 TEST:Amp 的值
- [ ] 能写入 TEST:Ctrl

#### Day 5: 添加更多功能

逐步添加：
- 更多数据类型（PHASE, VOLTAGE, POSITION）
- 波形记录
- 状态监控

---

### 6.4 集成测试（第 6-7 天）

#### Day 6: 功能测试

**测试清单**:

```bash
# 1. 测试所有 PV 可访问
$ caget $(cat pv_list.txt)

# 2. 测试读取值的范围
$ caget TEST:CH0:Amp TEST:CH1:Amp TEST:CH2:Amp
# 检查值是否在合理范围内

# 3. 测试写入功能
$ caput TEST:K1:Set 1.234
$ caget TEST:K1:RBV
# 验证回读值是否正确

# 4. 测试波形数据
$ caget -a TEST:AmpWave
# 检查波形长度和值

# 5. 测试 I/O Intr
$ camonitor TEST:Status:DataReady
# 观察是否触发更新
```

#### Day 7: 性能测试

**测试脚本**:

```bash
#!/bin/bash
# performance_test.sh

echo "=== Performance Test ==="

# 1. 测试单次读取延迟
echo "1. Single read latency:"
time caget TEST:CH0:Amp > /dev/null

# 2. 测试批量读取
echo "2. Batch read (100 PVs):"
time for i in {0..99}; do
    caget TEST:CH$i:Amp > /dev/null
done

# 3. 测试更新速率
echo "3. Update rate (monitor for 10 seconds):"
timeout 10 camonitor TEST:CH0:Amp | wc -l

# 4. 测试波形吞吐量
echo "4. Waveform throughput:"
time caget -a TEST:AmpWave > /dev/null

echo "=== Test Complete ==="
```

**预期结果**:
- 单次读取延迟 < 1 ms
- 批量读取 100 PVs < 100 ms
- 波形读取（10000 点）< 50 ms

---

### 6.5 优化与部署（第 8-10 天）

#### Day 8: 性能优化

**优化点**:

1. **减少日志输出**:
```c
// 发布版本关闭调试日志
#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif
```

2. **优化扫描周期**:
```
# 根据实际需求调整
record(ai, "BPM:CH0:Amp")
{
    field(SCAN, ".1 second")  # 10 Hz
}
```

3. **使用 I/O Intr 替代周期扫描**:
```
# 数据准备好时才更新
record(waveform, "BPM:AmpWave")
{
    field(SCAN, "I/O Intr")
}
```

#### Day 9: 文档编写

**必需文档**:

1. **README.md**:
```markdown
# MyBPM IOC

## 系统要求
- EPICS Base 7.0.7
- Zynq FPGA 固件 v2.0+
- Linux kernel 5.10+

## 编译
```
make
```

## 运行
```
cd iocBoot/iocMyBPM
sudo ./st.cmd
```

## PV 列表
见 docs/PVList.md
```

2. **PV 列表**（见第四部分 4.8.2）

3. **故障排查指南**（见第四部分 4.7.4）

#### Day 10: 部署与验收

**部署检查清单**:

- [ ] IOC 自动启动脚本
```bash
# /etc/systemd/system/mybpm-ioc.service
[Unit]
Description=MyBPM EPICS IOC
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/home/user/MyBPMIOC/iocBoot/iocMyBPM
ExecStart=/home/user/MyBPMIOC/iocBoot/iocMyBPM/st.cmd
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable mybpm-ioc
sudo systemctl start mybpm-ioc
sudo systemctl status mybpm-ioc
```

- [ ] 日志记录
```bash
# 修改 st.cmd
epicsEnvSet("EPICS_IOC_LOG_FILE_NAME", "/var/log/mybpm-ioc.log")
```

- [ ] 监控脚本
```bash
#!/bin/bash
# check_ioc.sh - 定期检查 IOC 是否运行

PV="TEST:Status:DataReady"

if ! caget $PV > /dev/null 2>&1; then
    echo "IOC is down! Restarting..."
    systemctl restart mybpm-ioc
fi
```

---

### 6.6 完整时间线总结

```
┌─────────────────────────────────────────────────────────────┐
│                    10 天实施计划                             │
├─────────┬───────────────────────────────────────────────────┤
│ Day 1   │ 安装 EPICS Base                                   │
├─────────┼───────────────────────────────────────────────────┤
│ Day 2   │ 验证硬件环境，运行 FPGA 自检程序                  │
├─────────┼───────────────────────────────────────────────────┤
│ Day 3   │ 创建 IOC 项目，搭建基本框架                       │
├─────────┼───────────────────────────────────────────────────┤
│ Day 4   │ 实现最小功能（1个AI + 1个AO）                     │
├─────────┼───────────────────────────────────────────────────┤
│ Day 5   │ 添加完整功能（多通道、波形、I/O Intr）            │
├─────────┼───────────────────────────────────────────────────┤
│ Day 6   │ 功能测试，验证所有 PV                             │
├─────────┼───────────────────────────────────────────────────┤
│ Day 7   │ 性能测试，调优                                    │
├─────────┼───────────────────────────────────────────────────┤
│ Day 8   │ 代码优化，减少延迟                                │
├─────────┼───────────────────────────────────────────────────┤
│ Day 9   │ 编写文档（README、PV列表、故障排查）              │
├─────────┼───────────────────────────────────────────────────┤
│ Day 10  │ 部署上线，配置自动启动和监控                      │
└─────────┴───────────────────────────────────────────────────┘
```

---

### 6.7 验证清单

完成所有开发后，使用此清单验证系统：

**功能验证**:
- [ ] 所有 PV 都能通过 `caget` 访问
- [ ] 读取的值在合理范围内
- [ ] 写入的值能正确回读
- [ ] 波形数据长度和内容正确
- [ ] I/O Intr 能正常触发
- [ ] 中断延迟 < 100 ms

**性能验证**:
- [ ] 单次读取延迟 < 1 ms
- [ ] 扫描周期稳定（无抖动）
- [ ] 波形读取速度 > 100 MB/s
- [ ] CPU 占用率 < 20%
- [ ] 内存占用 < 100 MB

**稳定性验证**:
- [ ] 连续运行 24 小时无崩溃
- [ ] FPGA 复位后 IOC 能恢复
- [ ] 网络中断后 CA 能重连
- [ ] 日志无异常错误

**文档验证**:
- [ ] README 完整且准确
- [ ] PV 列表与实际一致
- [ ] 寄存器映射文档清晰
- [ ] 用户手册易于理解

---

## 第六部分总结

按照本路线图，你可以在 **10 天内**从零搭建一个完整的 EPICS IOC 与 FPGA 集成系统：

1. **环境搭建**（Day 1-2）: EPICS + 硬件验证
2. **项目创建**（Day 3）: IOC 框架
3. **功能实现**（Day 4-5）: Device/Driver Support
4. **测试验证**（Day 6-7）: 功能 + 性能
5. **优化部署**（Day 8-10）: 优化 + 文档 + 上线

关键成功因素：
- 充分理解 FPGA 提供的接口文档
- 使用分层架构保持代码清晰
- 增量开发，先实现最小功能
- 及时测试，快速发现问题

---

## 附录

### 附录 A: 完整代码示例（基于 BPMIOC）

#### A.1 Device Support 完整实现

**参考文件**: `BPMmonitorApp/src/devBPMMonitor.c`

关键代码片段：

```c
// INP 字段解析示例
// 输入格式: "@AMP:0 ch=2" 或 "@REG:10 ch=0"
static int devIoParse(char *string, recordpara_t *recordpara)
{
    char typeName[10] = "";
    char *pchar = string;
    int nchar;
    
    // 解析类型名 (AMP, PHASE, REG, etc.)
    nchar = strcspn(pchar, ":");
    strncpy(typeName, pchar, nchar);
    typeName[nchar] = '\0';
    pchar += nchar + 1;
    
    // 映射到枚举
    if (strcmp(typeName, "AMP") == 0)
        recordpara->type = AMP;
    else if (strcmp(typeName, "PHASE") == 0)
        recordpara->type = PHASE;
    // ... 更多类型 ...
    
    // 解析 offset
    recordpara->offset = strtol(pchar, &pchar, 0);
    
    // 解析 channel
    pchar = strstr(pchar, "ch=");
    if (pchar) {
        recordpara->channel = strtol(pchar + 3, NULL, 0);
    }
    
    return 0;
}
```

#### A.2 Driver Support 动态库加载

**参考文件**: `BPMmonitorApp/src/driverWrapper.c`

```c
static long InitDevice()
{
    void *handle;
    int (*funcOpen)();
    
    // 1. 动态加载硬件库
    handle = dlopen("liblowlevel.so", RTLD_NOW);
    if (handle == NULL) {
        fprintf(stderr, "Failed to load: %s\n", dlerror());
        return -1;
    }
    
    // 2. 初始化硬件
    funcOpen = dlsym(handle, "SystemInit");
    if (funcOpen() != 0) {
        printf("Hardware init failed!\n");
        return -1;
    }
    
    // 3. 映射所有硬件函数
    funcGetRfInfo = dlsym(handle, "GetRfInfo");
    funcGetVcValue = dlsym(handle, "GetVcValue");
    funcSetBPMk1 = dlsym(handle, "SetBPMk1");
    // ... 更多函数 ...
    
    // 4. 创建后台线程
    pthread_t tid;
    pthread_create(&tid, NULL, pthread, NULL);
    
    // 5. 初始化 I/O Intr
    scanIoInit(&TriginScanPvt);
    
    return 0;
}
```

#### A.3 I/O Intr 后台线程

```c
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

---

### 附录 B: 常见问题排查

#### B.1 编译错误

**问题**: `undefined reference to 'epicsExportAddress'`

**原因**: 缺少 EPICS 头文件或链接库

**解决**:
```makefile
# 检查 Makefile
MyBPM_LIBS += $(EPICS_BASE_IOC_LIBS)  # ← 必须包含
```

---

**问题**: `dset/drvet` 未定义

**原因**: DBD 文件未正确包含

**解决**:
```makefile
# src/Makefile
MyBPM_DBD += base.dbd
MyBPM_DBD += devMyBPM.dbd  # ← 确保包含
```

---

#### B.2 运行时错误

**问题**: IOC 启动时段错误

```
Segmentation fault (core dumped)
```

**调试步骤**:

1. 使用 gdb 调试:
```bash
$ gdb ./MyBPM
(gdb) run st.cmd
# 记录崩溃位置
(gdb) backtrace
```

2. 检查常见原因:
   - 空指针访问
   - 内存未映射（mmap 失败）
   - 数组越界

---

**问题**: `Cannot find PV: TEST:Amp`

**原因**: 数据库未加载或 DTYP 不匹配

**解决**:
```bash
# 在 IOC shell 中检查
epics> dbl
# 如果看不到 PV，检查 st.cmd
# dbLoadRecords("db/test.db")  ← 是否加载？

epics> dbpr TEST:Amp
# 检查 DTYP 字段是否正确
```

---

**问题**: 读取的值总是 0 或 NaN

**调试步骤**:

1. 检查硬件:
```bash
$ sudo ./test_fpga
# 硬件是否正常？
```

2. 添加日志:
```c
float ReadData(int offset, int channel, int type)
{
    printf("DEBUG: offset=%d, ch=%d, type=%d\n", offset, channel, type);
    
    uint32_t raw = hwReadReg(addr);
    printf("DEBUG: raw=0x%08X\n", raw);
    
    float result = convert(raw);
    printf("DEBUG: result=%.4f\n", result);
    
    return result;
}
```

3. 检查单位转换:
```c
// 常见错误：忘记类型转换
float value = raw / 1000;  // ← 错误！整数除法
float value = (float)raw / 1000.0;  // ← 正确
```

---

**问题**: I/O Intr 不触发

**调试步骤**:

1. 检查 get_ioint_info 函数:
```c
static long devGetIoIntInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt)
{
    printf("get_ioint_info called for %s\n", record->name);  // ← 添加日志
    *ppvt = GetIoIntScanPvt();
    return 0;
}
```

2. 检查后台线程:
```c
void *pthread()
{
    printf("Thread started\n");  // ← 确认线程运行
    while(1) {
        if (data_ready()) {
            printf("Triggering I/O Intr\n");  // ← 添加日志
            scanIoRequest(scanPvt);
        }
        usleep(100000);
    }
}
```

3. 手动触发测试:
```bash
epics> scanIoRequest  # IOC shell 命令（如果实现了）
```

---

#### B.3 性能问题

**问题**: CPU 占用率过高

**原因**: 扫描周期太短或轮询过于频繁

**解决**:
```
# 调整扫描周期
record(ai, "BPM:Amp")
{
    field(SCAN, ".1 second")  # 从 ".01 second" 改为 ".1 second"
}

# 或使用 I/O Intr 替代周期扫描
record(ai, "BPM:Amp")
{
    field(SCAN, "I/O Intr")
}
```

---

**问题**: 波形读取缓慢

**原因**: 逐点读取，没有批量传输

**解决**:
```c
// 差的做法
void readWaveform(float *data, int len)
{
    for (int i = 0; i < len; i++) {
        data[i] = hwReadReg(FIFO_ADDR);  // 每次读取都有开销
    }
}

// 好的做法（使用 DMA 或批量传输）
void readWaveform(float *data, int len)
{
    hwBulkRead(FIFO_ADDR, data, len);  // 一次性读取
}
```

---

### 附录 C: EPICS 工具参考

#### C.1 常用 CA 命令

```bash
# caget - 读取 PV
caget PV_NAME
caget -t PV_NAME          # 显示时间戳
caget -a PV_NAME          # 显示数组（波形）
caget -n PV_NAME          # 不等待连接

# caput - 写入 PV
caput PV_NAME value
caput -t PV_NAME value    # 显示处理时间
caput -c PV_NAME value    # 等待 callback

# camonitor - 监控 PV
camonitor PV_NAME
camonitor -t PV_NAME      # 显示时间戳
camonitor -n PV_NAME      # 不显示初始值

# cainfo - PV 信息
cainfo PV_NAME            # 显示 PV 详细信息

# caget 批量操作
caget PV1 PV2 PV3 ...     # 读取多个 PV
cat pv_list.txt | xargs caget  # 从文件读取
```

#### C.2 IOC Shell 命令

```bash
# 数据库操作
dbl                       # 列出所有 PV
dbl "pattern*"            # 列出匹配的 PV
dbpr PV_NAME              # 显示 record 信息
dbpr PV_NAME 2            # 显示更详细的信息
dbgf PV_NAME.FIELD        # 获取字段值
dbpf PV_NAME.FIELD value  # 设置字段值

# 扫描相关
scanppl                   # 显示周期扫描列表
scanpel                   # 显示事件扫描列表
scanpiol                  # 显示 I/O 事件扫描列表

# 驱动相关
dbior                     # 显示所有驱动报告
dbior driver_name level   # 显示特定驱动信息

# 系统信息
epicsEnvShow              # 显示环境变量
epicsMutexShowAll         # 显示互斥锁
epicsThreadShowAll        # 显示所有线程
```

---

### 附录 D: 参考资源

#### D.1 官方文档

**EPICS 官方资源**:
- EPICS Home: https://epics-controls.org/
- EPICS Base Repository: https://github.com/epics-base/epics-base
- EPICS Documentation: https://docs.epics-controls.org/
- Application Developer's Guide: https://epics.anl.gov/base/R7-0/6-docs/AppDevGuide.pdf

**设备支持开发**:
- Device Support Tutorial: https://epics.anl.gov/modules/soft/asyn/
- Record Reference Manual: https://epics.anl.gov/base/R7-0/6-docs/RecordReference.html

#### D.2 FPGA 开发资源

**Xilinx Zynq**:
- Zynq-7000 TRM: https://www.xilinx.com/support/documentation/user_guides/ug585-Zynq-7000-TRM.pdf
- AXI Reference Guide: https://www.xilinx.com/support/documentation/ip_documentation/axi_ref_guide/latest/ug1037-vivado-axi-reference-guide.pdf
- UIO Driver: https://www.kernel.org/doc/html/latest/driver-api/uio-howto.html

**Intel (Altera)**:
- Avalon Interface Specifications: https://www.intel.com/content/www/us/en/programmable/documentation/
- PCIe IP Core User Guide: https://www.intel.com/content/www/us/en/programmable/documentation/

#### D.3 社区资源

**EPICS 社区**:
- EPICS Tech-Talk 邮件列表: https://epics.anl.gov/tech-talk/
- GitHub EPICS Modules: https://github.com/epics-modules
- EPICS Collaboration: https://epics-controls.org/resources-and-support/collaboration/

**示例项目**:
- ADCore (Area Detector): https://github.com/areaDetector/ADCore
- StreamDevice: https://github.com/paulscherrerinstitute/StreamDevice
- ASYN Driver: https://github.com/epics-modules/asyn

---

### 附录 E: 术语表

| 术语 | 英文全称 | 中文解释 |
|------|---------|---------|
| EPICS | Experimental Physics and Industrial Control System | 实验物理与工业控制系统 |
| IOC | Input/Output Controller | 输入输出控制器 |
| PV | Process Variable | 过程变量 |
| CA | Channel Access | 通道访问协议 |
| DBD | Database Definition | 数据库定义 |
| DTYP | Device Type | 设备类型 |
| INP/OUT | Input/Output | 输入/输出字段 |
| SCAN | Scan | 扫描机制 |
| dset | Device Support Entry Table | 设备支持函数表 |
| drvet | Driver Support Entry Table | 驱动支持函数表 |
| FPGA | Field Programmable Gate Array | 现场可编程门阵列 |
| AXI | Advanced eXtensible Interface | 高级可扩展接口 |
| UIO | Userspace I/O | 用户空间 I/O |
| DMA | Direct Memory Access | 直接内存访问 |
| PCIe | PCI Express | PCI 快速总线 |
| BAR | Base Address Register | 基地址寄存器 |
| MSI | Message Signaled Interrupts | 消息信号中断 |
| I/O Intr | I/O Interrupt | I/O 中断扫描 |

---

## 全文总结

本技术指南详细讲解了 EPICS IOC 与 FPGA 的交互机制，涵盖：

### 核心内容回顾

1. **系统架构** (第一部分):
   - EPICS 分层架构：Record → Device Support → Driver Support → Hardware
   - 数据流路径：PV 读写如何一步步到达 FPGA 寄存器
   - I/O Intr 事件驱动机制

2. **通信接口** (第二部分):
   - PCIe：高性能，适合大数据量
   - Zynq/AXI：BPMIOC 使用方案，ARM + FPGA 集成
   - Etherbone：远程访问
   - SPI/I2C：配置接口

3. **开发者必备** (第三部分):
   - FPGA 团队必须提供的文档
   - 硬件验证方法
   - 寄存器地址映射设计

4. **实战指南** (第四部分):
   - 从零创建 IOC 项目
   - Device Support 和 Driver Support 编写
   - 数据库定义和测试验证

5. **协作流程** (第五部分):
   - 明确职责划分
   - 接口规范和变更管理
   - 问题定位流程

6. **可执行路线图** (第六部分):
   - 10 天完整实施计划
   - 环境搭建 → 功能实现 → 测试部署
   - 验证清单

### 关键要点

✅ **理解分层架构**: Record、Device Support、Driver Support 各司其职

✅ **掌握数据流**: 从 PV 到 FPGA 寄存器的完整路径

✅ **使用 I/O Intr**: 事件驱动比周期扫描更高效

✅ **明确职责边界**: FPGA 团队提供接口文档，IOC 团队实现软件

✅ **增量开发**: 先实现最小功能，再逐步完善

✅ **及时测试**: 每一步都要验证，快速发现问题

### 最佳实践

1. **代码结构清晰**: 使用标准 EPICS 项目结构
2. **文档齐全**: 寄存器映射、PV 列表、用户手册
3. **日志完善**: 便于调试和问题定位
4. **性能优化**: 使用 I/O Intr、批量传输、合理的扫描周期
5. **版本管理**: 固件和 IOC 版本对应

### 下一步行动

1. **学习 EPICS 基础**: 阅读 Application Developer's Guide
2. **搭建开发环境**: 安装 EPICS Base，运行示例 IOC
3. **研究现有代码**: 分析 BPMIOC 或其他开源项目
4. **动手实践**: 按照第六部分的路线图，搭建自己的系统
5. **参与社区**: 加入 EPICS Tech-Talk，向社区学习

### 致谢

本文档基于 BPMIOC 项目的实际代码编写，感谢所有贡献者。

---

**文档版本**: v1.0  
**最后更新**: 2025-11-18  
**作者**: EPICS IOC 开发团队  
**联系方式**: [根据实际情况填写]

---

**版权声明**:

本文档采用 [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) 许可协议。
您可以自由地分享和修改本文档，但必须注明原作者并以相同方式共享。

---

## 文档结束

