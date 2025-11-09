# Part 9: 完整项目教程

> **目标**: 通过5个完整项目学会EPICS IOC开发
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 4-5周
> **前置知识**: Part 1-8

## 📋 本部分概述

Part 9提供5个完整的项目教程，从零开始构建功能完整的IOC应用。

每个教程都是一个独立的项目，包含：
- 需求分析
- 架构设计
- 代码实现
- 测试验证
- 部署上线

完成本部分后，你将能够：
- ✅ 独立设计和实现IOC应用
- ✅ 从需求到部署全流程掌握
- ✅ 解决实际项目问题
- ✅ 积累项目经验

## 📚 教程列表

| 教程 | 难度 | 时间 | 状态 | 描述 |
|------|------|------|------|------|
| **[Tutorial 1](./tutorial1-add-new-pv.md)** | ⭐⭐⭐ | 1周 | ✅ | 添加新PV - 信噪比监控 |
| **[Tutorial 2](./tutorial2-new-offset-type.md)** | ⭐⭐⭐⭐ | 1周 | ✅ | 实现新的Offset类型 - 波形数据采集 |
| **[Tutorial 3](./tutorial3-add-hardware-channel.md)** | ⭐⭐⭐⭐ | 1周 | ✅ | 添加新硬件通道 - 扩展到10个RF输入 |
| **[Tutorial 4](./tutorial4-temperature-ioc.md)** | ⭐⭐⭐⭐⭐ | 1-2周 | ✅ | 从零构建温度监控IOC |
| **[Tutorial 5](./tutorial5-power-supply-ioc.md)** | ⭐⭐⭐⭐⭐ | 1-2周 | ✅ | 构建电源控制IOC |

## 🎯 Tutorial 1: 添加新PV

### 目标
为BPMIOC添加一个新的PV: `LLRF:BPM:RF3SNR`（信噪比）

### 学习要点
- 修改.db文件
- 修改设备支持层
- 修改驱动层
- 测试验证

### 实现步骤
1. 在数据库层添加Record
2. 在驱动层添加offset case
3. 实现SNR计算逻辑
4. 编译测试

## 🎯 Tutorial 2: 实现新的Offset类型

### 目标
实现一个新的数据采集模式

### 学习要点
- Offset系统扩展
- 数据采集逻辑
- 算法实现

## 🎯 Tutorial 3: 添加新硬件通道

### 目标
为BPMIOC添加第9和第10个RF通道

### 学习要点
- 全链路修改
- 数据缓冲区扩展
- 配置管理

## 🎯 Tutorial 4: 温度监控IOC

### 目标
从零构建一个简单的温度监控IOC

### 内容
- 设计架构
- 实现驱动层
- 实现设备支持层
- 配置数据库
- 测试部署

### 目录结构
```
tempMonitorApp/
├── src/
│   ├── driverTemp.c
│   ├── devTemp.c
│   └── Makefile
├── Db/
│   └── temp.db
└── iocBoot/
    └── iocTemp/
        └── st.cmd
```

## 🎯 Tutorial 5: 电源控制IOC

### 目标
构建一个完整的电源控制IOC，包含读写功能

### 功能
- 电压/电流监控
- 开关控制
- 限流保护
- 报警机制

## 🔗 相关文档

- **Part 1-8**: 基础知识
- **Part 19**: 模拟器教程
- **simulator/**: 模拟器实现

## 📚 参考资源

- [EPICS Getting Started](https://epics-controls.org/resources-and-support/documents/getting-started/)
- [IOC Application Examples](https://github.com/epics-base/exampleCPP)

---

**开始学习**: 从Tutorial 1开始，循序渐进
