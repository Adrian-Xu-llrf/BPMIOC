# BPMIOC 8周开发学习计划

## 计划说明

本计划为期 **8 周**,每周约 **15-20 小时**学习时间,适合博士一年级学生在课余时间完成。

### 计划特点
- ✅ **任务驱动**: 每周有明确的实践任务
- ✅ **问题导向**: 每周带着问题学习,培养思考能力
- ✅ **循序渐进**: 从基础到进阶,逐步深入
- ✅ **可评估**: 每周有检查点,验证学习成果

### 学习方法建议
1. **先思考再动手**: 看到问题先思考5分钟再查资料
2. **做笔记**: 记录遇到的问题和解决方案
3. **画图**: 用架构图、流程图帮助理解
4. **提问**: 不懂就问,在注释中记录疑问

---

## 第 1 周: 环境搭建与 EPICS 初体验

### 本周目标
- ✅ 搭建完整的 EPICS 开发环境
- ✅ 运行第一个 IOC
- ✅ 理解 PV、Record、IOC 等基本概念

### 周一-周二: 安装 EPICS Base

#### 任务清单
- [ ] 下载并编译 EPICS Base 3.15.6
- [ ] 配置环境变量
- [ ] 验证安装 (运行 softIoc)

#### 实践步骤
```bash
# 1. 下载
cd ~
wget https://epics.anl.gov/download/base/base-3.15.6.tar.gz
tar -xzf base-3.15.6.tar.gz
cd base-3.15.6

# 2. 编译
make clean
make -j4

# 3. 配置环境变量
echo 'export EPICS_BASE=$HOME/base-3.15.6' >> ~/.bashrc
echo 'export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)' >> ~/.bashrc
echo 'export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}' >> ~/.bashrc
source ~/.bashrc

# 4. 验证
which softIoc
softIoc --version
```

#### 本节问题 (带着问题学习)
1. **EPICS Base 包含哪些核心组件?**
   - 提示: 查看 `bin/` 和 `lib/` 目录

2. **EPICS_HOST_ARCH 的值是什么?它的作用是什么?**
   - 提示: 执行 `echo $EPICS_HOST_ARCH`

3. **如果编译失败,如何查看错误信息?**
   - 提示: 查找 `*.log` 文件

### 周三-周四: 创建第一个 IOC

#### 任务清单
- [ ] 创建简单的数据库文件
- [ ] 编写启动脚本
- [ ] 运行 IOC 并访问 PV

#### 实践任务 1: 温度监测系统

创建文件 `~/epics_test/temperature.db`:

```
# 温度传感器 (模拟)
record(ai, "TEMP:Sensor1") {
    field(DESC, "Temperature Sensor 1")
    field(SCAN, "1 second")
    field(PREC, "2")
    field(EGU,  "C")
    field(HOPR, "100")
    field(LOPR, "0")
}

record(ai, "TEMP:Sensor2") {
    field(DESC, "Temperature Sensor 2")
    field(SCAN, "1 second")
    field(PREC, "2")
    field(EGU,  "C")
}

# 平均温度计算
record(calc, "TEMP:Average") {
    field(DESC, "Average Temperature")
    field(INPA, "TEMP:Sensor1")
    field(INPB, "TEMP:Sensor2")
    field(CALC, "(A+B)/2")
    field(PREC, "2")
    field(EGU,  "C")
    field(SCAN, "1 second")
}

# 温度差
record(calc, "TEMP:Difference") {
    field(DESC, "Temperature Difference")
    field(INPA, "TEMP:Sensor1")
    field(INPB, "TEMP:Sensor2")
    field(CALC, "ABS(A-B)")
    field(PREC, "2")
    field(EGU,  "C")
    field(SCAN, "1 second")
}

# 高温告警
record(bi, "TEMP:HighAlarm") {
    field(DESC, "High Temperature Alarm")
    field(INP,  "TEMP:Average")
    field(SCAN, "1 second")
    field(ZNAM, "Normal")
    field(ONAM, "Alarm")
}
```

创建启动脚本 `st.cmd`:
```bash
#!/usr/bin/env softIoc
dbLoadRecords("temperature.db")
iocInit()
dbl
```

运行:
```bash
chmod +x st.cmd
./st.cmd
```

#### 本节问题
1. **ai 记录的 SCAN 字段有哪些可能的值?**
   - 实验: 将 SCAN 改为 "Passive",观察行为变化

2. **calc 记录的 CALC 字段支持哪些运算符?**
   - 提示: 查阅 EPICS Application Developer's Guide

3. **如果删除 calc 记录的 SCAN 字段会怎样?**
   - 实验并观察结果

4. **如何在 IOC Shell 中手动触发 Passive 记录?**
   - 提示: 使用 `dbpf` 命令

### 周五: 使用 Channel Access 工具

#### 任务清单
- [ ] 使用 caget/caput/camonitor 访问 PV
- [ ] 编写简单的 Python 脚本读取 PV

#### 实践任务 2: CA 工具练习

保持上面的 IOC 运行,在新终端执行:

```bash
# 1. 读取单个 PV
caget TEMP:Sensor1

# 2. 读取多个 PV
caget TEMP:Sensor1 TEMP:Sensor2 TEMP:Average

# 3. 写入 PV
caput TEMP:Sensor1 25.5
caput TEMP:Sensor2 24.8

# 4. 监控 PV 变化
camonitor TEMP:Average TEMP:Difference

# 5. 查看 PV 详细信息
cainfo TEMP:Sensor1
```

#### 实践任务 3: Python 访问 EPICS

安装 pyepics:
```bash
pip install pyepics
```

创建 `monitor.py`:
```python
#!/usr/bin/env python3
import epics
import time

# 读取 PV
temp1 = epics.caget('TEMP:Sensor1')
print(f"Sensor 1: {temp1} C")

# 写入 PV
epics.caput('TEMP:Sensor1', 30.0)
print("Set Sensor 1 to 30.0 C")

# 监控 PV
def on_change(pvname=None, value=None, **kws):
    print(f"{pvname} changed to {value:.2f}")

pv1 = epics.PV('TEMP:Sensor1', callback=on_change)
pv2 = epics.PV('TEMP:Sensor2', callback=on_change)

print("Monitoring... Press Ctrl+C to stop")
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nStopped")
```

运行:
```bash
chmod +x monitor.py
./monitor.py
```

#### 本节问题
1. **Channel Access 使用哪个端口? 如何配置?**
   - 提示: 查找 `EPICS_CA_ADDR_LIST` 环境变量

2. **如果 IOC 和客户端不在同一台机器,如何访问?**
   - 提示: 涉及网络配置

3. **pyepics 的 PV.get() 和 epics.caget() 有什么区别?**
   - 实验并比较性能

### 周末: 总结与思考

#### 总结任务
1. **绘制概念图**: 画出 IOC、Record、PV、CA 之间的关系
2. **写学习笔记**: 记录本周学到的新概念
3. **整理问题**: 记录未解决的问题,下周寻找答案

#### 检查点 (自我评估)
- [ ] 能独立安装并配置 EPICS Base
- [ ] 能创建简单的数据库文件
- [ ] 能运行 IOC 并使用 CA 工具访问 PV
- [ ] 理解 ai、ao、calc、bi、bo 等基本记录类型
- [ ] 能用 Python 读写 PV

#### 挑战任务 (可选)
创建一个 PID 温度控制系统:
- 目标温度 (setpoint)
- 当前温度 (process variable)
- 加热器输出 (control output)
- 使用 EPICS 的 PID 记录或 calc 记录实现

