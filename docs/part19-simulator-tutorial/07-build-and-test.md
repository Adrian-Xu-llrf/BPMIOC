# Part 19.7: 编译和测试Mock库

> **目标**: 学会编译、测试和验证Mock库
> **难度**: ⭐⭐⭐☆☆
> **时间**: 1小时
> **前置**: 已完成05-complete-mock-implementation.md

## 📖 内容概览

本文档提供完整的Mock库编译和测试流程：
- 目录结构准备
- 编译步骤
- 单元测试
- 集成测试
- 故障排查

完成本文档后，你将拥有一个可用的`libbpm_mock.so`库！

---

## 1. 环境准备

### 1.1 检查编译工具

首先确认你的系统有必要的编译工具：

```bash
# 检查GCC
gcc --version
# 应该看到: gcc (Ubuntu ...) 7.5.0 或更高

# 检查Make
make --version
# 应该看到: GNU Make 4.1 或更高

# 检查必要的库
ldconfig -p | grep libm
ldconfig -p | grep libpthread
# 应该看到libm.so和libpthread.so
```

如果缺少工具，安装它们：

```bash
sudo apt-get update
sudo apt-get install build-essential
```

---

### 1.2 创建目录结构

```bash
# 在BPMIOC项目根目录下
cd ~/BPMIOC

# 创建simulator目录结构
mkdir -p simulator/src
mkdir -p simulator/include
mkdir -p simulator/config
mkdir -p simulator/test
mkdir -p simulator/lib
mkdir -p simulator/bin

# 查看结构
tree simulator/
```

**预期输出**:
```
simulator/
├── bin/          # 可执行测试程序
├── config/       # 配置文件
├── include/      # 头文件
├── lib/          # 编译生成的.so文件
├── src/          # 源代码
└── test/         # 测试代码
```

---

## 2. 准备源文件

### 2.1 复制源代码

从Part 19.5文档中复制源代码文件：

**文件列表**:
1. `simulator/include/libbpm_mock.h` - 头文件
2. `simulator/src/libbpm_mock.c` - Mock库实现
3. `simulator/test/test_mock.c` - 测试程序
4. `simulator/src/Makefile` - 编译脚本
5. `simulator/config/mock_config.ini` - 配置文件

---

### 2.2 创建Makefile

在`simulator/src/Makefile`中：

```makefile
# BPMIOC Mock Library Makefile

# 编译器和标志
CC = gcc
CFLAGS = -fPIC -Wall -O2 -g -I../include
LDFLAGS = -shared
LIBS = -lm -lpthread

# 目录
SRC_DIR = .
INC_DIR = ../include
LIB_DIR = ../lib
BIN_DIR = ../bin
TEST_DIR = ../test

# 源文件和目标
LIB_SRC = libbpm_mock.c
LIB_OBJ = libbpm_mock.o
LIB_TARGET = $(LIB_DIR)/libbpm_mock.so

TEST_SRC = $(TEST_DIR)/test_mock.c
TEST_TARGET = $(BIN_DIR)/test_mock

# 默认目标
all: dirs $(LIB_TARGET) $(TEST_TARGET)
	@echo "========================================="
	@echo "Build completed successfully!"
	@echo "Library: $(LIB_TARGET)"
	@echo "Test:    $(TEST_TARGET)"
	@echo "========================================="

# 创建必要目录
dirs:
	@mkdir -p $(LIB_DIR) $(BIN_DIR)

# 编译Mock库
$(LIB_TARGET): $(LIB_OBJ)
	@echo "Linking shared library..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Created: $@"

$(LIB_OBJ): $(LIB_SRC) $(INC_DIR)/libbpm_mock.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $<

# 编译测试程序
$(TEST_TARGET): $(TEST_SRC) $(LIB_TARGET)
	@echo "Compiling test program..."
	$(CC) -o $@ $< -I$(INC_DIR) -L$(LIB_DIR) -lbpm_mock -Wl,-rpath,$(shell cd $(LIB_DIR) && pwd)
	@echo "Created: $@"

# 运行测试
test: $(TEST_TARGET)
	@echo "========================================="
	@echo "Running Mock Library Tests..."
	@echo "========================================="
	$(TEST_TARGET)

# 性能测试
perf: $(TEST_TARGET)
	@echo "========================================="
	@echo "Running Performance Tests..."
	@echo "========================================="
	@echo "Calling GetRFInfo 10,000 times..."
	@time -p $(TEST_TARGET) perf

# 清理
clean:
	@echo "Cleaning build artifacts..."
	rm -f *.o
	rm -f $(LIB_DIR)/*.so
	rm -f $(BIN_DIR)/*
	@echo "Clean completed"

# 显示帮助
help:
	@echo "BPMIOC Mock Library Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build library and test program"
	@echo "  make test     - Build and run tests"
	@echo "  make perf     - Run performance tests"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help"
	@echo ""
	@echo "Files:"
	@echo "  Library:  $(LIB_TARGET)"
	@echo "  Test:     $(TEST_TARGET)"

.PHONY: all dirs test perf clean help
```

