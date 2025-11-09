# Part 6.5: st.cmd启动脚本详解

> **目标**: 深入理解IOC启动脚本的编写和执行过程
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 75分钟

## 📖 什么是st.cmd？

**st.cmd** = **Startup Command**（启动命令脚本）

它是IOC的**启动配置文件**，定义了：
- 加载哪些数据库
- 初始化哪些驱动
- 设置运行时参数
- 启动序列程序

**位置**: `iocBoot/ioc<name>/st.cmd`

**执行**: IOC启动时自动执行（由shebang指定的IOC可执行文件）

---

## 🏗️ BPMIOC的st.cmd

### 完整内容

```bash
#!/home/user/BPMIOC/bin/linux-arm/BPMmonitor

## You may have to change BPMmonitor to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase("../../dbd/BPMmonitor.dbd",0,0)
BPMmonitor_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("../../db/BPMMonitor.db","P=iLinac_007:BPM14And15, P1=iLinac_007:BPM14, P2=iLinac_007:BPM15")
dbLoadRecords("../../db/BPMCal.db","P=iLinac_007:BPM14And15, P1=iLinac_007:BPM14, P2=iLinac_007:BPM15")

iocInit()

## Start any sequence programs
#seq sncBPMmonitor,"user=gao"
```

**执行流程**:
```
1. Shebang行 → 指定IOC可执行文件
2. dbLoadDatabase → 加载.dbd定义
3. registerRecordDeviceDriver → 注册设备支持
4. dbLoadRecords → 加载Record实例
5. iocInit → 初始化IOC
6. (可选) 启动序列程序
```

---

## 🔍 逐行解析

### 1. Shebang（第1行）

```bash
#!/home/user/BPMIOC/bin/linux-arm/BPMmonitor
```

**作用**: 指定脚本的解释器

**工作原理**:
```bash
$ ./st.cmd
  ↓
Linux读取第一行#!
  ↓
使用/home/user/BPMIOC/bin/linux-arm/BPMmonitor执行脚本
  ↓
等价于：
$ /home/user/BPMIOC/bin/linux-arm/BPMmonitor st.cmd
```

**注意**:
- 必须是**绝对路径**
- 必须指向编译好的IOC可执行文件
- 路径随平台变化（linux-x86_64, linux-arm等）

**权限**:
```bash
chmod +x st.cmd  # 添加执行权限
```

---

### 2. envPaths（注释掉）

```bash
#< envPaths
```

**作用**: 加载环境变量定义

**envPaths文件内容**（自动生成）:
```bash
epicsEnvSet("ARCH","linux-arm")
epicsEnvSet("IOC","iocBPMmonitor")
epicsEnvSet("TOP","/home/user/BPMIOC")
epicsEnvSet("EPICS_BASE","/opt/epics/base")
```

**为什么注释掉？**
- BPMIOC使用硬编码路径
- 不依赖环境变量

**如果启用**:
```bash
< envPaths  # 加载环境变量

# 然后可以使用
dbLoadRecords("$(TOP)/db/BPMMonitor.db")
```

---

### 3. dbLoadDatabase

```bash
dbLoadDatabase("../../dbd/BPMmonitor.dbd",0,0)
```

**作用**: 加载数据库定义（Database Definition）

**参数**:
```c
dbLoadDatabase(filename, path, substitutions)
   │             │        │         │
   │             │        │         └─> 宏替换（通常为NULL/0）
   │             │        └───────────> 搜索路径（通常为NULL/0）
   │             └────────────────────> .dbd文件路径
   └──────────────────────────────────> 函数名
```

**.dbd文件包含**:
```
# BPMmonitor.dbd
recordtype(ai)         # 注册Record类型
device(ai, ...)        # 注册设备支持
driver(drvBPMmonitor)  # 注册驱动
function(...)          # 注册函数
variable(...)          # 注册全局变量
```

**为什么需要？**
- 告诉IOC有哪些Record类型
- 有哪些设备支持
- 有哪些驱动

