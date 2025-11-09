# 05: Offset系统设计详解

> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 70分钟
> **前置知识**: 01-architecture-overview.md, 02-data-flow.md, 04-memory-model.md

## 📋 本文目标

本文深入剖析BPMIOC的Offset系统，这是连接数据库层和驱动层的关键机制。

完成本文后，你将能够：
- ✅ 理解Offset系统的设计理念
- ✅ 掌握如何通过Offset访问不同类型的数据
- ✅ 理解Offset、Channel、Type三个参数的作用
- ✅ 能够添加新的Offset类型

## 🎯 什么是Offset系统？

### 1. 问题背景

BPMIOC需要处理很多不同类型的数据：
- RF幅度和相位（8个通道）
- BPM位置（X1, Y1, X2, Y2）
- FPGA状态（LED、寄存器）
- 历史数据准备状态
- ...

**问题**：如何让设备支持层告诉驱动层"我要读取RF3的幅度"？

**传统方案1**：为每种数据类型定义一个函数

```c
// 需要定义很多函数（不好）
float GetRF3Amp();
float GetRF3Phase();
float GetRF4Amp();
float GetRF4Phase();
// ... 上百个函数
```

❌ **缺点**：
- 函数太多，难以维护
- 添加新通道需要修改大量代码
- 接口不统一

**传统方案2**：字符串标识

```c
float ReadData(const char *name);  // ReadData("RF3Amp")
```

❌ **缺点**：
- 字符串比较慢
- 容易拼写错误
- 内存开销大

**BPMIOC的方案**：**Offset系统**

```c
// 统一接口，用整数offset标识数据类型
float ReadData(int offset, int channel, int type);

// 示例
value = ReadData(0, 3, AMP);   // 读取RF3幅度
value = ReadData(0, 3, PHASE); // 读取RF3相位
value = ReadData(7, 0, 0);     // 读取X1位置
```

✅ **优点**：
- **统一接口**：所有数据都通过同一个函数访问
- **性能高**：整数比较和switch比字符串快得多
- **可扩展**：添加新类型只需添加新的case
- **类型安全**：编译时检查，避免拼写错误

## 🏗️ Offset系统架构

### 1. 三个关键参数

```c
float ReadData(int offset, int channel, int type);
             ↓        ↓          ↓        ↓
           返回值   数据类别   通道号   数据类型
```

**参数解释**：

| 参数 | 类型 | 含义 | 示例 |
|------|------|------|------|
| `offset` | int | 数据类别（最重要） | 0=RF信息, 1=DI, 7=XY位置 |
| `channel` | int | 通道号（对于多通道数据） | 0-7 (RF通道0-7) |
| `type` | int | 数据类型 | 3=AMP, 4=PHASE, 5=POWER |
| 返回值 | float | 读取到的数据值 | 3.14（幅度值） |

### 2. Offset类别定义

```c
// driverWrapper.c 中 ReadData() 函数的 switch 分支

case 0:  // RF信息（幅度、相位、功率）
    if (type == 3)       return GetRFInfo(channel, 0);  // 幅度
    else if (type == 4)  return GetRFInfo(channel, 1);  // 相位
    else                 return GetRFInfo(channel, 0);  // 默认幅度

case 1:  // 数字输入
    return GetDI(channel);

case 2:  // FPGA LED0状态
    return GetFPGA_LED0_RBK();

case 3:  // FPGA LED1状态
    return GetFPGA_LED1_RBK();

case 4:  // 历史数据准备状态
    return GetHistoryDataReady();

case 5:  // BPM ABCD电压值
    return GetVabcdValue(channel);  // channel = 0-7 (Va1,Vb1,Vc1,Vd1,Va2,Vb2,Vc2,Vd2)

case 6:  // BPM相位值
    return funcGetBPMPhaseValue(channel-2);

case 7:  // BPM XY位置
    return GetXYPosition(channel);  // channel = 0:X1, 1:Y1, 2:X2, 3:Y2

case 8:  // BPM电压和
    return GetVsumValue(channel);

case 9:  // XY保护状态
    return funcGetxyProtect(channel);

case 10-11:  // BPM1差分电压
    return (GetVabcdValue(0) - GetVabcdValue(2));  // Va1 - Vc1 = X方向
    return (GetVabcdValue(1) - GetVabcdValue(3));  // Vb1 - Vd1 = Y方向

case 12-13:  // BPM2差分电压
    return (GetVabcdValue(4) - GetVabcdValue(6));  // Va2 - Vc2 = X方向
    return (GetVabcdValue(5) - GetVabcdValue(7));  // Vb2 - Vd2 = Y方向

case 14-15:  // BPM电压总和
    return (Va1+Vb1+Vc1+Vd1);  // BPM1总和
    return (Va2+Vb2+Vc2+Vd2);  // BPM2总和

case 16-19:  // BPM对向电压和
    return (Va1 + Vc1);  // X方向和
    return (Vb1 + Vd1);  // Y方向和
    // ...

case 20-23:  // BPM归一化位置
    return (Va1 - Vc1) / (Va1 + Vc1);  // X1归一化
    return (Vb1 - Vd1) / (Vb1 + Vd1);  // Y1归一化
    // ...

case 24:  // ABCD电压转换（V）
    return (GetVabcdValue(channel) / 1.28E+6) * sqrt(2);

case 25-28:  // 差分电压转换（V）
    return ((Va1 - Vc1) / 1.28E+6) * sqrt(2);
    // ...
```

