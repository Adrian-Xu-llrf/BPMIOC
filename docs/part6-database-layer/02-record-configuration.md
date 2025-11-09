# Part 6.2: Record配置详解

> **目标**: 掌握Record的各种字段配置和高级用法
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 90分钟

## 📖 Record配置概述

每个Record有**上百个字段**，但常用的只有**20-30个**。

**字段分类**:
1. **必需字段**: `DTYP`, `INP/OUT` (对于有硬件的Record)
2. **扫描字段**: `SCAN`, `PINI`
3. **数据字段**: `VAL`, `RVAL`
4. **显示字段**: `PREC`, `EGU`, `HOPR/LOPR`
5. **报警字段**: `HIHI`, `HIGH`, `LOW`, `LOLO`, `HHSV`, `HSV`, `LSV`, `LLSV`
6. **链接字段**: `FLNK`, `TSEL`, `SDIS`
7. **时间戳字段**: `TSE`, `TIME`

---

## 🏗️ BPMIOC中的典型配置

### 完整的ai Record配置

```
record(ai, "$(P):RFIn_01_Amp")
{
    # ===== 必需字段 =====
    field(DTYP, "BPMmonitor")         # 设备类型
    field(INP,  "@AMP:0 ch=0")        # 输入链接

    # ===== 扫描配置 =====
    field(SCAN, ".5 second")          # 每0.5秒扫描
    field(PINI, "YES")                # 初始化时处理

    # ===== 数据处理 =====
    field(PREC, "3")                  # 3位小数
    field(EGU,  "V")                  # 单位：伏特
    field(HOPR, "10")                 # 显示上限
    field(LOPR, "0")                  # 显示下限

    # ===== 报警配置 =====
    field(HIHI, "9.0")                # 高高限：9.0V
    field(HIGH, "8.0")                # 高限：8.0V
    field(LOW,  "1.0")                # 低限：1.0V
    field(LOLO, "0.5")                # 低低限：0.5V
    field(HHSV, "MAJOR")              # 高高报警级别
    field(HSV,  "MINOR")              # 高报警级别
    field(LSV,  "MINOR")              # 低报警级别
    field(LLSV, "MAJOR")              # 低低报警级别

    # ===== 其他 =====
    field(DESC, "RF Input 1 Amplitude")  # 描述
}
```

---

## 🔍 字段详解

### 1. DTYP - 设备类型

**用途**: 选择设备支持层的实现

**语法**:
```
field(DTYP, "<device_support_name>")
```

**BPMIOC的DTYP**:
```c
// 在devBPMMonitor.c中定义
epicsExportAddress(dset, devAiBPMmonitor);
epicsExportAddress(dset, devAoBPMmonitor);
epicsExportAddress(dset, devLiQBPMmonitor);
epicsExportAddress(dset, devWfBPMmonitor);
```

**对应的DTYP字段值**:
```
field(DTYP, "BPMmonitor")  # 对应devAiBPMmonitor等
```

**工作原理**:
```
Record的DTYP字段
    ↓
EPICS查找注册的dset
    ↓
找到devAiBPMmonitor
    ↓
调用其中的函数指针（init_record, read等）
```

---

### 2. INP/OUT - 输入/输出链接

**用途**: 传递硬件参数给设备支持层

#### ai/bi/longin/waveform使用INP

```
field(INP, "@AMP:0 ch=0")
           ↑
        INST_IO类型
```

**格式解析**:
```
"@AMP:0 ch=0"
  ↑   ↑    ↑
  │   │    └─ 附加参数（可选）
  │   └────── 主参数
  └────────── @开头表示INST_IO类型
```

**BPMIOC的INP格式**:

| 类型 | INP格式 | 示例 |
|------|---------|------|
| RF幅度 | `@AMP:0 ch=N` | `@AMP:0 ch=3` |
| XY位置 | `@XY:0 ch=N` | `@XY:0 ch=0` |
| 相位 | `@PHS:0 ch=N` | `@PHS:0 ch=5` |
| 按钮 | `@BTN:0 ch=N` | `@BTN:0 ch=1` |

