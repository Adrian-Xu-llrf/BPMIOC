# BPMIOC 项目文档

> **版本**: 2.0
> **更新日期**: 2025-11-09
> **适用人群**: 博士研究生、EPICS初学者、加速器控制系统开发者
> **文档状态**: 📝 持续更新中（已完成核心部分）

## 📖 欢迎

欢迎来到 BPMIOC 项目文档！本文档旨在帮助你：

- ✅ **快速复现**: 30分钟内运行起来BPMIOC（已完成）
- ✅ **深入理解**: 掌握EPICS IOC的三层架构（已完成）
- ✅ **PC模拟开发**: 无需硬件即可开发调试（已完成）
- ✅ **动手实践**: 通过15个实验学会开发（全部完成）
- 🚧 **独立开发**: 能够从零搭建类似项目（进行中）

## ✨ 当前已完成的核心内容

**可以直接使用的部分**：

1. ✅ **[Part 1](./part1-quick-reproduction/)** - 快速复现指南（4个文档）
   - 介绍、EPICS安装、模拟模式启用、README

2. ✅ **[Part 2](./part2-understanding-basics/)** - EPICS基础知识（11个文档）
   - EPICS概念、Record类型、扫描机制、CA工具、C语言要点等

3. ✅ **[Part 3](./part3-bpmioc-architecture/)** - BPMIOC架构（9个文档完整）
   - 架构概览、数据流、初始化、内存模型、Offset系统、线程模型、错误处理、性能分析、设计模式

4. ✅ **[Part 8](./part8-hands-on-labs/)** - 动手实验（15个全部完成）
   - Lab 1-5: RF信号追踪、扫描修改、调试添加、初始化理解、波形绘制
   - Lab 6-10: 添加PV、修改驱动层、Record类型、校准功能、报警配置
   - Lab 11-15: I/O中断、自定义设备、多线程调试、性能优化、综合项目

4. ✅ **[Part 4](./part4-driver-layer/)** - 驱动层详解（15个文档完整）
   - 概览、文件结构、全局缓冲区、InitDevice、dlopen、pthread、ReadData、SetReg、ReadWaveform、硬件函数、辅助函数、调试、故障排查、修改指南、最佳实践

5. ✅ **[Part 5](./part5-device-support-layer/)** - 设备支持层详解（5个文档完整）
   - 概览、DSET结构、init_record、read/write函数、DevPvt私有数据详解

6. ✅ **[Part 6](./part6-database-layer/)** - 数据库层详解（5个文档完整）
   - .db文件结构、Record配置、PV命名规范、扫描机制、st.cmd启动脚本

7. 🚧 **[Part 7, 9-10](.)** - 其他核心学习部分（框架完成）
   - 每个Part都有完整的README学习指南
   - 清晰的学习路径和目标

5. ✅ **[Part 11](./part11-weekly-plan/)** - 8周学习计划（12个文档完整）
   - 总览、学习路径、Week 1-8详细计划、进度追踪、每日模板

8. ✅ **[Part 15](./part15-reference/)** - 参考资料（2个表格）
   - PV完整列表、Offset完整列表

9. ✅ **[Part 19](./part19-simulator-tutorial/)** - 模拟器编写教程（完整）
   - 从零开始学习如何编写硬件模拟器（~1000行详细教程）

10. ✅ **[simulator/](../simulator/)** - 硬件模拟库（完整实现）
   - Mock库、数据生成器、文件回放、故障注入、测试用例
   - 🎯 **核心亮点**：支持在PC上开发，无需FPGA硬件！

## 🚀 快速开始

**第一次使用？** 请按照以下顺序：

1. 📖 阅读 [QUICK_START.md](./QUICK_START.md) - 30分钟快速上手
2. 🗺️ 查看 [ROADMAP.md](./ROADMAP.md) - 了解完整学习路线
3. 📚 进入 [Part 1: 快速复现](./part1-quick-reproduction/) - 详细安装步骤
4. 💡 学习 [Part 19: 模拟器教程](./part19-simulator-tutorial/how-to-write-simulator.md) - 掌握PC开发
5. 📅 参考 [Part 11: 8周学习计划](./part11-weekly-plan/) - 系统学习

