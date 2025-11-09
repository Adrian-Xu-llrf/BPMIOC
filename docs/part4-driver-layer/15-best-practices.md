# 驱动层开发最佳实践

> **阅读时间**: 20分钟
> **难度**: ⭐⭐⭐☆☆

## 📋 本文目标

- 掌握驱动层开发的最佳实践
- 避免常见陷阱
- 提高代码质量

## 1. 代码组织

### 1.1 文件结构

```c
// ✅ 推荐的文件结构
// driverWrapper.c

// 1. 头文件包含
#include <stdio.h>
#include <epicsTypes.h>
// ...

// 2. 宏定义
#define buf_len 10000
#define REG_NUM 100

// 3. 类型定义
typedef struct { ... } MyStruct;

// 4. 全局变量声明
static float rf3amp[buf_len];
static int Reg[REG_NUM];

// 5. 函数声明
static void initAllBuffers(void);
static float calculateAverage(const float *data, int len);

// 6. 核心函数实现
long InitDevice() { ... }
float ReadData(...) { ... }

// 7. 辅助函数实现
static void initAllBuffers(void) { ... }
```

### 1.2 命名规范

```c
// ✅ 好的命名
float GetRFInfo(int channel, int type);      // 清晰、描述性
static void initAllBuffers(void);            // 驼峰命名
#define BUF_LEN 10000                         // 宏大写

const char *getCurrentTime(void);             // 动词+名词
int isDataReady(void);                        // is/has开头(布尔)

// ❌ 不好的命名
float grf(int c, int t);                      // 缩写不清
void do_stuff();                              // 不描述性
#define Buf_Len 10000                         // 混合大小写
```

### 1.3 注释规范

```c
/**
 * 读取标量数据
 *
 * @param offset  数据类型 (0-29)
 * @param channel 通道号 (0-7)
 * @param type    子类型 (AMP/PHASE/REAL/IMAG)
 * @return        数据值
 *
 * 示例:
 *   float amp = ReadData(0, 3, AMP);  // RF3幅度
 */
float ReadData(int offset, int channel, int type)
{
    // ...
}

// 复杂逻辑添加注释
switch (offset) {
    case 0:  // RF信息
        // 计算实部: A * cos(φ)
        if (type == REAL) {
            float amp = funcGetRFInfo(channel, 0);
            float phase = funcGetRFInfo(channel, 1);
            return amp * cos(phase * M_PI / 180.0);
        }
        break;
}
```

## 2. 错误处理

### 2.1 参数验证

```c
// ✅ 严格的参数验证
float ReadData(int offset, int channel, int type)
{
    // 1. offset范围检查
    if (offset < 0 || offset > 28) {
        fprintf(stderr, "ERROR: Invalid offset: %d (range: 0-28)\n", offset);
        return 0.0;
    }

    // 2. channel范围检查
    if (offset == 0) {  // RF
        if (channel < 3 || channel > 6) {
            fprintf(stderr, "ERROR: Invalid RF channel: %d (range: 3-6)\n", channel);
            return 0.0;
        }
    }

    // 3. type范围检查
    if (type < 0 || type > 3) {
        fprintf(stderr, "ERROR: Invalid type: %d\n", type);
        return 0.0;
    }

    // 4. 指针检查
    if (funcGetRFInfo == NULL) {
        fprintf(stderr, "ERROR: funcGetRFInfo is NULL\n");
        return 0.0;
    }

    // 正常处理
    return funcGetRFInfo(channel, type);
}
```

### 2.2 函数返回值检查

```c
// ✅ 检查返回值
long InitDevice()
{
    // dlopen
    handle = dlopen(dll_filename, RTLD_LAZY);
    if (handle == NULL) {
        fprintf(stderr, "ERROR: dlopen failed: %s\n", dlerror());
        return -1;
    }

    // pthread_create
    int ret = pthread_create(&tidp1, NULL, pthread, NULL);
    if (ret != 0) {
        fprintf(stderr, "ERROR: pthread_create failed: %d\n", ret);
        dlclose(handle);  // 清理
        return -1;
    }

    // 硬件初始化
    if (funcSystemInit != NULL) {
        ret = funcSystemInit();
        if (ret != 0) {
            fprintf(stderr, "ERROR: SystemInit failed: %d\n", ret);
            dlclose(handle);
            return -1;
        }
    }

    return 0;
}
```

