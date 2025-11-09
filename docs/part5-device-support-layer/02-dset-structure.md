# Part 5.2: DSET结构详解

> **目标**: 深入理解DSET（设备支持表）的设计和使用
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 60分钟

## 📖 DSET深入理解

### DSET = Device Support Entry Table

DSET是EPICS设备支持的**核心接口**，它定义了：
- Record如何初始化
- Record如何读写数据
- Record如何处理I/O中断

**关键特点**:
- 每种Record类型有自己的DSET结构（ai、ao、longin等）
- 同一种Record类型可以有多个DSET实现（不同硬件）
- DSET通过DTYP字段选择

---

## 🏗️ AI Record的DSET结构

### 完整定义

```c
// 位置: EPICS Base - src/ioc/dbStatic/devSup.h
typedef long (*DEVSUPFUN)(void);  // 函数指针类型

struct aiRecord;  // 前向声明

typedef struct {
    long      number;           // 函数指针数量
    DEVSUPFUN report;           // 索引0: 报告函数
    DEVSUPFUN init;             // 索引1: 驱动初始化
    DEVSUPFUN init_record;      // 索引2: Record初始化 ⭐
    DEVSUPFUN get_ioint_info;   // 索引3: I/O中断信息 ⭐
    DEVSUPFUN read;             // 索引4: 读取函数 ⭐
    DEVSUPFUN special_linconv;  // 索引5: 线性转换
} aiDset;
```

---

## 📋 DSET函数详解

### 1. report() - 设备状态报告

**原型**:
```c
long report(int level);
```

**作用**: 打印设备状态信息（用于dbior命令）

**参数**:
- `level`: 详细级别（0-10）

**BPMIOC实现**:
```c
static long report_ai(int level)
{
    if (level > 0) {
        printf("BPM Monitor Device Support\n");
        printf("  Supported Records: ai, ao, longin, waveform\n");
        printf("  Active Records: %d\n", g_active_record_count);
    }
    return 0;
}
```

**可选**: BPMIOC中为NULL（未实现）

---

### 2. init() - 驱动全局初始化

**原型**:
```c
long init(int after);
```

**作用**: 驱动级别的一次性初始化

**参数**:
- `after`: 0=数据库加载前，1=数据库加载后

**调用时机**:
```
iocInit()
  ↓
devSupInit()  // EPICS Base调用
  ↓
对每个DSET:
  if (dset->init != NULL)
      dset->init(0);  // pass 0 (before database)
  ↓
加载数据库
  ↓
  if (dset->init != NULL)
      dset->init(1);  // pass 1 (after database)
```

**示例**:
```c
static long init_ai(int after)
{
    if (after == 0) {
        printf("BPM Device Support: Pre-database init\n");
        // 可以在这里初始化全局资源
    } else {
        printf("BPM Device Support: Post-database init\n");
        // 可以在这里检查驱动是否已初始化
    }
    return 0;
}
```

**BPMIOC**: 未使用（驱动初始化在drvBPMConfigure中）

---

### 3. init_record() - Record初始化 ⭐

**原型**:
```c
long init_record_ai(aiRecord *prec);
```

**作用**: 初始化每个Record实例

**关键工作**:
1. 分配私有数据（DevPvt）
2. 解析INP/OUT字段
3. 验证参数
4. 初始化scanIo
5. 设置初始值（可选）

**详细实现见**: [03-init-record.md](./03-init-record.md)

---

### 4. get_ioint_info() - I/O中断信息 ⭐

**原型**:
```c
long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt);
```

**作用**: 告诉EPICS这个Record应该在哪个I/O中断上扫描

**参数**:
- `cmd`: 命令（通常是0）
- `prec`: Record指针
- `ppvt`: 输出参数，返回IOSCANPVT

**实现**:
```c
static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    if (pPvt == NULL) {
        return S_dev_badInit;
    }

    // 返回这个Record的IOSCANPVT
    *ppvt = pPvt->ioscanpvt;

    return 0;
}
```

**工作原理**:
```
Record: SCAN="I/O Intr"
  ↓
iocInit时，EPICS调用get_ioint_info
  ↓
获得IOSCANPVT
  ↓
将Record注册到这个IOSCANPVT的链表中
  ↓
驱动调用scanIoRequest(ioscanpvt)时
  ↓
所有注册的Record被扫描
```

---

### 5. read() / write() - 数据读写 ⭐

**原型**:
```c
long read_ai(aiRecord *prec);
long write_ao(aoRecord *prec);
```

**作用**: 实际的数据读写操作

**详细实现见**: [04-read-write.md](./04-read-write.md)

---

### 6. special_linconv() - 线性转换

**原型**:
```c
long special_linconv(aiRecord *prec, int after);
```

**作用**: 处理线性转换参数（EGUF/EGUL）的修改

**参数**:
- `prec`: Record指针
- `after`: 0=字段修改前，1=字段修改后

**使用场景**: 硬件有原始值（raw value），需要转换为工程单位

