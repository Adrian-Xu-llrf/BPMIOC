# 09 - 数据库文件语法

> **目标**: 掌握EPICS数据库文件（.db）的语法
> **难度**: ⭐⭐☆☆☆
> **预计时间**: 30分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解.db文件的基本结构
- ✅ 掌握记录和字段的语法
- ✅ 使用宏替换和模板
- ✅ 理解include和path机制
- ✅ 能够编写和修改BPMIOC的数据库文件

## 📄 1. 数据库文件基础

### 什么是.db文件

**.db文件**包含EPICS记录定义，是IOC的核心配置。

```
源文件:   BPMMonitor.db.template
           ↓
宏替换:   P=$(PREFIX), 等
           ↓
最终文件: BPMMonitor.db
           ↓
加载到IOC: dbLoadRecords()
```

### 基本结构

```
# BPMMonitor.db

# 注释以#开头

record(记录类型, "PV名称")
{
    field(字段名, "值")
    field(字段名, "值")
    ...
}
```

## 🔤 2. 语法规则

### 记录定义语法

```
record(TYPE, "PV_NAME")
{
    field(FIELD1, "value1")
    field(FIELD2, "value2")
    field(FIELD3, value3)  # 数字可以不用引号
}
```

**示例**:
```
record(ai, "iLinac_007:BPM14And15:RFIn_01_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(PREC, "3")
    field(EGU,  "V")
    field(HOPR, "10")
    field(LOPR, "0")
}
```

### 注释

```
# 单行注释

/*
 * 多行注释
 * 可以跨越多行
 */

record(ai, "Temperature")  # 行尾注释也支持
{
    field(SCAN, "1 second")
    # 字段间的注释
    field(EGU, "C")
}
```

### 字段值类型

```
# 字符串（需要引号）
field(DESC, "Temperature Sensor")
field(EGU,  "V")

# 数字（引号可选）
field(PREC, "3")   # 或 field(PREC, 3)
field(HIGH, 100)   # 数字通常不加引号

# PV链接（需要引号）
field(INP, "OtherPV")
field(INP, "OtherPV CP")

# 设备链接（需要引号）
field(INP, "@AMP:0 ch=0")

# 枚举值（需要引号）
field(SCAN, "I/O Intr")
field(OMSL, "supervisory")
```

## 🔄 3. 宏替换

### 使用宏

宏允许创建可重用的模板：

```
# 模板文件: sensor.template
record(ai, "$(P):$(R)")
{
    field(SCAN, "$(SCAN)")
    field(DTYP, "$(DTYP)")
    field(INP,  "@$(TYPE):$(OFFSET) ch=$(CH)")
    field(PREC, "$(PREC)")
    field(EGU,  "$(EGU)")
}
```

### 宏展开

在st.cmd中加载时提供宏值：

```bash
dbLoadRecords("db/sensor.template", "P=MY,R=Temp1,SCAN=1 second,DTYP=BPMmonitor,TYPE=REG,OFFSET=29,CH=0,PREC=1,EGU=C")
```

展开后：
```
record(ai, "MY:Temp1")
{
    field(SCAN, "1 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
    field(PREC, "1")
    field(EGU,  "C")
}
```

### BPMIOC中的宏使用

查看BPMMonitor.db：

```bash
head -20 ~/BPMIOC/BPMmonitorApp/Db/BPMMonitor.db
```

可能看到：
```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
}
```

在st.cmd中：
```bash
dbLoadRecords("db/BPMMonitor.db", "P=iLinac_007:BPM14And15")
```

最终PV名：`iLinac_007:BPM14And15:RFIn_01_Amp`

## 📦 4. 多记录模板

### 循环展开模板

有时需要为多个通道创建相似的记录。虽然.db文件本身不支持循环，但可以：

1. **使用脚本生成**
2. **使用substitution文件**
3. **手动重复**（BPMIOC使用此方法）

### Substitution文件示例

创建 `sensors.substitutions`:

```
file "sensor.template" {
    pattern { P,        R,      OFFSET, CH,  EGU }
            { "MY",  "Temp1",   29,    0,   "C"  }
            { "MY",  "Temp2",   29,    1,   "C"  }
            { "MY",  "Temp3",   29,    2,   "C"  }
            { "MY",  "Temp4",   29,    3,   "C"  }
}
```

