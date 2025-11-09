# ReadData()函数详解

> **阅读时间**: 60分钟
> **难度**: ⭐⭐⭐⭐⭐
> **前置知识**: Offset系统、硬件函数、C语言switch-case

## 📋 本文目标

- 理解ReadData()的核心作用
- 掌握29个offset的处理逻辑
- 学会如何添加新的数据类型
- 理解offset、channel、type三个参数

## 🎯 ReadData()是什么？

### 定位

```
数据流: 硬件 → ReadData() → 设备支持层 → Record → CA
                   ↑
              核心路由函数
```

**ReadData()是整个IOC的数据读取入口**，所有标量数据的读取都通过它。

### 函数签名

```c
// driverWrapper.c line 601-1100

float ReadData(int offset, int channel, int type)
{
    // offset: 数据类型 (0-29)
    // channel: 通道号 (0-7)
    // type: 子类型 (AMP/PHASE/REAL/IMAG)

    switch (offset) {
        case 0:  // RF信息
        case 1:  // 中心频率
        // ... 共29个case
        default:
            return 0.0;
    }
}
```

### 与ReadWaveform()的区别

```
ReadData()        vs    ReadWaveform()
   ↓                         ↓
返回单个float值          返回float数组
用于ai/ao Record      用于waveform Record
每次读取1个值          每次读取10000个值
```

## 1. 三个参数详解

### 1.1 offset参数 (数据类型)

```c
// driverWrapper.c 宏定义区域
#define OFFSET_RF         0   // RF信息 (8通道)
#define OFFSET_CENTER_F   1   // 中心频率
#define OFFSET_X_OFFSET   2   // X偏移校准
#define OFFSET_Y_OFFSET   3   // Y偏移校准
#define OFFSET_Q_OFFSET   4   // Q偏移校准
#define OFFSET_BUTTON_SUM 5   // Button总和
#define OFFSET_ATT        6   // 衰减值
#define OFFSET_XY         7   // XY位置
#define OFFSET_Q          8   // 电荷Q
#define OFFSET_PHI        9   // 相位Phi
#define OFFSET_K          10  // 增益K
#define OFFSET_PHASE      11  // 相位
#define OFFSET_CORDIC_G   12  // CORDIC增益
#define OFFSET_TRIG_DELAY 13  // 触发延迟
#define OFFSET_BUTTON_I   14  // Button I分量
#define OFFSET_BUTTON_Q   15  // Button Q分量
#define OFFSET_BUTTON     16  // Button信号
#define OFFSET_TURN_CHAR  17  // 匝数特征
#define OFFSET_TURN_ID    18  // 匝数ID
#define OFFSET_PEAK_X     19  // X峰值
#define OFFSET_PEAK_Y     20  // Y峰值
#define OFFSET_PEAK_Q     21  // Q峰值
#define OFFSET_TRIP_X     22  // X历史 (trip)
#define OFFSET_TRIP_Y     23  // Y历史
#define OFFSET_TRIP_Q     24  // Q历史
#define OFFSET_GATE_START 25  // 门控开始
#define OFFSET_GATE_STOP  26  // 门控结束
#define OFFSET_TBT_DELAY  27  // Turn-by-turn延迟
#define OFFSET_REG        28  // 寄存器
// 共29个offset (0-28)
```

**offset是如何决定的？**
- 在设备支持层(devBPMMonitor.c)中，每个Record的INP/OUT字段包含offset
- 例如: `field(INP, "@0 3")` → offset=0, channel=3

### 1.2 channel参数 (通道号)

```
BPM硬件通道:
├─ RF通道: 0-7 (共8个)
│  ├─ 3, 4, 5, 6: 实际使用
│  └─ 0, 1, 2, 7: 预留
│
├─ XY通道: 0-3 (共4个)
│  ├─ 0: X1 (BPM 1的X)
│  ├─ 1: Y1 (BPM 1的Y)
│  ├─ 2: X2 (BPM 2的X)
│  └─ 3: Y2 (BPM 2的Y)
│
└─ Button通道: 0-7 (共8个)
```

