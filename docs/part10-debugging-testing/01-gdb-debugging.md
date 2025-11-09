# GDB调试完全指南

> **目标**: 掌握使用GDB调试EPICS IOC
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 2-3天
> **前置知识**: C语言基础、BPMIOC架构

## 📋 本文档内容

1. GDB基础
2. 在IOC中使用GDB
3. 常见调试场景
4. 多线程调试
5. 核心转储分析
6. 高级技巧

## 🎯 为什么需要GDB

当遇到以下问题时，GDB是最强大的工具：
- ❌ **段错误(Segmentation Fault)** - 程序崩溃
- ❌ **死锁** - 程序卡住不响应
- ❌ **数据错误** - PV值不对但不知道为什么
- ❌ **性能问题** - 需要找到瓶颈
- ❌ **逻辑错误** - 代码执行流程不符合预期

## 1️⃣ GDB基础

### 安装GDB

```bash
# Ubuntu/Debian
sudo apt-get install gdb

# CentOS/RHEL
sudo yum install gdb

# 验证安装
gdb --version
```

### 编译选项

**重要**: 必须使用调试信息编译IOC！

```makefile
# BPMmonitorApp/src/Makefile
# 添加调试选项
USR_CFLAGS += -g          # 添加调试符号
USR_CFLAGS += -O0         # 禁用优化（可选）
```

重新编译：
```bash
cd /path/to/BPMmonitor
make clean
make
```

### GDB基本命令

| 命令 | 简写 | 说明 |
|------|------|------|
| `run` | `r` | 运行程序 |
| `break` | `b` | 设置断点 |
| `continue` | `c` | 继续执行 |
| `next` | `n` | 单步执行（不进入函数） |
| `step` | `s` | 单步执行（进入函数） |
| `print` | `p` | 打印变量值 |
| `backtrace` | `bt` | 查看调用栈 |
| `list` | `l` | 查看源代码 |
| `info` | `i` | 查看信息 |
| `quit` | `q` | 退出GDB |

## 2️⃣ 在IOC中使用GDB

### 方法1: 直接启动IOC

```bash
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor

# 启动GDB
gdb ../../bin/linux-x86_64/BPMmonitor

# 在GDB中运行
(gdb) run st.cmd
```

### 方法2: 附加到运行中的IOC

```bash
# 找到IOC进程ID
ps aux | grep BPMmonitor

# 附加GDB (假设PID=12345)
sudo gdb -p 12345

# 或者
sudo gdb
(gdb) attach 12345
```

### 方法3: 使用启动脚本

创建调试脚本 `debug.sh`：

```bash
#!/bin/bash
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor
gdb --args ../../bin/linux-x86_64/BPMmonitor st.cmd
```

使用：
```bash
chmod +x debug.sh
./debug.sh
```

## 3️⃣ 常见调试场景

### 场景1: 段错误调试

**问题**: IOC启动时崩溃

```bash
$ ./st.cmd
Segmentation fault (core dumped)
```

**调试步骤**:

```bash
# 1. 启动GDB
gdb ../../bin/linux-x86_64/BPMmonitor

# 2. 运行到崩溃
(gdb) run st.cmd

# 输出类似：
# Program received signal SIGSEGV, Segmentation fault.
# 0x00007ffff7a8b123 in ReadData () at driverWrapper.c:234
# 234         float value = g_data_buffer[offset][channel];

# 3. 查看调用栈
(gdb) backtrace
#0  0x00007ffff7a8b123 in ReadData () at driverWrapper.c:234
#1  0x00007ffff7a8c456 in read_ai () at devBPMMonitor.c:178
#2  0x00007ffff7b2d789 in dbProcess () at dbAccess.c:567
#3  0x00007ffff7b3e012 in scanOnce () at dbScan.c:432

# 4. 查看局部变量
(gdb) info locals
offset = 0
channel = 15
type = 0

# 5. 打印相关变量
(gdb) print offset
$1 = 0
(gdb) print channel
$2 = 15

# 发现问题：channel=15超出了数组范围[0-7]！
```

**修复**:

在 `driverWrapper.c` 中添加检查：