---

### 4. registerRecordDeviceDriver

```bash
BPMmonitor_registerRecordDeviceDriver(pdbbase)
```

**作用**: 注册所有Record类型和设备支持

**这个函数哪里来的？**

编译时自动生成（在`O.Common`目录）：
```c
// BPMmonitor_registerRecordDeviceDriver.cpp (自动生成)
long BPMmonitor_registerRecordDeviceDriver(DBBASE *pdbbase)
{
    // 注册Record类型
    registerRecordDeviceDriver(pdbbase);

    // 注册设备支持
    extern dset devAiBPMmonitor;
    pvar_dset_devAiBPMmonitor = &devAiBPMmonitor;

    extern dset devAoBPMmonitor;
    pvar_dset_devAoBPMmonitor = &devAoBPMmonitor;

    // 注册驱动
    extern drvet drvBPMmonitor;
    pvar_drvet_drvBPMmonitor = &drvBPMmonitor;

    return 0;
}
```

**为什么需要？**
- 在内存中建立Record类型→实现的映射
- 在内存中建立DTYP→设备支持的映射

---

### 5. dbLoadRecords

```bash
dbLoadRecords("../../db/BPMMonitor.db",
              "P=iLinac_007:BPM14And15, P1=iLinac_007:BPM14, P2=iLinac_007:BPM15")
```

**作用**: 加载Record实例

**参数**:
```c
dbLoadRecords(filename, macros)
   │            │         │
   │            │         └─> 宏定义（key=value, key=value）
   │            └───────────> .db文件路径
   └────────────────────────> 函数名
```

**宏替换示例**:

**.db文件**:
```
record(ai, "$(P):RFIn_01_Amp") { ... }
record(ai, "$(P1):Status") { ... }
record(ai, "$(P2):Status") { ... }
```

**st.cmd**:
```bash
dbLoadRecords("BPMMonitor.db",
              "P=iLinac_007:BPM14And15, P1=iLinac_007:BPM14, P2=iLinac_007:BPM15")
```

**结果PV**:
```
iLinac_007:BPM14And15:RFIn_01_Amp
iLinac_007:BPM14:Status
iLinac_007:BPM15:Status
```

**可以多次调用**:
```bash
# 加载第一个BPM
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM01")

# 加载第二个BPM
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM02")
```

---

### 6. iocInit

```bash
iocInit()
```

**作用**: 初始化IOC（**最重要的命令**）

**执行过程**:

```
iocInit()
  ↓
1. 初始化Record支持
   - 调用每个Record的init_record()
   - 建立链接关系
  ↓
2. 初始化扫描线程
   - 创建周期扫描线程（.1s, .5s, 1s等）
   - 创建I/O中断扫描线程
  ↓
3. 处理PINI="YES"的Record
   - 读取初始值
  ↓
4. 启动CA Server
   - 开始接受客户端连接
  ↓
5. 启动扫描
   - 所有扫描线程开始工作
```

**输出示例**:
```
Starting iocInit
############################################################################
## EPICS R3.15.5
## EPICS Base built Nov  9 2024
############################################################################
iocRun: All initialization complete
```

**注意**:
- `iocInit()`之后，IOC开始运行
- 之后不应该再调用`dbLoadRecords()`
- 可以执行IOC Shell命令（如dbl, dbgf等）

---

### 7. 序列程序（可选）

```bash
#seq sncBPMmonitor,"user=gao"
```

**作用**: 启动State Notation Language（SNL）序列程序

**如果启用**:
```bash
seq sncBPMmonitor, "user=gao"
    │      │           │
    │      │           └─> 传递给序列程序的参数
    │      └─────────────> 序列程序名
    └────────────────────> 命令
```

**SNL程序**: 用于复杂的状态机逻辑（BPMIOC未使用）

---

## 🎨 高级用法

### 设置IOC参数

