# Week 1 - Database 文件深入

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 深入理解 EPICS Database 文件编写

---

## Day 1-2: Record 类型详解（4小时）

### 常用 Record 类型

| Record 类型 | 用途 | 示例 |
|------------|------|------|
| `ai` | 模拟输入 | 温度、电压读取 |
| `ao` | 模拟输出 | 电压设置 |
| `bi` | 数字输入 | 开关状态 |
| `bo` | 数字输出 | 继电器控制 |
| `calc` | 计算 | 数值运算 |
| `waveform` | 波形 | 数组数据 |

### ai Record 详解

```
record(ai, "BPM:CH0:Amp") {
    field(DESC, "Channel 0 Amplitude")   # 描述
    field(DTYP, "Soft Channel")          # 设备类型
    field(INP, "@some_address")          # 输入链接
    field(SCAN, "1 second")              # 扫描周期
    field(PREC, "3")                     # 精度
    field(EGU, "V")                      # 工程单位
    field(HOPR, "10.0")                  # 高显示限
    field(LOPR, "0.0")                   # 低显示限
    field(HIHI, "9.0")                   # 极高报警
    field(HIGH, "8.0")                   # 高报警
    field(LOW, "1.0")                    # 低报警
    field(LOLO, "0.5")                   # 极低报警
}
```

---

## Day 3-4: 链接和扫描（4小时）

### INP/OUT 链接

```
# 硬件链接
field(DTYP, "BPMmonitor")
field(INP, "@AMP:0 ch=0")

# 数据库链接（读取另一个 PV）
field(INP, "OtherPV CP")

# 计算链接
field(CALC, "A*B+C")
field(INPA, "PV1 CP")
field(INPB, "PV2 CP")
field(INPC, "PV3 CP")
```

### 扫描机制

```
# 被动扫描（不自动更新）
field(SCAN, "Passive")

# 周期扫描
field(SCAN, "1 second")
field(SCAN, ".1 second")
field(SCAN, "10 second")

# 事件扫描
field(SCAN, "Event")
field(EVNT, "1")

# I/O 中断扫描
field(SCAN, "I/O Intr")
```

---

## Day 5-6: 高级功能（4小时）

### calc Record

```
record(calc, "BPM:TotalPower") {
    field(DESC, "Total Power")
    field(CALC, "SQRT(A*A+B*B+C*C+D*D)")
    field(INPA, "BPM:CH0:Amp CP")
    field(INPB, "BPM:CH1:Amp CP")
    field(INPC, "BPM:CH2:Amp CP")
    field(INPD, "BPM:CH3:Amp CP")
    field(EGU, "V")
    field(PREC, "4")
}
```

### waveform Record

```
record(waveform, "BPM:CH0:Wave") {
    field(DESC, "Channel 0 Waveform")
    field(DTYP, "BPMmonitor")
    field(INP, "@WAVE:0 ch=0")
    field(FTVL, "FLOAT")    # 数据类型
    field(NELM, "1024")     # 元素数量
    field(SCAN, "I/O Intr")
}
```

---

## Day 7: 实战项目（2小时）

### 完整 BPM 数据库

创建 `BPMComplete.db`：

```
# === 原始数据 ===
record(ai, "BPM:CH$(CH):Amp") {
    field(DESC, "Channel $(CH) Amplitude")
    field(DTYP, "BPMmonitor")
    field(INP, "@AMP:0 ch=$(CH)")
    field(SCAN, "I/O Intr")
    field(EGU, "V")
    field(PREC, "3")
}

record(ai, "BPM:CH$(CH):Phase") {
    field(DESC, "Channel $(CH) Phase")
    field(DTYP, "BPMmonitor")
    field(INP, "@PHASE:0 ch=$(CH)")
    field(SCAN, "I/O Intr")
    field(EGU, "deg")
    field(PREC, "1")
}

# === 计算结果 ===
record(calc, "BPM:X:Pos") {
    field(DESC, "X Position")
    field(CALC, "(A-B)/(A+B)")
    field(INPA, "BPM:CH0:Amp CP")
    field(INPB, "BPM:CH2:Amp CP")
    field(EGU, "mm")
    field(PREC, "3")
}

record(calc, "BPM:Y:Pos") {
    field(DESC, "Y Position")
    field(CALC, "(A-B)/(A+B)")
    field(INPA, "BPM:CH1:Amp CP")
    field(INPB, "BPM:CH3:Amp CP")
    field(EGU, "mm")
    field(PREC, "3")
}

# === 波形数据 ===
record(waveform, "BPM:CH$(CH):Wave") {
    field(DESC, "Channel $(CH) Waveform")
    field(DTYP, "BPMmonitor")
    field(INP, "@WAVE:0 ch=$(CH)")
    field(FTVL, "FLOAT")
    field(NELM, "1024")
    field(SCAN, "I/O Intr")
}
```

使用宏展开：
```
# 在 st.cmd 中
dbLoadRecords("db/BPMComplete.db", "CH=0")
dbLoadRecords("db/BPMComplete.db", "CH=1")
dbLoadRecords("db/BPMComplete.db", "CH=2")
dbLoadRecords("db/BPMComplete.db", "CH=3")
```

---

## 练习

1. 创建包含 20+ PV 的完整数据库
2. 使用 calc record 进行数据处理
3. 添加报警配置
4. 使用宏实现通道复用

继续下周学习 Device Support！