**示例**:
```c
// 读取RF3的幅度
ReadData(0, 3, AMP);  // offset=0(RF), channel=3, type=AMP

// 读取X1位置
ReadData(7, 0, 0);    // offset=7(XY), channel=0(X1), type=0
```

### 1.3 type参数 (子类型)

```c
#define AMP     0    // 幅度
#define PHASE   1    // 相位
#define REAL    2    // 实部
#define IMAG    3    // 虚部
```

**用途**:
- 某些数据有多个子类型
- 例如RF信息：幅度、相位、实部、虚部
- 通过type参数选择返回哪个

## 2. 核心switch-case结构

### 2.1 整体结构

```c
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        case 0:   // RF信息 (最复杂)
            // ... 100行代码 ...
            break;

        case 1:   // 中心频率
            // ... 10行代码 ...
            break;

        // ... case 2-27 ...

        case 28:  // 寄存器
            // ... 10行代码 ...
            break;

        default:
            fprintf(stderr, "Unknown offset: %d\n", offset);
            return 0.0;
    }
}
```

**代码行数分布**:
- case 0 (RF信息): ~100行
- case 1-27: 每个10-30行
- 总计: ~500行

### 2.2 典型case分析

#### Case 0: RF信息 (复杂)

```c
case 0:  // OFFSET_RF
    // 通道范围检查
    if (channel < 3 || channel > 6) {
        fprintf(stderr, "Invalid RF channel: %d\n", channel);
        return 0.0;
    }

    // 根据type返回不同数据
    if (type == AMP) {
        // 幅度
        return funcGetRFInfo(channel, 0);
    }
    else if (type == PHASE) {
        // 相位
        return funcGetRFInfo(channel, 1);
    }
    else if (type == REAL) {
        // 实部
        float amp = funcGetRFInfo(channel, 0);
        float phase = funcGetRFInfo(channel, 1);
        return amp * cos(phase * M_PI / 180.0);
    }
    else if (type == IMAG) {
        // 虚部
        float amp = funcGetRFInfo(channel, 0);
        float phase = funcGetRFInfo(channel, 1);
        return amp * sin(phase * M_PI / 180.0);
    }
    else {
        fprintf(stderr, "Invalid type for RF: %d\n", type);
        return 0.0;
    }
    break;
```

**关键点**:
1. **参数验证**: 检查channel范围
2. **多种type**: AMP, PHASE, REAL, IMAG
3. **实时计算**: REAL和IMAG是计算得出的
4. **调用硬件函数**: funcGetRFInfo()

#### Case 1: 中心频率 (简单)

```c
case 1:  // OFFSET_CENTER_F
    // 中心频率 (与通道无关)
    if (funcGetCenterFrequency != NULL) {
        return funcGetCenterFrequency();
    } else {
        return 0.0;
    }
    break;
```

**关键点**:
1. **简单调用**: 直接调用硬件函数
2. **空指针检查**: 防止funcGetCenterFrequency为NULL
3. **忽略channel和type**: 中心频率全局唯一

#### Case 7: XY位置 (中等)

```c
case 7:  // OFFSET_XY
    // channel: 0=X1, 1=Y1, 2=X2, 3=Y2
    if (channel < 0 || channel > 3) {
        fprintf(stderr, "Invalid XY channel: %d\n", channel);
        return 0.0;
    }

    if (funcGetXYPosition != NULL) {
        return funcGetXYPosition(channel);
    } else {
        return 0.0;
    }
    break;
```

#### Case 16: Button信号 (数组访问)

```c
case 16:  // OFFSET_BUTTON
    // channel: 0-7 (8个button)
    if (channel < 0 || channel > 7) {
        fprintf(stderr, "Invalid button channel: %d\n", channel);
        return 0.0;
    }

    if (funcGetButtonSignal != NULL) {
        return funcGetButtonSignal(channel);
    } else {
        return 0.0;
    }
    break;
```

#### Case 28: 寄存器读取