**设备支持层的解析**:
```c
// devBPMMonitor.c中的init_record
char *param = prec->inp.value.instio.string;
// param = "@AMP:0 ch=3"

// 解析参数
int offset, channel;
sscanf(param, "@%*[^:]:%d ch=%d", &offset, &channel);
// offset = 0, channel = 3
```

#### ao/bo/longout使用OUT

```
record(ao, "$(P):SetReg")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:0")
}
```

---

### 3. SCAN - 扫描机制

**用途**: 控制Record何时被处理

**常用值**:

| 值 | 说明 | 使用场景 |
|-----|------|----------|
| `"Passive"` | 被动扫描（默认） | 只在被链接时处理 |
| `".1 second"` | 每0.1秒 | 快速更新 |
| `".5 second"` | 每0.5秒 | BPMIOC常用 |
| `"1 second"` | 每1秒 | 慢速数据 |
| `"I/O Intr"` | I/O中断扫描 | 事件驱动 |
| `"Event"` | 事件扫描 | 软件事件 |

**BPMIOC示例**:
```
# 周期扫描（0.5秒）
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")
}

# I/O中断扫描（硬件触发）
record(ai, "$(P):RFIn_Trig_Amp")
{
    field(SCAN, "I/O Intr")
}
```

**性能对比**:

```
周期扫描（.5 second）:
  优点：简单，稳定
  缺点：延迟0-0.5秒，CPU占用高

I/O中断扫描（I/O Intr）:
  优点：延迟低（<1ms），按需处理
  缺点：需要硬件触发支持
```

---

### 4. PINI - 初始化时处理

**用途**: IOC启动时立即处理Record一次

**值**:
```
field(PINI, "YES")   # 初始化时处理
field(PINI, "NO")    # 不处理（默认）
```

**使用场景**:

```
# 场景1: 读取硬件初始状态
record(ai, "$(P):HardwareStatus")
{
    field(PINI, "YES")  # IOC启动时读取
    field(SCAN, "10 second")
}

# 场景2: 初始化输出值
record(ao, "$(P):GainSetting")
{
    field(PINI, "YES")  # 写入初始值
    field(VAL,  "1.5")  # 初始增益
}
```

**处理顺序**:
```
iocInit()
  ↓
1. 所有Record的init_record()
  ↓
2. PINI="YES"的Record被处理（按依赖顺序）
  ↓
3. 正常扫描开始
```

---

### 5. PREC - 显示精度

**用途**: 控制浮点数的显示位数

**语法**:
```
field(PREC, "3")  # 保留3位小数
```

**示例**:
```
VAL = 1.23456789

PREC="0" → 显示 "1"
PREC="2" → 显示 "1.23"
PREC="3" → 显示 "1.235" (四舍五入)
PREC="6" → 显示 "1.234568"
```

**注意**:
- `PREC`只影响显示，不影响内部存储
- 内部VAL仍是双精度浮点数

---

### 6. EGU - 工程单位

**用途**: 定义数值的单位（Engineering Units）

```
field(EGU, "V")       # 伏特
field(EGU, "A")       # 安培
field(EGU, "mm")      # 毫米
field(EGU, "deg")     # 度
field(EGU, "MHz")     # 兆赫兹
```

**客户端显示**:
```bash
$ caget -g8 Test:Voltage
Test:Voltage 1.234 V
              ↑    ↑
            值   单位（来自EGU）
```

---

### 7. HOPR/LOPR - 显示范围

**用途**: 定义图形界面的显示范围

```
field(HOPR, "10")     # High Operating Range
field(LOPR, "0")      # Low Operating Range
```

**作用**:
- GUI（如CSS, EDM）使用此范围绘制图表
- 不影响实际值（值可以超出范围）

