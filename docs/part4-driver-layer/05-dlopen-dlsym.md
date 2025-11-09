# 动态库加载机制详解 (dlopen/dlsym)

> **阅读时间**: 50分钟
> **难度**: ⭐⭐⭐⭐⭐
> **前置知识**: C语言函数指针、Linux动态库、编译链接

## 📋 本文目标

- 理解为什么BPMIOC使用动态加载
- 掌握dlopen/dlsym的工作原理
- 学会如何添加新的硬件函数
- 理解硬件抽象层的设计模式

## 🎯 为什么需要动态加载？

### 传统静态链接的问题

```c
// ❌ 传统方式：编译时链接
#include "hardware_api.h"

void InitDevice()
{
    // 直接调用硬件函数
    SystemInit();
    SetupADC();
    // ...
}
```

**问题**:
1. **编译时依赖**: 必须有硬件库才能编译
2. **无法切换**: 无法在真实硬件和模拟器间切换
3. **难以测试**: 没有硬件就无法开发
4. **平台绑定**: x86和ARM需要不同的二进制

### BPMIOC的解决方案：动态加载

```c
// ✅ BPMIOC方式：运行时加载
void *handle = dlopen("libhardware.so", RTLD_LAZY);
int (*SystemInit)(void) = dlsym(handle, "SystemInit");

// 运行时决定加载哪个库
// PC: libhardware_mock.so (模拟器)
// ZYNQ: libhardware_real.so (真实硬件)
```

**优势**:
1. ✅ **解耦合**: 驱动层不依赖具体硬件实现
2. ✅ **灵活切换**: 通过配置选择硬件或模拟器
3. ✅ **便于开发**: PC上可以使用模拟器开发
4. ✅ **易于测试**: 可以mock所有硬件函数

## 🏗️ 动态加载架构

### 整体架构

```
┌─────────────────────────────────────────┐
│         driverWrapper.c (驱动层)         │
│  ┌────────────────────────────────────┐ │
│  │  硬件函数指针 (50+)                 │ │
│  │  ├─ funcSystemInit                 │ │
│  │  ├─ funcGetRFInfo                  │ │
│  │  └─ funcGetXYPosition              │ │
│  └────────────────────────────────────┘ │
│              ↓ dlsym                     │
│  ┌────────────────────────────────────┐ │
│  │  动态库句柄 handle                  │ │
│  └────────────────────────────────────┘ │
│              ↓ dlopen                    │
└─────────────────────────────────────────┘
                 ↓
    ┌────────────┴────────────┐
    │                          │
    ↓                          ↓
┌─────────────┐        ┌──────────────┐
│ Mock库 (PC)  │        │ Real库 (ZYNQ)│
│ libbpm_mock  │        │ libbpm_zynq  │
└─────────────┘        └──────────────┘
```

## 1. dlopen() - 加载动态库

### 1.1 函数原型

```c
#include <dlfcn.h>

void *dlopen(const char *filename, int flag);
```

**参数**:
- `filename`: 库文件路径
- `flag`: 加载模式

**返回值**:
- 成功: 库句柄
- 失败: NULL

### 1.2 BPMIOC中的使用

```c
// driverWrapper.c line 360-380

long InitDevice()
{
    const char *dll_filename;

    #ifdef SIMULATION_MODE
        dll_filename = "./libbpm_mock.so";   // 模拟器
    #else
        dll_filename = "./libbpm_zynq.so";   // 真实硬件
    #endif

    // 加载动态库
    handle = dlopen(dll_filename, RTLD_LAZY);

    if (handle == NULL) {
        fprintf(stderr, "Cannot load library: %s\n", dlerror());
        return -1;
    }

    printf("Loaded library: %s\n", dll_filename);

    return 0;
}
```

### 1.3 加载模式详解

#### RTLD_LAZY (延迟绑定)

```c
handle = dlopen("lib.so", RTLD_LAZY);
```

**特点**:
- 只在函数**第一次被调用**时解析符号
- 加载速度快
- 如果函数不存在，直到调用时才报错

**适用场景**: BPMIOC（50+函数，不是全部都会用到）

#### RTLD_NOW (立即绑定)

```c
handle = dlopen("lib.so", RTLD_NOW);
```

