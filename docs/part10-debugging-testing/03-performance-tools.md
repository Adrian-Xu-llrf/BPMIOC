# 性能分析工具完全指南

> **目标**: 掌握IOC性能分析和优化
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 2-3天
> **前置知识**: Linux基础、BPMIOC架构

## 📋 本文档内容

1. 性能分析工具概览
2. perf - CPU性能分析
3. Valgrind - 内存分析
4. strace - 系统调用追踪
5. tcpdump - 网络分析
6. EPICS性能监控
7. 实战案例

## 🎯 为什么需要性能分析

当遇到以下问题时，需要性能分析工具：
- ❌ **CPU占用高** - IOC占用大量CPU
- ❌ **响应慢** - caget/caput延迟高
- ❌ **内存泄漏** - 内存持续增长
- ❌ **吞吐量低** - 数据采集速率不够
- ❌ **网络问题** - CA连接不稳定

## 1️⃣ 性能分析工具概览

| 工具 | 分析目标 | 优点 | 缺点 | 适用场景 |
|------|----------|------|------|----------|
| **perf** | CPU使用 | 低开销、准确 | 需要root权限 | CPU热点分析 |
| **Valgrind** | 内存泄漏 | 详细报告 | 运行缓慢 | 内存问题 |
| **strace** | 系统调用 | 简单直观 | 有性能影响 | 系统调用分析 |
| **tcpdump** | 网络通信 | 完整抓包 | 需要过滤 | 网络问题 |
| **top/htop** | 整体资源 | 实时监控 | 信息有限 | 快速诊断 |
| **gprof** | 函数调用 | 调用图 | 需要重编译 | 代码优化 |

## 2️⃣ perf - CPU性能分析

### 安装perf

```bash
# Ubuntu/Debian
sudo apt-get install linux-tools-common linux-tools-generic

# CentOS/RHEL
sudo yum install perf

# 验证
perf --version
```

### 基本用法

#### 1. 记录性能数据

```bash
# 记录IOC性能数据（10秒）
perf record -g -p $(pidof BPMmonitor) sleep 10

# 参数说明：
# -g: 记录调用图
# -p: 指定进程PID
# sleep 10: 采样10秒

# 生成文件：perf.data
```

#### 2. 查看性能报告

```bash
# 文本报告
perf report

# 输出示例：
# Samples: 10K of event 'cycles'
#   Overhead  Command     Shared Object       Symbol
#   15.23%    BPMmonitor  libBPMDriver.so     [.] BPM_RFIn_ReadADC
#   12.45%    BPMmonitor  BPMmonitor          [.] ReadData
#    8.67%    BPMmonitor  libCom.so           [.] epicsEventWait
#    5.34%    BPMmonitor  libc.so.6           [.] memcpy
```

#### 3. 交互式查看

在 `perf report` 界面：
- 使用箭头键浏览
- 按 `Enter` 展开函数调用
- 按 `/` 搜索函数
- 按 `a` 查看汇编代码
- 按 `q` 退出

### 实战案例：找到CPU热点

**问题**: IOC CPU占用持续在50%

**分析步骤**:

```bash
# 1. 确认问题
top -p $(pidof BPMmonitor)
# 显示：CPU 50%

# 2. 记录性能数据
sudo perf record -g -p $(pidof BPMmonitor) sleep 30

# 3. 查看报告
sudo perf report

# 发现热点：
#   Overhead  Symbol
#   45.67%    [.] ReadData
#   30.12%    [.] memcpy
#   10.23%    [.] BPM_RFIn_ReadADC

# 4. 深入分析ReadData
# 按Enter展开ReadData，发现：
#   45.67% ReadData
#     └─ 42.34% memcpy  ← 问题在这里！
#        └─ 3.33% 其他
```

**发现问题**: `ReadData` 中有大量 `memcpy` 调用

**优化代码**:

```c
// 原来的代码（慢）
float ReadData(int offset, int channel, int type) {
    float buffer[MAX_RF_CHANNELS];
    memcpy(buffer, g_data_buffer[offset], sizeof(buffer));  // 不必要的拷贝
    return buffer[channel];
}

// 优化后的代码（快）
float ReadData(int offset, int channel, int type) {
    return g_data_buffer[offset][channel];  // 直接访问
}
```

