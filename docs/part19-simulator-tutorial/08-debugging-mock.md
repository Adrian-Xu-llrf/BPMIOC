# Part 19.8: Mock库调试技巧

> **目标**: 掌握Mock库的调试方法和故障排查
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 1小时
> **前置**: 已完成07-build-and-test.md

## 📖 内容概览

本文档介绍Mock库的调试技巧：
- 常见问题诊断
- GDB调试技巧
- 日志和跟踪
- 数据验证
- 性能分析

学会这些技巧后，你可以快速定位和解决Mock库的问题。

---

## 1. 常见问题诊断

### 1.1 问题分类

Mock库问题通常分为四类：

| 类别 | 症状 | 可能原因 |
|------|------|----------|
| **编译问题** | 无法编译 | 语法错误、缺少头文件 |
| **链接问题** | 链接失败 | 缺少库、符号未定义 |
| **运行时错误** | 崩溃、段错误 | 空指针、数组越界 |
| **逻辑错误** | 数据不正确 | 算法错误、配置错误 |

---

### 1.2 快速诊断流程

```
问题发生
    ↓
能否编译？
    ├─ 否 → 检查语法和头文件 → [1.3节]
    └─ 是 ↓
能否链接？
    ├─ 否 → 检查库依赖 → [1.4节]
    └─ 是 ↓
能否运行？
    ├─ 否 → 使用GDB找段错误 → [2节]
    └─ 是 ↓
数据正确吗？
    ├─ 否 → 添加日志验证逻辑 → [3节]
    └─ 是 → 完成！
```

---

### 1.3 编译问题

#### 症状1: 找不到头文件

```bash
libbpm_mock.c:1:10: fatal error: libbpm_mock.h: No such file or directory
```

**诊断**:
```bash
# 检查头文件是否存在
ls -l ../include/libbpm_mock.h

# 检查编译命令中的-I参数
make clean
make 2>&1 | grep "\-I"
```

**解决**:
```makefile
# 在Makefile中确保正确的include路径
CFLAGS = -fPIC -Wall -O2 -g -I../include
```

---

#### 症状2: 未声明的标识符

```bash
libbpm_mock.c:145:5: error: 'M_PI' undeclared
```

**诊断**:
```bash
# 检查是否包含必要的头文件
grep "#include <math.h>" libbpm_mock.c
```

**解决**:
```c
// 在文件开头添加必要的宏定义
#define _USE_MATH_DEFINES
#include <math.h>

// 或者自己定义
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
```

---

### 1.4 链接问题

#### 症状: undefined reference

```bash
undefined reference to `sin'
undefined reference to `pthread_create'
```

**诊断**:
```bash
# 检查Makefile中的链接库
grep "LIBS" Makefile
```

**解决**:
```makefile
# 确保链接数学库和pthread库
LIBS = -lm -lpthread
```

---

## 2. GDB调试

### 2.1 编译调试版本

```bash
# 确保Makefile包含-g标志
CFLAGS = -fPIC -Wall -O2 -g -I../include

# 重新编译
make clean && make
```

---

### 2.2 基本GDB操作

#### 启动GDB

```bash
cd ~/BPMIOC/simulator/bin

# 调试测试程序
gdb ./test_mock
```

**GDB基本命令**:

```gdb
# 运行程序
(gdb) run

# 设置断点
(gdb) break GetRFInfo
(gdb) break libbpm_mock.c:145

# 查看源代码
(gdb) list

# 单步执行
(gdb) step      # 进入函数
(gdb) next      # 跳过函数
(gdb) continue  # 继续运行

# 查看变量
(gdb) print ch_idx
(gdb) print g_rf_channels[0]
(gdb) print *cfg

# 查看调用栈
(gdb) backtrace
(gdb) bt

# 退出
(gdb) quit
```

---

### 2.3 调试段错误（Segmentation Fault）

#### 示例：空指针解引用

假设代码有bug：

```c
float GetRFInfo(int channel, int type) {
    RfChannelConfig *cfg = NULL;  // Bug: 忘记初始化
    return cfg->base_amplitude;   // 段错误！
}
```

**GDB调试**:

```bash
$ gdb ./test_mock
(gdb) run

Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7bd1a45 in GetRFInfo (channel=3, type=0) at libbpm_mock.c:245
245         return cfg->base_amplitude;

(gdb) bt
#0  0x00007ffff7bd1a45 in GetRFInfo (channel=3, type=0) at libbpm_mock.c:245
#1  0x0000555555554abc in main () at test_mock.c:23

(gdb) print cfg
$1 = (RfChannelConfig *) 0x0    # 空指针！

(gdb) list 240,250
240     float GetRFInfo(int channel, int type) {
241         RfChannelConfig *cfg = NULL;
242         // ...
245         return cfg->base_amplitude;  # 这里出错
```