## 📚 文档结构

本文档计划分为 **18个部分**，按学习顺序组织。**✅=已完成 🚧=进行中 📋=计划中**

### 🔴 入门必读（第1-2周）

| 部分 | 名称 | 状态 | 文档数 | 重要性 | 说明 |
|------|------|------|--------|--------|------|
| **[Part 1](./part1-quick-reproduction/)** | 快速复现 | ✅ | 4 | ⭐⭐⭐⭐⭐ | 30分钟运行起来 |
| **[Part 2](./part2-understanding-basics/)** | 理解基础 | ✅ | 11 | ⭐⭐⭐⭐⭐ | EPICS核心概念 |
| **[Part 3](./part3-bpmioc-architecture/)** | BPMIOC架构 | ✅ | 9/9 | ⭐⭐⭐⭐⭐ | 三层架构详解 ✅完成 |

### 🟠 核心学习（第3-6周）

| 部分 | 名称 | 状态 | 文档数 | 重要性 | 说明 |
|------|------|------|--------|--------|------|
| **[Part 4](./part4-driver-layer/)** | 驱动层详解 | ✅ | 15/15 | ⭐⭐⭐⭐⭐ | driverWrapper.c完全解析 ✅完成 |
| **[Part 5](./part5-device-support-layer/)** | 设备支持层 | ✅ | 5/5 | ⭐⭐⭐⭐⭐ | devBPMMonitor.c详解 ✅完成 |
| **[Part 6](./part6-database-layer/)** | 数据库层 | ✅ | 5/5 | ⭐⭐⭐⭐⭐ | 数据库设计与PV配置 ✅完成 |
| **[Part 7](./part7-build-system/)** | 构建系统 | 🚧 | 1/9 | ⭐⭐⭐ | Makefile和编译 ✅框架完成 |

### 🟡 实践提升（贯穿全程）

| 部分 | 名称 | 状态 | 文档数 | 重要性 | 说明 |
|------|------|------|--------|--------|------|
| **[Part 8](./part8-hands-on-labs/)** | 实验室 | ✅ | 15/15 | ⭐⭐⭐⭐⭐ | 递进式实验（基础5+修改5+高级5） ✅完成 |
| **[Part 9](./part9-tutorials/)** | 完整教程 | 🚧 | 1/5 | ⭐⭐⭐⭐ | 5个完整项目教程 ✅框架完成 |
| **[Part 10](./part10-debugging-testing/)** | 调试与测试 | 🚧 | 1/12 | ⭐⭐⭐⭐ | 调试技巧大全 ✅框架完成 |

### 🟢 系统学习（推荐）

| 部分 | 名称 | 状态 | 文档数 | 重要性 | 说明 |
|------|------|------|--------|--------|------|
| **[Part 11](./part11-weekly-plan/)** | 8周学习计划 | ✅ | 12/12 | ⭐⭐⭐⭐⭐ | 详细的周计划 ✅完成 |
| **[Part 13](./part13-deployment/)** | 部署上线 | 📋 | 0/9 | ⭐⭐⭐⭐ | 从模拟到真实硬件 |

### 🔵 进阶拓展（可选）

| 部分 | 名称 | 状态 | 文档数 | 重要性 | 说明 |
|------|------|------|--------|--------|------|
| **[Part 12](./part12-advanced-topics/)** | 进阶主题 | 📋 | 0/10 | ⭐⭐⭐ | 性能优化、CA编程等 |
| **[Part 14](./part14-case-studies/)** | 案例研究 | 📋 | 0/6 | ⭐⭐⭐ | 实际项目案例 |
| **[Part 16](./part16-best-practices/)** | 最佳实践 | 📋 | 0/8 | ⭐⭐⭐ | 代码规范和项目管理 |
| **[Part 17](./part17-physics-background/)** | 物理背景 | 📋 | 0/5 | ⭐⭐ | 加速器物理基础 |

