# Part 19.10: Mock库API完整参考

> **目标**: 完整的Mock库API参考手册
> **难度**: ⭐⭐☆☆☆
> **用途**: 查阅函数定义和使用方法

## 📖 内容概览

本文档提供Mock库的完整API参考：
- 所有50+函数的详细说明
- 参数和返回值
- 使用示例
- 注意事项

作为参考手册使用，按需查阅。

---

## API分类

Mock库包含7大类、50+个函数：

| 类别 | 函数数量 | 用途 |
|------|---------|------|
| **系统管理** | 3 | 初始化、关闭、配置 |
| **数据采集** | 2 | 触发、状态 |
| **RF信号** | 4 | RF3-RF6幅度和相位 |
| **XY位置** | 8 | XY1-XY4位置数据 |
| **Button信号** | 4 | Button1-Button4信号 |
| **波形数据** | 4 | RF和XY波形 |
| **寄存器** | 100 | 配置和状态寄存器 |

---

## 1. 系统管理API

### 1.1 SystemInit

**功能**: 初始化Mock库

**原型**:
```c
int SystemInit(void);
```

**参数**: 无

**返回值**:
- `0`: 成功
- `-1`: 失败

**描述**:
初始化Mock库的全局状态，包括：
- 设置随机数种子
- 初始化时间为0
- 配置RF通道参数
- 配置XY通道参数
- 初始化寄存器

**使用示例**:
```c
if (SystemInit() != 0) {
    printf("ERROR: SystemInit failed\n");
    return -1;
}
printf("Mock library initialized\n");
```

**注意事项**:
- 必须在调用其他函数前调用
- 可以多次调用，每次会重新初始化
- 线程安全（使用了初始化标志）

---

### 1.2 SystemClose

**功能**: 关闭Mock库

**原型**:
```c
int SystemClose(void);
```

**参数**: 无

**返回值**:
- `0`: 成功

**描述**:
清理Mock库资源：
- 清除初始化标志
- 释放动态分配的内存（如有）

**使用示例**:
```c
// 程序结束时
SystemClose();
```

**注意事项**:
- 调用后需要重新调用SystemInit才能继续使用
- 当前实现较简单，主要用于对称性

---

### 1.3 SystemReset

**功能**: 重置系统状态

**原型**:
```c
int SystemReset(void);
```

**参数**: 无

**返回值**:
- `0`: 成功

**描述**:
重置系统到初始状态：
- 重置时间为0
- 重置所有寄存器
- 保持配置不变

**使用示例**:
```c
// 重新开始模拟
SystemReset();
```

---

## 2. 数据采集API

### 2.1 TriggerAllDataReached

**功能**: 触发一次数据采集

**原型**:
```c
int TriggerAllDataReached(void);
```

**参数**: 无

**返回值**:
- `0`: 成功

**描述**:
模拟硬件的数据采集触发：
- 增加模拟时间（默认+0.1秒）
- 更新内部状态
- 准备新的数据

**使用示例**:
```c
// 在读取数据前触发
TriggerAllDataReached();

// 读取新数据
float amp = GetRFInfo(3, 0);
```

**注意事项**:
- **必须定期调用**，否则数据不会变化
- 典型调用频率：10 Hz（每100ms）
- 可以调用SetTimeIncrement修改时间增量

---

### 2.2 GetDataReachedStatus

**功能**: 获取数据就绪状态

**原型**:
```c
int GetDataReachedStatus(void);
```

**参数**: 无

**返回值**:
- `1`: 数据就绪
- `0`: 数据未就绪

**描述**:
检查是否有新数据可读。在Mock库中总是返回1。

**使用示例**:
```c
if (GetDataReachedStatus()) {
    // 读取数据
    float value = GetRFInfo(3, 0);
}
```

---

## 3. RF信号API

### 3.1 GetRFInfo

**功能**: 获取RF信号信息

**原型**:
```c
float GetRFInfo(int channel, int type);
```

