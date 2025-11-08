# 10 - EPICS IOC开发的C语言基础

> **目标**: 掌握EPICS IOC开发所需的C语言知识
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 50分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 理解BPMIOC中使用的C语言特性
- ✅ 掌握指针和结构体的使用
- ✅ 理解函数指针和回调
- ✅ 使用EPICS提供的数据类型和API
- ✅ 能够阅读和修改BPMIOC源码

## 🎯 1. 为什么需要C语言

### EPICS使用C的原因

- **性能**: 实时系统需要高效执行
- **硬件访问**: 直接内存操作
- **跨平台**: C编译器无处不在
- **历史**: EPICS始于1980年代

### 你需要的C知识层次

```
基础 ────────── 能读懂代码，做简单修改
  ↓
中级 ────────── 能添加新功能
  ↓
高级 ────────── 能设计新的设备支持或驱动
```

本节覆盖：**基础到中级**

## 📚 2. 基础数据类型

### EPICS中常用的类型

| C类型 | EPICS类型 | 大小 | 用途 | BPMIOC示例 |
|-------|----------|------|------|-----------|
| `int` | `int` | 4字节 | 整数 | channel编号 |
| `long` | `long` | 4/8字节 | 长整数 | 计数器 |
| `float` | `epicsFloat32` | 4字节 | 浮点数 | RF幅度值 |
| `double` | `epicsFloat64` | 8字节 | 双精度 | 高精度计算 |
| `char` | `char` | 1字节 | 字符 | 字符串构建 |
| `char*` | `char*` | 指针 | 字符串 | PV名称 |

### BPMIOC中的示例

```c
// driverWrapper.c

static float Amp[10];           // 幅度数组
static float Phase[10];         // 相位数组
static int use_simulation = 0;  // 模拟模式标志
static double scan_period = 0.1; // 扫描周期
```

## 🔗 3. 指针 (重要!)

### 什么是指针

指针存储内存地址：

```c
int value = 42;       // 普通变量
int *ptr = &value;    // ptr指向value的地址

printf("%d\n", value);   // 输出: 42
printf("%d\n", *ptr);    // 输出: 42 (解引用)
printf("%p\n", ptr);     // 输出: 0x7fff... (地址)
```

### BPMIOC中的指针使用

#### 示例1: 设备私有数据

```c
// devBPMMonitor.c

typedef struct {
    int offset;
    int channel;
    char type_str[16];
} DevPvt;

static long init_ai_record(aiRecord *prec)
{
    DevPvt *pPvt = malloc(sizeof(DevPvt));  // 分配内存

    if (!pPvt) return S_dev_noMemory;

    // 填充数据
    pPvt->offset = 0;
    pPvt->channel = 0;

    prec->dpvt = pPvt;  // 保存指针到记录
    return 0;
}

static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;  // 取回指针

    float value = ReadData(pPvt->offset,   // 使用->访问成员
                           pPvt->channel,
                           pPvt->type_str);
    prec->val = value;
    return 0;
}
```

**要点**:
- `malloc()`: 动态分配内存
- `->`: 通过指针访问结构体成员
- 类型转换: `(DevPvt *)` 将void*转为DevPvt*

#### 示例2: 函数指针

```c
// driverWrapper.c

// 定义函数指针类型
typedef void (*RfInfoFunc)(int ch_N, float* Amp, float* Phase);

static RfInfoFunc getRfInfoFunc = NULL;  // 函数指针变量

int InitDevice()
{
    // 从动态库加载函数
    getRfInfoFunc = (RfInfoFunc)dlsym(handle, "getRfInfo");

    if (!getRfInfoFunc) {
        printf("Failed to load getRfInfo\n");
        return -1;
    }

    return 0;
}

static void my_thread(void *arg)
{
    while (1) {
        // 通过函数指针调用
        for (int i = 0; i < 8; i++) {
            (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
        }
        // ...
    }
}
```

## 📦 4. 结构体

### 定义和使用

```c
// 定义结构体
struct Point {
    float x;
    float y;
};

// 使用typedef简化
typedef struct {
    float x;
    float y;
} Point;

// 创建和使用
Point p;
p.x = 1.5;
p.y = 2.3;

Point *ptr = &p;
ptr->x = 3.0;  // 通过指针访问
```

### BPMIOC中的结构体

```c
// devBPMMonitor.c

typedef struct {
    int offset;
    int channel;
    char type_str[16];
} DevPvt;

// EPICS记录也是结构体
typedef struct aiRecord {
    char name[61];
    epicsFloat64 val;
    epicsEnum16 sevr;
    // ... 100多个字段
} aiRecord;
```

## 🔢 5. 数组

### 静态数组

```c
// driverWrapper.c

static float Amp[10];     // 10个浮点数
static float Phase[10];   // 10个浮点数

// 访问
Amp[0] = 1.234;
Amp[1] = 2.456;

// 遍历
for (int i = 0; i < 10; i++) {
    printf("Amp[%d] = %.3f\n", i, Amp[i]);
}
```

