# 04: BPMIOC内存模型详解

> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 60分钟
> **前置知识**: 01-architecture-overview.md, 02-data-flow.md

## 📋 本文目标

本文深入剖析BPMIOC的内存管理模型，帮助你理解：
- 各层如何分配和管理内存
- 数据缓冲区的大小和用途
- 内存的生命周期
- 如何避免内存泄漏

完成本文后，你将能够：
- ✅ 理解全局数据缓冲区的设计
- ✅ 掌握EPICS Record的内存管理
- ✅ 理解私有数据结构的作用
- ✅ 能够估算IOC的内存占用

## 🏗️ 内存模型总览

BPMIOC采用**分层内存管理**模式：

```
+-------------------+
|   用户空间（CA）   |  ← CA网络缓冲（EPICS管理）
+-------------------+
         ↑
+-------------------+
|  数据库层（DB）    |  ← Record内存（EPICS管理）
+-------------------+  ← DevPvt结构（malloc分配）
         ↑
+-------------------+
|  设备支持层        |  ← 私有数据指针
+-------------------+
         ↑
+-------------------+
|   驱动层          |  ← 全局静态缓冲区（编译时分配）
+-------------------+  ← 函数指针（dlsym获取）
         ↑
+-------------------+
|  硬件库（.so）     |  ← 动态库内存（OS管理）
+-------------------+
```

## 📊 驱动层内存布局

### 1. 全局静态缓冲区

驱动层使用大量**静态全局数组**存储波形数据：

```c
// driverWrapper.c 定义的全局缓冲区

#define buf_len 10000           // 普通波形缓冲区大小
#define trip_buf_len 100000     // 历史波形缓冲区大小

// ===== 正常波形缓冲（10000点）=====
static float rf3amp[buf_len];      // RF3幅度波形
static float rf4amp[buf_len];      // RF4幅度波形
static float rf5amp[buf_len];      // RF5幅度波形
static float rf6amp[buf_len];      // RF6幅度波形
static float rf7amp[buf_len];      // RF7幅度波形
static float rf8amp[buf_len];      // RF8幅度波形
static float rf9amp[buf_len];      // RF9幅度波形
static float rf10amp[buf_len];     // RF10幅度波形

static float rf3phase[buf_len];    // RF3相位波形
static float rf4phase[buf_len];    // RF4相位波形
static float rf5phase[buf_len];    // RF5相位波形
static float rf6phase[buf_len];    // RF6相位波形
static float rf7phase[buf_len];    // RF7相位波形
static float rf8phase[buf_len];    // RF8相位波形
static float rf9phase[buf_len];    // RF9相位波形
static float rf10phase[buf_len];   // RF10相位波形

static float X1[buf_len];          // BPM1 X位置
static float Y1[buf_len];          // BPM1 Y位置
static float X2[buf_len];          // BPM2 X位置
static float Y2[buf_len];          // BPM2 Y位置

// ===== 历史波形缓冲（100000点）=====
static float HistoryX1[trip_buf_len];  // BPM1 X历史
static float HistoryY1[trip_buf_len];  // BPM1 Y历史
static float HistoryX2[trip_buf_len];  // BPM2 X历史
static float HistoryY2[trip_buf_len];  // BPM2 Y历史

// ===== 标量数据 =====
static float X1_avg = 0;           // BPM1 X平均值
static float Y1_avg = 0;           // BPM1 Y平均值
static float X2_avg = 0;           // BPM2 X平均值
static float Y2_avg = 0;           // BPM2 Y平均值

static float ph_ch3 = 0;           // RF3相位
static float ph_offset3 = 0;       // RF3相位偏移
// ... (ch4-ch10类似)

static int pulseMode = 0;          // 脉冲模式
static int AVGStart = 0;           // 平均开始点
static int AVGStop = 0;            // 平均结束点
```

### 2. 内存占用计算

让我们计算驱动层的内存占用：

```
正常波形缓冲区：
- RF幅度波形: 8个通道 × 10000点 × 4字节 = 320 KB
- RF相位波形: 8个通道 × 10000点 × 4字节 = 320 KB
- BPM位置波形: 4个通道 × 10000点 × 4字节 = 160 KB
小计: 800 KB

历史波形缓冲区：
- BPM历史波形: 4个通道 × 100000点 × 4字节 = 1600 KB
小计: 1600 KB

标量数据：
- 各类标量: ~200个 × 4字节 = 800 字节
小计: ~1 KB

函数指针：
- dlsym获取的函数指针: ~50个 × 8字节 = 400 字节
小计: ~1 KB

总计: ~2.4 MB (驱动层静态内存)
```

### 3. 为什么使用静态全局数组？

