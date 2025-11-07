# Part 1: 快速复现

> **目标**: 30分钟内让BPMIOC在模拟模式下运行起来
> **难度**: ⭐⭐☆☆☆
> **预计时间**: 1-3小时（含学习）

## 📋 本部分内容

| 文档 | 标题 | 时间 | 重要性 | 说明 |
|------|------|------|--------|------|
| [01-introduction.md](./01-introduction.md) | BPMIOC简介 | 10分钟 | ⭐⭐⭐⭐ | BPMIOC是什么 |
| [02-prerequisites.md](./02-prerequisites.md) | 前置条件 | 5分钟 | ⭐⭐⭐⭐ | 系统要求检查 |
| [03-install-epics.md](./03-install-epics.md) | 安装EPICS Base | 15分钟 | ⭐⭐⭐⭐⭐ | 详细安装步骤 |
| [04-clone-and-compile.md](./04-clone-and-compile.md) | 克隆和编译 | 10分钟 | ⭐⭐⭐⭐⭐ | 编译BPMIOC |
| [05-enable-simulation.md](./05-enable-simulation.md) | 启用模拟模式 | 10分钟 | ⭐⭐⭐⭐⭐ | 无硬件运行 |
| [06-first-run.md](./06-first-run.md) | 第一次运行 | 5分钟 | ⭐⭐⭐⭐⭐ | 启动IOC |
| [07-verify-pvs.md](./07-verify-pvs.md) | 验证PV访问 | 10分钟 | ⭐⭐⭐⭐⭐ | 测试PV |
| [08-troubleshooting.md](./08-troubleshooting.md) | 常见问题 | 按需 | ⭐⭐⭐⭐ | 故障排查 |
| [09-docker-alternative.md](./09-docker-alternative.md) | Docker部署 | 20分钟 | ⭐⭐⭐ | 可选方案 |

## 🎯 学习目标

完成本部分后，你将能够：
- ✅ 安装并配置EPICS Base
- ✅ 编译BPMIOC项目
- ✅ 在模拟模式下运行IOC
- ✅ 使用CA工具访问PV
- ✅ 用Python读写PV
- ✅ 解决常见的编译和运行问题

## 📖 建议阅读顺序

### 快速路径（已有EPICS环境）
```
01 → 04 → 05 → 06 → 07
总时间: 约40分钟
```

### 标准路径（从零开始）
```
01 → 02 → 03 → 04 → 05 → 06 → 07
总时间: 约1小时
```

### 完整路径（包含Docker）
```
01 → 02 → 03 → 04 → 05 → 06 → 07 → 09
遇到问题查看: 08
总时间: 约2小时
```

## ✅ 检查清单

在开始之前：
- [ ] Linux系统（Ubuntu 20.04+ 或 CentOS 7+）
- [ ] 至少5GB可用磁盘空间
- [ ] 2GB+内存
- [ ] 网络连接
- [ ] sudo权限（安装依赖）

完成本部分后：
- [ ] EPICS Base已安装并配置环境变量
- [ ] BPMIOC编译成功
- [ ] IOC能在模拟模式下启动
- [ ] 能用caget读取PV值
- [ ] 能用camonitor监控PV变化
- [ ] 能用Python访问PV

## 🚦 学习路线

```
你在这里 ↓

Part 1: 快速复现      ← 现在
│
├─ 安装EPICS
├─ 编译BPMIOC
├─ 运行IOC
└─ 验证PV
   │
   ↓
Part 2: 理解基础      ← 下一步
```

## 💡 学习建议

1. **按顺序阅读**: 文档之间有依赖关系
2. **动手实践**: 每个步骤都要实际操作
3. **复制命令**: 可以直接复制文档中的命令
4. **记录问题**: 遇到问题记录下来，查看08-troubleshooting.md
5. **不要跳步**: 即使看起来简单，也要执行每个步骤

## 🐛 遇到问题？

1. **先查看** [08-troubleshooting.md](./08-troubleshooting.md)
2. **检查系统要求** [02-prerequisites.md](./02-prerequisites.md)
3. **查看错误日志**: 仔细阅读错误信息
4. **Google错误信息**: 复制错误信息搜索
5. **提Issue**: 在GitHub上提问

## 📊 时间分配

```
├─ 阅读文档: 30%
├─ 实际操作: 50%
└─ 问题排查: 20%
```

## 🎓 常见问题预告

- ❓ EPICS Base编译失败 → 查看03文档
- ❓ BPMIOC编译失败 → 查看04文档和08文档
- ❓ IOC启动但PV访问不到 → 查看07文档
- ❓ caget命令找不到 → 检查环境变量（03文档）

## 🔗 相关资源

- [QUICK_START.md](../QUICK_START.md) - 更简洁的快速开始
- [Part 2: 理解基础](../part2-understanding-basics/) - 下一步学习
- [Part 18: 附录/FAQ](../part18-appendix/faq.md) - 常见问题

## 📞 获取帮助

- 💬 GitHub Discussions
- 🐛 GitHub Issues
- 📧 联系维护者

---

**准备好了吗？** 从 [01-introduction.md](./01-introduction.md) 开始吧！🚀