---

## 第 2 周: 深入理解 EPICS 记录和数据库

### 本周目标
- ✅ 掌握常用记录类型的用法
- ✅ 理解记录处理流程和链接机制
- ✅ 学会使用宏和数据库模板

### 周一: 记录类型深入学习

#### 任务清单
- [ ] 学习 10 种以上记录类型
- [ ] 理解记录的字段含义
- [ ] 实验不同的扫描机制

#### 实践任务 4: 记录类型实验

创建 `record_types.db`:

```
# 1. 字符串输入/输出
record(stringin, "SYS:DeviceName") {
    field(VAL, "BPM-001")
}

record(stringout, "SYS:Status") {
    field(VAL, "OK")
}

# 2. 长整数输入/输出
record(longin, "SYS:Counter") {
    field(DESC, "Event Counter")
    field(SCAN, "1 second")
}

record(longout, "SYS:CounterReset") {
    field(DESC, "Reset Counter")
    field(OUT,  "SYS:Counter PP")  # PP = Process Passive
}

# 3. 多位二进制输入/输出 (用于状态字)
record(mbbi, "SYS:MachineState") {
    field(DESC, "Machine State")
    field(ZRVL, "0")  field(ZRST, "Idle")
    field(ONVL, "1")  field(ONST, "Ready")
    field(TWVL, "2")  field(TWST, "Running")
    field(THVL, "3")  field(THST, "Error")
}

# 4. 波形记录
record(waveform, "SYS:DataBuffer") {
    field(DESC, "Data Buffer")
    field(FTVL, "DOUBLE")  # Field Type of VaLue
    field(NELM, "1000")    # Number of ELeMents
}

# 5. 子数组记录 (从波形中提取)
record(subArray, "SYS:DataSubset") {
    field(DESC, "Data Subset")
    field(INP,  "SYS:DataBuffer")
    field(FTVL, "DOUBLE")
    field(MALM, "1000")    # Maximum Number of Elements
    field(NELM, "100")     # 提取 100 个元素
    field(INDX, "0")       # 起始索引
}

# 6. 序列记录 (顺序执行多个操作)
record(seq, "SYS:Sequence") {
    field(DESC, "Action Sequence")
    field(SELM, "All")     # SELect Mechanism: 全部执行
    field(DOL1, "1")       # Value 1
    field(LNK1, "SYS:Action1 PP")
    field(DOL2, "2")
    field(LNK2, "SYS:Action2 PP")
    field(DOL3, "3")
    field(LNK3, "SYS:Action3 PP")
}

record(longout, "SYS:Action1") {
    field(DESC, "Action 1")
}

record(longout, "SYS:Action2") {
    field(DESC, "Action 2")
}

record(longout, "SYS:Action3") {
    field(DESC, "Action 3")
}

# 7. fanout 记录 (触发多个记录)
record(fanout, "SYS:Trigger") {
    field(DESC, "Trigger Multiple Records")
    field(LNK1, "TEMP:Sensor1")
    field(LNK2, "TEMP:Sensor2")
    field(LNK3, "TEMP:Average")
    field(SELM, "All")
}
```

#### 本节问题
1. **记录的 VAL 字段何时更新?**
   - 提示: 理解记录处理流程

2. **FLNK、OUT、INP 字段的区别是什么?**
   - 实验: 创建记录链,观察处理顺序

3. **波形记录和数组有什么区别?**
   - 提示: EPICS 中的数组是有限大小的

4. **如何创建一个每秒递增的计数器?**
   - 提示: 使用 calc 记录自引用

### 周二-周三: 记录链接和处理机制

#### 任务清单
- [ ] 理解前向链接 (FLNK)
- [ ] 理解输入/输出链接 (INP/OUT)
- [ ] 掌握 PP (Process Passive) 机制

#### 实践任务 5: 构建处理链

创建 `processing_chain.db`:

```
# 场景: 信号处理流水线
# 原始信号 → 滤波 → 缩放 → 偏移 → 输出

# 步骤 1: 原始信号
record(ai, "SIGNAL:Raw") {
    field(DESC, "Raw Signal")
    field(SCAN, ".5 second")
    field(FLNK, "SIGNAL:Filtered")  # 前向链接
}

# 步骤 2: 低通滤波 (简单移动平均)
record(calc, "SIGNAL:Filtered") {
    field(DESC, "Filtered Signal")
    field(INPA, "SIGNAL:Raw")
    field(INPB, "SIGNAL:Filtered")  # 自引用,实现递归滤波
    field(CALC, "0.3*A + 0.7*B")    # 低通滤波器
    field(FLNK, "SIGNAL:Scaled")
}

# 步骤 3: 缩放
record(calc, "SIGNAL:Scaled") {
    field(DESC, "Scaled Signal")
    field(INPA, "SIGNAL:Filtered")
    field(INPB, "SIGNAL:ScaleFactor")
    field(CALC, "A*B")
    field(FLNK, "SIGNAL:Offset")
}

record(ao, "SIGNAL:ScaleFactor") {
    field(DESC, "Scale Factor")
    field(VAL,  "2.0")
}

# 步骤 4: 偏移
record(calc, "SIGNAL:Offset") {
    field(DESC, "Signal with Offset")
    field(INPA, "SIGNAL:Scaled")
    field(INPB, "SIGNAL:OffsetValue")
    field(CALC, "A+B")
    field(FLNK, "SIGNAL:Output")
}

record(ao, "SIGNAL:OffsetValue") {
    field(DESC, "Offset Value")
    field(VAL,  "10.0")
}

# 步骤 5: 输出
record(ai, "SIGNAL:Output") {
    field(DESC, "Final Output")
    field(INP,  "SIGNAL:Offset")
}
```

#### 实验步骤
```bash
# 1. 运行 IOC
softIoc -d processing_chain.db

# 2. 监控整个处理链
camonitor SIGNAL:Raw SIGNAL:Filtered SIGNAL:Scaled SIGNAL:Offset SIGNAL:Output

# 3. 修改参数,观察影响
caput SIGNAL:ScaleFactor 5.0
caput SIGNAL:OffsetValue 20.0

# 4. 手动设置原始信号
caput SIGNAL:Raw 100
```

#### 本节问题
1. **如果删除 FLNK,使用 SCAN "1 second" 会有什么不同?**
   - 实验并比较时序

2. **PP、NPP、MS、NMS 链接标志的含义是什么?**
   - 提示: Process Passive, No Process Passive, Maximize Severity...

3. **如何避免循环链接造成的死锁?**
   - 实验: 创建 A → B → A 的循环,观察结果

4. **FLNK 和 OUT 字段都能触发其他记录,选择标准是什么?**

### 周四-周五: 宏和数据库模板

#### 任务清单
- [ ] 学习宏替换语法
- [ ] 创建可复用的数据库模板
- [ ] 理解实例化过程

#### 实践任务 6: 创建传感器模板

创建 `sensor.template`:

```
# 传感器通用模板
# 宏参数:
#   $(P)      - 前缀
#   $(NAME)   - 传感器名称
#   $(SCAN)   - 扫描周期
#   $(EGU)    - 工程单位
#   $(PREC)   - 精度
#   $(HOPR)   - 高量程
#   $(LOPR)   - 低量程
#   $(HIGH)   - 高告警阈值
#   $(LOW)    - 低告警阈值

# 传感器读数
record(ai, "$(P):$(NAME):Value") {
    field(DESC, "$(NAME) Value")
    field(SCAN, "$(SCAN)")
    field(PREC, "$(PREC)")
    field(EGU,  "$(EGU)")
    field(HOPR, "$(HOPR)")
    field(LOPR, "$(LOPR)")
    field(HIGH, "$(HIGH)")
    field(LOW,  "$(LOW)")
    field(HSV,  "MAJOR")  # High Severity
    field(LSV,  "MAJOR")  # Low Severity
    field(FLNK, "$(P):$(NAME):Status")
}

# 传感器状态
record(calc, "$(P):$(NAME):Status") {
    field(DESC, "$(NAME) Status")
    field(INPA, "$(P):$(NAME):Value")
    field(INPB, "$(HIGH)")
    field(INPC, "$(LOW)")
    field(CALC, "(A>B)||(A<C)")  # 1=告警, 0=正常
}

# 传感器使能/禁用
record(bo, "$(P):$(NAME):Enable") {
    field(DESC, "$(NAME) Enable")
    field(ZNAM, "Disabled")
    field(ONAM, "Enabled")
    field(VAL,  "1")
}

# 最小值记录
record(calc, "$(P):$(NAME):Min") {
    field(DESC, "$(NAME) Minimum")
    field(INPA, "$(P):$(NAME):Value")
    field(INPB, "$(P):$(NAME):Min")
    field(CALC, "B=0?A:MIN(A,B)")  # 初始化或取最小值
    field(PREC, "$(PREC)")
}

# 最大值记录
record(calc, "$(P):$(NAME):Max") {
    field(DESC, "$(NAME) Maximum")
    field(INPA, "$(P):$(NAME):Value")
    field(INPB, "$(P):$(NAME):Max")
    field(CALC, "MAX(A,B)")
    field(PREC, "$(PREC)")
}

# 复位最小/最大值
record(bo, "$(P):$(NAME):ResetMinMax") {
    field(DESC, "Reset Min/Max")
    field(ZNAM, "Normal")
    field(ONAM, "Reset")
}
```

创建实例文件 `sensors.substitutions`:

```
# 温度传感器实例
file "sensor.template" {
    pattern { P,        NAME,    SCAN,        EGU, PREC, HOPR,  LOPR, HIGH, LOW }
            { "LAB",    "Temp1", "1 second",  "C", "2",  "100", "0",  "80", "10" }
            { "LAB",    "Temp2", "1 second",  "C", "2",  "100", "0",  "80", "10" }
            { "LAB",    "Temp3", "1 second",  "C", "2",  "100", "0",  "80", "10" }
}

# 压力传感器实例
file "sensor.template" {
    pattern { P,        NAME,      SCAN,       EGU,   PREC, HOPR,   LOPR, HIGH,  LOW }
            { "LAB",    "Press1",  ".5 second", "Pa", "1",  "1000", "0",  "900", "100" }
            { "LAB",    "Press2",  ".5 second", "Pa", "1",  "1000", "0",  "900", "100" }
}
```

生成实例化数据库:
```bash
msi -S sensors.substitutions > sensors.db
```

查看生成的 `sensors.db`,你会看到所有实例展开的结果。

#### 本节问题
1. **宏替换何时发生? 编译时还是运行时?**
   - 提示: 使用 `msi` 工具

2. **如果模板中的宏没有提供值会怎样?**
   - 实验: 故意遗漏一个宏参数

3. **如何在启动脚本中直接使用宏?**
   - 提示: `dbLoadRecords("file.db", "P=XXX,Q=YYY")`

4. **模板和子程序 (subroutine record) 的区别?**

### 周末: BPMIOC 代码初探

#### 任务清单
- [ ] 克隆或获取 BPMIOC 代码
- [ ] 浏览目录结构
- [ ] 识别关键文件

#### 实践任务 7: 代码导览

```bash
cd ~/BPMIOC

# 1. 查看目录结构
tree -L 3 -I 'O.*|*.d'

# 2. 统计代码行数
wc -l BPMmonitorApp/src/*.c
wc -l BPMmonitorApp/src/*.h
wc -l BPMmonitorApp/Db/*.db

# 3. 查找关键函数
grep -n "ReadData" BPMmonitorApp/src/*.c
grep -n "OFFSET_" BPMmonitorApp/src/*.h

# 4. 查看数据库记录数量
grep -c "^record" BPMmonitorApp/Db/BPMMonitor.db
```

#### 思考问题
1. **BPMIOC 有多少个 offset 定义?**
2. **主要的全局数据结构有哪些?**
3. **设备支持层支持哪些记录类型?**

#### 检查点
- [ ] 掌握 10 种以上记录类型
- [ ] 能创建记录处理链
- [ ] 理解 FLNK、INP、OUT 的区别
- [ ] 能编写数据库模板
- [ ] 对 BPMIOC 代码结构有初步认识

---

## 第 3 周: 编译运行 BPMIOC

### 本周目标
- ✅ 成功编译 BPMIOC
- ✅ 理解 EPICS 构建系统
- ✅ 模拟硬件运行 IOC

### 周一-周二: 理解 EPICS 构建系统

#### 任务清单
- [ ] 学习 EPICS Makefile 结构
- [ ] 理解 configure 目录的作用
- [ ] 掌握构建依赖关系

#### 学习任务 8: Makefile 剖析

**顶层 Makefile** (`BPMIOC/Makefile`):
```makefile
# 定义子目录
DIRS := configure
DIRS += $(wildcard *App)
DIRS += iocBoot

# 定义依赖关系
BPMmonitorApp_DEPEND_DIRS = configure
iocBoot_DEPEND_DIRS = BPMmonitorApp

include $(TOP)/configure/RULES_TOP
```

**问题**:
1. **为什么 iocBoot 依赖 BPMmonitorApp?**
2. **如果添加新的 App 目录,需要修改哪里?**

**应用 Makefile** (`BPMmonitorApp/src/Makefile`):
```makefile
# 产品定义
PROD_IOC = BPMmonitor

# 数据库定义
DBD += BPMmonitor.dbd
BPMmonitor_DBD += base.dbd
BPMmonitor_DBD += devBPMMonitor.dbd

# 源文件
BPMmonitor_SRCS += driverWrapper.c
BPMmonitor_SRCS += devBPMMonitor.c
BPMmonitor_SRCS += BPMmonitor_registerRecordDeviceDriver.cpp
BPMmonitor_SRCS += BPMmonitorMain.cpp

# 库链接
BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)
BPMmonitor_SYS_LIBS += dl pthread
```

**问题**:
1. **为什么需要链接 dl 和 pthread?**
   - 提示: 查看 driverWrapper.c 中的 dlopen()

2. **registerRecordDeviceDriver.cpp 从哪里来?**
   - 提示: 这是自动生成的

3. **如何添加新的源文件?**

#### 实践任务 9: 修改 RELEASE 配置

```bash
cd ~/BPMIOC
vim configure/RELEASE
```

修改为:
```makefile
# EPICS Base 路径 (改为你的实际路径)
EPICS_BASE=/home/yourusername/base-3.15.6

# 如果有其他模块依赖,在此添加
# 例如:
# ASYN=$(EPICS_BASE)/../asyn-4-35
```

保存后:
```bash
make clean
make
```

### 周三-周四: 编译 BPMIOC

#### 任务清单
- [ ] 配置编译环境
- [ ] 解决编译错误
- [ ] 验证编译产物

#### 实践步骤