---

## 3. 编译Mock库

### 3.1 首次编译

```bash
cd ~/BPMIOC/simulator/src

# 查看Makefile帮助
make help

# 执行编译
make
```

**成功输出**:
```
Compiling libbpm_mock.c...
Linking shared library...
Created: ../lib/libbpm_mock.so
Compiling test program...
Created: ../bin/test_mock
=========================================
Build completed successfully!
Library: ../lib/libbpm_mock.so
Test:    ../bin/test_mock
=========================================
```

---

### 3.2 验证编译结果

```bash
# 检查库文件
ls -lh ../lib/
# 应该看到: libbpm_mock.so (约 50-100KB)

# 检查库的符号
nm -D ../lib/libbpm_mock.so | grep GetRFInfo
# 应该看到: 00001a20 T GetRFInfo

# 检查库依赖
ldd ../lib/libbpm_mock.so
# 应该看到:
#   libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6
#   libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0
#   libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6

# 检查测试程序
ls -lh ../bin/
# 应该看到: test_mock (约 20-30KB)
```

---

## 4. 运行测试

### 4.1 基本功能测试

```bash
# 运行测试程序
make test
```

**预期输出**:
```
=========================================
Running Mock Library Tests...
=========================================
../bin/test_mock

=== BPMIOC Mock Library Test ===

Test 1: System Init/Close
SystemInit() = 0
System initialized successfully
SystemClose() = 0

Test 2: RF Data Generation
RF3 Amplitude: 1.02
RF3 Phase: 0.15
RF4 Amplitude: 0.98
RF4 Phase: -0.12
RF5 Amplitude: 1.01
RF5 Phase: 0.05
RF6 Amplitude: 1.00
RF6 Phase: 0.00

Test 3: XY Position Data
XY1: X=0.10mm, Y=0.08mm
XY2: X=-0.05mm, Y=0.12mm
XY3: X=0.02mm, Y=-0.03mm
XY4: X=-0.08mm, Y=0.15mm

Test 4: Button Signal Data
Button1: 1.020
Button2: 0.980
Button3: 1.015
Button4: 0.985

Test 5: Register Access
SetReg(10, 12345) = 0
GetReg(10) = 12345

Test 6: Time Evolution
Time 0.00s: RF3_Amp=1.00
Time 0.10s: RF3_Amp=1.01
Time 0.20s: RF3_Amp=1.02
Time 0.30s: RF3_Amp=1.01
Time 0.40s: RF3_Amp=1.00

=== All Tests Passed! ===
```

---

### 4.2 手动测试

创建一个简单的测试程序：

```bash
# 创建 simple_test.c
cat > ../test/simple_test.c << 'EOF'
#include <stdio.h>
#include <dlfcn.h>

int main() {
    // 加载Mock库
    void *handle = dlopen("../lib/libbpm_mock.so", RTLD_LAZY);
    if (!handle) {
        printf("ERROR: %s\n", dlerror());
        return 1;
    }

    // 获取函数指针
    int (*funcSystemInit)(void) = dlsym(handle, "SystemInit");
    float (*funcGetRFInfo)(int, int) = dlsym(handle, "GetRFInfo");

    // 调用函数
    funcSystemInit();
    printf("RF3 Amplitude: %.2f\n", funcGetRFInfo(3, 0));
    printf("RF3 Phase: %.2f\n", funcGetRFInfo(3, 1));

    dlclose(handle);
    return 0;
}
EOF

# 编译并运行
gcc -o ../bin/simple_test ../test/simple_test.c -ldl
../bin/simple_test
```