**找到问题**: `cfg`是空指针

---

### 2.4 条件断点

```gdb
# 只在特定条件下停止
(gdb) break GetRFInfo if channel == 5

# 只在第10次调用时停止
(gdb) break GetRFInfo
(gdb) commands
> silent
> set $count = $count + 1
> if $count == 10
>   printf "10th call\n"
> else
>   continue
> end
> end
```

---

### 2.5 查看数组和结构体

```gdb
# 查看数组
(gdb) print g_rf_channels[0]
$2 = {
  base_amplitude = 1.0,
  amp_variation_freq = 0.1,
  amp_noise_level = 0.01,
  ...
}

# 查看数组所有元素
(gdb) print g_rf_channels@4

# 漂亮打印
(gdb) set print pretty on
(gdb) print g_rf_channels[0]
$3 = {
  base_amplitude = 1.0,
  amp_variation_freq = 0.1,
  amp_noise_level = 0.01,
  base_phase = 0.0,
  ...
}
```

---

## 3. 日志和跟踪

### 3.1 添加调试日志

#### 简单printf日志

```c
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
  #define DEBUG_PRINT(fmt, ...) \
    printf("[DEBUG] %s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(fmt, ...) do {} while(0)
#endif

float GetRFInfo(int channel, int type) {
    DEBUG_PRINT("Called with channel=%d, type=%d\n", channel, type);

    int ch_idx = channel - 3;
    DEBUG_PRINT("ch_idx=%d\n", ch_idx);

    if (ch_idx < 0 || ch_idx >= 4) {
        DEBUG_PRINT("ERROR: Invalid channel index\n");
        return 0.0f;
    }

    // ...
}
```

**输出**:
```
[DEBUG] libbpm_mock.c:245: Called with channel=3, type=0
[DEBUG] libbpm_mock.c:248: ch_idx=0
```

---

### 3.2 分级日志系统

```c
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3,
    LOG_VERBOSE = 4
} LogLevel;

static LogLevel g_log_level = LOG_INFO;

void SetLogLevel(int level) {
    g_log_level = level;
}

#define LOG(level, fmt, ...) \
    do { \
        if (g_log_level >= level) { \
            const char *level_str[] = {"ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"}; \
            printf("[%s] %s:%d: " fmt, \
                   level_str[level], __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } \
    } while(0)

// 使用示例
float GetRFInfo(int channel, int type) {
    LOG(LOG_VERBOSE, "channel=%d, type=%d\n", channel, type);

    int ch_idx = channel - 3;
    if (ch_idx < 0 || ch_idx >= 4) {
        LOG(LOG_ERROR, "Invalid channel: %d\n", channel);
        return 0.0f;
    }

    LOG(LOG_DEBUG, "Generating RF data for ch_idx=%d\n", ch_idx);
    // ...
}
```

**控制日志级别**:

```c
int main() {
    SetLogLevel(LOG_DEBUG);  // 显示DEBUG及以下
    SystemInit();

    SetLogLevel(LOG_ERROR);  // 只显示ERROR
    // ... 正常运行，不输出调试信息
}
```

---

### 3.3 日志文件输出

```c
static FILE *g_log_file = NULL;

void InitLogging(const char *filename) {
    if (filename) {
        g_log_file = fopen(filename, "w");
    }
}

void CloseLogging(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

#define LOG_TO_FILE(fmt, ...) \
    do { \
        if (g_log_file) { \
            fprintf(g_log_file, fmt, ##__VA_ARGS__); \
            fflush(g_log_file); \
        } \
    } while(0)

// 使用
int main() {
    InitLogging("mock_debug.log");
    SystemInit();
    // ...
    CloseLogging();
}
```

---

### 3.4 函数调用跟踪

```c
#define TRACE_ENABLED 1

#if TRACE_ENABLED
static int g_trace_indent = 0;

#define TRACE_ENTER() \
    do { \
        for (int i = 0; i < g_trace_indent; i++) printf("  "); \
        printf("→ %s\n", __FUNCTION__); \
        g_trace_indent++; \
    } while(0)

#define TRACE_EXIT() \
    do { \
        g_trace_indent--; \
        for (int i = 0; i < g_trace_indent; i++) printf("  "); \
        printf("← %s\n", __FUNCTION__); \
    } while(0)

#else
#define TRACE_ENTER() do {} while(0)
#define TRACE_EXIT() do {} while(0)
#endif

// 使用示例
float generateRfAmplitude(RfChannelConfig *cfg, double time) {
    TRACE_ENTER();

    float result = /* ... 计算 ... */;

    TRACE_EXIT();
    return result;
}

float GetRFInfo(int channel, int type) {
    TRACE_ENTER();

    // ...
    float amp = generateRfAmplitude(cfg, time);

    TRACE_EXIT();
    return amp;
}
```

