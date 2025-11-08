# 07 - 报警和归档

> **目标**: 理解EPICS的报警系统和数据归档
> **难度**: ⭐⭐☆☆☆
> **预计时间**: 40分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解EPICS报警机制
- ✅ 配置报警限值和严重性
- ✅ 了解报警传播和确认
- ✅ 理解数据归档的基本概念
- ✅ 在BPMIOC中应用报警策略

## 🚨 1. EPICS报警系统

### 报警的目的

报警系统帮助操作员：
- 🔴 快速识别异常状态
- 🟡 预警潜在问题
- ⚪ 跟踪系统健康状况
- 📊 记录历史事件

### 报警严重性级别

EPICS定义了4个严重性级别：

| 级别 | 枚举值 | 颜色 | 含义 | 示例 |
|------|-------|------|------|------|
| `NO_ALARM` | 0 | 🟢 绿色 | 正常 | 温度25°C |
| `MINOR` | 1 | 🟡 黄色 | 次要警告 | 温度75°C |
| `MAJOR` | 2 | 🔴 红色 | 主要警告 | 温度90°C |
| `INVALID` | 3 | ⚪ 白色 | 无效数据 | 传感器断开 |

### 报警状态类型

EPICS通过 `STAT` 字段指示报警原因：

| 状态 | 含义 | 示例 |
|------|------|------|
| `NO_ALARM` | 无报警 | 正常运行 |
| `READ` | 读取错误 | 硬件通信失败 |
| `WRITE` | 写入错误 | 设置失败 |
| `HIHI` | 高高限报警 | 超过HIHI值 |
| `HIGH` | 高限报警 | 超过HIGH值 |
| `LOLO` | 低低限报警 | 低于LOLO值 |
| `LOW` | 低限报警 | 低于LOW值 |
| `STATE` | 状态报警 | 状态不正常 |
| `COS` | Change of State | 状态变化 |
| `COMM` | 通信报警 | 网络问题 |
| `TIMEOUT` | 超时 | 响应超时 |
| `CALC` | 计算错误 | CALC表达式错误 |

## 📊 2. 配置报警限值

### ai记录的报警配置

```
record(ai, "$(P):Temperature1")
{
    field(SCAN, "1 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
    field(PREC, "1")
    field(EGU,  "C")

    # 报警限值
    field(HIHI, "85")       # 高高限：85°C
    field(HIGH, "70")       # 高限：70°C
    field(LOW,  "10")       # 低限：10°C
    field(LOLO, "5")        # 低低限：5°C

    # 报警严重性
    field(HHSV, "MAJOR")    # 高高严重性：主要
    field(HSV,  "MINOR")    # 高严重性：次要
    field(LSV,  "MINOR")    # 低严重性：次要
    field(LLSV, "MAJOR")    # 低低严重性：主要

    # 报警滞后（可选）
    field(HYST, "2")        # 滞后2°C
}
```

### 报警触发逻辑

```
值的范围:           报警状态:        严重性:

    ∞
    |
  85°C  ────────── HIHI ────────── MAJOR (红色)
    |
  70°C  ────────── HIGH ────────── MINOR (黄色)
    |
  正常区间 ──────── NO_ALARM ───── NO_ALARM (绿色)
    |
  10°C  ────────── LOW  ────────── MINOR (黄色)
    |
   5°C  ────────── LOLO ────────── MAJOR (红色)
    |
   -∞
```

### 报警滞后（HYST）

防止报警抖动：

```
设置: HIGH=70, HYST=2

温度变化:
68°C → 70°C → 72°C  # 72°C时触发HIGH报警
     ↓
72°C → 70°C → 68°C  # 68°C时（70-HYST）才清除报警

好处：在70°C附近小幅波动不会频繁触发/清除报警
```

## 🔔 3. BPMIOC中的报警应用

### 示例1: RF幅度报警

```bash
cd ~/BPMIOC
grep -A 15 "RFIn_01_Amp" BPMmonitorApp/Db/BPMMonitor.db
```

可能看到：

```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(PREC, "3")
    field(EGU,  "V")
    field(HOPR, "10")
    field(LOPR, "0")

    # 如果配置了报警
    field(HIHI, "9")
    field(HIGH, "8")
    field(LOW,  "0.5")
    field(LOLO, "0.1")

    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")
}
```

**报警场景**:
- RF信号过强（>8V）：可能损坏设备
- RF信号过弱（<0.5V）：束流丢失或硬件故障

### 示例2: 温度监控报警

```
record(ai, "$(P):Temperature1")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
    field(EGU,  "C")

    field(HIGH, "70")       # 警告：需要关注
    field(HIHI, "85")       # 危险：立即处理
    field(HSV,  "MINOR")
    field(HHSV, "MAJOR")
}
```

## 📡 4. 报警传播

### 使用MS标志传播报警

```
record(ai, "Sensor")
{
    field(SCAN, "1 second")
    field(HIHI, "100")
    field(HHSV, "MAJOR")
}

record(calc, "Derived")
{
    field(SCAN, "Passive")
    field(INPA, "Sensor CP MS")  # MS = Maximize Severity
    field(CALC, "A*2")
}
```