```c
float ReadData(int offset, int channel, int type) {
    // 添加边界检查
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        errlogPrintf("ERROR: Invalid channel %d\n", channel);
        return 0.0;
    }

    float value = g_data_buffer[offset][channel];
    return value;
}
```

### 场景2: 数据错误调试

**问题**: PV `LLRF:BPM:RFIn_01_Amp` 的值总是0

**调试步骤**:

```bash
# 1. 在read_ai设置断点
gdb ../../bin/linux-x86_64/BPMmonitor
(gdb) break devBPMMonitor.c:read_ai

# 2. 运行IOC
(gdb) run st.cmd

# 3. 触发断点（IOC扫描会自动触发）
# Breakpoint 1, read_ai (prec=0x7ffff0001234) at devBPMMonitor.c:178

# 4. 查看Record内容
(gdb) print *prec
$1 = {
  name = "LLRF:BPM:RFIn_01_Amp",
  val = 0.0,
  dpvt = 0x7ffff0005678,
  ...
}

# 5. 查看私有数据
(gdb) print *(DevPvt*)prec->dpvt
$2 = {
  offset = 0,
  channel = 0,
  type = 0,
  ioscanpvt = 0x0
}

# 6. 单步执行read_ai
(gdb) step
# 进入ReadData函数

(gdb) print offset
$3 = 0
(gdb) print channel
$4 = 0

# 7. 查看数据缓冲区
(gdb) print g_data_buffer[0][0]
$5 = 0.0

# 发现问题：数据缓冲区为空！说明数据采集线程没有运行
```

**检查数据采集线程**:

```bash
# 查看所有线程
(gdb) info threads
  Id   Target Id         Frame
  1    Thread 0x7ffff7fe4740 (LWP 12345) read_ai () at devBPMMonitor.c:178
  2    Thread 0x7ffff7200700 (LWP 12346) pthread_cond_wait () at ...
  3    Thread 0x7ffff6fff700 (LWP 12347) pthread_cond_wait () at ...

# 切换到线程2
(gdb) thread 2
(gdb) backtrace
#0  pthread_cond_wait () at ...
#1  epicsEventWait () at ...
#2  BPM_Trigin_thread () at driverWrapper.c:89

# 线程在等待事件，说明硬件触发没有来
# 检查模拟模式是否启用
```

### 场景3: 初始化问题调试

**问题**: IOC启动卡在 `iocInit()`

**调试步骤**:

```bash
# 1. 设置断点在InitDevice
(gdb) break InitDevice

# 2. 运行
(gdb) run st.cmd

# 3. 断点命中
Breakpoint 1, InitDevice () at driverWrapper.c:123

# 4. 单步执行，查看卡在哪里
(gdb) next
(gdb) next
...

# 假设卡在dlopen
(gdb) step
# 进入dlopen调用

# 5. 查看dlopen参数
(gdb) print libname
$1 = "/path/to/wrong/library.so"

# 发现问题：库路径错误
```

## 4️⃣ 多线程调试

BPMIOC有多个线程：
- **主线程**: IOC shell
- **扫描线程**: Record扫描
- **数据采集线程**: BPM_Trigin_thread
- **CA服务器线程**: Channel Access

### 查看所有线程

```bash
(gdb) info threads
  Id   Target Id                          Frame
* 1    Thread 0x7ffff7fe4740 (LWP 12345)  main ()
  2    Thread 0x7ffff7200700 (LWP 12346)  dbScan ()
  3    Thread 0x7ffff6fff700 (LWP 12347)  BPM_Trigin_thread ()
  4    Thread 0x7ffff6dfe700 (LWP 12348)  ca_server_thread ()
```

### 切换线程

```bash
# 切换到线程3
(gdb) thread 3

# 查看当前线程调用栈
(gdb) backtrace

# 查看所有线程的调用栈
(gdb) thread apply all backtrace
```

### 在特定线程设置断点

```bash
# 只在线程3触发断点
(gdb) break BPM_Trigin_thread thread 3

# 或者
(gdb) break driverWrapper.c:250 thread 3
```

### 调试死锁

**症状**: IOC卡住，不响应caget命令

