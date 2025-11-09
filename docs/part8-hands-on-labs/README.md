# Part 8: Hands-on Labs

> **目标**: 通过15个递进式实验，从零到精通BPMIOC开发
> **难度**: ⭐~⭐⭐⭐⭐⭐ (递进式)
> **总时间**: 30-40小时
> **前置知识**: Part 1-6

## 📋 实验室概述

Part 8包含**15个hands-on实验**，分为三个难度级别：

- **基础实验 (Lab 1-5)**: 理解代码流程，学会基本调试
- **修改实验 (Lab 6-10)**: 修改代码，添加新功能
- **高级实验 (Lab 11-15)**: 从零实现复杂功能

每个实验都包括：
- ✅ 明确的学习目标
- ✅ 详细的步骤指导
- ✅ 完整的代码示例
- ✅ 常见错误和解决方案
- ✅ 扩展挑战

---

## 🎯 学习路径

```
基础实验（理解）
  Lab 1-5
    ↓
修改实验（修改）
  Lab 6-10
    ↓
高级实验（创造）
  Lab 11-15
```

---

## 📚 实验室列表

### 🟢 基础实验 (Lab 1-5)

**目标**: 理解BPMIOC的工作流程

| Lab | 名称 | 难度 | 时间 | 核心技能 | 状态 |
|-----|------|------|------|----------|------|
| [Lab 1](./labs-basic/lab01-trace-rf-amp.md) | RF信号追踪 | ⭐ | 2h | GDB调试、代码流程 | ✅ |
| [Lab 2](./labs-basic/lab02-modify-scan.md) | 修改扫描周期 | ⭐ | 1.5h | .db配置、扫描机制 | ✅ |
| [Lab 3](./labs-basic/lab03-add-debug.md) | 添加调试信息 | ⭐ | 1.5h | 日志输出、调试 | ✅ |
| [Lab 4](./labs-basic/lab04-understand-init.md) | 理解初始化 | ⭐⭐ | 2h | 启动流程、init_record | ✅ |
| [Lab 5](./labs-basic/lab05-waveform-plot.md) | 波形数据绘制 | ⭐⭐ | 2.5h | waveform Record、数据可视化 | ✅ |

---

### 🟡 修改实验 (Lab 6-10)

**目标**: 修改和扩展BPMIOC功能

| Lab | 名称 | 难度 | 时间 | 核心技能 | 状态 |
|-----|------|------|------|----------|------|
| [Lab 6](./labs-modification/lab06-add-new-pv.md) | 添加新PV | ⭐⭐ | 2h | .db编写、PV命名 | 📝 |
| [Lab 7](./labs-modification/lab07-modify-driver.md) | 修改驱动层函数 | ⭐⭐⭐ | 3h | 驱动层修改、数据处理 | 📝 |
| [Lab 8](./labs-modification/lab08-add-record-type.md) | 添加Record类型 | ⭐⭐⭐ | 3h | 设备支持、DSET | 📝 |
| [Lab 9](./labs-modification/lab09-calibration.md) | 实现校准功能 | ⭐⭐⭐ | 3h | calc Record、数据转换 | 📝 |
| [Lab 10](./labs-modification/lab10-alarm-config.md) | 添加报警配置 | ⭐⭐ | 2h | 报警字段、监控 | 📝 |

---

### 🔴 高级实验 (Lab 11-15)

**目标**: 从零实现复杂功能

| Lab | 名称 | 难度 | 时间 | 核心技能 | 状态 |
|-----|------|------|------|----------|------|
| [Lab 11](./labs-advanced/lab11-io-interrupt.md) | I/O中断扫描 | ⭐⭐⭐⭐ | 4h | scanIoRequest、中断机制 | 📝 |
| [Lab 12](./labs-advanced/lab12-custom-device.md) | 自定义设备支持 | ⭐⭐⭐⭐ | 4h | 完整DSET实现 | 📝 |
| [Lab 13](./labs-advanced/lab13-multithreading.md) | 多线程调试 | ⭐⭐⭐⭐ | 3h | pthread、线程同步 | 📝 |
| [Lab 14](./labs-advanced/lab14-performance.md) | 性能分析优化 | ⭐⭐⭐⭐⭐ | 4h | profiling、优化技巧 | 📝 |
| [Lab 15](./labs-advanced/lab15-final-project.md) | 综合项目 | ⭐⭐⭐⭐⭐ | 6h | 完整功能开发 | 📝 |

---

## 🔍 实验室涵盖的知识点

### 数据库层 (Part 6)
- Lab 2: 扫描机制配置
- Lab 6: 添加新PV和Record
- Lab 9: calc Record使用
- Lab 10: 报警配置