**参数**:
- `channel`: RF通道号（3-6对应RF3-RF6）
- `type`: 数据类型
  - `0`: 幅度（Amplitude）
  - `1`: 相位（Phase）
  - `2`: 实部（Real）
  - `3`: 虚部（Imaginary）

**返回值**:
- 浮点数值：
  - 幅度：0.8 - 1.2（典型值~1.0）
  - 相位：-π到+π弧度
  - 实部/虚部：根据幅度和相位计算

**描述**:
返回指定RF通道的信号信息。数据包含：
- 基准值
- 长期漂移
- 周期性变化
- 随机噪声

**使用示例**:
```c
// 获取RF3幅度
float amp = GetRFInfo(3, 0);
printf("RF3 Amplitude: %.3f\n", amp);

// 获取RF4相位
float phase = GetRFInfo(4, 1);
printf("RF4 Phase: %.3f rad\n", phase);

// 获取RF5复数形式
float real = GetRFInfo(5, 2);
float imag = GetRFInfo(5, 3);
printf("RF5 = %.3f + %.3fi\n", real, imag);
```

**通道对应关系**:
| channel | 物理通道 |
|---------|---------|
| 3 | RF3 |
| 4 | RF4 |
| 5 | RF5 |
| 6 | RF6 |

**注意事项**:
- 无效通道返回0.0
- 无效类型返回0.0
- 需要先调用TriggerAllDataReached更新数据

---

### 3.2 GetRFAmplitude

**功能**: 获取RF幅度（简化接口）

**原型**:
```c
float GetRFAmplitude(int channel);
```

**参数**:
- `channel`: RF通道号（3-6）

**返回值**:
- RF幅度值

**描述**:
等价于 `GetRFInfo(channel, 0)`

**使用示例**:
```c
float amp = GetRFAmplitude(3);
```

---

### 3.3 GetRFPhase

**功能**: 获取RF相位（简化接口）

**原型**:
```c
float GetRFPhase(int channel);
```

**参数**:
- `channel`: RF通道号（3-6）

**返回值**:
- RF相位值（弧度）

**描述**:
等价于 `GetRFInfo(channel, 1)`

**使用示例**:
```c
float phase = GetRFPhase(4);
```

---

## 4. XY位置API

### 4.1 GetXYPosition

**功能**: 获取XY位置数据

**原型**:
```c
float GetXYPosition(int channel);
```

**参数**:
- `channel`: 位置通道（0-7）
  - 偶数通道：X位置
  - 奇数通道：Y位置

**返回值**:
- 位置值（单位：mm）
- 典型范围：-0.5 到 +0.5 mm

**描述**:
返回束流在BPM处的横向/纵向位置。包含：
- 轨道中心偏移
- 轨道运动
- 抖动
- 漂移

**使用示例**:
```c
// 获取XY1的X和Y位置
float x1 = GetXYPosition(0);  // XY1-X
float y1 = GetXYPosition(1);  // XY1-Y
printf("XY1: (%.3f, %.3f) mm\n", x1, y1);

// 获取所有4个BPM的位置
for (int i = 0; i < 4; i++) {
    float x = GetXYPosition(i * 2);
    float y = GetXYPosition(i * 2 + 1);
    printf("XY%d: (%.3f, %.3f) mm\n", i+1, x, y);
}
```

**通道映射**:
| channel | 位置 | BPM |
|---------|------|-----|
| 0 | X | XY1 |
| 1 | Y | XY1 |
| 2 | X | XY2 |
| 3 | Y | XY2 |
| 4 | X | XY3 |
| 5 | Y | XY3 |
| 6 | X | XY4 |
| 7 | Y | XY4 |

**注意事项**:
- 无效通道返回0.0
- XY位置包含抖动和漂移，更接近真实情况

---

### 4.2 GetXPosition

**功能**: 获取X位置（简化接口）

**原型**:
```c
float GetXPosition(int bpm_index);
```

**参数**:
- `bpm_index`: BPM索引（0-3对应XY1-XY4）

