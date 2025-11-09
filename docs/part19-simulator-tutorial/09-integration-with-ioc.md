# Part 19.9: 与BPMIOC IOC集成

> **目标**: 将Mock库集成到完整的BPMIOC IOC中
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 1.5小时
> **前置**: 已完成01-08所有文档

## 📖 内容概览

本文档介绍如何将Mock库集成到BPMIOC IOC：
- 理解driverWrapper的动态库加载机制
- 配置选择Mock或Real库
- 修改Makefile支持Mock库
- 完整的测试流程
- 故障排查

完成本文档后，你将能在PC上运行完整的BPMIOC IOC！

---

## 1. BPMIOC架构回顾

### 1.1 三层架构

```
┌─────────────────────────────────────┐
│  Database Layer (.db files)         │  ← EPICS PVs
│  - RF:Amp, RF:Phase, XY:X, XY:Y     │
└──────────────┬──────────────────────┘
               │ Device Support API
┌──────────────▼──────────────────────┐
│  Device Support Layer               │
│  (devBPMMonitor.c)                  │  ← Record processing
└──────────────┬──────────────────────┘
               │ ReadData(), SetReg()
┌──────────────▼──────────────────────┐
│  Driver Layer (driverWrapper.c)     │  ← Hardware abstraction
│  - dlopen() to load library         │
│  - ReadData(), pthread, etc.        │
└──────────────┬──────────────────────┘
               │ Hardware Functions
         ┌─────┴─────┐
    ┌────▼────┐ ┌────▼────┐
    │ Mock库  │ │ Real库  │
    │ (PC)    │ │ (ZYNQ)  │
    └─────────┘ └─────────┘
```

**关键点**:
- driverWrapper.c使用dlopen()动态加载库
- 库可以是libbpm_mock.so（PC）或libbpm_zynq.so（ZYNQ）
- 上层代码（Device Support和Database）无需改动

---

### 1.2 dlopen机制

```c
// driverWrapper.c中的代码（简化版）

void *g_lib_handle = NULL;

// 函数指针
int (*funcSystemInit)(void);
float (*funcGetRFInfo)(int channel, int type);
// ... 其他函数

int loadHardwareLibrary(const char *lib_path) {
    // 1. 加载库
    g_lib_handle = dlopen(lib_path, RTLD_LAZY);
    if (!g_lib_handle) {
        printf("ERROR: %s\n", dlerror());
        return -1;
    }

    // 2. 获取函数指针
    funcSystemInit = dlsym(g_lib_handle, "SystemInit");
    funcGetRFInfo = dlsym(g_lib_handle, "GetRFInfo");
    // ... 获取其他函数

    // 3. 检查是否成功
    if (!funcSystemInit || !funcGetRFInfo) {
        printf("ERROR: Cannot find required functions\n");
        dlclose(g_lib_handle);
        return -1;
    }

    return 0;
}

// 在IOC初始化时调用
int drvBPMConfigure(const char *portName, const char *lib_path) {
    if (loadHardwareLibrary(lib_path) != 0) {
        return -1;
    }

    // 初始化硬件
    if (funcSystemInit() != 0) {
        return -1;
    }

    // ... 其他初始化
    return 0;
}
```

---

## 2. 配置库路径

### 2.1 通过启动脚本配置

在EPICS IOC启动脚本`st.cmd`中指定库路径：

```bash
# st.cmd - IOC启动脚本

# 加载驱动
dbLoadDriver("dbd/BPMMonitor.dbd")

# 配置使用Mock库（在PC上）
drvBPMConfigure("BPM1", "/home/user/BPMIOC/simulator/lib/libbpm_mock.so")

# 或配置使用Real库（在ZYNQ上）
# drvBPMConfigure("BPM1", "/usr/lib/libbpm_zynq.so")

# 加载数据库
dbLoadRecords("db/bpm.db", "PREFIX=BPM:01:")

# 启动IOC
iocInit
```

---

### 2.2 通过环境变量配置

更灵活的方法是使用环境变量：

**修改driverWrapper.c**:

```c
int drvBPMConfigure(const char *portName, const char *lib_path) {
    const char *actual_lib_path = lib_path;

    // 如果设置了环境变量，优先使用环境变量
    const char *env_lib = getenv("BPM_LIB_PATH");
    if (env_lib != NULL) {
        actual_lib_path = env_lib;
        printf("Using library from environment: %s\n", actual_lib_path);
    }

    // 如果还是NULL，使用默认值
    if (actual_lib_path == NULL) {
        const char *sim_mode = getenv("BPM_SIMULATION_MODE");
        if (sim_mode != NULL && strcmp(sim_mode, "1") == 0) {
            actual_lib_path = "/home/user/BPMIOC/simulator/lib/libbpm_mock.so";
        } else {
            actual_lib_path = "/usr/lib/libbpm_zynq.so";
        }
    }

    printf("Loading hardware library: %s\n", actual_lib_path);

    return loadHardwareLibrary(actual_lib_path);
}
```

**使用环境变量**:

```bash
# 在PC上运行（使用Mock库）
export BPM_SIMULATION_MODE=1
./bin/linux-x86_64/BPMMonitor st.cmd

# 在ZYNQ上运行（使用Real库）
unset BPM_SIMULATION_MODE
./bin/linux-arm/BPMMonitor st.cmd

# 或直接指定库路径
export BPM_LIB_PATH=/home/user/BPMIOC/simulator/lib/libbpm_mock.so
./bin/linux-x86_64/BPMMonitor st.cmd
```

---

### 2.3 通过配置文件

创建配置文件`bpm_config.ini`:

```ini
# bpm_config.ini

[Hardware]
# 库路径（Mock或Real）
LibraryPath = /home/user/BPMIOC/simulator/lib/libbpm_mock.so

# 模拟模式（1=Mock, 0=Real）
SimulationMode = 1

[Mock]
# Mock库配置
ConfigFile = /home/user/BPMIOC/simulator/config/mock_config.ini
```

**读取配置文件**（需要添加ini解析代码）:

```c
#include "ini_parser.h"  // 第三方库，如inih

int drvBPMConfigure(const char *portName, const char *config_file) {
    // 解析配置文件
    const char *lib_path = ini_get_string(config_file, "Hardware", "LibraryPath");

    return loadHardwareLibrary(lib_path);
}
```

---

## 3. 修改BPMIOC构建系统

### 3.1 项目目录结构

```
BPMIOC/
├── configure/          # EPICS配置
├── BPMMonitorApp/      # IOC应用
│   ├── src/
│   │   ├── driverWrapper.c
│   │   ├── devBPMMonitor.c
│   │   └── Makefile
│   └── Db/
│       └── bpm.db
├── iocBoot/
│   └── iocBPMMonitor/
│       └── st.cmd      # 启动脚本
├── simulator/          # Mock库（新增）
│   ├── src/
│   │   └── libbpm_mock.c
│   └── lib/
│       └── libbpm_mock.so
└── Makefile
```

---

### 3.2 修改BPMMonitorApp/src/Makefile

```makefile
# BPMMonitorApp/src/Makefile

TOP=../..
include $(TOP)/configure/CONFIG

# 构建IOC应用
PROD_IOC = BPMMonitor

# 数据库定义
DBD += BPMMonitor.dbd
BPMMonitor_DBD += base.dbd
BPMMonitor_DBD += asyn.dbd

# 源文件
BPMMonitor_SRCS += driverWrapper.c
BPMMonitor_SRCS += devBPMMonitor.c
BPMMonitor_SRCS += BPMMonitor_registerRecordDeviceDriver.cpp

# 链接库
BPMMonitor_LIBS += asyn
BPMMonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

# ========== 新增：支持Mock库 ==========

# 如果定义了SIMULATION_MODE，添加Mock库路径
ifdef SIMULATION_MODE
  # 添加库搜索路径
  USR_LDFLAGS += -L$(TOP)/simulator/lib

  # 添加运行时库路径（rpath）
  USR_LDFLAGS += -Wl,-rpath,$(TOP)/simulator/lib

  # 定义宏
  USR_CFLAGS += -DSIMULATION_MODE=1
endif

# 添加dl库（dlopen需要）
BPMMonitor_SYS_LIBS += dl

include $(TOP)/configure/RULES
```