**特点**:
- dlopen时立即解析**所有符号**
- 加载速度慢
- 如果有函数不存在，立即报错

**适用场景**: 严格的生产环境

#### RTLD_GLOBAL (全局符号)

```c
handle = dlopen("lib.so", RTLD_LAZY | RTLD_GLOBAL);
```

**特点**:
- 库中的符号对后续加载的库可见
- 可能导致符号冲突

**BPMIOC不使用**: 避免符号污染

### 1.4 错误处理

```c
handle = dlopen(dll_filename, RTLD_LAZY);

if (handle == NULL) {
    // dlerror()返回错误描述
    const char *error = dlerror();
    fprintf(stderr, "dlopen error: %s\n", error);

    // 常见错误:
    // - "cannot open shared object file: No such file or directory"
    //   → 文件不存在或路径错误
    // - "undefined symbol: xxx"
    //   → 库依赖其他库但未找到

    return -1;
}
```

## 2. dlsym() - 获取函数指针

### 2.1 函数原型

```c
void *dlsym(void *handle, const char *symbol);
```

**参数**:
- `handle`: dlopen返回的句柄
- `symbol`: 函数名（字符串）

**返回值**:
- 成功: 函数指针
- 失败: NULL

### 2.2 BPMIOC中的使用

```c
// driverWrapper.c line 381-480

// 1. 声明函数指针
static int (*funcSystemInit)(void);
static void (*funcSystemClose)(void);
static float (*funcGetRFInfo)(int channel, int type);

// 2. 在InitDevice()中获取函数指针
long InitDevice()
{
    // ... dlopen ...

    // 系统初始化函数
    funcSystemInit = (int (*)(void))dlsym(handle, "SystemInit");
    if (funcSystemInit == NULL) {
        fprintf(stderr, "Cannot find symbol SystemInit: %s\n", dlerror());
        return -1;
    }

    // 系统关闭函数
    funcSystemClose = (void (*)(void))dlsym(handle, "SystemClose");
    if (funcSystemClose == NULL) {
        fprintf(stderr, "Cannot find symbol SystemClose: %s\n", dlerror());
        return -1;
    }

    // RF信息函数
    funcGetRFInfo = (float (*)(int, int))dlsym(handle, "GetRFInfo");
    if (funcGetRFInfo == NULL) {
        fprintf(stderr, "Cannot find symbol GetRFInfo: %s\n", dlerror());
        return -1;
    }

    // ... 获取其他50+函数 ...

    return 0;
}
```

### 2.3 类型转换详解

```c
funcGetRFInfo = (float (*)(int, int))dlsym(handle, "GetRFInfo");
                └────────┬────────┘
                    类型转换
```

**分解**:
```c
// dlsym返回void*
void *raw_ptr = dlsym(handle, "GetRFInfo");

// 函数原型: float GetRFInfo(int channel, int type)
// 函数指针类型: float (*)(int, int)

// 类型转换
funcGetRFInfo = (float (*)(int, int))raw_ptr;
```

**为什么需要类型转换？**
- dlsym返回通用指针`void*`
- 需要转换为具体的函数指针类型
- 否则编译器不知道如何调用

### 2.4 错误处理

```c
// ⚠️ 陷阱：dlsym可能返回NULL但不是错误
void *ptr = dlsym(handle, "optional_function");
if (ptr == NULL) {
    const char *error = dlerror();
    if (error != NULL) {
        // 真正的错误
        fprintf(stderr, "dlsym error: %s\n", error);
    } else {
        // 函数就是NULL（极少见）
        printf("Function is NULL but no error\n");
    }
}
```

**最佳实践**:
```c
// 清除之前的错误
dlerror();

void *ptr = dlsym(handle, "GetRFInfo");

// 检查错误
const char *error = dlerror();
if (error != NULL) {
    fprintf(stderr, "dlsym error: %s\n", error);
    return -1;
}
```

## 3. BPMIOC中的50+硬件函数

### 3.1 函数分类

