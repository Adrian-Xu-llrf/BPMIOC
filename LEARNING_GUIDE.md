# BPMIOC 学习指南

## 目标读者
本指南面向正在学习 C 语言和 EPICS IOC 开发的研究生,旨在帮助你通过实际项目 (BPMIOC) 系统学习 EPICS 开发。

---

## 学习目标

完成本指南后,你将能够:
1. ✅ 理解 EPICS 的核心概念和架构
2. ✅ 阅读和理解现有的 EPICS IOC 代码
3. ✅ 修改和扩展 EPICS IOC 功能
4. ✅ 调试 EPICS 应用程序
5. ✅ 开发新的设备支持和驱动
6. ✅ 理解加速器控制系统的基本原理

---

## 前置知识要求

### 必备基础
- ✅ C 语言基础 (变量、函数、控制流)
- ✅ Linux 基本命令 (cd, ls, cat, grep, make)
- ✅ 文本编辑器使用 (vim, nano, 或 VSCode)

### 建议掌握 (可边学边用)
- 🔸 C 语言进阶 (指针、结构体、动态内存)
- 🔸 Makefile 基础
- 🔸 Git 版本控制
- 🔸 基本的物理学概念 (电压、相位、频率)

---

## 学习路径图

```
┌─────────────────────────────────────────────────────────┐
│  第 1 阶段: 环境准备与 EPICS 基础 (第 1-2 周)           │
│  - 安装 EPICS Base                                      │
│  - 运行简单的 IOC 示例                                  │
│  - 学习 Channel Access 工具                             │
└─────────────────┬───────────────────────────────────────┘
                  ▼
┌─────────────────────────────────────────────────────────┐
│  第 2 阶段: 理解 BPMIOC 架构 (第 3-4 周)                │
│  - 代码结构分析                                         │
│  - 编译和运行 BPMIOC                                    │
│  - PV 访问实验                                          │
└─────────────────┬───────────────────────────────────────┘
                  ▼
┌─────────────────────────────────────────────────────────┐
│  第 3 阶段: 深入驱动层和设备支持 (第 5-6 周)            │
│  - 分析 driverWrapper.c                                 │
│  - 分析 devBPMMonitor.c                                 │
│  - 理解数据流                                           │
└─────────────────┬───────────────────────────────────────┘
                  ▼
┌─────────────────────────────────────────────────────────┐
│  第 4 阶段: 实践开发 (第 7-8 周)                        │
│  - 添加新功能                                           │
│  - 修改现有算法                                         │
│  - 编写测试代码                                         │
└─────────────────────────────────────────────────────────┘
```

---

## 第 1 阶段: 环境准备与 EPICS 基础

### 学习目标
- 建立 EPICS 开发环境
- 理解 EPICS 的基本概念
- 能够运行简单的 IOC 并访问 PV

### 1.1 安装 EPICS Base

#### 下载源码
```bash
cd ~
wget https://epics.anl.gov/download/base/base-3.15.6.tar.gz
tar -xzf base-3.15.6.tar.gz
cd base-3.15.6
```

#### 配置
```bash
# 编辑 configure/CONFIG_SITE
# 确保 CROSS_COMPILER_TARGET_ARCHS 为空 (仅编译本地架构)
```

#### 编译
```bash
make clean
make
```

#### 设置环境变量
```bash
# 添加到 ~/.bashrc
export EPICS_BASE=$HOME/base-3.15.6
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}

# 生效
source ~/.bashrc
```

#### 验证安装
```bash
which softIoc
# 应显示: /home/yourusername/base-3.15.6/bin/linux-x86_64/softIoc
```

### 1.2 运行第一个 IOC

#### 创建测试数据库
```bash
mkdir -p ~/epics_test
cd ~/epics_test
cat > test.db << 'EOF'
record(ai, "test:temperature") {
    field(DESC, "Temperature sensor")
    field(SCAN, "1 second")
    field(PREC, "2")
    field(EGU,  "C")
    field(HOPR, "100")
    field(LOPR, "0")
}

record(calc, "test:fahrenheit") {
    field(DESC, "Temperature in F")
    field(INPA, "test:temperature")
    field(CALC, "A*9/5+32")
    field(PREC, "2")
    field(EGU,  "F")
    field(SCAN, "1 second")
}

record(ao, "test:setpoint") {
    field(DESC, "Temperature setpoint")
    field(PREC, "2")
    field(EGU,  "C")
    field(DRVH, "100")
    field(DRVL, "0")
    field(VAL,  "25")
}
EOF
```

#### 创建启动脚本
```bash
cat > st.cmd << 'EOF'
#!/usr/bin/env softIoc

# 加载数据库
dbLoadRecords("test.db")

# 初始化 IOC
iocInit()

# 打印所有 PV
dbl
EOF

chmod +x st.cmd
```