### 火焰图 (Flame Graph)

安装火焰图工具：

```bash
git clone https://github.com/brendangregg/FlameGraph
cd FlameGraph
```

生成火焰图：

```bash
# 1. 记录性能数据
sudo perf record -F 99 -p $(pidof BPMmonitor) -g -- sleep 30

# 2. 生成火焰图
sudo perf script | ./FlameGraph/stackcollapse-perf.pl | \
    ./FlameGraph/flamegraph.pl > bpm_flamegraph.svg

# 3. 在浏览器中打开
firefox bpm_flamegraph.svg
```

火焰图解读：
- **宽度**: 函数占用CPU时间（越宽越热）
- **高度**: 调用栈深度
- **颜色**: 随机（无意义）
- **可点击**: 缩放到特定函数

## 3️⃣ Valgrind - 内存分析

### 安装Valgrind

```bash
# Ubuntu/Debian
sudo apt-get install valgrind

# CentOS/RHEL
sudo yum install valgrind

# 验证
valgrind --version
```

### 内存泄漏检测

```bash
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor

# 运行Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind.log \
         ../../bin/linux-x86_64/BPMmonitor st.cmd

# 参数说明：
# --leak-check=full: 详细的泄漏检测
# --show-leak-kinds=all: 显示所有类型的泄漏
# --track-origins=yes: 追踪未初始化值的来源
# --log-file: 输出到文件
```

### 分析Valgrind报告

示例报告：

```
==12345== HEAP SUMMARY:
==12345==     in use at exit: 1,024 bytes in 1 blocks
==12345==   total heap usage: 1,234 allocs, 1,233 frees, 456,789 bytes allocated
==12345==
==12345== 1,024 bytes in 1 blocks are definitely lost in loss record 1 of 1
==12345==    at 0x4C2FB0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==12345==    by 0x108A23: init_record_ai (devBPMMonitor.c:123)
==12345==    by 0x4E8F234: iocInit (iocInit.c:234)
==12345==    by 0x108F56: main (main.c:45)
==12345==
==12345== LEAK SUMMARY:
==12345==    definitely lost: 1,024 bytes in 1 blocks
==12345==    indirectly lost: 0 bytes in 0 blocks
==12345==      possibly lost: 0 bytes in 0 blocks
==12345==    still reachable: 0 bytes in 0 blocks
==12345==         suppressed: 0 bytes in 0 blocks
```

**解读**:
- **definitely lost**: 明确的泄漏（必须修复）
- **indirectly lost**: 间接泄漏（通过指针）
- **possibly lost**: 可能的泄漏（内部指针）
- **still reachable**: 仍可访问（程序结束时未释放，不算严重）

**修复泄漏**:

```c
// 原来的代码（泄漏）
static long init_record_ai(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)malloc(sizeof(DevPvt));
    if (!pPvt) return S_db_noMemory;

    // ... 初始化 ...

    prec->dpvt = pPvt;
    return 0;
}
// 问题：没有对应的free

// 修复后的代码
static long init_record_ai(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)malloc(sizeof(DevPvt));
    if (!pPvt) return S_db_noMemory;

    // ... 初始化 ...

    prec->dpvt = pPvt;
    return 0;
}

// 添加清理函数
static long cleanup_record(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)prec->dpvt;
    if (pPvt) {
        free(pPvt);
        prec->dpvt = NULL;
    }
    return 0;
}
```

### 未初始化内存检测

```c
// 错误代码
float ReadData(int offset, int channel, int type) {
    float value;  // 未初始化
    if (channel == 0) {
        value = g_data_buffer[offset][channel];
    }
    // 如果channel != 0，value未初始化！
    return value;
}
```

Valgrind会报告：

```
==12345== Conditional jump or move depends on uninitialised value(s)
==12345==    at 0x108B45: ReadData (driverWrapper.c:234)
==12345==  Uninitialised value was created by a stack allocation
==12345==    at 0x108B12: ReadData (driverWrapper.c:230)
```

