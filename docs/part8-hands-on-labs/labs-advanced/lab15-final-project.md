# Lab 15: 综合项目 - 振动监控IOC

> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 6小时
> **前置**: Lab 1-14

## 🎯 项目目标

从零开发一个完整的振动监控IOC，综合应用所有学到的技能。

---

## 📋 项目需求

### 功能需求

1. **数据采集**:
   - 3轴加速度数据（X/Y/Z）
   - 温度数据
   - 采样率可配置

2. **数据处理**:
   - 计算振动幅度（sqrt(X²+Y²+Z²)）
   - 移动平均滤波
   - FFT频谱分析（可选）

3. **监控告警**:
   - 振动幅度超限报警
   - 温度超限报警
   - 报警日志记录

4. **控制功能**:
   - 启动/停止采集
   - 采样率设置
   - 数据清零

---

## 🔧 实现步骤

### 步骤1: 设计数据结构

```c
// 驱动层
typedef struct {
    float acc_x;
    float acc_y;
    float acc_z;
    float temperature;
} VibrData;
```

### 步骤2: 实现驱动层

```c
// drvVibration.c
float ReadAccel(int channel, int axis);
float ReadTemp(void);
void SetSampleRate(int rate);
```

### 步骤3: 实现设备支持层

```c
// devVibration.c
static long init_record_ai(aiRecord *prec);
static long read_ai(aiRecord *prec);
```

### 步骤4: 创建数据库

```
# 加速度
record(ai, "$(P):Accel_X") {...}
record(ai, "$(P):Accel_Y") {...}
record(ai, "$(P):Accel_Z") {...}

# 计算振动幅度
record(calc, "$(P):Vibr_Amp") {
    field(INPA, "$(P):Accel_X CP")
    field(INPB, "$(P):Accel_Y CP")
    field(INPC, "$(P):Accel_Z CP")
    field(CALC, "SQRT(A*A+B*B+C*C)")
}

# 报警
record(ai, "$(P):Vibr_Amp") {
    field(HIHI, "10.0")
    field(HIGH, "8.0")
    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
}
```

### 步骤5: 测试和调试

- 使用Mock库模拟数据
- GDB调试问题
- 性能优化

---

## ✅ 项目检查清单

- [ ] 驱动层实现完整
- [ ] 设备支持层支持ai/ao
- [ ] .db文件配置正确
- [ ] 报警功能正常
- [ ] calc计算正确
- [ ] 性能满足要求（<10ms处理时间）
- [ ] 添加了调试信息
- [ ] 编写了测试脚本

---

## 🎉 完成奖励

**恭喜完成全部15个实验！** 

你已经掌握了：
- ✅ EPICS IOC开发全流程
- ✅ 三层架构深入理解
- ✅ 调试和优化技能
- ✅ 独立开发能力

**你现在可以**:
- 开发自己的EPICS IOC项目
- 阅读和修改其他EPICS项目
- 为真实硬件编写设备支持

**继续学习**: Part 9完整教程，或开始你的实际项目！🚀

---

**祝贺你成为EPICS IOC开发者！** 💪🎉