**BPMIOC**: 未使用（数据已经是工程单位）

---

## 🔧 BPMIOC的4个DSET实现

### 1. AI Record DSET

```c
aiDset devAiBPMMonitor = {
    6,                    // 6个函数指针
    NULL,                 // report: 未实现
    NULL,                 // init: 未实现
    init_record_ai,       // init_record: 必须 ⭐
    get_ioint_info,       // get_ioint_info: 必须 ⭐
    read_ai,              // read: 必须 ⭐
    NULL                  // special_linconv: 未实现
};
epicsExportAddress(dset, devAiBPMMonitor);
```

**用途**: 读取模拟量（RF幅度、相位、XY位置）

---

### 2. AO Record DSET

```c
aoDset devAoBPMMonitor = {
    6,
    NULL,
    NULL,
    init_record_ao,       // 解析OUT字段
    NULL,                 // AO不需要I/O中断
    write_ao,             // write而不是read
    NULL
};
epicsExportAddress(dset, devAoBPMMonitor);
```

**用途**: 写入控制参数（寄存器值）

**关键区别**:
- 使用`write_ao`而不是`read`
- `get_ioint_info`为NULL（输出不需要I/O中断）

---

### 3. Longin Record DSET

```c
longinDset devLonginBPMMonitor = {
    5,                    // longin只有5个函数
    NULL,
    NULL,
    init_record_longin,
    get_ioint_info,
    read_longin
};
epicsExportAddress(dset, devLonginBPMMonitor);
```

**用途**: 读取整数值（寄存器）

---

### 4. Waveform Record DSET

```c
waveformDset devWaveformBPMMonitor = {
    5,
    NULL,
    NULL,
    init_record_waveform,
    get_ioint_info,
    read_waveform          // 读取波形数据
};
epicsExportAddress(dset, devWaveformBPMMonitor);
```

**用途**: 读取波形数据（10000点ADC数据）

---

## 📊 DSET选择机制

### 如何选择DSET？

通过Record的**DTYP**字段：

```
record(ai, "BPM:01:RF3:Amp") {
    field(DTYP, "BPM Monitor")  ← 这里选择设备支持
}
```

### EPICS如何找到DSET？

1. **注册阶段**（编译时）:
```c
epicsExportAddress(dset, devAiBPMMonitor);
// 将devAiBPMMonitor注册为可用的设备支持
```

2. **链接阶段**（iocInit时）:
```c
// EPICS查找名为"BPM Monitor"的设备支持
// 对于ai Record，查找devAi<名称>
// 查找: devAiBPMMonitor ← 匹配！
```

3. **使用阶段**（运行时）:
```c
// EPICS调用DSET中的函数
aiRecord *prec = ...;
aiDset *pdset = (aiDset *)prec->dset;
pdset->init_record(prec);
pdset->read(prec);
```

---

## 🔍 常见问题

### Q1: 为什么不同Record类型需要不同的DSET？

**A**: 每种Record类型有不同的：
- 字段结构（aiRecord vs aoRecord）
- 处理逻辑（read vs write）
- 数据类型（float vs long vs array）

### Q2: 可以一个硬件支持多个DSET吗？

**A**: 可以！例如BPMIOC有4个：
```c
devAiBPMMonitor       // ai类型
devAoBPMMonitor       // ao类型
devLonginBPMMonitor   // longin类型
devWaveformBPMMonitor // waveform类型
```

都对应同一个硬件（BPM Monitor）

### Q3: DSET中哪些函数是必须实现的？

**A**:
- `init_record` - **必须**（初始化Record）
- `read` 或 `write` - **必须**（数据访问）
- `get_ioint_info` - 如果使用I/O中断扫描则**必须**
- 其他函数 - 可选

---

## 💡 设计模式：接口与实现分离

DSET体现了**接口与实现分离**的设计模式：

```
接口层（EPICS定义）:
  aiDset结构 - 定义必须实现的函数

实现层（设备支持编写）:
  devAiBPMMonitor - 具体实现这些函数

使用层（数据库配置）:
  DTYP="BPM Monitor" - 选择实现
```

**好处**:
- EPICS Base不需要知道具体硬件
- 可以轻松支持新硬件（只需新的DSET实现）
- 数据库配置可以灵活切换不同设备

---

## ✅ 学习检查点

- [ ] 理解DSET的6个函数指针及其作用
- [ ] 知道哪些函数是必须实现的
- [ ] 理解DTYP字段如何选择DSET
- [ ] 理解epicsExportAddress的作用
- [ ] 知道不同Record类型需要不同DSET

---

## 🎯 下一步

- **[03-init-record.md](./03-init-record.md)** - 详解init_record实现
- **[04-read-write.md](./04-read-write.md)** - 详解read/write函数
- **[05-devpvt.md](./05-devpvt.md)** - DevPvt数据结构管理

---

**关键理解**: DSET是EPICS设备支持的**契约**（contract），定义了设备支持必须提供的接口。💡