**输出**:
```
RF3 Amplitude: 1.02
RF3 Phase: 0.15
```

---

## 5. 性能测试

### 5.1 运行性能测试

```bash
make perf
```

**预期输出**:
```
=========================================
Running Performance Tests...
=========================================
Calling GetRFInfo 10,000 times...
real 0.02
user 0.01
sys 0.00
```

**分析**:
- 10,000次调用耗时 ~0.02秒
- 平均每次调用: 2微秒
- **结论**: 性能完全满足BPMIOC需求（10 Hz = 100ms周期）

---

### 5.2 详细性能测试

创建更详细的性能测试：

```bash
cat > ../test/perf_test.c << 'EOF'
#include <stdio.h>
#include <sys/time.h>
#include "../include/libbpm_mock.h"

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main() {
    SystemInit();

    const int N = 100000;
    double start, end;

    // 测试GetRFInfo
    start = get_time_ms();
    for (int i = 0; i < N; i++) {
        GetRFInfo(3, 0);
    }
    end = get_time_ms();
    printf("GetRFInfo: %.2f ms for %d calls (%.2f us/call)\n",
           end - start, N, (end - start) * 1000.0 / N);

    // 测试GetXYPosition
    start = get_time_ms();
    for (int i = 0; i < N; i++) {
        GetXYPosition(0);
    }
    end = get_time_ms();
    printf("GetXYPosition: %.2f ms for %d calls (%.2f us/call)\n",
           end - start, N, (end - start) * 1000.0 / N);

    // 测试GetButton
    start = get_time_ms();
    for (int i = 0; i < N; i++) {
        GetButton(0);
    }
    end = get_time_ms();
    printf("GetButton: %.2f ms for %d calls (%.2f us/call)\n",
           end - start, N, (end - start) * 1000.0 / N);

    // 测试完整的数据采集周期
    start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        TriggerAllDataReached();
        // 模拟读取所有数据
        for (int ch = 3; ch <= 6; ch++) {
            GetRFInfo(ch, 0);
            GetRFInfo(ch, 1);
        }
        for (int ch = 0; ch < 8; ch++) {
            GetXYPosition(ch);
        }
        for (int ch = 0; ch < 4; ch++) {
            GetButton(ch);
        }
    }
    end = get_time_ms();
    printf("\nFull acquisition cycle: %.2f ms for 1000 cycles (%.2f ms/cycle)\n",
           end - start, (end - start) / 1000.0);
    printf("=> Can support up to %.0f Hz update rate\n",
           1000.0 / ((end - start) / 1000.0));

    SystemClose();
    return 0;
}
EOF

# 编译并运行
gcc -o ../bin/perf_test ../test/perf_test.c \
    -I../include -L../lib -lbpm_mock -lm \
    -Wl,-rpath,../lib

../bin/perf_test
```

**预期输出**:
```
GetRFInfo: 124.35 ms for 100000 calls (1.24 us/call)
GetXYPosition: 98.72 ms for 100000 calls (0.99 us/call)
GetButton: 45.23 ms for 100000 calls (0.45 us/call)

Full acquisition cycle: 26.45 ms for 1000 cycles (0.03 ms/cycle)
=> Can support up to 37800 Hz update rate
```

**结论**: Mock库性能远超BPMIOC的10 Hz需求！

---

## 6. 集成测试

### 6.1 测试与driverWrapper的接口兼容性

创建模拟driverWrapper的测试：

