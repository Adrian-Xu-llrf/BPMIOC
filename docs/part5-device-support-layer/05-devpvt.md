# Part 5.5: DevPvt私有数据结构详解

> **目标**: 深入理解DevPvt的设计和使用
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 45分钟

## 📖 什么是DevPvt？

**DevPvt** = **Device Private Data**（设备私有数据）

它是设备支持层为每个Record实例存储的私有信息。

---

## 🏗️ BPMIOC的DevPvt结构

### 定义

```c
// 位置: BPMmonitorApp/src/devBPMMonitor.c

typedef struct {
    int offset;           // ReadData的offset参数
    int channel;          // ReadData的channel参数
    int type;             // ReadData的type参数
    IOSCANPVT ioscanpvt;  // I/O中断扫描结构
} DevPvt;
```

---

## 🤔 为什么需要DevPvt？

### 问题

Record是**通用的**，不知道具体硬件的参数：

```c
// aiRecord结构（EPICS定义）
struct aiRecord {
    char name[61];
    double val;       // 值
    double eslo;      // 工程单位斜率
    // ... 很多通用字段
    void *dpvt;       // ← 这是给设备支持用的
};
```

**问题**: 如何存储特定于BPM硬件的参数（offset、channel、type）？

---

### 解决方案

使用`dpvt`字段存储指向DevPvt的指针：

```
aiRecord
├─ name: "BPM:01:RF3:Amp"
├─ val: 1.023
├─ ...
└─ dpvt: ───────┐
                 ↓
            DevPvt
            ├─ offset: 0
            ├─ channel: 3
            ├─ type: 0
            └─ ioscanpvt: ...
```

---

## 🔄 DevPvt的生命周期

### 1. 创建（init_record）

```c
static long init_record_ai(aiRecord *prec)
{
    // 分配内存
    DevPvt *pPvt = (DevPvt *)malloc(sizeof(DevPvt));
    if (pPvt == NULL) {
        return S_dev_noMemory;
    }

    // 解析INP字段，填充DevPvt
    sscanf(prec->inp.value.instio.string, "@%d %d %d",
           &pPvt->offset, &pPvt->channel, &pPvt->type);

    // 初始化scanIo
    scanIoInit(&pPvt->ioscanpvt);

    // 保存到Record
    prec->dpvt = pPvt;

    return 0;
}
```

---

### 2. 使用（read/write）

```c
static long read_ai(aiRecord *prec)
{
    // 获取DevPvt
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 使用DevPvt中的参数
    float value = ReadData(pPvt->offset,
                          pPvt->channel,
                          pPvt->type);

    prec->val = value;
    return 0;
}
```

---

### 3. 销毁（可选）

**BPMIOC未实现**，但应该实现：

```c
// 如果EPICS Base支持设备支持的cleanup函数
static long cleanup_record(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    if (pPvt != NULL) {
        // 释放ioscanpvt（如果需要）
        // ...

        // 释放DevPvt内存
        free(pPvt);
        prec->dpvt = NULL;
    }

    return 0;
}
```

---

## 🔍 DevPvt字段详解

### 字段1: offset

**用途**: 指定数据类型（Offset系统）

```c
// 在driverWrapper.c中定义
#define OFFSET_RF           0  // RF数据
#define OFFSET_XY           1  // XY位置
#define OFFSET_BUTTON       2  // Button信号
#define OFFSET_PHASE        3  // 相位
#define OFFSET_REAL_IMAG    4  // 实部虚部
#define OFFSET_WAVEFORM     6  // 波形数据
// ...
```

**示例**:
```
INP="@0 3 0" → offset=0 → OFFSET_RF → 读取RF数据
INP="@1 0 0" → offset=1 → OFFSET_XY → 读取XY位置
```

---

### 字段2: channel

**用途**: 指定硬件通道号

**含义取决于offset**:
```c
if (offset == OFFSET_RF) {
    // channel = 3-6 (RF3, RF4, RF5, RF6)
} else if (offset == OFFSET_XY) {
    // channel = 0-7 (XY1-X, XY1-Y, ..., XY4-Y)
} else if (offset == OFFSET_BUTTON) {
    // channel = 0-3 (Button1-Button4)
}
```

---

### 字段3: type

**用途**: 指定数据的子类型

**RF offset的type**:
```c
type = 0: 幅度（Amplitude）
type = 1: 相位（Phase）
type = 2: 实部（Real）
type = 3: 虚部（Imaginary）
```

**XY offset的type**:
```c
type = 0: 位置值
type = 1: RMS值
type = 2: 平均值
```

---

### 字段4: ioscanpvt

**类型**: `IOSCANPVT`（EPICS定义的不透明类型）

**用途**: I/O中断扫描支持

**工作原理**:
```
1. init_record时
   scanIoInit(&pPvt->ioscanpvt);
     ↓
   创建IOSCANPVT结构

2. get_ioint_info时
   *ppvt = pPvt->ioscanpvt;
     ↓
   EPICS将Record注册到这个扫描列表

3. 驱动触发时
   scanIoRequest(TriginScanPvt);
     ↓
   所有注册的Record被处理
```

**关键**: 每个Record有自己的ioscanpvt，但在BPMIOC中，所有Record共享同一个全局`TriginScanPvt`（在driverWrapper.c中）。

---

