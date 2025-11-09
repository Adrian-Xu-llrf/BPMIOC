# SetReg()函数详解

> **阅读时间**: 35分钟
> **难度**: ⭐⭐⭐☆☆
> **前置知识**: ReadData()、寄存器概念、ao Record

## 📋 本文目标

- 理解SetReg()的作用和设计
- 掌握寄存器读写机制
- 学会如何添加新的寄存器
- 理解软硬件同步策略

## 🎯 SetReg()是什么？

### 定位

```
控制流: CA客户端 → ao Record → 设备支持层 → SetReg() → 硬件
                                                 ↓
                                            唯一写入接口
```

**SetReg()是整个IOC的写入接口**，所有配置参数的修改都通过它。

### 函数签名

```c
// driverWrapper.c line 1101-1150

long SetReg(int addr, int value)
{
    // addr: 寄存器地址 (0-99)
    // value: 要写入的值

    // 1. 参数验证
    if (addr < 0 || addr >= REG_NUM) {
        printf("ERROR: Invalid register address: %d\n", addr);
        return -1;
    }

    // 2. 更新本地缓存
    Reg[addr] = value;

    // 3. 写入硬件
    if (funcSetReg != NULL) {
        funcSetReg(addr, value);
    }

    printf("SetReg: Reg[%d] = %d\n", addr, value);

    return 0;  // 成功
}
```

### 与ReadData()的关系

```
ReadData()     vs     SetReg()
   ↓                     ↓
读取数据              写入数据
用于ai Record      用于ao Record
29种offset         1种接口(寄存器)
返回float           接受int
只读操作            写操作
```

## 1. 寄存器系统设计

### 1.1 全局寄存器数组

```c
// driverWrapper.c 全局变量区域

#define REG_NUM 100       // 寄存器数量

static int Reg[REG_NUM];  // 软件缓存
```

**设计考虑**:
- 100个寄存器足够存储所有配置参数
- 软件缓存提高读取速度
- 与硬件寄存器保持同步

### 1.2 寄存器地址分配

```
Reg[0-9]:    系统配置
├─ Reg[0]:   系统状态 (0=停止, 1=运行)
├─ Reg[1]:   采样率设置
├─ Reg[2]:   触发模式
└─ Reg[3-9]: 预留

Reg[10-19]:  RF通道配置
├─ Reg[10]:  RF3增益
├─ Reg[11]:  RF4增益
├─ Reg[12]:  RF5增益
├─ Reg[13]:  RF6增益
└─ Reg[14-19]: 预留

Reg[20-29]:  XY位置配置
├─ Reg[20]:  X偏移校准
├─ Reg[21]:  Y偏移校准
├─ Reg[22]:  X比例因子
└─ Reg[23-29]: 预留

Reg[30-39]:  Button配置
├─ Reg[30]:  Button1增益
├─ Reg[31]:  Button2增益
└─ Reg[32-39]: 预留

Reg[40-49]:  触发和门控
├─ Reg[40]:  触发延迟
├─ Reg[41]:  门控开始
├─ Reg[42]:  门控结束
└─ Reg[43-49]: 预留

Reg[50-99]:  预留扩展
```

**设计原则**:
- 按功能分组
- 每组预留空间
- 便于扩展

### 1.3 软硬件同步策略

```
┌─────────────┐         ┌─────────────┐
│  Reg[] 数组  │ ←─同步─→ │ 硬件寄存器  │
│  (软件缓存)  │         │  (FPGA)     │
└─────────────┘         └─────────────┘
      ↑                        ↑
      │                        │
  SetReg()                funcSetReg()
  ReadData()              funcGetReg()
```

**同步规则**:
1. **写入**: SetReg() → 更新Reg[] → 调用funcSetReg() → 写入硬件
2. **读取**: ReadData() → 直接返回Reg[] (不读硬件)
3. **初始化**: InitDevice() → funcGetReg() → 同步Reg[]

