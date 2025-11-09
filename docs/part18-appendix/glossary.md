# 术语表 (Glossary)

EPICS和加速器相关术语的中英文对照和定义。

## A

### Accelerator (加速器)
利用电磁场加速带电粒子到高能量的装置。

### ADC (Analog-to-Digital Converter, 模数转换器)
将模拟信号转换为数字信号的器件。
- **示例**: 14-bit ADC @ 100MSPS

### AI (Analog Input, 模拟输入)
EPICS Record类型，用于读取模拟输入值。
- **对应**: `aiRecord`

### AO (Analog Output, 模拟输出)
EPICS Record类型，用于输出模拟值。
- **对应**: `aoRecord`

### Alarm (报警)
EPICS中指示PV状态异常的机制。
- **严重度**: NO_ALARM, MINOR, MAJOR, INVALID

### ASLO (Analog Slope, 模拟斜率)
AI/AO record的线性转换斜率参数。
- **公式**: `val = raw * ASLO + AOFF`

### asyn (异步驱动框架)
EPICS中用于异步设备通信的框架。

## B

### Base (EPICS Base)
EPICS的核心库和工具集。
- **版本**: 3.15, 7.0等
- **路径**: `/opt/epics/base`

### Beam (束流)
加速器中的带电粒子流。

### Betatron (Beta振荡)
束流在加速器中的横向振荡运动。

### BI (Binary Input, 二进制输入)
EPICS Record类型，用于读取开关量。

### BO (Binary Output, 二进制输出)
EPICS Record类型，用于输出开关量。

### BPM (Beam Position Monitor, 束流位置监测器)
测量束流横向位置的非侵入式诊断设备。

## C

### CA (Channel Access, 通道访问)
EPICS的网络通信协议。
- **端口**: 5064 (UDP), 5065 (TCP)
- **工具**: caget, caput, camonitor

### Calc (Calculate, 计算)
EPICS Record类型，用于数学计算。
- **表达式**: 类C语法

### Callback (回调)
异步操作完成后调用的函数。

### Calibration (标定)
确定测量系统参数的过程。

### CEXP (C Expression)
calc record中的计算表达式。

### Channel (通道)
- **物理**: BPM的一个电极信号
- **EPICS**: CA连接

## D

### DAC (Digital-to-Analog Converter, 数模转换器)
将数字信号转换为模拟信号的器件。

### Database (数据库)
EPICS中指.db文件，定义record和PV。

### DBD (Database Definition, 数据库定义)
定义record类型、字段、菜单等的文件。
- **示例**: `base.dbd`

### DBR (Database Request, 数据库请求)
CA协议中的数据类型。
- **类型**: DBR_STRING, DBR_DOUBLE, DBR_LONG等

### DESC (Description, 描述)
Record的描述性文本字段。

### Device Support (设备支持)
EPICS三层架构中连接Record和驱动层的中间层。

### DSET (Device Support Entry Table, 设备支持入口表)
设备支持层的函数指针表。

### Driver (驱动)
与硬件直接交互的软件层。

### DTYP (Device Type, 设备类型)
Record的设备支持类型字段。

## E

### EGU (Engineering Units, 工程单位)
PV值的物理单位。
- **示例**: "mm", "dBm", "deg"

### EPICS (Experimental Physics and Industrial Control System)
实验物理与工业控制系统。

### EPICS Base
EPICS核心软件包。

### EPICS_HOST_ARCH
环境变量，指定主机架构。
- **示例**: `linux-x86_64`, `linux-arm`

### EPICS_BASE
环境变量，指向EPICS Base安装路径。

### Event (事件)
EPICS中的同步机制。

## F

### Fan-out (扇出)
一个PV触发多个其他PV处理。

### Field (字段)
Record的一个属性。
- **示例**: VAL, SCAN, INP, OUT

### FLNK (Forward Link, 前向链接)
Record处理完后要触发的下一个record。

### FPGA (Field-Programmable Gate Array, 现场可编程门阵列)
可编程硬件，常用于高速信号处理。

### Forward Link
见FLNK。

## G

### Gateway (网关)
CA协议的转发和访问控制服务器。

### GDD (Generic Data Descriptor)
EPICS的数据描述符类。

## H

### HIHI, HIGH, LOW, LOLO
AI record的报警阈值字段。
- **HIHI**: 高高报警
- **HIGH**: 高报警
- **LOW**: 低报警
- **LOLO**: 低低报警

### HOPR, LOPR (High/Low Operating Range)
操作范围的上下限。

## I

### I/O Intr (I/O Interrupt, I/O中断)
Record的扫描类型，由硬件事件触发。

### INP (Input Link, 输入链接)
Record的输入源。
- **格式**: `@offset channel`

### IOC (Input/Output Controller, 输入输出控制器)
EPICS的核心运行时程序。

### I/Q (In-phase / Quadrature)
RF信号的同相/正交分量。

## K

### Klystron (速调管)
RF功率放大器。

## L

### LINR (Linearization, 线性化)
AI record的线性化方式。
- **选项**: NO_CONVERSION, LINEAR, typeKdegF等

### LLRF (Low-Level Radio Frequency)
低功率射频控制系统。