```c
case 28:  // OFFSET_REG
    // channel作为寄存器地址
    if (channel < 0 || channel >= REG_NUM) {
        fprintf(stderr, "Invalid register address: %d\n", channel);
        return 0.0;
    }

    // 直接返回全局寄存器数组
    return (float)Reg[channel];
```

**关键点**:
- channel被重用为寄存器地址
- 直接访问全局数组Reg[]
- 不调用硬件函数（Reg[]已经和硬件同步）

## 3. 完整offset列表和用法

### 3.1 RF相关 (offset 0, 11, 12)

```c
// offset=0: RF信息
ReadData(0, 3, AMP);    // RF3幅度
ReadData(0, 3, PHASE);  // RF3相位
ReadData(0, 4, AMP);    // RF4幅度
ReadData(0, 5, AMP);    // RF5幅度
ReadData(0, 6, AMP);    // RF6幅度

// offset=11: 相位 (专用)
ReadData(11, channel, 0);

// offset=12: CORDIC增益
ReadData(12, 0, 0);
```

### 3.2 位置相关 (offset 7, 19, 20, 22, 23)

```c
// offset=7: 实时XY位置
ReadData(7, 0, 0);  // X1
ReadData(7, 1, 0);  // Y1
ReadData(7, 2, 0);  // X2
ReadData(7, 3, 0);  // Y2

// offset=19: X峰值
ReadData(19, 0, 0);  // X1峰值

// offset=20: Y峰值
ReadData(20, 1, 0);  // Y1峰值

// offset=22: X历史 (trip)
ReadData(22, 0, 0);  // 通常用ReadWaveform

// offset=23: Y历史
ReadData(23, 1, 0);
```

### 3.3 Button相关 (offset 14, 15, 16)

```c
// offset=14: Button I分量
ReadData(14, 0, 0);  // Button1 I分量
ReadData(14, 1, 0);  // Button2 I分量

// offset=15: Button Q分量
ReadData(15, 0, 0);  // Button1 Q分量

// offset=16: Button信号 (原始)
ReadData(16, 0, 0);  // Button1
ReadData(16, 1, 0);  // Button2
// ... Button3-8
```

### 3.4 配置相关 (offset 2-6, 10, 13, 25-28)

```c
// offset=2: X偏移校准
ReadData(2, 0, 0);

// offset=3: Y偏移校准
ReadData(3, 0, 0);

// offset=6: 衰减值
ReadData(6, channel, 0);

// offset=10: 增益K
ReadData(10, 0, 0);

// offset=13: 触发延迟
ReadData(13, 0, 0);

// offset=25: 门控开始
ReadData(25, 0, 0);

// offset=26: 门控结束
ReadData(26, 0, 0);

// offset=28: 寄存器
ReadData(28, 0, 0);  // Reg[0]
ReadData(28, 1, 0);  // Reg[1]
```

## 4. 调用链详解

### 4.1 完整数据流

```
1. CA客户端
   caget LLRF:BPM:RF3Amp
         ↓

2. CA网络
   请求到达IOC
         ↓

3. EPICS Record
   aiRecord处理
   precord->rset->process(precord)
         ↓

4. 设备支持层
   devBPMMonitor.c: read_ai()
   DevPvt *pPvt = (DevPvt *)precord->dpvt;
         ↓

5. 驱动层
   ReadData(pPvt->offset, pPvt->channel, pPvt->type)
   ReadData(0, 3, AMP)
         ↓

6. 硬件函数
   funcGetRFInfo(3, 0)
         ↓

7. 硬件库
   Mock: 返回模拟数据 100.0 + rand()
   Real: Xil_In32(REG_RF3_AMP)
         ↓

8. 返回值
   ReadData() 返回 float
         ↓

9. Record更新
   precord->val = value;
         ↓

10. CA网络
    发送给客户端
```

### 4.2 代码示例

