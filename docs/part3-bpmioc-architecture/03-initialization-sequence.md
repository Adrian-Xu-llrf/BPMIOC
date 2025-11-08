# 03 - 启动序列详解

> **目标**: 深入理解BPMIOC从启动到就绪的完整初始化过程
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 75分钟
> **前置知识**: 01-architecture-overview.md, 02-data-flow.md

## 📋 学习目标

完成本节后,你将能够：
- ✅ 理解IOC的完整启动流程
- ✅ 掌握InitDevice()的详细执行步骤
- ✅ 理解动态库加载机制（dlopen/dlsym）
- ✅ 理解记录初始化顺序
- ✅ 理解线程创建和I/O中断初始化
- ✅ 能够调试初始化问题

## 🚀 1. 启动流程总览

### 从st.cmd到IOC运行

```
用户执行:
  $ ./st.cmd
     ↓
1. EPICS初始化
     ↓
2. 加载数据库
     ↓
3. 初始化记录
     ↓
4. 启动IOC (iocInit)
     ↓
5. 启动扫描线程
     ↓
6. IOC就绪
```

### 时间线

```
T=0ms:     启动 ./st.cmd
T=10ms:    dbLoadRecords() 开始
T=50ms:    数据库加载完成（168个记录）
T=60ms:    iocInit() 开始
T=65ms:    第一个ai记录初始化 → 触发 InitDevice()
T=100ms:   InitDevice() 完成（dlopen, 线程创建）
T=105ms:   所有记录初始化完成
T=110ms:   扫描线程启动
T=115ms:   DataAcquisitionThread 开始运行
T=120ms:   第一次 scanIoRequest()
T=125ms:   IOC完全就绪
```

## 📂 2. st.cmd启动脚本

### 典型st.cmd内容

```bash
#!../../bin/linux-x86_64/BPMmonitor

# 1. 设置环境变量
epicsEnvSet("ARCH","linux-x86_64")
epicsEnvSet("IOC","iocBPMmonitor")

# 2. 切换到IOC目录
cd "${TOP}"

# 3. 注册所有支持的记录类型和设备支持
dbLoadDatabase "dbd/BPMmonitor.dbd"
BPMmonitor_registerRecordDeviceDriver pdbbase

# 4. 加载数据库
dbLoadRecords("db/BPMMonitor.db", "P=iLinac_007:BPM14And15")

# 5. 设置IOC日志
iocLogInit

# 6. 初始化IOC（关键步骤！）
iocInit

# 7. 启动CA服务器（自动）
# Channel Access现在开始监听5064/5065端口
```

### 关键命令解析

#### dbLoadDatabase

```c
// 加载.dbd文件（数据库定义）
dbLoadDatabase("dbd/BPMmonitor.dbd")

// .dbd文件内容:
include "base.dbd"           // EPICS标准记录类型
device(ai, INST_IO, devAiBPMmonitor, "BPMmonitor")  // ← 注册设备支持
device(ao, INST_IO, devAoBPMmonitor, "BPMmonitor")
device(waveform, INST_IO, devWaveformBPMmonitor, "BPMmonitor")
```

**作用**:
- 告诉EPICS哪些记录类型可用
- 注册设备支持层接口（dset）
- 建立DTYP字段和dset的映射

#### dbLoadRecords

```c
// 加载记录数据库
dbLoadRecords("db/BPMMonitor.db", "P=iLinac_007:BPM14And15")

// 执行过程:
// 1. 读取.db文件
// 2. 宏替换: $(P) → iLinac_007:BPM14And15
// 3. 创建记录实例（168个）
// 4. 存入内存数据库（尚未初始化）
```

**此时**:
- ✅ 记录定义已加载到内存
- ❌ 记录尚未初始化
- ❌ 设备支持层的init_record未调用
- ❌ InitDevice未调用
- ❌ 硬件库未加载

#### iocInit

```c
iocInit()

// 这是魔法发生的地方！
```

让我们深入这个函数...

## 🔧 3. iocInit详解

### iocInit流程图

