# 03 - 创建你的第一个IOC

> **目标**: 从零创建一个最简单的EPICS IOC
> **难度**: ⭐⭐☆☆☆
> **预计时间**: 30分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 使用makeBaseApp.pl创建IOC模板
- ✅ 理解IOC的基本目录结构
- ✅ 编写简单的数据库文件
- ✅ 运行并测试IOC
- ✅ 理解IOC启动流程

## 🎯 为什么要自己创建IOC

在学习BPMIOC之前，先创建一个简单IOC可以帮助你：
- 理解IOC的基本组成部分
- 熟悉EPICS的构建系统
- 掌握数据库文件的基本语法
- 理解启动脚本的作用

## 📁 IOC项目结构

一个标准的EPICS IOC项目包含：

```
myioc/
├── configure/          # 配置文件
│   ├── CONFIG         # 构建配置
│   └── RELEASE        # 依赖声明
├── myiocApp/          # 应用程序目录
│   ├── src/           # 源代码
│   │   └── Makefile
│   └── Db/            # 数据库文件
│       └── myioc.db
├── iocBoot/           # 启动脚本
│   └── iocmyioc/
│       └── st.cmd     # 启动命令
└── Makefile           # 顶层Makefile
```

## 🚀 步骤1: 创建IOC模板

### 1.1 使用makeBaseApp.pl

EPICS Base提供了工具自动生成IOC模板：

```bash
# 创建项目目录
cd ~
mkdir myioc
cd myioc

# 生成IOC应用模板
makeBaseApp.pl -t ioc myioc

# 生成启动脚本
makeBaseApp.pl -i -t ioc myioc
```

**交互提示时的输入**:
```
What application should the IOC(s) boot?
The default uses the IOC's name, even if not listed above.
Application name? myioc
```

### 1.2 配置EPICS_BASE路径

编辑 `configure/RELEASE`:

```bash
vim configure/RELEASE
```

确保EPICS_BASE路径正确：
```makefile
# configure/RELEASE
EPICS_BASE=/home/yourname/base-3.15.6
```

## 📝 步骤2: 创建简单的数据库

### 2.1 创建数据库文件

```bash
vim myiocApp/Db/myioc.db
```

添加以下内容：

```
# myioc.db - 我的第一个数据库

# 1. 模拟温度传感器（ai记录）
record(ai, "MY:Temperature") {
    field(DESC, "Temperature Sensor")
    field(SCAN, "1 second")      # 每秒扫描一次
    field(INP,  "0")              # 输入值（这里简化为常数）
    field(EGU,  "Celsius")        # 工程单位
    field(PREC, "2")              # 显示精度（小数点后2位）
    field(HIHI, "100")            # 高高报警
    field(HIGH, "80")             # 高报警
    field(LOW,  "10")             # 低报警
    field(LOLO, "0")              # 低低报警
}

# 2. 计数器（calc记录）
record(calc, "MY:Counter") {
    field(DESC, "Simple Counter")
    field(SCAN, "1 second")
    field(CALC, "(A<100)?(A+1):0")  # 如果A<100则加1，否则归零
    field(INPA, "MY:Counter")        # 引用自己
    field(PREC, "0")
}

# 3. 手动设置值（ao记录）
record(ao, "MY:SetPoint") {
    field(DESC, "Manual Setpoint")
    field(EGU,  "Volts")
    field(PREC, "3")
    field(DRVH, "10")             # 驱动高限
    field(DRVL, "0")              # 驱动低限
}

# 4. 字符串记录（stringin）
record(stringin, "MY:Status") {
    field(DESC, "Status Message")
    field(VAL,  "Ready")          # 初始值
}

# 5. 二进制输出（bo记录）
record(bo, "MY:Enable") {
    field(DESC, "Enable Switch")
    field(ZNAM, "Disabled")       # 0时的名称
    field(ONAM, "Enabled")        # 1时的名称
}
```

### 2.2 理解记录类型

| 记录类型 | 全称 | 用途 | 典型应用 |
|---------|------|------|----------|
| `ai` | Analog Input | 模拟量输入 | 读取传感器值 |
| `ao` | Analog Output | 模拟量输出 | 设置控制值 |
| `bi` | Binary Input | 二进制输入 | 读取开关状态 |
| `bo` | Binary Output | 二进制输出 | 控制开关 |
| `calc` | Calculation | 计算 | 数学运算 |
| `stringin` | String Input | 字符串输入 | 文本信息 |

### 2.3 理解常用字段