```c
// ===== 1. 系统管理 (5个) =====
static int (*funcSystemInit)(void);
static void (*funcSystemClose)(void);
static int (*funcGetSystemStatus)(void);
static void (*funcResetSystem)(void);
static const char* (*funcGetVersion)(void);

// ===== 2. 数据采集 (10个) =====
static int (*funcTriggerAllDataReached)(void);
static void (*funcGetAllWaveData)(void);
static void (*funcStartAcquisition)(void);
static void (*funcStopAcquisition)(void);
static int (*funcIsDataReady)(void);
// ...

// ===== 3. RF信息 (8个) =====
static float (*funcGetRFInfo)(int channel, int type);
static float (*funcGetRF3Amp)(void);
static float (*funcGetRF3Phase)(void);
static float (*funcGetRF4Amp)(void);
// ...

// ===== 4. XY位置 (6个) =====
static float (*funcGetXYPosition)(int channel);
static float (*funcGetX1)(void);
static float (*funcGetY1)(void);
static float (*funcGetX2)(void);
static float (*funcGetY2)(void);
static void (*funcGetXYPair)(float *x, float *y);

// ===== 5. Button信号 (8个) =====
static float (*funcGetButtonSignal)(int index);
static float (*funcGetButton1)(void);
static float (*funcGetButton2)(void);
// ...

// ===== 6. 波形数据 (10个) =====
static int (*funcGetRFWaveData)(int channel, int index, int type);
static int (*funcGetXYWaveData)(int channel, int index);
static int (*funcGetButtonWaveData)(int button, int index);
// ...

// ===== 7. 寄存器操作 (5个) =====
static void (*funcSetReg)(int addr, int value);
static int (*funcGetReg)(int addr);
static void (*funcSetRegBit)(int addr, int bit, int value);
static int (*funcGetRegBit)(int addr, int bit);
static void (*funcResetReg)(int addr);
```

### 3.2 dlsym加载循环

```c
// 简化版本
typedef struct {
    void **func_ptr;     // 函数指针的地址
    const char *name;    // 符号名称
} FunctionMap;

FunctionMap function_table[] = {
    {(void**)&funcSystemInit, "SystemInit"},
    {(void**)&funcSystemClose, "SystemClose"},
    {(void**)&funcGetRFInfo, "GetRFInfo"},
    // ... 共50+项
};

long InitDevice()
{
    // ... dlopen ...

    // 循环加载所有函数
    for (int i = 0; i < sizeof(function_table)/sizeof(FunctionMap); i++) {
        *function_table[i].func_ptr = dlsym(handle, function_table[i].name);

        if (*function_table[i].func_ptr == NULL) {
            fprintf(stderr, "Cannot find symbol %s: %s\n",
                    function_table[i].name, dlerror());
            return -1;
        }
    }

    return 0;
}
```

**实际BPMIOC代码**: 没有使用循环，而是逐个调用dlsym（代码冗长但清晰）

## 4. 硬件库实现

### 4.1 Mock库实现 (PC模拟器)

```c
// libbpm_mock.c

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// 系统初始化
int SystemInit(void)
{
    printf("[Mock] SystemInit called\n");
    return 0;
}

// RF信息
float GetRFInfo(int channel, int type)
{
    // 生成模拟数据
    if (type == 0) {  // 幅度
        return 100.0 + rand() % 10;  // 100-110随机值
    } else {  // 相位
        return (rand() % 360) - 180;  // -180到180度
    }
}

// XY位置
float GetXYPosition(int channel)
{
    // 正弦波模拟
    static float t = 0;
    t += 0.01;
    return sin(t) * 10.0;  // ±10mm
}

// ... 实现其他50+函数 ...
```

**编译**:
```bash
gcc -shared -fPIC libbpm_mock.c -o libbpm_mock.so -lm
```

### 4.2 Real库实现 (ZYNQ硬件)

