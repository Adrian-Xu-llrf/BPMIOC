# 单元测试完全指南

> **目标**: 掌握EPICS IOC单元测试
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 2-3天
> **前置知识**: C语言、BPMIOC架构、Make构建系统

## 📋 本文档内容

1. 单元测试概述
2. C语言测试框架
3. EPICS测试环境
4. Mock库测试
5. 测试驱动开发(TDD)
6. 实战案例

## 🎯 为什么需要单元测试

单元测试的好处：
- ✅ **快速反馈**: 立即发现代码问题
- ✅ **重构信心**: 修改代码时不怕破坏功能
- ✅ **文档作用**: 测试即文档，展示如何使用代码
- ✅ **代码质量**: 强制编写可测试的代码
- ✅ **回归防护**: 防止旧bug重新出现

## 1️⃣ 单元测试概述

### 什么是单元测试

**单元测试**: 测试代码的最小可测试单元（函数、模块）

```
系统测试
  └── 集成测试
      └── 单元测试  ← 我们关注这层
```

### 好的单元测试特征

**F.I.R.S.T原则**:
- **Fast**: 快速（毫秒级）
- **Independent**: 独立（测试间无依赖）
- **Repeatable**: 可重复（每次运行结果一致）
- **Self-Validating**: 自我验证（通过/失败明确）
- **Timely**: 及时（代码写完立即测试）

### 测试覆盖率

| 类型 | 说明 | 目标 |
|------|------|------|
| **函数覆盖率** | 每个函数是否被调用 | 100% |
| **语句覆盖率** | 每条语句是否被执行 | >80% |
| **分支覆盖率** | 每个if/else分支是否被执行 | >70% |
| **条件覆盖率** | 每个条件的true/false是否都测试 | >70% |

## 2️⃣ C语言测试框架

### 流行的C测试框架

| 框架 | 特点 | 适用场景 |
|------|------|----------|
| **Unity** | 轻量级、简单 | 嵌入式、小项目 |
| **CUnit** | 功能完整 | 中大型项目 |
| **Check** | 子进程隔离 | 健壮性要求高 |
| **Google Test (C++)** | 功能强大 | C++项目 |
| **CMocka** | 内置mock支持 | 需要大量mock |

本文档使用 **Unity** 作为示例（轻量级，易于集成）

### 安装Unity

```bash
# 下载Unity
cd /opt
git clone https://github.com/ThrowTheSwitch/Unity.git
cd Unity

# 编译
gcc -c src/unity.c -o unity.o
ar rcs libunity.a unity.o

# 安装
sudo cp libunity.a /usr/local/lib/
sudo cp src/unity.h /usr/local/include/
sudo cp src/unity_internals.h /usr/local/include/
```

### Unity基本用法

创建简单测试 `test_example.c`:

```c
#include "unity.h"

// 测试设置（每个测试前调用）
void setUp(void) {
    // 初始化
}

// 测试清理（每个测试后调用）
void tearDown(void) {
    // 清理
}

// 测试函数
void test_addition(void) {
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

void test_subtraction(void) {
    TEST_ASSERT_EQUAL(0, 2 - 2);
}

// 主函数
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_addition);
    RUN_TEST(test_subtraction);
    return UNITY_END();
}
```

编译运行：

```bash
# 编译
gcc test_example.c -lunity -o test_example

# 运行
./test_example

# 输出：
# test_example.c:10:test_addition:PASS
# test_example.c:14:test_subtraction:PASS
#
# -----------------------
# 2 Tests 0 Failures 0 Ignored
# OK
```

### Unity断言

```c
// 基本断言
TEST_ASSERT(condition);                    // condition为真
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);

// 整数比较
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_EQUAL_INT(expected, actual);
TEST_ASSERT_NOT_EQUAL(expected, actual);
TEST_ASSERT_INT_WITHIN(delta, expected, actual);  // |expected - actual| <= delta

// 浮点数比较
TEST_ASSERT_EQUAL_FLOAT(expected, actual);
TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual);
TEST_ASSERT_EQUAL_DOUBLE(expected, actual);

// 字符串比较
TEST_ASSERT_EQUAL_STRING(expected, actual);
TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, len);

// 数组比较
TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, num_elements);
TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, actual, num_elements);

// 内存比较
TEST_ASSERT_EQUAL_MEMORY(expected, actual, len);
```