---

### 3.3 修改顶层Makefile

```makefile
# BPMIOC/Makefile

TOP = .
include $(TOP)/configure/CONFIG

# 子目录
DIRS := configure
DIRS += BPMMonitorApp
DIRS += iocBoot

# ========== 新增：构建Mock库 ==========

# 在SIMULATION_MODE下，先构建Mock库
ifdef SIMULATION_MODE
  DIRS := simulator $(DIRS)
endif

include $(TOP)/configure/RULES_TOP

# 自定义目标：构建Mock库
.PHONY: simulator
simulator:
	@echo "Building Mock library..."
	$(MAKE) -C simulator/src
	@echo "Mock library built successfully"

# 清理Mock库
clean-simulator:
	@echo "Cleaning Mock library..."
	$(MAKE) -C simulator/src clean
```

---

### 3.4 创建simulator/Makefile

```makefile
# simulator/Makefile

.PHONY: all clean

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
```

---

## 4. 完整编译和运行

### 4.1 编译Mock库

```bash
cd ~/BPMIOC/simulator/src
make clean && make

# 验证
ls -lh ../lib/libbpm_mock.so
```

---

### 4.2 编译BPMIOC IOC

```bash
cd ~/BPMIOC

# 设置EPICS环境变量
export EPICS_BASE=/opt/epics/base
export EPICS_HOST_ARCH=linux-x86_64

# 使用Mock库编译
export SIMULATION_MODE=1
make clean
make

# 检查生成的可执行文件
ls -l bin/linux-x86_64/BPMMonitor
```

---

### 4.3 修改启动脚本

编辑`iocBoot/iocBPMMonitor/st.cmd`:

```bash
#!../../bin/linux-x86_64/BPMMonitor

# st.cmd - BPMIOC启动脚本

< envPaths

cd "${TOP}"

## 注册所有支持的组件
dbLoadDatabase "dbd/BPMMonitor.dbd"
BPMMonitor_registerRecordDeviceDriver pdbbase

## ========== 配置驱动 ==========

# 加载Mock库
epicsEnvSet("BPM_LIB_PATH", "$(TOP)/simulator/lib/libbpm_mock.so")

# 配置BPM端口
drvBPMConfigure("BPM1", "$(BPM_LIB_PATH)")

## ========== 加载数据库 ==========

# 加载BPM记录
dbLoadRecords("db/bpm.db", "PREFIX=BPM:01:,PORT=BPM1")

## ========== 启动IOC ==========

cd "${TOP}/iocBoot/${IOC}"
iocInit

## ========== 测试命令 ==========

# 显示一些PV值
dbl
dbpr BPM:01:RF3:Amp 2
dbpr BPM:01:XY1:X 2
```

---

### 4.4 运行IOC

```bash
cd ~/BPMIOC/iocBoot/iocBPMMonitor

# 运行IOC
../../bin/linux-x86_64/BPMMonitor st.cmd
```

**预期输出**:

```
#!../../bin/linux-x86_64/BPMMonitor
< envPaths
...
Loading hardware library: /home/user/BPMIOC/simulator/lib/libbpm_mock.so
Mock library loaded successfully
SystemInit() = 0
...
Starting iocInit
############################################################################
## EPICS R7.0.6
## Rev. R7.0.6
############################################################################
iocRun: All initialization complete

# 测试PV
epics> dbpr BPM:01:RF3:Amp 2
ASG:                DESC: RF3 Amplitude           DISA: 0
DISP: 0             DISV: 1                       NAME: BPM:01:RF3:Amp
SEVR: NO_ALARM      STAT: NO_ALARM                SVAL: 1.02
TPRO: 0             VAL: 1.02

epics> dbpr BPM:01:XY1:X 2
...
VAL: 0.10
```

---

## 5. 使用caget/caput测试

### 5.1 在另一个终端测试PVs