```c
// libbpm_zynq.c

#include <stdio.h>
#include "xil_io.h"  // Xilinx I/O库

// 寄存器地址
#define FPGA_BASE_ADDR 0x43C00000
#define REG_RF3_AMP    (FPGA_BASE_ADDR + 0x00)
#define REG_RF3_PHASE  (FPGA_BASE_ADDR + 0x04)

// 系统初始化
int SystemInit(void)
{
    printf("[ZYNQ] Initializing hardware...\n");

    // 初始化FPGA寄存器
    Xil_Out32(FPGA_BASE_ADDR, 0x00000001);  // 启动FPGA

    return 0;
}

// RF信息
float GetRFInfo(int channel, int type)
{
    uint32_t reg_addr;
    uint32_t raw_value;

    // 计算寄存器地址
    if (channel == 3 && type == 0) {
        reg_addr = REG_RF3_AMP;
    } else if (channel == 3 && type == 1) {
        reg_addr = REG_RF3_PHASE;
    }
    // ... 其他channel

    // 读取FPGA寄存器
    raw_value = Xil_In32(reg_addr);

    // 转换为物理单位
    float value = (float)raw_value * 0.001;  // 假设比例因子

    return value;
}

// ... 实现其他50+函数 ...
```

**编译**:
```bash
arm-linux-gnueabihf-gcc -shared -fPIC libbpm_zynq.c -o libbpm_zynq.so -lxil
```

### 4.3 两个库的API一致性

```
Mock库                    Real库
  ↓                         ↓
funcSystemInit()        funcSystemInit()
funcGetRFInfo()         funcGetRFInfo()
funcGetXYPosition()     funcGetXYPosition()
  |                         |
  └────── 相同API ──────────┘
            ↓
    driverWrapper.c
    (无需修改代码)
```

**关键**: 两个库必须导出**完全相同**的函数名和函数签名

## 5. 运行时库选择

### 5.1 编译时选择 (#ifdef)

```c
// driverWrapper.c

long InitDevice()
{
    const char *dll_filename;

    #ifdef SIMULATION_MODE
        dll_filename = "./libbpm_mock.so";
    #else
        dll_filename = "./libbpm_zynq.so";
    #endif

    handle = dlopen(dll_filename, RTLD_LAZY);
    // ...
}
```

**Makefile**:
```makefile
# PC模拟模式
ifeq ($(SIMULATION), YES)
    USR_CFLAGS += -DSIMULATION_MODE
endif
```

**编译**:
```bash
# PC模拟模式
make SIMULATION=YES

# ZYNQ真实模式
make SIMULATION=NO
```

### 5.2 运行时选择 (环境变量)

```c
long InitDevice()
{
    const char *dll_filename;

    // 从环境变量读取
    const char *mode = getenv("BPM_MODE");

    if (mode != NULL && strcmp(mode, "REAL") == 0) {
        dll_filename = "./libbpm_zynq.so";
    } else {
        dll_filename = "./libbpm_mock.so";  // 默认模拟
    }

    handle = dlopen(dll_filename, RTLD_LAZY);
    // ...
}
```

**使用**:
```bash
# 模拟模式
export BPM_MODE=MOCK
./st.cmd

# 真实模式
export BPM_MODE=REAL
./st.cmd
```

### 5.3 配置文件选择 (推荐)

```c
// 读取配置文件
long InitDevice()
{
    const char *dll_filename;
    FILE *config = fopen("bpm_config.txt", "r");

    if (config != NULL) {
        char mode[32];
        fscanf(config, "MODE=%s", mode);
        fclose(config);

        if (strcmp(mode, "REAL") == 0) {
            dll_filename = "./libbpm_zynq.so";
        } else {
            dll_filename = "./libbpm_mock.so";
        }
    } else {
        dll_filename = "./libbpm_mock.so";  // 默认
    }

    handle = dlopen(dll_filename, RTLD_LAZY);
    // ...
}
```

**bpm_config.txt**:
```
MODE=MOCK
```

**优势**: 无需重新编译或设置环境变量

## 6. 如何添加新的硬件函数

### 示例：添加GetTemperature()

#### Step 1: 在硬件库中实现

```c
// libbpm_mock.c
float GetTemperature(void)
{
    return 25.0 + (rand() % 10) * 0.1;  // 25.0-26.0℃
}

// libbpm_zynq.c
float GetTemperature(void)
{
    uint32_t raw = Xil_In32(REG_TEMPERATURE);
    return (float)raw * 0.01;  // 转换为℃
}
```

#### Step 2: 在driverWrapper.c中声明函数指针

```c
// driverWrapper.c 全局变量区域
static float (*funcGetTemperature)(void);
```

#### Step 3: 在InitDevice()中加载