```bash
cd ~/BPMIOC

# 1. 清理旧的编译文件
make clean

# 2. 编译 (使用 4 个并行任务)
make -j4

# 3. 查看编译输出
ls -lh bin/*/BPMmonitor
ls -lh db/*.db
ls -lh dbd/*.dbd
```

#### 可能遇到的问题

**问题 1**: `liblowlevel.so` 找不到
```
解决: 这是正常的!编译不需要这个库,只有运行时需要。
```

**问题 2**: 编译器警告
```c
// 例如: unused variable
// 解决: 可以先忽略,或添加 (void)variable; 消除警告
```

**问题 3**: 权限错误
```bash
# 检查文件权限
ls -l configure/
chmod +x configure/*
```

#### 本节问题
1. **编译生成了哪些文件?**
   - 列出 bin、lib、dbd、db 目录内容

2. **如果只修改了 .db 文件,需要完整重新编译吗?**
   - 实验: 修改 BPMMonitor.db,然后 make

3. **如何查看编译过程的详细命令?**
   - 提示: 查找 make 的 verbose 选项

### 周五: 模拟硬件运行

由于没有真实的 FPGA 硬件,我们需要修改代码以模拟运行。

#### 任务清单
- [ ] 修改驱动层支持模拟模式
- [ ] 运行 IOC
- [ ] 验证 PV 访问

#### 实践任务 10: 添加模拟模式

修改 `BPMmonitorApp/src/driverWrapper.c`:

```c
// 在文件顶部添加全局标志
static int use_simulation = 0;

// 修改 InitDevice()
int InitDevice()
{
    printf("=== BPM Monitor Driver Initialization ===\n");

    // 尝试加载硬件库
    handle = dlopen("/usr/lib/liblowlevel.so", RTLD_LAZY);

    if (!handle) {
        printf("WARNING: Cannot load liblowlevel.so: %s\n", dlerror());
        printf("WARNING: Using SIMULATION mode\n");
        use_simulation = 1;

        // 初始化 I/O 中断
        scanIoInit(&ioScanPvt);

        // 创建后台线程
        epicsThreadCreate("BPMMonitor", 50, 20000,
                          (EPICSTHREADFUNC)my_thread, NULL);

        return 0;
    }

    // ... 原有的硬件初始化代码 ...
}

// 修改 my_thread()
static void my_thread(void *arg)
{
    static double sim_time = 0.0;

    while (1) {
        if (use_simulation) {
            // 模拟数据: 8 个 RF 通道
            for (int i = 0; i < 8; i++) {
                // 振幅: 1.0 ± 0.5 正弦波
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);

                // 相位: 线性增加 + 通道偏移
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);

                // 模拟触发波形
                for (int j = 0; j < 10000; j++) {
                    TrigWaveform[i][j] = sin(j * 0.01 + i * 0.1) * Amp[i];
                }
            }

            sim_time += 0.1;  // 时间步进

            // 更新时间戳 (使用系统时间)
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            TAIsec = ts.tv_sec;
            TAInsec = ts.tv_nsec;

        } else {
            // 原有的硬件读取代码
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }
            (*getTimeStampFunc)(&TAIsec, &TAInsec);
        }

        // 触发 I/O 中断
        scanIoRequest(ioScanPvt);

        // 休眠 100ms
        epicsThreadSleep(0.1);
    }
}
```

添加必要的头文件:
```c
#include <time.h>
#include <math.h>
```

重新编译:
```bash
cd ~/BPMIOC
make clean
make
```

#### 运行 IOC

```bash
cd iocBoot/iocBPMmonitor
./st.cmd
```

你应该看到:
```
=== BPM Monitor Driver Initialization ===
WARNING: Cannot load liblowlevel.so: ...
WARNING: Using SIMULATION mode
Starting iocInit
...
iocInit: All initialization complete
```

#### 测试 PV

在新终端:
```bash
# 读取 RF 振幅
caget iLinac_007:BPM14And15:RF3Amp
caget iLinac_007:BPM14And15:RF4Amp

# 监控变化
camonitor iLinac_007:BPM14And15:RF3Amp iLinac_007:BPM14And15:RF3Phase

# 读取波形 (使用 Python)
python3 << EOF
import epics
waveform = epics.caget('iLinac_007:BPM14And15:RF3TrigWaveform')
print(f"Waveform length: {len(waveform)}")
print(f"First 10 points: {waveform[:10]}")
EOF
```

### 周末: 代码阅读与注释

#### 任务清单
- [ ] 阅读 driverWrapper.c (至少理解 50%)
- [ ] 阅读 devBPMMonitor.c (至少理解 50%)
- [ ] 添加注释帮助理解

#### 实践任务 11: 添加个人注释

在代码中添加你自己的理解:

```c
// driverWrapper.c

// [个人注释] 这个函数通过 offset 统一接口,避免为每种数据写单独函数
double ReadData(int offset, int channel, int type)
{
    // [个人注释] 使用 switch-case 实现多态,类似面向对象的虚函数
    switch(offset) {
        case OFFSET_AMP:
            // [个人注释] 直接从全局缓冲读取,后台线程负责更新
            return Amp[channel];
        // ...
    }
}
```

#### 检查点
- [ ] 成功编译 BPMIOC
- [ ] 在模拟模式下运行 IOC
- [ ] 能访问所有主要 PV
- [ ] 理解构建系统的基本原理
- [ ] 对代码有 50% 以上的理解

---

## 第 4 周: 深入驱动层 (driverWrapper.c)

### 本周目标
- ✅ 完全理解 driverWrapper.c 的每个函数
- ✅ 掌握 C 语言高级特性 (函数指针、动态库)
- ✅ 理解多线程和同步机制

### 周一: 函数指针和动态库

#### 学习任务
- [ ] 复习函数指针语法
- [ ] 学习 dlopen/dlsym 用法
- [ ] 理解动态链接原理

#### 实践任务 12: 动态库实验

创建一个简单的动态库:

`libmath_ops.c`:
```c
#include <stdio.h>

double add(double a, double b) {
    printf("add() called: %f + %f\n", a, b);
    return a + b;
}

double multiply(double a, double b) {
    printf("multiply() called: %f * %f\n", a, b);
    return a * b;
}

double power(double base, double exp) {
    printf("power() called: %f ^ %f\n", base, exp);
    double result = 1.0;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}
```

编译为动态库:
```bash
gcc -shared -fPIC -o libmath_ops.so libmath_ops.c
```

创建加载器 `dynamic_loader.c`:
```c
#include <stdio.h>
#include <dlfcn.h>

// 定义函数指针类型
typedef double (*MathFunc)(double, double);

int main() {
    void *handle;
    MathFunc add_func, multiply_func, power_func;
    char *error;

    // 1. 打开动态库
    handle = dlopen("./libmath_ops.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return 1;
    }

    // 2. 清除之前的错误
    dlerror();

    // 3. 获取函数地址
    *(void **)(&add_func) = dlsym(handle, "add");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "dlsym error: %s\n", error);
        return 1;
    }

    *(void **)(&multiply_func) = dlsym(handle, "multiply");
    *(void **)(&power_func) = dlsym(handle, "power");

    // 4. 调用函数
    printf("Result 1: %f\n", (*add_func)(3.0, 4.0));
    printf("Result 2: %f\n", (*multiply_func)(3.0, 4.0));
    printf("Result 3: %f\n", (*power_func)(2.0, 10.0));

    // 5. 关闭库
    dlclose(handle);

    return 0;
}
```

