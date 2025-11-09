# Part 7: 构建系统详解

> **目标**: 理解EPICS构建系统和Makefile
> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 1周
> **前置知识**: Part 1, 基础Makefile知识

## 📋 本部分概述

Part 7详细分析EPICS构建系统，包括Makefile的编写和编译流程。

主要内容：
- EPICS构建系统架构
- Makefile详解
- 交叉编译配置
- 依赖管理
- 常见编译问题

完成本部分后，你将能够：
- ✅ 理解EPICS构建流程
- ✅ 能够修改Makefile
- ✅ 掌握交叉编译配置
- ✅ 解决编译错误

## 📚 核心文档

| 文档 | 描述 | 状态 |
|------|------|------|
| README.md | 本文档 | ✅ |
| 01-build-overview.md | 构建系统概述 | ✅ |
| 02-makefile-structure.md | Makefile结构 | ✅ |
| 03-cross-compile.md | 交叉编译 | ✅ |
| 04-dependencies.md | 依赖管理 | ✅ |

## 🎯 学习要点

### EPICS Makefile模板

```makefile
TOP=../..
include $(TOP)/configure/CONFIG

# 库名称
LIBRARY_IOC += BPMmonitor

# 源文件
BPMmonitor_SRCS += driverWrapper.c
BPMmonitor_SRCS += devBPMMonitor.c

# 依赖库
BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

# 数据库文件
DB += BPMMonitor.db

include $(TOP)/configure/RULES
```

### 构建命令

```bash
# 本地编译
make

# 交叉编译（ARM）
make CROSS_COMPILER_TARGET_ARCHS=linux-arm

# 清理
make clean

# 重新配置
make distclean
```

## 🔗 相关文档

- **Part 1**: 快速复现
- **Part 4-6**: 源码分析

## 📚 参考资源

- [EPICS Build System](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/BuildFacility.html)
- [GNU Make Manual](https://www.gnu.org/software/make/manual/)

---

**源码**: `BPMmonitorApp/src/Makefile`, `configure/`