#### 运行 IOC
```bash
./st.cmd
```

你应该看到:
```
Starting iocInit
... (初始化信息)
iocRun: All initialization complete
epics>
```

#### 在 IOC Shell 中实验

```bash
# 列出所有 PV
epics> dbl
test:temperature
test:fahrenheit
test:setpoint

# 查看记录详细信息
epics> dbpr "test:temperature"

# 读取值
epics> dbgf "test:temperature"
DBR_DOUBLE:          0

# 设置值
epics> dbpf "test:temperature" "23.5"
DBR_DOUBLE:          23.5

# 再次读取 (应显示 23.5)
epics> dbgf "test:temperature"

# 读取华氏温度 (应自动计算)
epics> dbgf "test:fahrenheit"
```

### 1.3 使用 Channel Access 工具

**打开新终端**,运行以下命令 (保持 IOC 运行):

#### caget - 读取 PV
```bash
caget test:temperature
# 输出: test:temperature 23.5
```

#### caput - 写入 PV
```bash
caput test:temperature 25.0
# 输出: Old : test:temperature 23.5
#       New : test:temperature 25.0
```

#### camonitor - 监控 PV
```bash
camonitor test:temperature test:fahrenheit
# 实时显示值的变化
# 按 Ctrl+C 停止
```

#### cainfo - 查看 PV 信息
```bash
cainfo test:temperature
```

### 1.4 EPICS 核心概念速查

| 概念 | 解释 | 类比 |
|------|------|------|
| **PV** (Process Variable) | EPICS 中的数据点 | 数据库中的一条记录 |
| **Record** | PV 的容器,包含值和处理逻辑 | 对象 (OOP) |
| **Field** | Record 的属性 | 对象的成员变量 |
| **IOC** | 运行 EPICS 数据库的程序 | 服务器程序 |
| **Channel Access** | 网络协议,用于访问 PV | HTTP/REST API |
| **Device Support** | 连接 Record 和硬件的代码 | 驱动程序 |
| **SCAN** | 何时处理 Record | 定时器/事件触发器 |

### 1.5 实验任务

#### 任务 1: 创建带计算的数据库
创建一个数据库,包含:
- 3 个 ai 记录 (x, y, z 坐标)
- 1 个 calc 记录计算距离: sqrt(x^2 + y^2 + z^2)

**提示**: calc 记录支持 12 个输入 (INPA-INPL) 和公式 (CALC)

<details>
<summary>点击查看答案</summary>

```
record(ai, "pos:x") {
    field(VAL, "3.0")
}

record(ai, "pos:y") {
    field(VAL, "4.0")
}

record(ai, "pos:z") {
    field(VAL, "0.0")
}

record(calc, "pos:distance") {
    field(INPA, "pos:x")
    field(INPB, "pos:y")
    field(INPC, "pos:z")
    field(CALC, "SQRT(A*A+B*B+C*C)")
    field(PREC, "3")
}
```
</details>

#### 任务 2: 自动扫描实验
修改上面的数据库,让 x 坐标每秒自增 1:

<details>
<summary>点击查看提示</summary>

使用 `calc` 记录的前向链接 (FLNK) 和自引用:
```
record(calc, "counter:x") {
    field(SCAN, "1 second")
    field(INPA, "counter:x")
    field(CALC, "A+1")
    field(VAL,  "0")
}
```
</details>

---

## 第 2 阶段: 理解 BPMIOC 架构

### 学习目标
- 能够编译和运行 BPMIOC
- 理解代码的目录结构
- 识别关键组件和数据流

### 2.1 代码结构导览

#### 使用 tree 命令查看结构
```bash
cd ~/BPMIOC
tree -L 2 -I 'O.*|*.d'  # 忽略编译输出
```

#### 关键文件清单

创建一个文件清单,记录每个文件的作用:

| 文件 | 行数 | 作用 | 重要度 |
|------|------|------|--------|
| `driverWrapper.c` | 1540 | 驱动层,硬件接口 | ⭐⭐⭐⭐⭐ |
| `driverWrapper.h` | ~100 | 驱动层头文件 | ⭐⭐⭐⭐⭐ |
| `devBPMMonitor.c` | 423 | 设备支持层 | ⭐⭐⭐⭐⭐ |
| `devBPMMonitor.h` | ~50 | 设备支持头文件 | ⭐⭐⭐⭐ |
| `BPMMonitor.db` | 1891 | 主数据库 | ⭐⭐⭐⭐⭐ |
| `BPMCal.db` | 155 | 校准数据库 | ⭐⭐⭐ |
| `st.cmd` | ~20 | 启动脚本 | ⭐⭐⭐⭐ |
| `Makefile (各层)` | ~30 | 构建规则 | ⭐⭐⭐ |

