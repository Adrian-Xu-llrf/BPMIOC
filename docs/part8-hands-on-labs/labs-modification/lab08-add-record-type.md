# Lab 8: 添加新Record类型支持

> **难度**: ⭐⭐⭐
> **时间**: 3小时
> **前置**: Lab 6-7, Part 5

## 🎯 实验目标

学会如何在设备支持层添加对新Record类型的支持（以stringin Record为例）。

---

## 📚 背景知识

### DSET机制

每种Record类型需要对应的DSET（Device Support Entry Table）：

```c
// ai Record的DSET
typedef struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;      // ⭐ 核心
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;             // ⭐ 核心
    DEVSUPFUN special_linconv;
} aiDset;

// stringin Record的DSET
typedef struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;      // ⭐ 核心
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;             // ⭐ 核心
} stringinDset;
```

---

## 🔧 实验任务

### 任务: 添加stringin Record支持（读取版本字符串）

---

### 步骤1: 在devBPMMonitor.c添加stringin函数

```c
#include <stringinRecord.h>

// init_record for stringin
static long init_record_stringin(stringinRecord *prec)
{
    struct link *plink = &prec->inp;
    DevPvt *pPvt;

    if (plink->type != INST_IO) {
        recGblRecordError(S_db_badField, (void *)prec,
                         "init_record_stringin: Illegal INP field");
        return S_db_badField;
    }

    pPvt = (DevPvt *)malloc(sizeof(DevPvt));
    if (!pPvt) return S_dev_noMemory;

    // 解析INP字段
    char *param = plink->value.instio.string;
    sscanf(param, "@%*[^:]:%d", &pPvt->offset);

    // 保存到record
    prec->dpvt = pPvt;

    return 0;
}

// read function for stringin
static long read_stringin(stringinRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 根据offset读取不同字符串
    switch (pPvt->offset) {
        case 100:  // 版本信息
            strcpy(prec->val, "BPMIOC v1.0.0");
            break;
        case 101:  // FPGA版本
            strcpy(prec->val, "FPGA_2024.11");
            break;
        default:
            strcpy(prec->val, "Unknown");
            break;
    }

    prec->udf = 0;
    return 0;
}

// DSET定义
stringinDset devStringinBPMmonitor = {
    6,                          // number of functions
    NULL,                       // report
    NULL,                       // init
    init_record_stringin,       // init_record
    get_ioint_info,             // get_ioint_info (共用)
    read_stringin,              // read
};

epicsExportAddress(dset, devStringinBPMmonitor);
```

---

### 步骤2: 在.dbd文件中注册

**编辑BPMmonitorApp/src/BPMmonitor.dbd**:

```
device(stringin, INST_IO, devStringinBPMmonitor, "BPMmonitor")
```

---

### 步骤3: 在.db文件中添加stringin Record

```
record(stringin, "$(P):Version")
{
    field(DESC, "IOC Version")
    field(DTYP, "BPMmonitor")
    field(INP,  "@VER:100")
    field(SCAN, "Passive")
    field(PINI, "YES")
}

record(stringin, "$(P):FPGA_Ver")
{
    field(DESC, "FPGA Version")
    field(DTYP, "BPMmonitor")
    field(INP,  "@VER:101")
    field(SCAN, "Passive")
    field(PINI, "YES")
}
```

---

### 步骤4: 编译和测试

```bash
cd /home/user/BPMIOC
make clean && make
cd iocBoot/iocBPMmonitor
./st.cmd
```

**测试**:
```bash
caget iLinac_007:BPM14And15:Version
# 输出：iLinac_007:BPM14And15:Version BPMIOC v1.0.0

caget iLinac_007:BPM14And15:FPGA_Ver
# 输出：iLinac_007:BPM14And15:FPGA_Ver FPGA_2024.11
```

---

## 🚀 扩展挑战

### 挑战1: 添加longout支持（寄存器写入）

实现longout Record支持，用于写入32位寄存器。

### 挑战2: 添加waveform支持

实现waveform Record支持，读取ADC波形数据。

---

## 📚 相关知识

- Part 5.2: DSET结构详解
- Part 5.3: init_record实现
- Lab 6: 添加新PV

---

**恭喜完成Lab 8！** 你已学会添加新Record类型支持！💪