```
iocInit()
  ↓
├→ 1. 初始化数据库系统
├→ 2. 初始化驱动支持（driver support）
├→ 3. 初始化所有记录 ← InitDevice在这里被调用
│     ↓
│     对每个记录:
│       ├→ 调用 init_record()
│       ├→ 第一个ai记录 → devAiBPMmonitor::init_ai_record()
│       │                      ↓
│       │                    InitDevice() ← 首次调用
│       │                      ↓
│       │                    [加载硬件库、创建线程...]
│       ├→ 第二个ai记录 → init_ai_record()
│       │                    InitDevice() ← 已初始化，跳过
│       └→ ... 继续处理所有168个记录
│
├→ 4. 初始化扫描系统
├→ 5. 启动CA服务器
├→ 6. 启动扫描线程
└→ 7. 返回成功
```

### 伪代码

```c
int iocInit(void)
{
    // 1. 初始化数据库
    if (dbInitialize() != 0) {
        printf("Database initialization failed\n");
        return -1;
    }

    // 2. 初始化所有记录
    for (each record in database) {
        // 根据记录类型调用对应的init函数
        if (record->rtype == ai) {
            aiRecord *pai = (aiRecord *)record;

            // 调用设备支持层的init_record
            if (pai->dset && pai->dset->init_record) {
                status = pai->dset->init_record(pai);
                // ↑ 这里会调用 devBPMMonitor.c::init_ai_record()
            }
        }
        // ... 对其他记录类型做同样操作
    }

    // 3. 初始化扫描系统
    scanInit();

    // 4. 启动CA服务器
    caServerInit();

    // 5. 启动所有扫描线程
    scanRun();

    printf("iocInit: All initialization complete\n");
    return 0;
}
```

## 📝 4. 记录初始化：init_ai_record()

### 调用时机

```
iocInit()
  → 遍历所有记录
  → 发现 ai 记录: RFIn_01_Amp
  → DTYP="BPMmonitor" → 查找 devAiBPMmonitor
  → 调用 devAiBPMmonitor.init_record(pRec)
```

### devBPMMonitor.c::init_ai_record()

```c
static long init_ai_record(aiRecord *pRec)
{
    struct instio *pInstio;
    DevPvt *pPvt;
    char type_str[16];
    int offset_num, channel;

    // 1. 检查INP字段类型
    if (pRec->inp.type != INST_IO) {
        recGblRecordError(S_db_badField, pRec,
            "devAiBPMmonitor: INP field must be INST_IO");
        return S_db_badField;
    }

    // 2. 解析INP字段
    //    INP = "@AMP:0 ch=0"
    pInstio = &pRec->inp.value.instio;
    int ret = sscanf(pInstio->string, "@%[^:]:%d ch=%d",
                     type_str, &offset_num, &channel);

    if (ret != 3) {
        recGblRecordError(S_db_badField, pRec,
            "devAiBPMmonitor: INP parse failed");
        return S_db_badField;
    }

    // 3. 第一次调用？初始化设备
    static int device_initialized = 0;
    if (!device_initialized) {
        printf("First record initialization, calling InitDevice()\n");

        // 调用驱动层初始化
        int status = InitDevice(0);  // ← 关键！

        if (status != 0) {
            printf("InitDevice failed: %d\n", status);
            return -1;
        }

        device_initialized = 1;
        printf("InitDevice succeeded\n");
    }

    // 4. 分配私有数据结构
    pPvt = (DevPvt *)malloc(sizeof(DevPvt));
    if (pPvt == NULL) {
        return S_db_noMemory;
    }

    // 5. 保存offset和channel
    pPvt->offset = offset_num;   // 0
    pPvt->channel = channel;     // 0
    strcpy(pPvt->type_str, type_str);  // "AMP"

    // 6. 附加到记录
    pRec->dpvt = pPvt;

    printf("init_ai_record: %s → offset=%d, ch=%d\n",
           pRec->name, offset_num, channel);

    return 0;
}
```

### 关键点

1. **静态标志**: `device_initialized`确保InitDevice只调用一次
2. **INP解析**: 提取type/offset/channel三元组
3. **DevPvt分配**: 每个记录都有独立的DevPvt结构
4. **错误处理**: 任何失败都会阻止IOC启动

## 🔌 5. InitDevice详解

### 完整流程

```
InitDevice(int type)
  ↓
Step 1: 动态加载硬件库 (dlopen)
  ↓
Step 2: 加载所有函数指针 (dlsym × 30+)
  ↓
Step 3: 调用硬件初始化 (funcSystemInit)
  ↓
Step 4: 分配全局数组内存 (malloc)
  ↓
Step 5: 创建数据采集线程 (pthread_create)
  ↓
Step 6: 初始化I/O中断扫描 (scanIoInit)
  ↓
返回成功
```