编译并运行:
```bash
gcc -o dynamic_loader dynamic_loader.c -ldl
./dynamic_loader
```

#### 本节问题
1. **为什么使用 `*(void **)(&func)` 而不是直接 `func = dlsym(...)`?**
   - 提示: ISO C 禁止函数指针和 void* 之间的转换

2. **RTLD_LAZY 和 RTLD_NOW 的区别?**
   - 实验: 改为 RTLD_NOW,观察加载时间

3. **如果动态库依赖其他库怎么办?**
   - 提示: 查看 LD_LIBRARY_PATH

4. **如何查看可执行文件依赖的动态库?**
   ```bash
   ldd ./BPMmonitor
   ```

### 周二-周三: 深入分析 driverWrapper.c

#### 任务清单
- [ ] 理解所有全局数据结构
- [ ] 分析 InitDevice() 流程
- [ ] 理解 ReadData() 和 SetReg() 机制

#### 分析任务 13: 数据结构清单

创建文档 `data_structures.md`:

```markdown
# driverWrapper.c 数据结构清单

## 全局缓冲区

| 变量名 | 类型 | 大小 | 用途 | 更新频率 |
|--------|------|------|------|----------|
| Amp[8] | double | 8 | RF 振幅 | 100ms (后台线程) |
| Phase[8] | double | 8 | RF 相位 | 100ms |
| TrigWaveform[8][10000] | float | 8×10000 | 触发波形 | 触发时 |
| AVG_Voltage[8] | double | 8 | 平均电压 | 计算后 |
| AVGStart[8] | int | 8 | 平均起始位置 | 用户设置 |
| AVGStop[8] | int | 8 | 平均结束位置 | 用户设置 |
| ... | ... | ... | ... | ... |

## 函数指针

| 函数指针 | 原型 | 用途 |
|----------|------|------|
| systemInitFunc | int (*)(void) | 初始化 FPGA 系统 |
| getRfInfoFunc | int (*)(int, double*, double*) | 读取 RF 信息 |
| ... | ... | ... |

## Offset 映射

| Offset | 名称 | 描述 |
|--------|------|------|
| 0 | OFFSET_AMP | RF 振幅 |
| 1 | OFFSET_PHASE | RF 相位 |
| 34 | OFFSET_AVG_VOLTAGE | 平均电压 |
| ... | ... | ... |
```

**任务**: 补全所有数据结构和 offset 定义。

#### 分析任务 14: 函数调用图

绘制 `InitDevice()` 的调用流程:

```
InitDevice()
├─ dlopen("liblowlevel.so")
├─ dlsym("SystemInit") → systemInitFunc
├─ dlsym("GetRfInfo") → getRfInfoFunc
├─ ... (50+ 函数指针)
├─ (*systemInitFunc)()
├─ Amp2PowerInit()
│  └─ 读取 CSV 校准文件
├─ LoadParam()
│  └─ 读取 BPM 参数文件
├─ scanIoInit(&ioScanPvt)
└─ epicsThreadCreate("BPMMonitor", ..., my_thread, ...)
    └─ 启动后台线程
```

#### 本节问题
1. **为什么需要这么多全局变量? 能改用结构体吗?**
   - 思考: 如何重构代码

2. **后台线程每 100ms 扫描一次,会不会太频繁?**
   - 计算: CPU 占用率估算

3. **如果两个线程同时访问 Amp[],会有竞争吗?**
   - 分析: 是否需要加锁

4. **calculateAvgVoltage() 何时被调用?**
   - 追踪调用路径

### 周四-周五: 实现新功能

#### 任务清单
- [ ] 添加 RF 标准差计算
- [ ] 添加数据有效性检查
- [ ] 测试新功能

#### 实践任务 15: RF 稳定性监测

需求: 计算最近 100 个采样点的振幅标准差,评估信号稳定性。

**步骤 1**: 在 `driverWrapper.c` 中添加数据结构

```c
// 滑动窗口大小
#define WINDOW_SIZE 100

// 振幅历史记录
static double AmpHistory[8][WINDOW_SIZE];
static int AmpHistoryIndex[8] = {0};  // 当前写入位置
static int AmpHistoryfull[8] = {0};   // 是否已填满

// 标准差结果
static double AmpStdDev[8] = {0.0};
```

**步骤 2**: 添加计算函数

```c
// 计算标准差
void calculateStdDev(int channel)
{
    if (channel < 0 || channel >= 8) return;

    int count = AmpHistoryfull[channel] ? WINDOW_SIZE : AmpHistoryIndex[channel];
    if (count < 2) {
        AmpStdDev[channel] = 0.0;
        return;
    }

    // 1. 计算平均值
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += AmpHistory[channel][i];
    }
    double mean = sum / count;

    // 2. 计算方差
    double variance = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = AmpHistory[channel][i] - mean;
        variance += diff * diff;
    }
    variance /= count;

    // 3. 标准差
    AmpStdDev[channel] = sqrt(variance);
}
```

**步骤 3**: 在后台线程中更新

```c
static void my_thread(void *arg)
{
    while (1) {
        // ... 读取 RF 数据 ...

        // 更新历史记录和计算标准差
        for (int i = 0; i < 8; i++) {
            // 保存到历史缓冲
            AmpHistory[i][AmpHistoryIndex[i]] = Amp[i];

            // 更新索引
            AmpHistoryIndex[i]++;
            if (AmpHistoryIndex[i] >= WINDOW_SIZE) {
                AmpHistoryIndex[i] = 0;
                AmpHistoryfull[i] = 1;
            }

            // 计算标准差
            calculateStdDev(i);
        }

        // ... 剩余代码 ...
    }
}
```

**步骤 4**: 在 driverWrapper.h 中添加 offset

```c
#define OFFSET_AMP_STDDEV   37  // 振幅标准差
```

**步骤 5**: 在 ReadData() 中添加 case

```c
case OFFSET_AMP_STDDEV:
    value = AmpStdDev[channel];
    break;
```

**步骤 6**: 在数据库中添加记录 (BPMMonitor.db)

```
record(ai, "$(P):RF3AmpStdDev") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@AMP_STDDEV:37 ch=0")
    field(SCAN, "I/O Intr")
    field(PREC, "4")
    field(EGU,  "V")
    field(DESC, "RF3 Amplitude Std Dev")
}
```

为所有 8 个通道添加类似记录。

**步骤 7**: 编译测试

```bash
make clean
make
cd iocBoot/iocBPMmonitor
./st.cmd

# 新终端测试
camonitor iLinac_007:BPM14And15:RF3Amp iLinac_007:BPM14And15:RF3AmpStdDev
```

#### 本节问题
1. **为什么用循环缓冲而不是每次移动数组?**
   - 分析: 时间复杂度

2. **滑动窗口大小如何选择?**
   - 思考: 太小和太大的影响

3. **能否用在线算法避免每次遍历整个窗口?**
   - 研究: Welford's online algorithm

### 周末: 性能分析

#### 任务清单
- [ ] 测量后台线程的 CPU 占用
- [ ] 分析内存使用
- [ ] 优化热点代码

#### 分析任务 16: 性能测试

添加计时代码:

```c
#include <sys/time.h>

static void my_thread(void *arg)
{
    struct timeval start, end;

    while (1) {
        gettimeofday(&start, NULL);

        // ... 原有代码 ...

        gettimeofday(&end, NULL);

        long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 +
                          (end.tv_usec - start.tv_usec);

        if (elapsed_us > 50000) {  // 超过 50ms 报警
            printf("WARNING: Thread took %ld us\n", elapsed_us);
        }

        epicsThreadSleep(0.1);
    }
}
```

