# 04 - EPICS记录类型详解

> **目标**: 理解BPMIOC中使用的各种EPICS记录类型
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 45分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解常用的EPICS记录类型
- ✅ 知道何时使用哪种记录类型
- ✅ 理解记录字段的含义
- ✅ 能够在BPMIOC中找到各类型的使用示例
- ✅ 能够为新功能选择合适的记录类型

## 🎯 记录类型概述

EPICS提供了50+种记录类型，但BPMIOC主要使用以下几种：

### BPMIOC使用的记录类型统计

| 记录类型 | 数量 | 占比 | 主要用途 |
|---------|-----|------|---------|
| `ai` | 120+ | 55% | RF幅度、相位、温度、电压读取 |
| `ao` | 60+ | 27% | 阈值设置、配置参数写入 |
| `waveform` | 30+ | 14% | 波形数据（10K-100K点） |
| `calc` | 8 | 4% | 功率计算 |
| `longin` | 少量 | <1% | 整数状态读取 |

## 📖 详细记录类型

### 1. ai - Analog Input（模拟量输入）

#### 用途
从硬件读取模拟量数据（浮点数）。

#### BPMIOC中的使用

**示例1: RF幅度**
```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")      # 扫描方式：每0.5秒
    field(DTYP, "BPMmonitor")     # 设备类型
    field(INP,  "@AMP:0 ch=0")   # 输入链接：offset=0, channel=0
    field(PREC, "3")              # 精度：小数点后3位
    field(EGU,  "V")              # 工程单位：伏特
    field(HOPR, "10")             # 显示高限：10V
    field(LOPR, "0")              # 显示低限：0V
    field(HIHI, "9")              # 高高报警：9V
    field(HIGH, "8")              # 高报警：8V
    field(LOW,  "1")              # 低报警：1V
    field(LOLO, "0.5")            # 低低报警：0.5V
    field(HHSV, "MAJOR")          # 高高严重性：主要
    field(HSV,  "MINOR")          # 高严重性：次要
    field(LSV,  "MINOR")          # 低严重性：次要
    field(LLSV, "MAJOR")          # 低低严重性：主要
}
```

**示例2: 温度传感器**
```
record(ai, "$(P):Temperature1")
{
    field(SCAN, "I/O Intr")       # I/O中断扫描
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")  # 读取温度寄存器
    field(PREC, "1")              # 1位小数
    field(EGU,  "C")              # 摄氏度
    field(HIGH, "70")             # 高温报警：70°C
    field(HIHI, "85")             # 高高报警：85°C
}
```

#### 重要字段详解

| 字段 | 全称 | 含义 | 示例值 | 说明 |
|------|------|------|--------|------|
| `SCAN` | Scan Mechanism | 扫描机制 | "I/O Intr" | 何时处理此记录 |
| `DTYP` | Device Type | 设备类型 | "BPMmonitor" | 使用哪个设备支持 |
| `INP` | Input Specification | 输入规范 | "@AMP:0 ch=0" | 从哪里读取数据 |
| `PREC` | Display Precision | 显示精度 | "3" | 小数点后位数 |
| `EGU` | Engineering Units | 工程单位 | "V" | 显示单位 |
| `HOPR` | High Operating Range | 显示高限 | "10" | 绘图时的上限 |
| `LOPR` | Low Operating Range | 显示低限 | "0" | 绘图时的下限 |
| `HIHI` | Hihi Alarm Limit | 高高报警限值 | "9" | 超过触发严重报警 |
| `HIGH` | High Alarm Limit | 高报警限值 | "8" | 超过触发次要报警 |
| `LOW` | Low Alarm Limit | 低报警限值 | "1" | 低于触发次要报警 |
| `LOLO` | Lolo Alarm Limit | 低低报警限值 | "0.5" | 低于触发严重报警 |
| `HHSV` | Hihi Severity | 高高严重性 | "MAJOR" | 报警级别 |
| `HSV` | High Severity | 高严重性 | "MINOR" | 报警级别 |
| `LSV` | Low Severity | 低严重性 | "MINOR" | 报警级别 |
| `LLSV` | Lolo Severity | 低低严重性 | "MAJOR" | 报警级别 |

#### 报警严重性级别

```
NO_ALARM  (0) - 无报警（绿色）
MINOR     (1) - 次要报警（黄色）
MAJOR     (2) - 主要报警（红色）
INVALID   (3) - 无效数据（白色）
```

#### 数据流

```
硬件/驱动层
    ↓
ReadData(offset, channel, type) 返回 float
    ↓
设备支持层（read_ai函数）
    ↓
RVAL (Raw Value) → 转换 → VAL (Value)
    ↓
检查报警限值
    ↓
更新 SEVR (Severity) 和 STAT (Status)
    ↓
通过Channel Access广播
```

