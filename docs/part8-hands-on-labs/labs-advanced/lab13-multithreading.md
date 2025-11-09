# Lab 13: 多线程调试

> **难度**: ⭐⭐⭐⭐
> **时间**: 3小时
> **前置**: Lab 11, Part 4.6

## 🎯 实验目标

学会调试EPICS IOC的多线程问题。

---

## 🔧 GDB多线程调试

### 常用命令

```bash
# 启动GDB
gdb ./bin/linux-arm/BPMmonitor

# 运行
(gdb) run st.cmd

# 查看所有线程
(gdb) info threads

# 切换线程
(gdb) thread 3

# 查看线程调用栈
(gdb) bt

# 所有线程的backtrace
(gdb) thread apply all bt
```

---

## 📝 实验任务

### 任务1: 定位数据竞争

使用GDB找出多个线程同时访问全局变量的问题。

### 任务2: 添加互斥锁

```c
#include <epicsMutex.h>

epicsMutexId data_mutex;

void init() {
    data_mutex = epicsMutexCreate();
}

void read_data() {
    epicsMutexLock(data_mutex);
    // 访问共享数据
    epicsMutexUnlock(data_mutex);
}
```

---

**恭喜完成Lab 13！** 你已掌握多线程调试技能！💪