```c
// 设备支持层调用 (devBPMMonitor.c)
static long read_ai(aiRecord *precord)
{
    DevPvt *pPvt = (DevPvt *)precord->dpvt;

    // 调用ReadData
    float value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 更新Record
    precord->val = value;

    return 0;
}
```

```c
// 驱动层实现 (driverWrapper.c)
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        case 0:  // RF信息
            if (type == AMP) {
                // 调用硬件函数
                return funcGetRFInfo(channel, 0);
            }
            // ...
    }
}
```

## 5. 如何添加新的offset

### 示例：添加温度读取 (offset=29)

#### Step 1: 定义offset

```c
// driverWrapper.c 宏定义区域
#define OFFSET_TEMPERATURE 29  // 温度
```

#### Step 2: 在ReadData()中添加case

```c
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        // ... 现有case 0-28 ...

        case 29:  // OFFSET_TEMPERATURE
            // channel: 0-3 (4个温度传感器)
            if (channel < 0 || channel > 3) {
                fprintf(stderr, "Invalid temperature channel: %d\n", channel);
                return 0.0;
            }

            if (funcGetTemperature != NULL) {
                return funcGetTemperature(channel);
            } else {
                return 25.0;  // 默认25℃
            }
            break;

        default:
            return 0.0;
    }
}
```

#### Step 3: 添加硬件函数

```c
// 声明函数指针
static float (*funcGetTemperature)(int channel);

// 在InitDevice()中加载
funcGetTemperature = (float (*)(int))dlsym(handle, "GetTemperature");
```

#### Step 4: 实现硬件函数

```c
// libbpm_mock.c
float GetTemperature(int channel)
{
    return 25.0 + channel * 0.5 + (rand() % 10) * 0.1;
}

// libbpm_zynq.c
float GetTemperature(int channel)
{
    uint32_t reg = Xil_In32(REG_TEMP_BASE + channel * 4);
    return (float)reg * 0.01;  // 转换为℃
}
```

#### Step 5: 在数据库中使用

```
# BPMMonitor.db
record(ai, "LLRF:BPM:Temp1") {
    field(DESC, "Temperature 1")
    field(DTYP, "BPMMonitor")
    field(INP,  "@29 0")    # offset=29, channel=0
    field(SCAN, "I/O Intr")
    field(EGU,  "degC")
    field(PREC, "2")
}
```

## 6. 错误处理和调试

### 6.1 参数验证

```c
float ReadData(int offset, int channel, int type)
{
    // 1. offset范围检查
    if (offset < 0 || offset > 28) {
        fprintf(stderr, "ERROR: Invalid offset: %d\n", offset);
        return 0.0;
    }

    switch (offset) {
        case 0:  // RF
            // 2. channel范围检查
            if (channel < 3 || channel > 6) {
                fprintf(stderr, "ERROR: Invalid RF channel: %d (must be 3-6)\n", channel);
                return 0.0;
            }

            // 3. type范围检查
            if (type != AMP && type != PHASE && type != REAL && type != IMAG) {
                fprintf(stderr, "ERROR: Invalid RF type: %d\n", type);
                return 0.0;
            }

            // 4. 函数指针检查
            if (funcGetRFInfo == NULL) {
                fprintf(stderr, "ERROR: funcGetRFInfo is NULL\n");
                return 0.0;
            }

            // 正常处理
            return funcGetRFInfo(channel, type);
    }
}
```

### 6.2 添加调试输出

```c
#define DEBUG_READDATA 1  // 调试开关

float ReadData(int offset, int channel, int type)
{
    float value;

    #if DEBUG_READDATA
        printf("ReadData(offset=%d, channel=%d, type=%d)\n", offset, channel, type);
    #endif

    switch (offset) {
        case 0:
            value = funcGetRFInfo(channel, type);
            break;
        // ...
    }

    #if DEBUG_READDATA
        printf("  → value=%.3f\n", value);
    #endif

    return value;
}
```

**输出示例**:
```
ReadData(offset=0, channel=3, type=0)
  → value=105.234
ReadData(offset=7, channel=0, type=0)
  → value=2.567
```