**修复**:

```c
float ReadData(int offset, int channel, int type) {
    float value = 0.0;  // 初始化
    if (channel == 0) {
        value = g_data_buffer[offset][channel];
    }
    return value;
}
```

## 4️⃣ strace - 系统调用追踪

### 基本用法

```bash
# 追踪运行中的IOC
sudo strace -p $(pidof BPMmonitor)

# 追踪启动过程
strace -o strace.log ../../bin/linux-x86_64/BPMmonitor st.cmd

# 只追踪特定系统调用
strace -e open,read,write -p $(pidof BPMmonitor)

# 追踪并显示时间
strace -tt -T -p $(pidof BPMmonitor)
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `-p PID` | 附加到进程 |
| `-o FILE` | 输出到文件 |
| `-e SYSCALL` | 只追踪特定系统调用 |
| `-tt` | 显示微秒时间戳 |
| `-T` | 显示每个调用耗时 |
| `-c` | 统计系统调用次数 |
| `-f` | 追踪子进程 |

### 实战案例：诊断启动慢

**问题**: IOC启动需要30秒

```bash
# 追踪启动过程
strace -tt -T -o startup.log ../../bin/linux-x86_64/BPMmonitor st.cmd

# 分析日志
grep "<[0-9]" startup.log | sort -k2 -r | head -10
```

输出：

```
14:23:45.123456 open("/opt/BPMDriver/lib/libBPMDriver.so", ...) = 3 <25.234567>
14:23:20.123456 connect(4, {...}, 16) = -1 ETIMEDOUT <5.000123>
14:23:15.123456 read(5, "...", 1024) = 1024 <0.123456>
```

**发现问题**:
1. `open()` 耗时25秒 - 库文件可能在慢速NFS上
2. `connect()` 超时5秒 - 网络连接失败

**解决方案**:
1. 将库文件复制到本地磁盘
2. 检查网络配置

### 统计系统调用

```bash
# 统计系统调用次数和耗时
sudo strace -c -p $(pidof BPMmonitor)
# 运行一段时间后按Ctrl+C

# 输出示例：
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 45.67    0.123456          12     10234           read
 30.12    0.081234           8     10234           write
 15.23    0.041234          41      1000           futex
  5.34    0.014234         142       100           open
  3.64    0.009876          98       100           close
------ ----------- ----------- --------- --------- ----------------
100.00    0.270034                 31668       total
```

## 5️⃣ tcpdump - 网络分析

### 基本用法

```bash
# 抓取Channel Access流量（端口5064, 5065）
sudo tcpdump -i any port 5064 or port 5065

# 保存到文件
sudo tcpdump -i any port 5064 or port 5065 -w ca_traffic.pcap

# 过滤特定主机
sudo tcpdump -i any host 192.168.1.100 and port 5064

# 显示详细信息
sudo tcpdump -i any -vv port 5064
```

### 分析CA通信

```bash
# 1. 启动抓包
sudo tcpdump -i any port 5064 or port 5065 -w ca.pcap

# 2. 在另一个终端执行caget
caget LLRF:BPM:RFIn_01_Amp

# 3. 停止抓包（Ctrl+C）

# 4. 使用Wireshark分析
wireshark ca.pcap
```

### CA协议分析

典型的CA通信流程：

```
1. UDP广播（客户端查找IOC）
   Client → 255.255.255.255:5064 [CA_PROTO_SEARCH]

2. UDP响应（IOC回复）
   IOC → Client:5064 [CA_PROTO_SEARCH_RESPONSE]

3. TCP连接（建立虚拟电路）
   Client → IOC:5064 [SYN]
   IOC → Client [SYN-ACK]
   Client → IOC [ACK]

4. CA握手
   Client → IOC [CA_PROTO_VERSION]
   Client → IOC [CA_PROTO_CREATE_CHAN]

5. 读取值
   Client → IOC [CA_PROTO_READ_NOTIFY]
   IOC → Client [CA_PROTO_READ_NOTIFY_RESPONSE]
```

### 诊断CA问题

**问题**: caget超时

```bash
# 抓包分析
sudo tcpdump -i any -vv port 5064 or port 5065

