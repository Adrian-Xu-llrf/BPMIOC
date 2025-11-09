# Part 6.3: PV命名规范详解

> **目标**: 掌握PV命名的最佳实践和BPMIOC的命名体系
> **难度**: ⭐⭐⭐☆☆
> **时间**: 45分钟

## 📖 为什么需要命名规范？

**问题**: 如果没有命名规范

```
# ❌ 糟糕的命名
record(ai, "Amp1") { ... }
record(ai, "V2") { ... }
record(ai, "BPM_Position_X_Channel_1") { ... }  # 太长
record(ai, "x1") { ... }  # 太模糊
```

**结果**:
- 🔴 维护困难（不知道PV的含义）
- 🔴 扩展困难（命名冲突）
- 🔴 集成困难（不同系统的PV混在一起）

**解决方案**: 统一的命名规范！

---

## 🏗️ EPICS命名规范基础

### 标准结构

```
<系统>:<子系统>:<设备>:<信号>:<属性>
  │      │       │      │      │
  │      │       │      │      └─> 可选：RBV, SP等
  │      │       │      └────────> 数据类型：Amp, Phs等
  │      │       └───────────────> 设备实例：BPM01等
  │      └───────────────────────> 子系统：LLRF, Diag等
  └──────────────────────────────> 加速器系统：iLinac等
```

**分隔符**: 使用`:`（推荐）或`_`

---

### 基本原则

1. **层次性**: 从大到小（系统→设备→信号）
2. **可读性**: 名称能表达含义
3. **唯一性**: 全局唯一
4. **一致性**: 同类PV使用相同模式
5. **可扩展性**: 容易添加新设备

---

## 🎯 BPMIOC的命名体系

### 实际案例分析

```
iLinac_007:BPM14:RFIn_01_Amp
    │         │       │    │
    │         │       │    └─> 数据类型（幅度）
    │         │       └──────> 通道号（01）
    │         └──────────────> 设备名（BPM14）
    └────────────────────────> 系统名（iLinac_007）
```

**完整示例**:
```bash
# RF输入幅度
iLinac_007:BPM14:RFIn_01_Amp
iLinac_007:BPM14:RFIn_02_Amp
...
iLinac_007:BPM14:RFIn_16_Amp

# RF输出幅度
iLinac_007:BPM14:RFOut_01_Amp
...
iLinac_007:BPM14:RFOut_04_Amp

# XY位置
iLinac_007:BPM14:XY1_X
iLinac_007:BPM14:XY1_Y
iLinac_007:BPM14:XY2_X
iLinac_007:BPM14:XY2_Y
```

---

### 宏替换实现

**.db文件**:
```
record(ai, "$(P):RFIn_01_Amp")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
}
```

**st.cmd中替换**:
```bash
dbLoadRecords("BPMMonitor.db", "P=iLinac_007:BPM14")
                                ↑
                              宏定义
```

**结果**:
```
PV名 = "iLinac_007:BPM14:RFIn_01_Amp"
```

**优势**:
- ✅ .db文件通用（可复用）
- ✅ 不同设备只需改宏
- ✅ 易于批量部署

---

## 📊 命名模式详解

### 模式1: 通道编号

**适用**: 多个相似通道

```
$(P):RFIn_01_Amp
$(P):RFIn_02_Amp
$(P):RFIn_03_Amp
...
$(P):RFIn_16_Amp
```

**编号规则**:
- 从01开始（不是0）
- 使用2位数（01, 02, ..., 16）
- 保持位数一致（便于排序）

---

### 模式2: 数据类型后缀

**适用**: 同一信号的不同属性

```
$(P):RF3_Amp      # 幅度
$(P):RF3_Phs      # 相位
$(P):RF3_Real     # 实部
$(P):RF3_Imag     # 虚部
```

**常用后缀**:

| 后缀 | 含义 | 示例 |
|------|------|------|
| `Amp` | Amplitude（幅度） | `RF3_Amp` |
| `Phs` | Phase（相位） | `RF3_Phs` |
| `Real` | Real part（实部） | `RF3_Real` |
| `Imag` | Imaginary part（虚部） | `RF3_Imag` |
| `X`, `Y` | 位置坐标 | `XY1_X`, `XY1_Y` |
| `Freq` | Frequency（频率） | `Osc_Freq` |
| `Temp` | Temperature（温度） | `Amp_Temp` |

---

### 模式3: 设备实例

**适用**: 多个相同类型设备

```
# 使用设备编号
iLinac_007:BPM14:...
iLinac_007:BPM15:...
iLinac_007:BPM16:...

# 或使用位置
iLinac_007:BPM_North:...
iLinac_007:BPM_South:...
```

---

### 模式4: 读写区分

**适用**: 可读写的参数

```
$(P):Gain          # 读当前值（Readback）
$(P):Gain:SP       # 写设定值（Setpoint）

或者：
$(P):Gain:RBV      # Readback Value
$(P):Gain:Set      # Set Value
```

**示例**:
```
record(ai, "$(P):Gain:RBV")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@GAIN:0")
    field(SCAN, ".5 second")
}

record(ao, "$(P):Gain:Set")
{
    field(DTYP, "BPMmonitor")
    field(OUT,  "@GAIN:0")
    field(FLNK, "$(P):Gain:RBV")  # 写后读回
}
```

---

## 🎨 命名最佳实践

### 1. 长度控制

**EPICS限制**: PV名最长60个字符

**建议**:
- ✅ 控制在40个字符内
- ✅ 使用缩写（但要有文档说明）
- ❌ 避免无意义的缩写

**示例**:
```
# ✅ 好：清晰且不太长
iLinac_007:BPM14:RFIn_01_Amp  (33字符)

# ❌ 差：太长
iLinac_007:BeamPositionMonitor14:RadioFrequencyInput01:Amplitude  (72字符)

# ❌ 差：缩写无意义
iL7:B14:R1A  (11字符，但难以理解)
```

---

### 2. 大小写规范

**推荐**: 使用一致的大小写风格

**选项A: 驼峰命名**
```
iLinac:BPM14:RfInAmp01
```

**选项B: 下划线分隔（BPMIOC采用）**
```
iLinac_007:BPM14:RFIn_01_Amp
```

**关键**: 全项目保持一致！

---

### 3. 分隔符使用

**系统间**: 使用`:`
```
iLinac_007:BPM14:RFIn_01_Amp
    ↑       ↑     ↑
    └───────┴─────┴─ 用:分隔
```

**单词间**: 使用`_`
```
RFIn_01_Amp
   ↑  ↑
   └──┴─ 用_分隔
```

**避免**:
- ❌ 混用分隔符（如`BPM-14:RF_In`）
- ❌ 使用特殊字符（如`@`, `#`, `$`）

---

### 4. 编号规范

**使用前导零**:
```
# ✅ 好：便于排序
RFIn_01_Amp
RFIn_02_Amp
...
RFIn_10_Amp

# ❌ 差：排序混乱
RFIn_1_Amp
RFIn_2_Amp
...
RFIn_10_Amp  # 排在RFIn_1之前！
```

**从1开始还是从0开始？**

```
# 硬件通道（从0开始）
ch=0, ch=1, ch=2, ...

# PV名称（从1开始，用户友好）
RFIn_01, RFIn_02, RFIn_03, ...
```

---

## 🔍 BPMIOC完整命名示例

### 1. RF信号

```
# 输入幅度
$(P):RFIn_01_Amp ... $(P):RFIn_16_Amp

# 输出幅度
$(P):RFOut_01_Amp ... $(P):RFOut_04_Amp

# 相位
$(P):RFIn_01_Phs ... $(P):RFIn_16_Phs
$(P):RFOut_01_Phs ... $(P):RFOut_04_Phs

# 实部/虚部
$(P):RFIn_01_Real, $(P):RFIn_01_Imag
```

---

### 2. 位置信号

```
# XY位置（4个通道，各有X和Y）
$(P):XY1_X, $(P):XY1_Y
$(P):XY2_X, $(P):XY2_Y
$(P):XY3_X, $(P):XY3_Y
$(P):XY4_X, $(P):XY4_Y
```