## 3️⃣ 测试BPMIOC代码

### 项目结构

```
BPMmonitorApp/
├── src/
│   ├── driverWrapper.c       # 被测代码
│   ├── devBPMMonitor.c
│   └── Makefile
├── test/                     # ← 新增测试目录
│   ├── test_driverWrapper.c
│   ├── test_devBPMMonitor.c
│   ├── mocks/
│   │   └── mock_hardware.c
│   └── Makefile
└── ...
```

### 示例1: 测试ReadData函数

创建 `test/test_driverWrapper.c`:

```c
#include "unity.h"
#include <string.h>

// 模拟全局数据缓冲区
#define MAX_RF_CHANNELS 8
#define NUM_OFFSETS 10
float g_data_buffer[NUM_OFFSETS][MAX_RF_CHANNELS];

// 从driverWrapper.c复制ReadData函数（或包含头文件）
#define OFFSET_AMP 0
#define OFFSET_PHA 2

float ReadData(int offset, int channel, int type) {
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        return 0.0;
    }

    float ret = 0.0;

    switch (offset) {
        case OFFSET_AMP:
            ret = g_data_buffer[offset][channel];
            break;
        case OFFSET_PHA:
            ret = g_data_buffer[offset][channel];
            break;
        default:
            break;
    }

    return ret;
}

// setUp: 每个测试前调用
void setUp(void) {
    // 清空缓冲区
    memset(g_data_buffer, 0, sizeof(g_data_buffer));
}

void tearDown(void) {
    // 清理（如果需要）
}

// 测试1: 正常读取
void test_ReadData_normal(void) {
    // Arrange: 准备测试数据
    g_data_buffer[OFFSET_AMP][0] = 12.5;
    g_data_buffer[OFFSET_AMP][1] = 15.3;

    // Act: 执行被测函数
    float result0 = ReadData(OFFSET_AMP, 0, 0);
    float result1 = ReadData(OFFSET_AMP, 1, 0);

    // Assert: 验证结果
    TEST_ASSERT_EQUAL_FLOAT(12.5, result0);
    TEST_ASSERT_EQUAL_FLOAT(15.3, result1);
}

// 测试2: 边界检查 - 通道号过大
void test_ReadData_invalid_channel_high(void) {
    float result = ReadData(OFFSET_AMP, 999, 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0, result);
}

// 测试3: 边界检查 - 通道号为负
void test_ReadData_invalid_channel_negative(void) {
    float result = ReadData(OFFSET_AMP, -1, 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0, result);
}

// 测试4: 不同的Offset
void test_ReadData_different_offsets(void) {
    g_data_buffer[OFFSET_AMP][0] = 10.0;
    g_data_buffer[OFFSET_PHA][0] = 45.0;

    float amp = ReadData(OFFSET_AMP, 0, 0);
    float pha = ReadData(OFFSET_PHA, 0, 0);

    TEST_ASSERT_EQUAL_FLOAT(10.0, amp);
    TEST_ASSERT_EQUAL_FLOAT(45.0, pha);
}

// 测试5: 所有通道
void test_ReadData_all_channels(void) {
    // 设置所有通道
    for (int i = 0; i < MAX_RF_CHANNELS; i++) {
        g_data_buffer[OFFSET_AMP][i] = (float)i * 10.0;
    }

    // 验证所有通道
    for (int i = 0; i < MAX_RF_CHANNELS; i++) {
        float result = ReadData(OFFSET_AMP, i, 0);
        TEST_ASSERT_EQUAL_FLOAT((float)i * 10.0, result);
    }
}

// 主函数
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ReadData_normal);
    RUN_TEST(test_ReadData_invalid_channel_high);
    RUN_TEST(test_ReadData_invalid_channel_negative);
    RUN_TEST(test_ReadData_different_offsets);
    RUN_TEST(test_ReadData_all_channels);

    return UNITY_END();
}
```

### 示例2: 测试SetReg函数