#### 检查点
- [ ] 理解 driverWrapper.c 中的每个函数
- [ ] 能独立添加新的监测功能
- [ ] 理解动态库加载机制
- [ ] 掌握性能分析方法

---

## 第 5 周: 深入设备支持层 (devBPMMonitor.c)

### 本周目标
- ✅ 理解 EPICS 设备支持规范
- ✅ 掌握 DSET 结构和生命周期
- ✅ 能编写新的设备支持

### 周一-周二: DSET 详解

#### 学习任务
- [ ] 理解设备支持表 (DSET) 结构
- [ ] 学习每个函数的调用时机
- [ ] 掌握私有数据结构的使用

#### 分析任务 17: DSET 生命周期

绘制时序图:

```
IOC 启动
  │
  ├─ 加载数据库
  │   │
  │   ├─ 遇到 ai 记录,DTYP="BPMMonitor"
  │   │   │
  │   │   ├─ 调用 devAiBPMMonitor.init(0)  [首次调用]
  │   │   │
  │   │   ├─ 调用 init_ai_record(prec)
  │   │   │   ├─ 解析 INP 字段
  │   │   │   ├─ 创建私有数据 (BPMMonitorPvt)
  │   │   │   └─ 保存到 prec->dpvt
  │   │   │
  │   │   └─ 如果 SCAN="I/O Intr"
  │   │       └─ 调用 get_ioint_info(prec, &ioscanpvt)
  │   │
  │   └─ (对每个 ai 记录重复)
  │
  ├─ iocInit()
  │   ├─ 调用 InitDevice()  [驱动层初始化]
  │   └─ 启动扫描线程
  │
  └─ 运行时
      ├─ 定时扫描 或 I/O 中断触发
      │   └─ 调用 read_ai(prec)
      │       ├─ 从 prec->dpvt 获取私有数据
      │       ├─ 调用 ReadData(offset, channel, type)
      │       ├─ 更新 prec->val
      │       └─ 返回 status
      │
      └─ (持续运行)
```

#### 实践任务 18: 添加调试输出

在 `devBPMMonitor.c` 中每个函数添加调试输出:

```c
#include <epicsExport.h>

// 调试级别: 0=关闭, 1=基本, 2=详细
int BPMMonitorDebug = 0;
epicsExportAddress(int, BPMMonitorDebug);

static long init_ai_record(struct aiRecord *prec)
{
    if (BPMMonitorDebug >= 1) {
        printf("[init_ai_record] %s\n", prec->name);
    }

    // ... 原有代码 ...

    if (BPMMonitorDebug >= 2) {
        printf("  type=%s, offset=%d, channel=%d\n",
               pPvt->type_str, pPvt->offset, pPvt->channel);
    }

    return 0;
}

static long read_ai(struct aiRecord *prec)
{
    if (BPMMonitorDebug >= 2) {
        printf("[read_ai] %s\n", prec->name);
    }

    // ... 原有代码 ...

    if (BPMMonitorDebug >= 2) {
        printf("  value=%f\n", prec->val);
    }

    return 2;
}
```

编译后测试:

```bash
./st.cmd
epics> var BPMMonitorDebug 1   # 开启调试
epics> var BPMMonitorDebug 0   # 关闭调试
```

#### 本节问题
1. **为什么 init_ai_record() 返回 0,而 read_ai() 返回 2?**
   - 查阅: EPICS Device Support Reference

2. **私有数据结构能包含指针吗? 需要释放吗?**
   - 思考: 内存生命周期

3. **如果 init_ai_record() 返回错误,记录会怎样?**
   - 实验: 故意返回 -1

### 周三-周四: 参数解析机制

#### 任务清单
- [ ] 理解 INST_IO 链接类型
- [ ] 掌握 sscanf 解析技巧
- [ ] 支持更复杂的参数格式

#### 实践任务 19: 扩展参数格式

当前格式: `"TYPE:offset ch=channel"`

新增需求: 支持可选的 scale 和 offset 参数
新格式: `"TYPE:offset ch=channel scale=X offset=Y"`

**步骤 1**: 修改私有数据结构

```c
typedef struct {
    char type_str[32];
    int type;
    int offset;
    int channel;
    double scale;      // 新增: 缩放因子 (默认 1.0)
    double offset_val; // 新增: 偏移值 (默认 0.0)
} BPMMonitorPvt;
```

**步骤 2**: 修改解析函数

```c
static long init_ai_record(struct aiRecord *prec)
{
    // ... 原有代码 ...

    // 解析基本参数
    int parsed = sscanf(params, "%[^:]:%d ch=%d",
                        pPvt->type_str, &pPvt->offset, &pPvt->channel);

    if (parsed < 3) {
        printf("ERROR: Failed to parse parameters: %s\n", params);
        return -1;
    }

    // 默认值
    pPvt->scale = 1.0;
    pPvt->offset_val = 0.0;

    // 查找可选参数
    char *scale_str = strstr(params, "scale=");
    if (scale_str) {
        sscanf(scale_str, "scale=%lf", &pPvt->scale);
    }

    char *offset_str = strstr(params, "offset=");
    if (offset_str) {
        sscanf(offset_str, "offset=%lf", &pPvt->offset_val);
    }

    if (BPMMonitorDebug >= 1) {
        printf("  scale=%f, offset=%f\n", pPvt->scale, pPvt->offset_val);
    }

    // ... 剩余代码 ...
}
```

**步骤 3**: 在读取函数中应用

```c
static long read_ai(struct aiRecord *prec)
{
    BPMMonitorPvt *pPvt = (BPMMonitorPvt *)prec->dpvt;

    // 从驱动层读取原始值
    double raw_value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 应用缩放和偏移
    double scaled_value = raw_value * pPvt->scale + pPvt->offset_val;

    prec->val = scaled_value;
    prec->udf = FALSE;

    return 2;
}
```

**步骤 4**: 在数据库中使用

```
record(ai, "$(P):RF3AmpScaled") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@AMP:0 ch=0 scale=2.0 offset=10.0")
    field(SCAN, "I/O Intr")
    field(PREC, "3")
    field(EGU,  "V")
}
```

#### 本节问题
1. **为什么用字符串而不是结构体传递参数?**
   - 思考: EPICS 的设计哲学

2. **如果参数很复杂 (如 JSON),如何解析?**
   - 研究: cJSON 库

3. **参数解析失败应该如何处理?**
   - 思考: 错误处理策略

### 周五: 添加新的记录类型支持

#### 任务清单
- [ ] 添加 stringin 记录支持
- [ ] 实现设备信息读取
- [ ] 测试新功能

#### 实践任务 20: 支持 stringin 记录

需求: 添加 PV 显示 IOC 版本信息、运行时间等。

**步骤 1**: 在 driverWrapper.c 添加信息接口

```c
// 版本信息
#define IOC_VERSION "1.0.0"

// 获取字符串信息
// type: 0=version, 1=build_date, 2=uptime
const char* GetStringInfo(int type)
{
    static char buffer[256];
    static time_t start_time = 0;

    if (start_time == 0) {
        start_time = time(NULL);
    }

    switch(type) {
        case 0:  // 版本
            return IOC_VERSION;

        case 1:  // 编译日期
            return __DATE__ " " __TIME__;

        case 2:  // 运行时间
            time_t now = time(NULL);
            int uptime = (int)(now - start_time);
            snprintf(buffer, sizeof(buffer),
                     "%d days, %d hours, %d minutes",
                     uptime / 86400,
                     (uptime % 86400) / 3600,
                     (uptime % 3600) / 60);
            return buffer;

        default:
            return "Unknown";
    }
}
```