### 3. Offset分类表

| Offset | 类别 | Channel | Type | 示例PV |
|--------|------|---------|------|--------|
| 0 | RF信息 | 0-7 | 3(AMP)/4(PHASE) | `LLRF:BPM:RF3Amp` |
| 1 | 数字输入 | 0-N | - | `LLRF:BPM:DI0` |
| 2 | FPGA_LED0 | - | - | `LLRF:BPM:LED0` |
| 3 | FPGA_LED1 | - | - | `LLRF:BPM:LED1` |
| 4 | 历史数据状态 | - | - | `LLRF:BPM:HistoryReady` |
| 5 | ABCD原始电压 | 0-7 | - | `LLRF:BPM:Va1`, `LLRF:BPM:Vb1` |
| 6 | BPM相位 | 2-N | - | `LLRF:BPM:BPMPhase` |
| 7 | XY位置 | 0-3 | - | `LLRF:BPM:X1`, `LLRF:BPM:Y1` |
| 8 | 电压和 | 0-3 | - | `LLRF:BPM:Vsum1` |
| 9 | XY保护 | 0-3 | - | `LLRF:BPM:XYProtect` |
| 10-11 | BPM1差分电压 | - | - | `LLRF:BPM:DiffX1` |
| 12-13 | BPM2差分电压 | - | - | `LLRF:BPM:DiffX2` |
| 14-15 | BPM总电压 | - | - | `LLRF:BPM:SumTotal1` |
| 16-19 | 对向电压和 | - | - | `LLRF:BPM:SumAC1` |
| 20-23 | 归一化位置 | - | - | `LLRF:BPM:NormX1` |
| 24 | ABCD电压值(V) | 0-7 | - | `LLRF:BPM:Va1_V` |
| 25-28 | 差分电压(V) | - | - | `LLRF:BPM:DiffX1_V` |

## 🔄 数据流：从DB到驱动层

### 1. 数据库层配置

在`.db`文件中，通过`INP`字段的字符串指定offset：

```
# BPMMonitor.db

# RF3幅度（offset=0, channel=3, type=AMP）
record(ai, "LLRF:BPM:RF3Amp")
{
    field(DTYP, "BPM")
    field(INP,  "@0:3")      # 格式: @offset:channel
    field(SCAN, "I/O Intr")
}

# X1位置（offset=7, channel=0）
record(ai, "LLRF:BPM:X1")
{
    field(DTYP, "BPM")
    field(INP,  "@7:0")      # offset=7, channel=0
    field(SCAN, "I/O Intr")
}

# FPGA LED0状态（offset=2，无channel）
record(ai, "LLRF:BPM:LED0")
{
    field(DTYP, "BPM")
    field(INP,  "@2")        # 只有offset
    field(SCAN, "I/O Intr")
}
```

### 2. 设备支持层解析

`init_record_ai()` 解析INP字段：

