# 06 - 记录链接与数据转发

> **目标**: 掌握EPICS记录间的链接和数据流动
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 50分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解记录链接（Record Links）的概念
- ✅ 掌握各种链接标志（PP, CP, MS等）
- ✅ 理解数据转发（Forward Links）
- ✅ 能够设计复杂的记录处理链
- ✅ 避免常见的链接陷阱

## 🎯 什么是记录链接

### 核心概念

**记录链接**允许一个记录引用另一个记录的值，实现数据流动和处理链。

```
记录A → 链接 → 记录B
  ↓               ↓
 温度            显示
```

### 链接类型

EPICS支持多种链接方式：

| 链接字段 | 用途 | 示例 |
|---------|------|------|
| `INP` | 输入链接 | 从哪里读取数据 |
| `OUT` | 输出链接 | 写入数据到哪里 |
| `DOL` | 期望输出位置 | ao记录的数据源 |
| `FLNK` | 转发链接 | 处理完后触发谁 |
| `SDIS` | 扫描禁用链接 | 何时禁用扫描 |

## 🔗 1. 输入链接（INP）

### 基本语法

```
field(INP, "PV_NAME")           # 静态读取
field(INP, "PV_NAME PP")        # Process Passive
field(INP, "PV_NAME CP")        # Change Process
field(INP, "PV_NAME CP MS")     # CP + Maximize Severity
field(INP, "PV_NAME NPP")       # No Process Passive
```

### 示例1: 静态链接

```
record(ai, "Temperature")
{
    field(SCAN, "1 second")
    field(DTYP, "...")
    field(INP,  "@...")
}

record(calc, "TempF")
{
    field(SCAN, "1 second")
    field(CALC, "A*1.8+32")
    field(INPA, "Temperature")  # 静态链接
}
```

**行为**:
- TempF在初始化时读取Temperature的值
- 之后每秒独立扫描，但不再更新INPA
- ❌ 结果：TempF总是使用初始值

### 示例2: PP链接（Process Passive）

```
record(calc, "TempF")
{
    field(SCAN, "1 second")
    field(CALC, "A*1.8+32")
    field(INPA, "Temperature PP")  # PP标志
}
```

**行为**:
- TempF每秒扫描时
- 先处理Temperature（如果它是Passive）
- 然后读取Temperature的当前值
- 用于CALC计算

### 示例3: CP链接（Change Process）⭐最常用

```
record(ai, "Temperature")
{
    field(SCAN, "1 second")
    field(DTYP, "...")
}

record(calc, "TempF")
{
    field(SCAN, "Passive")        # 被动！
    field(CALC, "A*1.8+32")
    field(INPA, "Temperature CP")  # CP标志
}
```

**行为**:
- Temperature每秒扫描更新
- 每次Temperature变化时，**自动处理TempF**
- TempF不需要独立扫描
- ✅ 结果：TempF总是反映最新的Temperature

### 示例4: CP MS链接（传播报警）

```
record(ai, "Temperature")
{
    field(SCAN, "1 second")
    field(HIGH, "100")
    field(HSV,  "MAJOR")
}

record(calc, "TempF")
{
    field(SCAN, "Passive")
    field(CALC, "A*1.8+32")
    field(INPA, "Temperature CP MS")  # MS = Maximize Severity
}
```

**行为**:
- 当Temperature超过100°C进入MAJOR报警
- TempF也会继承MAJOR报警状态
- 用于将报警传播到派生PV

### 链接标志总结

| 标志 | 全称 | 何时使用 | 处理源记录 | 处理本记录 |
|------|------|---------|----------|----------|
| (无) | - | 只读一次初始值 | ❌ | ❌ |
| `NPP` | No Process Passive | 只读值，不处理 | ❌ | ❌ |
| `PP` | Process Passive | 每次读取时处理源 | ✅ | ❌ |
| `CP` | Change Process | 源变化时处理本记录 | ❌ | ✅ |
| `CPP` | Change + Process | 组合 | ✅ | ✅ |
| `MS` | Maximize Severity | 传播报警（与CP组合） | - | 继承报警 |

## 🔗 2. 输出链接（OUT）

### 基本语法

```
field(OUT, "PV_NAME")           # 写入
field(OUT, "PV_NAME PP")        # 写入并处理目标
field(OUT, "PV_NAME CA")        # 通过Channel Access写入
```