**返回值**:
- X位置（mm）

**描述**:
等价于 `GetXYPosition(bpm_index * 2)`

**使用示例**:
```c
float x = GetXPosition(0);  // XY1的X位置
```

---

### 4.3 GetYPosition

**功能**: 获取Y位置（简化接口）

**原型**:
```c
float GetYPosition(int bpm_index);
```

**参数**:
- `bpm_index`: BPM索引（0-3）

**返回值**:
- Y位置（mm）

**描述**:
等价于 `GetXYPosition(bpm_index * 2 + 1)`

**使用示例**:
```c
float y = GetYPosition(0);  // XY1的Y位置
```

---

## 5. Button信号API

### 5.1 GetButton

**功能**: 获取Button电极信号

**原型**:
```c
float GetButton(int channel);
```

**参数**:
- `channel`: Button通道（0-3对应Button1-Button4）

**返回值**:
- Button信号强度
- 典型范围：0.8 - 1.2

**描述**:
返回BPM四个电极的信号强度。包含：
- 基准值（~1.0）
- 与XY位置的耦合
- 随机噪声

**使用示例**:
```c
// 读取所有Button信号
for (int i = 0; i < 4; i++) {
    float btn = GetButton(i);
    printf("Button%d: %.3f\n", i+1, btn);
}

// 计算Button和
float sum = 0.0f;
for (int i = 0; i < 4; i++) {
    sum += GetButton(i);
}
printf("Button Sum: %.3f\n", sum);
```

**Button与位置的关系**:
```
    Button1  Button2
        ┌──────┐
        │      │
        │  ●   │  ← 束流
        │      │
        └──────┘
    Button3  Button4

位置 = f(Button1, Button2, Button3, Button4)
```

**注意事项**:
- Button信号和应接近4.0
- 位置偏移会导致Button信号不对称

---

## 6. 波形数据API

### 6.1 ReadWaveform

**功能**: 读取波形数据

**原型**:
```c
int ReadWaveform(int type, int channel, float *buffer, int size);
```

**参数**:
- `type`: 波形类型
  - `0`: RF波形
  - `1`: XY波形
- `channel`: 通道号
  - RF: 0-3（对应RF3-RF6）
  - XY: 0-7（对应XY1-XY4的X/Y）
- `buffer`: 输出缓冲区
- `size`: 缓冲区大小（点数）

**返回值**:
- 实际读取的点数
- `-1`: 错误

**描述**:
读取完整的ADC波形数据（10,000或100,000点）

**使用示例**:
```c
// 读取RF3波形
float rf_waveform[10000];
int n = ReadWaveform(0, 0, rf_waveform, 10000);
printf("Read %d points\n", n);

// 读取XY1-X波形
float xy_waveform[10000];
n = ReadWaveform(1, 0, xy_waveform, 10000);
```

**注意事项**:
- 确保buffer足够大
- 波形数据是实时生成的
- 可能比较耗时

---

### 6.2 GetWaveformLength

**功能**: 获取波形长度

**原型**:
```c
int GetWaveformLength(int type);
```

**参数**:
- `type`: 波形类型（0=RF, 1=XY）

**返回值**:
- 波形点数（默认10,000）

**使用示例**:
```c
int len = GetWaveformLength(0);
float *buffer = malloc(len * sizeof(float));
ReadWaveform(0, 0, buffer, len);
free(buffer);
```

---

## 7. 寄存器API

### 7.1 SetReg

**功能**: 写寄存器

**原型**:
```c
int SetReg(int addr, int value);
```

**参数**:
- `addr`: 寄存器地址（0-99）
- `value`: 要写入的值

**返回值**:
- `0`: 成功
- `-1`: 地址无效

**描述**:
设置配置或控制寄存器

**使用示例**:
```c
// 设置采样率
SetReg(1, 100);  // 寄存器1 = 采样率

// 设置触发模式
SetReg(2, 1);    // 寄存器2 = 触发模式
```

