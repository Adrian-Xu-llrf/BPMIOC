# Lab 9: 实现校准功能

> **难度**: ⭐⭐⭐
> **时间**: 3小时
> **前置**: Lab 6-8, Part 6.2

## 🎯 实验目标

使用calc Record实现数据校准和转换功能。

---

## 📚 背景知识

### 为什么需要校准？

```
原始ADC值 → 校准转换 → 物理单位

例如：
ADC: 2048 (12-bit)
↓ 线性校准
Voltage: 5.0V
↓ 增益校准
Power: 10.0 dBm
```

---

## 🔧 实验任务

### 任务1: RF功率校准

**需求**: 将ADC值转换为dBm功率值

---

### 步骤1: 添加校准calc Record

```
# 读取原始ADC值
record(ai, "$(P):RFIn_01_Amp_Raw")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(SCAN, ".5 second")
}

# 校准为电压 (假设: 0-4095 → 0-10V)
record(calc, "$(P):RFIn_01_Volt")
{
    field(DESC, "RF Input 1 Voltage")
    field(INPA, "$(P):RFIn_01_Amp_Raw CP")
    field(CALC, "A*10.0/4095.0")
    field(PREC, "3")
    field(EGU,  "V")
}

# 校准为功率 (假设: V = 10^(P/20) * Vref)
record(calc, "$(P):RFIn_01_Power")
{
    field(DESC, "RF Input 1 Power")
    field(INPA, "$(P):RFIn_01_Volt CP")
    field(INPB, "1.0")  # Vref
    field(CALC, "20*LOG(A/B)")
    field(PREC, "2")
    field(EGU,  "dBm")
}
```

**字段说明**:
- `CP`: Cause Process - 当INPA更新时处理此calc Record
- `LOG()`: calc支持的对数函数

---

### 步骤2: 添加增益校准

```
# 用户可调的增益校准值
record(ao, "$(P):RFIn_01_Gain")
{
    field(DESC, "Gain Calibration")
    field(PINI, "YES")
    field(VAL,  "1.0")
    field(PREC, "3")
}

# 应用增益校准
record(calc, "$(P):RFIn_01_Calibrated")
{
    field(INPA, "$(P):RFIn_01_Power CP")
    field(INPB, "$(P):RFIn_01_Gain CP")
    field(CALC, "A*B")
    field(PREC, "2")
    field(EGU,  "dBm")
}
```

**测试**:
```bash
# 读取校准后的值
caget iLinac_007:BPM14:RFIn_01_Calibrated

# 调整增益
caput iLinac_007:BPM14:RFIn_01_Gain 1.1

# 再次读取，值应变化
caget iLinac_007:BPM14:RFIn_01_Calibrated
```

---

### 步骤3: 温度补偿

```
record(calc, "$(P):RFIn_01_TempComp")
{
    field(INPA, "$(P):RFIn_01_Power CP")
    field(INPB, "$(P):Temperature CP")
    field(INPC, "0.01")  # 温度系数 (dBm/°C)
    field(CALC, "A-C*(B-25.0)")  # 补偿到25°C基准
    field(PREC, "2")
    field(EGU,  "dBm")
}
```

---

## 🚀 扩展挑战

### 挑战1: 多项式校准

使用多项式实现非线性校准：

```
record(calc, "$(P):Calibrated")
{
    field(INPA, "$(P):Raw CP")
    field(INPB, "$(P):Cal_C0")  # 常数项
    field(INPC, "$(P):Cal_C1")  # 一次项
    field(INPD, "$(P):Cal_C2")  # 二次项
    field(CALC, "B+C*A+D*A*A")
}
```

### 挑战2: 查表校准

使用一组calc Record实现分段线性插值。

---

## ✅ 学习检查点

- [ ] 理解校准的必要性
- [ ] 掌握calc Record的CALC表达式
- [ ] 能够实现线性和非线性校准
- [ ] 理解CP/CPP链接模式

---

**恭喜完成Lab 9！** 你已掌握数据校准技能！💪
