# Tutorial 5: 构建电源控制IOC

> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 1-2周（25-35小时）
> **前置**: Tutorial 1-4, Part 1-8

## 📖 项目概述

### 目标

构建一个完整的可编程电源控制IOC，支持电压/电流读写、限流保护、报警等功能。

### 功能需求

1. **监控功能**:
   - 输出电压监控（Readback）
   - 输出电流监控
   - 电源状态（开/关）
   - 故障状态

2. **控制功能**:
   - 电压设定（Setpoint）
   - 电流限制设定
   - 电源开关控制
   - 紧急关闭

3. **保护功能**:
   - 过压保护（OVP）
   - 过流保护（OCP）
   - 限流模式检测
   - 互锁检测

---

## 🏗️ 系统架构

```
可编程电源（串口/以太网）
    ↓
通信协议（SCPI）
    ↓
驱动层
  ├─ ReadVoltage/Current()
  ├─ SetVoltage/Current()
  ├─ PowerOn/Off()
  └─ GetStatus()
    ↓
设备支持层
  ├─ ai/ao/bi/bo Record支持
  └─ 异步I/O（可选）
    ↓
数据库层
  ├─ 监控PV（ai/bi）
  ├─ 控制PV（ao/bo）
  ├─ 保护逻辑（calc）
  └─ 状态管理（stringin）
    ↓
Channel Access / OPI
```

---

## 🚀 完整实现

### 第1步: 驱动层实现

**powerSupplyApp/src/driverPower.c**:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <epicsExport.h>
#include <epicsMutex.h>

// 电源状态
typedef struct {
    float voltage_set;      // 电压设定值
    float voltage_rbv;      // 电压读回值
    float current_set;      // 电流限制
    float current_rbv;      // 电流读回值
    int   power_on;         // 电源状态（0=关，1=开）
    int   fault;            // 故障标志
} PowerSupplyState;

static PowerSupplyState g_ps_state = {0};
static epicsMutexId g_mutex;
static int g_serial_fd = -1;

/**
 * @brief 发送SCPI命令
 */
static int SendCommand(const char *cmd)
{
    if (g_serial_fd < 0) return -1;
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\n", cmd);
    
    epicsMutexLock(g_mutex);
    int ret = write(g_serial_fd, buffer, strlen(buffer));
    epicsMutexUnlock(g_mutex);
    
    return (ret > 0) ? 0 : -1;
}

/**
 * @brief 查询SCPI命令
 */
static int QueryCommand(const char *cmd, char *response, size_t maxlen)
{
    if (g_serial_fd < 0) return -1;
    
    SendCommand(cmd);
    
    epicsMutexLock(g_mutex);
    usleep(50000);  // 等待50ms
    int len = read(g_serial_fd, response, maxlen - 1);
    epicsMutexUnlock(g_mutex);
    
    if (len > 0) {
        response[len] = '\0';
        return 0;
    }
    return -1;
}

/**
 * @brief 读取输出电压
 */
float ReadVoltage(void)
{
    char response[64];
    
    if (QueryCommand("MEAS:VOLT?", response, sizeof(response)) == 0) {
        return atof(response);
    }
    
    // Mock数据
    return g_ps_state.voltage_set + (rand() % 20 - 10) / 100.0;
}

/**
 * @brief 读取输出电流
 */
float ReadCurrent(void)
{
    char response[64];
    
    if (QueryCommand("MEAS:CURR?", response, sizeof(response)) == 0) {
        return atof(response);
    }
    
    // Mock数据
    return (rand() % 1000) / 1000.0;  // 0-1A
}

/**
 * @brief 设置输出电压
 */
int SetVoltage(float voltage)
{
    char cmd[64];
    
    if (voltage < 0 || voltage > 30.0) {
        printf("ERROR: Voltage out of range (0-30V)\n");
        return -1;
    }
    
    snprintf(cmd, sizeof(cmd), "SOUR:VOLT %.3f", voltage);
    
    if (SendCommand(cmd) == 0) {
        g_ps_state.voltage_set = voltage;
        return 0;
    }
    
    return -1;
}

/**
 * @brief 设置电流限制
 */
int SetCurrentLimit(float current)
{
    char cmd[64];
    
    if (current < 0 || current > 5.0) {
        printf("ERROR: Current out of range (0-5A)\n");
        return -1;
    }
    
    snprintf(cmd, sizeof(cmd), "SOUR:CURR %.3f", current);
    
    if (SendCommand(cmd) == 0) {
        g_ps_state.current_set = current;
        return 0;
    }
    
    return -1;
}

