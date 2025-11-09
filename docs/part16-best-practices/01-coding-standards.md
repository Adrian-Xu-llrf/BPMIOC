# 代码规范

> **目标**: 统一代码风格，提高可读性
> **难度**: ⭐⭐
> **预计时间**: 1天

## C语言代码规范

### 命名规范

#### 变量命名

```c
// 好的命名
int channel_count;
float rf_amplitude;
double temperature_celsius;

// 避免的命名
int n;        // 太短，不清楚
int temp;     // 缩写不明确
int i, j, k;  // 除循环外避免单字母
```

#### 函数命名

```c
// 动词+名词模式
int ReadData(int offset, int channel, int type);
int SetRegister(int addr, int value);
float CalculateSNR(float signal, float noise);

// 模块前缀
int BPM_DeviceInit(void);
float BPM_RFIn_ReadADC(int channel, int type);
```

#### 常量命名

```c
// 全大写，下划线分隔
#define MAX_RF_CHANNELS 8
#define OFFSET_AMP 0
#define DEFAULT_TIMEOUT_MS 1000

// 枚举
typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR = -1,
    STATUS_TIMEOUT = -2
} Status_t;
```

### 代码格式

#### 缩进

```c
// 使用4个空格缩进（不使用Tab）
int InitDevice(void) {
    if (condition) {
        DoSomething();
        if (another_condition) {
            DoMore();
        }
    }
    return 0;
}
```

#### 括号风格

```c
// K&R风格（推荐）
int function(int arg) {
    if (condition) {
        // code
    } else {
        // code
    }
}

// 始终使用括号，即使只有一行
if (condition) {
    single_statement();  // 好
}

if (condition)
    single_statement();  // 不推荐
```

#### 行长度

```c
// 每行不超过80字符
// 长语句应该换行
float result = CalculateComplexValue(parameter1, parameter2,
                                      parameter3, parameter4);

// 或者
float result = CalculateComplexValue(
    parameter1,
    parameter2,
    parameter3,
    parameter4
);
```

### 注释规范

#### 文件头注释

```c
/**
 * @file driverWrapper.c
 * @brief BPMIOC驱动层包装器
 * @author Your Name
 * @date 2025-11-09
 * @version 1.0
 * 
 * 驱动层负责与硬件交互，提供统一的API给设备支持层。
 */
```

#### 函数注释

```c
/**
 * @brief 读取数据
 * 
 * 从指定的offset和channel读取数据。
 * 
 * @param offset 数据类型偏移（OFFSET_AMP、OFFSET_PHA等）
 * @param channel 通道号（0-7）
 * @param type 数据类型（保留，当前未使用）
 * @return 读取的数据值
 * 
 * @note 该函数线程安全
 * @warning channel必须在有效范围内，否则返回0
 */
float ReadData(int offset, int channel, int type);
```

#### 代码注释

```c
// 好的注释：解释为什么
// 使用双缓冲避免数据竞争
float temp_buffer[MAX_RF_CHANNELS];
memcpy(temp_buffer, g_data_buffer[offset], sizeof(temp_buffer));

// 不好的注释：重复代码
// 将i设置为0
i = 0;  // 多余的注释
```

### 错误处理

```c
// 检查所有返回值
int ret = InitDevice();
if (ret != 0) {
    errlogPrintf("ERROR: InitDevice failed: %d\n", ret);
    return -1;
}

// 检查指针
DevPvt *pPvt = (DevPvt*)prec->dpvt;
if (!pPvt) {
    errlogPrintf("ERROR: NULL pointer\n");
    return S_db_badField;
}

// 检查边界
if (channel < 0 || channel >= MAX_RF_CHANNELS) {
    errlogPrintf("ERROR: Invalid channel %d\n", channel);
    return 0.0;
}
```

## .clang-format配置

```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 80
PointerAlignment: Right
AlignConsecutiveAssignments: true
AllowShortFunctionsOnASingleLine: None
BreakBeforeBraces: Linux
```

使用：

```bash
clang-format -i *.c *.h
```

## 静态分析

```bash
# cppcheck
cppcheck --enable=all --suppress=missingIncludeSystem driverWrapper.c

# 编译警告
gcc -Wall -Wextra -Werror driverWrapper.c
```

## 🔗 相关文档

- [02-project-structure.md](./02-project-structure.md)
- [05-code-review.md](./05-code-review.md)