**行为**:
```
Sensor = 105°C
    ↓
触发HIHI报警，SEVR = MAJOR
    ↓ CP MS传播
Derived继承MAJOR严重性
```

### 不传播的情况

```
record(calc, "Derived2")
{
    field(INPA, "Sensor CP")    # 没有MS！
    field(CALC, "A*2")
}
```

**行为**: Derived2不会继承Sensor的报警，只获取数值。

## 🔍 5. 查询和监控报警

### 使用caget查看报警

```bash
# 查看值、状态和严重性
caget -tS iLinac_007:BPM14And15:Temperature1

# 输出示例（正常）:
# iLinac_007:BPM14And15:Temperature1 2025-11-08 10:30:45.123456 25.5 NO_ALARM

# 输出示例（报警）:
# iLinac_007:BPM14And15:Temperature1 2025-11-08 10:35:22.654321 75.2 MINOR HIGH
```

### 使用camonitor监控报警变化

```bash
camonitor -S iLinac_007:BPM14And15:Temperature1

# 输出示例:
# iLinac_007:BPM14And15:Temperature1 25.5 NO_ALARM
# iLinac_007:BPM14And15:Temperature1 71.2 MINOR HIGH  ← 报警触发
# iLinac_007:BPM14And15:Temperature1 68.5 NO_ALARM    ← 报警清除
```

### Python获取报警信息

```python
import epics

pv = epics.PV('iLinac_007:BPM14And15:Temperature1')

# 获取当前值和报警
value = pv.get()
severity = pv.severity  # 0=NO_ALARM, 1=MINOR, 2=MAJOR, 3=INVALID
status = pv.status      # 状态描述

print(f"值: {value}")
print(f"严重性: {severity}")
print(f"状态: {status}")

# 监控报警变化
def alarm_callback(pvname=None, severity=None, **kws):
    if severity > 0:
        print(f"⚠️  报警: {pvname} 严重性={severity}")

pv = epics.PV('iLinac_007:BPM14And15:Temperature1',
              callback=alarm_callback,
              auto_monitor=True)
```

## 💾 6. 数据归档基础

### 为什么需要归档

- 📈 长期趋势分析
- 🔍 故障回溯
- 📊 性能优化
- 📝 合规记录

### EPICS归档工具

常用的归档工具：

| 工具 | 特点 | 适用场景 |
|------|------|---------|
| **Channel Archiver** | 官方工具，成熟稳定 | 传统EPICS系统 |
| **Archiver Appliance** | 现代化，高性能 | 大规模系统 |
| **pvaPy** | Python接口 | 自定义归档 |
| **时序数据库** | InfluxDB, Prometheus | 现代监控栈 |

### 归档配置示例（Channel Archiver）

假设有归档配置文件 `archive.cfg`:

```
# 高频率归档（重要参数）
<channel>
    <name>iLinac_007:BPM14And15:RFIn_01_Amp</name>
    <period>1</period>        # 1秒
    <monitor/>                # 监控模式（值变化时归档）
</channel>

# 低频率归档（慢速数据）
<channel>
    <name>iLinac_007:BPM14And15:Temperature1</name>
    <period>60</period>       # 60秒
    <scan/>                   # 扫描模式（定期采样）
</channel>
```

### 归档策略建议

| 数据类型 | 归档频率 | 保留时间 | 原因 |
|---------|---------|---------|------|
| RF幅度/相位 | 1-10秒 | 1个月 | 快速变化，重要 |
| BPM位置 | 1-10秒 | 1个月 | 束流质量分析 |
| 温度 | 1分钟 | 6个月 | 慢速变化 |
| 配置参数 | 值变化时 | 1年+ | 配置历史 |
| 报警事件 | 立即 | 长期 | 故障分析 |

## 🧪 实践练习

### 练习1: 添加温度报警

为BPMIOC添加温度报警配置：

```
要求:
- 正常范围: 20-60°C
- 警告: <15°C 或 >65°C (MINOR)
- 严重: <10°C 或 >70°C (MAJOR)
- 滞后: 2°C
```

<details>
<summary>答案</summary>

```
record(ai, "$(P):Temperature1")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")
    field(PREC, "1")
    field(EGU,  "C")

    field(HIHI, "70")
    field(HIGH, "65")
    field(LOW,  "15")
    field(LOLO, "10")

    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")

    field(HYST, "2")
}
```
</details>

### 练习2: 报警通知calc记录

创建一个calc记录，当任何RF通道报警时输出1：

<details>
<summary>答案</summary>

```
record(calc, "$(P):AnyRFAlarm")
{
    field(SCAN, "Passive")
    field(CALC, "(A||B||C||D||E||F||G||H)?1:0")
    field(INPA, "$(P):RFIn_01_Amp.SEVR CP")
    field(INPB, "$(P):RFIn_02_Amp.SEVR CP")
    field(INPC, "$(P):RFIn_03_Amp.SEVR CP")
    field(INPD, "$(P):RFIn_04_Amp.SEVR CP")
    field(INPE, "$(P):RFIn_05_Amp.SEVR CP")
    field(INPF, "$(P):RFIn_06_Amp.SEVR CP")
    field(INPG, "$(P):RFIn_07_Amp.SEVR CP")
    field(INPH, "$(P):RFIn_08_Amp.SEVR CP")
    field(PREC, "0")
}

# 注意: .SEVR 是访问严重性字段的语法
# SEVR>0 表示有报警（MINOR或MAJOR）
```
</details>

