# Tutorial 2: 实现新的Offset类型 - 波形数据采集

> **难度**: ⭐⭐⭐⭐
> **时间**: 1周（15-20小时）
> **前置**: Tutorial 1, Part 4-6

## 📖 项目概述

### 目标

实现一个新的offset类型用于采集RF波形数据，而不仅仅是单点幅度值。

### 需求

1. **功能需求**:
   - 支持采集1024点波形数据
   - 采样率可配置
   - 支持触发模式（内部/外部触发）
   - 波形数据通过waveform Record提供

2. **技术需求**:
   - 新增offset: OFFSET_WAVEFORM (30)
   - 实现ReadWaveform()驱动函数
   - 添加waveform设备支持
   - 支持Mock库模拟

---

## 🎯 学习目标

- ✅ 理解Offset系统的扩展机制
- ✅ 掌握waveform Record的使用
- ✅ 实现波形数据采集
- ✅ 处理大数据量的PV

---

## 🔧 实现步骤

### 步骤1: 定义新offset和数据结构

**driverWrapper.c**:

```c
#define OFFSET_WAVEFORM  30

// 波形数据缓冲区
#define WAVEFORM_SIZE 1024
static float g_waveform_buffer[WAVEFORM_SIZE];
static int g_waveform_ready = 0;
```

---

### 步骤2: 实现波形采集函数

```c
/**
 * @brief 采集RF波形数据
 * @param channel RF通道
 * @param buffer 输出缓冲区
 * @param npts 采样点数
 * @return 实际采集的点数
 */
int ReadWaveform(int channel, float *buffer, int npts)
{
    if (npts > WAVEFORM_SIZE) {
        npts = WAVEFORM_SIZE;
    }
    
    // 等待触发
    WaitForTrigger();
    
    // 采集数据
    for (int i = 0; i < npts; i++) {
        buffer[i] = BPM_RFIn_ReadADC_Fast(channel);
        // 假设采样间隔由硬件控制
    }
    
    return npts;
}
```

---

### 步骤3: 添加waveform设备支持

**devBPMMonitor.c**:

```c
#include <waveformRecord.h>

static long init_record_waveform(waveformRecord *prec)
{
    DevPvt *pPvt;
    
    if (prec->inp.type != INST_IO) return S_db_badField;
    
    pPvt = (DevPvt *)malloc(sizeof(DevPvt));
    // 解析INP字段: "@WAVEFORM:30 ch=0"
    sscanf(prec->inp.value.instio.string, "@%*[^:]:%d ch=%d",
           &pPvt->offset, &pPvt->channel);
    
    prec->dpvt = pPvt;
    return 0;
}

static long read_waveform(waveformRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    
    // 读取波形数据
    int npts = ReadWaveform(pPvt->channel, (float *)prec->bptr, prec->nelm);
    
    // 更新实际读取的点数
    prec->nord = npts;
    prec->udf = 0;
    
    return 0;
}

// DSET定义
waveformDset devWfBPMmonitor = {
    5,
    NULL,
    NULL,
    init_record_waveform,
    NULL,
    read_waveform
};

epicsExportAddress(dset, devWfBPMmonitor);
```

---

### 步骤4: 注册waveform设备支持

**devBPMMonitor.dbd**:

```
device(waveform, INST_IO, devWfBPMmonitor, "BPMmonitor")
```

---

### 步骤5: 添加waveform Record

**BPMMonitor.db**:

```
record(waveform, "$(P):RFIn_01_Waveform")
{
    field(DESC, "RF Input 1 Waveform")
    field(DTYP, "BPMmonitor")
    field(INP,  "@WAVEFORM:30 ch=0")
    field(SCAN, "1 second")
    field(FTVL, "FLOAT")        # 元素类型：浮点
    field(NELM, "1024")         # 最大1024点
    field(PINI, "YES")
}

# 触发控制Record
record(bo, "$(P):Waveform_Trigger")
{
    field(DESC, "Waveform Trigger")
    field(DTYP, "Soft Channel")
    field(OUT,  "$(P):RFIn_01_Waveform.PROC PP")
    field(ZNAM, "Stop")
    field(ONAM, "Start")
}
```

---

### 步骤6: 测试

```bash
# 读取波形数据
caget -a iLinac_007:BPM14And15:RFIn_01_Waveform

# 输出：
# iLinac_007:BPM14And15:RFIn_01_Waveform 1024 0.123 0.234 0.345 ...

# 触发采集
caput iLinac_007:BPM14And15:Waveform_Trigger 1
```

---

## 📊 Python可视化

```python
import matplotlib.pyplot as plt
from epics import caget

# 读取波形
waveform = caget('iLinac_007:BPM14And15:RFIn_01_Waveform')

# 绘图
plt.plot(waveform)
plt.title('RF Waveform')
plt.xlabel('Sample')
plt.ylabel('Amplitude (V)')
plt.grid(True)
plt.show()
```

---

## ✅ 学习成果

- ✅ 掌握了waveform Record的完整实现
- ✅ 理解了大数据量PV的处理
- ✅ 学会了数据可视化方法

**下一个教程**: Tutorial 3 - 添加新硬件通道！