### 2.2 编译 BPMIOC

#### 配置 EPICS_BASE 路径
```bash
cd ~/BPMIOC
vim configure/RELEASE

# 修改为你的 EPICS Base 路径:
EPICS_BASE=/home/yourusername/base-3.15.6
```

#### 编译
```bash
make clean
make
```

**可能遇到的问题**:

**问题 1**: `liblowlevel.so` 找不到
```
解决: 这是正常的!这个库是硬件相关的,在开发环境中不存在。
IOC 在运行时通过 dlopen() 加载,如果找不到会有错误提示,但不影响编译。
```

**问题 2**: 编译器警告
```bash
# 警告通常不影响功能,但建议修复
# 常见警告: 未使用的变量、隐式转换等
```

#### 验证编译结果
```bash
ls -lh bin/*/BPMmonitor
ls -lh db/*.db
ls -lh dbd/*.dbd
```

### 2.3 阅读代码的策略

#### 自顶向下阅读法

**第 1 步: 启动脚本** → 理解初始化流程
```bash
less iocBoot/iocBPMmonitor/st.cmd
```

**第 2 步: 数据库** → 理解 PV 结构
```bash
less BPMmonitorApp/Db/BPMMonitor.db
# 搜索: /RF3Amp (查找 RF3 振幅相关记录)
```

**第 3 步: 设备支持** → 理解 Record ↔ Driver 映射
```bash
less BPMmonitorApp/src/devBPMMonitor.c
# 重点: init_ai_record(), read_ai()
```

**第 4 步: 驱动层** → 理解硬件交互
```bash
less BPMmonitorApp/src/driverWrapper.c
# 重点: InitDevice(), ReadData()
```

#### 自底向上追踪法

以 "RF3 振幅是如何读取的?" 为例:

**追踪路径**:
```
1. 数据库: BPMMonitor.db 中找到 RF3Amp 记录
   record(ai, "$(P):RF3Amp") {
       field(INP, "@AMP:0 ch=0")  ← 参数: AMP, offset=0, channel=0
   }

2. 设备支持: devBPMMonitor.c::init_ai_record()
   解析 "@AMP:0 ch=0" → type=TYPE_AMP, offset=0, channel=0

3. 设备支持: devBPMMonitor.c::read_ai()
   调用 ReadData(0, 0, TYPE_AMP)

4. 驱动层: driverWrapper.c::ReadData()
   case OFFSET_AMP: return Amp[channel];  ← 从全局数组读取

5. 驱动层: driverWrapper.c::my_thread()
   周期性调用 (*getRfInfoFunc)(i, &Amp[i], &Phase[i])  ← 从硬件读取

6. 硬件层: liblowlevel.so::GetRfInfo()
   (黑盒,由硬件厂商提供)
```

### 2.4 使用调试输出理解代码

#### 添加 printf 调试

在 `devBPMMonitor.c` 的 `init_ai_record()` 中添加:

```c
static long init_ai_record(struct aiRecord *prec)
{
    // ... 原有代码 ...

    // 添加调试输出
    printf("=== DEBUG: init_ai_record ===\n");
    printf("  Record Name: %s\n", prec->name);
    printf("  INP String:  %s\n", pinstio->string);
    printf("  Parsed -> type: %s, offset: %d, channel: %d\n",
           pPvt->type_str, pPvt->offset, pPvt->channel);
    printf("=============================\n");

    return 0;
}
```

重新编译后运行,你会看到每个 ai 记录初始化时的详细信息。

### 2.5 绘制你自己的架构图

**任务**: 用纸笔或工具 (draw.io, Excalidraw) 绘制:
1. BPMIOC 的三层架构图
2. RF3Amp 的数据流图
3. 波形平均电压计算的流程图

**目的**: 通过绘图加深理解,建立系统性思维。

### 2.6 实验任务

#### 任务 1: 找出所有 RF 通道的 PV 名称
使用 grep 命令:
```bash
grep "record(ai" BPMmonitorApp/Db/BPMMonitor.db | grep "RF"
```

统计有多少个 RF 相关的 ai 记录?

#### 任务 2: 追踪 X 位置的数据流
从 `BPMMonitor.db` 中找到 X 位置的记录,追踪到 `driverWrapper.c`,写出完整的数据流路径。

#### 任务 3: 理解波形记录
找到触发波形的记录定义,回答:
- 使用的设备类型是什么?
- NELM 字段的值是多少?
- FTVL 字段的含义是什么?

---

## 第 3 阶段: 深入驱动层和设备支持

### 学习目标
- 深入理解 C 语言高级特性 (函数指针、动态库)
- 掌握 EPICS 设备支持的编写方法
- 能够修改现有驱动代码

### 3.1 C 语言高级特性复习

#### 函数指针

