# Part 6.1: .db文件结构详解

> **目标**: 深入理解EPICS数据库文件的结构和语法
> **难度**: ⭐⭐⭐☆☆
> **时间**: 60分钟

## 📖 什么是.db文件？

**.db文件** = **Database File**（数据库文件）

它是EPICS IOC的**配置文件**，定义了：
- 所有PV（Process Variable）
- Record的类型和字段
- 设备支持的连接
- 扫描机制
- 报警限值
- 链接关系

**类比**: .db文件就像是数据库的SQL DDL语句，定义了数据结构。

---

## 🏗️ 基本结构

### 最简单的Record定义

```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
}
```

**语法解析**:
```
record(<record_type>, "<pv_name>")
{
    field(<field_name>, "<field_value>")
    field(<field_name>, "<field_value>")
    ...
}
```

---

## 🔍 语法详解

### 1. record声明

```c
record(ai, "$(P):RFIn_01_Amp")
       ↑          ↑
    类型名    PV名称（可包含宏）
```

**Record类型**:
- `ai` - Analog Input（模拟输入）
- `ao` - Analog Output（模拟输出）
- `bi` - Binary Input（二进制输入）
- `bo` - Binary Output（二进制输出）
- `longin` - Long Integer Input
- `waveform` - Waveform Record
- `calc` - Calculation Record
- `...` - 更多类型

**PV名称规则**:
- 可包含宏（如`$(P)`，运行时替换）
- 最长60个字符
- 建议使用分隔符`:`或`_`
- 区分大小写
- 不能包含空格

---

### 2. field定义

每个`field()`定义Record的一个字段：

```c
field(SCAN, ".5 second")
      ↑          ↑
   字段名    字段值
```

**字段类型**:
- 字符串: `field(EGU, "V")`  → 工程单位
- 数字: `field(PREC, "3")`   → 精度
- 枚举: `field(SCAN, "I/O Intr")` → 扫描类型
- 链接: `field(INP, "@AMP:0 ch=0")` → 输入链接

---

### 3. 宏替换

**.db文件中的宏**:

```
record(ai, "$(P):RFIn_01_Amp")
```

**在st.cmd中展开**:

```bash
dbLoadRecords("BPMMonitor.db", "P=iLinac_007:BPM14")
                                ↑
                         宏定义：P的值
```

**结果**:
```
PV名称 = "iLinac_007:BPM14:RFIn_01_Amp"
```

**多个宏**:
```bash
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM, SCAN=0.1 second")
```

在.db中使用：
```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, "$(SCAN)")
}
```

---

## 📊 BPMIOC的.db文件结构

### 文件位置

```
BPMmonitorApp/Db/
├── BPMMonitor.db    ← 主数据库文件
└── BPMCal.db        ← 校准相关Record
```

---

### BPMMonitor.db的组织

**按功能分组**:

```
BPMMonitor.db
│
├─ RF输入幅度（RFIn_01_Amp ~ RFIn_16_Amp）
│  └─ 16个ai Record
│
├─ RF输出幅度（RFOut_01_Amp ~ RFOut_04_Amp）
│  └─ 4个ai Record
│
├─ XY位置（XY1_X, XY1_Y, ... XY4_Y）
│  └─ 8个ai Record
│
├─ 相位（RFIn_01_Phs ~ RFIn_16_Phs）
│  └─ 16个ai Record
│
├─ 按钮信号（Button_01 ~ Button_04）
│  └─ 4个ai Record
│
└─ 控制输出（如SetReg）
   └─ 若干ao Record
```

---

### 典型Record示例

#### 1. RF输入幅度

```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")      # 每0.5秒扫描一次
    field(DTYP, "BPMmonitor")     # 设备类型
    field(INP,  "@AMP:0 ch=0")    # 输入链接
}
```

**字段说明**:
- `SCAN`: 扫描周期（被动扫描，定时触发）
- `DTYP`: 设备类型，指向`devBPMMonitor.c`
- `INP`: 输入链接，传递给设备支持层

---

#### 2. XY位置

```
record(ai, "$(P):XY1_X")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@XY:0 ch=0")     # XY offset, channel 0
}
```

**注意**: `INP`字段的格式`@XY:0 ch=0`由设备支持层解析。

---

#### 3. 输出控制

```
record(ao, "$(P):SetReg")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:0")
}
```

**ao Record**:
- 使用`OUT`字段而非`INP`
- 写入数据到硬件

---

## 🔗 字段的分类

### 通用字段（所有Record共有）

| 字段 | 说明 | 示例 |
|------|------|------|
| `DTYP` | 设备类型 | `"BPMmonitor"` |
| `SCAN` | 扫描机制 | `".5 second"` |
| `PINI` | 初始化时处理 | `"YES"` |
| `DESC` | 描述 | `"RF Input 1 Amplitude"` |
| `TSE` | 时间戳事件 | `"-2"` |

---

### ai Record特有字段

| 字段 | 说明 | 示例 |
|------|------|------|
| `INP` | 输入链接 | `"@AMP:0 ch=0"` |
| `PREC` | 显示精度 | `"3"` (3位小数) |
| `EGU` | 工程单位 | `"V"` (伏特) |
| `HOPR/LOPR` | 显示范围 | `"10"`, `"0"` |
| `HIHI/HIGH` | 高高/高限 | `"9.0"`, `"8.0"` |
| `LOW/LOLO` | 低/低低限 | `"1.0"`, `"0.5"` |
| `HHSV/HSV` | 高高/高报警级别 | `"MAJOR"`, `"MINOR"` |