### 2.3 资源清理

```c
// ✅ 正确的资源管理
long InitDevice()
{
    // 分配资源
    handle = dlopen(...);
    if (handle == NULL) {
        return -1;
    }

    // 出错时清理
    if (pthread_create(...) != 0) {
        dlclose(handle);  // 清理已分配的资源
        handle = NULL;
        return -1;
    }

    return 0;
}

void ShutdownDevice()
{
    // 停止线程
    thread_should_exit = 1;
    pthread_join(tidp1, NULL);

    // 关闭硬件
    if (funcSystemClose != NULL) {
        funcSystemClose();
    }

    // 关闭库
    if (handle != NULL) {
        dlclose(handle);
        handle = NULL;
    }
}
```

## 3. 性能优化

### 3.1 避免重复计算

```c
// ❌ 不好 - 重复计算
float value1 = funcGetRFInfo(3, 0);
float value2 = funcGetRFInfo(3, 0);  // 重复调用
float avg = (value1 + value2) / 2.0;

// ✅ 好 - 缓存结果
float value = funcGetRFInfo(3, 0);
float avg = value;  // 直接使用
```

### 3.2 使用合适的数据类型

```c
// ❌ 不必要的double
static double rf3amp[buf_len];  // 占用更多内存

// ✅ float足够
static float rf3amp[buf_len];
```

### 3.3 批量操作

```c
// ❌ 逐个拷贝
for (int i = 0; i < buf_len; i++) {
    buf[i] = rf3amp[i];
}

// ✅ 批量拷贝
memcpy(buf, rf3amp, buf_len * sizeof(float));
```

## 4. 线程安全

### 4.1 全局变量访问

```c
// BPMIOC的选择：无锁设计
// 原因：写入频率低(10Hz)，读取时间短，碰撞概率极低

// 如需线程安全：
static epicsMutexId bufferLock;

void *pthread(void *arg)
{
    while (1) {
        epicsMutexLock(bufferLock);
        funcTriggerAllDataReached();  // 更新buffer
        epicsMutexUnlock(bufferLock);

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}

float ReadData(...)
{
    epicsMutexLock(bufferLock);
    float value = funcGetRFInfo(channel, type);
    epicsMutexUnlock(bufferLock);

    return value;
}
```

### 4.2 避免竞态条件

```c
// ❌ 竞态条件
static int counter = 0;

void increment() {
    counter++;  // 非原子操作
}

// ✅ 原子操作
static volatile unsigned long counter = 0;

void increment() {
    __sync_fetch_and_add(&counter, 1);  // GCC原子操作
}
```

## 5. 调试友好

### 5.1 可控的调试输出

```c
// ✅ 可控制的调试级别
static int debug_level = 0;  // Reg[99]

#define DEBUG_PRINT(level, fmt, ...) \
    do { \
        if (debug_level >= level) { \
            printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        } \
    } while(0)

float ReadData(int offset, int channel, int type)
{
    DEBUG_PRINT(2, "ReadData(offset=%d, ch=%d, type=%d)", offset, channel, type);

    // ...

    DEBUG_PRINT(3, "  -> value=%.3f", value);
    return value;
}
```

### 5.2 断言检查

```c
#include <assert.h>

float ReadData(int offset, int channel, int type)
{
    assert(offset >= 0 && offset <= 28);
    assert(channel >= 0 && channel <= 7);

    // ... 正常处理 ...
}
```

### 5.3 日志记录

```c
// 记录关键事件
void logEvent(const char *event)
{
    FILE *fp = fopen("bpm.log", "a");
    if (fp != NULL) {
        fprintf(fp, "[%s] %s\n", getCurrentTime(), event);
        fclose(fp);
    }
}

long InitDevice()
{
    logEvent("InitDevice started");

    // ... 初始化 ...

    logEvent("InitDevice completed successfully");
    return 0;
}
```

## 6. 可维护性

### 6.1 避免魔法数字

```c
// ❌ 魔法数字
if (offset == 7) { ... }
usleep(100000);

// ✅ 使用宏
#define OFFSET_XY 7
#define ACQUISITION_PERIOD_US 100000

if (offset == OFFSET_XY) { ... }
usleep(ACQUISITION_PERIOD_US);
```

### 6.2 提取重复逻辑