**基础示例**:
```c
#include <stdio.h>

// 定义函数类型
typedef int (*MathFunc)(int, int);

// 实现函数
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

int main() {
    MathFunc func;

    func = add;
    printf("3 + 5 = %d\n", func(3, 5));  // 输出: 8

    func = multiply;
    printf("3 * 5 = %d\n", func(3, 5));  // 输出: 15

    return 0;
}
```

**BPMIOC 中的应用** (driverWrapper.c):
```c
// 定义函数指针类型
typedef int (*SystemInitFunc)(void);
typedef int (*GetRfInfoFunc)(int, double*, double*);

// 声明全局函数指针
SystemInitFunc systemInitFunc = NULL;
GetRfInfoFunc getRfInfoFunc = NULL;

// 动态加载
void LoadLibrary() {
    void *handle = dlopen("liblowlevel.so", RTLD_LAZY);

    // 获取函数地址
    *(void **)(&systemInitFunc) = dlsym(handle, "SystemInit");
    *(void **)(&getRfInfoFunc) = dlsym(handle, "GetRfInfo");
}

// 调用
void UseLibrary() {
    (*systemInitFunc)();  // 调用 SystemInit()

    double amp, phase;
    (*getRfInfoFunc)(0, &amp, &phase);  // 调用 GetRfInfo(0, ...)
}
```

**练习**: 编写一个程序,使用函数指针实现简单的计算器 (+, -, *, /)

#### 动态库加载

**创建动态库** (libmath.c):
```c
// libmath.c
#include <math.h>

double calculate_distance(double x, double y, double z) {
    return sqrt(x*x + y*y + z*z);
}
```

**编译为动态库**:
```bash
gcc -shared -fPIC -o libmath.so libmath.c -lm
```

**动态加载** (main.c):
```c
#include <stdio.h>
#include <dlfcn.h>

typedef double (*DistFunc)(double, double, double);

int main() {
    // 1. 打开库
    void *handle = dlopen("./libmath.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    // 2. 获取函数
    DistFunc func;
    *(void **)(&func) = dlsym(handle, "calculate_distance");
    if (!func) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    // 3. 调用
    double dist = (*func)(3.0, 4.0, 0.0);
    printf("Distance: %f\n", dist);  // 输出: 5.000000

    // 4. 关闭库
    dlclose(handle);
    return 0;
}
```

**编译并运行**:
```bash
gcc -o main main.c -ldl
./main
```

### 3.2 深入分析 driverWrapper.c

#### 关键数据结构

**全局缓冲区**:
```c
// driverWrapper.c

static double Amp[8];           // RF 振幅 (8 通道)
static double Phase[8];         // RF 相位 (8 通道)
static float TrigWaveform[8][10000];  // 触发波形
static double AVG_Voltage[8];   // 平均电压
static int AVGStart[8];         // 平均起始位置
static int AVGStop[8];          // 平均结束位置
// ... 更多缓冲区
```

**为什么用全局变量?**
- 后台线程持续更新 → 设备支持层读取
- 避免频繁的硬件访问
- 多个 Record 共享数据

#### 后台线程详解

```c
static void my_thread(void *arg)
{
    while (1) {
        // 1. 读取所有 RF 通道
        for (int i = 0; i < 8; i++) {
            (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
        }

        // 2. 读取时间戳
        (*getTimeStampFunc)(&TAIsec, &TAInsec);

        // 3. 触发 I/O 中断 (所有 I/O Intr 记录会被扫描)
        scanIoRequest(ioScanPvt);

        // 4. 休眠 100ms
        epicsThreadSleep(0.1);
    }
}
```

**关键函数**: `scanIoRequest(ioScanPvt)`
- 通知 EPICS 扫描器: "数据已更新,请扫描相关记录"
- 所有 `SCAN: I/O Intr` 且使用该 `ioScanPvt` 的记录会被处理

#### ReadData() 函数剖析

```c
double ReadData(int offset, int channel, int type)
{
    double value = 0.0;

    switch(offset) {
        case OFFSET_AMP:  // 0
            value = Amp[channel];
            break;

        case OFFSET_PHASE:  // 1
            value = Phase[channel];
            break;

        case OFFSET_POWER:  // 17
            // 功率转换 (查表)
            value = amp2power(Amp[channel], channel);
            break;

        case OFFSET_AVG_VOLTAGE:  // 34
            value = AVG_Voltage[channel];
            break;

        // ... 30+ 个 case
    }

    return value;
}
```

**设计模式**: 通过 offset 统一接口,避免为每种数据类型写单独的函数。

### 3.3 深入分析 devBPMMonitor.c

#### 私有数据结构