### ⚪ 参考资料（按需查阅）

| 部分 | 名称 | 状态 | 说明 |
|------|------|------|------|
| **[Part 15](./part15-reference/)** | 参考手册 | 🚧 | API、数据结构、PV列表（已有PV和Offset表） |
| **[Part 18](./part18-appendix/)** | 附录 | 📋 | 术语表、FAQ、资源链接 |

### 🎯 扩展学习

| 部分 | 名称 | 状态 | 文档数 | 重要性 | 说明 |
|------|------|------|--------|--------|------|
| **[Part 19](./part19-simulator-tutorial/)** | 模拟器编写教程 | ✅ | 1 | ⭐⭐⭐⭐⭐ | 从零开始学习写模拟器 |

### 🎯 Simulator（独立项目）

| 部分 | 名称 | 状态 | 说明 |
|------|------|------|------|
| **[simulator/](../simulator/)** | 硬件模拟库 | ✅ | 完整的Mock库实现，支持PC开发 |

## 🎯 三个学习层次

根据你的目标，选择不同的学习路径：

### 层次1: 基础复现（1-2周）
**目标**: 能运行IOC，访问PV，理解基本概念

```
Part 1 → Part 2 → Part 8 (基础实验)
```

**完成标准**:
- ✅ BPMIOC在模拟模式下运行
- ✅ 能用caget/caput访问PV
- ✅ 理解PV、Record、IOC、CA等概念
- ✅ 能用Python读写PV

### 层次2: 功能复现（3-6周）
**目标**: 理解三层架构，能修改代码，添加简单功能

```
Part 3 → Part 4 → Part 5 → Part 6 → Part 8 (修改实验)
```

**完成标准**:
- ✅ 理解驱动层、设备支持层、数据库层
- ✅ 能追踪数据流（从硬件到PV）
- ✅ 能添加新的PV和功能
- ✅ 能调试和解决问题

### 层次3: 完整复现（7-8周）
**目标**: 从零搭建类似项目，部署到真实硬件

```
Part 19 (模拟器教程) → simulator/ → Part 8 (高级实验) → Part 13
```

**完成标准**:
- ✅ 能独立设计IOC架构
- ✅ 能编写硬件模拟器进行PC开发
- ✅ 能实现驱动层和设备支持层
- ✅ 能设计数据库和PV结构
- ✅ 能交叉编译并部署到目标板

## 📅 推荐学习路径

### 路径A: 快速入门（适合时间紧张）
**总时长**: 1周

```
Day 1-2: Part 1 (快速复现)
Day 3-4: Part 2 (EPICS基础)
Day 5-7: Part 8 (基础实验 1-5)
```

### 路径B: 系统学习（推荐）
**总时长**: 8周

遵循 **[Part 11: 8周学习计划](./part11-weekly-plan/)**

### 路径C: 目标导向（适合有经验者）
**总时长**: 灵活

1. 快速浏览 Part 1-3
2. 深入你感兴趣的部分（Part 4/5/6）
3. 完成相关实验（Part 8）
4. 参考文档（Part 15）按需查阅

## 🔍 如何查找内容

### 按主题查找

- **数据流分析** → Part 3 ([02-data-flow.md](./part3-bpmioc-architecture/02-data-flow.md)), Part 8 ([lab01](./part8-hands-on-labs/labs-basic/lab01-trace-rf-amp.md))
- **初始化过程** → Part 3 ([03-initialization-sequence.md](./part3-bpmioc-architecture/03-initialization-sequence.md)), Part 8 ([lab04](./part8-hands-on-labs/labs-basic/lab04-understand-init.md))
- **PC模拟开发** → Part 19 ([模拟器教程](./part19-simulator-tutorial/how-to-write-simulator.md)), [simulator/](../simulator/)
- **PV列表** → Part 15 ([pv-table.md](./part15-reference/tables/pv-table.md))
- **Offset列表** → Part 15 ([offset-table.md](./part15-reference/tables/offset-table.md))
- **编译问题** → Part 1 ([05-enable-simulation.md](./part1-quick-reproduction/05-enable-simulation.md))
- **调试技巧** → Part 8 ([lab03](./part8-hands-on-labs/labs-basic/lab03-add-debug.md))
- **波形绘制** → Part 8 ([lab05](./part8-hands-on-labs/labs-basic/lab05-waveform-plot.md))

