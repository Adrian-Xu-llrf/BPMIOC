# Part 5.1: 设备支持层概述

> **目标**: 理解设备支持层在EPICS中的作用和设计
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 45分钟
> **前置**: Part 3架构总览、Part 4驱动层

## 📖 什么是设备支持层？

设备支持层（Device Support Layer）是EPICS IOC三层架构中的**中间层**，负责连接：
- **上层**：数据库层（Record实例）
- **下层**：驱动层（硬件抽象）

```
数据库层 (.db files)
    ↕ DSET接口
设备支持层 (devBPMMonitor.c) ← 你在这里
    ↕ Driver API
驱动层 (driverWrapper.c)
    ↕ Hardware API
硬件/Mock库
```

---

## 🎯 设备支持层的作用

### 作用1: 协议转换

将Record的通用接口转换为特定驱动的调用：

```c
// Record调用（通用）
process(aiRecord *prec) → read_ai(prec)

// 设备支持转换
read_ai(aiRecord *prec) {
    DevPvt *pPvt = prec->dpvt;
    // 转换为驱动调用
    float value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);
    prec->val = value;
}

// 驱动调用（特定）
ReadData(int offset, int channel, int type)
```

---

### 作用2: 参数解析

解析Record的INP/OUT字段，提取驱动需要的参数：

```c
// .db文件中的配置
record(ai, "BPM:01:RF3:Amp") {
    field(INP, "@0 3 0")  // "@offset channel type"
}

// 设备支持解析
init_record_ai(aiRecord *prec) {
    DevPvt *pPvt = malloc(sizeof(DevPvt));
    sscanf(prec->inp.value.instio.string, "@%d %d %d",
           &pPvt->offset,   // 0
           &pPvt->channel,  // 3
           &pPvt->type);    // 0
    prec->dpvt = pPvt;  // 保存解析结果
}
```

---

### 作用3: I/O中断集成

将驱动的I/O中断与EPICS扫描机制集成：

```c
// 驱动层触发
scanIoRequest(TriginScanPvt);  // 驱动层发出中断

// 设备支持注册
get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt) {
    DevPvt *pPvt = prec->dpvt;
    *ppvt = pPvt->ioscanpvt;  // 返回中断扫描结构
    return 0;
}

// EPICS自动处理
// I/O中断 → 扫描所有注册的Record → 调用read_ai()
```

---

## 🏗️ BPMIOC的设备支持层结构

### 文件: devBPMMonitor.c

**代码规模**: ~500行

**主要组成**:
```c
devBPMMonitor.c
├─ 数据结构定义
│  └─ DevPvt (私有数据)
│
├─ DSET定义（4个）
│  ├─ devAi (analog input)
│  ├─ devAo (analog output)
│  ├─ devLongin (long input)
│  └─ devWaveform (waveform)
│
├─ 初始化函数（4个）
│  ├─ init_record_ai()
│  ├─ init_record_ao()
│  ├─ init_record_longin()
│  └─ init_record_waveform()
│
├─ 读写函数（4个）
│  ├─ read_ai()
│  ├─ write_ao()
│  ├─ read_longin()
│  └─ read_waveform()
│
└─ 辅助函数（1个）
   └─ get_ioint_info()
```

---

## 🔄 设备支持层的工作流程

### 流程1: IOC启动时（初始化）

```
iocInit()
  ↓
dbInitRecords()  // 初始化所有Record
  ↓
对每个Record:
  ↓
  找到对应的DSET（根据DTYP字段）
  ↓
  调用 init_record()
      ↓
      分配DevPvt内存
      ↓
      解析INP/OUT字段
      ↓
      初始化scanIo
      ↓
      保存到prec->dpvt
  ↓
完成！Record可以使用了
```

---

### 流程2: 运行时（数据读取）

```
驱动层: scanIoRequest(TriginScanPvt)
  ↓
EPICS扫描线程被唤醒
  ↓
对所有SCAN="I/O Intr"的Record:
  ↓
  调用 process(aiRecord *prec)
      ↓
      调用 read_ai(prec)
          ↓
          DevPvt *pPvt = prec->dpvt
          ↓
          value = ReadData(pPvt->offset, pPvt->channel, pPvt->type)
          ↓
          prec->val = value
          ↓
          prec->udf = 0
      ↓
      触发前向链接（FLNK）
      ↓
      通知CA客户端
```

---

## 📊 DSET结构详解

### DSET是什么？

**DSET** = **Device Support Entry Table**（设备支持入口表）

它是一个**函数指针数组**，定义了设备支持必须实现的函数接口。

### DSET for AI Record

```c
typedef struct {
    long      number;           // 函数指针数量（通常是6）
    DEVSUPFUN report;           // 报告设备状态（可选）
    DEVSUPFUN init;             // 全局初始化（可选）
    DEVSUPFUN init_record;      // Record初始化 ⭐ 必须
    DEVSUPFUN get_ioint_info;   // 获取I/O中断信息 ⭐ 必须
    DEVSUPFUN read;             // 读取数据 ⭐ 必须
    DEVSUPFUN special_linconv;  // 线性转换（可选）
} aiDset;

// BPMIOC的实现
aiDset devAiBPMMonitor = {
    6,                    // number
    NULL,                 // report (未实现)
    NULL,                 // init (未实现)
    init_record_ai,       // init_record ⭐
    get_ioint_info,       // get_ioint_info ⭐
    read_ai,              // read ⭐
    NULL                  // special_linconv (未实现)
};

// 导出给EPICS
epicsExportAddress(dset, devAiBPMMonitor);
```