### Step 1: dlopen加载库

```c
int InitDevice(int type)
{
    void *handle;
    char lib_path[256];

    // 1. 构造库路径
    //    假设库文件在 /opt/BPMIOC/lib/
    sprintf(lib_path, "/opt/BPMIOC/lib/libBPMboard14And15.so");

    printf("Loading hardware library: %s\n", lib_path);

    // 2. 动态加载共享库
    handle = dlopen(lib_path, RTLD_LAZY);

    if (handle == NULL) {
        printf("dlopen failed: %s\n", dlerror());
        return -1;
    }

    printf("Library loaded successfully\n");

    // 保存句柄供后续使用
    global_lib_handle = handle;

    // 继续...
}
```

**dlopen参数**:
- `lib_path`: 库文件的完整路径
- `RTLD_LAZY`: 延迟绑定（函数在首次调用时解析）
- 返回值: 库句柄（用于dlsym）

### Step 2: dlsym加载函数

```c
    // 3. 加载所有硬件函数
    printf("Loading hardware functions...\n");

    // 系统初始化
    funcSystemInit = dlsym(handle, "SystemInit");
    if (funcSystemInit == NULL) {
        printf("Failed to load SystemInit: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    // RF信息读取
    funcGetRfInfo = dlsym(handle, "GetRfInfo");
    if (funcGetRfInfo == NULL) {
        printf("Failed to load GetRfInfo\n");
        dlclose(handle);
        return -1;
    }

    // 触发波形读取
    funcGetTrigWaveform = dlsym(handle, "GetTrigWaveform");
    if (funcGetTrigWaveform == NULL) {
        printf("Failed to load GetTrigWaveform\n");
        dlclose(handle);
        return -1;
    }

    // 设置函数（写操作）
    funcSetPhaseOffset = dlsym(handle, "SetPhaseOffset");
    funcSetAmpOffset = dlsym(handle, "SetAmpOffset");
    funcSetPowerThreshold = dlsym(handle, "SetPowerThreshold");

    // ... 加载更多函数（总共30+个）

    printf("All %d functions loaded\n", function_count);
```

**函数指针声明**（在头文件中）:
```c
// driverWrapper.h
extern int (*funcSystemInit)(void);
extern int (*funcGetRfInfo)(float *, float *, float *,
                            float *, float *, float *, float *, int *);
extern int (*funcGetTrigWaveform)(int channel, float *buffer, int *npts);
extern int (*funcSetPhaseOffset)(int channel, float value);
// ... 更多声明
```

### Step 3: 调用硬件初始化

```c
    // 4. 初始化硬件
    printf("Calling SystemInit()...\n");

    int status = funcSystemInit();

    if (status != 0) {
        printf("SystemInit failed: %d\n", status);
        dlclose(handle);
        return -1;
    }

    printf("Hardware initialized successfully\n");
```

**funcSystemInit做什么**（在libBPMboard14And15.so内）:
```c
int SystemInit(void)
{
    // 1. 初始化VME总线
    vme_init();

    // 2. 检测BPM板卡
    if (detect_bpm_board() != 0) {
        return -1;
    }

    // 3. 配置寄存器
    configure_registers();

    // 4. 自检
    if (self_test() != 0) {
        return -1;
    }

    return 0;
}
```

### Step 4: 分配全局数组

```c
    // 5. 分配全局数据缓冲区
    printf("Allocating global arrays...\n");

    Amp = (float *)malloc(8 * sizeof(float));
    Phase = (float *)malloc(8 * sizeof(float));
    Power = (float *)malloc(8 * sizeof(float));
    VBPM = (float *)malloc(8 * sizeof(float));
    IBPM = (float *)malloc(8 * sizeof(float));
    VCRI = (float *)malloc(8 * sizeof(float));
    ICRI = (float *)malloc(8 * sizeof(float));
    PSET = (int *)malloc(8 * sizeof(int));

    if (Amp == NULL || Phase == NULL || Power == NULL) {
        printf("Memory allocation failed\n");
        return -1;
    }

    // 初始化为0
    memset(Amp, 0, 8 * sizeof(float));
    memset(Phase, 0, 8 * sizeof(float));
    memset(Power, 0, 8 * sizeof(float));
    // ... 其他数组

    printf("Global arrays allocated\n");
```