## 2. SetReg()详细实现

### 2.1 完整代码分析

```c
long SetReg(int addr, int value)
{
    // ===== Step 1: 参数验证 =====
    if (addr < 0 || addr >= REG_NUM) {
        printf("ERROR: Invalid register address: %d (range: 0-%d)\n",
               addr, REG_NUM - 1);
        return -1;
    }

    // ===== Step 2: 值的范围检查 (可选) =====
    // 例如某些寄存器只能是0或1
    if (addr == 0) {  // 系统状态
        if (value != 0 && value != 1) {
            printf("ERROR: Reg[0] must be 0 or 1, got %d\n", value);
            return -1;
        }
    }

    // ===== Step 3: 更新软件缓存 =====
    int old_value = Reg[addr];
    Reg[addr] = value;

    printf("SetReg: Reg[%d] = %d (was %d)\n", addr, value, old_value);

    // ===== Step 4: 写入硬件 =====
    if (funcSetReg != NULL) {
        int ret = funcSetReg(addr, value);
        if (ret != 0) {
            printf("ERROR: Hardware SetReg failed: %d\n", ret);
            // 恢复旧值
            Reg[addr] = old_value;
            return -1;
        }
    } else {
        printf("WARNING: funcSetReg is NULL, software-only mode\n");
    }

    // ===== Step 5: 触发相关操作 (可选) =====
    // 例如写入Reg[0]时，需要重启系统
    if (addr == 0) {
        if (value == 1) {
            printf("Starting system...\n");
            // 调用其他初始化函数
        } else {
            printf("Stopping system...\n");
        }
    }

    return 0;  // 成功
}
```

### 2.2 错误处理策略

#### 地址越界

```c
if (addr < 0 || addr >= REG_NUM) {
    printf("ERROR: Invalid register address: %d\n", addr);
    return -1;  // EPICS会标记Record为INVALID_ALARM
}
```

#### 硬件写入失败

```c
int ret = funcSetReg(addr, value);
if (ret != 0) {
    // 恢复旧值
    Reg[addr] = old_value;
    printf("ERROR: Hardware write failed, rolled back\n");
    return -1;
}
```

#### 值范围检查

```c
// 示例：Reg[1]采样率必须是1-1000
if (addr == 1) {
    if (value < 1 || value > 1000) {
        printf("ERROR: Invalid sampling rate: %d (range: 1-1000)\n", value);
        return -1;
    }
}
```

## 3. 硬件函数实现

### 3.1 funcSetReg声明和加载

```c
// driverWrapper.c 全局变量区域

static int (*funcSetReg)(int addr, int value);
static int (*funcGetReg)(int addr);
```

```c
// InitDevice()中加载
funcSetReg = (int (*)(int, int))dlsym(handle, "SetReg");
if (funcSetReg == NULL) {
    fprintf(stderr, "WARNING: Cannot find symbol SetReg: %s\n", dlerror());
    // 继续运行，但无法写硬件
}

funcGetReg = (int (*)(int))dlsym(handle, "GetReg");
if (funcGetReg == NULL) {
    fprintf(stderr, "WARNING: Cannot find symbol GetReg: %s\n", dlerror());
}
```

### 3.2 Mock库实现

```c
// libbpm_mock.c

static int mock_registers[100];  // 模拟硬件寄存器

int SetReg(int addr, int value)
{
    if (addr < 0 || addr >= 100) {
        return -1;
    }

    printf("[Mock] SetReg(%d, %d)\n", addr, value);

    // 模拟写入延迟
    usleep(100);  // 100μs

    mock_registers[addr] = value;

    // 模拟特殊寄存器的副作用
    if (addr == 0 && value == 1) {
        printf("[Mock] System started\n");
    }

    return 0;  // 成功
}

int GetReg(int addr)
{
    if (addr < 0 || addr >= 100) {
        return 0;
    }

    return mock_registers[addr];
}
```