/**
 * @brief 电源开关控制
 */
int PowerControl(int on)
{
    const char *cmd = on ? "OUTP ON" : "OUTP OFF";
    
    if (SendCommand(cmd) == 0) {
        g_ps_state.power_on = on;
        printf("Power supply %s\n", on ? "ON" : "OFF");
        return 0;
    }
    
    return -1;
}

/**
 * @brief 获取电源状态
 */
int GetPowerStatus(void)
{
    char response[64];
    
    if (QueryCommand("OUTP?", response, sizeof(response)) == 0) {
        return (response[0] == '1') ? 1 : 0;
    }
    
    return g_ps_state.power_on;
}

/**
 * @brief 初始化电源
 */
static int drvPowerInit(const char *device)
{
    printf("Initializing power supply on %s\n", device);
    
    g_mutex = epicsMutexCreate();
    
    // 打开串口（示例）
    g_serial_fd = open(device, O_RDWR | O_NOCTTY);
    if (g_serial_fd < 0) {
        printf("WARNING: Cannot open %s, using mock mode\n", device);
        return 0;  // Mock模式
    }
    
    // 配置串口
    struct termios tty;
    tcgetattr(g_serial_fd, &tty);
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag |= (CLOCAL | CREAD);
    tcsetattr(g_serial_fd, TCSANOW, &tty);
    
    // 发送复位命令
    SendCommand("*RST");
    sleep(1);
    
    printf("Power supply initialized\n");
    return 0;
}

// IOC Shell注册
#include <iocsh.h>

static const iocshArg drvPowerInitArg0 = {"device", iocshArgString};
static const iocshArg *drvPowerInitArgs[] = {&drvPowerInitArg0};
static const iocshFuncDef drvPowerInitDef = {
    "drvPowerInit", 1, drvPowerInitArgs
};

static void drvPowerInitCall(const iocshArgBuf *args)
{
    drvPowerInit(args[0].sval);
}

void drvPowerRegister(void)
{
    iocshRegister(&drvPowerInitDef, drvPowerInitCall);
}

epicsExportRegistrar(drvPowerRegister);
```

---

### 第2步: 设备支持层

**powerSupplyApp/src/devPower.c**:

```c
// ai Record支持（读取电压/电流）
static long read_ai(aiRecord *prec)
{
    PowerDevPvt *pPvt = (PowerDevPvt *)prec->dpvt;
    
    switch (pPvt->type) {
        case TYPE_VOLTAGE:
            prec->val = ReadVoltage();
            break;
        case TYPE_CURRENT:
            prec->val = ReadCurrent();
            break;
        default:
            return -1;
    }
    
    prec->udf = 0;
    return 0;
}

// ao Record支持（设置电压/电流）
static long write_ao(aoRecord *prec)
{
    PowerDevPvt *pPvt = (PowerDevPvt *)prec->dpvt;
    int ret = 0;
    
    switch (pPvt->type) {
        case TYPE_VOLTAGE:
            ret = SetVoltage(prec->val);
            break;
        case TYPE_CURRENT:
            ret = SetCurrentLimit(prec->val);
            break;
        default:
            return -1;
    }
    
    return ret;
}

// bo Record支持（电源开关）
static long write_bo(boRecord *prec)
{
    return PowerControl(prec->val);
}

// bi Record支持（状态读取）
static long read_bi(biRecord *prec)
{
    prec->rval = GetPowerStatus();
    prec->udf = 0;
    return 0;
}
```

---

### 第3步: 数据库配置

**powerSupplyApp/Db/power.db**:

```
#================================================
# 电压监控和控制
#================================================

# 电压设定
record(ao, "$(P):Volt:Set")
{
    field(DESC, "Voltage Setpoint")
    field(DTYP, "Power")
    field(OUT,  "@VOLT:SET")
    field(PREC, "3")
    field(EGU,  "V")
    field(DRVH, "30")      # 最大30V
    field(DRVL, "0")       # 最小0V
    field(DOL,  "0")       # 初始值0V
    field(PINI, "YES")
}

# 电压读回
record(ai, "$(P):Volt:Rbv")
{
    field(DESC, "Voltage Readback")
    field(DTYP, "Power")
    field(INP,  "@VOLT:RBV")
    field(SCAN, ".5 second")
    field(PREC, "3")
    field(EGU,  "V")
    
    # 过压报警
    field(HIHI, "31")
    field(HHSV, "MAJOR")
}

#================================================
# 电流监控和控制
#================================================