```c
typedef struct {
    char type_str[32];   // "AMP", "PHASE", etc.
    int type;            // TYPE_AMP, TYPE_PHASE, etc.
    int offset;          // OFFSET_AMP, OFFSET_PHASE, etc.
    int channel;         // 0-7 for RF, 0-1 for BPM
} BPMMonitorPvt;
```

**每个记录的 `dpvt` 字段指向这个结构**,存储解析后的参数。

#### 参数解析详解

```c
static long init_ai_record(struct aiRecord *prec)
{
    // 1. 获取 INP 字段
    struct instio *pinstio = (struct instio *)&(prec->inp.value);
    char *params = pinstio->string;  // 例如: "AMP:0 ch=3"

    // 2. 分配私有数据
    BPMMonitorPvt *pPvt = malloc(sizeof(BPMMonitorPvt));
    prec->dpvt = pPvt;

    // 3. 解析参数
    sscanf(params, "%[^:]:%d ch=%d",
           pPvt->type_str,   // "AMP"
           &pPvt->offset,    // 0
           &pPvt->channel);  // 3

    // 4. 确定类型枚举
    if (strcmp(pPvt->type_str, "AMP") == 0)
        pPvt->type = TYPE_AMP;
    // ...

    return 0;
}
```

**格式约定**: `"TYPE:offset ch=channel"`
- TYPE: 数据类型字符串 (用于调试)
- offset: 传递给 ReadData() 的第一个参数
- channel: 传递给 ReadData() 的第二个参数

#### 读取函数详解

```c
static long read_ai(struct aiRecord *prec)
{
    BPMMonitorPvt *pPvt = (BPMMonitorPvt *)prec->dpvt;

    // 调用驱动层
    double value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 更新记录
    prec->val = value;
    prec->udf = FALSE;  // 清除 "undefined" 标志

    return 2;  // 不需要进行线性转换
}
```

**返回值的含义**:
- `0`: 成功,需要转换 (使用 ESLO, EOFF 字段)
- `2`: 成功,不需要转换
- 负数: 错误

#### DSET 结构详解

```c
typedef struct {
    long number;          // 6: 函数数量
    DEVSUPFUN report;     // 设备报告 (可选)
    DEVSUPFUN init;       // 驱动初始化 (仅调用一次)
    DEVSUPFUN init_record;// 记录初始化 (每个记录调用一次)
    DEVSUPFUN get_ioint_info; // I/O 中断信息
    DEVSUPFUN read_ai;    // 读取函数
    DEVSUPFUN special_linconv; // 特殊线性转换 (可选)
} AI_DSET;
```

**调用顺序**:
1. IOC 启动时: `init()` (如果定义)
2. 加载数据库时: 每个记录调用 `init_record()`
3. 扫描时: 调用 `read_ai()`
4. 如果 `SCAN: I/O Intr`: 调用 `get_ioint_info()` 获取 IOSCANPVT

### 3.4 实验任务

#### 任务 1: 添加调试输出

在 `driverWrapper.c` 的 `ReadData()` 函数中添加:
```c
printf("ReadData(offset=%d, ch=%d, type=%d) = %f\n",
       offset, channel, type, value);
```

重新编译,运行 IOC,观察输出。你会看到大量的调用记录。

**思考**: 为什么会有这么多调用?

#### 任务 2: 修改扫描周期

修改后台线程的扫描周期从 100ms 改为 1 秒:
```c
epicsThreadSleep(1.0);  // 原来是 0.1
```

重新编译运行,观察 PV 值的更新频率是否变化。

#### 任务 3: 模拟硬件数据

由于没有真实硬件,在 `InitDevice()` 中添加模拟数据:

```c
int InitDevice()
{
    // ... 原有代码 ...

    // 如果库加载失败,使用模拟数据
    if (!handle) {
        printf("WARNING: Cannot load liblowlevel.so, using simulated data\n");
        use_simulation = 1;  // 添加全局标志
    }

    // ...
}

// 修改 my_thread()
static void my_thread(void *arg)
{
    while (1) {
        if (use_simulation) {
            // 模拟数据: 正弦波
            static double t = 0.0;
            for (int i = 0; i < 8; i++) {
                Amp[i] = 1.0 + 0.5 * sin(t + i * 0.2);
                Phase[i] = t * 10.0;
            }
            t += 0.1;
        } else {
            // 真实硬件
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }
        }

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.1);
    }
}
```

这样即使没有硬件,也能看到 PV 值在变化!

---

## 第 4 阶段: 实践开发

### 学习目标
- 能够独立添加新功能
- 掌握完整的开发流程
- 学会调试和测试

### 4.1 项目 1: 添加 RF 最大值监测

**需求**: 为每个 RF 通道添加一个 PV,记录自启动以来的最大振幅值。

#### Step 1: 驱动层添加支持

在 `driverWrapper.h` 中添加:
```c
#define OFFSET_MAX_AMP  36  // 新的 offset
```

