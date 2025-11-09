# Part 10: 调试与测试

> **目标**: 掌握IOC调试和测试技巧
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 1-2周
> **前置知识**: Part 1-6

## 📋 本部分概述

Part 10提供全面的调试和测试指南，帮助你快速定位和解决问题。

主要内容：
- GDB调试技巧
- 日志和追踪
- 性能分析工具
- 单元测试
- 集成测试

完成本部分后，你将能够：
- ✅ 使用GDB调试IOC
- ✅ 添加有效的日志
- ✅ 分析性能瓶颈
- ✅ 编写测试用例

## 📚 核心文档

| 文档 | 描述 | 状态 |
|------|------|------|
| README.md | 本文档 | ✅ |
| **[01-gdb-debugging.md](./01-gdb-debugging.md)** | GDB调试完全指南 | ✅ |
| **[02-logging.md](./02-logging.md)** | 日志系统完全指南 | ✅ |
| **[03-performance-tools.md](./03-performance-tools.md)** | 性能分析工具完全指南 | ✅ |
| **[04-unit-testing.md](./04-unit-testing.md)** | 单元测试完全指南 | ✅ |
| **[05-integration-testing.md](./05-integration-testing.md)** | 集成测试完全指南 | ✅ |

## 🎯 调试技巧速查

### GDB基础命令

```bash
# 启动IOC并附加gdb
gdb --args ./st.cmd

# 设置断点
(gdb) break InitDevice
(gdb) break driverWrapper.c:250

# 运行
(gdb) run

# 单步执行
(gdb) step    # 进入函数
(gdb) next    # 跳过函数

# 查看变量
(gdb) print rf3amp[0]
(gdb) print *prec

# 查看调用栈
(gdb) backtrace

# 查看线程
(gdb) info threads
(gdb) thread 2
```

### 添加调试日志

```c
// 使用printf
printf("[DEBUG] %s:%d - value=%.3f\n", __FILE__, __LINE__, value);

// 使用EPICS errlog
#include <errlog.h>
errlogPrintf("ReadData: offset=%d, channel=%d\n", offset, channel);
```

### 性能分析

```bash
# CPU分析
perf record -g -p $(pidof st.cmd) sleep 10
perf report

# 内存分析
valgrind --leak-check=full ./st.cmd

# 网络分析
tcpdump -i any port 5064 or port 5065
```

## 🔗 相关文档

- **Part 3**: 线程模型、性能分析
- **Part 4-6**: 源码分析
- **Part 8**: 实验室

## 📚 参考资源

- [GDB Manual](https://sourceware.org/gdb/current/onlinedocs/gdb/)
- [Valgrind User Manual](http://valgrind.org/docs/manual/manual.html)
- [Linux Performance Tools](http://www.brendangregg.com/linuxperf.html)

---

**工具**: GDB, Valgrind, perf, strace, tcpdump
