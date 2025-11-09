# 案例6: 代码重构案例

> **项目**: 重构遗留BPM IOC代码
> **时长**: 3天（评估0.5天 + 测试覆盖1天 + 重构1天 + 验证0.5天）
> **难度**: ⭐⭐⭐⭐
> **关键技术**: 重构模式、单元测试、Git分支策略

## 1. 背景

### 1.1 遗留代码现状

接手一个5年前开发的BPM IOC，代码存在诸多问题：

```c
// legacy_driver.c (简化版本)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// 全局变量遍地
float amp[8], pha[8], q[8], i[8];
int ch_num = 8;
pthread_t tid;
int running = 1;
void *handle;

// 500行的初始化函数
int init() {
    printf("init start\n");
    handle = dlopen("/opt/lib/libBPM.so", 2);  // magic number
    if (!handle) {
        printf("err\n");
        return -1;
    }
    void (*f1)() = dlsym(handle, "init");
    if (f1) f1();
    void (*f2)(int) = dlsym(handle, "setChannels");
    if (f2) f2(8);
    // ... 还有400行 ...
    pthread_create(&tid, NULL, thread_func, NULL);
    return 0;
}

// 混乱的线程函数
void *thread_func(void *arg) {
    float (*read_func)(int, int);
    read_func = dlsym(handle, "readADC");
    while (running) {
        for (int i = 0; i < ch_num; i++) {
            amp[i] = read_func(i, 0);
            pha[i] = read_func(i, 2);
            q[i] = read_func(i, 4);
            // 直接操作全局变量，无锁
        }
        usleep(100000);
    }
    return NULL;
}

// 读取函数：直接返回全局变量
float getData(int offset, int ch) {
    if (offset == 0) return amp[ch];
    if (offset == 2) return pha[ch];
    if (offset == 4) return q[ch];
    return 0;
}

// 没有cleanup函数
// 没有错误处理
// 没有日志
// 没有注释
```

### 1.2 问题清单

| 问题类型 | 具体问题 | 影响 |
|----------|----------|------|
| **架构** | 无模块划分，所有代码在一个文件 | 难以理解和维护 |
| **全局变量** | 大量全局变量，无封装 | 线程不安全 |
| **Magic Number** | `dlopen(..., 2)`等硬编码 | 可读性差 |
| **错误处理** | 仅`printf("err")`，无详细信息 | 调试困难 |
| **内存管理** | 无cleanup，资源泄漏 | 长期运行不稳定 |
| **代码重复** | 多处相同的读取逻辑 | 维护成本高 |
| **函数过长** | `init()`超过500行 | 难以测试 |
| **无测试** | 没有任何单元测试 | 修改风险大 |

### 1.3 重构目标

1. **可维护性**: 模块化、清晰的职责划分
2. **可靠性**: 线程安全、错误处理完善
3. **可测试性**: 单元测试覆盖率>80%
4. **向后兼容**: 不改变外部接口（API）

## 2. 重构策略

### 2.1 分步重构计划

```
Phase 1: 建立测试覆盖（1天）
  └─ 编写集成测试
  └─ 建立回归测试基线

Phase 2: 提取函数（0.5天）
  └─ 拆分长函数
  └─ 消除代码重复

Phase 3: 引入数据结构（0.5天）
  └─ 封装全局变量
  └─ 添加访问接口

Phase 4: 改进错误处理（0.5天）
  └─ 统一错误码
  └─ 添加日志

Phase 5: 线程安全（0.5天）
  └─ 添加互斥锁
  └─ 消除竞态条件

Phase 6: 清理和文档（0.5天）
  └─ 消除magic number
  └─ 添加注释和文档
```

### 2.2 重构原则

1. **每次只做一个改动**
2. **每次改动后运行测试**
3. **保持功能不变**
4. **使用Git记录每个步骤**

## 3. 重构实施

### 3.1 Phase 1: 建立测试覆盖

#### 集成测试