### 6.3 使用gdb调试

```bash
# 在gdb中设置断点
gdb ./st.cmd
(gdb) break ReadData
(gdb) run
(gdb) continue

# 当断点触发
Breakpoint 1, ReadData (offset=0, channel=3, type=0)

# 查看参数
(gdb) print offset
$1 = 0
(gdb) print channel
$2 = 3
(gdb) print type
$3 = 0

# 单步执行
(gdb) step
(gdb) print funcGetRFInfo(3, 0)
$4 = 105.234
```

## 7. 性能考虑

### 7.1 调用频率

```
每个ai Record: 每次I/O Intr调用1次read_ai() → 1次ReadData()
假设50个ai Record: 每100ms调用50次ReadData()
频率: 50 / 0.1s = 500 Hz
```

**性能要求**: ReadData()必须快速（< 1ms）

### 7.2 优化技巧

#### 使用查找表

```c
// ❌ 慢速方式：每次计算sin/cos
case 0:
    if (type == REAL) {
        float amp = funcGetRFInfo(channel, 0);
        float phase = funcGetRFInfo(channel, 1);
        return amp * cos(phase * M_PI / 180.0);  // 慢
    }

// ✅ 快速方式：预计算
static float cos_table[360];  // 初始化时计算

void initCosTa ble(void) {
    for (int i = 0; i < 360; i++) {
        cos_table[i] = cos(i * M_PI / 180.0);
    }
}

case 0:
    if (type == REAL) {
        float amp = funcGetRFInfo(channel, 0);
        float phase = funcGetRFInfo(channel, 1);
        int phase_int = (int)(phase + 180.0);  // -180~180 → 0~360
        return amp * cos_table[phase_int];  // 快
    }
```

#### 缓存结果

```c
// 如果硬件读取很慢，可以缓存
static float cached_center_freq = 0.0;
static time_t cache_time = 0;

case 1:  // OFFSET_CENTER_F
    time_t now = time(NULL);
    if (now - cache_time > 1) {  // 缓存1秒
        cached_center_freq = funcGetCenterFrequency();
        cache_time = now;
    }
    return cached_center_freq;
```

## ❓ 常见问题

### Q1: 为什么返回值是float而不是double？
**A**:
- EPICS的ai Record内部使用double
- 但硬件精度通常只有float
- 返回float足够，Record会自动转换

### Q2: 可以在ReadData中修改硬件吗？
**A**:
- **不建议**，ReadData应该是只读的
- 修改硬件应该用SetReg()或ao Record

### Q3: ReadData会被多线程调用吗？
**A**:
- 是的，多个Record可能并发调用
- 但EPICS的dbScanLock保护每个Record
- 只要不修改全局状态，就是安全的

### Q4: 如何知道哪个PV对应哪个offset？
**A**:
```bash
# 查看数据库文件
grep "INP.*@" ~/BPMIOC/BPMmonitorApp/Db/BPMMonitor.db

# 输出示例:
# field(INP, "@0 3")   # offset=0, channel=3
# field(INP, "@7 0")   # offset=7, channel=0
```

## 📚 延伸阅读

- [Part 3: 05-offset-system.md](../part3-bpmioc-architecture/05-offset-system.md) - Offset系统详解
- [Part 5: 01-devbpmmonitor.md](../part5-device-support-layer/01-devbpmmonitor.md) - 设备支持层调用ReadData
- [09-readwaveform.md](./09-readwaveform.md) - ReadWaveform函数详解

## 🎓 本章总结

- ✅ ReadData()是标量数据读取的核心函数
- ✅ 29个offset处理不同类型的数据
- ✅ 通过offset、channel、type三个参数路由
- ✅ 每个case调用对应的硬件函数
- ✅ 添加新数据类型需要4个步骤

**核心思想**: 统一接口 + switch路由 = 灵活扩展

**下一步**: 阅读 [08-setreg.md](./08-setreg.md) 学习SetReg()函数

---

**实验任务**: 添加offset=29读取温度，并创建对应的ai Record