### LOLO, LOW
见HIHI, HIGH, LOW, LOLO。

## M

### Makefile
EPICS的构建配置文件。

### makeBaseApp.pl
创建EPICS应用的脚本。

### MEDM (Motif Editor and Display Manager)
EPICS的传统GUI工具。

### Module (模块)
EPICS的扩展软件包。

## N

### NELM (Number of Elements, 元素数量)
Waveform record的最大长度。

## O

### OPI (Operator Interface, 操作界面)
CS-Studio等GUI工具。

### OUT (Output Link, 输出链接)
Record的输出目标。

## P

### PHAS (Phase, 相位)
扫描阶段。

### Phase (相位)
- **物理**: RF信号的相位
- **EPICS**: 扫描phase

### PINI (Process at Initialization, 初始化时处理)
IOC启动时是否处理record。
- **选项**: NO, YES, RUN, RUNNING, PAUSE, PAUSED

### PREC (Precision, 精度)
浮点数显示的小数位数。

### PV (Process Variable, 过程变量)
EPICS中的数据点。
- **命名**: `SYSTEM:SUBSYS:DEVICE:PARAM`

### PVAccess
EPICS 7的新一代通信协议。

## Q

### Q值 (Quality Factor, 品质因子)
谐振器的特性参数。
- **定义**: Q = f₀ / Δf

## R

### Record (记录)
EPICS数据库的基本单元。
- **类型**: ai, ao, bi, bo, calc, waveform等

### RF (Radio Frequency, 射频)
高频电磁波。
- **频率**: MHz到GHz

### RSET (Record Support Entry Table)
Record类型的函数指针表。

## S

### SCAN (扫描)
Record的更新触发方式。
- **选项**: Passive, Event, I/O Intr, .1 second等

### SDIS (Scan Disable, 扫描禁用)
禁用record扫描的输入链接。

### Severity (严重度)
报警的严重程度。
- **级别**: NO_ALARM (0), MINOR (1), MAJOR (2), INVALID (3)

### SNR (Signal-to-Noise Ratio, 信噪比)
信号功率与噪声功率的比值。

### st.cmd (Startup Command, 启动命令)
IOC启动脚本。

### Status (状态)
报警的类型。
- **类型**: READ, WRITE, HIHI, HIGH, LOW, LOLO等

### StreamDevice
基于流协议的设备支持。

### Synchrotron (同步加速器)
粒子在环形轨道上循环加速的加速器。

## T

### TbT (Turn-by-Turn, 逐圈)
每圈测量一次的数据采集模式。

### Timestamp (时间戳)
PV更新的时间标记。

### TSE (Time Stamp Event, 时间戳事件)
时间戳来源字段。

## U

### UDF (Undefined, 未定义)
Record值是否有效的标志。

## V

### VAL (Value, 值)
Record的主要值字段。

### VDCT (Visual Database Configuration Tool)
可视化数据库编辑工具。

## W

### Waveform (波形)
EPICS Record类型，用于数组数据。
- **容量**: 由NELM定义

## X

### XFEL (X-ray Free-Electron Laser, X射线自由电子激光)
先进的同步辐射光源。

## 缩写速查

| 缩写 | 全称 | 中文 |
|------|------|------|
| **ADC** | Analog-to-Digital Converter | 模数转换器 |
| **BPM** | Beam Position Monitor | 束流位置监测器 |
| **CA** | Channel Access | 通道访问 |
| **DAC** | Digital-to-Analog Converter | 数模转换器 |
| **DBD** | Database Definition | 数据库定义 |
| **DSET** | Device Support Entry Table | 设备支持入口表 |
| **EPICS** | Experimental Physics and Industrial Control System | 实验物理与工业控制系统 |
| **FPGA** | Field-Programmable Gate Array | 现场可编程门阵列 |
| **IOC** | Input/Output Controller | 输入输出控制器 |
| **I/Q** | In-phase / Quadrature | 同相/正交 |
| **LLRF** | Low-Level Radio Frequency | 低功率射频 |
| **PV** | Process Variable | 过程变量 |
| **RF** | Radio Frequency | 射频 |
| **RSET** | Record Support Entry Table | Record支持入口表 |
| **SNR** | Signal-to-Noise Ratio | 信噪比 |
| **TbT** | Turn-by-Turn | 逐圈 |

## 中英文对照

| 中文 | 英文 |
|------|------|
| 加速器 | Accelerator |
| 束流 | Beam |
| 束流位置监测器 | Beam Position Monitor (BPM) |
| 通道访问 | Channel Access (CA) |
| 数据库 | Database |
| 设备支持 | Device Support |
| 驱动 | Driver |
| 过程变量 | Process Variable (PV) |
| 记录 | Record |
| 射频 | Radio Frequency (RF) |
| 低功率射频 | Low-Level RF (LLRF) |
| 扫描 | Scan |
| 信噪比 | Signal-to-Noise Ratio (SNR) |
| 同步加速器 | Synchrotron |
| 波形 | Waveform |

---

**使用提示**:
- 使用Ctrl+F搜索术语
- 不确定的缩写先查"缩写速查"
- 需要中英对照时查"中英文对照"