**优点**：
- ✅ **编译时分配**：程序启动时就已准备好，无需运行时malloc
- ✅ **性能高**：访问速度快，无需动态分配和释放
- ✅ **无碎片**：不会产生内存碎片
- ✅ **线程安全**：配合互斥锁保护，多线程访问安全

**缺点**：
- ❌ **固定大小**：无法根据需要动态调整
- ❌ **内存浪费**：即使不使用某些缓冲区，也会占用内存
- ❌ **不可移植**：缓冲区大小硬编码

**为什么这样设计？**

对于实时控制系统，**可预测性**比**灵活性**更重要：
- 波形采集是固定大小的（10000点）
- 历史数据长度是确定的（100000点）
- 运行时不需要改变缓冲区大小
- 避免malloc失败导致系统崩溃

## 📦 设备支持层内存管理

### 1. DevPvt私有数据结构

每个Record都有一个私有数据结构 `DevPvt`：

```c
// devBPMMonitor.c

typedef struct {
    long        number;         // Record编号
    IOSCANPVT   ioscanpvt;     // I/O扫描私有数据
    long        type;           // 数据类型（offset类型）
    char        *parm;          // 参数字符串（INP/OUT字段）
} DevPvt;
```

### 2. DevPvt的分配和释放

**分配时机**：在 `init_record()` 时

```c
long init_ai(aiRecord *prec)
{
    struct instio *pinstio;
    DevPvt *pPvt;

    // 1. 分配DevPvt内存
    pPvt = (DevPvt *)malloc(sizeof(DevPvt));
    if (!pPvt) {
        recGblRecordError(S_db_noMemory, (void *)prec,
                          "devAiBPM (init_record) malloc failed");
        return S_db_noMemory;
    }

    // 2. 初始化DevPvt
    pPvt->number = prec->rval;
    pPvt->type = OFFSET_UNKNOWN;
    pPvt->parm = NULL;

    // 3. 保存到Record的dpvt字段
    prec->dpvt = pPvt;

    // 4. 解析INP字段，确定offset类型
    pinstio = (struct instio *)&(prec->inp.value);
    pPvt->parm = pinstio->string;

    if (strcmp(pPvt->parm, "RF3Amp") == 0) {
        pPvt->type = OFFSET_RF3Amp;
    } else if (strcmp(pPvt->parm, "RF3Phase") == 0) {
        pPvt->type = OFFSET_RF3Phase;
    }
    // ... 其他类型

    return 0;
}
```

**释放时机**：EPICS会在Record被删除时自动调用 `del_record()`

```c
// EPICS Base会调用这个函数（如果定义）
long del_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    if (pPvt) {
        free(pPvt);  // 释放DevPvt内存
        prec->dpvt = NULL;
    }

    return 0;
}
```

**注意**：BPMIOC目前**没有定义del_record**，因为IOC通常不会删除Record。这是可以接受的，因为：
- IOC启动后Record数量固定
- IOC关闭时整个进程退出，OS会回收所有内存
- DevPvt占用很小（每个Record ~32字节）

### 3. DevPvt的内存占用

假设IOC有500个PV（Record）：

```
DevPvt内存占用：
- 每个DevPvt: sizeof(DevPvt) = 32字节（64位系统）
- 500个PV: 500 × 32 = 16 KB

非常小，可以忽略不计！
```

## 🗄️ 数据库层（Record）内存管理

### 1. Record结构体

每个Record都是一个大型结构体，由EPICS Base定义：

```c
// EPICS Base中的aiRecord定义
typedef struct aiRecord {
    char            name[61];   // Record名称
    char            desc[41];   // 描述
    epicsEnum16     scan;       // 扫描机制
    DBLINK          inp;        // 输入链接
    double          val;        // 当前值
    double          oval;       // 上一次的值
    // ... 还有很多字段（总共100+个字段）
    void            *dpvt;      // 设备私有数据（指向DevPvt）
} aiRecord;
```

### 2. Record内存由谁分配？

**EPICS Base负责**：
- 在加载`.db`文件时，EPICS Base解析数据库
- 为每个Record分配内存（malloc）
- 初始化Record的所有字段
- 调用设备支持层的`init_record()`

**大小**：
- `aiRecord`: ~512字节
- `waveformRecord`: ~800字节（包含数据指针）

### 3. Waveform Record的特殊内存管理

Waveform Record需要额外的数据缓冲区：

```c
// EPICS Base中waveformRecord的定义
typedef struct waveformRecord {
    // ... 其他字段
    epicsUInt32     nelm;       // 元素数量（最大容量）
    void            *bptr;      // 数据缓冲区指针
    // ...
} waveformRecord;
```