加载：
```bash
dbLoadTemplate("db/sensors.substitutions")
```

这会自动生成4个记录。

### BPMIOC的方法

BPMIOC直接手写8个RF通道：

```
record(ai, "$(P):RFIn_01_Amp") { ... }
record(ai, "$(P):RFIn_02_Amp") { ... }
record(ai, "$(P):RFIn_03_Amp") { ... }
...
record(ai, "$(P):RFIn_08_Amp") { ... }
```

优点：直观、易于单独修改
缺点：重复代码较多

## 🔗 5. Include文件

### 包含其他数据库

```
# main.db

# 包含另一个数据库文件
#include "basic_records.db"
#include "advanced_records.db"

# 自己的记录
record(ai, "MyPV") { ... }
```

### BPMIOC示例

检查是否有include：

```bash
grep -n "include" ~/BPMIOC/BPMmonitorApp/Db/BPMMonitor.db
```

如果有，可能看到：
```
#include "common_records.db"
```

## 📝 6. 字段引用

### 访问其他记录的字段

可以链接到其他记录的特定字段：

```
# 引用整个记录的值
field(INPA, "Temperature")

# 引用特定字段
field(INPA, "Temperature.VAL")    # 值（默认）
field(INPB, "Temperature.SEVR")   # 严重性
field(INPC, "Temperature.STAT")   # 状态
field(INPD, "Temperature.HIGH")   # 高限值
```

### 常用字段引用

| 字段 | 含义 | 类型 |
|------|------|------|
| `.VAL` | 当前值 | 根据记录类型 |
| `.SEVR` | 严重性 | 0-3 |
| `.STAT` | 状态 | 枚举 |
| `.TIME` | 时间戳 | 时间 |
| `.PREC` | 精度 | 整数 |
| `.EGU` | 工程单位 | 字符串 |

### 示例：报警汇总

```
record(calc, "AnyAlarm")
{
    field(SCAN, "Passive")
    field(CALC, "(A||B||C||D)?1:0")
    field(INPA, "Sensor1.SEVR CP")
    field(INPB, "Sensor2.SEVR CP")
    field(INPC, "Sensor3.SEVR CP")
    field(INPD, "Sensor4.SEVR CP")
}
```

## 🧩 7. 实际示例

### 示例1: 完整的温度监控

```
# temperature.db

# 原始温度读取
record(ai, "$(P):RawTemp")
{
    field(SCAN, "1 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=$(CH)")
    field(PREC, "1")
    field(EGU,  "C")
    field(FLNK, "$(P):CalibratedTemp")
}

# 校准
record(calc, "$(P):CalibratedTemp")
{
    field(SCAN, "Passive")
    field(CALC, "A*0.98+1.5")
    field(INPA, "$(P):RawTemp")
    field(PREC, "1")
    field(EGU,  "C")

    # 报警
    field(HIGH, "70")
    field(HIHI, "85")
    field(HSV,  "MINOR")
    field(HHSV, "MAJOR")

    field(FLNK, "$(P):TempF")
}

# 华氏度
record(calc, "$(P):TempF")
{
    field(SCAN, "Passive")
    field(CALC, "A*1.8+32")
    field(INPA, "$(P):CalibratedTemp")
    field(PREC, "1")
    field(EGU,  "F")
}
```

加载：
```bash
dbLoadRecords("db/temperature.db", "P=MY,CH=0")
```

### 示例2: 控制记录组

```
# rf_control.db

# RF使能开关
record(bo, "$(P):RF_Enable")
{
    field(DESC, "RF Enable Switch")
    field(ZNAM, "Disabled")
    field(ONAM, "Enabled")
    field(FLNK, "$(P):RF_UpdateStatus")
}

# 状态更新
record(calc, "$(P):RF_UpdateStatus")
{
    field(SCAN, "Passive")
    field(CALC, "A")
    field(INPA, "$(P):RF_Enable")
    field(OOPT, "Every Time")
    field(DOPT, "Use CALC")
    field(OUT,  "$(P):RF_Status PP")
}

# 状态显示
record(mbbi, "$(P):RF_Status")
{
    field(SCAN, "Passive")
    field(ZRST, "Off")
    field(ONST, "On")
}
```

## 🚫 8. 常见错误

### 错误1: 缺少引号