**示例**:
```
record(ai, "$(P):Current")
{
    field(EGU,  "mA")
    field(HOPR, "100")    # GUI显示0-100mA
    field(LOPR, "0")
    field(PREC, "2")
}
```

---

### 8. 报警限值和级别

#### 报警限值字段

```
HIHI (High High)    ─┐
HIGH (High)         ─┤ 上限报警
                     │
正常范围             │
                     │
LOW (Low)           ─┤ 下限报警
LOLO (Low Low)      ─┘
```

**示例**:
```
field(HIHI, "9.0")    # 高高限：9.0
field(HIGH, "8.0")    # 高限：8.0
field(LOW,  "1.0")    # 低限：1.0
field(LOLO, "0.5")    # 低低限：0.5
```

#### 报警级别字段

```
field(HHSV, "MAJOR")     # High High Severity
field(HSV,  "MINOR")     # High Severity
field(LSV,  "MINOR")     # Low Severity
field(LLSV, "MAJOR")     # Low Low Severity
```

**报警级别**:
- `"NO_ALARM"`: 无报警（默认）
- `"MINOR"`: 次要报警（黄色）
- `"MAJOR"`: 主要报警（红色）
- `"INVALID"`: 无效数据

---

#### 报警工作流程

```
假设配置：
  HIHI = 9.0, HHSV = "MAJOR"
  HIGH = 8.0, HSV  = "MINOR"
  LOW  = 1.0, LSV  = "MINOR"
  LOLO = 0.5, LLSV = "MAJOR"

当VAL变化时：
  VAL = 10.0  →  STAT=HIHI, SEVR=MAJOR  (红色报警)
  VAL = 8.5   →  STAT=HIGH, SEVR=MINOR  (黄色报警)
  VAL = 5.0   →  STAT=NO_ALARM          (正常)
  VAL = 0.8   →  STAT=LOW, SEVR=MINOR   (黄色报警)
  VAL = 0.3   →  STAT=LOLO, SEVR=MAJOR  (红色报警)
```

**查看报警状态**:
```bash
$ caget -t Test:Voltage
Test:Voltage 2024-11-09 10:30:15.123 10.5 HIHI MAJOR
                        ↑            ↑    ↑    ↑
                     时间戳         值  状态  级别
```

---

### 9. FLNK - Forward Link

**用途**: 处理完成后触发下一个Record

```
record(ai, "$(P):Input")
{
    field(SCAN, ".5 second")
    field(FLNK, "$(P):Process")  # 处理后触发Process
}

record(calc, "$(P):Process")
{
    field(SCAN, "Passive")       # 被动扫描
    field(INPA, "$(P):Input")
    field(CALC, "A*2")
}
```

**处理链**:
```
$(P):Input被扫描
    ↓
read_ai()读取数据
    ↓
更新VAL
    ↓
检查FLNK字段
    ↓
触发$(P):Process
    ↓
calc计算A*2
```

---

### 10. DESC - 描述

**用途**: 文档和调试

```
field(DESC, "RF Input Channel 1 Amplitude")
```

**查看**:
```bash
$ dbpr Test:Voltage 1
...
DESC: RF Input Channel 1 Amplitude
...
```

---

## 🎨 高级配置

### 线性转换（ai Record）

**字段**:
```
field(EGUF, "10.0")      # Engineering Units Full scale
field(EGUL, "0.0")       # Engineering Units Low scale
field(ASLO, "1.0")       # Adjustment Slope
field(AOFF, "0.0")       # Adjustment Offset
```

**转换公式**:
```
VAL = (RVAL * ASLO + AOFF)

如果设置了LINR="LINEAR":
VAL = EGUL + (EGUF - EGUL) * RVAL / (EGUF_raw - EGUL_raw)
```

**示例**:
```
# ADC读取0-4095，对应0-10V
record(ai, "$(P):Voltage")
{
    field(DTYP, "BPMmonitor")
    field(LINR, "LINEAR")
    field(EGUL, "0.0")       # 0 counts → 0V
    field(EGUF, "10.0")      # 4095 counts → 10V
    field(EGU,  "V")
}
```