**内存分配**：

```c
// EPICS Base在init_record时分配
waveformRecord *prec = ...;

// 根据NELM字段和数据类型分配缓冲区
size_t buffer_size = prec->nelm * element_size;
prec->bptr = malloc(buffer_size);

// 例如：NELM=10000, FTVL=FLOAT
// buffer_size = 10000 × 4 = 40000 字节 (40 KB)
```

**BPMIOC的Waveform缓冲区**：

```
假设有20个Waveform Record (RF幅度、RF相位、BPM位置)：
- 普通波形: 16个 × 10000点 × 4字节 = 640 KB
- 历史波形: 4个 × 100000点 × 4字节 = 1600 KB

总计: ~2.2 MB (Waveform缓冲区)
```

**注意**：这些缓冲区与驱动层的全局数组**是分开的**！
- 驱动层全局数组：存储从硬件读取的原始数据
- Waveform Record缓冲区：存储要发送给客户端的数据
- 数据流：`硬件 → 驱动层全局数组 → Waveform Record缓冲区 → CA网络`

## 🔄 内存的生命周期

### 1. 驱动层全局缓冲区

```
程序启动 ──┬──> 编译时分配静态内存
           │
           ├──> IOC初始化
           │      └──> memset清零（可选）
           │
           ├──> 运行期间
           │      ├──> pthread线程写入数据
           │      └──> read_xxx()函数读取数据
           │
           └──> 程序退出 ──> OS自动回收
```

**特点**：
- 内存地址固定（编译时确定）
- 无需手动释放
- 整个程序生命周期内都存在

### 2. DevPvt结构体

```
IOC启动 ──> iocInit() ──> 初始化所有Record
                              │
                              ├──> 调用init_ai()
                              │      └──> malloc(DevPvt)
                              │            └──> 保存到prec->dpvt
                              │
运行期间 ──> read_ai()/write_ao()
                └──> 访问prec->dpvt获取offset类型

IOC关闭 ──> 进程退出 ──> OS回收所有内存
```

**特点**：
- 每个Record一个DevPvt
- IOC运行期间不会释放
- 内存很小（~32字节/Record）

### 3. Record和Waveform缓冲区

```
加载.db文件 ──> dbLoadRecords()
                    │
                    ├──> 为每个Record分配内存
                    │      └──> malloc(aiRecord/waveformRecord)
                    │
                    ├──> 对于Waveform
                    │      └──> malloc(nelm × element_size)
                    │            └──> 保存到prec->bptr
                    │
运行期间 ──> 处理Record
                └──> 访问prec->val / prec->bptr

IOC关闭 ──> 进程退出 ──> OS回收
```

## 🔐 线程安全与内存保护

### 1. 全局缓冲区的并发访问

**问题**：驱动层全局数组被多个线程访问：
- `pthread`线程：写入数据（从硬件读取）
- EPICS扫描线程：读取数据（`read_ai()`等）

**解决方案**：使用互斥锁

```c
// driverWrapper.c

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// pthread线程写入数据时
void *MonitorThread(void *arg)
{
    while (running) {
        // 从硬件读取数据到临时缓冲
        float temp_amp[8];
        float temp_phase[8];
        GetRfInfo(temp_amp, temp_phase, ...);

        // 加锁保护全局数组
        pthread_mutex_lock(&data_mutex);

        // 复制数据到全局数组
        memcpy(rf3amp, &temp_amp[0], buf_len * sizeof(float));
        memcpy(rf4amp, &temp_amp[1], buf_len * sizeof(float));
        // ...

        pthread_mutex_unlock(&data_mutex);

        // 触发I/O中断扫描
        scanIoRequest(TriginScanPvt);
    }
}

// read函数读取时也需要加锁
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    float value;

    pthread_mutex_lock(&data_mutex);

    switch (pPvt->type) {
        case OFFSET_X1_avg:
            value = X1_avg;  // 读取全局变量
            break;
        case OFFSET_Y1_avg:
            value = Y1_avg;
            break;
        // ...
    }

    pthread_mutex_unlock(&data_mutex);

    prec->val = value;
    return 0;
}
```

### 2. Waveform数据的零拷贝

**优化**：对于Waveform，使用指针共享而非复制

```c
static long read_wf(waveformRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 方法1：拷贝数据（慢）
    // memcpy(prec->bptr, rf3amp, buf_len * sizeof(float));

    // 方法2：共享指针（快，但不安全）
    // prec->bptr = rf3amp;  // 危险！Record不拥有这块内存

    // 方法3：实际使用（折中）
    // BPMIOC使用memcpy，确保数据一致性
    pthread_mutex_lock(&data_mutex);

    switch (pPvt->type) {
        case OFFSET_WF_RF3Amp:
            memcpy(prec->bptr, rf3amp, prec->nelm * sizeof(float));
            prec->nord = buf_len;  // 实际数据点数
            break;
        // ...
    }

    pthread_mutex_unlock(&data_mutex);

    return 0;
}
```