**常用寄存器**:
| 地址 | 名称 | 用途 | 默认值 |
|------|------|------|--------|
| 0 | SYSTEM_STATUS | 系统状态 | 1 |
| 1 | SAMPLE_RATE | 采样率 | 100 |
| 2 | TRIGGER_MODE | 触发模式 | 0 |
| 3 | DATA_READY | 数据就绪 | 1 |
| 10-19 | USER_REG | 用户寄存器 | 0 |

---

### 7.2 GetReg

**功能**: 读寄存器

**原型**:
```c
int GetReg(int addr);
```

**参数**:
- `addr`: 寄存器地址（0-99）

**返回值**:
- 寄存器值
- `-1`: 地址无效

**描述**:
读取寄存器值

**使用示例**:
```c
// 读取系统状态
int status = GetReg(0);
printf("System status: %d\n", status);

// 读取采样率
int rate = GetReg(1);
printf("Sample rate: %d\n", rate);
```

---

## 8. 时间控制API

### 8.1 SetTimeIncrement

**功能**: 设置时间增量

**原型**:
```c
void SetTimeIncrement(double dt);
```

**参数**:
- `dt`: 时间增量（秒）

**描述**:
设置每次调用TriggerAllDataReached时增加的时间

**使用示例**:
```c
// 设置为10ms（100 Hz）
SetTimeIncrement(0.01);

// 设置为100ms（10 Hz，默认）
SetTimeIncrement(0.1);
```

---

### 8.2 GetSimulationTime

**功能**: 获取当前模拟时间

**原型**:
```c
double GetSimulationTime(void);
```

**返回值**:
- 当前模拟时间（秒）

**使用示例**:
```c
double t = GetSimulationTime();
printf("Simulation time: %.3f s\n", t);
```

---

## 9. 配置API（可选）

### 9.1 LoadConfig

**功能**: 从文件加载配置

**原型**:
```c
int LoadConfig(const char *filename);
```

**参数**:
- `filename`: 配置文件路径

**返回值**:
- `0`: 成功
- `-1`: 失败

**描述**:
从INI文件加载Mock库配置

**使用示例**:
```c
if (LoadConfig("mock_config.ini") == 0) {
    printf("Configuration loaded\n");
}
```

---

## 10. 调试API（可选）

### 10.1 SetDebugLevel

**功能**: 设置调试级别

**原型**:
```c
void SetDebugLevel(int level);
```

**参数**:
- `level`: 调试级别（0-4）
  - `0`: ERROR
  - `1`: WARN
  - `2`: INFO
  - `3`: DEBUG
  - `4`: VERBOSE

**使用示例**:
```c
SetDebugLevel(3);  // 显示DEBUG及以下
```

---

### 10.2 PrintStatus

**功能**: 打印Mock库状态

**原型**:
```c
void PrintStatus(void);
```

**描述**:
打印当前Mock库的状态信息

**使用示例**:
```c
PrintStatus();
```

**输出示例**:
```
=== Mock Library Status ===
Initialized: Yes
Simulation time: 12.345 s
Time increment: 0.100 s
RF Channels: 4
XY Channels: 8
Registers: 100
```

---

## 11. 完整使用示例

### 11.1 基本使用流程

```c
#include "libbpm_mock.h"

int main() {
    // 1. 初始化
    if (SystemInit() != 0) {
        printf("ERROR: Initialization failed\n");
        return 1;
    }

    // 2. 可选：加载配置
    LoadConfig("mock_config.ini");

    // 3. 可选：设置时间增量
    SetTimeIncrement(0.1);  // 100ms

    // 4. 主循环
    for (int i = 0; i < 100; i++) {
        // 触发数据采集
        TriggerAllDataReached();

        // 读取RF数据
        for (int ch = 3; ch <= 6; ch++) {
            float amp = GetRFInfo(ch, 0);
            float phase = GetRFInfo(ch, 1);
            printf("RF%d: Amp=%.3f, Phase=%.3f\n", ch, amp, phase);
        }

        // 读取XY位置
        for (int i = 0; i < 4; i++) {
            float x = GetXPosition(i);
            float y = GetYPosition(i);
            printf("XY%d: (%.3f, %.3f)\n", i+1, x, y);
        }

        // 读取Button信号
        for (int i = 0; i < 4; i++) {
            float btn = GetButton(i);
            printf("Button%d: %.3f\n", i+1, btn);
        }

        // 延迟
        usleep(100000);  // 100ms
    }

    // 5. 关闭
    SystemClose();

    return 0;
}
```