```python
# test/test_legacy_integration.py
import epics
import time
import subprocess
import pytest

@pytest.fixture(scope="module")
def ioc_process():
    """启动IOC"""
    proc = subprocess.Popen(['./st.cmd'],
                           cwd='../../iocBoot/iocBPMmonitor')
    time.sleep(3)
    yield proc
    proc.terminate()
    proc.wait()

def test_basic_connectivity(ioc_process):
    """测试PV连接"""
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp', connection_timeout=2.0)
    assert pv.connected, "PV not connected"

def test_data_range(ioc_process):
    """测试数据范围"""
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')
    value = pv.get()
    assert 0 <= value <= 30, f"Value out of range: {value}"

def test_all_channels(ioc_process):
    """测试所有通道"""
    for ch in range(1, 9):
        pv_amp = epics.PV(f'LLRF:BPM:RFIn_{ch:02d}_Amp')
        pv_pha = epics.PV(f'LLRF:BPM:RFIn_{ch:02d}_Phase')

        assert pv_amp.connected
        assert pv_pha.connected

        amp = pv_amp.get()
        pha = pv_pha.get()

        assert 0 <= amp <= 30
        assert -180 <= pha <= 180

def test_update_rate(ioc_process):
    """测试更新率"""
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

    values = []
    for _ in range(20):
        values.append(pv.get())
        time.sleep(0.1)

    # 检查值在变化（不是静态）
    assert len(set(values)) > 1, "PV not updating"
```

运行建立基线：

```bash
pytest test/test_legacy_integration.py -v

# 保存测试结果为基线
pytest test/ > baseline_results.txt
```

### 3.2 Phase 2: 提取函数

#### 重构：拆分init函数

**Before**:
```c
int init() {
    // 500行代码...
}
```

**After**:
```c
static void* LoadDriverLibrary(const char *lib_path) {
    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "ERROR: Cannot load %s: %s\n",
                lib_path, dlerror());
        return NULL;
    }
    return handle;
}

static int InitializeHardware(void *lib_handle) {
    void (*init_func)() = dlsym(lib_handle, "init");
    if (!init_func) {
        fprintf(stderr, "ERROR: Cannot find init function\n");
        return -1;
    }
    init_func();
    return 0;
}

static int ConfigureChannels(void *lib_handle, int num_channels) {
    void (*set_channels)(int) = dlsym(lib_handle, "setChannels");
    if (!set_channels) {
        fprintf(stderr, "ERROR: Cannot find setChannels function\n");
        return -1;
    }
    set_channels(num_channels);
    return 0;
}

static int StartAcquisitionThread() {
    int ret = pthread_create(&tid, NULL, thread_func, NULL);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Cannot create thread: %s\n",
                strerror(ret));
        return -1;
    }
    return 0;
}

int init() {
    const char *lib_path = "/opt/lib/libBPM.so";

    // 1. 加载库
    handle = LoadDriverLibrary(lib_path);
    if (!handle) return -1;

    // 2. 初始化硬件
    if (InitializeHardware(handle) != 0) return -1;

    // 3. 配置通道
    if (ConfigureChannels(handle, 8) != 0) return -1;

    // 4. 启动采集线程
    if (StartAcquisitionThread() != 0) return -1;

    printf("Initialization successful\n");
    return 0;
}
```

**提交**:
```bash
git add .
git commit -m "refactor: extract functions from init()"
pytest test/  # 验证功能未破坏
```

#### 重构：消除代码重复

**Before**:
```c
float getData(int offset, int ch) {
    if (offset == 0) return amp[ch];
    if (offset == 2) return pha[ch];
    if (offset == 4) return q[ch];
    return 0;
}
```

**After**:
```c
// 统一的数据结构
static float g_data[MAX_OFFSETS][MAX_CHANNELS];

#define OFFSET_AMP 0
#define OFFSET_PHA 2
#define OFFSET_Q   4

float getData(int offset, int ch) {
    if (ch < 0 || ch >= MAX_CHANNELS) {
        fprintf(stderr, "ERROR: Invalid channel %d\n", ch);
        return 0.0;
    }

    if (offset >= MAX_OFFSETS) {
        fprintf(stderr, "ERROR: Invalid offset %d\n", offset);
        return 0.0;
    }

    return g_data[offset][ch];
}
```

### 3.3 Phase 3: 引入数据结构

#### 封装全局状态

**Before**:
```c
float amp[8], pha[8], q[8];
pthread_t tid;
int running;
void *handle;
```

**After**:
```c
// driverState.h
typedef struct {
    // 数据
    float data[MAX_OFFSETS][MAX_CHANNELS];

    // 线程控制
    pthread_t acquire_thread;
    volatile int running;

    // 同步
    pthread_mutex_t data_mutex;

    // 驱动库
    void *lib_handle;
    struct {
        float (*read_adc)(int channel, int offset);
        void (*set_reg)(unsigned int addr, unsigned int value);
    } hw_functions;

    // 配置
    int num_channels;
    int update_interval_us;
} DriverState;

// 单例实例
static DriverState g_driver_state = {
    .running = 0,
    .data_mutex = PTHREAD_MUTEX_INITIALIZER,
    .num_channels = 8,
    .update_interval_us = 100000
};

// 访问接口
DriverState* GetDriverState() {
    return &g_driver_state;
}
```