**调试级别**:
```bash
var devBPMmonitorDebug 1  # 设置调试级别
```

**CA参数**:
```bash
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "10000000")  # 增加数组大小限制
```

**扫描线程优先级**:
```bash
scanppl  # 查看扫描线程信息
```

---

### 条件执行

```bash
# 只在Linux上执行
epicsEnvShow("ARCH")
< linux.cmd

# 检查环境变量
epicsEnvShow("USE_REAL_HW")
ifeq("$(USE_REAL_HW)", "YES")
    drvBPMmonitorInit()  # 初始化真实硬件
else
    drvBPMmonitorMockInit()  # 初始化模拟器
endif
```

---

### 调用驱动初始化函数

**示例**:
```bash
# 驱动初始化（如果需要手动调用）
drvBPMmonitorInit()

# 传递参数
drvBPMmonitorConfig(0, "192.168.1.100")  # (卡号, IP地址)
```

**对应的C代码**:
```c
// driverWrapper.c
static const iocshArg drvBPMmonitorInitArg0 = {"card", iocshArgInt};
static const iocshArg drvBPMmonitorInitArg1 = {"ip", iocshArgString};

static const iocshArg *drvBPMmonitorInitArgs[] = {
    &drvBPMmonitorInitArg0,
    &drvBPMmonitorInitArg1
};

static const iocshFuncDef drvBPMmonitorInitDef = {
    "drvBPMmonitorInit", 2, drvBPMmonitorInitArgs
};

// 注册到IOC Shell
void drvBPMmonitorRegister(void)
{
    iocshRegister(&drvBPMmonitorInitDef, drvBPMmonitorInitCall);
}
```

---

### 加载多个数据库文件

```bash
# 主数据库
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM")

# 校准数据库
dbLoadRecords("BPMCal.db", "P=LLRF:BPM")

# 诊断数据库
dbLoadRecords("BPMDiag.db", "P=LLRF:BPM")

# 自动保存/恢复
dbLoadRecords("save_restore.db", "P=LLRF:BPM")
```

---

## 🔧 调试启动脚本

### 启用调试输出

```bash
# 在st.cmd开头添加
var recGblInitHookDebug 1  # Record初始化调试
var dbRecordsOnceOnly 0    # 允许重复加载Record（调试用）
```

**输出示例**:
```
initHookAtBeginning
initHookAfterCallbackInit
initHookAtBeginning
...
```

---

### 交互式IOC Shell

**不直接运行st.cmd，而是手动执行命令**:

```bash
$ /home/user/BPMIOC/bin/linux-arm/BPMmonitor

epics> dbLoadDatabase("../../dbd/BPMmonitor.dbd")
epics> BPMmonitor_registerRecordDeviceDriver(pdbbase)
epics> dbLoadRecords("../../db/BPMMonitor.db", "P=TEST")
epics> dbl  # 列出所有PV
TEST:RFIn_01_Amp
TEST:RFIn_02_Amp
...
epics> iocInit()
epics> exit
```

---

### 常用IOC Shell命令

**查看PV**:
```bash
dbl                  # 列出所有PV
dbl ai               # 列出所有ai类型的PV
dbgrep "TEST:*"      # 列出匹配的PV
```

**查看Record信息**:
```bash
dbpr "TEST:PV" 1     # 打印Record字段
dbgf "TEST:PV.VAL"   # 获取字段值
dbpf "TEST:PV.VAL" 10  # 设置字段值
```

**扫描相关**:
```bash
scanppl              # 打印扫描线程信息
scanpel              # 打印事件扫描信息
scanpiol             # 打印I/O中断扫描信息
```

---

## 📊 启动顺序图