# 在另一个终端
caget LLRF:BPM:RFIn_01_Amp
# Channel connect timed out
```

检查抓包输出：

```
# 情况1: 看不到任何包
# → 防火墙阻止，或IOC未运行

# 情况2: 看到SEARCH但无RESPONSE
# → IOC未监听5064端口，或在不同子网

# 情况3: 看到RESPONSE但无TCP连接
# → 防火墙阻止TCP 5064
```

## 6️⃣ EPICS性能监控

### CA统计信息

在IOC Shell中：

```bash
epics> casr 2  # CA Server Report, level 2
```

输出：

```
Channel Access Server V4.13
  Active Channels: 120
  Virtual Circuits: 5
  Connections: 8

  Channel "LLRF:BPM:RFIn_01_Amp"
    Connected: 192.168.1.100:12345
    Read count: 1234
    Write count: 0

  Channel "LLRF:BPM:RFIn_02_Amp"
    Connected: 192.168.1.101:23456
    Read count: 5678
    Write count: 0
```

### Record扫描统计

```bash
epics> scanppl  # Scan Period Print List
```

输出：

```
1 second scan list:
  LLRF:BPM:RFIn_01_Amp
  LLRF:BPM:RFIn_02_Amp
  ... (共50个Record)

0.5 second scan list:
  LLRF:BPM:RF3Amp
  LLRF:BPM:RF3Pha
  ... (共8个Record)

0.1 second scan list:
  LLRF:BPM:Trigin
```

### 数据库统计

```bash
epics> dbl  # Database List (所有PV)
epics> dbl "LLRF:BPM:*Amp"  # 匹配模式

epics> dbpr "LLRF:BPM:RFIn_01_Amp" 2  # Print Record, level 2
```

输出：

```
ACKS: NO_ALARM      ACKT: YES           ADEL: 0
ALST: 12.345        AMSG:
ASG:                ASP: (nil)          BKPT: 00
DESC:               DISA: 0             DISP: 0
DISS: NO_ALARM      DISV: 1             DPVT: 0x7fff12345678
DTYP: BPMmonitor    EGU: dBm            EGUF: 0
EGUL: 0             ESLO: 1             EVNT: 0
...
SCAN: .5 second     SDIS: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0...
SEVR: NO_ALARM      SIML: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0...
SIMM: NO            SIMS: NO_ALARM      SIOL: 00 00 00 00 00 00 00 00...
SSCN: 65535         STAT: NO_ALARM      SVAL: 0
TIME: 2025-11-09 14:23:45.123456789     TPRO: 0
TSE: 0              TSEL: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0...
UDF: 0              VAL: 12.345
```

### 实时性能监控脚本

创建 `monitor_perf.sh`:

```bash
#!/bin/bash

IOC_NAME="BPMmonitor"
LOG_FILE="/var/log/bpm_performance.log"

while true; do
    echo "=== $(date) ===" >> $LOG_FILE

    # CPU和内存
    ps aux | grep $IOC_NAME | grep -v grep >> $LOG_FILE

    # CA连接数
    echo "CA Connections:" >> $LOG_FILE
    casr 1 | grep "Virtual Circuits" >> $LOG_FILE

    # 线程数
    echo "Threads:" >> $LOG_FILE
    ps -eLf | grep $IOC_NAME | wc -l >> $LOG_FILE

    echo "" >> $LOG_FILE
    sleep 60  # 每分钟记录一次
done
```

## 7️⃣ 综合实战案例

### 案例：IOC性能优化全流程

**问题描述**:
- IOC CPU占用60%
- caget延迟200ms
- 内存缓慢增长

**步骤1: 初步诊断**

```bash
# 1. 查看整体资源
top -p $(pidof BPMmonitor)
# CPU: 60%, MEM: 5.2% (持续增长)

# 2. 查看线程
ps -eLf | grep BPMmonitor
# 发现12个线程（正常应该8个）

# 3. 查看CA连接
casr 1
# Active Channels: 120 (正常)
```

**步骤2: CPU分析**

```bash
# perf分析
sudo perf record -g -p $(pidof BPMmonitor) sleep 30
sudo perf report

