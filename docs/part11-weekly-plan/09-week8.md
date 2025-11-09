# Week 8: 独立项目（Independent Project）

> **学习目标**: 从零开发一个完整的IOC项目
> **关键词**: 独立开发、完整项目、毕业设计
> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 15-20小时

## 📅 本周概览

```
周一: 项目设计
  ├─ 需求分析
  ├─ 架构设计
  └─ 接口定义

周二-周四: 项目实现
  ├─ Mock库开发
  ├─ 驱动层实现
  ├─ 设备支持层实现
  └─ 数据库设计

周五: 测试和文档
  ├─ 单元测试
  ├─ 集成测试
  └─ 文档编写

周末: 答辩准备
  ├─ 整理成果
  └─ 准备演示
```

---

## 🎯 Week 8学习目标

### 最终目标
- [ ] 独立完成一个IOC项目
- [ ] 实现Mock库支持PC开发
- [ ] 代码质量符合规范
- [ ] 功能正常运行
- [ ] 文档完整清晰

---

## 🚀 项目选择

### 推荐项目1: 多通道温度监控系统

**需求**:
- 8个温度传感器（0-100°C）
- 支持温度报警（上下限）
- 历史数据记录（最大/最小/平均）
- 温度趋势分析
- Web监控界面

**技术要点**:
- 多通道数据采集
- 报警处理
- 统计计算
- 数据归档

---

### 推荐项目2: 电源监控系统

**需求**:
- 4路电压监控（0-24V）
- 4路电流监控（0-10A）
- 功率计算和显示
- 过流保护
- 远程开关控制

**技术要点**:
- 模拟量采集
- 数字量控制
- 计算Record
- 联锁保护

---

### 推荐项目3: 步进电机控制系统

**需求**:
- 位置读取和控制
- 速度设置
- 限位开关检测
- 自动回零功能
- 运动轨迹记录

**技术要点**:
- 运动控制
- 状态机设计
- 位置反馈
- 轨迹规划

---

## 📚 Day-by-Day Schedule

### Day 1 (Monday) - 项目设计

**任务**:
1. 选择项目（温度监控系统）
2. 需求分析
3. 架构设计
4. 接口定义

**设计文档**:

```markdown
# 温度监控IOC设计文档

## 1. 需求分析
- 功能需求：
  * 8通道温度采集（0-100°C, 1°C精度）
  * 采样率：1 Hz
  * 温度报警：可配置上下限
  * 历史数据：最大/最小/平均值

- 性能需求：
  * 响应时间：< 1秒
  * 数据更新率：1 Hz
  * 可靠性：7x24小时运行

## 2. 架构设计

### 三层架构
```
数据库层: 40个PV
  ├─ Temp[1-8]:Value (ai)
  ├─ Temp[1-8]:HighLimit (ao)
  ├─ Temp[1-8]:LowLimit (ao)
  ├─ Temp[1-8]:Max (ai)
  └─ Temp[1-8]:Avg (calc)

设备支持层: devTempMonitor.c
  ├─ init_record_ai()
  ├─ read_ai()
  └─ write_ao()

驱动层: drvTempMonitor.c
  ├─ SystemInit()
  ├─ ReadTemperature()
  └─ SetAlarmLimit()
```

## 3. 接口定义

### 硬件接口
```c
int SystemInit(void);
float ReadTemperature(int channel);  // channel: 0-7
int SetAlarmLimit(int channel, float high, float low);
```

### PV接口
```
TEMP:CH1:Value          - 温度值 (ai, I/O Intr)
TEMP:CH1:HighLimit      - 上限 (ao)
TEMP:CH1:LowLimit       - 下限 (ao)
TEMP:CH1:Max            - 最大值 (ai)
TEMP:CH1:Min            - 最小值 (ai)
TEMP:CH1:Avg            - 平均值 (calc)
```
```

---

### Day 2 (Tuesday) - Mock库开发

**文件**: `libtempmonitor_mock.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static double g_sim_time = 0.0;
static float g_base_temps[8] = {25, 30, 28, 32, 26, 29, 31, 27};
static float g_alarm_high[8] = {80, 80, 80, 80, 80, 80, 80, 80};
static float g_alarm_low[8] = {10, 10, 10, 10, 10, 10, 10, 10};

int SystemInit(void) {
    srand(time(NULL));
    g_sim_time = 0.0;
    printf("Temperature Monitor Mock Library Initialized\n");
    return 0;
}

float ReadTemperature(int channel) {
    if (channel < 0 || channel >= 8) {
        return 0.0f;
    }

    // 模拟温度：基准值 + 慢变化 + 噪声
    float base = g_base_temps[channel];
    float slow_var = 2.0 * sin(2.0 * M_PI * 0.01 * g_sim_time);
    float noise = 0.5 * ((float)rand() / RAND_MAX - 0.5);

    return base + slow_var + noise;
}

int SetAlarmLimit(int channel, float high, float low) {
    if (channel < 0 || channel >= 8) {
        return -1;
    }
    g_alarm_high[channel] = high;
    g_alarm_low[channel] = low;
    return 0;
}

int TriggerDataAcquisition(void) {
    g_sim_time += 1.0;
    return 0;
}
```