---

### 3. 按钮信号

```
$(P):Button_01 ... $(P):Button_04
```

---

### 4. 控制PV

```
$(P):SetReg        # 寄存器设置
$(P):TriggerMode   # 触发模式
$(P):SampleRate    # 采样率
```

---

## 🔧 多设备部署

### 场景：两个BPM

```bash
# st.cmd

# BPM14
dbLoadRecords("BPMMonitor.db", "P=iLinac_007:BPM14")

# BPM15
dbLoadRecords("BPMMonitor.db", "P=iLinac_007:BPM15")
```

**结果PV**:
```
iLinac_007:BPM14:RFIn_01_Amp
iLinac_007:BPM14:RFIn_02_Amp
...

iLinac_007:BPM15:RFIn_01_Amp
iLinac_007:BPM15:RFIn_02_Amp
...
```

**优势**:
- ✅ 一个.db文件支持多个设备
- ✅ PV名自动不冲突
- ✅ 易于扩展

---

## ⚠️ 常见错误

### 错误1: PV名冲突

```bash
# ❌ 错误：两次加载，宏相同
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM")
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM")
# 结果：PV名重复，IOC启动失败！

# ✅ 正确：使用不同的宏
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM1")
dbLoadRecords("BPMMonitor.db", "P=LLRF:BPM2")
```

---

### 错误2: 包含空格

```
# ❌ 错误
record(ai, "RF Input 01")  # PV名不能包含空格！

# ✅ 正确
record(ai, "RFInput_01")
```

---

### 错误3: 过度缩写

```
# ❌ 差：难以理解
$(P):RI1A   # RFIn_01_Amp?

# ✅ 好：清晰
$(P):RFIn_01_Amp
```

---

## 📚 命名规范文档化

### 创建命名规范文档

**示例文档**:
```markdown
# BPM IOC PV命名规范

## 格式
<System>:<Device>:<Signal>_<Channel>_<Type>

## 系统名（System）
- iLinac_007: 注入器直线加速器7号
- HEBT: 高能传输线

## 设备名（Device）
- BPM14, BPM15: BPM设备编号

## 信号名（Signal）
- RFIn: RF输入
- RFOut: RF输出
- XY: 位置

## 通道号（Channel）
- 01-16: 通道编号（两位数）

## 类型（Type）
- Amp: 幅度
- Phs: 相位
- Real/Imag: 实部/虚部

## 示例
iLinac_007:BPM14:RFIn_01_Amp  # 注入器7号BPM14的RF输入通道1幅度
```

---

## 🎯 PV发现和文档

### 使用dbgrep查找PV

```bash
# 列出所有PV
dbgrep "*" | sort

# 查找BPM14的所有PV
dbgrep "iLinac_007:BPM14:*"

# 查找所有幅度PV
dbgrep "*:*Amp"
```

---

### 自动生成PV列表

**在st.cmd中**:
```bash
# 启动后，输出所有PV到文件
dbl > /tmp/pv_list.txt

# 或带Record类型
dbl ai > /tmp/ai_pvs.txt
```

---

## ✅ 学习检查点

- [ ] 理解PV命名的重要性
- [ ] 掌握BPMIOC的命名模式
- [ ] 能够使用宏实现可复用的.db文件
- [ ] 知道命名的最佳实践
- [ ] 能够为新项目设计命名规范

---

## 🎯 总结

### 命名规范的核心

**好的命名应该**:
1. **自描述**: 看到名字就知道含义
2. **分层次**: 系统→设备→信号
3. **可扩展**: 易于添加新设备
4. **一致性**: 全项目统一风格

### 实施建议

1. **项目初期**: 制定命名规范文档
2. **开发过程**: 严格遵守规范
3. **代码审查**: 检查命名一致性
4. **文档维护**: 及时更新规范文档

**下一步**: 学习Part 6.4 - 扫描机制详解，理解SCAN字段的各种配置！

---

**关键理解**: 好的PV命名不仅是技术问题，更是工程管理问题 - 它影响系统的可维护性和可扩展性！💡