```bash
# 1. 附加到运行中的IOC
sudo gdb -p $(pidof BPMmonitor)

# 2. 查看所有线程
(gdb) info threads

# 3. 查看每个线程在做什么
(gdb) thread apply all backtrace

# 输出示例：
# Thread 3:
# #0  pthread_mutex_lock () at ...
# #1  ReadData () at driverWrapper.c:234
#
# Thread 4:
# #0  pthread_mutex_lock () at ...
# #1  SetReg () at driverWrapper.c:456

# 发现：两个线程都在等待锁！
```

## 5️⃣ 核心转储分析

### 启用核心转储

```bash
# 检查核心转储限制
ulimit -c

# 如果是0，启用核心转储
ulimit -c unlimited

# 永久启用（添加到~/.bashrc）
echo "ulimit -c unlimited" >> ~/.bashrc
```

### 设置核心文件路径

```bash
# 查看当前设置
cat /proc/sys/kernel/core_pattern

# 设置到特定目录
sudo sysctl -w kernel.core_pattern=/tmp/core-%e-%p-%t

# 永久设置
echo "kernel.core_pattern=/tmp/core-%e-%p-%t" | sudo tee -a /etc/sysctl.conf
```

### 分析核心文件

```bash
# IOC崩溃后生成core文件
$ ./st.cmd
Segmentation fault (core dumped)

# 使用GDB分析
gdb ../../bin/linux-x86_64/BPMmonitor /tmp/core-BPMmonitor-12345-1234567890

# 自动显示崩溃位置
(gdb)
Core was generated by `./BPMmonitor st.cmd'.
Program terminated with signal SIGSEGV, Segmentation fault.
#0  0x00007ffff7a8b123 in ReadData () at driverWrapper.c:234
234         float value = g_data_buffer[offset][channel];

# 查看调用栈
(gdb) backtrace

# 查看变量
(gdb) info locals
```

## 6️⃣ 高级技巧

### 条件断点

```bash
# 只在channel=3时中断
(gdb) break ReadData if channel == 3

# 只在value > 100时中断
(gdb) break read_ai if prec->val > 100.0
```

### 监视点 (Watchpoint)

```bash
# 监视变量rf3amp[0]的变化
(gdb) watch rf3amp[0]

# 当rf3amp[0]被修改时，GDB会中断并显示旧值和新值
Hardware watchpoint 2: rf3amp[0]
Old value = 12.5
New value = 15.3
```

### 打印复杂数据结构

```bash
# 打印aiRecord结构
(gdb) print *prec
# 输出很多...

# 只打印特定字段
(gdb) print prec->name
(gdb) print prec->val
(gdb) print prec->dpvt

# 打印数组
(gdb) print rf3amp
(gdb) print rf3amp[0]@8  # 打印前8个元素
```

### 自动命令

创建GDB命令文件 `.gdbinit`:

```bash
# .gdbinit
# 自动加载符号
set auto-load safe-path /

# 设置断点
break InitDevice
break ReadData
break read_ai

# 定义自定义命令
define print_pv
  print prec->name
  print prec->val
end

# 启动时运行
run st.cmd
```

使用：
```bash
gdb -x .gdbinit ../../bin/linux-x86_64/BPMmonitor
```

### 反向调试 (Reverse Debugging)

**需要GDB 7.0+**

```bash
# 启用记录
(gdb) target record-full

# 反向执行
(gdb) reverse-next     # 反向单步
(gdb) reverse-step     # 反向进入函数
(gdb) reverse-continue # 反向运行到上一个断点
```

### Python脚本

GDB支持Python扩展：

创建 `gdb_pretty.py`:

```python
import gdb

class DevPvtPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        offset = self.val['offset']
        channel = self.val['channel']
        type_val = self.val['type']
        return f"DevPvt(offset={offset}, channel={channel}, type={type_val})"

def lookup_type(val):
    if str(val.type) == "DevPvt":
        return DevPvtPrinter(val)
    return None

gdb.pretty_printers.append(lookup_type)
```

使用：
```bash
(gdb) source gdb_pretty.py
(gdb) print *(DevPvt*)prec->dpvt
DevPvt(offset=0, channel=3, type=0)
```

## 📝 实战案例

### 案例1: 调试PV值异常

**问题**: `LLRF:BPM:RFIn_03_Pha` 相位值突然跳变到奇怪的数值

**调试过程**:

```bash
# 1. 设置条件断点
(gdb) break read_ai if strcmp(prec->name, "LLRF:BPM:RFIn_03_Pha") == 0