```c
// 假设SetReg的签名：
// int SetReg(int offset, int channel, int reg_addr, int value);

// Mock硬件写入函数
static int mock_write_called = 0;
static int mock_last_addr = 0;
static int mock_last_value = 0;

int BPM_RF3_WriteReg(int addr, int value) {
    mock_write_called = 1;
    mock_last_addr = addr;
    mock_last_value = value;
    return 0;
}

void setUp(void) {
    mock_write_called = 0;
    mock_last_addr = 0;
    mock_last_value = 0;
}

void test_SetReg_writes_to_hardware(void) {
    int ret = SetReg(5, 0, 0x1000, 0xABCD);

    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_write_called);
    TEST_ASSERT_EQUAL(0x1000, mock_last_addr);
    TEST_ASSERT_EQUAL(0xABCD, mock_last_value);
}

void test_SetReg_invalid_channel(void) {
    int ret = SetReg(5, 999, 0x1000, 0xABCD);

    TEST_ASSERT_NOT_EQUAL(0, ret);  // 应该返回错误
    TEST_ASSERT_EQUAL(0, mock_write_called);  // 不应该调用硬件
}
```

### Makefile for tests

创建 `test/Makefile`:

```makefile
# Compiler
CC = gcc

# Flags
CFLAGS = -I../src -I/usr/local/include -Wall -Wextra -g
LDFLAGS = -L/usr/local/lib -lunity -lm

# Source files
SRC_DIR = ../src
TEST_DIR = .

# Test executables
TESTS = test_driverWrapper test_devBPMMonitor

# Default target
all: $(TESTS)

# Build test_driverWrapper
test_driverWrapper: test_driverWrapper.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Build test_devBPMMonitor
test_devBPMMonitor: test_devBPMMonitor.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Run all tests
test: $(TESTS)
	@echo "Running all tests..."
	@for test in $(TESTS); do \
		echo ""; \
		echo "=== Running $$test ==="; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

# Clean
clean:
	rm -f $(TESTS) *.o

.PHONY: all test clean
```

运行测试：

```bash
cd test
make
make test
```

输出：

```
Running all tests...

=== Running test_driverWrapper ===
test_driverWrapper.c:45:test_ReadData_normal:PASS
test_driverWrapper.c:55:test_ReadData_invalid_channel_high:PASS
test_driverWrapper.c:61:test_ReadData_invalid_channel_negative:PASS
test_driverWrapper.c:67:test_ReadData_different_offsets:PASS
test_driverWrapper.c:77:test_ReadData_all_channels:PASS

-----------------------
5 Tests 0 Failures 0 Ignored
OK

All tests passed!
```

## 4️⃣ Mock库测试

### 为什么需要Mock

**Mock**: 模拟外部依赖（硬件、网络、文件系统）

```
被测代码
  ↓ 调用
硬件函数
  ↓ 实际硬件不可用！
  ✗

解决方案：
被测代码
  ↓ 调用
Mock硬件函数
  ↓ 返回预设值
  ✓
```

### 手动Mock示例

创建 `test/mocks/mock_hardware.c`:

```c
#include <stdio.h>
#include <string.h>

// Mock状态
static int mock_init_called = 0;
static int mock_init_return = 0;

static float mock_adc_values[8] = {0};
static int mock_read_count = 0;

static int mock_write_count = 0;
static int mock_last_write_addr = 0;
static int mock_last_write_value = 0;

// Mock函数实现
int BPM_DeviceInit(void) {
    mock_init_called = 1;
    return mock_init_return;
}

float BPM_RFIn_ReadADC(int channel, int type) {
    mock_read_count++;
    if (channel >= 0 && channel < 8) {
        return mock_adc_values[channel];
    }
    return 0.0;
}

int BPM_RF3_WriteReg(int addr, int value) {
    mock_write_count++;
    mock_last_write_addr = addr;
    mock_last_write_value = value;
    return 0;
}

// Mock控制函数
void mock_reset(void) {
    mock_init_called = 0;
    mock_init_return = 0;
    memset(mock_adc_values, 0, sizeof(mock_adc_values));
    mock_read_count = 0;
    mock_write_count = 0;
    mock_last_write_addr = 0;
    mock_last_write_value = 0;
}

void mock_set_adc_value(int channel, float value) {
    if (channel >= 0 && channel < 8) {
        mock_adc_values[channel] = value;
    }
}

void mock_set_init_return(int ret) {
    mock_init_return = ret;
}

int mock_get_init_called(void) {
    return mock_init_called;
}

int mock_get_read_count(void) {
    return mock_read_count;
}

int mock_get_write_count(void) {
    return mock_write_count;
}
```

