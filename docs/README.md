# BPMIOC 项目文档

> **版本**: 2.0
> **更新日期**: 2025-11-07
> **适用人群**: 博士研究生、EPICS初学者、加速器控制系统开发者

## 📖 欢迎

欢迎来到 BPMIOC 项目文档！本文档旨在帮助你：

- ✅ **快速复现**: 30分钟内运行起来BPMIOC
- ✅ **深入理解**: 掌握EPICS IOC的三层架构
- ✅ **动手实践**: 通过15个实验学会开发
- ✅ **独立开发**: 能够从零搭建类似项目

## 🚀 快速开始

**第一次使用？** 请按照以下顺序：

1. 📖 阅读 [QUICK_START.md](./QUICK_START.md) - 30分钟快速上手
2. 🗺️ 查看 [ROADMAP.md](./ROADMAP.md) - 了解完整学习路线
3. 📚 进入 [Part 1: 快速复现](./part1-quick-reproduction/) - 详细安装步骤
4. 📅 参考 [Part 11: 8周学习计划](./part11-weekly-plan/) - 系统学习

## 📚 文档结构

本文档分为 **18个部分**，按学习顺序组织：

### 🔴 入门必读（第1-2周）

| 部分 | 名称 | 文档数 | 重要性 | 说明 |
|------|------|--------|--------|------|
| **[Part 1](./part1-quick-reproduction/)** | 快速复现 | 9 | ⭐⭐⭐⭐⭐ | 30分钟运行起来 |
| **[Part 2](./part2-understanding-basics/)** | 理解基础 | 10 | ⭐⭐⭐⭐⭐ | EPICS核心概念 |
| **[Part 3](./part3-bpmioc-architecture/)** | BPMIOC架构 | 9 | ⭐⭐⭐⭐⭐ | 三层架构详解 |

### 🟠 核心学习（第3-6周）

| 部分 | 名称 | 文档数 | 重要性 | 说明 |
|------|------|--------|--------|------|
| **[Part 4](./part4-driver-layer/)** | 驱动层详解 | 15 | ⭐⭐⭐⭐⭐ | driverWrapper.c完全解析 |
| **[Part 5](./part5-device-support-layer/)** | 设备支持层 | 12 | ⭐⭐⭐⭐⭐ | devBPMMonitor.c详解 |
| **[Part 6](./part6-database-layer/)** | 数据库层 | 11 | ⭐⭐⭐⭐⭐ | 数据库设计与PV配置 |
| **[Part 7](./part7-build-system/)** | 构建系统 | 9 | ⭐⭐⭐ | Makefile和编译 |

### 🟡 实践提升（贯穿全程）

| 部分 | 名称 | 文档数 | 重要性 | 说明 |
|------|------|--------|--------|------|
| **[Part 8](./part8-hands-on-labs/)** | 实验室 | 15 | ⭐⭐⭐⭐⭐ | 15个hands-on实验 |
| **[Part 9](./part9-tutorials/)** | 完整教程 | 5 | ⭐⭐⭐⭐ | 5个完整项目教程 |
| **[Part 10](./part10-debugging-testing/)** | 调试与测试 | 12 | ⭐⭐⭐⭐ | 调试技巧大全 |

### 🟢 系统学习（推荐）

| 部分 | 名称 | 文档数 | 重要性 | 说明 |
|------|------|--------|--------|------|
| **[Part 11](./part11-weekly-plan/)** | 8周学习计划 | 11 | ⭐⭐⭐⭐⭐ | 详细的周计划 |
| **[Part 13](./part13-deployment/)** | 部署上线 | 9 | ⭐⭐⭐⭐ | 从模拟到真实硬件 |

### 🔵 进阶拓展（可选）

| 部分 | 名称 | 文档数 | 重要性 | 说明 |
|------|------|--------|--------|------|
| **[Part 12](./part12-advanced-topics/)** | 进阶主题 | 10 | ⭐⭐⭐ | 性能优化、CA编程等 |
| **[Part 14](./part14-case-studies/)** | 案例研究 | 6 | ⭐⭐⭐ | 实际项目案例 |
| **[Part 16](./part16-best-practices/)** | 最佳实践 | 8 | ⭐⭐⭐ | 代码规范和项目管理 |
| **[Part 17](./part17-physics-background/)** | 物理背景 | 5 | ⭐⭐ | 加速器物理基础 |

### ⚪ 参考资料（按需查阅）

| 部分 | 名称 | 说明 |
|------|------|------|
| **[Part 15](./part15-reference/)** | 参考手册 | API、数据结构、PV列表、命令参考 |
| **[Part 18](./part18-appendix/)** | 附录 | 术语表、FAQ、资源链接 |

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
Part 9 → Part 8 (高级实验) → Part 13
```

**完成标准**:
- ✅ 能独立设计IOC架构
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

- **编译问题** → Part 1, Part 7, Part 18 (FAQ)
- **数据流分析** → Part 3 (04-data-flow.md), Part 8 (lab01)
- **添加新PV** → Part 9 (tutorial01)
- **调试技巧** → Part 10
- **性能优化** → Part 12 (01-performance-optimization.md)
- **PV列表** → Part 15 (tables/pv-table.md)
- **Offset列表** → Part 15 (tables/offset-table.md)

### 按文件类型查找

- **`.c` 文件详解** → Part 4, Part 5
- **`.db` 文件详解** → Part 6
- **Makefile详解** → Part 7
- **st.cmd详解** → Part 6 (10-startup-script.md)

### 按问题类型查找

- **报错信息** → Part 18 (troubleshooting-guide.md)
- **概念不理解** → Part 2, Part 18 (glossary.md)
- **不知道怎么做** → Part 8, Part 9

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
- **解决编译问题** → [Part 1: 08-troubleshooting](./part1-quick-reproduction/08-troubleshooting.md)
- **理解数据流** → [Part 3: 04-data-flow](./part3-bpmioc-architecture/04-data-flow.md)
- **添加新功能** → [Part 9: tutorial01](./part9-tutorials/tutorial01-add-new-pv/)
- **查看PV列表** → [Part 15: PV参考表](./part15-reference/tables/pv-table.md)
- **查询API** → [Part 15: API参考](./part15-reference/api/)
- **看案例** → [Part 14: 案例研究](./part14-case-studies/)

---

**开始你的EPICS之旅吧！** 🚀

建议从 [QUICK_START.md](./QUICK_START.md) 开始，30分钟后你就能看到BPMIOC运行起来！