# 2. 运行IOC
(gdb) run st.cmd

# 3. 断点命中时查看数据流
(gdb) print *(DevPvt*)prec->dpvt
$1 = {offset = 2, channel = 2, type = 0}  # OFFSET_PHA=2

# 4. 单步进入ReadData
(gdb) step

# 5. 在ReadData中检查
(gdb) print offset
$2 = 2
(gdb) print channel
$3 = 2
(gdb) print g_data_buffer[2][2]
$4 = 1234567.89  # 异常值！

# 6. 查看是谁写入了这个值
(gdb) watch g_data_buffer[2][2]

# 7. 继续执行
(gdb) continue

# 输出：
# Hardware watchpoint 3: g_data_buffer[2][2]
# Old value = 1234567.89
# New value = 45.23
# BPM_Trigin_thread () at driverWrapper.c:95

# 发现：是数据采集线程写入的
# 检查BPM_RFIn_ReadADC函数
```

### 案例2: 调试内存泄漏

虽然内存泄漏通常用Valgrind，但也可用GDB：

```bash
# 1. 在init_record设置断点
(gdb) break init_record_ai

# 2. 运行IOC
(gdb) run st.cmd

# 3. 每次断点命中时检查malloc调用
(gdb) backtrace
(gdb) continue

# 重复多次后，检查堆内存
(gdb) call malloc_stats()
```

## 🔍 调试技巧总结

### ✅ 最佳实践

1. **始终使用 `-g` 编译**
   - 生产环境可以用 `-g -O2`（保留优化）

2. **善用断点**
   - 函数断点：`break InitDevice`
   - 行断点：`break driverWrapper.c:234`
   - 条件断点：`break read_ai if channel == 3`

3. **理解调用栈**
   - 使用 `backtrace` 查看函数调用链
   - 使用 `frame N` 切换栈帧

4. **检查所有线程**
   - 多线程程序必须检查所有线程
   - `thread apply all backtrace`

5. **保存核心文件**
   - 启用核心转储用于事后分析
   - `ulimit -c unlimited`

### ⚠️ 常见陷阱

1. **编译优化影响调试**
   - `-O2` 会导致变量被优化掉
   - 调试时使用 `-O0`

2. **符号剥离**
   - `strip` 会删除调试符号
   - 保留未剥离的二进制文件

3. **内联函数**
   - 内联函数无法设置断点
   - 使用 `__attribute__((noinline))` 或 `-fno-inline`

4. **时间相关的Bug**
   - GDB会改变程序时序
   - 某些竞态条件在调试时可能不出现

## 🎯 练习任务

### 练习1: 基础调试

1. 在 `InitDevice` 设置断点
2. 查看 `g_lib_handle` 的值
3. 单步执行 `dlopen` 调用
4. 打印加载的库路径

### 练习2: 数据追踪

1. 在 `read_ai` 设置断点
2. 查看 `prec->dpvt` 指向的DevPvt结构
3. 单步进入 `ReadData`
4. 打印返回值

### 练习3: 多线程调试

1. 启动IOC
2. 附加GDB
3. 查看所有线程
4. 在数据采集线程设置断点

### 练习4: 核心转储分析

1. 故意制造一个段错误（访问NULL指针）
2. 生成核心文件
3. 用GDB分析核心文件
4. 找到崩溃原因

## 📚 参考资源

- **GDB官方文档**: https://sourceware.org/gdb/current/onlinedocs/gdb/
- **GDB教程**: https://www.gnu.org/software/gdb/documentation/
- **EPICS调试**: https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/node8.html

## 🔗 相关文档

- **[02-logging.md](./02-logging.md)** - 日志系统
- **[03-performance-tools.md](./03-performance-tools.md)** - 性能分析工具
- **[Part 3: 07-error-handling.md](../part3-bpmioc-architecture/07-error-handling.md)** - 错误处理

---

**下一步**: 学习 [日志系统](./02-logging.md)，配合GDB使用更高效！