### 动态数组（使用malloc）

```c
// 分配100个float的数组
float *waveform = (float *)malloc(100 * sizeof(float));

if (!waveform) {
    printf("Memory allocation failed\n");
    return -1;
}

// 使用
for (int i = 0; i < 100; i++) {
    waveform[i] = i * 0.1;
}

// 释放
free(waveform);
```

### BPMIOC波形示例

```c
// devBPMMonitor.c - 读取波形

static long read_waveform(waveformRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    unsigned int nelem = prec->nelm;  // 最大元素数
    float *data = (float *)prec->bptr;  // 数据缓冲区

    // 从驱动层读取
    readWaveform(pPvt->offset, pPvt->channel, nelem, data,
                 &tai_s, &tai_ns);

    prec->nord = nelem;  // 实际读取的元素数
    return 0;
}
```

## 🔄 6. 字符串处理

### C字符串基础

```c
// C中字符串是字符数组，以'\0'结尾
char str[16] = "Hello";  // ['H','e','l','l','o','\0',...]

// 字符串操作
char dest[32];
strcpy(dest, "Hello");          // 复制
strcat(dest, " World");         // 连接，dest现在是"Hello World"
int len = strlen(dest);         // 长度：11
int cmp = strcmp(dest, "Hi");   // 比较：<0, 0, >0
```

### BPMIOC中的字符串处理

```c
// devBPMMonitor.c

static long init_ai_record(aiRecord *prec)
{
    DevPvt *pPvt = malloc(sizeof(DevPvt));

    char *pchar = prec->inp.value.instio.string;  // "@AMP:0 ch=0"

    // 跳过@
    pchar++;

    // 查找冒号
    char *type_end = strchr(pchar, ':');
    if (type_end) {
        int type_len = type_end - pchar;
        strncpy(pPvt->type_str, pchar, type_len);  // 复制"AMP"
        pPvt->type_str[type_len] = '\0';  // 添加终止符
    }

    // 比较类型
    if (strcmp(pPvt->type_str, "AMP") == 0) {
        // 处理AMP类型
    }

    return 0;
}
```

### 字符串函数速查

| 函数 | 功能 | 示例 |
|------|------|------|
| `strcpy(dest, src)` | 复制字符串 | `strcpy(buf, "text")` |
| `strncpy(dest, src, n)` | 复制最多n个字符 | `strncpy(buf, s, 10)` |
| `strcat(dest, src)` | 连接字符串 | `strcat(buf, " more")` |
| `strlen(str)` | 字符串长度 | `len = strlen(str)` |
| `strcmp(s1, s2)` | 比较字符串 | `if (strcmp(a,b)==0)` |
| `strchr(str, ch)` | 查找字符 | `p = strchr(str, ':')` |
| `strstr(s1, s2)` | 查找子串 | `p = strstr(s, "ch=")` |
| `sprintf(buf, fmt, ...)` | 格式化字符串 | `sprintf(b, "V=%d", v)` |

## ⚙️ 7. 预处理器

### 宏定义

```c
// driverWrapper.h

#define OFFSET_AMP    0
#define OFFSET_PHASE  0
#define OFFSET_REG    1

#define MAX_CHANNELS  10
```

### 条件编译

```c
#ifdef DEBUG
    printf("Debug: value = %d\n", value);
#endif

#ifndef DRIVER_WRAPPER_H
#define DRIVER_WRAPPER_H
// 头文件内容
#endif
```

### 宏函数

```c
// 简单宏
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 使用
int max_value = MAX(x, y);
```

### BPMIOC中的宏示例

```c
// 调试宏
#define DEBUG_INFO(fmt, ...)  if (debug_level >= 2) \
    printf("[INFO] %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

// 使用
DEBUG_INFO("Initializing device");
DEBUG_INFO("Channel %d, value %.3f", channel, value);
```

## 🧵 8. EPICS特定概念

### EPICS数据类型

```c
#include <epicsTypes.h>

epicsInt16 i16;       // 16位整数
epicsUInt16 u16;      // 16位无符号整数
epicsInt32 i32;       // 32位整数
epicsFloat32 f32;     // 32位浮点数
epicsFloat64 f64;     // 64位浮点数
```

### EPICS线程

```c
#include <epicsThread.h>

static void my_thread(void *arg)
{
    while (1) {
        // 线程工作
        epicsThreadSleep(0.1);  // 睡眠100ms
    }
}

// 创建线程
epicsThreadCreate("BPMMonitor",      // 名称
                  50,                 // 优先级(0-99)
                  20000,              // 栈大小
                  (EPICSTHREADFUNC)my_thread,  // 函数
                  NULL);              // 参数
```

### EPICS互斥锁