```c
// devBPMMonitor.c

long init_record_ai(aiRecord *prec)
{
    struct instio *pinstio;
    recordpara_t *recordpara;

    // 1. 分配私有数据结构
    recordpara = (recordpara_t *)malloc(sizeof(recordpara_t));

    // 2. 获取INP字段字符串
    pinstio = (struct instio *)&(prec->inp.value);
    char *parm = pinstio->string;  // 例如: "0:3" 或 "7:0"

    // 3. 解析offset和channel
    char *pchar = parm;

    // 解析offset
    recordpara->offset = strtol(pchar, &pchar, 0);  // 例如: offset = 0

    // 如果有冒号，解析channel
    if (*pchar == ':') {
        pchar++;
        recordpara->channel = strtol(pchar, &pchar, 0);  // 例如: channel = 3
    } else {
        recordpara->channel = 0;  // 默认channel=0
    }

    // 4. 确定type（从Record类型推断）
    if (strcmp(prec->name, "RF3Amp") != -1) {
        recordpara->type = AMP;   // type = 3
    } else if (strcmp(prec->name, "RF3Phase") != -1) {
        recordpara->type = PHASE; // type = 4
    } else {
        recordpara->type = NONE;  // type = 0
    }

    // 5. 保存到Record的dpvt字段
    prec->dpvt = recordpara;

    return 0;
}
```

### 3. read函数调用驱动层

```c
// devBPMMonitor.c

long read_ai(aiRecord *prec)
{
    recordpara_t *priv = (recordpara_t *)prec->dpvt;
    float value;

    // 调用驱动层的ReadData函数
    // 传递offset, channel, type三个参数
    value = ReadData(priv->offset, priv->channel, priv->type);

    // 将读取到的值赋给Record
    prec->val = value;

    return 0;
}
```

### 4. 驱动层处理

```c
// driverWrapper.c

float ReadData(int offset, int channel, int type)
{
    switch (offset)
    {
        case 0:  // RF信息
            if (type == AMP)   // type == 3
                return GetRFInfo(channel, 0);  // 读取RF3的幅度
            else if (type == PHASE)  // type == 4
                return GetRFInfo(channel, 1);  // 读取RF3的相位
            break;

        case 7:  // XY位置
            return GetXYPosition(channel);  // channel=0 返回X1
            break;

        case 2:  // FPGA LED0
            return GetFPGA_LED0_RBK();
            break;

        // ... 其他case
    }

    return 0.0;
}
```

## 📊 完整数据流示例

以`LLRF:BPM:RF3Amp`为例：

```
1. 数据库配置（BPMMonitor.db）
   record(ai, "LLRF:BPM:RF3Amp") {
       field(INP, "@0:3")
   }

2. init_record_ai() 解析
   parm = "0:3"
   ↓ 解析
   offset = 0
   channel = 3
   type = AMP (3)
   ↓ 保存到 recordpara
   prec->dpvt = recordpara

3. 触发扫描（I/O中断）
   scanIoRequest() ──> 触发Record处理

4. read_ai() 读取数据
   recordpara = prec->dpvt
   ↓
   value = ReadData(0, 3, AMP)
             ↓        ↓  ↓  ↓
           offset   ch type

5. ReadData() 驱动层处理
   switch (offset) {
       case 0:  // RF信息
           if (type == AMP)
               return GetRFInfo(3, 0);  // 读取RF3幅度
                        ↓      ↓  ↓
                     channel RF类型(0=幅度)

6. GetRFInfo() 从全局数组读取
   return rf3amp[0];  // 返回RF3幅度的第一个点

7. 数据返回给Record
   prec->val = value;  // 例如: 3.14

8. 客户端读取
   caget LLRF:BPM:RF3Amp
   输出: 3.14
```

## 🆕 如何添加新的Offset类型

假设你要添加一个新的数据类型：**RF功率因数** (Power Factor)

### Step 1: 在驱动层添加case

```c
// driverWrapper.c

float ReadData(int offset, int channel, int type)
{
    switch (offset)
    {
        // ... 现有case

        case 29:  // 新增：RF功率因数
            return GetRFPowerFactor(channel);
            break;

        default:
            return 0.0;
    }
}

// 添加获取函数
static float GetRFPowerFactor(int channel)
{
    // 假设功率因数 = 有功功率 / 视在功率
    float amp = GetRFInfo(channel, 0);
    float power = GetRFInfo(channel, 2);

    if (amp > 0) {
        return power / (amp * amp);
    }
    return 0.0;
}
```

### Step 2: 在数据库中添加PV