### 3.3 Real库实现 (ZYNQ)

```c
// libbpm_zynq.c

#include "xil_io.h"

#define FPGA_REG_BASE 0x43C00000

int SetReg(int addr, int value)
{
    if (addr < 0 || addr >= 100) {
        return -1;
    }

    // 计算寄存器地址
    uint32_t reg_addr = FPGA_REG_BASE + (addr * 4);

    // 写入FPGA寄存器
    Xil_Out32(reg_addr, (uint32_t)value);

    // 等待写入完成
    usleep(10);  // 10μs

    // 读回验证
    uint32_t readback = Xil_In32(reg_addr);
    if (readback != (uint32_t)value) {
        printf("ERROR: Write verification failed: wrote %d, read %d\n",
               value, readback);
        return -1;
    }

    return 0;  // 成功
}

int GetReg(int addr)
{
    if (addr < 0 || addr >= 100) {
        return 0;
    }

    uint32_t reg_addr = FPGA_REG_BASE + (addr * 4);
    uint32_t value = Xil_In32(reg_addr);

    return (int)value;
}
```

## 4. 调用链详解

### 4.1 完整写入流程

```
1. CA客户端
   caput LLRF:BPM:SetGain 50
         ↓

2. CA网络
   请求到达IOC
         ↓

3. EPICS Record
   aoRecord处理
   precord->rset->process(precord)
         ↓

4. 设备支持层
   devBPMMonitor.c: write_ao()
   DevPvt *pPvt = (DevPvt *)precord->dpvt;
         ↓

5. 驱动层
   SetReg(pPvt->offset, (int)precord->val)
   SetReg(10, 50)  // 假设Reg[10]是增益
         ↓

6. 更新缓存
   Reg[10] = 50
         ↓

7. 硬件函数
   funcSetReg(10, 50)
         ↓

8. 硬件库
   Mock: mock_registers[10] = 50
   Real: Xil_Out32(0x43C00028, 50)
         ↓

9. 返回
   成功或失败
```

### 4.2 代码示例

```c
// 设备支持层 (devBPMMonitor.c)
static long write_ao(aoRecord *precord)
{
    DevPvt *pPvt = (DevPvt *)precord->dpvt;

    // 将float转为int
    int value = (int)precord->val;

    // 调用SetReg (offset作为地址)
    long status = SetReg(pPvt->offset, value);

    if (status != 0) {
        // 写入失败
        recGblSetSevr(precord, WRITE_ALARM, INVALID_ALARM);
        return -1;
    }

    return 0;
}
```

## 5. 读取寄存器

### 5.1 通过ReadData读取

```c
// driverWrapper.c ReadData()

float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        // ... 其他case ...

        case 28:  // OFFSET_REG
            // channel作为寄存器地址
            if (channel < 0 || channel >= REG_NUM) {
                printf("ERROR: Invalid register address: %d\n", channel);
                return 0.0;
            }

            // 直接返回软件缓存
            return (float)Reg[channel];

        default:
            return 0.0;
    }
}
```

**关键点**:
- **不读硬件**: 直接返回Reg[]缓存
- **channel重用**: channel参数作为寄存器地址
- **offset=28**: 专门用于寄存器读取

### 5.2 在数据库中使用

```
# BPMMonitor.db

# 读取Reg[10] (增益)
record(ai, "LLRF:BPM:GetGain") {
    field(DESC, "Read Gain")
    field(DTYP, "BPMMonitor")
    field(INP,  "@28 10")     # offset=28(REG), channel=10
    field(SCAN, "1 second")
}

# 写入Reg[10]
record(ao, "LLRF:BPM:SetGain") {
    field(DESC, "Set Gain")
    field(DTYP, "BPMMonitor")
    field(OUT,  "@10")        # offset=10 (寄存器地址)
    field(DRVL, "0")          # 最小值
    field(DRVH, "100")        # 最大值
}
```