```
./st.cmd 执行
    ↓
[Shebang] → 使用IOC可执行文件运行
    ↓
[dbLoadDatabase] → 加载.dbd定义
    ↓
    内存中建立：
    - Record类型表
    - 设备支持表
    - 驱动表
    ↓
[registerRecordDeviceDriver] → 注册实现
    ↓
    建立映射：
    - recordtype → 实现
    - device → dset指针
    ↓
[dbLoadRecords] → 加载Record实例
    ↓
    创建Record结构：
    - aiRecord实例
    - 填充字段值
    - 宏替换PV名
    ↓
[iocInit] → 初始化IOC
    ↓
    执行初始化：
    1. 调用init_record() → 创建DevPvt
    2. 建立链接 → FLNK, INP等
    3. 创建扫描线程
    4. 处理PINI="YES"
    5. 启动CA Server
    ↓
IOC运行中...
    ↓
    - 扫描线程处理Record
    - CA Server响应客户端
    - 驱动层读写硬件
```

---

## ⚠️ 常见错误

### 错误1: .dbd文件未找到

```
dbLoadDatabase("BPMmonitor.dbd")
Error: Can't open file 'BPMmonitor.dbd'
```

**原因**: 路径错误

**解决**:
```bash
# 使用相对路径（从iocBoot/ioc<name>目录）
dbLoadDatabase("../../dbd/BPMmonitor.dbd")

# 或绝对路径
dbLoadDatabase("/home/user/BPMIOC/dbd/BPMmonitor.dbd")
```

---

### 错误2: PV名重复

```
dbLoadRecords("BPMMonitor.db", "P=TEST")
dbLoadRecords("BPMMonitor.db", "P=TEST")  # ❌ 重复！

Error: Duplicate record name 'TEST:RFIn_01_Amp'
```

**解决**: 使用不同的宏值

---

### 错误3: 缺少registerRecordDeviceDriver

```
dbLoadRecords(...)
iocInit()

Error: No device support for record type 'ai'
```

**原因**: 忘记调用`registerRecordDeviceDriver`

**解决**: 在`dbLoadRecords`之前调用

---

### 错误4: iocInit失败

```
iocInit()
Error: ... initialization failed
```

**调试**:
```bash
# 启用详细输出
var recGblInitHookDebug 1

# 检查Record配置
dbl
dbpr "FailedRecord" 3
```

---

## ✅ 学习检查点

- [ ] 理解st.cmd的作用和执行流程
- [ ] 能够编写基本的st.cmd脚本
- [ ] 理解dbLoadDatabase和registerRecordDeviceDriver的作用
- [ ] 掌握dbLoadRecords的宏替换机制
- [ ] 理解iocInit的初始化过程
- [ ] 能够调试启动脚本错误

---

## 🎯 总结

### st.cmd的本质

st.cmd是IOC的**配置即代码**：
- 声明式配置（.db文件）
- 命令式启动（st.cmd脚本）
- 组合成完整的IOC应用

### 关键命令

| 命令 | 阶段 | 作用 |
|------|------|------|
| `dbLoadDatabase` | 1 | 加载定义 |
| `registerRecordDeviceDriver` | 2 | 注册实现 |
| `dbLoadRecords` | 3 | 加载实例 |
| `iocInit` | 4 | 启动IOC |

### BPMIOC的启动

```
BPMmonitor可执行文件
    ↓
加载BPMmonitor.dbd（定义）
    ↓
注册设备支持（devBPMMonitor.c）
    ↓
加载BPMMonitor.db（2次，不同宏）
    ↓
iocInit（启动）
    ↓
IOC运行...
```

---

## 🎯 Part 6总结

完成Part 6的5个核心文档后，你现在理解了：

✅ **数据库文件结构** - .db文件的语法和组织
✅ **Record配置** - 各种字段的配置和用法
✅ **PV命名规范** - 如何设计清晰的命名体系
✅ **扫描机制** - 如何控制Record的处理时机
✅ **启动脚本** - st.cmd的编写和IOC启动流程

**下一步**: 学习Part 7构建系统，或者跳到Part 8开始动手实验！

---

**关键理解**: st.cmd是连接"定义"（.dbd）、"实现"（.c）、"配置"（.db）三者的桥梁！💡