在 `driverWrapper.c` 中添加:
```c
// 全局变量: 最大振幅
static double MaxAmp[8] = {0};  // 初始化为 0

// 在 my_thread() 中更新
static void my_thread(void *arg)
{
    while (1) {
        for (int i = 0; i < 8; i++) {
            (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);

            // 更新最大值
            if (Amp[i] > MaxAmp[i]) {
                MaxAmp[i] = Amp[i];
            }
        }
        // ...
    }
}

// 在 ReadData() 中添加 case
double ReadData(int offset, int channel, int type)
{
    // ...
    case OFFSET_MAX_AMP:
        value = MaxAmp[channel];
        break;
    // ...
}
```

#### Step 2: 数据库添加记录

在 `BPMMonitor.db` 中添加 (以 RF3 为例):
```
record(ai, "$(P):RF3MaxAmp") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@MAX_AMP:36 ch=0")
    field(SCAN, "1 second")
    field(PREC, "3")
    field(EGU,  "V")
    field(DESC, "Max amplitude since startup")
}
```

**为所有 8 个通道添加类似记录** (ch=0 到 ch=7)

#### Step 3: 设备支持层添加类型

在 `devBPMMonitor.c` 的 `init_ai_record()` 中添加:
```c
else if (strcmp(pPvt->type_str, "MAX_AMP") == 0)
    pPvt->type = TYPE_MAX_AMP;
```

在 `devBPMMonitor.h` 中添加:
```c
#define TYPE_MAX_AMP  10  // 新的类型
```

#### Step 4: 编译测试

```bash
make clean
make
cd iocBoot/iocBPMmonitor
./st.cmd
```

在 IOC Shell 中:
```bash
epics> dbgf "iLinac_007:BPM14And15:RF3MaxAmp"
```

### 4.2 项目 2: 添加复位最大值功能

**需求**: 添加一个 bo (二进制输出) 记录,允许用户复位最大值。

#### Step 1: 驱动层添加复位函数

在 `driverWrapper.c` 中:
```c
void ResetMaxAmp(int channel)
{
    if (channel >= 0 && channel < 8) {
        MaxAmp[channel] = 0.0;
        printf("Reset MaxAmp[%d]\n", channel);
    }
}
```

在 `driverWrapper.h` 中声明:
```c
void ResetMaxAmp(int channel);
```

#### Step 2: 设备支持层添加 bo 支持

在 `devBPMMonitor.c` 中添加:

```c
// bo 记录初始化
static long init_bo_record(struct boRecord *prec)
{
    struct instio *pinstio;
    BPMMonitorPvt *pPvt;

    pPvt = (BPMMonitorPvt *)malloc(sizeof(BPMMonitorPvt));
    prec->dpvt = pPvt;

    pinstio = (struct instio *)&(prec->out.value);
    sscanf(pinstio->string, "%[^:]:%d ch=%d",
           pPvt->type_str, &pPvt->offset, &pPvt->channel);

    return 0;
}

// bo 记录写入
static long write_bo(struct boRecord *prec)
{
    BPMMonitorPvt *pPvt = (BPMMonitorPvt *)prec->dpvt;

    if (strcmp(pPvt->type_str, "RESET_MAX_AMP") == 0) {
        if (prec->val == 1) {  // 按下按钮
            ResetMaxAmp(pPvt->channel);
        }
    }

    return 0;
}

// DSET
BO_DSET devBoBPMMonitor = {
    5,
    NULL,
    NULL,
    init_bo_record,
    NULL,
    write_bo
};
```

在 `.dbd` 文件中注册:
```c
device(bo, INST_IO, devBoBPMMonitor, "BPMMonitor")
```

#### Step 3: 数据库添加记录

```
record(bo, "$(P):RF3ResetMaxAmp") {
    field(DTYP, "BPMMonitor")
    field(OUT,  "@RESET_MAX_AMP:0 ch=0")
    field(ZNAM, "Normal")
    field(ONAM, "Reset")
    field(DESC, "Reset max amplitude")
}
```

#### Step 4: 测试

```bash
# 读取当前最大值
caget iLinac_007:BPM14And15:RF3MaxAmp

# 复位
caput iLinac_007:BPM14And15:RF3ResetMaxAmp 1

# 再次读取 (应该是 0 或接近 0)
caget iLinac_007:BPM14And15:RF3MaxAmp
```

### 4.3 项目 3: 修改波形平均算法

**需求**: 将简单平均改为中位数,减少异常值影响。

#### Step 1: 实现中位数函数

在 `driverWrapper.c` 中添加:

```c
#include <stdlib.h>  // for qsort

// 比较函数 (用于 qsort)
static int compare_float(const void *a, const void *b)
{
    float diff = (*(float*)a - *(float*)b);
    return (diff > 0) - (diff < 0);
}

// 计算中位数
static double median(float *data, int count)
{
    if (count == 0) return 0.0;

    // 复制数据 (避免修改原数组)
    float *temp = (float *)malloc(count * sizeof(float));
    memcpy(temp, data, count * sizeof(float));

    // 排序
    qsort(temp, count, sizeof(float), compare_float);

    // 取中位数
    double result;
    if (count % 2 == 0) {
        result = (temp[count/2 - 1] + temp[count/2]) / 2.0;
    } else {
        result = temp[count/2];
    }

    free(temp);
    return result;
}
```

#### Step 2: 修改 calculateAvgVoltage()

```c
void calculateAvgVoltage(int channel)
{
    if (channel < 0 || channel >= 8) return;

    float *waveform = TrigWaveform[channel];

    // 提取信号区域
    int signal_count = AVGStop[channel] - AVGStart[channel] + 1;
    float *signal_data = &waveform[AVGStart[channel]];

    // 提取背景区域
    int bg_count = BackGroundStop[channel] - BackGroundStart[channel] + 1;
    float *bg_data = &waveform[BackGroundStart[channel]];

    // 计算中位数 (替代平均值)
    double signal_median = median(signal_data, signal_count);
    double bg_median = median(bg_data, bg_count);

    AVG_Voltage[channel] = signal_median - bg_median;
}
```

#### Step 3: 测试

对比修改前后的结果,使用含噪声的测试数据。

### 4.4 调试技巧进阶

#### 使用 EPICS 调试变量

许多 EPICS 模块支持调试变量:

```c
// 在 devBPMMonitor.c 中添加
#include <iocsh.h>
#include <epicsExport.h>

int BPMMonitorDebug = 0;  // 0=关闭, 1=基本, 2=详细
epicsExportAddress(int, BPMMonitorDebug);

// 在代码中使用
static long read_ai(struct aiRecord *prec)
{
    if (BPMMonitorDebug >= 1) {
        printf("read_ai: %s\n", prec->name);
    }
    // ...
}
```

在 IOC Shell 中:
```bash
epics> var BPMMonitorDebug 1  # 开启调试
epics> var BPMMonitorDebug 0  # 关闭调试
```

#### 使用 GDB 调试

```bash
# 启动 GDB
gdb ../../bin/linux-x86_64/BPMmonitor

# 设置断点
(gdb) break ReadData
(gdb) break read_ai

# 运行
(gdb) run < st.cmd

# 断点触发后
(gdb) print offset
(gdb) print channel
(gdb) print value
(gdb) continue

# 退出
(gdb) quit
```

### 4.5 综合项目: RF 信号质量监测

**需求**: 实现一个综合的信号质量评估系统。

**功能**:
1. 计算振幅标准差 (稳定性指标)
2. 检测相位跳变 (异常检测)
3. 生成质量分数 (0-100)

**实现提示**:
- 使用滑动窗口存储最近 N 个采样点
- 计算标准差公式: σ = sqrt(Σ(x - μ)² / N)
- 相位跳变: 检测相邻采样点的差值 > 阈值
- 质量分数: 根据稳定性和异常次数计算

**挑战**: 自己设计数据结构、算法和数据库记录!

---

## 进阶主题

### 5.1 EPICS 多线程编程

#### 创建 EPICS 线程

```c
#include <epicsThread.h>

void my_task(void *arg)
{
    int *id = (int *)arg;
    while (1) {
        printf("Thread %d running\n", *id);
        epicsThreadSleep(1.0);
    }
}

// 创建线程
int task_id = 1;
epicsThreadCreate("MyTask",
                  epicsThreadPriorityMedium,  // 优先级
                  epicsThreadGetStackSize(epicsThreadStackMedium),
                  my_task,
                  &task_id);
```

#### 线程同步: Mutex

```c
#include <epicsMutex.h>

epicsMutexId my_lock;
int shared_counter = 0;

void InitLock()
{
    my_lock = epicsMutexCreate();
}

void IncrementCounter()
{
    epicsMutexLock(my_lock);
    shared_counter++;
    epicsMutexUnlock(my_lock);
}
```

### 5.2 Channel Access 编程

#### C 语言客户端

```c
#include <cadef.h>

int main()
{
    chid pv_id;
    double value;

    // 1. 初始化 CA
    ca_context_create(ca_disable_preemptive_callback);

    // 2. 连接 PV
    ca_create_channel("iLinac_007:BPM14And15:RF3Amp",
                      NULL, NULL, 0, &pv_id);
    ca_pend_io(1.0);

    // 3. 读取值
    ca_get(DBR_DOUBLE, pv_id, &value);
    ca_pend_io(1.0);
    printf("RF3 Amp: %f\n", value);

    // 4. 清理
    ca_clear_channel(pv_id);
    ca_context_destroy();
    return 0;
}
```