---

### ao Record特有字段

| 字段 | 说明 | 示例 |
|------|------|------|
| `OUT` | 输出链接 | `"@REG:0"` |
| `DRVH/DRVL` | 驱动限值 | `"10"`, `"0"` |
| `OMSL` | 输出模式选择 | `"supervisory"` |

---

## 📝 注释语法

### 单行注释

```
# 这是注释
record(ai, "Test:PV")  # 行尾注释
{
    field(SCAN, ".5 second")  # 扫描周期
}
```

### 多行注释

**不支持**！EPICS .db文件只支持`#`单行注释。

**推荐**:
```
#================================================
# RF输入通道幅度
#
# 包括16个输入通道，每0.5秒更新一次
#================================================
```

---

## 🎨 代码风格建议

### 缩进和对齐

```
# ✅ 推荐：字段对齐
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(PREC, "3")
    field(EGU,  "V")
}

# ❌ 不推荐：不对齐
record(ai, "$(P):RFIn_01_Amp")
{
field(SCAN, ".5 second")
field(DTYP, "BPMmonitor")
field(INP, "@AMP:0 ch=0")
}
```

---

### 分组和分隔

```
#================================================
# 第一组：RF输入幅度
#================================================

record(ai, "$(P):RFIn_01_Amp") { ... }
record(ai, "$(P):RFIn_02_Amp") { ... }
...

#================================================
# 第二组：XY位置
#================================================

record(ai, "$(P):XY1_X") { ... }
record(ai, "$(P):XY1_Y") { ... }
```

---

## ⚠️ 常见错误

### 错误1: 缺少逗号

```c
// ❌ 错误
record(ai "Test:PV")  // 缺少逗号

// ✅ 正确
record(ai, "Test:PV")
```

---

### 错误2: 字段值未加引号

```c
// ❌ 错误
field(DTYP, BPMmonitor)  // 值必须加引号

// ✅ 正确
field(DTYP, "BPMmonitor")
```

---

### 错误3: PV名称重复

```c
// ❌ 错误：两个Record的PV名相同
record(ai, "Test:PV") { ... }
record(ao, "Test:PV") { ... }  // 名称冲突！

// ✅ 正确：PV名必须唯一
record(ai, "Test:PV:Input") { ... }
record(ao, "Test:PV:Output") { ... }
```

---

### 错误4: 宏未定义

**.db文件**:
```
record(ai, "$(P):Test")
```

**st.cmd忘记定义P**:
```bash
dbLoadRecords("test.db")  # ❌ P未定义！
```

**结果**: PV名变成`$(P):Test`（字面值）

**正确**:
```bash
dbLoadRecords("test.db", "P=MySystem")  # ✅
```

---

## 🔧 数据库工具

### 1. dbExpand - 展开宏

```bash
# 查看宏展开后的.db内容
dbExpand -I/path/to/dbd -o expanded.db BPMMonitor.db

# 或者在IOC shell中
dbLoadRecords("BPMMonitor.db", "P=TEST", 1)  # debug=1
```

---

### 2. dbdToHtml - 生成文档

```bash
# 从.dbd生成HTML文档
dbdToHtml BPMmonitor.dbd
```

---

### 3. softIoc - 测试数据库

```bash
# 加载.db文件到软IOC测试
softIoc -d BPMmonitor.dbd -m "P=TEST" BPMMonitor.db
```

---

## 📊 .db文件的加载过程

### 启动时序

```
1. IOC启动
   ↓
2. st.cmd执行
   ↓
3. dbLoadDatabase("BPMmonitor.dbd")  ← 加载定义
   ↓
4. BPMmonitor_registerRecordDeviceDriver()  ← 注册设备支持
   ↓
5. dbLoadRecords("BPMMonitor.db", "P=...")  ← 加载Record实例
   ↓
6. iocInit()  ← 初始化IOC
   ↓
   调用每个Record的init_record()
```

---

### 内存中的表示

**.db文件**加载后在内存中变成：

```
数据库（Database）
│
├─ Record Instance 1
│  ├─ PV名称: "LLRF:BPM:RFIn_01_Amp"
│  ├─ Record类型: aiRecord
│  ├─ 字段值: SCAN=".5 second", DTYP="BPMmonitor", ...
│  └─ dpvt: → DevPvt结构
│
├─ Record Instance 2
│  ├─ ...
│
└─ ...
```

---

## ✅ 学习检查点

- [ ] 理解.db文件的基本语法
- [ ] 知道record和field的定义格式
- [ ] 理解宏替换机制
- [ ] 能够阅读BPMMonitor.db的结构
- [ ] 知道常见错误及其避免方法

---

## 🎯 总结

### .db文件的本质

.db文件是EPICS IOC的**声明式配置**：
- **声明**: 我需要什么PV（What）
- **不关心**: 如何实现（How，由设备支持层负责）

### 设计哲学

体现了**配置与代码分离**的设计模式：
- .db文件 = 配置（可由非程序员编写）
- .c文件 = 实现（程序员编写）

**下一步**: 学习Part 6.2 - Record配置详解，了解各种字段的高级用法！

---

**关键理解**: .db文件定义了"有哪些PV"和"PV的基本属性"，而设备支持层定义了"如何获取PV的值"！💡