**全局数组声明**（在driverWrapper.c顶部）:
```c
// 全局数据缓冲区
float *Amp = NULL;      // 8个通道幅度
float *Phase = NULL;    // 8个通道相位
float *Power = NULL;    // 8个通道功率
float *VBPM = NULL;     // BPM电压
float *IBPM = NULL;     // BPM电流
float *VCRI = NULL;     // CRI电压
float *ICRI = NULL;     // CRI电流
int *PSET = NULL;       // 相位设置
```

### Step 5: 创建数据采集线程

```c
    // 6. 创建数据采集线程
    printf("Creating data acquisition thread...\n");

    pthread_t thread_id;
    int ret = pthread_create(
        &thread_id,                  // 线程ID
        NULL,                        // 默认属性
        DataAcquisitionThread,       // 线程函数
        NULL                         // 参数（无）
    );

    if (ret != 0) {
        printf("pthread_create failed: %d\n", ret);
        return -1;
    }

    // 分离线程（不需要join）
    pthread_detach(thread_id);

    printf("Data acquisition thread started (TID=%lu)\n", thread_id);
```

**线程函数**:
```c
void *DataAcquisitionThread(void *arg)
{
    printf("[Thread] Data acquisition thread started\n");

    while (1) {
        // 读取硬件数据
        int status = funcGetRfInfo(Amp, Phase, Power,
                                   VBPM, IBPM, VCRI, ICRI, PSET);

        if (status == 0) {
            // 触发I/O中断扫描
            scanIoRequest(ioScanPvt);
        }

        // 延时100ms
        epicsThreadSleep(0.1);
    }

    return NULL;
}
```

### Step 6: 初始化I/O中断扫描

```c
    // 7. 初始化I/O中断扫描
    printf("Initializing I/O interrupt scan...\n");

    scanIoInit(&ioScanPvt);

    printf("I/O interrupt scan initialized\n");

    // 8. 完成
    printf("InitDevice completed successfully\n");
    return 0;
}
```

**ioScanPvt**:
```c
// 全局变量
IOSCANPVT ioScanPvt = NULL;

// scanIoInit 分配并初始化这个结构
// 所有 SCAN="I/O Intr" 的记录将注册到这个pvt
```

## 🔄 6. get_ioint_info()

### 何时调用

```
iocInit()
  → 初始化记录
  → 发现 SCAN="I/O Intr"
  → 调用 get_ioint_info() 获取ioScanPvt
  → 注册记录到这个pvt
```

### 代码

```c
static long get_ioint_info(int cmd, aiRecord *pRec, IOSCANPVT *ppvt)
{
    // 返回全局的ioScanPvt
    *ppvt = ioScanPvt;
    return 0;
}
```

**作用**:
- 告诉EPICS这个记录应该注册到哪个I/O中断源
- 当`scanIoRequest(ioScanPvt)`被调用时，所有注册的记录将被处理

## 📊 7. 完整初始化时序图

