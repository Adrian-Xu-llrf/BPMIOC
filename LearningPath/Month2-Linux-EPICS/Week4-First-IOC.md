# Week 4 - 创建第一个 IOC

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 从零创建一个简单的 BPM IOC

---

## Day 1-2: IOC 项目结构（4小时）

### 使用 makeBaseApp 创建项目

```bash
$ cd ~/epics
$ mkdir -p support/BPMMonitor
$ cd support/BPMMonitor

# 创建应用
$ makeBaseApp.pl -t ioc BPMMonitor
$ makeBaseApp.pl -i -t ioc BPMMonitor
```

### 项目结构

```
BPMMonitor/
├── configure/
│   ├── CONFIG
│   ├── Makefile
│   └── RULES
├── BPMMonitorApp/
│   ├── Db/          # 数据库文件
│   ├── src/         # 源代码
│   └── Makefile
├── iocBoot/
│   └── iocBPMMonitor/
│       └── st.cmd   # 启动脚本
└── Makefile
```

---

## Day 3-4: 创建数据库文件（4小时）

### 编写 BPM.db

在 `BPMMonitorApp/Db/` 创建 `BPM.db`：

```
# BPM 幅度记录
record(ai, "BPM:CH0:Amp") {
    field(DESC, "Channel 0 Amplitude")
    field(SCAN, "1 second")
    field(EGU, "V")
    field(PREC, "3")
}

record(ai, "BPM:CH1:Amp") {
    field(DESC, "Channel 1 Amplitude")
    field(SCAN, "1 second")
    field(EGU, "V")
    field(PREC, "3")
}

record(ai, "BPM:CH2:Amp") {
    field(DESC, "Channel 2 Amplitude")
    field(SCAN, "1 second")
    field(EGU, "V")
    field(PREC, "3")
}

record(ai, "BPM:CH3:Amp") {
    field(DESC, "Channel 3 Amplitude")
    field(SCAN, "1 second")
    field(EGU, "V")
    field(PREC, "3")
}

# BPM 控制记录
record(ao, "BPM:Gain") {
    field(DESC, "BPM Gain")
    field(VAL, "1.0")
    field(EGU, "")
    field(PREC, "2")
}
```

### 修改 Makefile

在 `BPMMonitorApp/Db/Makefile` 中添加：

```makefile
DB += BPM.db
```

---

## Day 5-6: 配置和编译（4小时）

### 修改 st.cmd

在 `iocBoot/iocBPMMonitor/st.cmd`：

```bash
#!../../bin/linux-x86_64/BPMMonitor

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/BPMMonitor.dbd"
BPMMonitor_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadRecords("db/BPM.db")

cd "${TOP}/iocBoot/${IOC}"
iocInit
```

### 编译

```bash
$ cd ~/epics/support/BPMMonitor
$ make
```

### 运行 IOC

```bash
$ cd iocBoot/iocBPMMonitor
$ ./st.cmd
```

---

## Day 7: 测试和调试（2小时）

### 测试 PV

```bash
# 在另一个终端

# 读取所有通道
$ caget BPM:CH0:Amp BPM:CH1:Amp BPM:CH2:Amp BPM:CH3:Amp

# 设置增益
$ caput BPM:Gain 2.0

# 监控一个通道
$ camonitor BPM:CH0:Amp
```

### IOC Shell 命令

在 IOC 控制台中：

```
epics> dbl          # 列出所有 PV
epics> dbpr BPM:CH0:Amp 2   # 打印记录详情
epics> dbgf BPM:CH0:Amp     # 获取字段值
epics> dbpf BPM:CH0:Amp 0.5 # 设置字段值
```

---

## 综合练习

1. 创建自己的 IOC 项目
2. 添加 10 个 PV（不同类型）
3. 编译并运行
4. 使用 CA 工具测试所有 PV
5. 修改 PV 配置，重新加载

---

## 总结

本周你学会了：
- ✅ 使用 makeBaseApp 创建 IOC
- ✅ 编写数据库文件
- ✅ 配置启动脚本
- ✅ 编译和运行 IOC
- ✅ 使用 CA 工具测试

**下个月**: 深入 IOC 开发，学习 Device Support 和 Driver Support！