**输出**:
```
→ GetRFInfo
  → generateRfAmplitude
  ← generateRfAmplitude
← GetRFInfo
```

---

## 4. 数据验证

### 4.1 断言（Assertions）

```c
#include <assert.h>

float GetRFInfo(int channel, int type) {
    // 参数检查
    assert(channel >= 3 && channel <= 6);
    assert(type >= 0 && type <= 3);

    int ch_idx = channel - 3;
    assert(ch_idx >= 0 && ch_idx < 4);

    RfChannelConfig *cfg = &g_rf_channels[ch_idx];
    assert(cfg != NULL);

    // ...
}
```

**注意**: 在发布版本中禁用断言：
```makefile
# Release版本
CFLAGS = -fPIC -Wall -O2 -DNDEBUG
```

---

### 4.2 数值范围检查

```c
float GetRFInfo(int channel, int type) {
    // ...
    float value = generateRfAmplitude(cfg, time);

    // 检查数值合理性
    if (value < 0.0f || value > 10.0f) {
        LOG(LOG_WARN, "RF amplitude out of range: %.2f\n", value);
    }

    if (isnan(value) || isinf(value)) {
        LOG(LOG_ERROR, "RF amplitude is NaN or Inf!\n");
        return 1.0f;  // 返回默认值
    }

    return value;
}
```

---

### 4.3 数据一致性检查

```c
// 检查RF相位是否在[-π, π]范围内
float GetRFInfo(int channel, int type) {
    if (type == 1) {  // Phase
        float phase = generateRfPhase(cfg, time);

        // 归一化到[-π, π]
        while (phase > M_PI) phase -= 2.0 * M_PI;
        while (phase < -M_PI) phase += 2.0 * M_PI;

        assert(phase >= -M_PI && phase <= M_PI);

        return phase;
    }
    // ...
}
```

---

## 5. 性能分析

### 5.1 简单性能测量

```c
#include <sys/time.h>

double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

void test_performance(void) {
    const int N = 100000;
    double start, end;

    start = get_time_us();
    for (int i = 0; i < N; i++) {
        GetRFInfo(3, 0);
    }
    end = get_time_us();

    printf("GetRFInfo: %.2f μs per call\n", (end - start) / N);
}
```

---

### 5.2 使用perf工具

```bash
# 性能分析
perf record -g ../bin/test_mock

# 查看报告
perf report

# 查看热点函数
perf top
```

---

### 5.3 使用gprof

```bash
# 编译时添加-pg
gcc -pg -o test_mock test_mock.c -L../lib -lbpm_mock

# 运行程序（生成gmon.out）
./test_mock

# 查看分析结果
gprof test_mock gmon.out > analysis.txt
less analysis.txt
```

---

## 6. 内存调试

### 6.1 使用Valgrind

```bash
# 内存泄漏检查
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ../bin/test_mock
```

**示例输出**:
```
==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 10 allocs, 10 frees, 1,024 bytes allocated
==12345==
==12345== All heap blocks were freed -- no leaks are possible
```

---

### 6.2 检测内存错误

```bash
# 检测未初始化的内存使用
valgrind --track-origins=yes ../bin/test_mock

# 检测数组越界
valgrind --track-origins=yes \
         --read-var-info=yes \
         ../bin/test_mock
```

---

### 6.3 手动内存检查

```c
// 在SystemInit中初始化
int SystemInit(void) {
    // 清零所有内存
    memset(&g_config, 0, sizeof(g_config));
    memset(g_rf_channels, 0, sizeof(g_rf_channels));

    // 检查内存是否正确清零
    assert(g_config.simulation_time == 0.0);
    assert(g_rf_channels[0].base_amplitude == 0.0);

    // ... 初始化
}
```

---

## 7. 调试实战案例

### 案例1: RF数据不变化

**症状**: GetRFInfo总是返回相同的值

**调试步骤**:

```bash
# 1. 添加日志
$ cat > debug_rf.c
#define DEBUG_ENABLED 1
#include "libbpm_mock.c"  // 直接包含以添加调试

# 2. 运行并观察
$ gcc -o debug_rf debug_rf.c -lm && ./debug_rf
```

**发现问题**:
```c
float GetRFInfo(int channel, int type) {
    double time = g_config.simulation_time;
    DEBUG_PRINT("time=%.3f\n", time);  // 发现time始终是0.0！

    // 原因：忘记调用TriggerAllDataReached()
}
```