---

### 2. ao - Analog Output（模拟量输出）

#### 用途
向硬件写入模拟量数据（浮点数）。

#### BPMIOC中的使用

**示例1: 设置报警阈值**
```
record(ao, "$(P):SetAlarmThreshold_Ch1")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:0 ch=0")   # 输出到寄存器0
    field(PREC, "2")              # 2位小数
    field(EGU,  "V")
    field(DRVH, "10")             # 驱动高限：最大10V
    field(DRVL, "0")              # 驱动低限：最小0V
    field(HOPR, "10")
    field(LOPR, "0")
}
```

**示例2: 设置积分时间**
```
record(ao, "$(P):SetIntegTime")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:21")
    field(PREC, "0")              # 整数
    field(EGU,  "us")             # 微秒
    field(DRVH, "1000")           # 最大1000μs
    field(DRVL, "1")              # 最小1μs
}
```

#### 重要字段详解

| 字段 | 全称 | 含义 | 示例值 | 说明 |
|------|------|------|--------|------|
| `OUT` | Output Specification | 输出规范 | "@REG:0 ch=0" | 写入到哪里 |
| `DRVH` | Drive High Limit | 驱动高限 | "10" | 允许写入的最大值 |
| `DRVL` | Drive Low Limit | 驱动低限 | "0" | 允许写入的最小值 |
| `DOL` | Desired Output Location | 期望输出源 | "..." | 从哪里获取要写入的值 |
| `OMSL` | Output Mode Select | 输出模式选择 | "supervisory" | closed_loop或supervisory |

#### 数据流（写入）

```
用户执行 caput
    ↓
新值写入 VAL
    ↓
检查 DRVH/DRVL 限制
    ↓
VAL → 转换 → RVAL
    ↓
设备支持层（write_ao函数）
    ↓
SetReg(offset, channel, value)
    ↓
驱动层写入硬件
```

#### ao vs ai 对比

| 方面 | ao | ai |
|------|----|----|
| 数据方向 | 写入硬件 | 从硬件读取 |
| 链接字段 | OUT | INP |
| 限制字段 | DRVH/DRVL | HOPR/LOPR |
| 报警 | 通常不设置 | HIHI/HIGH/LOW/LOLO |
| 用户交互 | 经常caput | 经常caget/camonitor |

---

### 3. waveform - 波形记录

#### 用途
存储和传输数组数据（大量数据点）。

#### BPMIOC中的使用

**示例1: 触发波形（10,000点）**
```
record(waveform, "$(P):RFIn_01_TrigWaveform")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@ARRAY:1")      # 数组offset=1
    field(FTVL, "FLOAT")          # 字段类型：浮点
    field(NELM, "10000")          # 元素数量：10000
    field(PREC, "3")
    field(EGU,  "V")
}
```

**示例2: Trip波形（100,000点）**
```
record(waveform, "$(P):RFIn_01_TripWaveform")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@ARRAY:21")
    field(FTVL, "FLOAT")
    field(NELM, "100000")         # 大数据量
    field(PREC, "3")
    field(EGU,  "V")
}
```

#### 重要字段详解

| 字段 | 全称 | 含义 | 示例值 | 说明 |
|------|------|------|--------|------|
| `FTVL` | Field Type of Value | 值字段类型 | "FLOAT" | 数组元素类型 |
| `NELM` | Number of Elements | 元素数量 | "10000" | 数组最大长度 |
| `NORD` | Number of elements Read | 实际读取数 | 动态 | 运行时填充 |

#### FTVL支持的类型

```
"STRING"  - 字符串
"CHAR"    - 8位整数
"UCHAR"   - 8位无符号整数
"SHORT"   - 16位整数
"USHORT"  - 16位无符号整数
"LONG"    - 32位整数
"ULONG"   - 32位无符号整数
"FLOAT"   - 32位浮点数 ← BPMIOC使用
"DOUBLE"  - 64位浮点数
```

#### 数据流（波形读取）

```
硬件触发或I/O中断
    ↓
readWaveform(offset, ch, nelem, data_array, timestamp)
    ↓
驱动层填充数组（10K或100K个点）
    ↓
设备支持层接收数据
    ↓
更新 NORD（实际点数）
    ↓
波形数据可通过CA读取
```

#### 访问波形数据

```bash
# 读取前100个点
caget -# 100 iLinac_007:BPM14And15:RFIn_01_TrigWaveform

# 读取全部10000个点
caget -# 10000 iLinac_007:BPM14And15:RFIn_01_TrigWaveform

# Python访问
import epics
wf = epics.caget('iLinac_007:BPM14And15:RFIn_01_TrigWaveform')
print(f"波形长度: {len(wf)}")
print(f"最大值: {max(wf):.3f} V")
```