```bash
cat > ../test/integration_test.c << 'EOF'
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

// 模拟driverWrapper的函数指针
static int (*funcSystemInit)(void);
static int (*funcSystemClose)(void);
static int (*funcTriggerAllDataReached)(void);
static float (*funcGetRFInfo)(int channel, int type);
static float (*funcGetXYPosition)(int channel);
static float (*funcGetButton)(int channel);

int main() {
    printf("=== Integration Test: driverWrapper Interface ===\n\n");

    // 1. 动态加载Mock库（模拟driverWrapper的dlopen）
    void *handle = dlopen("../lib/libbpm_mock.so", RTLD_LAZY);
    if (!handle) {
        printf("ERROR: Cannot load library: %s\n", dlerror());
        return 1;
    }
    printf("✓ Library loaded successfully\n");

    // 2. 获取所有函数指针（模拟driverWrapper的dlsym）
    funcSystemInit = dlsym(handle, "SystemInit");
    funcSystemClose = dlsym(handle, "SystemClose");
    funcTriggerAllDataReached = dlsym(handle, "TriggerAllDataReached");
    funcGetRFInfo = dlsym(handle, "GetRFInfo");
    funcGetXYPosition = dlsym(handle, "GetXYPosition");
    funcGetButton = dlsym(handle, "GetButton");

    if (!funcSystemInit || !funcGetRFInfo) {
        printf("ERROR: Cannot find required functions\n");
        dlclose(handle);
        return 1;
    }
    printf("✓ All function symbols resolved\n");

    // 3. 初始化系统
    if (funcSystemInit() != 0) {
        printf("ERROR: SystemInit failed\n");
        dlclose(handle);
        return 1;
    }
    printf("✓ System initialized\n");

    // 4. 模拟10个采集周期（模拟pthread的行为）
    printf("\n=== Simulating 10 acquisition cycles ===\n");
    for (int cycle = 0; cycle < 10; cycle++) {
        printf("\nCycle %d:\n", cycle);

        // 触发数据采集
        funcTriggerAllDataReached();

        // 读取RF数据
        printf("  RF: ");
        for (int ch = 3; ch <= 6; ch++) {
            float amp = funcGetRFInfo(ch, 0);
            printf("RF%d=%.2f ", ch, amp);
        }
        printf("\n");

        // 读取XY位置
        printf("  XY: ");
        for (int i = 0; i < 4; i++) {
            float x = funcGetXYPosition(i * 2);
            float y = funcGetXYPosition(i * 2 + 1);
            printf("XY%d=(%.2f,%.2f) ", i+1, x, y);
        }
        printf("\n");

        usleep(100000); // 100ms，模拟BPMIOC的10 Hz
    }

    // 5. 关闭系统
    funcSystemClose();
    printf("\n✓ System closed\n");

    // 6. 卸载库
    dlclose(handle);
    printf("✓ Library unloaded\n");

    printf("\n=== Integration Test PASSED ===\n");
    return 0;
}
EOF

# 编译并运行
gcc -o ../bin/integration_test ../test/integration_test.c -ldl
../bin/integration_test
```

---

### 6.2 测试多线程安全性

```bash
cat > ../test/thread_test.c << 'EOF'
#include <stdio.h>
#include <pthread.h>
#include "../include/libbpm_mock.h"

#define NUM_THREADS 4
#define CALLS_PER_THREAD 10000

void *thread_func(void *arg) {
    int thread_id = *(int *)arg;

    for (int i = 0; i < CALLS_PER_THREAD; i++) {
        float val = GetRFInfo(3 + (thread_id % 4), 0);

        if (i % 1000 == 0) {
            printf("Thread %d: iteration %d, value=%.2f\n",
                   thread_id, i, val);
        }
    }

    return NULL;
}

int main() {
    printf("=== Multi-thread Safety Test ===\n");

    SystemInit();

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // 创建线程
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }

    // 等待线程结束
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    SystemClose();

    printf("=== Multi-thread Test PASSED ===\n");
    printf("Total calls: %d × %d = %d\n",
           NUM_THREADS, CALLS_PER_THREAD,
           NUM_THREADS * CALLS_PER_THREAD);

    return 0;
}
EOF

# 编译并运行
gcc -o ../bin/thread_test ../test/thread_test.c \
    -I../include -L../lib -lbpm_mock -lpthread -lm \
    -Wl,-rpath,../lib

../bin/thread_test
```

---

## 7. 故障排查

### 7.1 常见编译错误

#### 错误1: 找不到头文件

**错误信息**:
```
libbpm_mock.c:1:10: fatal error: libbpm_mock.h: No such file or directory
```