**注意**:
- ai Record: `INP="@28 10"` (offset=28, channel=10)
- ao Record: `OUT="@10"` (offset=10，直接是地址)

## 6. 初始化时同步

### 6.1 从硬件读取寄存器

```c
// InitDevice()中

long InitDevice()
{
    // ... dlopen/dlsym ...

    // 初始化硬件
    funcSystemInit();

    // 从硬件读取所有寄存器
    if (funcGetReg != NULL) {
        printf("Synchronizing registers from hardware...\n");

        for (int addr = 0; addr < REG_NUM; addr++) {
            Reg[addr] = funcGetReg(addr);
            printf("  Reg[%d] = %d\n", addr, Reg[addr]);
        }

        printf("Register synchronization complete\n");
    } else {
        printf("WARNING: funcGetReg is NULL, using default values\n");

        // 使用默认值
        memset(Reg, 0, sizeof(Reg));
        Reg[0] = 1;   // 系统运行
        Reg[1] = 100; // 采样率100kHz
        // ...
    }

    // ... 创建线程 ...

    return 0;
}
```

### 6.2 初始化默认值

```c
void initDefaultRegisters(void)
{
    // 系统配置
    Reg[0] = 1;      // 系统运行
    Reg[1] = 100;    // 采样率100kHz
    Reg[2] = 0;      // 触发模式: 软件

    // RF增益
    Reg[10] = 50;    // RF3增益
    Reg[11] = 50;    // RF4增益
    Reg[12] = 50;    // RF5增益
    Reg[13] = 50;    // RF6增益

    // 位置校准
    Reg[20] = 0;     // X偏移
    Reg[21] = 0;     // Y偏移

    // 触发和门控
    Reg[40] = 100;   // 触发延迟100ns
    Reg[41] = 0;     // 门控开始
    Reg[42] = 10000; // 门控结束

    printf("Default registers initialized\n");
}
```

## 7. 添加新寄存器

### 示例：添加Reg[50]作为温度阈值

#### Step 1: 分配地址

```c
// 在文档或注释中记录
// Reg[50]: 温度阈值 (单位: 0.1℃)
```

#### Step 2: 设置默认值

```c
void initDefaultRegisters(void)
{
    // ... 其他寄存器 ...

    Reg[50] = 300;  // 30.0℃
}
```

#### Step 3: 在数据库中创建Record

```
# BPMMonitor.db

record(ao, "LLRF:BPM:SetTempThreshold") {
    field(DESC, "Temperature Threshold")
    field(DTYP, "BPMMonitor")
    field(OUT,  "@50")        # Reg[50]
    field(EGU,  "0.1degC")
    field(DRVL, "0")          # 最小0℃
    field(DRVH, "1000")       # 最大100℃
    field(PREC, "1")
}

record(ai, "LLRF:BPM:GetTempThreshold") {
    field(DESC, "Read Temperature Threshold")
    field(DTYP, "BPMMonitor")
    field(INP,  "@28 50")     # offset=28, channel=50
    field(SCAN, "1 second")
    field(EGU,  "0.1degC")
    field(PREC, "1")
}
```

#### Step 4: 在硬件库中处理

```c
// libbpm_mock.c

int SetReg(int addr, int value)
{
    // ... 通用处理 ...

    mock_registers[addr] = value;

    // Reg[50]特殊处理
    if (addr == 50) {
        printf("[Mock] Temperature threshold set to %.1f degC\n",
               value * 0.1);
    }

    return 0;
}
```

#### Step 5: 在逻辑中使用

```c
// 在采集线程或其他地方使用阈值
void checkTemperature(void)
{
    float current_temp = funcGetTemperature(0);
    float threshold = Reg[50] * 0.1;  // 转换为℃

    if (current_temp > threshold) {
        printf("WARNING: Temperature %.1f > threshold %.1f\n",
               current_temp, threshold);
    }
}
```