```bash
# 新开一个终端

# 读取RF幅度
caget BPM:01:RF3:Amp
# 输出: BPM:01:RF3:Amp 1.02

caget BPM:01:RF3:Phase
# 输出: BPM:01:RF3:Phase 0.15

# 读取XY位置
caget BPM:01:XY1:X BPM:01:XY1:Y
# 输出:
# BPM:01:XY1:X 0.10
# BPM:01:XY1:Y 0.08

# 监控PV变化
camonitor BPM:01:RF3:Amp
# 输出:
# BPM:01:RF3:Amp 1.02 2024-01-15 10:30:00.123456
# BPM:01:RF3:Amp 1.01 2024-01-15 10:30:00.223456
# BPM:01:RF3:Amp 1.03 2024-01-15 10:30:00.323456
# ...
```

---

### 5.2 测试寄存器写入

```bash
# 写入寄存器
caput BPM:01:Reg10 12345

# 读取寄存器
caget BPM:01:Reg10
# 输出: BPM:01:Reg10 12345
```

---

### 5.3 使用CSS/EDM监控

如果安装了CSS或EDM图形界面：

```bash
# 使用CSS
css-launcher &

# 或使用EDM
edm -x bpm_monitor.edl &
```

**在图形界面中**:
- 添加PV: `BPM:01:RF3:Amp`
- 添加PV: `BPM:01:XY1:X`
- 观察实时更新的数值和图表

---

## 6. 验证Mock库工作

### 6.1 验证数据变化

```bash
# 测试脚本
cat > test_pv_variation.sh << 'EOF'
#!/bin/bash

echo "=== Testing PV Time Variation ==="
for i in {1..10}; do
    echo -n "[$i] "
    caget -t BPM:01:RF3:Amp
    sleep 0.5
done
EOF

chmod +x test_pv_variation.sh
./test_pv_variation.sh
```

**预期输出**（数值应该变化）:

```
=== Testing PV Time Variation ===
[1] 1.02
[2] 1.01
[3] 1.03
[4] 1.00
[5] 1.02
[6] 1.01
...
```

---

### 6.2 验证多通道

```bash
# 测试所有RF通道
for ch in 3 4 5 6; do
    echo "RF${ch}:"
    caget BPM:01:RF${ch}:Amp BPM:01:RF${ch}:Phase
done
```

**输出**:

```
RF3:
BPM:01:RF3:Amp  1.02
BPM:01:RF3:Phase 0.15

RF4:
BPM:01:RF4:Amp  0.98
BPM:01:RF4:Phase -0.12

RF5:
BPM:01:RF5:Amp  1.01
BPM:01:RF5:Phase 0.05

RF6:
BPM:01:RF6:Amp  1.00
BPM:01:RF6:Phase 0.00
```

---

### 6.3 验证波形数据

```bash
# 读取波形（如果PV支持）
caget -a BPM:01:RF3:Waveform

# 或使用Python测试
cat > test_waveform.py << 'EOF'
from epics import caget

wf = caget("BPM:01:RF3:Waveform")
print(f"Waveform length: {len(wf)}")
print(f"First 10 points: {wf[:10]}")
print(f"Min: {min(wf):.4f}, Max: {max(wf):.4f}")
EOF

python3 test_waveform.py
```

---

## 7. 切换Mock和Real库

### 7.1 快速切换方法

**方法1: 修改启动脚本**

```bash
# st.cmd

# Mock库（PC开发）
#drvBPMConfigure("BPM1", "$(TOP)/simulator/lib/libbpm_mock.so")

# Real库（ZYNQ硬件）
drvBPMConfigure("BPM1", "/usr/lib/libbpm_zynq.so")
```

**方法2: 环境变量**

```bash
# 使用Mock库
export BPM_LIB_PATH=~/BPMIOC/simulator/lib/libbpm_mock.so
./BPMMonitor st.cmd

# 使用Real库
export BPM_LIB_PATH=/usr/lib/libbpm_zynq.so
./BPMMonitor st.cmd
```

**方法3: 启动脚本参数**

```bash
# 修改st.cmd支持参数
< envPaths

# 从环境变量获取库路径，默认使用Mock库
epicsEnvSet("BPM_LIB_PATH", "$(TOP)/simulator/lib/libbpm_mock.so", "")

# 如果环境变量BPM_USE_REAL=1，使用Real库
#epicsEnvSet("BPM_USE_REAL", "0", "")
#epicsEnvIf("BPM_USE_REAL", "1", "epicsEnvSet(\"BPM_LIB_PATH\", \"/usr/lib/libbpm_zynq.so\")")

drvBPMConfigure("BPM1", "$(BPM_LIB_PATH)")
```

