# Lab 10: 添加报警配置

> **难度**: ⭐⭐
> **时间**: 2小时
> **前置**: Lab 6, Part 6.2

## 🎯 实验目标

学会配置Record的报警限值和级别，实现实时监控。

---

## 📚 背景知识

### EPICS报警机制

```
报警限值：
  HIHI (High High) ─┐
  HIGH (High)      ─┤ 上限
                    │
  正常范围          │
                    │
  LOW (Low)        ─┤ 下限
  LOLO (Low Low)   ─┘

报警级别：
  NO_ALARM  - 无报警
  MINOR     - 次要报警（黄色）
  MAJOR     - 主要报警（红色）
  INVALID   - 无效数据
```

---

## 🔧 实验任务

### 任务1: 为RF幅度添加报警

---

### 步骤1: 编辑BPMMonitor.db

```
record(ai, "$(P):RFIn_01_Amp")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(SCAN, ".5 second")
    field(PREC, "3")
    field(EGU,  "V")

    # 报警配置 ← 新添加
    field(HIHI, "9.0")      # 9V以上: MAJOR报警
    field(HIGH, "8.0")      # 8V以上: MINOR报警
    field(LOW,  "1.0")      # 1V以下: MINOR报警
    field(LOLO, "0.5")      # 0.5V以下: MAJOR报警

    field(HHSV, "MAJOR")    # High High Severity
    field(HSV,  "MINOR")    # High Severity
    field(LSV,  "MINOR")    # Low Severity
    field(LLSV, "MAJOR")    # Low Low Severity

    field(HYST, "0.1")      # 滞后（避免抖动）
}
```

---

### 步骤2: 测试报警

**查看报警状态**:
```bash
caget -tS iLinac_007:BPM14:RFIn_01_Amp

# 正常情况输出：
iLinac_007:BPM14:RFIn_01_Amp 2024-11-09 12:30:15.123 5.5 NO_ALARM NO_SEVERITY

# 高限报警输出：
iLinac_007:BPM14:RFIn_01_Amp 2024-11-09 12:30:20.456 8.2 HIGH MINOR

# 高高限报警输出：
iLinac_007:BPM14:RFIn_01_Amp 2024-11-09 12:30:25.789 9.5 HIHI MAJOR
```

---

### 步骤3: 监控报警事件

**使用camonitor -S**:
```bash
camonitor -S iLinac_007:BPM14:RFIn_01_Amp

# 只在报警状态变化时输出
iLinac_007:BPM14:RFIn_01_Amp 2024-11-09 12:30:15 5.5 NO_ALARM
iLinac_007:BPM14:RFIn_01_Amp 2024-11-09 12:30:20 8.2 HIGH MINOR  ← 报警！
iLinac_007:BPM14:RFIn_01_Amp 2024-11-09 12:30:45 5.1 NO_ALARM   ← 恢复
```

---

## 📝 任务2: 配置温度报警

```
record(ai, "$(P):Temperature")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@TEMP:10")
    field(SCAN, "5 second")
    field(PREC, "1")
    field(EGU,  "C")

    # 温度报警
    field(HIHI, "85")       # 85°C以上: MAJOR (过热)
    field(HIGH, "75")       # 75°C以上: MINOR (偏高)
    field(LOW,  "0")        # 0°C以下: MINOR (偏低)
    field(LOLO, "-10")      # -10°C以下: MAJOR (异常)

    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")
}
```

---

## 🔍 深入理解

### 滞后（Hysteresis）

**问题**: 值在限值附近抖动导致报警频繁切换

```
无滞后:
  8.0 → HIGH
  7.9 → NO_ALARM
  8.0 → HIGH
  7.9 → NO_ALARM  (频繁切换！)

有滞后(HYST=0.1):
  8.0 → HIGH
  7.95 → HIGH (仍在HIGH)
  7.9 → NO_ALARM
  7.85 → NO_ALARM (仍在NO_ALARM)
```

---

### ACK字段（Acknowledge）

用户可确认报警：

```bash
# 查看ACK状态
caget iLinac_007:BPM14:RFIn_01_Amp.ACKT

# 确认报警
caput iLinac_007:BPM14:RFIn_01_Amp.ACKS 2  # 2 = Acknowledge
```

---

## 🚀 扩展挑战

### 挑战1: 级联报警

温度过高时自动触发风扇：

```
# calc Record监控温度
record(calc, "$(P):Fan_Control")
{
    field(INPA, "$(P):Temperature CP")
    field(CALC, "(A>70)?100:50")  # 超过70°C全速
    field(OUT,  "$(P):Fan_Speed PP")
}
```

### 挑战2: 报警日志

创建stringout Record记录报警事件。

---

## ✅ 学习检查点

- [ ] 理解HIHI/HIGH/LOW/LOLO的作用
- [ ] 能够配置报警级别
- [ ] 理解滞后机制
- [ ] 能够监控报警状态变化

---

**恭喜完成Lab 10！** 修改实验全部完成！准备进入高级实验！💪
