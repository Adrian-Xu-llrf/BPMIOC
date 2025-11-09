# Tutorial 4: 从零构建温度监控IOC

> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 1-2周（20-30小时）
> **前置**: Tutorial 1-3, Part 1-8

## 📖 项目概述

### 目标

从零开始构建一个完整的温度监控IOC，支持4个温度传感器。

### 功能需求

1. **数据采集**:
   - 4个温度传感器（DS18B20）
   - 采样频率：5秒
   - 温度范围：-20°C ~ 100°C
   - 精度：0.1°C

2. **监控报警**:
   - 高温报警（>80°C）
   - 低温报警（<0°C）
   - 传感器故障检测

3. **数据处理**:
   - 温度平均值计算
   - 最高/最低温度记录
   - 温度趋势分析（可选）

---

## 🏗️ 系统架构

```
DS18B20传感器（4个）
    ↓
1-Wire总线
    ↓
驱动层（driverTemp.c）
  ├─ ReadTemperature()
  ├─ DetectSensors()
  └─ CheckSensorHealth()
    ↓
设备支持层（devTemp.c）
  ├─ init_record_ai()
  └─ read_ai()
    ↓
数据库层（temp.db）
  ├─ ai Record（温度）
  ├─ calc Record（平均值）
  └─ ai Record（最高/最低）
    ↓
Channel Access
```

---

## 🚀 完整实现流程

### 第1步: 创建项目结构

```bash
# 创建IOC应用
mkdir -p tempMonitorApp/{src,Db}
mkdir -p iocBoot/iocTemp

# 创建configure/目录
cp -r /opt/epics/base/templates/makeBaseApp/top/configure .
```

---

### 第2步: 实现驱动层

**tempMonitorApp/src/driverTemp.c**:

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <epicsExport.h>
#include <iocsh.h>

// 温度传感器配置
#define MAX_SENSORS 4
static char *sensor_ids[MAX_SENSORS] = {
    "28-000000001234",
    "28-000000001235",
    "28-000000001236",
    "28-000000001237"
};

/**
 * @brief 读取DS18B20温度传感器
 * @param sensor_id 传感器ID
 * @return 温度值（°C）
 */
float ReadDS18B20(const char *sensor_id)
{
    char path[256];
    FILE *fp;
    float temp;
    
    // 真实硬件：读取/sys/bus/w1/devices/下的文件
    snprintf(path, sizeof(path), 
             "/sys/bus/w1/devices/%s/w1_slave", sensor_id);
    
    fp = fopen(path, "r");
    if (!fp) {
        // Mock实现：生成模拟数据
        return 20.0 + (rand() % 200 - 100) / 10.0;  // 10-30°C
    }
    
    // 解析w1_slave文件（示例）
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "t=")) {
            sscanf(strstr(line, "t=") + 2, "%f", &temp);
            temp /= 1000.0;  // 转换为°C
            break;
        }
    }
    
    fclose(fp);
    return temp;
}

/**
 * @brief 读取温度（IOC接口）
 */
float ReadTemperature(int channel)
{
    if (channel < 0 || channel >= MAX_SENSORS) {
        printf("ERROR: Invalid sensor channel %d\n", channel);
        return -999.0;
    }
    
    return ReadDS18B20(sensor_ids[channel]);
}

/**
 * @brief 驱动初始化
 */
static int drvTempInit(void)
{
    printf("Temperature Monitor Driver initialized\n");
    printf("Supported sensors: %d\n", MAX_SENSORS);
    return 0;
}

// IOC Shell注册
static const iocshFuncDef drvTempInitDef = {"drvTempInit", 0, NULL};
static void drvTempInitCall(const iocshArgBuf *args)
{
    drvTempInit();
}

void drvTempRegister(void)
{
    iocshRegister(&drvTempInitDef, drvTempInitCall);
}

epicsExportRegistrar(drvTempRegister);
```

---

### 第3步: 实现设备支持层

**tempMonitorApp/src/devTemp.c**:

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aiRecord.h>
#include <devSup.h>
#include <epicsExport.h>

// 外部驱动函数
extern float ReadTemperature(int channel);

typedef struct {
    int channel;
} TempDevPvt;

static long init_record_ai(aiRecord *prec)
{
    struct link *plink = &prec->inp;
    TempDevPvt *pPvt;
    
    if (plink->type != INST_IO) {
        recGblRecordError(S_db_badField, (void *)prec,
                         "devTemp: Illegal INP field");
        return S_db_badField;
    }
    
    pPvt = (TempDevPvt *)malloc(sizeof(TempDevPvt));
    if (!pPvt) return S_dev_noMemory;
    
    // 解析INP: "@TEMP:0"
    sscanf(plink->value.instio.string, "@TEMP:%d", &pPvt->channel);
    
    prec->dpvt = pPvt;
    return 0;
}

static long read_ai(aiRecord *prec)
{
    TempDevPvt *pPvt = (TempDevPvt *)prec->dpvt;
    
    prec->val = ReadTemperature(pPvt->channel);
    
    // 检查传感器故障（-999表示错误）
    if (prec->val < -900) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }
    
    prec->udf = 0;
    return 0;
}

aiDset devAiTemp = {
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai,
    NULL
};

epicsExportAddress(dset, devAiTemp);
```

---

### 第4步: 创建.dbd文件

**tempMonitorApp/src/devTemp.dbd**:

```
device(ai, INST_IO, devAiTemp, "Temperature")
registrar(drvTempRegister)
```

---

### 第5步: 配置Makefile

**tempMonitorApp/src/Makefile**:

```makefile
TOP=../..
include $(TOP)/configure/CONFIG

# IOC应用
PROD_IOC = tempMonitor

# .dbd文件
DBD += tempMonitor.dbd
tempMonitor_DBD += base.dbd
tempMonitor_DBD += devTemp.dbd

# 源文件
tempMonitor_SRCS += tempMonitor_registerRecordDeviceDriver.cpp
tempMonitor_SRCS_DEFAULT += tempMonitorMain.cpp
tempMonitor_SRCS += driverTemp.c
tempMonitor_SRCS += devTemp.c

# 系统库
tempMonitor_SYS_LIBS += m

# EPICS库
tempMonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
```

---

### 第6步: 创建数据库

**tempMonitorApp/Db/temp.db**:

```
#================================================
# 温度传感器
#================================================

record(ai, "$(P):Temp_Sensor_01")
{
    field(DESC, "Temperature Sensor 1")
    field(DTYP, "Temperature")
    field(INP,  "@TEMP:0")
    field(SCAN, "5 second")
    field(PREC, "1")
    field(EGU,  "C")
    field(HOPR, "100")
    field(LOPR, "-20")
    
    # 报警
    field(HIHI, "80")
    field(HIGH, "70")
    field(LOW,  "0")
    field(LOLO, "-10")
    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")
    
    field(PINI, "YES")
}

# 传感器2-4类似...
record(ai, "$(P):Temp_Sensor_02")
{
    field(DTYP, "Temperature")
    field(INP,  "@TEMP:1")
    field(SCAN, "5 second")
    field(PREC, "1")
    field(EGU,  "C")
    field(HIHI, "80")
    field(HHSV, "MAJOR")
    field(PINI, "YES")
}

#================================================
# 温度平均值
#================================================

record(calc, "$(P):Temp_Average")
{
    field(DESC, "Average Temperature")
    field(SCAN, "5 second")
    field(INPA, "$(P):Temp_Sensor_01 CP")
    field(INPB, "$(P):Temp_Sensor_02 CP")
    field(INPC, "$(P):Temp_Sensor_03 CP")
    field(INPD, "$(P):Temp_Sensor_04 CP")
    field(CALC, "(A+B+C+D)/4")
    field(PREC, "2")
    field(EGU,  "C")
}

#================================================
# 最高温度
#================================================

record(calc, "$(P):Temp_Max")
{
    field(DESC, "Maximum Temperature")
    field(SCAN, "5 second")
    field(INPA, "$(P):Temp_Sensor_01 CP")
    field(INPB, "$(P):Temp_Sensor_02 CP")
    field(INPC, "$(P):Temp_Sensor_03 CP")
    field(INPD, "$(P):Temp_Sensor_04 CP")
    field(CALC, "MAX(MAX(A,B),MAX(C,D))")
    field(PREC, "1")
    field(EGU,  "C")
}
```

---

### 第7步: 创建启动脚本

**iocBoot/iocTemp/st.cmd**:

```bash
#!../../bin/linux-x86_64/tempMonitor

< envPaths

dbLoadDatabase("../../dbd/tempMonitor.dbd")
tempMonitor_registerRecordDeviceDriver(pdbbase)

# 初始化驱动
drvTempInit

# 加载数据库
dbLoadRecords("../../db/temp.db", "P=LAB:TEMP")

iocInit()

# 显示所有PV
dbl
```

---

### 第8步: 编译和测试

```bash
# 编译
cd tempMonitor
make

# 运行
cd iocBoot/iocTemp
chmod +x st.cmd
./st.cmd

# 测试（在另一个终端）
caget LAB:TEMP:Temp_Sensor_01
caget LAB:TEMP:Temp_Average
caget LAB:TEMP:Temp_Max

camonitor LAB:TEMP:Temp_Sensor_01
```

---

## 📊 数据可视化

**Python脚本（monitor.py）**:

```python
#!/usr/bin/env python3
import time
import matplotlib.pyplot as plt
from epics import caget
from collections import deque

# 数据缓冲
history = {i: deque(maxlen=100) for i in range(1, 5)}

plt.ion()
fig, ax = plt.subplots()

while True:
    # 读取温度
    for i in range(1, 5):
        temp = caget(f'LAB:TEMP:Temp_Sensor_0{i}')
        history[i].append(temp)
    
    # 绘图
    ax.clear()
    for i in range(1, 5):
        ax.plot(list(history[i]), label=f'Sensor {i}')
    
    ax.set_xlabel('Time (5s intervals)')
    ax.set_ylabel('Temperature (°C)')
    ax.set_title('Temperature Monitoring')
    ax.legend()
    ax.grid(True)
    
    plt.pause(5)
```

---

## ✅ 学习成果

完成本教程后，你已经掌握了：

1. **完整IOC开发流程**:
   - 项目结构规划
   - 驱动层实现
   - 设备支持实现
   - 数据库配置
   - 启动脚本编写

2. **实际硬件接口**:
   - DS18B20传感器读取
   - 1-Wire总线操作
   - 错误处理

3. **数据处理**:
   - 多传感器数据聚合
   - calc Record高级用法
   - 实时数据可视化

4. **完整项目经验**:
   - 从零到部署
   - 测试验证
   - 问题排查

---

## 🚀 扩展挑战

1. **添加数据记录**: 使用autosave保存温度历史
2. **Web界面**: 创建简单的Web监控界面
3. **报警通知**: 温度超限时发送邮件/短信
4. **趋势分析**: 实现温度变化率监控

**下一个教程**: Tutorial 5 - 构建电源控制IOC，学习读写控制！