## 🔧 扩展DevPvt

### 添加新字段

如果需要更多信息，可以扩展DevPvt：

```c
typedef struct {
    // 原有字段
    int offset;
    int channel;
    int type;
    IOSCANPVT ioscanpvt;

    // 新增字段
    float scale_factor;     // 缩放因子
    float offset_adjust;    // 偏移调整
    int  averaging;         // 平均次数
    char unit[16];          // 单位字符串
} DevPvt;
```

**使用场景**:
```c
static long init_record_ai(aiRecord *prec)
{
    // ...

    // 解析扩展的INP字段
    // 例如: "@0 3 0 1.5 0.1 10 V"
    int nvals = sscanf(plink->value.instio.string,
                      "@%d %d %d %f %f %d %s",
                      &pPvt->offset,
                      &pPvt->channel,
                      &pPvt->type,
                      &pPvt->scale_factor,    // 新字段
                      &pPvt->offset_adjust,   // 新字段
                      &pPvt->averaging,       // 新字段
                      pPvt->unit);            // 新字段

    // ...
}

static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 读取原始值
    float raw = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 应用DevPvt中的缩放和偏移
    float scaled = raw * pPvt->scale_factor + pPvt->offset_adjust;

    prec->val = scaled;
    return 0;
}
```

---

## 📊 多种DevPvt设计

### 方案1: 共享结构（BPMIOC采用）

**优点**: 简单、统一
**缺点**: 所有Record类型共享同一结构，可能浪费内存

```c
typedef struct {
    int offset;
    int channel;
    int type;
    IOSCANPVT ioscanpvt;
} DevPvt;

// 所有Record类型使用相同的DevPvt
```

---

### 方案2: 分离结构

**优点**: 每种Record只存储需要的信息
**缺点**: 代码复杂

```c
// AI Record的私有数据
typedef struct {
    int offset;
    int channel;
    int type;
    IOSCANPVT ioscanpvt;
} AiDevPvt;

// AO Record的私有数据（不需要ioscanpvt）
typedef struct {
    int channel;
    int type;
} AoDevPvt;

// Waveform的私有数据
typedef struct {
    int offset;
    int channel;
    int index;
    IOSCANPVT ioscanpvt;
    float *buffer;  // 波形缓冲区
    int buffer_size;
} WaveformDevPvt;
```

---

## ⚠️ 常见陷阱

### 陷阱1: 忘记初始化

```c
// ❌ 错误
DevPvt *pPvt = malloc(sizeof(DevPvt));
// pPvt->offset可能是随机值！
prec->dpvt = pPvt;

// ✅ 正确
DevPvt *pPvt = malloc(sizeof(DevPvt));
memset(pPvt, 0, sizeof(DevPvt));  // 清零
// 或者
DevPvt *pPvt = calloc(1, sizeof(DevPvt));  // 分配并清零
```

---

### 陷阱2: 类型转换错误

```c
// ❌ 错误
static long read_ai(aiRecord *prec)
{
    AoDevPvt *pPvt = (AoDevPvt *)prec->dpvt;  // 错误的类型！
    // ...
}

// ✅ 正确
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;  // 正确的类型
    // ...
}
```

---

### 陷阱3: 内存泄漏

```c
// ❌ 问题：Record删除时DevPvt未释放
// 导致：长期运行的IOC可能慢慢耗尽内存

// ✅ 解决：实现cleanup函数（如果EPICS支持）
// 或者：确保IOC不频繁创建/删除Record
```

---

## 🎨 调试技巧

### 打印DevPvt内容

```c
static void print_devpvt(DevPvt *pPvt, const char *label)
{
    if (pPvt == NULL) {
        printf("%s: DevPvt is NULL\n", label);
        return;
    }

    printf("%s: DevPvt {\n", label);
    printf("  offset  = %d\n", pPvt->offset);
    printf("  channel = %d\n", pPvt->channel);
    printf("  type    = %d\n", pPvt->type);
    printf("  ioscanpvt = %p\n", (void *)pPvt->ioscanpvt);
    printf("}\n");
}

// 使用
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    print_devpvt(pPvt, "read_ai");
    // ...
}
```

---

## ✅ 学习检查点

- [ ] 理解DevPvt的作用和必要性
- [ ] 理解DevPvt的生命周期
- [ ] 知道DevPvt的4个字段及其用途
- [ ] 能够扩展DevPvt添加新字段
- [ ] 知道如何调试DevPvt问题

---

## 🎯 总结

### DevPvt的本质

DevPvt是设备支持层的**记忆**：
- 在init_record时**学习**（解析INP字段）
- 在read/write时**回忆**（使用解析的参数）

### 设计模式

DevPvt体现了**桥接模式**：
- 桥接通用Record和特定硬件
- 解耦Record定义和硬件实现

---

## 🎯 Part 5总结

完成Part 5的5个核心文档后，你现在理解了：

✅ **设备支持层的作用** - 连接Record和驱动
✅ **DSET结构** - 设备支持的接口定义
✅ **init_record** - Record初始化过程
✅ **read/write** - 数据读写实现
✅ **DevPvt** - 私有数据管理

**下一步**: 学习Part 6数据库层，了解如何配置PV和Record！

---

**关键理解**: DevPvt是设备支持层的**核心数据结构**，存储了连接Record和驱动所需的所有信息！💡