使用Mock测试：

```c
// test_with_mock.c
#include "unity.h"

// Mock控制函数声明
extern void mock_reset(void);
extern void mock_set_adc_value(int channel, float value);
extern int mock_get_read_count(void);

// 被测函数
extern float ReadData(int offset, int channel, int type);

void setUp(void) {
    mock_reset();
}

void test_ReadData_calls_hardware(void) {
    // 设置mock返回值
    mock_set_adc_value(0, 123.45);

    // 调用被测函数
    float result = ReadData(OFFSET_AMP, 0, 0);

    // 验证结果
    TEST_ASSERT_EQUAL_FLOAT(123.45, result);

    // 验证是否调用了硬件函数
    TEST_ASSERT_EQUAL(1, mock_get_read_count());
}
```

## 5️⃣ 测试驱动开发(TDD)

### TDD流程

```
1. 写测试（红灯）
   ↓
2. 写代码使测试通过（绿灯）
   ↓
3. 重构（保持绿灯）
   ↓
回到步骤1
```

### TDD示例：添加SNR计算

**需求**: 添加SNR（信噪比）计算函数

**步骤1**: 先写测试（测试会失败）

```c
// test_snr.c
#include "unity.h"
#include <math.h>

// 声明待实现的函数
float CalculateSNR(float signal, float noise);

void test_SNR_normal(void) {
    // SNR = 20 * log10(signal / noise)
    // signal=100, noise=10
    // SNR = 20 * log10(10) = 20 dB
    float snr = CalculateSNR(100.0, 10.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 20.0, snr);
}

void test_SNR_equal(void) {
    // signal == noise
    // SNR = 20 * log10(1) = 0 dB
    float snr = CalculateSNR(10.0, 10.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, snr);
}

void test_SNR_low_signal(void) {
    // signal < noise
    // SNR应该是负数
    float snr = CalculateSNR(1.0, 10.0);
    TEST_ASSERT_TRUE(snr < 0.0);
}

void test_SNR_zero_noise(void) {
    // noise = 0会导致除零
    // 应该处理这种情况
    float snr = CalculateSNR(100.0, 0.0);
    TEST_ASSERT_EQUAL_FLOAT(0.0, snr);  // 或返回错误值
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_SNR_normal);
    RUN_TEST(test_SNR_equal);
    RUN_TEST(test_SNR_low_signal);
    RUN_TEST(test_SNR_zero_noise);
    return UNITY_END();
}
```

编译会失败（CalculateSNR未定义）

**步骤2**: 实现最简代码使测试通过

```c
// snr.c
#include <math.h>

float CalculateSNR(float signal, float noise) {
    // 处理除零
    if (noise < 0.001) {
        return 0.0;
    }

    // SNR = 20 * log10(signal / noise)
    float ratio = signal / noise;
    float snr_db = 20.0 * log10(ratio);

    return snr_db;
}
```

**步骤3**: 运行测试（应该全部通过）

```bash
gcc test_snr.c snr.c -lunity -lm -o test_snr
./test_snr

# 输出：
# test_snr.c:10:test_SNR_normal:PASS
# test_snr.c:17:test_SNR_equal:PASS
# test_snr.c:23:test_SNR_low_signal:PASS
# test_snr.c:29:test_SNR_zero_noise:PASS
#
# -----------------------
# 4 Tests 0 Failures 0 Ignored
# OK
```

**步骤4**: 重构（如果需要）

```c
// 重构：添加参数校验
float CalculateSNR(float signal, float noise) {
    // 参数校验
    if (signal < 0 || noise < 0) {
        return 0.0;  // 错误值
    }

    // 处理除零
    if (noise < 0.001) {
        noise = 0.001;  // 最小噪声
    }

    // SNR = 20 * log10(signal / noise)
    return 20.0 * log10(signal / noise);
}
```

再次运行测试，确保仍然通过。