```c
#include <epicsMutex.h>

static epicsMutexId lock;

void init()
{
    lock = epicsMutexCreate();
}

void critical_section()
{
    epicsMutexLock(lock);  // 加锁

    // 临界区代码
    shared_data++;

    epicsMutexUnlock(lock);  // 解锁
}
```

## 🔧 9. 动态库加载

### dlopen/dlsym基础

```c
#include <dlfcn.h>

void *handle = dlopen("/path/to/library.so", RTLD_LAZY);
if (!handle) {
    printf("Error: %s\n", dlerror());
    return -1;
}

// 获取函数指针
void (*func)() = dlsym(handle, "function_name");
if (!func) {
    printf("Error: %s\n", dlerror());
    return -1;
}

// 调用函数
(*func)();

// 关闭库
dlclose(handle);
```

### BPMIOC中的使用

```c
// driverWrapper.c

static void *handle = NULL;
static RfInfoFunc getRfInfoFunc = NULL;

int InitDevice()
{
    handle = dlopen("/usr/lib/liblowlevel.so", RTLD_LAZY);

    if (!handle) {
        printf("WARNING: %s\n", dlerror());
        printf("WARNING: Using SIMULATION mode\n");
        use_simulation = 1;
        return 0;
    }

    // 加载函数
    getRfInfoFunc = (RfInfoFunc)dlsym(handle, "getRfInfo");
    if (!getRfInfoFunc) {
        printf("ERROR: %s\n", dlerror());
        return -1;
    }

    // 调用初始化
    InitFunc initFunc = (InitFunc)dlsym(handle, "Init");
    if (initFunc) {
        (*initFunc)();
    }

    return 0;
}
```

## 🎓 10. 常见模式

### 模式1: 错误检查

```c
// 检查指针是否为NULL
DevPvt *pPvt = malloc(sizeof(DevPvt));
if (!pPvt) {
    printf("ERROR: Memory allocation failed\n");
    return S_dev_noMemory;
}

// 检查函数返回值
int ret = SomeFunction();
if (ret != 0) {
    printf("ERROR: SomeFunction failed with code %d\n", ret);
    return ret;
}
```

### 模式2: 初始化模式

```c
static int initialized = 0;

int InitDevice()
{
    if (initialized) {
        printf("WARNING: Already initialized\n");
        return 0;
    }

    // 初始化代码
    // ...

    initialized = 1;
    return 0;
}
```

### 模式3: 单例模式（全局数据）

```c
// 驱动层通常使用静态全局变量
static float Amp[10];
static float Phase[10];
static IOSCANPVT ioScanPvt;

// 只有本文件中的函数能访问这些变量
```

## 📝 11. 代码阅读技巧

### 从main()或Init()开始

```c
// BPMmonitorApp/src/BPMmonitorMain.cpp

int main(int argc,char *argv[])
{
    // ...
    iocsh(NULL);  // 进入IOC shell
    return(0);
}
```

### 跟踪函数调用

```
iocInit()
    ↓
devInit()
    ↓
init_ai_record()  ← 你的设备支持初始化
    ↓
InitDevice()  ← 驱动层初始化
```

### 使用grep查找

```bash
# 查找函数定义
grep -rn "ReadData" BPMmonitorApp/src/

# 查找函数调用
grep -rn "ReadData(" BPMmonitorApp/src/

# 查找结构体定义
grep -rn "typedef struct" BPMmonitorApp/src/
```

## 🔗 相关文档

- [Part 4: Driver Layer](../../part4-driver-layer/) - 驱动层C代码详解
- [Part 5: Device Support Layer](../../part5-device-support-layer/) - 设备支持层C代码
- [C Programming Tutorial](https://www.cprogramming.com/)

## 📝 总结

### EPICS IOC开发的C语言核心

1. **指针**: 理解指针是关键（`*`, `&`, `->`）
2. **结构体**: 数据组织方式
3. **函数指针**: 回调和动态加载
4. **字符串**: C字符串处理
5. **EPICS API**: 线程、互斥锁、数据类型

### 学习路径

```
1. 阅读BPMIOC代码，理解现有模式
    ↓
2. 做小修改（添加调试输出）
    ↓
3. 添加简单功能（新的offset）
    ↓
4. 独立实现新的设备支持
```

### 推荐练习

1. **修改调试输出**: 在`ReadData()`中添加printf
2. **添加新offset**: 在驱动层添加新的寄存器读取
3. **创建新calc记录**: 在数据库中计算派生值

### 下一步

- [Part 4: Driver Layer](../../part4-driver-layer/) - 深入驱动层
- [Part 5: Device Support](../../part5-device-support-layer/) - 深入设备支持
- [Part 8: Labs](../part8-hands-on-labs/) - 实践练习

---

**🎉 恭喜！** 你已经掌握了EPICS IOC开发所需的C语言基础知识！现在可以开始深入BPMIOC的源码了！