### DSET for AO Record

```c
aoDset devAoBPMMonitor = {
    6,
    NULL,
    NULL,
    init_record_ao,
    NULL,               // AO不需要I/O中断
    write_ao,           // write而不是read
    NULL
};
epicsExportAddress(dset, devAoBPMMonitor);
```

---

## 🔑 核心数据结构: DevPvt

### 定义

```c
typedef struct {
    int offset;           // Offset类型（对应ReadData的第1个参数）
    int channel;          // 通道号（对应ReadData的第2个参数）
    int type;             // 数据类型（对应ReadData的第3个参数）
    IOSCANPVT ioscanpvt;  // I/O中断扫描结构
} DevPvt;
```

### 为什么需要DevPvt？

**问题**: Record需要知道如何调用驱动层函数，但Record是通用的，不知道具体的参数。

**解决**: 在初始化时，将解析的参数保存在DevPvt中，读取时直接使用。

**生命周期**:
```
init_record_ai()
  ↓
  malloc DevPvt
  ↓
  解析INP字段 → 填充DevPvt
  ↓
  prec->dpvt = pPvt  // 保存指针
  ↓
Record运行时
  ↓
  read_ai()
    ↓
    DevPvt *pPvt = prec->dpvt  // 获取指针
    ↓
    使用pPvt中的参数调用驱动
```

---

## 📝 示例: 完整的ai Record支持

### .db文件配置

```
record(ai, "BPM:01:RF3:Amp") {
    field(DTYP, "BPM Monitor")    # 选择设备支持
    field(INP,  "@0 3 0")         # 参数: offset=0, channel=3, type=0
    field(SCAN, "I/O Intr")       # 使用I/O中断扫描
    field(EGU,  "V")
    field(PREC, "3")
}
```

### init_record_ai实现

```c
static long init_record_ai(aiRecord *prec)
{
    struct link *plink = &prec->inp;

    // 检查INP类型
    if (plink->type != INST_IO) {
        recGblRecordError(S_db_badField, prec, "Bad INP field");
        return S_db_badField;
    }

    // 分配DevPvt
    DevPvt *pPvt = malloc(sizeof(DevPvt));
    if (!pPvt) {
        return S_dev_noMemory;
    }

    // 解析INP字段 "@0 3 0"
    int nvals = sscanf(plink->value.instio.string, "@%d %d %d",
                       &pPvt->offset, &pPvt->channel, &pPvt->type);
    if (nvals != 3) {
        free(pPvt);
        recGblRecordError(S_db_badField, prec, "Invalid INP format");
        return S_db_badField;
    }

    // 初始化I/O中断扫描
    scanIoInit(&pPvt->ioscanpvt);

    // 保存到Record
    prec->dpvt = pPvt;

    return 0;
}
```

### read_ai实现

```c
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 调用驱动层ReadData
    float value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 更新Record值
    prec->val = value;
    prec->udf = 0;  // 标记数据有效

    return 0;
}
```

### get_ioint_info实现

```c
static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 返回I/O中断扫描结构
    *ppvt = pPvt->ioscanpvt;

    return 0;
}
```

---

## 🔗 与其他层的接口

### 与数据库层的接口

```c
// 数据库层通过DTYP选择设备支持
record(ai, "PV_NAME") {
    field(DTYP, "BPM Monitor")  // → 查找devAiBPMMonitor
}

// EPICS自动调用DSET中的函数
init_record_ai(prec);  // 初始化时
read_ai(prec);         // 处理时
```

### 与驱动层的接口

```c
// 设备支持调用驱动层提供的API
float ReadData(int offset, int channel, int type);
int SetReg(int addr, int value);
int ReadWaveform(int type, int channel, float *buffer, int size);

// 驱动层触发I/O中断
scanIoRequest(TriginScanPvt);
```

---

## ✅ 学习检查点

完成本文后，你应该能回答：

- [ ] 设备支持层在三层架构中起什么作用？
- [ ] DSET是什么？包含哪些关键函数？
- [ ] DevPvt结构的作用是什么？
- [ ] init_record在什么时候被调用？做什么工作？
- [ ] read/write函数如何调用驱动层？
- [ ] I/O中断扫描如何工作？

---

## 🎯 下一步

继续深入学习设备支持层：

- **[02-dset-structure.md](./02-dset-structure.md)** - DSET结构详解
- **[03-init-record.md](./03-init-record.md)** - init_record实现
- **[04-read-write.md](./04-read-write.md)** - read/write函数详解
- **[05-devpvt.md](./05-devpvt.md)** - DevPvt管理

---

**重要**: 设备支持层是理解EPICS的关键！花时间理解DSET机制会让后续学习更轻松。💡