---

### 11.2 与EPICS IOC集成

```c
// driverWrapper.c

// 全局函数指针
static int (*funcSystemInit)(void) = NULL;
static int (*funcTriggerAllDataReached)(void) = NULL;
static float (*funcGetRFInfo)(int, int) = NULL;
static float (*funcGetXYPosition)(int) = NULL;

// 加载Mock库
int loadMockLibrary(void) {
    void *handle = dlopen("libbpm_mock.so", RTLD_LAZY);
    if (!handle) return -1;

    funcSystemInit = dlsym(handle, "SystemInit");
    funcTriggerAllDataReached = dlsym(handle, "TriggerAllDataReached");
    funcGetRFInfo = dlsym(handle, "GetRFInfo");
    funcGetXYPosition = dlsym(handle, "GetXYPosition");

    return funcSystemInit();
}

// ReadData接口实现
float ReadData(int offset, int channel, int type) {
    if (offset == OFFSET_RF) {
        return funcGetRFInfo(channel, type);
    } else if (offset == OFFSET_XY) {
        return funcGetXYPosition(channel);
    }
    return 0.0f;
}

// 数据采集线程
void *acquisitionThread(void *arg) {
    while (running) {
        funcTriggerAllDataReached();
        usleep(100000);  // 10 Hz
    }
    return NULL;
}
```

---

## 12. API对比：Mock vs Real

| 功能 | Mock库 | Real库 | 注意事项 |
|------|--------|--------|----------|
| SystemInit | 软件初始化 | 硬件初始化 | Mock更快 |
| GetRFInfo | 数学公式生成 | 读取硬件 | Mock数据更理想 |
| GetXYPosition | 算法生成轨迹 | 真实束流位置 | Mock需要调参 |
| SetReg | 写内存数组 | 写FPGA寄存器 | Mock立即生效 |
| ReadWaveform | 实时生成 | 读取ADC | Mock更快 |

---

## 13. 快速查询表

### 函数快速索引

| 函数名 | 分类 | 频繁度 |
|--------|------|--------|
| `SystemInit()` | 系统 | 一次 |
| `SystemClose()` | 系统 | 一次 |
| `TriggerAllDataReached()` | 采集 | 高频 |
| `GetRFInfo(ch, type)` | RF | 高频 |
| `GetXYPosition(ch)` | XY | 高频 |
| `GetButton(ch)` | Button | 中频 |
| `ReadWaveform(...)` | 波形 | 低频 |
| `SetReg(addr, val)` | 寄存器 | 低频 |
| `GetReg(addr)` | 寄存器 | 低频 |

---

## 14. 总结

本文档提供了Mock库的完整API参考。

### 关键要点

✅ **核心函数**
- SystemInit/Close: 必须调用
- TriggerAllDataReached: 定期调用
- GetRFInfo/GetXYPosition: 数据读取

✅ **使用流程**
1. SystemInit()
2. 循环: TriggerAllDataReached() → 读取数据
3. SystemClose()

✅ **注意事项**
- 检查返回值
- 验证参数范围
- 定期触发数据更新

---

### 相关文档

- **[11-best-practices.md](./11-best-practices.md)** - 最佳实践
- **[05-complete-mock-implementation.md](./05-complete-mock-implementation.md)** - 完整实现
- **[09-integration-with-ioc.md](./09-integration-with-ioc.md)** - IOC集成

---

**📚 提示**: 将此文档加入书签，作为日常开发的快速参考！