#### 重构函数使用新结构

**Before**:
```c
void *thread_func(void *arg) {
    float (*read_func)(int, int) = dlsym(handle, "readADC");
    while (running) {
        for (int i = 0; i < ch_num; i++) {
            amp[i] = read_func(i, 0);
            // ...
        }
        usleep(100000);
    }
}
```

**After**:
```c
void *AcquireThread(void *arg) {
    DriverState *state = GetDriverState();

    while (state->running) {
        pthread_mutex_lock(&state->data_mutex);

        for (int ch = 0; ch < state->num_channels; ch++) {
            for (int off = 0; off < MAX_OFFSETS; off++) {
                state->data[off][ch] =
                    state->hw_functions.read_adc(ch, off);
            }
        }

        pthread_mutex_unlock(&state->data_mutex);
        usleep(state->update_interval_us);
    }

    return NULL;
}
```

### 3.4 Phase 4: 改进错误处理

#### 统一错误码

```c
// error.h
typedef enum {
    ERR_OK = 0,
    ERR_INVALID_PARAM = -1,
    ERR_LIBRARY_LOAD = -2,
    ERR_SYMBOL_NOT_FOUND = -3,
    ERR_HARDWARE_INIT = -4,
    ERR_THREAD_CREATE = -5,
    ERR_MUTEX_LOCK = -6
} ErrorCode;

const char* GetErrorString(ErrorCode err);

// error.c
const char* GetErrorString(ErrorCode err) {
    switch (err) {
    case ERR_OK: return "Success";
    case ERR_INVALID_PARAM: return "Invalid parameter";
    case ERR_LIBRARY_LOAD: return "Failed to load library";
    case ERR_SYMBOL_NOT_FOUND: return "Symbol not found";
    case ERR_HARDWARE_INIT: return "Hardware initialization failed";
    case ERR_THREAD_CREATE: return "Thread creation failed";
    case ERR_MUTEX_LOCK: return "Mutex lock failed";
    default: return "Unknown error";
    }
}
```

#### 添加日志系统

```c
// logging.h
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

void Log(LogLevel level, const char *format, ...);

// logging.c
#include <stdarg.h>
#include <time.h>
#include <errlog.h>  // EPICS logging

void Log(LogLevel level, const char *format, ...) {
    const char *level_str[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
    };

    char timestamp[32];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
             localtime(&now));

    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // 输出到EPICS日志
    errlogSevPrintf(
        level >= LOG_ERROR ? errlogFatal : errlogInfo,
        "[%s] %s: %s\n",
        timestamp, level_str[level], message
    );
}
```

#### 重构使用日志

**Before**:
```c
if (!handle) {
    printf("err\n");
    return -1;
}
```

**After**:
```c
if (!handle) {
    Log(LOG_ERROR, "Cannot load library %s: %s",
        lib_path, dlerror());
    return ERR_LIBRARY_LOAD;
}
```

### 3.5 Phase 5: 线程安全

#### 添加锁保护

```c
float ReadData(int offset, int channel) {
    DriverState *state = GetDriverState();

    if (channel < 0 || channel >= state->num_channels) {
        Log(LOG_ERROR, "Invalid channel %d", channel);
        return 0.0;
    }

    if (offset >= MAX_OFFSETS) {
        Log(LOG_ERROR, "Invalid offset %d", offset);
        return 0.0;
    }

    pthread_mutex_lock(&state->data_mutex);
    float value = state->data[offset][channel];
    pthread_mutex_unlock(&state->data_mutex);

    return value;
}
```

### 3.6 Phase 6: 清理和文档

#### 消除Magic Number

**Before**:
```c
dlopen(lib_path, 2);  // 什么是2？
```

**After**:
```c
#include <dlfcn.h>
dlopen(lib_path, RTLD_LAZY);  // 清晰的语义
```

#### 添加文档注释