**解决方法**:
```bash
# 检查头文件位置
ls -l ../include/libbpm_mock.h

# 确保Makefile中的-I路径正确
grep "CFLAGS.*-I" Makefile
# 应该看到: CFLAGS = -fPIC -Wall -O2 -g -I../include
```

---

#### 错误2: 链接错误

**错误信息**:
```
undefined reference to `sin'
undefined reference to `pthread_create'
```

**解决方法**:
```bash
# 检查Makefile中的库链接
grep "LIBS" Makefile
# 应该看到: LIBS = -lm -lpthread

# 重新编译
make clean && make
```

---

#### 错误3: 运行时找不到库

**错误信息**:
```
./test_mock: error while loading shared libraries: libbpm_mock.so: cannot open shared object file
```

**解决方法**:

```bash
# 方法1: 使用-rpath（推荐，Makefile已包含）
# 编译时: -Wl,-rpath,/path/to/lib

# 方法2: 设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/BPMIOC/simulator/lib
../bin/test_mock

# 方法3: 安装到系统路径（不推荐用于开发）
sudo cp ../lib/libbpm_mock.so /usr/local/lib/
sudo ldconfig
```

---

### 7.2 运行时问题

#### 问题1: 数据不变化

**症状**: GetRFInfo总是返回相同的值

**检查**:
```c
// 确保调用了TriggerAllDataReached
TriggerAllDataReached();  // 增加时间
float val = GetRFInfo(3, 0);
```

---

#### 问题2: 数值异常

**症状**: 返回NaN或inf

**调试**:
```bash
# 使用valgrind检查
valgrind --leak-check=full ../bin/test_mock

# 使用gdb调试
gdb ../bin/test_mock
(gdb) run
(gdb) bt  # 如果崩溃，查看backtrace
```

---

## 8. 验证清单

完成编译和测试后，确认以下项目：

### 编译验证
- [ ] `libbpm_mock.so` 文件存在于 `simulator/lib/`
- [ ] 库大小合理（50-100KB）
- [ ] `nm -D libbpm_mock.so` 显示所有导出函数
- [ ] `ldd libbpm_mock.so` 显示依赖库正确链接

### 功能验证
- [ ] `make test` 所有测试通过
- [ ] GetRFInfo返回合理的RF数据
- [ ] GetXYPosition返回合理的位置数据
- [ ] GetButton返回合理的Button信号
- [ ] 寄存器读写正常工作
- [ ] 时间演化正确（数据随时间变化）

### 性能验证
- [ ] `make perf` 性能测试通过
- [ ] 单次函数调用 < 2微秒
- [ ] 完整采集周期 < 1毫秒
- [ ] 可支持 > 1000 Hz更新率

### 集成验证
- [ ] dlopen/dlsym加载成功
- [ ] 函数指针调用正常
- [ ] 多线程安全（如需要）

---

## 9. 下一步

Mock库编译测试完成后：

1. **学习调试技巧**: [08-debugging-mock.md](./08-debugging-mock.md)
2. **与IOC集成**: [09-integration-with-ioc.md](./09-integration-with-ioc.md)
3. **查阅API文档**: [10-mock-api-reference.md](./10-mock-api-reference.md)

---

## 10. 快速参考

### 常用命令

```bash
# 编译
cd ~/BPMIOC/simulator/src
make

# 测试
make test

# 性能测试
make perf

# 清理
make clean

# 重新编译
make clean && make

# 查看帮助
make help
```

### 目录结构

```
~/BPMIOC/simulator/
├── src/
│   ├── libbpm_mock.c         # 源代码
│   └── Makefile              # 编译脚本
├── include/
│   └── libbpm_mock.h         # 头文件
├── lib/
│   └── libbpm_mock.so        # 编译生成的库
├── bin/
│   ├── test_mock             # 测试程序
│   ├── perf_test             # 性能测试
│   └── integration_test      # 集成测试
├── test/
│   └── *.c                   # 测试源码
└── config/
    └── mock_config.ini       # 配置文件
```

---

**🎯 重要**: 确保所有测试都通过后，再继续学习如何将Mock库集成到BPMIOC IOC中！