```c
// ❌ 重复代码
case 0:
    if (funcGetRFInfo == NULL) return 0.0;
    return funcGetRFInfo(channel, type);

case 1:
    if (funcGetCenterFreq == NULL) return 0.0;
    return funcGetCenterFreq();

// ✅ 提取为辅助函数
static float callIfNotNull(void *func_ptr, ...)
{
    if (func_ptr == NULL) return 0.0;
    // ... 调用函数 ...
}
```

### 6.3 模块化设计

```c
// 按功能分组
// === RF相关函数 ===
static float readRFData(int channel, int type);
static void updateRFBuffers(void);

// === XY相关函数 ===
static float readXYData(int channel);
static void updateXYBuffers(void);

// === 系统管理函数 ===
static void systemInit(void);
static void systemShutdown(void);
```

## 7. 文档化

### 7.1 函数文档

```c
/**
 * 初始化设备驱动
 *
 * 完成以下步骤:
 * 1. 初始化IOSCANPVT
 * 2. 加载硬件库 (dlopen)
 * 3. 获取硬件函数指针 (dlsym)
 * 4. 初始化硬件 (funcSystemInit)
 * 5. 创建数据采集线程
 *
 * @return 0=成功, -1=失败
 *
 * @note 只应在IOC启动时调用一次
 * @see ShutdownDevice()
 */
long InitDevice(void);
```

### 7.2 代码内注释

```c
// 环形缓冲区：从history_index开始读取，保证时间连续性
for (int i = 0; i < trip_buf_len; i++) {
    int idx = (history_index + i) % trip_buf_len;
    buf[i] = HistoryX1[idx];
}
```

### 7.3 TODO标记

```c
// TODO: 添加温度传感器支持 (2025-01-15, Adrian)
// FIXME: pthread退出机制不完善
// XXX: 这里的性能可能有问题
```

## 8. 测试

### 8.1 单元测试

```c
// test_driver.c
void test_calculateAverage()
{
    float data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    float avg = calculateAverage(data, 5);

    assert(fabs(avg - 3.0) < 0.001);
    printf("test_calculateAverage: PASSED\n");
}

int main()
{
    test_calculateAverage();
    // 更多测试...
    return 0;
}
```

### 8.2 集成测试

```bash
#!/bin/bash
# test_integration.sh

# 启动IOC
./st.cmd &
IOC_PID=$!

sleep 5

# 测试PV访问
caget LLRF:BPM:RF3Amp || exit 1
caget LLRF:BPM:X1 || exit 1

# 停止IOC
kill $IOC_PID

echo "All tests passed"
```

## 9. 安全性

### 9.1 输入验证

```c
// 永远不要信任外部输入
long SetReg(int addr, int value)
{
    // 范围检查
    if (addr < 0 || addr >= REG_NUM) {
        return -1;
    }

    // 值检查
    if (addr == 0 && (value != 0 && value != 1)) {
        return -1;  // Reg[0]只能是0或1
    }

    // ...
}
```

### 9.2 缓冲区溢出防护

```c
// ✅ 检查长度
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    if (buf == NULL || len == NULL) {
        return -1;
    }

    int max_len = buf_len;  // 最大长度

    // 不超过缓冲区
    int copy_len = (*len < max_len) ? *len : max_len;

    memcpy(buf, rf3amp, copy_len * sizeof(float));
    *len = copy_len;

    return 0;
}
```

## 10. 清单

### 10.1 提交前检查

```
☐ 代码编译无warning
☐ 所有函数有注释
☐ 添加了错误处理
☐ 没有魔法数字
☐ 通过所有测试
☐ 更新了文档
☐ Git commit message清晰
```

### 10.2 Code Review清单

```
☐ 代码风格一致
☐ 命名清晰
☐ 注释充分
☐ 错误处理完整
☐ 没有内存泄漏
☐ 线程安全
☐ 性能可接受
```

## 📚 延伸阅读

- C编程最佳实践
- EPICS Application Developer's Guide
- Linux Programming Best Practices

## 🎓 本章总结

- ✅ 遵循一致的代码规范
- ✅ 严格的错误处理和参数验证
- ✅ 性能和可维护性并重
- ✅ 充分的文档和测试
- ✅ 安全第一

**核心原则**: Clean Code + Defensive Programming = Quality

---

**建议**: 将这些最佳实践打印出来，贴在显示器旁边！