**解决**:
```c
// 在测试程序中
TriggerAllDataReached();  // 增加时间
float val = GetRFInfo(3, 0);
```

---

### 案例2: 段错误

**症状**: 程序崩溃

```bash
$ ./test_mock
Segmentation fault (core dumped)
```

**调试**:

```bash
# 启用core dump
$ ulimit -c unlimited

# 重新运行
$ ./test_mock
Segmentation fault (core dumped)

# 使用GDB分析core文件
$ gdb ./test_mock core
(gdb) bt
#0  0x00007ffff7bd1a45 in GetRFInfo (channel=7, type=0) at libbpm_mock.c:245
#1  0x0000555555554abc in main () at test_mock.c:23

(gdb) frame 0
(gdb) print channel
$1 = 7

(gdb) print ch_idx
$2 = 4    # 超出数组范围！
```

**发现问题**: channel=7超出范围（应该是3-6）

**解决**:
```c
float GetRFInfo(int channel, int type) {
    // 添加边界检查
    if (channel < 3 || channel > 6) {
        LOG(LOG_ERROR, "Invalid channel: %d\n", channel);
        return 0.0f;
    }
    // ...
}
```

---

### 案例3: 性能下降

**症状**: Mock库变慢

**调试**:

```bash
# 使用perf分析
$ perf record -g ../bin/test_mock
$ perf report

# 发现sin()函数占用80%时间
```

**解决**: 使用查表法优化（参考06-advanced-techniques.md）

---

## 8. 调试工具箱

### 8.1 常用工具

| 工具 | 用途 | 命令示例 |
|------|------|----------|
| **GDB** | 源码级调试 | `gdb ./test_mock` |
| **Valgrind** | 内存错误检测 | `valgrind --leak-check=full ./test_mock` |
| **strace** | 系统调用跟踪 | `strace ./test_mock` |
| **ltrace** | 库函数调用跟踪 | `ltrace ./test_mock` |
| **perf** | 性能分析 | `perf record -g ./test_mock` |
| **gprof** | 函数性能分析 | `gprof ./test_mock` |
| **objdump** | 反汇编 | `objdump -d libbpm_mock.so` |
| **nm** | 符号表查看 | `nm -D libbpm_mock.so` |
| **ldd** | 库依赖查看 | `ldd test_mock` |

---

### 8.2 GDB快速参考

```gdb
# 启动
gdb ./program
gdb ./program core        # 分析core dump
gdb --args ./program arg1 arg2

# 断点
break main
break file.c:123
break function if var == 5

# 执行
run
continue
step / s
next / n
finish

# 查看
print variable
print *pointer
print array[0]@10
backtrace / bt
info locals
info args

# 修改
set var = value
```

---

## 9. 调试清单

遇到问题时，按以下清单逐项检查：

### 编译问题
- [ ] 头文件路径正确（-I参数）
- [ ] 所有必要的头文件都包含
- [ ] 没有语法错误

### 链接问题
- [ ] 链接了必要的库（-lm -lpthread）
- [ ] 库文件路径正确（-L参数）
- [ ] 函数都正确声明和定义

### 运行时崩溃
- [ ] 使用GDB定位崩溃位置
- [ ] 检查空指针
- [ ] 检查数组越界
- [ ] 使用Valgrind检查内存错误

### 数据错误
- [ ] 添加日志跟踪数据流
- [ ] 检查数值范围
- [ ] 验证算法逻辑
- [ ] 检查配置参数

### 性能问题
- [ ] 测量函数执行时间
- [ ] 使用perf找热点
- [ ] 优化计算密集的代码

---

## 10. 总结

### 你学到了什么？

✅ **问题诊断**
- 快速定位问题类型
- 系统性的排查方法

✅ **GDB调试**
- 设置断点和单步执行
- 查看变量和调用栈
- 条件断点和core dump分析

✅ **日志系统**
- 分级日志
- 函数调用跟踪
- 日志文件输出

✅ **数据验证**
- 断言检查
- 数值范围验证
- 一致性检查

✅ **性能分析**
- 测量函数性能
- 使用perf/gprof工具
- 优化热点代码

✅ **内存调试**
- Valgrind内存检查
- 泄漏检测
- 越界检测

---

### 下一步

现在你已经掌握了Mock库的调试技巧！

继续学习：
- **[09-integration-with-ioc.md](./09-integration-with-ioc.md)** - 与BPMIOC IOC集成
- **[10-mock-api-reference.md](./10-mock-api-reference.md)** - API完整参考
- **[11-best-practices.md](./11-best-practices.md)** - 最佳实践

---

**🎯 重要提示**: 调试是开发过程中不可避免的一部分。掌握这些技巧会大大提高你的开发效率！