在 driverWrapper.h 中声明:
```c
const char* GetStringInfo(int type);
```

**步骤 2**: 在 devBPMMonitor.c 添加 stringin 支持

```c
// stringin 记录初始化
static long init_stringin_record(struct stringinRecord *prec)
{
    struct instio *pinstio;
    BPMMonitorPvt *pPvt;

    pPvt = (BPMMonitorPvt *)malloc(sizeof(BPMMonitorPvt));
    prec->dpvt = pPvt;

    pinstio = (struct instio *)&(prec->inp.value);
    sscanf(pinstio->string, "%[^:]:%d",
           pPvt->type_str, &pPvt->offset);

    return 0;
}

// stringin 记录读取
static long read_stringin(struct stringinRecord *prec)
{
    BPMMonitorPvt *pPvt = (BPMMonitorPvt *)prec->dpvt;

    const char *str = GetStringInfo(pPvt->offset);
    strncpy(prec->val, str, sizeof(prec->val) - 1);
    prec->val[sizeof(prec->val) - 1] = '\0';

    prec->udf = FALSE;
    return 0;
}

// DSET
STRINGIN_DSET devStringinBPMMonitor = {
    5,
    NULL,
    NULL,
    init_stringin_record,
    NULL,
    read_stringin
};
```

**步骤 3**: 在 .dbd 文件中注册

```c
device(stringin, INST_IO, devStringinBPMMonitor, "BPMMonitor")
```

**步骤 4**: 在数据库中添加记录

```
record(stringin, "$(P):IOCVersion") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@STRING:0")
    field(SCAN, "Passive")
    field(PINI, "YES")
}

record(stringin, "$(P):BuildDate") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@STRING:1")
    field(SCAN, "Passive")
    field(PINI, "YES")
}

record(stringin, "$(P):Uptime") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@STRING:2")
    field(SCAN, "1 second")
}
```

**步骤 5**: 编译测试

```bash
make
./st.cmd

# 测试
epics> dbgf "iLinac_007:BPM14And15:IOCVersion"
epics> dbgf "iLinac_007:BPM14And15:Uptime"
```

### 周末: 综合项目

#### 综合任务 21: 实现告警系统

需求: 当 RF 振幅超出范围时,自动触发告警。

**功能要求**:
1. 每个 RF 通道有高/低阈值
2. 告警状态 PV (0=正常, 1=高告警, 2=低告警)
3. 告警计数器
4. 告警复位功能

**提示**:
- 需要修改驱动层和设备支持层
- 使用 bi 或 mbbi 记录显示状态
- 使用 longin 记录显示计数
- 使用 bo 记录复位计数

自己设计并实现!

#### 检查点
- [ ] 理解 DSET 的完整生命周期
- [ ] 能添加新记录类型支持
- [ ] 掌握参数解析技巧
- [ ] 完成告警系统实现

---

## 第 6 周: 数据库高级技巧

### 本周目标
- ✅ 掌握数据库高级特性
- ✅ 学会使用 subroutine 记录
- ✅ 理解告警和归档

### 周一-周二: 告警和归档

#### 学习任务
- [ ] 理解 EPICS 告警机制
- [ ] 配置告警阈值
- [ ] 学习归档系统 (Channel Archiver)

#### 实践任务 22: 配置完整告警

为 RF 监测添加多级告警:

```
record(ai, "$(P):RF3Amp") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@AMP:0 ch=0")
    field(SCAN, "I/O Intr")
    field(PREC, "3")
    field(EGU,  "V")

    # 运行范围
    field(HOPR, "2.0")
    field(LOPR, "0.0")

    # 告警限值
    field(HIHI, "1.8")   # 非常高
    field(HIGH, "1.5")   # 高
    field(LOW,  "0.3")   # 低
    field(LOLO, "0.1")   # 非常低

    # 告警严重性
    field(HHSV, "MAJOR")    # HIHI 严重性
    field(HSV,  "MINOR")    # HIGH 严重性
    field(LSV,  "MINOR")    # LOW 严重性
    field(LLSV, "MAJOR")    # LOLO 严重性

    # 告警死区 (避免抖动)
    field(HYST, "0.05")
}
```

#### 实践任务 23: 告警响应记录

创建告警响应链:

```
# RF 振幅
record(ai, "$(P):RF3Amp") {
    field(HIHI, "1.8")
    field(HHSV, "MAJOR")
    field(FLNK, "$(P):RF3AlarmCheck")
}

# 告警检查
record(calc, "$(P):RF3AlarmCheck") {
    field(INPA, "$(P):RF3Amp.SEVR")  # 读取严重性
    field(CALC, "A>=2")  # MAJOR=2, MINOR=1
    field(FLNK, "$(P):RF3AlarmCounter")
}

# 告警计数
record(calc, "$(P):RF3AlarmCounter") {
    field(INPA, "$(P):RF3AlarmCheck")
    field(INPB, "$(P):RF3AlarmCounter")
    field(CALC, "B+A")
}

# 告警复位
record(bo, "$(P):RF3AlarmReset") {
    field(OUT,  "$(P):RF3AlarmCounter PP")
    field(OMSL, "supervisory")
    field(DOL,  "0")
}
```

### 周三-周四: Subroutine 记录

#### 学习任务
- [ ] 理解 subroutine 记录
- [ ] 编写 C 函数实现自定义逻辑
- [ ] 注册到 EPICS

#### 实践任务 24: 自定义计算函数

需求: 实现一个复杂的信号质量评分算法。

**步骤 1**: 创建 subroutine 源文件

`BPMmonitorApp/src/signalQuality.c`:

```c
#include <stdio.h>
#include <math.h>
#include <subRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>

// 信号质量评分函数
// A: 振幅
// B: 标准差
// C: 相位稳定性
// 返回: 0-100 的质量分数
static long calculateQuality(struct subRecord *prec)
{
    double amp = prec->a;
    double stddev = prec->b;
    double phase_stability = prec->c;

    double quality = 100.0;

    // 1. 振幅评分 (期望 1.0 ± 0.2)
    double amp_error = fabs(amp - 1.0);
    if (amp_error > 0.2) {
        quality -= (amp_error - 0.2) * 50.0;
    }

    // 2. 稳定性评分 (标准差越小越好)
    quality -= stddev * 100.0;

    // 3. 相位评分 (期望接近 0)
    quality -= fabs(phase_stability) * 0.5;

    // 限制在 0-100
    if (quality < 0) quality = 0;
    if (quality > 100) quality = 100;

    prec->val = quality;

    return 0;
}

// 注册函数
epicsRegisterFunction(calculateQuality);
```

**步骤 2**: 修改 Makefile

```makefile
BPMmonitor_SRCS += signalQuality.c
```

**步骤 3**: 在 .dbd 中注册

`devBPMMonitor.dbd`:
```c
function(calculateQuality)
```

**步骤 4**: 在数据库中使用

```
record(sub, "$(P):RF3Quality") {
    field(DESC, "RF3 Signal Quality")
    field(SNAM, "calculateQuality")
    field(SCAN, "1 second")
    field(INPA, "$(P):RF3Amp")
    field(INPB, "$(P):RF3AmpStdDev")
    field(INPC, "$(P):RF3Phase")
    field(PREC, "1")
}
```