```
时间 | 操作                                      | 位置
-----|------------------------------------------|------------------
0ms  | ./st.cmd 执行                            | Shell
5ms  | dbLoadDatabase("BPMmonitor.dbd")         | st.cmd
10ms | dbLoadRecords("BPMMonitor.db", ...)      | st.cmd
     |   → 创建168个记录实例                    | EPICS Core
50ms | iocInit()                                | st.cmd
51ms |   → dbInitialize()                       | EPICS Core
52ms |   → 遍历记录数据库                       | EPICS Core
53ms |   → 第1个ai记录: RFIn_01_Amp            | EPICS Core
54ms |     → init_ai_record(pRec)               | devBPMMonitor.c
55ms |       → InitDevice(0)                    | driverWrapper.c
56ms |         → dlopen("libBPM...so")          | driverWrapper.c
70ms |         → dlsym × 30 (加载函数)          | driverWrapper.c
75ms |         → funcSystemInit()               | libBPMboard.so
     |           → VME初始化                    | 硬件层
     |           → 板卡检测                     | 硬件层
90ms |         → malloc全局数组                 | driverWrapper.c
92ms |         → pthread_create()               | driverWrapper.c
     |           → DataAcquisitionThread启动   | 新线程
95ms |         → scanIoInit(&ioScanPvt)        | driverWrapper.c
96ms |       ← InitDevice返回0                 | driverWrapper.c
97ms |       → malloc(DevPvt)                   | devBPMMonitor.c
98ms |     ← init_ai_record返回0               | devBPMMonitor.c
99ms |   → 第2个ai记录: RFIn_02_Amp            | EPICS Core
100ms|     → init_ai_record(pRec)               | devBPMMonitor.c
     |       → InitDevice已初始化，跳过         | devBPMMonitor.c
101ms|   → 第3个ai记录...                       | EPICS Core
     |   ...                                    |
110ms|   → 第168个记录初始化完成                | EPICS Core
111ms|   → scanInit()                           | EPICS Core
112ms|   → scanRun()                            | EPICS Core
     |     → 启动I/O Intr扫描线程               | EPICS Core
     |     → 启动周期扫描线程                   | EPICS Core
113ms|   → caServerInit()                       | EPICS Core
     |     → 监听5064/5065端口                  | CA服务器
115ms| ← iocInit返回                            | st.cmd
116ms| IOC就绪提示                              | IOC Shell
     |                                          |
120ms| [DataAcquisitionThread]                  | 独立线程
     |   → funcGetRfInfo(...)                   | 线程
     |   → 更新全局数组                         | 线程
     |   → scanIoRequest(ioScanPvt)            | 线程
121ms|   → 触发所有I/O Intr记录处理             | EPICS扫描线程
     |   → epicsThreadSleep(0.1)                | 线程
220ms|   → 下一次采集...                        | 线程
```

## 🐛 8. 调试初始化问题

### 8.1 添加调试输出

**在init_ai_record中**:
```c
static long init_ai_record(aiRecord *pRec)
{
    printf("[INIT] Initializing record: %s\n", pRec->name);
    printf("[INIT]   INP string: %s\n", pRec->inp.value.instio.string);

    // ... 解析INP

    printf("[INIT]   Parsed: type=%s, offset=%d, channel=%d\n",
           type_str, offset_num, channel);

    if (!device_initialized) {
        printf("[INIT] === Calling InitDevice ===\n");
        int status = InitDevice(0);
        printf("[INIT] === InitDevice returned %d ===\n", status);
        device_initialized = 1;
    }

    printf("[INIT] Record %s initialized successfully\n", pRec->name);
    return 0;
}
```

### 8.2 在InitDevice中添加阶段标记

```c
int InitDevice(int type)
{
    printf("\n");
    printf("========================================\n");
    printf("InitDevice START\n");
    printf("========================================\n");

    printf("[Phase 1] Loading library...\n");
    handle = dlopen(lib_path, RTLD_LAZY);
    if (handle == NULL) {
        printf("[Phase 1] FAILED: %s\n", dlerror());
        return -1;
    }
    printf("[Phase 1] SUCCESS\n");

    printf("[Phase 2] Loading functions...\n");
    // ... dlsym调用
    printf("[Phase 2] SUCCESS (loaded %d functions)\n", count);

    printf("[Phase 3] Hardware initialization...\n");
    int status = funcSystemInit();
    if (status != 0) {
        printf("[Phase 3] FAILED: %d\n", status);
        return -1;
    }
    printf("[Phase 3] SUCCESS\n");

    printf("[Phase 4] Allocating memory...\n");
    // ... malloc
    printf("[Phase 4] SUCCESS\n");

    printf("[Phase 5] Creating thread...\n");
    // ... pthread_create
    printf("[Phase 5] SUCCESS (TID=%lu)\n", thread_id);

    printf("[Phase 6] I/O interrupt init...\n");
    scanIoInit(&ioScanPvt);
    printf("[Phase 6] SUCCESS\n");

    printf("========================================\n");
    printf("InitDevice COMPLETE\n");
    printf("========================================\n\n");

    return 0;
}
```

### 8.3 输出示例