---

### 死区（Deadband）

**用途**: 避免微小变化触发更新

```
field(MDEL, "0.1")       # Monitor Deadband (监控死区)
field(ADEL, "0.05")      # Archive Deadband (存档死区)
```

**工作原理**:
```
MDEL: 只有|新值 - 上次monitor值| > MDEL时，才发送monitor
ADEL: 只有|新值 - 上次存档值| > ADEL时，才触发存档
```

**示例**:
```
record(ai, "$(P):SlowSensor")
{
    field(SCAN, ".1 second")
    field(MDEL, "0.5")      # 变化>0.5才通知客户端
}
```

---

### 禁用Record

**DISV和SDIS字段**:

```
record(ai, "$(P):Sensor")
{
    field(SDIS, "$(P):Enable")   # 禁用链接
    field(DISV, "0")             # 禁用值
}

record(bi, "$(P):Enable")
{
    field(VAL, "1")  # 1=启用，0=禁用
}
```

**工作原理**:
```
处理$(P):Sensor时:
1. 读取SDIS指向的PV ($(P):Enable)
2. 如果值 == DISV (0)，则跳过处理
3. 否则正常处理
```

---

## 🔧 常用Record类型对比

### ai vs. ao

| 特性 | ai (Analog Input) | ao (Analog Output) |
|------|-------------------|-------------------|
| 用途 | 读取模拟输入 | 写入模拟输出 |
| 链接字段 | `INP` | `OUT` |
| DSET函数 | `read()` | `write()` |
| 示例 | 读温度传感器 | 设置电压 |

---

### waveform

**用途**: 存储数组数据

```
record(waveform, "$(P):ADC_Waveform")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@WF:0")
    field(FTVL, "FLOAT")      # 元素类型：浮点数
    field(NELM, "1024")       # 数组长度：1024
}
```

**字段**:
- `FTVL`: Field Type of VaLue（元素类型）
  - `"CHAR"`, `"SHORT"`, `"LONG"`, `"FLOAT"`, `"DOUBLE"`
- `NELM`: Number of ELeMents（最大元素数）
- `NORD`: Number Of elements ReaD（实际读取的元素数）

---

### calc - 计算Record

**用途**: 对输入执行数学运算

```
record(calc, "$(P):Average")
{
    field(SCAN, "1 second")
    field(INPA, "$(P):Sensor1")
    field(INPB, "$(P):Sensor2")
    field(CALC, "(A+B)/2")
    field(PREC, "3")
}
```

**支持的运算**:
- 算术: `+`, `-`, `*`, `/`, `%`
- 函数: `SIN()`, `COS()`, `EXP()`, `LOG()`, `SQRT()`
- 逻辑: `&&`, `||`, `!`, `>`, `<`, `==`
- 条件: `A?B:C` (三元运算符)

---

## ✅ 学习检查点

- [ ] 理解DTYP和INP/OUT字段的作用
- [ ] 掌握SCAN机制的各种配置
- [ ] 能够配置报警限值和级别
- [ ] 理解FLNK链接的工作原理
- [ ] 知道如何配置线性转换和死区

---

## 🎯 总结

### Record配置的核心

**三个关键问题**:
1. **数据从哪来？** → `DTYP` + `INP/OUT`
2. **何时更新？** → `SCAN` + `PINI`
3. **如何显示？** → `PREC` + `EGU` + `HOPR/LOPR`

### 最佳实践

1. **总是设置DTYP**: 除非是软Record
2. **合理选择SCAN周期**: 不要过于频繁
3. **配置合适的报警**: 帮助及时发现问题
4. **添加DESC**: 方便调试和维护
5. **使用宏**: 提高可复用性

**下一步**: 学习Part 6.3 - PV命名规范，掌握如何设计清晰的PV命名体系！

---

**关键理解**: Record配置是"声明式"的 - 你描述"想要什么"，而不是"如何实现"！💡