```
# BPMMonitor.db

record(ai, "LLRF:BPM:RF3PowerFactor")
{
    field(DTYP, "BPM")
    field(INP,  "@29:3")     # offset=29, channel=3
    field(SCAN, "I/O Intr")
    field(PREC, "3")
    field(DESC, "RF3 Power Factor")
}
```

### Step 3: 重新编译和加载

```bash
# 编译
cd ~/BPMIOC
make

# 加载数据库
cd iocBoot/iocBPMmonitor
./st.cmd

# 测试
caget LLRF:BPM:RF3PowerFactor
```

✅ 完成！新的Offset类型添加成功。

## 💡 Offset系统的优缺点分析

### 优点

1. **统一接口**：
   - 所有数据访问都通过同一个函数
   - 代码结构清晰

2. **性能优秀**：
   - Switch语句编译为跳转表，O(1)时间复杂度
   - 无字符串比较开销

3. **易于扩展**：
   - 添加新类型只需添加新case
   - 不影响现有代码

4. **类型安全**：
   - 编译时检查
   - 避免字符串拼写错误

### 缺点

1. **可读性**：
   - Offset是数字，不如字符串直观
   - 需要查文档才知道`offset=7`代表什么

2. **维护成本**：
   - 需要维护Offset映射表
   - 容易产生Offset冲突

3. **调试困难**：
   - 调试时看到`ReadData(7, 0, 0)`，不知道读取什么
   - 需要对照文档

### 改进建议

```c
// 使用枚举增强可读性
typedef enum {
    OFFSET_RF_INFO = 0,
    OFFSET_DI = 1,
    OFFSET_FPGA_LED0 = 2,
    OFFSET_FPGA_LED1 = 3,
    OFFSET_HISTORY_READY = 4,
    OFFSET_ABCD_VOLTAGE = 5,
    OFFSET_BPM_PHASE = 6,
    OFFSET_XY_POSITION = 7,
    // ...
} OffsetType;

// 使用时更清晰
value = ReadData(OFFSET_XY_POSITION, 0, 0);  // 读取X1
```

## 📚 Offset系统与其他系统的对比

### EPICS StreamDevice

StreamDevice使用字符串协议：

```
# stream.proto
Terminator = "\n";
getTemp {
    out "GET:TEMP";
    in "%f";
}
```

❌ **缺点**：字符串解析慢，不适合高频数据

### EPICS asynDriver

asynDriver使用reason（整数）：

```c
asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int reason = pasynUser->reason;

    switch (reason) {
        case REASON_TEMP:
            *value = getTemperature();
            break;
        case REASON_PRESS:
            *value = getPressure();
            break;
    }
}
```

✅ **类似BPMIOC的Offset系统**！reason等价于offset

## ✅ 学习检查点

完成本文后，你应该能够回答：

1. **Offset系统基础**：
   - [ ] Offset系统解决了什么问题？
   - [ ] Offset、Channel、Type三个参数的作用？
   - [ ] 为什么不使用字符串标识数据类型？

2. **数据流**：
   - [ ] INP字段的`@0:3`是什么意思？
   - [ ] 设备支持层如何解析offset？
   - [ ] ReadData如何根据offset返回数据？

3. **实践应用**：
   - [ ] 如何添加新的Offset类型？
   - [ ] 如何查看某个PV使用的offset？
   - [ ] Offset=7代表什么数据？

4. **设计分析**：
   - [ ] Offset系统的优缺点？
   - [ ] 如何改进Offset系统的可读性？

## 🔗 相关文档

- **[01-architecture-overview.md](./01-architecture-overview.md)** - 理解三层架构
- **[02-data-flow.md](./02-data-flow.md)** - 理解数据流动
- **[04-memory-model.md](./04-memory-model.md)** - 内存模型
- **[Part 15: Offset参考表](/docs/part15-reference/tables/offset-table.md)** - 完整Offset列表

## 📚 扩展阅读

- [EPICS asynDriver Documentation](https://epics.anl.gov/modules/soft/asyn/)
- [Design Patterns: Strategy Pattern](https://refactoring.guru/design-patterns/strategy)

---

**下一篇**: [06-thread-model.md](./06-thread-model.md) - 线程模型和同步

**实践练习**:
1. 查找`LLRF:BPM:Y2`使用的offset、channel、type
2. 添加一个新的Offset类型：RF信噪比(SNR)
3. 绘制RF3Amp的完整数据流图（从.db到硬件）