```
IOC starting...
[INIT] Initializing record: iLinac_007:BPM14And15:RFIn_01_Amp
[INIT]   INP string: @AMP:0 ch=0
[INIT]   Parsed: type=AMP, offset=0, channel=0
[INIT] === Calling InitDevice ===

========================================
InitDevice START
========================================
[Phase 1] Loading library...
[Phase 1] SUCCESS
[Phase 2] Loading functions...
[Phase 2] SUCCESS (loaded 32 functions)
[Phase 3] Hardware initialization...
[Phase 3] SUCCESS
[Phase 4] Allocating memory...
[Phase 4] SUCCESS
[Phase 5] Creating thread...
[Phase 5] SUCCESS (TID=1234567890)
[Phase 6] I/O interrupt init...
[Phase 6] SUCCESS
========================================
InitDevice COMPLETE
========================================

[INIT] === InitDevice returned 0 ===
[INIT] Record iLinac_007:BPM14And15:RFIn_01_Amp initialized successfully
[INIT] Initializing record: iLinac_007:BPM14And15:RFIn_02_Amp
[INIT]   INP string: @AMP:0 ch=1
[INIT]   Parsed: type=AMP, offset=0, channel=1
[INIT] Record iLinac_007:BPM14And15:RFIn_02_Amp initialized successfully
...
iocInit completed successfully
epics>
```

### 8.4 常见初始化错误

#### 错误1: dlopen失败

```
[Phase 1] FAILED: libBPMboard14And15.so: cannot open shared object file
```

**原因**: 库文件找不到

**解决方案**:
```bash
# 检查库文件存在
ls -l /opt/BPMIOC/lib/libBPMboard14And15.so

# 设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/BPMIOC/lib:$LD_LIBRARY_PATH

# 或在st.cmd中设置
epicsEnvSet("LD_LIBRARY_PATH", "/opt/BPMIOC/lib")
```

#### 错误2: dlsym失败

```
[Phase 2] Failed to load GetRfInfo: undefined symbol: GetRfInfo
```

**原因**: 函数名不匹配（可能是C++符号修饰）

**解决方案**:
```bash
# 查看库中的符号
nm -D libBPMboard14And15.so | grep GetRfInfo

# 如果看到:
# 00001234 T _Z9GetRfInfoPfS_S_...  (C++修饰)
# 需要在硬件库中用extern "C"包裹:
extern "C" {
    int GetRfInfo(...);
}
```

#### 错误3: funcSystemInit失败

```
[Phase 3] FAILED: -1
```

**原因**: 硬件初始化失败（板卡未连接、VME总线错误等）

**解决方案**:
- 检查硬件连接
- 查看硬件库的日志
- 运行硬件自检程序

#### 错误4: pthread_create失败

```
[Phase 5] FAILED: 11 (Resource temporarily unavailable)
```

**原因**: 系统资源不足

**解决方案**:
```bash
# 检查系统限制
ulimit -a

# 增加线程数限制
ulimit -u 4096
```

## ✅ 学习检查点

完成本节后，你应该能够：

### 流程理解
- [ ] 能画出从./st.cmd到IOC就绪的完整流程
- [ ] 理解iocInit何时被调用
- [ ] 理解InitDevice何时被调用（第一个记录初始化时）
- [ ] 知道为什么InitDevice只调用一次

### 技术细节
- [ ] 理解dlopen/dlsym的作用
- [ ] 理解全局数组何时分配
- [ ] 理解DataAcquisitionThread何时创建
- [ ] 理解scanIoInit的作用

### 调试能力
- [ ] 能够添加调试输出追踪初始化
- [ ] 能够定位初始化失败的阶段
- [ ] 能够解决常见的初始化错误

## 🚀 下一步

现在你已经理解了初始化序列，接下来：

👉 [04-memory-model.md](./04-memory-model.md) - 深入理解内存模型和全局变量

或者深入其他主题：
- [06-thread-model.md](./06-thread-model.md) - 线程模型和同步
- [Part 4: Driver Layer](../part4-driver-layer/) - 驱动层实现细节

---

**💡 思考题**:

1. 为什么InitDevice使用静态标志而不是全局标志？
2. 如果第50个记录初始化失败，会发生什么？
3. DataAcquisitionThread在什么时候第一次调用funcGetRfInfo？
4. 如果dlopen成功但dlsym失败，需要清理什么资源？

**⏱️ 实践建议**: 在st.cmd中添加-d参数增加调试输出，观察完整的初始化流程。

```bash
./st.cmd -d 3  # 调试级别3
```