## 6️⃣ 实战案例

### 案例1: 测试init_record_ai

```c
// test_init_record.c
#include "unity.h"
#include <stdlib.h>
#include <string.h>

// 简化的aiRecord结构
typedef struct {
    char name[61];
    void *dpvt;
    struct {
        int type;  // INST_IO = 0
        struct {
            char string[40];
        } instio;
    } inp;
} aiRecord;

#define INST_IO 0
#define S_db_badField -1
#define S_db_noMemory -2

// DevPvt结构
typedef struct {
    int offset;
    int channel;
    int type;
} DevPvt;

// 被测函数（简化版）
static long init_record_ai(aiRecord *prec) {
    if (prec->inp.type != INST_IO) {
        return S_db_badField;
    }

    DevPvt *pPvt = (DevPvt*)malloc(sizeof(DevPvt));
    if (!pPvt) {
        return S_db_noMemory;
    }

    int nvals = sscanf(prec->inp.instio.string, "@%d %d %d",
                       &pPvt->offset, &pPvt->channel, &pPvt->type);

    if (nvals != 3) {
        free(pPvt);
        return S_db_badField;
    }

    prec->dpvt = pPvt;
    return 0;
}

// 测试
void setUp(void) {
    // 每个测试前清理
}

void tearDown(void) {
    // 每个测试后清理
}

void test_init_record_success(void) {
    aiRecord rec;
    memset(&rec, 0, sizeof(rec));
    strcpy(rec.name, "TEST:PV");
    rec.inp.type = INST_IO;
    strcpy(rec.inp.instio.string, "@0 0 0");

    long ret = init_record_ai(&rec);

    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(rec.dpvt);

    DevPvt *pPvt = (DevPvt*)rec.dpvt;
    TEST_ASSERT_EQUAL(0, pPvt->offset);
    TEST_ASSERT_EQUAL(0, pPvt->channel);
    TEST_ASSERT_EQUAL(0, pPvt->type);

    free(rec.dpvt);
}

void test_init_record_invalid_link_type(void) {
    aiRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.inp.type = 999;  // 无效类型

    long ret = init_record_ai(&rec);

    TEST_ASSERT_EQUAL(S_db_badField, ret);
    TEST_ASSERT_NULL(rec.dpvt);
}

void test_init_record_invalid_format(void) {
    aiRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.inp.type = INST_IO;
    strcpy(rec.inp.instio.string, "@invalid");

    long ret = init_record_ai(&rec);

    TEST_ASSERT_EQUAL(S_db_badField, ret);
    TEST_ASSERT_NULL(rec.dpvt);
}

void test_init_record_different_values(void) {
    aiRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.inp.type = INST_IO;
    strcpy(rec.inp.instio.string, "@2 5 1");

    long ret = init_record_ai(&rec);

    TEST_ASSERT_EQUAL(0, ret);

    DevPvt *pPvt = (DevPvt*)rec.dpvt;
    TEST_ASSERT_EQUAL(2, pPvt->offset);
    TEST_ASSERT_EQUAL(5, pPvt->channel);
    TEST_ASSERT_EQUAL(1, pPvt->type);

    free(rec.dpvt);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_record_success);
    RUN_TEST(test_init_record_invalid_link_type);
    RUN_TEST(test_init_record_invalid_format);
    RUN_TEST(test_init_record_different_values);
    return UNITY_END();
}
```

### 案例2: 测试边界条件

```c
// 测试ReadData的所有边界条件
void test_ReadData_boundaries(void) {
    // 最小有效通道
    g_data_buffer[OFFSET_AMP][0] = 1.0;
    TEST_ASSERT_EQUAL_FLOAT(1.0, ReadData(OFFSET_AMP, 0, 0));

    // 最大有效通道
    g_data_buffer[OFFSET_AMP][7] = 2.0;
    TEST_ASSERT_EQUAL_FLOAT(2.0, ReadData(OFFSET_AMP, 7, 0));

    // 刚好超出上界
    TEST_ASSERT_EQUAL_FLOAT(0.0, ReadData(OFFSET_AMP, 8, 0));

    // 刚好超出下界
    TEST_ASSERT_EQUAL_FLOAT(0.0, ReadData(OFFSET_AMP, -1, 0));
}
```