### 设备支持层 (Part 5)
- Lab 4: init_record流程
- Lab 8: 添加新Record类型支持
- Lab 11: I/O中断实现
- Lab 12: 自定义设备支持

### 驱动层 (Part 4)
- Lab 1: 驱动层数据流
- Lab 7: 修改驱动函数
- Lab 13: 多线程调试
- Lab 14: 性能优化

### 综合技能
- Lab 3: 调试技巧
- Lab 5: 数据可视化
- Lab 15: 完整项目开发

---

## 📖 如何使用实验室

### 推荐学习顺序

**Week 1-2** (基础实验):
```
Day 1: Lab 1 (RF信号追踪)
Day 2: Lab 2 (修改扫描周期)
Day 3: Lab 3 (添加调试)
Day 4-5: Lab 4 (理解初始化)
Day 6-7: Lab 5 (波形绘制)
```

**Week 3-4** (修改实验):
```
Day 1-2: Lab 6 (添加新PV)
Day 3-4: Lab 7 (修改驱动层)
Day 5-6: Lab 8 (添加Record类型)
Day 7-8: Lab 9 (校准功能)
Day 9: Lab 10 (报警配置)
```

**Week 5-6** (高级实验):
```
Day 1-2: Lab 11 (I/O中断)
Day 3-4: Lab 12 (自定义设备支持)
Day 5: Lab 13 (多线程调试)
Day 6-7: Lab 14 (性能优化)
Day 8-10: Lab 15 (综合项目)
```

---

### 实验准备

**环境要求**:
- 已完成Part 1（编译成功）
- 熟悉基本的Linux命令
- 安装GDB调试器
- 安装Python（用于数据可视化）

**推荐工具**:
```bash
# GDB增强
sudo apt-get install gdb-multiarch

# Python绘图
pip3 install matplotlib numpy

# EPICS客户端工具
# caget, caput, camonitor已随EPICS安装
```

---

### 实验报告模板

每个实验后，建议写简短的实验报告：

```markdown
# Lab X实验报告

## 实验目标
- ...

## 完成情况
- [x] 步骤1
- [x] 步骤2
- [ ] 步骤3

## 遇到的问题
1. 问题描述：...
   解决方案：...

## 学到的知识
- ...

## 扩展实验
- [ ] 挑战1
- [ ] 挑战2

## 代码修改总结
- 文件：driverWrapper.c:123
  修改：添加了...
```

---

## 🎯 学习目标

完成所有15个实验后，你将能够：

✅ **理解能力**:
- 深入理解BPMIOC三层架构
- 掌握数据从硬件到PV的完整流程
- 理解EPICS的扫描和初始化机制

✅ **修改能力**:
- 能够添加新的PV和Record
- 能够修改驱动层、设备支持层代码
- 能够配置报警、校准等功能

✅ **创造能力**:
- 能够实现I/O中断扫描
- 能够从零编写设备支持层
- 能够独立开发新功能

✅ **调试能力**:
- 熟练使用GDB调试
- 能够分析性能问题
- 能够处理多线程问题

---

## 📝 实验记录

使用此表格追踪你的学习进度：

| Lab | 完成日期 | 用时 | 难度评价 | 备注 |
|-----|----------|------|----------|------|
| Lab 1 | YYYY-MM-DD | 2h | ⭐ | ... |
| Lab 2 |  |  |  |  |
| ... |  |  |  |  |

---

## ⚠️ 注意事项

1. **循序渐进**: 不要跳过基础实验直接做高级实验
2. **动手实践**: 必须亲自写代码，不要只看不做
3. **记录笔记**: 记录遇到的问题和解决方法
4. **提问讨论**: 遇到问题及时提问，不要死磕
5. **代码备份**: 每次修改前备份代码（使用git）

---

## 🔗 相关文档

- **Part 1**: 快速开始 - 环境搭建
- **Part 2**: EPICS基础 - 基础概念
- **Part 3**: 架构详解 - 整体架构
- **Part 4**: 驱动层详解 - 驱动层实现
- **Part 5**: 设备支持层 - 设备支持实现
- **Part 6**: 数据库层 - .db配置和st.cmd
- **Part 10**: 调试与测试 - 调试技巧大全
- **Part 11**: 学习计划 - 8周学习路线

---

## 🎉 完成奖励

完成全部15个实验后：
- ✅ 你已掌握EPICS IOC开发的核心技能
- ✅ 可以独立开发类似的IOC项目
- ✅ 可以阅读和修改其他EPICS项目
- ✅ 为真实项目开发打下坚实基础

**恭喜你！继续学习Part 9完整教程，或直接开始Week 7-8的独立项目！** 🚀

---

**准备好了吗？从[Lab 1](./labs-basic/lab01-trace-rf-amp.md)开始你的EPICS IOC开发之旅吧！** 💪
