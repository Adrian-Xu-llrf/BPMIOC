# Lab 12: 自定义设备支持

> **难度**: ⭐⭐⭐⭐
> **时间**: 4小时  
> **前置**: Lab 8, Part 5

## 🎯 实验目标

从零编写一个完整的设备支持层（为虚拟温度传感器）。

---

## 🔧 实验任务

创建新文件`devTempSensor.c`，实现完整的ai Record设备支持。

---

### 完整代码框架

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <epicsExport.h>
#include <aiRecord.h>
#include <devSup.h>

typedef struct {
    int sensor_id;
} TempDevPvt;

// init_record
static long init_record_ai(aiRecord *prec)
{
    struct link *plink = &prec->inp;
    TempDevPvt *pPvt;

    if (plink->type != INST_IO) return S_db_badField;

    pPvt = (TempDevPvt *)malloc(sizeof(TempDevPvt));
    sscanf(plink->value.instio.string, "@sensor=%d", &pPvt->sensor_id);

    prec->dpvt = pPvt;
    return 0;
}

// read function
static long read_ai(aiRecord *prec)
{
    TempDevPvt *pPvt = (TempDevPvt *)prec->dpvt;

    // 模拟读取温度
    prec->val = 25.0 + pPvt->sensor_id * 5.0 + (rand() % 20 - 10) / 10.0;
    prec->udf = 0;

    return 0;
}

// DSET definition
aiDset devAiTempSensor = {
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai,
    NULL
};

epicsExportAddress(dset, devAiTempSensor);
```

---

### 使用新设备支持

**.dbd**:
```
device(ai, INST_IO, devAiTempSensor, "TempSensor")
```

**.db**:
```
record(ai, "$(P):Sensor_01")
{
    field(DTYP, "TempSensor")
    field(INP,  "@sensor=1")
    field(SCAN, "1 second")
}
```

---

**恭喜完成Lab 12！** 你已能从零实现设备支持！💪
