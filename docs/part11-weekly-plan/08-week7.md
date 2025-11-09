# Week 7: 综合开发（Integrated Development）

> **学习目标**: Mock库开发，完整功能实现
> **关键词**: Mock库、PC开发、完整流程
> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 15-20小时

## 📅 本周概览

```
周一-周二: Mock库开发
  ├─ Part 19: 模拟器教程
  ├─ 开发libbpm_mock.so
  └─ 与IOC集成

周三-周五: 完整功能开发
  ├─ 需求分析
  ├─ 架构设计
  ├─ 三层实现
  └─ 测试验证

周末: 复习总结
  └─ 准备Week 8独立项目
```

---

## 🎯 Week 7学习目标

### 知识目标
- [ ] 掌握Mock库开发方法
- [ ] 理解硬件抽象层设计
- [ ] 掌握完整开发流程
- [ ] 理解PC开发和硬件部署的区别

### 技能目标
- [ ] 能开发Mock库模拟硬件
- [ ] 能在PC上完成完整功能开发
- [ ] 能进行三层协同开发
- [ ] 能编写测试用例

---

## 📚 Day-by-Day Schedule

### Day 1-2 - Mock库开发

**学习内容**:
- ✅ Part 19: 01-how-to-write-simulator.md
- ✅ Part 19: 05-complete-mock-implementation.md
- ✅ Part 19: 09-integration-with-ioc.md

**实践**: 开发简单Mock库

```c
// simple_mock.c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static double g_time = 0.0;

int SystemInit(void) {
    srand(time(NULL));
    g_time = 0.0;
    return 0;
}

int TriggerAllDataReached(void) {
    g_time += 0.1;
    return 0;
}

float GetRFInfo(int channel, int type) {
    // 模拟RF数据
    float base = 1.0;
    float variation = 0.01 * sin(2.0 * M_PI * 0.1 * g_time);
    float noise = 0.001 * ((float)rand() / RAND_MAX - 0.5);
    return base + variation + noise;
}

// ... 实现其他函数
```

**编译和测试**:
```bash
gcc -shared -fPIC simple_mock.c -o libsimple_mock.so -lm
# 测试
export BPM_LIB_PATH=./libsimple_mock.so
./BPMMonitor st.cmd
```

---

### Day 3-4 - 完整功能开发

**项目**: 添加温度监控功能

**需求**:
- 监控4个温度传感器
- 温度范围：0-100°C
- 超过80°C报警
- 支持PC模拟

**实现步骤**:

**1. 驱动层 (driverWrapper.c)**:
```c
// 添加新offset
#define OFFSET_TEMPERATURE 10

// 添加温度读取函数
float ReadTemperature(int sensor_id) {
    if (funcGetTemperature != NULL) {
        return funcGetTemperature(sensor_id);
    }
    return 0.0f;
}
```

**2. 设备支持层 (devBPMMonitor.c)**:
```c
static long read_ai_temp(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    float temp = ReadData(OFFSET_TEMPERATURE,
                         pPvt->channel, 0);
    prec->val = temp;
    return 0;
}
```

**3. 数据库层 (.db)**:
```
record(ai, "BPM:01:Temp1") {
    field(DTYP, "BPM Monitor")
    field(INP,  "@10 0 0")
    field(SCAN, "1 second")
    field(EGU,  "C")
    field(PREC, "1")
    field(HIHI, "80.0")
    field(HHSV, "MAJOR")
}

# Temp2, Temp3, Temp4 类似
```

**4. Mock库**:
```c
float GetTemperature(int sensor_id) {
    // 模拟温度：基准值 + 随机波动
    float base_temp = 25.0 + sensor_id * 5.0;
    float fluctuation = 2.0 * ((float)rand() / RAND_MAX - 0.5);
    return base_temp + fluctuation;
}
```

---

### Day 5 - 测试和调试

**测试用例**:
```python
# test_temperature.py
from epics import caget, caput, camonitor
import time

def test_temperature_reading():
    """测试温度读取"""
    for i in range(1, 5):
        pv = f"BPM:01:Temp{i}"
        val = caget(pv)
        assert val is not None
        assert 0 <= val <= 100
        print(f"{pv} = {val:.1f}°C")

def test_temperature_alarm():
    """测试温度报警"""
    # 监控Temp1的报警状态
    status = caget("BPM:01:Temp1.SEVR")
    print(f"Alarm status: {status}")

if __name__ == "__main__":
    test_temperature_reading()
    test_temperature_alarm()
```

---

### Weekend - 准备独立项目

**Saturday**:
1. 复习Week 1-7所有内容
2. 整理开发流程笔记
3. 设计Week 8的独立项目

**Sunday**:
1. 准备开发环境
2. 阅读Part 9教程
3. 规划项目时间表

---

## ✅ Week 7检查点

**完成标准**:
- [ ] 成功开发了Mock库
- [ ] 完成了温度监控功能
- [ ] 三层协同工作正常
- [ ] 测试用例全部通过
- [ ] 在PC上完成了90%开发

**能力测试**:
```c
// 能独立完成以下任务吗？
Q1: 为新硬件设计Mock库
Q2: 在三层中添加新功能
Q3: 编写测试用例验证
Q4: 调试并解决问题
Q5: 编写技术文档
```

---

## 🎯 下一步

恭喜完成Week 7！现在你已经具备独立开发的能力了！

👉 [09-week8.md](./09-week8.md) - Week 8: 独立项目

---

**Week 7加油！** 这是质的飞跃 - 从理解到创造！ 🚀