```
# ❌ 错误
field(DESC, Temperature Sensor)

# ✅ 正确
field(DESC, "Temperature Sensor")
```

### 错误2: 拼写错误

```
# ❌ 错误
filed(SCAN, "1 second")  # filed而不是field

# ✅ 正确
field(SCAN, "1 second")
```

### 错误3: PV名称中有空格

```
# ❌ 错误
record(ai, "My Temperature")  # 空格会导致问题

# ✅ 正确
record(ai, "My:Temperature")  # 使用冒号或下划线
record(ai, "My_Temperature")
```

### 错误4: 重复的PV名称

```
# ❌ 错误：两个记录同名
record(ai, "Temperature")
{
    field(SCAN, "1 second")
}

record(ai, "Temperature")  # 重复！
{
    field(SCAN, "2 second")
}

# IOC会报错
```

### 错误5: 链接到不存在的PV

```
# ⚠️ 警告：NonExistent不存在
record(calc, "MyCalc")
{
    field(INPA, "NonExistent CP")
}

# IOC启动时会警告
```

## 🛠️ 9. 调试和验证

### 使用dbExpand

展开宏后查看最终结果：

```bash
cd ~/BPMIOC
dbExpand -p "P=iLinac_007:BPM14And15" BPMmonitorApp/Db/BPMMonitor.db | head -20
```

### 检查语法

EPICS会在加载时检查语法：

```bash
# 在IOC shell中
dbLoadRecords("db/BPMMonitor.db", "P=TEST")

# 如果有语法错误会报错
```

### 列出所有PV

IOC启动后：

```bash
# IOC shell
dbl

# 或者在IOC外
caget -a PREFIX:*
```

## 📚 10. 最佳实践

### 1. 使用描述性名称

```
# ❌ 不好
record(ai, "$(P):A1") {}

# ✅ 好
record(ai, "$(P):RFIn_01_Amp") {}
```

### 2. 添加注释

```
# 温度传感器1 - 位于机柜顶部
record(ai, "$(P):Temperature1")
{
    field(DESC, "Cabinet Top Temperature")
    field(SCAN, "1 second")
    # 高温报警设置基于夏季测试数据
    field(HIGH, "70")
}
```

### 3. 使用宏提高可重用性

```
# ✅ 好：可重用的模板
record(ai, "$(P):$(R)")
{
    field(SCAN, "$(SCAN=1 second)")  # 提供默认值
    field(EGU,  "$(EGU=)")
}
```

### 4. 组织相关记录

```
# ======================
# RF Monitoring
# ======================

record(ai, "$(P):RFIn_01_Amp") {}
record(ai, "$(P):RFIn_01_Phase") {}
record(calc, "$(P):RF1Power") {}

# ======================
# Temperature Monitoring
# ======================

record(ai, "$(P):Temperature1") {}
record(ai, "$(P):Temperature2") {}
```

### 5. 保持一致的格式

```
# 对齐字段名称，提高可读性
record(ai, "$(P):Temperature")
{
    field(SCAN, "1 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
    field(PREC, "1")
    field(EGU,  "C")
}
```

## 🔗 相关文档

- [Part 6: 02-record-definitions.md](../../part6-database-layer/02-record-definitions.md) - 记录定义详解
- [Part 6: 08-macros-templates.md](../../part6-database-layer/08-macros-templates.md) - 宏和模板
- [EPICS Application Developer's Guide - Chapter 3](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/DatabaseDefinition.html)

## 📝 总结

### 核心语法

```
record(TYPE, "PV_NAME")
{
    field(FIELD, "value")
}
```

### 关键要点

1. **引号规则**: 字符串和链接用引号，数字可选
2. **注释**: 使用 `#` 或 `/* */`
3. **宏**: 使用 `$(MACRO_NAME)` 实现模板
4. **字段引用**: 用 `.FIELD` 访问其他字段

### 检查清单

- ✅ PV名称唯一
- ✅ 字符串有引号
- ✅ 链接PV存在
- ✅ 使用宏提高复用
- ✅ 添加注释说明

### 下一步

- [10-c-essentials.md](./10-c-essentials.md) - C语言基础
- [Part 6: Database Layer](../../part6-database-layer/) - 深入数据库层
- [Part 8: Labs](../part8-hands-on-labs/) - 动手实践

---

**🎉 恭喜！** 你已经掌握了EPICS数据库文件的语法！