### 周五: 状态机实现

#### 实践任务 25: BPM 运行状态机

实现一个状态机管理 BPM 系统的运行状态。

状态定义:
- 0: INIT (初始化)
- 1: STANDBY (待机)
- 2: READY (就绪)
- 3: RUNNING (运行中)
- 4: ERROR (错误)

转换规则:
- INIT → STANDBY (初始化完成)
- STANDBY → READY (系统检查通过)
- READY ⇄ RUNNING (开始/停止命令)
- 任何状态 → ERROR (检测到错误)
- ERROR → STANDBY (错误恢复)

**自己设计数据库实现状态机!**

提示: 使用 mbbi、mbbo、calc、seq 等记录。

### 周末: 文档和测试

#### 任务清单
- [ ] 为新功能编写文档
- [ ] 创建测试脚本
- [ ] 整理本周成果

#### 文档任务

创建 `USER_GUIDE.md`,包含:
1. 所有 PV 列表
2. 每个 PV 的含义和范围
3. 使用示例
4. 常见问题

#### 测试脚本

创建 `test_bpm.py`:

```python
#!/usr/bin/env python3
import epics
import time

def test_basic_readback():
    """测试基本读回功能"""
    print("Test 1: Basic readback")

    pvs = [
        "iLinac_007:BPM14And15:RF3Amp",
        "iLinac_007:BPM14And15:RF3Phase",
        "iLinac_007:BPM14And15:RF3Power"
    ]

    for pv in pvs:
        value = epics.caget(pv)
        print(f"  {pv} = {value}")

    print("  PASS\n")

def test_alarm_system():
    """测试告警系统"""
    print("Test 2: Alarm system")

    # 设置超过阈值
    epics.caput("iLinac_007:BPM14And15:RF3Amp", 2.0)
    time.sleep(1)

    # 检查告警
    severity = epics.caget("iLinac_007:BPM14And15:RF3Amp.SEVR")
    print(f"  Severity = {severity}")

    if severity >= 1:
        print("  PASS\n")
    else:
        print("  FAIL\n")

def test_statistics():
    """测试统计功能"""
    print("Test 3: Statistics")

    stddev = epics.caget("iLinac_007:BPM14And15:RF3AmpStdDev")
    max_amp = epics.caget("iLinac_007:BPM14And15:RF3MaxAmp")

    print(f"  StdDev = {stddev}")
    print(f"  Max = {max_amp}")
    print("  PASS\n")

if __name__ == "__main__":
    print("=== BPM IOC Test Suite ===\n")

    test_basic_readback()
    test_alarm_system()
    test_statistics()

    print("=== All tests completed ===")
```

#### 检查点
- [ ] 理解 EPICS 告警机制
- [ ] 能使用 subroutine 记录
- [ ] 能设计状态机
- [ ] 完成文档和测试

---

## 第 7-8 周: 综合项目与进阶

### 两周综合目标
- ✅ 完成一个完整的新功能模块
- ✅ 学习进阶主题
- ✅ 准备项目展示

### 综合项目选项 (选择一个)

#### 项目 A: 数据记录与回放系统

**功能**:
1. 记录所有 RF 数据到文件
2. 支持触发记录 (仅在事件发生时记录)
3. 回放功能 (从文件读取并更新 PV)
4. 数据导出 (CSV, HDF5)

**技术要点**:
- 文件 I/O
- 多线程
- 数据序列化
- 回放同步

#### 项目 B: 自适应阈值系统

**功能**:
1. 自动学习正常运行时的数据分布
2. 动态调整告警阈值
3. 异常检测算法 (如 3-sigma 规则)
4. 告警抑制 (避免误报)

**技术要点**:
- 统计算法
- 在线学习
- 配置持久化

#### 项目 C: Web 监控界面

**功能**:
1. 使用 Flask/FastAPI 创建 Web API
2. 实时数据展示 (WebSocket)
3. 历史数据查询
4. 控制接口

**技术要点**:
- Python Channel Access
- Web 框架
- JavaScript/React (可选)
- 实时通信

### 进阶主题学习

#### 主题 1: EPICS 模块开发

学习常用 EPICS 模块:
- **asyn**: 异步驱动框架
- **StreamDevice**: 基于协议的设备支持
- **motor**: 电机控制

#### 主题 2: 分布式系统

- 多 IOC 协作
- 网关 (Gateway)
- 命名服务器

#### 主题 3: 高级控制算法

- PID 控制
- 前馈控制
- 状态估计

### 项目展示准备

#### 展示内容
1. **架构图**: 系统整体设计
2. **代码走查**: 核心代码讲解
3. **功能演示**: 实时运行演示
4. **问题与解决**: 遇到的挑战和解决方案
5. **未来计划**: 还想实现的功能

#### 文档整理
- 技术文档更新
- API 文档
- 用户手册
- 测试报告

---

## 附录

### A. 常用命令速查

```bash
# EPICS IOC Shell
dbl                          # 列出所有记录
dbpr "PV_NAME", 2            # 查看记录详细信息
dbgf "PV_NAME"               # 读取 PV
dbpf "PV_NAME" value         # 写入 PV
var variableName value       # 设置变量
drvReport 2                  # 驱动报告

# Channel Access
caget PV_NAME                # 读取
caput PV_NAME value          # 写入
camonitor PV_NAME            # 监控
cainfo PV_NAME               # 信息

# 编译
make clean                   # 清理
make                         # 编译
make install                 # 安装
```

### B. 学习资源

#### 官方文档
- EPICS Homepage: https://epics-controls.org
- Application Developer's Guide
- Device Support Reference

#### 在线教程
- PSI EPICS Course
- APS EPICS Training

#### 书籍
- *Experimental Physics and Industrial Control System*

### C. 调试技巧总结

1. **printf 调试**: 最简单直接
2. **var 命令**: 动态调整调试级别
3. **GDB**: 用于复杂问题
4. **日志文件**: 长期运行监控
5. **camonitor**: 观察 PV 变化

### D. 代码风格建议

```c
// 1. 函数命名: 驼峰式
int InitDevice();
double ReadData(int offset, int channel, int type);

// 2. 变量命名:
// - 全局: 首字母大写
// - 局部: 小写
static double Amp[8];
int channel = 0;

// 3. 常量: 全大写
#define OFFSET_AMP  0
#define WINDOW_SIZE 100

// 4. 注释: 说明意图,不说明代码
// 计算标准差评估信号稳定性
calculateStdDev(channel);
```

### E. 每周检查清单模板

```
第 X 周检查清单
================

学习内容:
□ 主题 1
□ 主题 2
□ 主题 3

实践任务:
□ 任务 1
□ 任务 2
□ 任务 3

问题记录:
1. 问题描述
   - 尝试的方法
   - 最终解决方案

2. 未解决问题
   - 问题描述
   - 下周计划

时间统计:
- 学习: X 小时
- 编码: Y 小时
- 调试: Z 小时

下周计划:
- 目标 1
- 目标 2
```

---

## 结语

这份 8 周计划是一个起点,真正的学习在于:
1. **主动思考**: 不仅要知道"怎么做",更要理解"为什么"
2. **动手实践**: 纸上得来终觉浅,绝知此事要躬行
3. **记录总结**: 好记性不如烂笔头
4. **持续学习**: 8 周后才是真正的开始

**祝你学习顺利,享受探索的过程!**

---

**计划版本**: 1.0
**更新日期**: 2025-11-06
**作者**: Claude (基于 BPMIOC 项目)