## 8. 调试技巧

### 8.1 打印所有寄存器

```c
void dumpAllRegisters(void)
{
    printf("=== Register Dump ===\n");
    for (int addr = 0; addr < REG_NUM; addr++) {
        if (Reg[addr] != 0) {  // 只打印非零值
            printf("Reg[%2d] = %d\n", addr, Reg[addr]);
        }
    }
    printf("====================\n");
}

// 在iocsh中调用
// iocsh> dumpAllRegisters
```

### 8.2 监控寄存器修改

```c
long SetReg(int addr, int value)
{
    // ... 参数验证 ...

    int old_value = Reg[addr];

    if (old_value != value) {
        printf("SetReg[%d]: %d → %d (change: %+d)\n",
               addr, old_value, value, value - old_value);
    }

    Reg[addr] = value;

    // ... 写入硬件 ...

    return 0;
}
```

### 8.3 记录寄存器历史

```c
#define REG_HISTORY_LEN 10

typedef struct {
    time_t timestamp;
    int value;
} RegHistory;

static RegHistory reg_history[REG_NUM][REG_HISTORY_LEN];
static int history_index[REG_NUM] = {0};

long SetReg(int addr, int value)
{
    // ... 常规处理 ...

    // 记录历史
    int idx = history_index[addr];
    reg_history[addr][idx].timestamp = time(NULL);
    reg_history[addr][idx].value = value;

    history_index[addr] = (idx + 1) % REG_HISTORY_LEN;

    return 0;
}

// 查看历史
void showRegHistory(int addr)
{
    printf("=== Reg[%d] History ===\n", addr);
    for (int i = 0; i < REG_HISTORY_LEN; i++) {
        if (reg_history[addr][i].timestamp > 0) {
            printf("%s: %d\n",
                   ctime(&reg_history[addr][i].timestamp),
                   reg_history[addr][i].value);
        }
    }
}
```

## ❓ 常见问题

### Q1: 为什么SetReg接受int而不是float？
**A**:
- 寄存器通常是整数（硬件限制）
- 如需浮点数，可以用定点表示（如0.1℃ → 1单位）
- ao Record的VAL是double，会自动转换

### Q2: 如何实现读-修改-写？
**A**:
```c
// 设置Reg[0]的bit 2
int value = Reg[0];        // 读
value |= (1 << 2);         // 修改
SetReg(0, value);          // 写
```

### Q3: SetReg是线程安全的吗？
**A**:
- **当前实现不是**
- 如需线程安全，加锁:
```c
static epicsMutexId regLock;

long SetReg(int addr, int value)
{
    epicsMutexLock(regLock);
    // ... 常规处理 ...
    epicsMutexUnlock(regLock);
}
```

### Q4: 能否批量写入多个寄存器？
**A**:
```c
long SetRegBulk(int start_addr, const int *values, int count)
{
    for (int i = 0; i < count; i++) {
        if (SetReg(start_addr + i, values[i]) != 0) {
            return -1;
        }
    }
    return 0;
}
```

## 📚 延伸阅读

- [07-readdata.md](./07-readdata.md) - ReadData函数详解
- [Part 5: 03-write-ao.md](../part5-device-support-layer/03-write-ao.md) - ao Record写入
- [10-hardware-functions.md](./10-hardware-functions.md) - 硬件函数详解

## 🎓 本章总结

- ✅ SetReg()是唯一的写入接口
- ✅ 100个寄存器用于存储配置
- ✅ 软件缓存Reg[]与硬件同步
- ✅ 支持范围检查和错误恢复
- ✅ 添加新寄存器只需分配地址

**核心设计**: 软件缓存 + 硬件同步 = 高效访问

**下一步**: 阅读 [09-readwaveform.md](./09-readwaveform.md) 学习波形读取

---

**实验任务**: 添加Reg[60]作为调试级别，0=关闭，1=基本，2=详细