**Makefile**:
```makefile
CC = gcc
CFLAGS = -fPIC -Wall -O2
LDFLAGS = -shared -lm

all: libtempmonitor_mock.so

libtempmonitor_mock.so: libtempmonitor_mock.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.so
```

---

### Day 3 (Wednesday) - 驱动层实现

**文件**: `drvTempMonitor.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <scanIo.h>

// 函数指针
static int (*funcSystemInit)(void) = NULL;
static float (*funcReadTemperature)(int) = NULL;
static int (*funcSetAlarmLimit)(int, float, float) = NULL;
static int (*funcTriggerDataAcquisition)(void) = NULL;

// 全局变量
static float g_temperatures[8] = {0};
static IOSCANPVT g_scan_pvt;
static int g_initialized = 0;

// 数据采集线程
static void *acquisitionThread(void *arg) {
    while (1) {
        // 触发数据采集
        if (funcTriggerDataAcquisition) {
            funcTriggerDataAcquisition();
        }

        // 读取所有温度
        for (int i = 0; i < 8; i++) {
            if (funcReadTemperature) {
                g_temperatures[i] = funcReadTemperature(i);
            }
        }

        // 触发I/O中断
        scanIoRequest(g_scan_pvt);

        usleep(1000000);  // 1秒
    }
    return NULL;
}

// IOC Shell命令：初始化
int drvTempMonitorInit(const char *lib_path) {
    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        printf("ERROR: Cannot load %s\n", lib_path);
        return -1;
    }

    funcSystemInit = dlsym(handle, "SystemInit");
    funcReadTemperature = dlsym(handle, "ReadTemperature");
    funcSetAlarmLimit = dlsym(handle, "SetAlarmLimit");
    funcTriggerDataAcquisition = dlsym(handle, "TriggerDataAcquisition");

    if (funcSystemInit) {
        funcSystemInit();
    }

    scanIoInit(&g_scan_pvt);

    pthread_t tid;
    pthread_create(&tid, NULL, acquisitionThread, NULL);

    g_initialized = 1;
    printf("Temperature Monitor Driver Initialized\n");
    return 0;
}

// 读取温度
float ReadTemperature(int channel) {
    if (channel < 0 || channel >= 8) {
        return 0.0f;
    }
    return g_temperatures[channel];
}

// 设置报警限值
int SetAlarmLimit(int channel, float high, float low) {
    if (funcSetAlarmLimit) {
        return funcSetAlarmLimit(channel, high, low);
    }
    return -1;
}

// 获取I/O中断扫描PVT
IOSCANPVT GetScanPvt(void) {
    return g_scan_pvt;
}

// IOC Shell注册
static const iocshArg initArg0 = {"lib_path", iocshArgString};
static const iocshArg *initArgs[] = {&initArg0};
static const iocshFuncDef initFuncDef = {"drvTempMonitorInit", 1, initArgs};

static void initCallFunc(const iocshArgBuf *args) {
    drvTempMonitorInit(args[0].sval);
}

static void drvTempMonitorRegister(void) {
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(drvTempMonitorRegister);
```

---

### Day 4 (Thursday) - 设备支持和数据库

**设备支持** (`devTempMonitor.c`) 和 **数据库** (.db文件) 实现...

[由于篇幅限制，省略详细代码，但包含完整结构]

---

### Day 5 (Friday) - 测试和文档

**测试**:
```bash
# 编译
make

# 运行
./bin/linux-x86_64/TempMonitor st.cmd

# 测试
caget TEMP:CH1:Value
camonitor TEMP:CH1:Value
```

**文档**:
- README.md
- 设计文档
- 用户手册
- 测试报告

---

## ✅ Week 8检查点（最终评估）

### 项目交付物
- [ ] 源代码（完整且有注释）
- [ ] Mock库实现
- [ ] README文档
- [ ] 测试用例
- [ ] 运行截图

### 能力评估
- [ ] 能独立设计IOC架构
- [ ] 能实现三层代码
- [ ] 能编写Mock库
- [ ] 能测试和调试
- [ ] 能编写技术文档

---

## 🎓 8周学习总结

### 你的成长

**Week 1-2**: 从不会到会用
**Week 3-4**: 从会用到理解
**Week 5-6**: 从理解到能改
**Week 7-8**: 从能改到能造

### 技能树

```
✅ EPICS基础知识
✅ 三层架构理解
✅ 驱动层开发
✅ 设备支持层开发
✅ 数据库设计
✅ Mock库开发
✅ 完整项目开发
✅ 测试和调试
✅ 文档编写
```

---

## 🚀 下一步

完成8周学习后，你可以：

1. **继续深入**:
   - Part 12: 进阶主题（性能优化、CA编程）
   - Part 13: 部署上线（交叉编译、硬件部署）

2. **实际应用**:
   - 为实验室开发IOC
   - 参与开源EPICS项目

3. **持续学习**:
   - 阅读EPICS官方文档
   - 参加EPICS社区活动

---

## 🎉 恭喜毕业！

你已经完成了从零基础到独立开发EPICS IOC的全部学习！

**你现在能够**:
- ✅ 独立设计和开发IOC
- ✅ 在PC上进行开发和测试
- ✅ 部署到真实硬件
- ✅ 解决实际工程问题

**继续保持学习和实践，成为EPICS专家！** 🏆

---

**Week 8加油！** 这是你证明自己的时刻！ 💪🚀