| 字段 | 含义 | 示例 |
|------|------|------|
| `DESC` | 描述 | "Temperature Sensor" |
| `SCAN` | 扫描周期 | "1 second", "Passive", "I/O Intr" |
| `VAL` | 值 | 当前数值或字符串 |
| `EGU` | 工程单位 | "Celsius", "Volts", "mA" |
| `PREC` | 显示精度 | "2" (小数点后2位) |
| `HIHI/HIGH/LOW/LOLO` | 报警限值 | 高高/高/低/低低报警 |
| `INP` | 输入链接 | 从哪里获取数据 |

## 🔧 步骤3: 配置Makefile

### 3.1 编辑应用Makefile

```bash
vim myiocApp/src/Makefile
```

确保包含数据库文件：

```makefile
TOP=../..

include $(TOP)/configure/CONFIG

# 数据库文件
DB += myioc.db

# 构建IOC应用
PROD_IOC = myioc

# myioc.dbd将由以下文件生成
DBD += myioc.dbd

# myioc_registerRecordDeviceDriver.cpp来自myioc.dbd
myioc_SRCS += myioc_registerRecordDeviceDriver.cpp

# 构建主程序
myioc_SRCS_DEFAULT += myiocMain.cpp
myioc_SRCS_vxWorks += -nil-

# 链接EPICS Base库
myioc_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
```

### 3.2 编辑DBD文件

```bash
vim myiocApp/src/myioc.dbd
```

内容：

```
include "base.dbd"
```

这会包含EPICS Base提供的所有标准记录类型。

## 🏗️ 步骤4: 编译IOC

```bash
cd ~/myioc
make clean
make -j$(nproc)
```

**预期输出**:
```
...
make[1]: Entering directory '/home/yourname/myioc/myiocApp/src'
...
make[1]: Leaving directory '/home/yourname/myioc/iocBoot'
```

**验证编译结果**:
```bash
ls -lh bin/*/myioc
ls -lh db/myioc.db
ls -lh dbd/myioc.dbd
```

应该看到：
- `bin/linux-x86_64/myioc` - IOC可执行文件
- `db/myioc.db` - 数据库文件
- `dbd/myioc.dbd` - 数据库定义

## 🚀 步骤5: 编辑启动脚本

```bash
vim iocBoot/iocmyioc/st.cmd
```

修改为：

```bash
#!../../bin/linux-x86_64/myioc

< envPaths

cd "${TOP}"

## 注册所有支持组件
dbLoadDatabase "dbd/myioc.dbd"
myioc_registerRecordDeviceDriver pdbbase

## 加载记录实例
dbLoadRecords "db/myioc.db"

cd "${TOP}/iocBoot/${IOC}"
iocInit

## 启动完成后的操作
dbl   # 列出所有PV
```

**使脚本可执行**:
```bash
chmod +x iocBoot/iocmyioc/st.cmd
```

## 🎮 步骤6: 运行IOC

### 6.1 启动IOC

```bash
cd iocBoot/iocmyioc
./st.cmd
```

**预期输出**:
```
#!../../bin/linux-x86_64/myioc
< envPaths
cd "/home/yourname/myioc"
dbLoadDatabase "dbd/myioc.dbd"
myioc_registerRecordDeviceDriver pdbbase
dbLoadRecords "db/myioc.db"
cd "/home/yourname/myioc/iocBoot/iocmyioc"
iocInit
Starting iocInit
############################################################################
## EPICS R3.15.6
## EPICS Base built Nov  8 2025
############################################################################
iocRun: All initialization complete
dbl
MY:Counter
MY:Enable
MY:SetPoint
MY:Status
MY:Temperature
epics>
```

### 6.2 在IOC Shell中测试

在 `epics>` 提示符下尝试：

```bash
# 查看所有PV
epics> dbl

# 查看某个PV的值
epics> dbgf "MY:Counter"
DBR_DOUBLE:         1

# 查看详细信息
epics> dbpr "MY:Temperature"
ASG:                DESC: Temperature Sensor     DISA: 0
DISP: 0             DISV: 1                 NAME: MY:Temperature
...

# 设置值
epics> dbpf "MY:SetPoint" "5.5"
DBR_DOUBLE:         5.5

# 设置字符串
epics> dbpf "MY:Status" "Running"
DBR_STRING:         Running
```

## 🔬 步骤7: 使用CA工具访问

**打开新终端窗口**，保持IOC运行：

### 7.1 读取PV

```bash
# 读取计数器（应该每秒增加）
caget MY:Counter
# MY:Counter                     15

# 读取温度
caget MY:Temperature
# MY:Temperature                 0

# 读取状态
caget MY:Status
# MY:Status                      Running
```

### 7.2 监控PV变化

```bash
# 监控计数器（Ctrl+C停止）
camonitor MY:Counter
# MY:Counter                     2025-11-08 10:30:45.123456 25
# MY:Counter                     2025-11-08 10:30:46.123456 26
# MY:Counter                     2025-11-08 10:30:47.123456 27
```