编译:
```bash
gcc -o ca_client ca_client.c -I${EPICS_BASE}/include \
    -L${EPICS_BASE}/lib/${EPICS_HOST_ARCH} -lca -lCom
```

### 5.3 性能优化

#### 1. 减少不必要的扫描

```
# 不好: 配置参数也使用快速扫描
field(SCAN, "1 second")

# 好: 配置参数使用 Passive
field(SCAN, "Passive")
```

#### 2. 批量读取

```c
// 不好: 每个通道单独调用
for (int i = 0; i < 8; i++) {
    (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
}

// 好: 使用批量接口 (如果硬件支持)
(*getRfInfoBatchFunc)(Amp, Phase, 8);
```

### 5.4 添加 Python 接口

使用 `pyepics` 创建高层接口:

```python
# bpm_monitor.py
import epics
import time

class BPMMonitor:
    def __init__(self, prefix):
        self.prefix = prefix

    def get_rf_amp(self, channel):
        """获取 RF 振幅"""
        pv_name = f"{self.prefix}:RF{channel+3}Amp"
        return epics.caget(pv_name)

    def get_all_rf_amps(self):
        """获取所有 RF 振幅"""
        return [self.get_rf_amp(i) for i in range(8)]

    def reset_max_amp(self, channel):
        """复位最大值"""
        pv_name = f"{self.prefix}:RF{channel+3}ResetMaxAmp"
        epics.caput(pv_name, 1)

    def monitor(self, callback):
        """监控所有 RF 通道"""
        pvs = [epics.PV(f"{self.prefix}:RF{i+3}Amp",
                        callback=callback) for i in range(8)]
        return pvs

# 使用示例
bpm = BPMMonitor("iLinac_007:BPM14And15")

# 读取
amps = bpm.get_all_rf_amps()
print(f"RF Amplitudes: {amps}")

# 监控
def on_change(pvname, value, **kws):
    print(f"{pvname} = {value:.3f} V")

pvs = bpm.monitor(on_change)

# 保持运行
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    pass
```

---

## 学习资源

### 官方文档
- **EPICS 主页**: https://epics-controls.org/
- **Application Developer's Guide**: https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide.pdf
- **Channel Access Reference**: https://epics.anl.gov/base/R3-15/6-docs/CAref.html

### 在线教程
- **EPICS Training Materials**: https://epics.anl.gov/docs/training.php
- **PSI EPICS Course**: https://github.com/paulscherrerinstitute/epics_course

### 书籍
- *Experimental Physics and Industrial Control System* (EPICS 官方文档集)

### 社区
- **EPICS Tech-Talk**: https://epics.anl.gov/tech-talk/
- **EPICS GitHub**: https://github.com/epics-base

---

## 常见问题 FAQ

### Q1: EPICS 和 PLC 有什么区别?

**EPICS**:
- 分布式架构
- 开源、免费
- 适合大型科学设施
- 灵活、可定制

**PLC**:
- 集中式控制
- 商业产品
- 适合工业自动化
- 可靠性高、易用

### Q2: 为什么用 C 而不是 C++?

历史原因和兼容性。EPICS Base 核心用 C 编写,但支持 C++ (如 BPMmonitorMain.cpp)。
驱动层通常用 C 是因为:
- 更接近硬件
- ABI 稳定
- 与第三方库兼容性好

### Q3: 如何学习加速器物理?

推荐资源:
- *Accelerator Physics* by S.Y. Lee
- *Handbook of Accelerator Physics and Engineering*
- 在线课程: CERN Accelerator School

### Q4: EPICS 适合工业控制吗?

可以,但要考虑:
- ✅ 优势: 免费、灵活、强大
- ❌ 劣势: 学习曲线陡、商业支持少

工业界更常用: Siemens PLC, Schneider Electric, Rockwell Automation

---

## 下一步

完成本学习指南后,建议:

1. **深入特定领域**:
   - 加速器控制
   - 真空系统
   - 磁铁电源
   - 射频系统

2. **学习相关技术**:
   - **EPICS 模块**: motor, asyn, StreamDevice
   - **控制理论**: PID 控制、状态机
   - **网络编程**: Channel Access, pvAccess
   - **可视化**: CS-Studio, Phoebus

3. **参与社区**:
   - 订阅 Tech-Talk 邮件列表
   - 参加 EPICS Collaboration Meeting
   - 贡献开源项目

4. **实际项目**:
   - 为实验室设备开发 EPICS 驱动
   - 搭建小型控制系统
   - 参与大科学装置建设

---

**祝学习顺利!**

如有问题,请查阅技术文档或在 EPICS 社区提问。