---

### 4. calc - 计算记录

#### 用途
对输入值进行数学计算。

#### BPMIOC中的使用

**示例1: 功率计算**
```
record(calc, "$(P):RF3Power")
{
    field(SCAN, "1 second")
    field(CALC, "A*A*50")         # 功率 = 幅度² × 50Ω
    field(INPA, "$(P):RF3Amp")   # A = RF3幅度
    field(PREC, "2")
    field(EGU,  "W")              # 瓦特
}
```

**示例2: BPM位置计算**
```
record(calc, "$(P):BPM14:XPos")
{
    field(SCAN, "Passive")        # 被动扫描（通过链接触发）
    field(CALC, "(A-B)/(A+B)*10") # X位置公式
    field(INPA, "$(P):RFIn_01_Amp CP")  # CP = Change Process
    field(INPB, "$(P):RFIn_02_Amp CP")
    field(PREC, "3")
    field(EGU,  "mm")
}
```

#### 重要字段详解

| 字段 | 全称 | 含义 | 示例值 | 说明 |
|------|------|------|--------|------|
| `CALC` | Calculation | 计算表达式 | "A*A*50" | 使用A-L作为输入 |
| `INPA` | Input A | 输入A | "$(P):RF3Amp" | 可以是PV链接 |
| `INPB` | Input B | 输入B | "5.0" | 也可以是常数 |
| ... | ... | 输入C-L | ... | 最多12个输入 |

#### CALC表达式语法

**基本运算符**:
```
+  -  *  /  %      # 加减乘除，取模
^                  # 幂运算
()                 # 括号
```

**比较运算符**:
```
>  <  >=  <=  =  !=  # 返回0或1
```

**逻辑运算符**:
```
&&  ||  !          # 与、或、非
```

**条件运算符**:
```
A?B:C              # 如果A为真，返回B，否则返回C
```

**函数**:
```
ABS(A)             # 绝对值
SIN(A), COS(A), TAN(A)  # 三角函数（弧度）
ASIN(A), ACOS(A), ATAN(A)
EXP(A), LN(A), LOG(A)   # 指数和对数
SQRT(A)            # 平方根
MAX(A,B), MIN(A,B) # 最大值、最小值
```

#### 计算示例

```
# 功率计算（欧姆定律）
CALC: "A*A*50"
A=2V → (2*2*50) = 200W

# 位置计算
CALC: "(A-B)/(A+B)*10"
A=3, B=1 → (3-1)/(3+1)*10 = 5mm

# 温度转换（摄氏→华氏）
CALC: "A*1.8+32"
A=25°C → 25*1.8+32 = 77°F

# 条件判断
CALC: "A>5?B:C"
A=7, B=10, C=20 → 结果=10 (因为A>5为真)
```

#### 输入链接选项

```
"PV_NAME"          # 静态链接（初始化时读取一次）
"PV_NAME PP"       # Process Passive（每次处理时读取）
"PV_NAME CP"       # Change Process（PV变化时触发处理）
"PV_NAME CP MS"    # 还传播报警严重性（Maximize Severity）
"PV_NAME NPP"      # No Process Passive（不处理）
```

**示例**:
```
field(INPA, "$(P):RF3Amp CP")
# 当RF3Amp变化时，自动处理本calc记录
```

---

### 5. longin - 长整数输入

#### 用途
读取32位整数值。

#### BPMIOC中的使用

**示例: 系统状态**
```
record(longin, "$(P):SystemStatus")
{
    field(SCAN, "1 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:32")
    field(EGU,  "")
}
```

**状态码解析**（可以用calc记录）:
```
record(calc, "$(P):IsOnline")
{
    field(CALC, "(A&0x01)?1:0")  # 提取bit 0
    field(INPA, "$(P):SystemStatus")
}
```

---

## 🔄 记录类型对比表

### 输入记录对比

| 特性 | ai | longin | waveform | calc |
|------|----|----|-------|------|
| 数据类型 | float | int32 | 数组 | float |
| 数据源 | 硬件 | 硬件 | 硬件 | 其他PV或常数 |
| 报警 | 支持 | 支持 | 不支持 | 支持 |
| 典型用途 | 传感器读数 | 状态码 | 波形数据 | 计算结果 |

### 输出记录对比

| 特性 | ao | longout | waveform(写) |
|------|----|----|------|
| 数据类型 | float | int32 | 数组 |
| 数据方向 | 写入硬件 | 写入硬件 | 写入硬件 |
| 限制 | DRVH/DRVL | DRVH/DRVL | NELM |
| 典型用途 | 设置参数 | 命令/配置 | 写入波形 |