---

## 8. 故障排查

### 8.1 库加载失败

**症状**:

```
ERROR: cannot open shared object file: libbpm_mock.so
```

**解决**:

```bash
# 检查库文件存在
ls -l ~/BPMIOC/simulator/lib/libbpm_mock.so

# 检查库路径
ldd bin/linux-x86_64/BPMMonitor | grep libbpm

# 添加到LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/BPMIOC/simulator/lib
```

---

### 8.2 符号未找到

**症状**:

```
ERROR: undefined symbol: GetRFInfo
```

**检查符号**:

```bash
nm -D ~/BPMIOC/simulator/lib/libbpm_mock.so | grep GetRFInfo
# 应该看到: 00001a20 T GetRFInfo
```

如果没有，重新编译Mock库：

```bash
cd ~/BPMIOC/simulator/src
make clean && make
```

---

### 8.3 PV没有更新

**症状**: caget总是返回相同的值

**检查**:

```bash
# 1. 确认pthread正在运行
# 在IOC shell中
epics> var drvBPMDebug 1

# 2. 检查Mock库是否正确调用TriggerAllDataReached
# 在Mock库中添加日志

# 3. 检查扫描速率
epics> dbpr BPM:01:RF3:Amp 2
...
SCAN: .1 second
```

---

### 8.4 数值异常

**症状**: PV显示NaN或超大值

**调试**:

```c
// 在driverWrapper.c中添加检查
float ReadData(int offset, int channel, int type) {
    float value = funcGetRFInfo(channel, type);

    // 添加检查
    if (isnan(value) || isinf(value)) {
        printf("ERROR: Invalid value from GetRFInfo: %f\n", value);
        return 0.0f;
    }

    return value;
}
```

---

## 9. 集成验证清单

完成集成后，使用此清单验证：

### 编译验证
- [ ] Mock库成功编译（libbpm_mock.so存在）
- [ ] IOC成功编译（BPMMonitor可执行文件存在）
- [ ] 无链接错误

### 启动验证
- [ ] IOC启动无错误
- [ ] Mock库成功加载
- [ ] SystemInit()返回成功
- [ ] 所有PV创建成功

### 功能验证
- [ ] caget可以读取RF PV
- [ ] caget可以读取XY PV
- [ ] caget可以读取Button PV
- [ ] caput可以写入寄存器PV
- [ ] PV值随时间变化

### 性能验证
- [ ] IOC稳定运行
- [ ] CPU使用率正常（< 10%）
- [ ] PV更新速率正常（10 Hz）

---

## 10. 总结

### 你学到了什么？

✅ **dlopen机制**
- 动态库加载原理
- 函数指针获取

✅ **集成配置**
- 启动脚本配置
- 环境变量配置
- 构建系统修改

✅ **完整流程**
- 编译Mock库
- 编译IOC
- 运行和测试

✅ **测试验证**
- caget/caput测试
- 图形界面监控
- 故障排查

---

### 下一步

现在你已经实现了完整的Mock集成！

继续学习：
- **[10-mock-api-reference.md](./10-mock-api-reference.md)** - API完整参考
- **[11-best-practices.md](./11-best-practices.md)** - 最佳实践

---

## 11. 快速参考

### 编译命令

```bash
# Mock库
cd ~/BPMIOC/simulator/src
make clean && make

# IOC（使用Mock）
cd ~/BPMIOC
export SIMULATION_MODE=1
make clean && make
```

### 运行命令

```bash
# 启动IOC
cd ~/BPMIOC/iocBoot/iocBPMMonitor
../../bin/linux-x86_64/BPMMonitor st.cmd
```

### 测试命令

```bash
# 读取PV
caget BPM:01:RF3:Amp

# 监控PV
camonitor BPM:01:RF3:Amp

# 写入PV
caput BPM:01:Reg10 12345
```

---

**🎯 重要**: 成功集成Mock库后，你就可以在PC上进行90%的BPMIOC开发工作了！只有最终验证才需要ZYNQ硬件。