### 7.3 写入PV

```bash
# 设置Setpoint
caput MY:SetPoint 7.89
# Old : MY:SetPoint                     5.5
# New : MY:SetPoint                     7.89

# 验证
caget MY:SetPoint
# MY:SetPoint                    7.89

# 切换开关
caput MY:Enable 1
# Old : MY:Enable                       Disabled
# New : MY:Enable                       Enabled
```

### 7.4 用Python访问

```python
import epics
import time

# 读取值
counter = epics.caget('MY:Counter')
print(f"Counter: {counter}")

# 监控回调
def on_change(pvname=None, value=None, **kws):
    print(f"{pvname} changed to {value}")

pv = epics.PV('MY:Counter', callback=on_change)

# 等待10秒观察变化
time.sleep(10)

# 写入值
epics.caput('MY:SetPoint', 3.14)
```

## 📊 理解IOC启动流程

### 启动序列

```
1. 加载envPaths（环境路径）
   ↓
2. 切换到TOP目录
   ↓
3. dbLoadDatabase（加载数据库定义）
   ↓
4. registerRecordDeviceDriver（注册驱动）
   ↓
5. dbLoadRecords（加载记录实例）
   ↓
6. iocInit（初始化IOC）
   ↓
7. IOC运行（epics> 提示符）
```

### iocInit做了什么

```c
iocInit() {
    1. 初始化所有设备支持
    2. 初始化所有记录
    3. 解析记录间的链接
    4. 启动扫描任务
    5. 启动CA服务器
    6. 处理所有初始值
}
```

## 🔍 常见问题

### Q1: IOC启动报错 "undefined symbol"

**原因**: DBD文件不完整或Makefile配置错误

**解决**:
```bash
# 检查myioc.dbd是否包含base.dbd
cat myiocApp/src/myioc.dbd

# 重新编译
make clean && make
```

### Q2: caget找不到PV

**原因**: 可能是网络配置问题

**解决**:
```bash
# 检查CA地址列表
echo $EPICS_CA_ADDR_LIST
# 应该包含localhost或127.0.0.1

# 如果未设置，添加到~/.bashrc
export EPICS_CA_ADDR_LIST=localhost
export EPICS_CA_AUTO_ADDR_LIST=NO
```

### Q3: 计数器不增加

**原因**: SCAN字段设置错误或CALC表达式错误

**解决**:
```bash
# 检查记录状态
epics> dbpr "MY:Counter"
# 查看SCAN字段是否为"1 second"
# 查看CALC字段是否正确
```

## 🎯 验证理解

完成以下任务以验证你的理解：

1. **修改计数器**: 让它每2秒加1，从0数到50后归零
2. **添加新PV**: 创建一个温度设定点PV（ao类型）
3. **添加计算**: 创建一个calc记录计算华氏温度 = 摄氏温度 * 1.8 + 32
4. **测试报警**: 设置MY:Temperature超过80时触发HIGH报警

<details>
<summary>答案提示</summary>

**任务1** - 修改SCAN和CALC:
```
field(SCAN, "2 second")
field(CALC, "(A<50)?(A+1):0")
```

**任务2** - 添加ao记录:
```
record(ao, "MY:TempSetpoint") {
    field(DESC, "Temperature Setpoint")
    field(EGU,  "Celsius")
    field(PREC, "1")
}
```

**任务3** - 添加calc记录:
```
record(calc, "MY:TempF") {
    field(DESC, "Temperature in Fahrenheit")
    field(SCAN, "1 second")
    field(CALC, "A*1.8+32")
    field(INPA, "MY:Temperature CP")  # CP = Change Process
    field(EGU,  "Fahrenheit")
    field(PREC, "1")
}
```

**任务4** - 测试报警:
```bash
# 在IOC shell中
epics> dbpf "MY:Temperature" "85"
# 应该看到报警信息
```
</details>

## 🚀 下一步

现在你已经：
- ✅ 创建了第一个IOC
- ✅ 理解了IOC的基本结构
- ✅ 编写了数据库文件
- ✅ 使用CA工具访问PV

**接下来学习**:
- [04-record-types.md](./04-record-types.md) - 深入理解各种记录类型
- [05-scanning-basics.md](./05-scanning-basics.md) - 扫描机制详解
- [Part 8: lab01](../part8-hands-on-labs/labs-basic/lab01-trace-rf-amp.md) - 实际项目中的数据流

## 📚 参考资源

- [EPICS Application Developer's Guide](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/)
- [Record Reference Manual](https://epics.anl.gov/base/R3-15/6-docs/RecordReference.html)
- [Database Definition (DBD) Files](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/DatabaseDefinition.html)

---

**🎉 恭喜！** 你已经掌握了创建EPICS IOC的基础知识！