---

## 🧪 实践练习

### 练习1: 创建温度监控

创建一个记录监控温度，并在超过70°C时报警：

<details>
<summary>答案</summary>

```
record(ai, "MY:Temperature")
{
    field(SCAN, "1 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
    field(PREC, "1")
    field(EGU,  "C")
    field(HIGH, "70")
    field(HIHI, "85")
    field(HSV,  "MINOR")
    field(HHSV, "MAJOR")
}
```
</details>

### 练习2: 计算平均值

创建calc记录计算4个通道的平均值：

<details>
<summary>答案</summary>

```
record(calc, "MY:AverageAmp")
{
    field(SCAN, "Passive")
    field(CALC, "(A+B+C+D)/4")
    field(INPA, "$(P):RFIn_01_Amp CP")
    field(INPB, "$(P):RFIn_02_Amp CP")
    field(INPC, "$(P):RFIn_03_Amp CP")
    field(INPD, "$(P):RFIn_04_Amp CP")
    field(PREC, "3")
    field(EGU,  "V")
}
```
</details>

### 练习3: 条件报警

创建calc记录：当温度>80°C且湿度>60%时，输出1（报警）：

<details>
<summary>答案</summary>

```
record(calc, "MY:OverheatAlarm")
{
    field(SCAN, "Passive")
    field(CALC, "(A>80 && B>60)?1:0")
    field(INPA, "MY:Temperature CP")
    field(INPB, "MY:Humidity CP")
    field(PREC, "0")
}
```
</details>

---

## 🔍 在BPMIOC中查找示例

### 查找ai记录

```bash
cd ~/BPMIOC
grep -n "^record(ai" BPMmonitorApp/Db/BPMMonitor.db | head -5
```

### 查找waveform记录

```bash
grep -n "^record(waveform" BPMmonitorApp/Db/BPMMonitor.db
```

### 查找calc记录

```bash
grep -n "^record(calc" BPMmonitorApp/Db/BPMMonitor.db
```

### 查看完整记录定义

```bash
# 查看第一个ai记录
sed -n '1,6p' BPMmonitorApp/Db/BPMMonitor.db
```

---

## 📊 记录类型选择指南

### 何时使用ai记录

- ✅ 从硬件读取浮点数（电压、温度、功率等）
- ✅ 需要报警功能
- ✅ 需要单位转换
- ✅ 数据变化连续

### 何时使用ao记录

- ✅ 向硬件写入浮点数
- ✅ 设置阈值、配置参数
- ✅ 需要限制输入范围（DRVH/DRVL）
- ✅ 用户可调参数

### 何时使用waveform记录

- ✅ 传输大量数据点（>100）
- ✅ 时域波形、频谱数据
- ✅ 历史数据缓冲
- ✅ 数组操作

### 何时使用calc记录

- ✅ 基于其他PV计算派生值
- ✅ 单位转换
- ✅ 简单逻辑判断
- ✅ 避免在驱动层实现简单运算

### 何时使用longin/longout记录

- ✅ 状态码、枚举值
- ✅ 计数器
- ✅ 位字段操作
- ✅ 整数配置

---

## 🔗 相关文档

- [EPICS Record Reference Manual](https://epics.anl.gov/base/R3-15/6-docs/RecordReference.html)
- [Part 6: 02-record-definitions.md](../../part6-database-layer/02-record-definitions.md) - 记录定义详解
- [Part 6: 03-field-reference.md](../../part6-database-layer/03-field-reference.md) - 字段参考
- [Part 8: lab01](../part8-hands-on-labs/labs-basic/lab01-trace-rf-amp.md) - 追踪ai记录

---

## 📝 总结

### 关键要点

1. **ai/ao**: 最常用，用于模拟量I/O
2. **waveform**: 大数据传输，BPMIOC中用于波形
3. **calc**: 简单计算，避免在C代码中实现
4. **选择原则**: 根据数据类型、数据流向、是否需要报警来选择

### 记忆技巧

```
ai - Analog In   - 读浮点数
ao - Analog Out  - 写浮点数
bi - Binary In   - 读0/1
bo - Binary Out  - 写0/1
calc - Calculator - 计算
waveform - Wave   - 波形数组
```

### 下一步

- [05-scanning-basics.md](./05-scanning-basics.md) - 学习扫描机制
- [06-links-and-forwarding.md](./06-links-and-forwarding.md) - 记录间链接
- [Part 6](../../part6-database-layer/) - 深入数据库层

---

**🎉 恭喜！** 你已经掌握了EPICS记录类型的基础知识！