record(ao, "$(P):Curr:Limit")
{
    field(DESC, "Current Limit")
    field(DTYP, "Power")
    field(OUT,  "@CURR:SET")
    field(PREC, "3")
    field(EGU,  "A")
    field(DRVH, "5")
    field(DRVL, "0")
    field(DOL,  "1")       # 默认1A限流
    field(PINI, "YES")
}

record(ai, "$(P):Curr:Rbv")
{
    field(DESC, "Current Readback")
    field(DTYP, "Power")
    field(INP,  "@CURR:RBV")
    field(SCAN, ".5 second")
    field(PREC, "3")
    field(EGU,  "A")
    
    # 过流报警
    field(HIHI, "4.5")
    field(HHSV, "MAJOR")
}

#================================================
# 电源开关控制
#================================================

record(bo, "$(P):Power:Switch")
{
    field(DESC, "Power On/Off")
    field(DTYP, "Power")
    field(OUT,  "@POWER:CTRL")
    field(ZNAM, "OFF")
    field(ONAM, "ON")
    field(DOL,  "0")       # 初始关闭
    field(PINI, "YES")
}

record(bi, "$(P):Power:Status")
{
    field(DESC, "Power Status")
    field(DTYP, "Power")
    field(INP,  "@POWER:STATUS")
    field(SCAN, "1 second")
    field(ZNAM, "OFF")
    field(ONAM, "ON")
}

#================================================
# 保护逻辑
#================================================

# 限流模式检测
record(calc, "$(P):ConstantCurrent")
{
    field(DESC, "CC Mode Detection")
    field(SCAN, "1 second")
    field(INPA, "$(P):Curr:Rbv CP")
    field(INPB, "$(P):Curr:Limit CP")
    field(CALC, "(ABS(A-B)<0.1)?1:0")  # 电流接近限值
    field(ZNAM, "CV")
    field(ONAM, "CC")
}

# 紧急关闭
record(calcout, "$(P):EmergencyShutdown")
{
    field(DESC, "Emergency Shutdown")
    field(SCAN, ".5 second")
    field(INPA, "$(P):Volt:Rbv CP")
    field(INPB, "$(P):Curr:Rbv CP")
    field(CALC, "(A>31)||(B>5)?1:0")
    field(OOPT, "When Non-zero")
    field(OUT,  "$(P):Power:Switch PP")
    field(DOPT, "Use CALC")
}
```

---

### 第4步: 测试

```bash
# 启动IOC
./st.cmd

# 测试（另一终端）

# 1. 设置电压
caput LAB:PS:Volt:Set 12.5

# 2. 设置电流限制
caput LAB:PS:Curr:Limit 2.0

# 3. 打开电源
caput LAB:PS:Power:Switch 1

# 4. 监控输出
camonitor LAB:PS:Volt:Rbv LAB:PS:Curr:Rbv

# 5. 检查限流模式
caget LAB:PS:ConstantCurrent

# 6. 关闭电源
caput LAB:PS:Power:Switch 0
```

---

## 📊 OPI界面（CSS/BOY）

```xml
<!-- power_supply.opi -->
<display>
  <widget type="label">
    <text>Power Supply Control</text>
  </widget>
  
  <widget type="textupdate">
    <pv_name>LAB:PS:Volt:Rbv</pv_name>
    <format>%.3f V</format>
  </widget>
  
  <widget type="textentry">
    <pv_name>LAB:PS:Volt:Set</pv_name>
    <limits_from_pv>true</limits_from_pv>
  </widget>
  
  <widget type="actionbutton">
    <pv_name>LAB:PS:Power:Switch</pv_name>
    <text>Power ON/OFF</text>
  </widget>
</display>
```

---

## ✅ 学习成果

完成本教程后，你已经完全掌握：

1. **完整IOC开发**:
   - 从需求到部署全流程
   - 读写控制的完整实现
   - 保护逻辑设计

2. **硬件通信**:
   - 串口通信
   - SCPI协议
   - 异步I/O（可选）

3. **系统设计**:
   - 分层架构
   - 错误处理
   - 保护机制

4. **项目管理**:
   - 代码组织
   - 测试方法
   - 文档编写

---

## 🎉 Part 9完成

**恭喜！** 你已经完成了Part 9的全部5个教程！

现在你已经能够：
✅ 修改现有IOC（Tutorial 1-3）
✅ 从零构建新IOC（Tutorial 4-5）
✅ 处理各种实际应用场景
✅ 独立开发EPICS IOC项目

**继续学习**: Part 10 - 调试与测试，或开始你的实际项目！💪