# 发现热点：
#   45% - read_ai
#   30% - ReadData
#   15% - memcpy
```

查看源代码，发现问题：

```c
// read_ai中有不必要的日志
static long read_ai(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)prec->dpvt;

    // 问题：每次调用都打印日志！
    errlogPrintf("read_ai: %s\n", prec->name);  // ← 移除这行

    float value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);
    prec->val = value;
    return 0;
}
```

**步骤3: 内存分析**

```bash
# Valgrind检测
valgrind --leak-check=full --log-file=valgrind.log \
         ../../bin/linux-x86_64/BPMmonitor st.cmd
```

发现泄漏：

```c
// 数据采集线程中
void *BPM_Trigin_thread(void *arg) {
    while (1) {
        char *buffer = malloc(1024);  // ← 泄漏！
        // ... 使用buffer ...
        // 忘记free(buffer)
    }
}
```

**步骤4: 网络分析**

```bash
# 抓包分析
sudo tcpdump -i any -c 100 port 5064 or port 5065

# 发现大量重复的SEARCH请求
# → 客户端重复查找PV
```

检查客户端代码，发现问题：

```python
# 错误的Python代码
while True:
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')  # ← 重复创建PV对象
    value = pv.get()
    time.sleep(0.1)
```

应该改为：

```python
# 正确的代码
pv = epics.PV('LLRF:BPM:RFIn_01_Amp')  # 只创建一次
while True:
    value = pv.get()
    time.sleep(0.1)
```

**步骤5: 验证优化效果**

```bash
# 重新测试
top -p $(pidof BPMmonitor)
# CPU: 15% (从60%降低)

# 测试延迟
time caget LLRF:BPM:RFIn_01_Amp
# real 0m0.005s (从200ms降低到5ms)

# 内存监控
watch -n 60 "ps aux | grep BPMmonitor | grep -v grep"
# 内存稳定不增长
```

## 📝 练习任务

### 练习1: perf分析

1. 启动BPMIOC
2. 使用perf记录30秒性能数据
3. 找出CPU占用最高的3个函数
4. 生成火焰图

### 练习2: Valgrind检测

1. 使用Valgrind运行BPMIOC
2. 查找内存泄漏
3. 修复泄漏并验证

### 练习3: strace分析

1. 使用strace追踪IOC启动
2. 找出耗时最长的系统调用
3. 分析原因并优化

### 练习4: CA流量分析

1. 使用tcpdump抓取CA流量
2. 分析caget的完整通信过程
3. 测量从发送请求到收到响应的延迟

## 🔍 性能优化技巧总结

### ✅ 最佳实践

1. **减少日志输出**
   - 生产环境禁用调试日志
   - 使用条件编译

2. **优化数据访问**
   - 避免不必要的内存拷贝
   - 使用缓存减少重复计算

3. **合理设置扫描周期**
   - 根据实际需求选择扫描频率
   - 避免过快的周期扫描

4. **线程优化**
   - 减少锁竞争
   - 使用无锁数据结构

5. **网络优化**
   - 复用CA连接
   - 批量读写PV

### ⚠️ 常见性能陷阱

1. **过度日志**: 每次read/write都打印
2. **内存泄漏**: malloc/free不匹配
3. **锁竞争**: 过度使用互斥锁
4. **系统调用**: 频繁的open/close
5. **网络重连**: 重复创建CA连接

## 📚 参考资源

- **perf教程**: http://www.brendangregg.com/perf.html
- **Valgrind手册**: http://valgrind.org/docs/manual/manual.html
- **火焰图**: http://www.brendangregg.com/flamegraphs.html
- **EPICS性能**: https://epics-controls.org/resources-and-support/documents/appdev/

## 🔗 相关文档

- **[01-gdb-debugging.md](./01-gdb-debugging.md)** - GDB调试
- **[02-logging.md](./02-logging.md)** - 日志系统
- **[Part 3: 08-performance-analysis.md](../part3-bpmioc-architecture/08-performance-analysis.md)** - 性能分析

---

**下一步**: 学习 [单元测试](./04-unit-testing.md)，确保代码质量！