### 练习3: 设计报警抑制

在系统启动时抑制报警（给硬件预热时间）：

<details>
<summary>答案</summary>

```
# 启动标志
record(bo, "$(P):SystemReady")
{
    field(DESC, "System Ready Flag")
    field(ZNAM, "Warming Up")
    field(ONAM, "Ready")
}

# 温度记录使用SDIS抑制
record(ai, "$(P):Temperature1")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@REG:29 ch=0")

    # 报警配置
    field(HIHI, "70")
    field(HHSV, "MAJOR")

    # 抑制配置
    field(SDIS, "$(P):SystemReady")  # 扫描禁用链接
    field(DISV, "0")                 # 当SystemReady=0时禁用
}

# 启动脚本中，延迟设置SystemReady
# dbpf "iLinac_007:BPM14And15:SystemReady" "1"
```
</details>

## 📊 7. 报警最佳实践

### 1. 设置合理的限值

```
❌ 不好: HIHI=1000 (永不触发)
✅ 好: HIHI=根据实际操作经验设置

❌ 不好: 所有参数都设MAJOR
✅ 好: 区分MINOR（警告）和MAJOR（紧急）

❌ 不好: 没有滞后，报警抖动
✅ 好: 设置适当的HYST
```

### 2. 报警分级

```
MAJOR (红色，立即处理):
  - 设备可能损坏
  - 束流丢失
  - 安全问题

MINOR (黄色，需要关注):
  - 性能下降
  - 接近限值
  - 异常但可运行

NO_ALARM (绿色):
  - 正常运行
```

### 3. 避免报警洪水

```
问题: 100个PV同时报警，操作员无从下手

解决方案:
  1. 设置主报警PV（汇总）
  2. 使用报警组
  3. 优先级排序
  4. 根源报警分析
```

### 4. 报警文档化

每个报警应该有：
- 📝 触发条件
- 🔍 可能原因
- 🛠️ 处理步骤
- 📞 联系人

## 📚 报警字段速查表

### 报警限值字段（ai记录）

| 字段 | 含义 | 示例 |
|------|------|------|
| `HIHI` | 高高限值 | 90 |
| `HIGH` | 高限值 | 80 |
| `LOW` | 低限值 | 10 |
| `LOLO` | 低低限值 | 5 |

### 报警严重性字段

| 字段 | 含义 | 可选值 |
|------|------|-------|
| `HHSV` | 高高严重性 | NO_ALARM/MINOR/MAJOR/INVALID |
| `HSV` | 高严重性 | NO_ALARM/MINOR/MAJOR/INVALID |
| `LSV` | 低严重性 | NO_ALARM/MINOR/MAJOR/INVALID |
| `LLSV` | 低低严重性 | NO_ALARM/MINOR/MAJOR/INVALID |

### 其他报警字段

| 字段 | 含义 | 说明 |
|------|------|------|
| `HYST` | 报警滞后 | 防止抖动 |
| `AFTC` | Alarm Filter Time Constant | 报警滤波 |
| `AFVL` | Alarm Filter Value | 滤波后的值 |

### 只读报警状态字段

| 字段 | 含义 | 类型 |
|------|------|------|
| `SEVR` | 当前严重性 | 0-3 |
| `STAT` | 当前状态 | 枚举 |
| `ACKS` | 确认严重性 | 0-3 |
| `ACKT` | 确认转换 | YES/NO |

## 🔗 相关文档

- [Part 6: 07-alarm-configuration.md](../../part6-database-layer/07-alarm-configuration.md) - 报警配置详解
- [Part 13: 06-alarm-system.md](../../part13-deployment/06-alarm-system.md) - 生产环境报警
- [EPICS Application Developer's Guide - Chapter 6: Database Scanning](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/DatabaseScanning.html)

## 📝 总结

### 关键要点

1. **四级严重性**: NO_ALARM < MINOR < MAJOR < INVALID
2. **双限值系统**: HIGH/LOW(警告) + HIHI/LOLO(严重)
3. **滞后防抖**: HYST避免报警抖动
4. **MS传播**: 让派生PV继承报警
5. **合理归档**: 根据数据特性选择频率

### 报警配置检查清单

- ✅ 限值是否基于实际操作经验？
- ✅ MINOR和MAJOR是否区分明确？
- ✅ 是否设置了滞后（HYST）？
- ✅ 重要的派生PV是否使用MS传播？
- ✅ 是否有报警处理文档？

### 下一步

- [08-ca-tools.md](./08-ca-tools.md) - Channel Access工具
- [Part 13: Deployment](../../part13-deployment/) - 生产环境配置

---

**🎉 恭喜！** 你已经掌握了EPICS报警和归档的基础知识！