### 示例1: 直接输出

```
record(calc, "Average")
{
    field(SCAN, "1 second")
    field(CALC, "(A+B)/2")
    field(INPA, "PV1 CP")
    field(INPB, "PV2 CP")
}

record(ao, "Display")
{
    field(SCAN, "Passive")
    field(DOL,  "Average")      # 从Average获取值
    field(OMSL, "closed_loop")  # 闭环模式
}
```

### 示例2: BPMIOC中的输出

```
record(ao, "$(P):SetAlarmThreshold_Ch1")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:0 ch=0")  # 输出到设备
    field(DRVH, "10")
    field(DRVL, "0")
}
```

**数据流**:
```
用户: caput SetAlarmThreshold_Ch1 5.5
    ↓
OUT链接 → @REG:0 ch=0
    ↓
设备支持层 write_ao()
    ↓
驱动层 SetReg(0, 0, 5.5)
    ↓
硬件寄存器
```

## 🔄 3. 转发链接（FLNK）

### 概念

FLNK（Forward Link）在记录处理完成后，触发另一个记录的处理。

```
记录A处理完成
    ↓
通过FLNK
    ↓
记录B开始处理
```

### 示例：处理链

```
# 步骤1: 读取温度
record(ai, "RawTemp")
{
    field(SCAN, "1 second")
    field(DTYP, "...")
    field(INP,  "@...")
    field(FLNK, "CalibratedTemp")  # ← 转发到下一个
}

# 步骤2: 校准
record(calc, "CalibratedTemp")
{
    field(SCAN, "Passive")
    field(CALC, "A*0.98+1.5")     # 校准公式
    field(INPA, "RawTemp")
    field(FLNK, "TempF")          # ← 继续转发
}

# 步骤3: 转换单位
record(calc, "TempF")
{
    field(SCAN, "Passive")
    field(CALC, "A*1.8+32")
    field(INPA, "CalibratedTemp")
    field(FLNK, "CheckAlarm")     # ← 继续转发
}

# 步骤4: 检查报警
record(calc, "CheckAlarm")
{
    field(SCAN, "Passive")
    field(CALC, "A>100?1:0")
    field(INPA, "TempF")
    # 链的结束
}
```

**执行流程**:
```
1秒定时器触发
    ↓
RawTemp读取 (1.0s)
    ↓ FLNK
CalibratedTemp计算 (1.0001s)
    ↓ FLNK
TempF计算 (1.0002s)
    ↓ FLNK
CheckAlarm检查 (1.0003s)
```

所有4个记录在同一秒内顺序处理！

### FLNK vs CP的区别

| 方面 | FLNK | CP |
|------|------|-----|
| 方向 | 单向（A→B） | 反向（B监听A） |
| 定义位置 | 源记录 | 目标记录 |
| 触发时机 | 处理完成后 | 值变化时 |
| 执行顺序 | 严格顺序 | 同一扫描周期 |
| 多目标 | 一个 | 可以多个 |

**何时使用**:
- **FLNK**: 明确的处理顺序（步骤1→2→3）
- **CP**: 多个派生值（温度→多个显示）

## 🔗 4. BPMIOC中的链接实例

### 实例1: RF功率计算

```bash
cd ~/BPMIOC
grep -A 5 "RF3Power" BPMmonitorApp/Db/BPMCal.db
```

可能看到类似：

```
record(calc, "$(P):RF3Power")
{
    field(SCAN, "Passive")
    field(CALC, "A*A*50")
    field(INPA, "$(P):RF3Amp CP")  # ← CP链接
    field(PREC, "2")
    field(EGU,  "W")
}
```

**工作流程**:
```
RF3Amp被I/O中断更新（每100ms）
    ↓
EPICS检测到RF3Amp变化
    ↓
查找所有链接到RF3Amp且带CP标志的记录
    ↓
处理RF3Power（计算功率）
    ↓
RF3Power更新
```

### 实例2: BPM位置计算

假设BPMCal.db中有：

