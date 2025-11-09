# 数据库设计

> **目标**: 设计高效可维护的数据库
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 3-5天

## 设计原则

### 1. 模块化设计

```
# 分离关注点
BPMmonitor.substitutions  # 主数据库
  ├── rf_inputs.substitutions   # RF输入
  ├── rf_outputs.substitutions  # RF输出
  └── registers.substitutions   # 寄存器
```

### 2. Record链接

#### calc链接

```
record(ai, "$(P):RFIn_01_Amp") {
    field(INP, "@AMP:0 ch=0")
    field(SCAN, ".5 second")
    field(FLNK, "$(P):SNR:Ch01")
}

record(calc, "$(P):SNR:Ch01") {
    field(INPA, "$(P):RFIn_01_Amp")
    field(INPB, "$(P):Noise:Ch01")
    field(CALC, "20*LOG(A/B)")
    field(EGU, "dB")
}
```

#### transform Record

```
record(transform, "$(P):BeamPosition") {
    field(INPA, "$(P):BPM:A")
    field(INPB, "$(P):BPM:B")
    field(INPC, "$(P):BPM:C")
    field(INPD, "$(P):BPM:D")
    field(CLCA, "(A+C)-(B+D)")  # X position
    field(CLCB, "(A+B)-(C+D)")  # Y position
    field(CMTA, "X Position")
    field(CMTB, "Y Position")
    field(OUTA, "$(P):Position:X PP")
    field(OUTB, "$(P):Position:Y PP")
}
```

## PV命名规范

### 层次化命名

```
系统:子系统:设备:参数[:属性]

示例：
LLRF:BPM:RFIn_01_Amp          # RF输入1幅度
LLRF:BPM:RFIn_01_Amp:RBV      # 回读值
LLRF:BPM:RFIn_01_Amp:Alarm    # 告警状态
```

## 告警配置

```
record(ai, "$(P):Temperature") {
    field(HIGH, "70")     # 高告警
    field(HSV, "MINOR")   # 高告警严重性
    field(HIHI, "80")     # 高高告警
    field(HHSV, "MAJOR")  # 高高告警严重性
}
```

## 🔗 相关文档

- [02-ca-programming.md](./02-ca-programming.md)
- [Part 6: Database Layer](../part6-database-layer/)