## 📏 内存占用总结

让我们总结BPMIOC的总内存占用：

```
驱动层（driverWrapper.c）：
├─ 全局波形缓冲区: 800 KB
├─ 历史缓冲区: 1600 KB
├─ 标量数据: 1 KB
├─ 函数指针: 1 KB
└─ 小计: ~2.4 MB

设备支持层（devBPMMonitor.c）：
└─ DevPvt结构 (500个PV): 16 KB

数据库层（EPICS Record）：
├─ Record结构体 (500个): 256 KB
├─ Waveform数据缓冲区: 2.2 MB
└─ 小计: ~2.5 MB

EPICS Base开销：
├─ 任务堆栈: ~2 MB
├─ CA服务器: ~1 MB
├─ 其他: ~1 MB
└─ 小计: ~4 MB

硬件库（.so）：
└─ libBPMboard14And15.so: ~500 KB

===========================================
总计: ~9.5 MB (不含OS内核)
===========================================
```

**结论**：
- BPMIOC是一个轻量级IOC（~10 MB）
- 主要内存用于波形缓冲区
- 适合在嵌入式ZYNQ板上运行（通常有512 MB RAM）

## 🎯 内存优化建议

### 1. 如果内存紧张

```c
// 方案1：减小缓冲区大小
#define buf_len 5000       // 从10000减到5000
#define trip_buf_len 50000 // 从100000减到50000

// 方案2：使用动态分配（牺牲实时性）
static float *rf3amp = NULL;
void InitBuffers() {
    rf3amp = (float *)malloc(buf_len * sizeof(float));
}

// 方案3：删除不用的缓冲区
// 如果不需要RF9和RF10，注释掉相关数组
```

### 2. 如果需要更大缓冲区

```c
// 增加缓冲区大小
#define buf_len 20000       // 增加到20000点
#define trip_buf_len 200000 // 增加到200000点

// 注意：需要重新编译IOC
```

### 3. 内存泄漏检测

```bash
# 使用valgrind检测内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./st.cmd

# 正常情况下应该没有内存泄漏
# 因为所有内存都是静态分配或由EPICS管理
```

## ✅ 学习检查点

完成本文后，你应该能够回答：

1. **全局缓冲区**：
   - [ ] 驱动层有哪些主要的全局数组？
   - [ ] 为什么使用静态数组而不是malloc？
   - [ ] `buf_len`和`trip_buf_len`的区别？

2. **DevPvt结构**：
   - [ ] DevPvt何时分配？存储什么信息？
   - [ ] DevPvt如何与Record关联？
   - [ ] 为什么不需要手动释放DevPvt？

3. **Waveform缓冲区**：
   - [ ] Waveform Record的缓冲区何时分配？
   - [ ] 驱动层数组和Waveform缓冲区是同一块内存吗？
   - [ ] 数据如何从驱动层传递到Waveform Record？

4. **线程安全**：
   - [ ] 为什么需要互斥锁保护全局数组？
   - [ ] 哪些线程会访问全局数组？
   - [ ] 如何避免死锁？

5. **内存估算**：
   - [ ] BPMIOC大约占用多少内存？
   - [ ] 如果增加一个10000点的Waveform PV，增加多少内存？

## 🔗 相关文档

- **[01-architecture-overview.md](./01-architecture-overview.md)** - 理解三层架构
- **[02-data-flow.md](./02-data-flow.md)** - 理解数据如何流动
- **[05-offset-system.md](./05-offset-system.md)** - Offset系统设计（下一篇）
- **[06-thread-model.md](./06-thread-model.md)** - 线程模型详解

## 📚 扩展阅读

### EPICS官方文档
- [Record Reference Manual](https://epics.anl.gov/base/R3-15/6-docs/RecordReference.html)
- [Application Developer's Guide - Chapter 2](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/node4.html)

### C语言内存管理
- 《The C Programming Language》(K&R) - Chapter 5, 7
- 《Expert C Programming》- Chapter 4: The Shocking Truth

---

**下一篇**: [05-offset-system.md](./05-offset-system.md) - Offset系统设计

**实践练习**:
1. 计算如果添加2个新的RF通道，内存会增加多少
2. 使用`ps aux`查看实际IOC的内存占用
3. 修改`buf_len`大小并重新编译，观察内存变化