### 按文件类型查找

- **`.c` 文件详解** → Part 4 (计划中), Part 5 (计划中)
- **`.db` 文件详解** → Part 6 (计划中), Part 2 ([09-database-files.md](./part2-understanding-basics/09-database-files.md))
- **Makefile详解** → Part 7 (计划中)
- **st.cmd详解** → Part 6 (计划中)

### 按问题类型查找

- **概念不理解** → Part 2 ([基础概念](./part2-understanding-basics/))
- **不知道怎么做** → Part 8 ([动手实验](./part8-hands-on-labs/)), Part 19 ([模拟器教程](./part19-simulator-tutorial/))
- **想在PC上开发** → [simulator/](../simulator/), Part 19 ([模拟器教程](./part19-simulator-tutorial/how-to-write-simulator.md))

## 💡 使用建议

### 第一次阅读
1. 不要试图一次看完所有文档
2. 先完成 Part 1，确保能运行
3. 边看文档边动手实践
4. 遇到不懂的概念先记录，继续往下看

### 深入学习
1. 每个部分都有 README，先看 README
2. 按照推荐顺序阅读
3. 做实验前先看相关理论
4. 每完成一个实验就更新进度追踪表

### 遇到问题
1. 先查 Part 18 (FAQ 和 troubleshooting-guide)
2. 使用文档内的搜索功能（Ctrl+F）
3. 查看相关实验的常见问题部分
4. 在代码中搜索关键词

## 📊 进度追踪

使用 [Part 11 进度追踪表](./part11-weekly-plan/10-progress-tracker.md) 记录学习进度：

```markdown
- [x] Part 1: 快速复现
- [x] Part 2: 理解基础
- [ ] Part 3: BPMIOC架构
- [ ] Part 4: 驱动层详解
...
```

## 🤝 贡献指南

欢迎改进文档！请查看 [CONTRIBUTING.md](../CONTRIBUTING.md)

## 📜 许可证

本文档遵循 [MIT License](../LICENSE)

## 📮 反馈

- **问题报告**: 在GitHub提Issue
- **改进建议**: 提交Pull Request
- **学习交流**: [讨论区](../../discussions)

## 🔗 相关资源

- **EPICS官网**: https://epics-controls.org
- **EPICS文档**: https://epics.anl.gov/base/R3-15/6-docs/
- **本项目仓库**: https://github.com/your-org/BPMIOC

---

## 📖 快速导航

### 我想...

- **马上运行BPMIOC** → [QUICK_START.md](./QUICK_START.md)
- **了解学习路线** → [ROADMAP.md](./ROADMAP.md)
- **跟着计划学习** → [Part 11](./part11-weekly-plan/)
- **理解数据流** → [Part 3: 02-data-flow](./part3-bpmioc-architecture/02-data-flow.md)
- **学习写模拟器** → [Part 19: 模拟器教程](./part19-simulator-tutorial/how-to-write-simulator.md)
- **PC开发无需硬件** → [simulator/](../simulator/)
- **查看PV列表** → [Part 15: PV参考表](./part15-reference/tables/pv-table.md)
- **查看Offset列表** → [Part 15: Offset参考表](./part15-reference/tables/offset-table.md)
- **动手实验** → [Part 8: 实验室](./part8-hands-on-labs/)

---

**开始你的EPICS之旅吧！** 🚀

建议从 [QUICK_START.md](./QUICK_START.md) 开始，30分钟后你就能看到BPMIOC运行起来！