### 案例3: 参数化测试

```c
// 测试多组数据
void test_ReadData_multiple_values(void) {
    struct {
        int offset;
        int channel;
        float expected;
    } test_cases[] = {
        {OFFSET_AMP, 0, 10.0},
        {OFFSET_AMP, 1, 20.0},
        {OFFSET_PHA, 0, 30.0},
        {OFFSET_PHA, 1, 40.0},
    };

    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < num_cases; i++) {
        // 设置测试数据
        g_data_buffer[test_cases[i].offset][test_cases[i].channel] =
            test_cases[i].expected;

        // 执行测试
        float result = ReadData(test_cases[i].offset,
                                test_cases[i].channel, 0);

        // 验证结果
        TEST_ASSERT_EQUAL_FLOAT(test_cases[i].expected, result);
    }
}
```

## 📝 练习任务

### 练习1: 基础测试

为以下函数编写单元测试：
1. `ReadData()` - 至少5个测试用例
2. `SetReg()` - 至少3个测试用例
3. `init_record_ai()` - 至少4个测试用例

### 练习2: Mock测试

1. 创建Mock硬件函数
2. 测试`InitDevice()`是否正确调用硬件初始化
3. 验证错误处理逻辑

### 练习3: TDD实践

使用TDD方式实现一个新功能：
1. 先写测试：测试计算平均值函数
2. 实现代码使测试通过
3. 重构代码提高质量
4. 确保测试仍然通过

### 练习4: 覆盖率

1. 运行测试并生成覆盖率报告
2. 找出未覆盖的代码
3. 添加测试覆盖这些代码

## 🔍 测试最佳实践

### ✅ 好的测试

```c
// 1. 测试名称清晰
void test_ReadData_returns_zero_for_invalid_channel(void) {
    float result = ReadData(OFFSET_AMP, 999, 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0, result);
}

// 2. 使用AAA模式（Arrange-Act-Assert）
void test_ReadData_reads_correct_value(void) {
    // Arrange: 准备
    g_data_buffer[OFFSET_AMP][0] = 12.5;

    // Act: 执行
    float result = ReadData(OFFSET_AMP, 0, 0);

    // Assert: 验证
    TEST_ASSERT_EQUAL_FLOAT(12.5, result);
}

// 3. 一个测试只测一件事
void test_ReadData_channel_0(void) {
    g_data_buffer[OFFSET_AMP][0] = 10.0;
    TEST_ASSERT_EQUAL_FLOAT(10.0, ReadData(OFFSET_AMP, 0, 0));
}

void test_ReadData_channel_1(void) {
    g_data_buffer[OFFSET_AMP][1] = 20.0;
    TEST_ASSERT_EQUAL_FLOAT(20.0, ReadData(OFFSET_AMP, 1, 0));
}
```

### ❌ 不好的测试

```c
// 1. 测试名称不清楚
void test_1(void) {  // 测什么？
    // ...
}

// 2. 测试多个功能
void test_everything(void) {
    // 测试ReadData
    // 测试SetReg
    // 测试InitDevice
    // ... 太多了！
}

// 3. 没有断言
void test_ReadData(void) {
    ReadData(OFFSET_AMP, 0, 0);
    // 没有验证结果！
}

// 4. 魔法数字
void test_ReadData(void) {
    g_data_buffer[0][0] = 12.345;  // 0是什么？12.345代表什么？
    float result = ReadData(0, 0, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 12.345, result);
}
```

## 📚 参考资源

- **Unity**: https://github.com/ThrowTheSwitch/Unity
- **TDD**: https://www.amazon.com/Test-Driven-Development-Kent-Beck/dp/0321146530
- **C单元测试**: https://throwtheswitch.org/
- **EPICS测试**: https://epics-controls.org/resources-and-support/documents/appdev/

## 🔗 相关文档

- **[05-integration-testing.md](./05-integration-testing.md)** - 集成测试
- **[Part 19: 模拟器教程](../part19-simulator-tutorial/)** - Mock库实现
- **[simulator/](../../simulator/)** - 完整Mock库

---

**下一步**: 学习 [集成测试](./05-integration-testing.md)，测试整个IOC系统！