```c
/**
 * @brief 初始化驱动层
 *
 * 加载硬件驱动库，初始化硬件，启动数据采集线程。
 *
 * @param lib_path 驱动库路径，如"/opt/lib/libBPM.so"
 * @return ErrorCode
 *   - ERR_OK: 成功
 *   - ERR_LIBRARY_LOAD: 库加载失败
 *   - ERR_HARDWARE_INIT: 硬件初始化失败
 *   - ERR_THREAD_CREATE: 线程创建失败
 *
 * @note 此函数应在IOC启动时调用一次
 * @see Cleanup()
 *
 * @par 示例:
 * @code
 * ErrorCode ret = InitDriver("/opt/lib/libBPM.so");
 * if (ret != ERR_OK) {
 *     fprintf(stderr, "Init failed: %s\n", GetErrorString(ret));
 * }
 * @endcode
 */
ErrorCode InitDriver(const char *lib_path);
```

## 4. 重构前后对比

### 4.1 代码指标

| 指标 | 重构前 | 重构后 | 改善 |
|------|--------|--------|------|
| **文件数** | 1 | 5 | 模块化✅ |
| **最长函数** | 512行 | 45行 | -91% ✅ |
| **全局变量** | 15个 | 1个结构体 | 封装✅ |
| **圈复杂度** | 23 | 4 | -82% ✅ |
| **注释覆盖** | 5% | 85% | +80% ✅ |
| **测试覆盖** | 0% | 82% | +82% ✅ |

### 4.2 文件结构对比

**Before**:
```
BPMmonitorApp/src/
└── legacy_driver.c  (1500行)
```

**After**:
```
BPMmonitorApp/src/
├── driverWrapper.c     (主逻辑, 300行)
├── driverState.h       (数据结构, 80行)
├── hardwareAbstraction.c (硬件抽象, 200行)
├── error.c             (错误处理, 50行)
├── logging.c           (日志系统, 100行)
└── test/
    ├── test_driver.c   (单元测试, 400行)
    └── test_integration.py (集成测试, 150行)
```

### 4.3 质量改善

```bash
# 静态分析对比
cppcheck --enable=all src/

# 重构前
[legacy_driver.c:234]: (warning) Possible null pointer dereference: handle
[legacy_driver.c:456]: (error) Memory leak: data
[legacy_driver.c:567]: (warning) Uninitialized variable: offset
Total: 23 warnings, 5 errors

# 重构后
Total: 0 warnings, 0 errors ✅
```

## 5. 经验教训

### ✅ 成功经验

1. **测试先行**
   - 先建立测试覆盖再重构
   - 每次改动立即测试

2. **小步前进**
   - 一次只做一个改动
   - 每步都可独立验证

3. **Git分支**
   - 每个phase创建分支
   - 便于回滚和review

### ❌ 踩过的坑

1. **过度重构**
   - 最初想一次性重写
   - 导致功能破坏，花费2天回滚

2. **忽略性能**
   - 添加过多锁导致性能下降20%
   - 后续优化使用读写锁改善

### 💡 最佳实践

1. **重构检查清单**
   - [ ] 测试覆盖建立
   - [ ] 功能验证通过
   - [ ] 性能无明显下降
   - [ ] 静态分析通过
   - [ ] 代码审查完成
   - [ ] 文档更新

2. **何时停止重构**
   - 达到目标质量指标
   - 性能满足要求
   - 测试覆盖>80%
   - 技术债务显著降低

## 6. 持续改进

### 6.1 CI/CD集成

```yaml
# .github/workflows/quality-check.yml
name: Code Quality

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Build
      run: make
    - name: Unit Test
      run: make test
    - name: Coverage
      run: |
        gcov src/*.c
        gcovr -r . --xml > coverage.xml
    - name: Static Analysis
      run: cppcheck --enable=all --xml src/ 2> cppcheck.xml
    - name: Upload Coverage
      uses: codecov/codecov-action@v2
      with:
        files: ./coverage.xml
```

### 6.2 技术债务追踪

```markdown
# TECHNICAL_DEBT.md

## 已解决
- [x] 消除全局变量 (2025-11-05)
- [x] 拆分长函数 (2025-11-06)
- [x] 添加线程安全 (2025-11-07)

## 待解决（按优先级）
- [ ] P1: 添加配置文件支持（硬编码路径）
- [ ] P2: 实现优雅退出（cleanup不完整）
- [ ] P3: 性能优化（减少锁粒度）
```

## 🔗 相关资源

- [Refactoring: Improving the Design of Existing Code](https://martinfowler.com/books/refactoring.html)
- [Working Effectively with Legacy Code](https://www.oreilly.com/library/view/working-effectively-with/0131177052/)
- [Part 16: 最佳实践](../part16-best-practices/)
- [Part 10: 单元测试](../part10-debugging-testing/04-unit-testing.md)