```
record(calc, "$(P):BPM14:XPos")
{
    field(SCAN, "Passive")
    field(CALC, "(A-B)/(A+B)*10")
    field(INPA, "$(P):RFIn_01_Amp CP")  # 左侧
    field(INPB, "$(P):RFIn_02_Amp CP")  # 右侧
    field(PREC, "3")
    field(EGU,  "mm")
}

record(calc, "$(P):BPM14:YPos")
{
    field(SCAN, "Passive")
    field(CALC, "(C-D)/(C+D)*10")
    field(INPC, "$(P):RFIn_03_Amp CP")  # 上方
    field(INPD, "$(P):RFIn_04_Amp CP")  # 下方
    field(PREC, "3")
    field(EGU,  "mm")
}
```

**一次I/O中断触发链**:
```
scanIoRequest(ioScanPvt)
    ↓
同时更新4个RF幅度PV
    ↓
触发XPos和YPos计算
    ↓
BPM位置同步更新
```

## 🔗 5. 高级链接技巧

### 技巧1: 多输入聚合

```
record(calc, "TotalPower")
{
    field(SCAN, "Passive")
    field(CALC, "A+B+C+D+E+F+G+H")
    field(INPA, "RF3Power CP")
    field(INPB, "RF4Power CP")
    field(INPC, "RF5Power CP")
    field(INPD, "RF6Power CP")
    field(INPE, "RF7Power CP")
    field(INPF, "RF8Power CP")
    field(INPG, "RF9Power CP")
    field(INPH, "RF10Power CP")
    field(EGU,  "W")
}
```

当任何一个RF功率变化时，总功率都会重新计算。

### 技巧2: 条件处理（SDIS）

```
record(ai, "ProcessedTemp")
{
    field(SCAN, "1 second")
    field(SDIS, "Enable")       # 扫描禁用链接
    field(DISV, "0")            # 禁用值
    field(DTYP, "...")
}

record(bo, "Enable")
{
    field(ZNAM, "Disabled")
    field(ONAM, "Enabled")
}
```

**行为**:
- 当Enable=0时，ProcessedTemp停止扫描
- 当Enable=1时，ProcessedTemp恢复扫描

### 技巧3: 报警传播链

```
record(ai, "SensorA")
{
    field(HIHI, "100")
    field(HHSV, "MAJOR")
}

record(calc, "DerivedB")
{
    field(INPA, "SensorA CP MS")  # MS传播报警
}

record(calc, "DerivedC")
{
    field(INPA, "DerivedB CP MS")  # 继续传播
}
```

**报警传播**:
```
SensorA超过100 → MAJOR报警
    ↓ MS
DerivedB继承 → MAJOR报警
    ↓ MS
DerivedC继承 → MAJOR报警
```

## ⚠️ 6. 常见陷阱和解决方案

### 陷阱1: 循环链接

```
# ❌ 错误：死循环
record(calc, "A")
{
    field(SCAN, "Passive")
    field(INPA, "B CP")
    field(CALC, "A+1")
}

record(calc, "B")
{
    field(SCAN, "Passive")
    field(INPA, "A CP")
    field(CALC, "A+1")
}
```

**问题**: A触发B，B触发A，无限循环！

**解决方案**: 确保链接是单向的，或使用FLNK替代。

### 陷阱2: 忘记CP标志

```
# ❌ 错误：TempF永不更新
record(calc, "TempF")
{
    field(SCAN, "Passive")
    field(INPA, "Temperature")  # 忘记CP！
    field(CALC, "A*1.8+32")
}
```

**问题**: TempF是Passive，没有CP链接，永不被处理。

**解决方案**: 添加CP标志或给TempF一个扫描周期。

### 陷阱3: 过度使用PP

```
# ⚠️ 性能问题
record(calc, "Result")
{
    field(SCAN, "10 second")
    field(INPA, "SlowSensor PP")  # 每次都处理SlowSensor
    field(CALC, "A*2")
}
```

**问题**: 每10秒都要处理SlowSensor，即使它可能已经在独立扫描。

**解决方案**: 通常不需要PP，除非源记录确实是Passive。

### 陷阱4: 链接到不存在的PV

```
record(calc, "Test")
{
    field(INPA, "NonExistent CP")
}
```

**问题**: IOC启动时会报错，Test可能无法正常工作。

**调试**:
```bash
# IOC启动日志
dbLoadRecords("test.db")
Warning: Test.INPA: Cannot resolve PV 'NonExistent'
```

## 🧪 实践练习

### 练习1: 设计温度监控链