```c
long InitDevice()
{
    // ... dlopen ...

    // ... 其他dlsym ...

    // 新增
    funcGetTemperature = (float (*)(void))dlsym(handle, "GetTemperature");
    if (funcGetTemperature == NULL) {
        fprintf(stderr, "Cannot find symbol GetTemperature: %s\n", dlerror());
        return -1;
    }

    // ...
}
```

#### Step 4: 在ReadData()中使用

```c
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        // ... 现有case ...

        case 29:  // 温度
            return funcGetTemperature();

        default:
            return 0.0;
    }
}
```

#### Step 5: 重新编译硬件库和IOC

```bash
# 编译Mock库
gcc -shared -fPIC libbpm_mock.c -o libbpm_mock.so

# 编译Real库
arm-linux-gnueabihf-gcc -shared -fPIC libbpm_zynq.c -o libbpm_zynq.so

# 编译IOC
cd ~/BPMIOC
make
```

## 7. 调试技巧

### 7.1 检查库是否加载成功

```c
if (handle == NULL) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
} else {
    printf("Library loaded successfully: handle=%p\n", handle);
}
```

### 7.2 检查函数是否找到

```c
funcGetRFInfo = (float (*)(int, int))dlsym(handle, "GetRFInfo");
if (funcGetRFInfo == NULL) {
    fprintf(stderr, "dlsym failed: %s\n", dlerror());
} else {
    printf("Function found: funcGetRFInfo=%p\n", funcGetRFInfo);
}
```

### 7.3 使用nm查看库符号

```bash
# 查看库中导出的符号
nm -D libbpm_mock.so | grep GetRFInfo

# 输出示例:
# 0000000000001234 T GetRFInfo
#                  ↑
#                  T = Text (函数代码)
```

### 7.4 使用ldd查看库依赖

```bash
ldd libbpm_zynq.so

# 输出示例:
# libxil.so => /usr/lib/libxil.so (0x00007f...)
# libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f...)
```

## ❓ 常见问题

### Q1: dlopen找不到库文件？
**A**:
```bash
# 检查文件是否存在
ls -l ./libbpm_mock.so

# 检查LD_LIBRARY_PATH
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

# 或使用绝对路径
handle = dlopen("/full/path/to/libbpm_mock.so", RTLD_LAZY);
```

### Q2: dlsym找不到符号？
**A**:
```bash
# 检查符号是否导出
nm -D libbpm_mock.so | grep SystemInit

# 如果没有，可能需要添加:
// libbpm_mock.c
__attribute__((visibility("default"))) int SystemInit(void);
```

### Q3: 编译时没问题，运行时崩溃？
**A**:
- 检查函数签名是否匹配
- 检查是否所有函数都正确加载
- 使用gdb调试:
```bash
gdb ./st.cmd
(gdb) run
(gdb) bt  # 查看崩溃时的调用栈
```

### Q4: 如何在PC和ZYNQ间共享代码？
**A**:
- 驱动层代码(driverWrapper.c)完全相同
- 只需准备两个硬件库
- 通过配置选择加载哪个库

## 📊 性能影响

### 动态调用 vs 静态调用

```c
// 静态调用
float value = GetRFInfo(3, 0);
// → 1次函数调用 (直接跳转)

// 动态调用
float value = funcGetRFInfo(3, 0);
// → 1次指针解引用 + 1次函数调用
```

**性能开销**:
- 额外1次内存访问 (~1ns)
- 对于BPMIOC几乎可忽略 (函数本身耗时>>1ns)

## 📚 延伸阅读

- [04-initdevice.md](./04-initdevice.md) - InitDevice()完整实现
- [10-hardware-functions.md](./10-hardware-functions.md) - 硬件函数详解
- `man dlopen` - Linux手册

## 🎓 本章总结

- ✅ dlopen/dlsym实现运行时动态加载
- ✅ 支持Mock库和Real库无缝切换
- ✅ 50+硬件函数通过函数指针调用
- ✅ 硬件抽象层的经典设计模式
- ✅ 添加新函数需要4个步骤

**核心思想**: 接口抽象 + 运行时绑定 = 灵活性

**下一步**: 阅读 [06-pthread.md](./06-pthread.md) 学习数据采集线程

---

**实验任务**: 在Mock库中添加一个新函数，并在驱动层调用它
