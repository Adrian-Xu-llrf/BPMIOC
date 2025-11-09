# Part 2: 理解基础

> **目标**: 掌握EPICS核心概念和基础知识
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 2-3天

## 📋 本部分内容

| 文档 | 标题 | 时间 | 重要性 | 说明 |
|------|------|------|--------|------|
| [01-what-is-epics.md](./01-what-is-epics.md) | EPICS是什么 | 20分钟 | ⭐⭐⭐⭐⭐ | EPICS概述 |
| [02-key-concepts.md](./02-key-concepts.md) | 核心概念 | 30分钟 | ⭐⭐⭐⭐⭐ | PV、Record、IOC等 |
| [03-your-first-ioc.md](./03-your-first-ioc.md) | 第一个IOC | 30分钟 | ⭐⭐⭐⭐⭐ | 创建简单IOC |
| [04-record-types.md](./04-record-types.md) | 记录类型 | 45分钟 | ⭐⭐⭐⭐⭐ | ai、ao、calc等 |
| [05-scanning-basics.md](./05-scanning-basics.md) | 扫描机制 | 30分钟 | ⭐⭐⭐⭐ | Passive、I/O Intr |
| [06-links-and-forwarding.md](./06-links-and-forwarding.md) | 记录链接与数据转发 | 30分钟 | ⭐⭐⭐⭐ | 链接机制、转发 |
| [07-alarms-and-archive.md](./07-alarms-and-archive.md) | 报警和归档 | 25分钟 | ⭐⭐⭐⭐ | 报警、归档系统 |
| [08-ca-tools.md](./08-ca-tools.md) | CA工具 | 20分钟 | ⭐⭐⭐⭐ | caget、caput等 |
| [09-database-files.md](./09-database-files.md) | 数据库文件语法 | 30分钟 | ⭐⭐⭐⭐ | .db文件结构 |
| [10-c-essentials.md](./10-c-essentials.md) | C语言要点 | 40分钟 | ⭐⭐⭐⭐ | 函数指针、动态库 |

## 🎯 学习目标

完成本部分后，你将能够：
- ✅ 理解EPICS的核心概念（PV、Record、IOC、CA）
- ✅ 知道常用记录类型的用途
- ✅ 能创建简单的数据库文件
- ✅ 能使用CA工具访问PV
- ✅ 能用Python编写EPICS客户端
- ✅ 理解EPICS的扫描机制
- ✅ 掌握C语言开发EPICS所需的知识

## 📖 建议阅读顺序

### 快速路径（核心概念）
```
01 → 02 → 04 → 08
总时间: 约2小时
```

### 标准路径（全面学习）
```
01 → 02 → 03 → 04 → 05 → 06 → 08 → 09
总时间: 约4小时
```

### 完整路径（含开发基础）
```
按顺序阅读所有文档
总时间: 约6小时
```

## ✅ 检查清单

学习前：
- [ ] 已完成Part 1（BPMIOC能运行）
- [ ] 能访问PV
- [ ] 对EPICS有初步印象

学习后：
- [ ] 能用自己的话解释PV、Record、IOC
- [ ] 知道ai、ao、calc等记录类型
- [ ] 能创建简单的.db文件
- [ ] 能用caget/caput访问PV
- [ ] 能用Python读写PV
- [ ] 理解Passive和I/O Intr的区别
- [ ] 了解函数指针和动态库

## 🚦 学习路线

```
Part 1: 快速复现      ← 已完成
│
Part 2: 理解基础      ← 你在这里
│  ├─ EPICS概念
│  ├─ 记录类型
│  ├─ CA工具
│  └─ Python接口
│
Part 3: BPMIOC架构    ← 下一步
```

## 💡 学习建议

### 核心概念理解
1. **PV是什么？**
   - 类比：网址URL
   - 本质：分布式数据库的一条记录

2. **Record是什么？**
   - 类比：对象（OOP）
   - 本质：数据+处理逻辑的容器

3. **IOC是什么？**
   - 类比：Web服务器
   - 本质：运行Record的程序

4. **Channel Access是什么？**
   - 类比：HTTP协议
   - 本质：网络通信协议

### 实践建议
1. **边学边做**: 每学一个概念就试一下
2. **创建示例**: 自己写简单的.db文件
3. **画图理解**: 画出PV、Record、IOC的关系图
4. **提问记录**: 记下不理解的地方

### 常见困惑
- ❓ PV和Record的区别？
  → PV是名字，Record是实体

- ❓ 为什么需要设备支持？
  → 连接抽象Record和具体硬件

- ❓ 扫描机制怎么工作？
  → 后面会详细讲，先有个印象

## 📊 概念关系图

```
┌─────────────────────────────────────────┐
│               EPICS 系统                 │
├─────────────────────────────────────────┤
│                                          │
│  IOC (Input/Output Controller)         │
│  ┌────────────────────────────────┐    │
│  │  Database (Record集合)          │    │
│  │  ┌──────────────────────────┐  │    │
│  │  │  Record (ai, ao, etc.)   │  │    │
│  │  │  - 字段 (VAL, SCAN, ...) │  │    │
│  │  │  - 处理逻辑              │  │    │
│  │  └──────────────────────────┘  │    │
│  │                                 │    │
│  │  Device Support                │    │
│  │  Driver                        │    │
│  └────────────────────────────────┘    │
│                                          │
└──────────┬───────────────────────────────┘
           │ Channel Access (网络)
           │
┌──────────▼───────────────────────────────┐
│         Client (客户端)                   │
│  - caget/caput                           │
│  - Python (pyepics)                      │
│  - CSS (控制界面)                         │
└──────────────────────────────────────────┘
```

## 🎓 重要概念速查

### PV (Process Variable)
- **定义**: EPICS中的数据点
- **格式**: `系统:设备:参数` (例: `iLinac_007:BPM14:RF3Amp`)
- **特点**: 全局唯一，网络可访问

### Record
- **定义**: PV的实现，包含值和处理逻辑
- **类型**: ai, ao, bi, bo, calc, waveform等
- **字段**: VAL, SCAN, DESC, EGU, PREC等

### IOC
- **定义**: 运行EPICS数据库的程序
- **功能**: 数据采集、控制、网络服务
- **特点**: 实时、分布式、可扩展

### Channel Access
- **定义**: EPICS的网络协议
- **功能**: 读写PV、监控变化、获取元数据
- **端口**: 5064 (UDP), 5065 (TCP)

## 🔗 相关资源

### 本部分实践
- [Part 1: 06-first-run](../part1-quick-reproduction/06-first-run.md) - 运行IOC
- [Part 1: 07-verify-pvs](../part1-quick-reproduction/07-verify-pvs.md) - 访问PV

### 进阶学习
- [Part 3: BPMIOC架构](../part3-bpmioc-architecture/) - 架构详解
- [Part 8: 基础实验](../part8-hands-on-labs/labs-basic/) - 动手实验

### 官方资源
- [EPICS官网](https://epics-controls.org/)
- [Record Reference](https://epics.anl.gov/base/R3-15/6-docs/)

## 📝 笔记模板

```markdown
# EPICS 核心概念笔记

## PV (Process Variable)
定义：
示例：
我的理解：

## Record
定义：
常用类型：
字段说明：
我的理解：

## IOC
定义：
作用：
我的理解：

## Channel Access
定义：
如何使用：
我的理解：

## 疑问记录
1.
2.
```

---

**准备好了吗？** 从 [01-what-is-epics.md](./01-what-is-epics.md) 开始深入理解EPICS！🚀