要求：
1. 读取8个传感器温度
2. 计算平均温度
3. 如果平均温度>70°C，设置报警PV

<details>
<summary>答案</summary>

```
# 8个传感器（假设已有）
# Sensor1, Sensor2, ..., Sensor8

# 计算平均
record(calc, "AvgTemp")
{
    field(SCAN, "Passive")
    field(CALC, "(A+B+C+D+E+F+G+H)/8")
    field(INPA, "Sensor1 CP")
    field(INPB, "Sensor2 CP")
    field(INPC, "Sensor3 CP")
    field(INPD, "Sensor4 CP")
    field(INPE, "Sensor5 CP")
    field(INPF, "Sensor6 CP")
    field(INPG, "Sensor7 CP")
    field(INPH, "Sensor8 CP")
    field(FLNK, "CheckAlarm")  # 转发到报警检查
}

# 报警检查
record(calc, "TempAlarm")
{
    field(SCAN, "Passive")
    field(CALC, "A>70?1:0")
    field(INPA, "AvgTemp")
    field(PREC, "0")
}
```
</details>

### 练习2: 实现数据滤波

要求：使用FLNK实现简单的移动平均滤波（3点平均）

<details>
<summary>答案</summary>

```
# 原始数据
record(ai, "RawData")
{
    field(SCAN, "1 second")
    field(DTYP, "...")
    field(FLNK, "UpdateHistory")
}

# 历史值移位
record(calcout, "UpdateHistory")
{
    field(SCAN, "Passive")
    field(CALC, "A")
    field(INPA, "RawData")
    field(OUT,  "Hist2 PP")     # Hist3 ← Hist2
    field(FLNK, "UpdateHist2")
}

record(calcout, "UpdateHist2")
{
    field(SCAN, "Passive")
    field(CALC, "A")
    field(INPA, "Hist1")
    field(OUT,  "Hist1 PP")     # Hist2 ← Hist1
    field(FLNK, "CalcAverage")
}

# 计算平均
record(calc, "FilteredData")
{
    field(SCAN, "Passive")
    field(CALC, "(A+B+C)/3")
    field(INPA, "RawData")
    field(INPB, "Hist1")
    field(INPC, "Hist2")
}

# 历史值存储
record(ao, "Hist1") {}
record(ao, "Hist2") {}
```
</details>

## 📊 链接性能考虑

### CP链接开销

每个CP链接都需要EPICS在值变化时：
1. 检查监视器列表
2. 触发目标记录处理
3. 可能触发更多链接

**建议**:
- ✅ 对于派生值，使用CP
- ✅ 一个源PV可以有多个CP链接（EPICS会优化）
- ⚠️ 避免长链（>5级）
- ❌ 避免循环链接

### FLNK链开销

FLNK是顺序的，每个记录依次处理。

**示例时间**:
```
记录1: 0.1ms
    ↓ FLNK
记录2: 0.2ms
    ↓ FLNK
记录3: 0.15ms
总计: 0.45ms
```

**建议**:
- ✅ 用于明确的处理顺序
- ✅ 保持链短（<10个记录）
- ⚠️ 长链可能影响实时性

## 🔗 相关文档

- [Part 6: 04-inp-out-links.md](../../part6-database-layer/04-inp-out-links.md) - INP/OUT链接详解
- [Part 6: 05-forward-links.md](../../part6-database-layer/05-forward-links.md) - 转发链接高级用法
- [Part 5: 04-record-processing.md](../../part5-device-support-layer/04-record-processing.md) - 记录处理机制

## 📝 总结

### 关键要点

1. **CP最常用**: 用于派生值和计算
2. **FLNK用于顺序**: 明确的步骤1→2→3
3. **MS传播报警**: 让派生PV也有报警状态
4. **避免循环**: 单向数据流

### 链接选择指南

```
派生计算（如功率=幅度²） → CP
多步骤处理（读取→校准→转换） → FLNK
报警传播 → CP MS
条件处理 → SDIS
输出到硬件 → OUT
```

### 下一步

- [07-alarms-and-archive.md](./07-alarms-and-archive.md) - 报警系统详解
- [Part 8: lab01](../part8-hands-on-labs/labs-basic/lab01-trace-rf-amp.md) - 追踪链接数据流

---

**🎉 恭喜！** 你已经掌握了EPICS记录链接的核心知识！
