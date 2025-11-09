# 08: BPMIOC性能分析

> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 60分钟
> **前置知识**: 06-thread-model.md, 04-memory-model.md

## 📋 本文目标

本文分析BPMIOC的性能特征，帮助你理解系统瓶颈和优化方向。

完成本文后，你将能够：
- ✅ 理解系统的性能指标
- ✅ 识别性能瓶颈
- ✅ 掌握性能测量工具
- ✅ 实施性能优化

## ⏱️ 性能指标总览

### 1. 关键性能指标（KPI）

| 指标 | 目标值 | 实际值 | 说明 |
|------|--------|--------|------|
| **数据采集频率** | 10 Hz | 10 Hz | 每秒10次采集 |
| **端到端延迟** | < 20ms | ~12ms | 硬件触发到客户端收到 |
| **PV更新速率** | 10 Hz/PV | 10 Hz | 每个I/O Intr PV |
| **CPU占用率** | < 10% | ~5% | 在ZYNQ板上 |
| **内存占用** | < 20 MB | ~10 MB | IOC进程 |
| **网络带宽** | < 10 Mb/s | ~2 Mb/s | CA协议流量 |

### 2. 时间分解

```
总周期: 100ms (10 Hz)

├─ 硬件采集: ~1ms (1%)
│   └─ funcTriggerAllDataReached()
│
├─ scanIoRequest触发: ~1μs (0.001%)
│
├─ Record处理: ~10ms (10%)
│   ├─ read_ai(): ~0.1ms × 50 = 5ms
│   ├─ read_wf(): ~1ms × 5 = 5ms
│   └─ 其他处理
│
├─ CA网络传输: ~1ms (1%)
│   └─ 发送到所有客户端
│
└─ 空闲等待: ~88ms (88%)
    └─ usleep(100000)

性能瓶颈: Record处理（10ms，主要是波形拷贝）
```

## 📊 CPU性能分析

### 1. CPU时间分布

使用`perf`工具分析：

```bash
# 安装perf
sudo apt-get install linux-tools-generic

# 采样30秒
sudo perf record -p $(pidof st.cmd) -g sleep 30

# 生成报告
sudo perf report

# 输出示例（简化）:
# Samples: 1K of event 'cpu-clock'
#  30.00%  pthread         libBPMboard14And15.so  [.] funcTriggerAllDataReached
#  25.00%  callback-high   st.cmd                 [.] read_wf
#  15.00%  callback-high   st.cmd                 [.] read_ai
#  10.00%  CA-client       libc.so.6              [.] __memcpy_avx_unaligned
#   5.00%  CA-server       libca.so               [.] ca_process_messages
#  15.00%  其他
```

**分析**：
- 30% 时间在硬件采集（正常）
- 25% 时间在Waveform读取（可优化）
- 15% 时间在标量读取
- 10% 时间在memcpy（波形拷贝）

### 2. CPU占用测量

```bash
# 方法1：使用top
top -p $(pidof st.cmd)

# 输出：
  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
 1234 user      20   0   15280  10240   5120 S   5.0  2.0   0:05.23 st.cmd

# 方法2：使用htop（更直观）
htop -p $(pidof st.cmd)
```

## 🧠 内存性能分析

### 1. 内存占用测量

```bash
# 查看详细内存映射
cat /proc/$(pidof st.cmd)/status | grep -i vm

# 输出：
VmPeak:    15280 kB  # 峰值虚拟内存
VmSize:    15280 kB  # 当前虚拟内存
VmRSS:     10240 kB  # 实际物理内存（~10 MB）
VmData:     8192 kB  # 数据段（全局数组）
VmStk:       132 kB  # 栈
VmExe:       256 kB  # 代码段
```

### 2. 内存访问模式

```c
// 缓存友好的访问（连续）
for (int i = 0; i < buf_len; i++) {
    rf3amp[i] = rf3amp[i] * 2.0;  // 顺序访问，缓存命中率高
}

// 缓存不友好的访问（跳跃）
for (int i = 0; i < buf_len; i += 1000) {
    rf3amp[i] = ...;  // 跳跃访问，缓存命中率低
}
```

## 🌐 网络性能分析

### 1. CA协议流量

```bash
# 使用tcpdump抓包
sudo tcpdump -i any port 5064 or port 5065 -w ca.pcap

# 使用wireshark分析
wireshark ca.pcap

# 流量估算
每个PV更新 = ~100字节（CA协议开销）
500个PV × 10 Hz × 100 bytes = 500 KB/s = 4 Mb/s
```

### 2. CA Monitor优化

```
# 方法1：使用camonitor（推荐）
$ camonitor LLRF:BPM:RF3Amp
# 自动使用CA Monitor，只接收变化的数据

# 方法2：使用caget轮询（不推荐）
$ while true; do caget LLRF:BPM:RF3Amp; sleep 0.1; done
# 每次都发起新连接，效率低
```

## 🚀 性能优化策略

### 优化1：减少Waveform拷贝

**问题**：每次读取波形都memcpy 10000点

```c
// 当前实现（慢）
long read_wf(waveformRecord *prec)
{
    // 拷贝10000点 × 4字节 = 40 KB
    memcpy(prec->bptr, rf3amp, prec->nelm * sizeof(float));
    prec->nord = buf_len;
    return 0;
}
```

**优化方案**：零拷贝（需要修改EPICS Base）

```c
// 理想实现（快，但需要EPICS支持）
long read_wf(waveformRecord *prec)
{
    // 直接指向全局数组（零拷贝）
    prec->bptr = rf3amp;
    prec->nord = buf_len;
    return 0;
}
```

**实际优化**：减少拷贝频率

```c
// 方案：只在数据变化时拷贝
static float last_rf3amp[buf_len];

long read_wf(waveformRecord *prec)
{
    // 检查数据是否变化
    if (memcmp(rf3amp, last_rf3amp, buf_len * sizeof(float)) == 0) {
        // 数据未变化，跳过拷贝
        return 0;
    }

    // 数据变化了，拷贝
    memcpy(prec->bptr, rf3amp, prec->nelm * sizeof(float));
    memcpy(last_rf3amp, rf3amp, buf_len * sizeof(float));

    prec->nord = buf_len;
    return 0;
}
```

### 优化2：使用SIMD指令加速

```c
// 使用AVX2指令集加速memcpy
#include <immintrin.h>

void fast_copy_float(float *dst, const float *src, size_t n)
{
    size_t i = 0;

    // 每次处理8个float（256位）
    for (; i + 8 <= n; i += 8) {
        __m256 vec = _mm256_loadu_ps(&src[i]);
        _mm256_storeu_ps(&dst[i], vec);
    }

    // 处理剩余元素
    for (; i < n; i++) {
        dst[i] = src[i];
    }
}
```

### 优化3：调整采集周期

```c
// 根据实际需求调整周期

// 快速模式（20 Hz）
#define ACQUISITION_PERIOD_US 50000   // 50ms

// 正常模式（10 Hz）
#define ACQUISITION_PERIOD_US 100000  // 100ms

// 节能模式（5 Hz）
#define ACQUISITION_PERIOD_US 200000  // 200ms

void *pthread(void *arg)
{
    while (1) {
        // ...
        usleep(ACQUISITION_PERIOD_US);
    }
}
```

### 优化4：减少PV数量

```
# 方法1：合并相关PV
# 不要为每个通道创建单独的PV

# 差：8个独立PV
LLRF:BPM:Va1
LLRF:BPM:Vb1
LLRF:BPM:Vc1
LLRF:BPM:Vd1
...

# 好：1个Waveform PV包含所有通道
LLRF:BPM:VabcdArray  # Waveform [8]
```

### 优化5：使用Channel Access Archive

```c
// 不要每次都传输所有PV
// 使用CA的Monitor Deadband

record(ai, "LLRF:BPM:RF3Amp")
{
    field(DTYP, "BPM")
    field(INP,  "@0:3")
    field(SCAN, "I/O Intr")

    # 只有变化超过0.01时才通知客户端
    field(MDEL, "0.01")  # Monitor Deadband
    field(ADEL, "0.01")  # Archive Deadband
}
```

## 📈 性能测试工具

### 1. 吞吐量测试

```bash
# 测试IOC能处理多少个caget请求
#!/bin/bash

start_time=$(date +%s)
count=0

for i in {1..1000}; do
    caget LLRF:BPM:RF3Amp > /dev/null
    ((count++))
done

end_time=$(date +%s)
elapsed=$((end_time - start_time))

echo "Throughput: $((count / elapsed)) requests/second"
```

### 2. 延迟测试

```bash
# 测量端到端延迟
#!/bin/bash

# 记录触发时间（在IOC中添加时间戳PV）
trigger_time=$(caget -t LLRF:BPM:TriggerTime)

# 等待数据更新
sleep 0.05

# 读取数据时间戳
data_time=$(caget -t LLRF:BPM:RF3Amp.TIME)

# 计算延迟
latency=$((data_time - trigger_time))
echo "Latency: ${latency}ms"
```

### 3. 负载测试

```python
# 使用pyepics模拟多个客户端

import epics
import threading
import time

def monitor_pv(pv_name):
    pv = epics.PV(pv_name)
    def callback(pvname=None, value=None, **kw):
        pass  # 只接收，不处理
    pv.add_callback(callback)

# 创建100个客户端，每个监控10个PV
for i in range(100):
    for j in range(10):
        pv_name = f"LLRF:BPM:RF{j%8}Amp"
        threading.Thread(target=monitor_pv, args=(pv_name,)).start()

# 运行5分钟
time.sleep(300)

print("Load test completed")
```

## 🎯 性能优化清单

### 启动优化
- [ ] 减少不必要的PV
- [ ] 延迟加载非关键数据库
- [ ] 优化st.cmd脚本

### 运行时优化
- [ ] 调整数据采集周期
- [ ] 使用Monitor Deadband
- [ ] 减少Waveform拷贝
- [ ] 优化Record处理顺序

### 内存优化
- [ ] 减小缓冲区大小（如果可行）
- [ ] 使用内存池避免碎片
- [ ] 监控内存泄漏

### 网络优化
- [ ] 使用CA Monitor而非轮询
- [ ] 减少PV数量
- [ ] 使用本地网络减少延迟

## ✅ 学习检查点

完成本文后，你应该能够回答：

1. **性能指标**：
   - [ ] BPMIOC的主要性能瓶颈在哪里？
   - [ ] 端到端延迟是多少？
   - [ ] CPU和内存占用是多少？

2. **性能分析**：
   - [ ] 如何测量CPU占用率？
   - [ ] 如何分析内存使用？
   - [ ] 如何测量网络流量？

3. **性能优化**：
   - [ ] 如何减少Waveform拷贝开销？
   - [ ] Monitor Deadband的作用？
   - [ ] 如何优化CA网络流量？

## 🔗 相关文档

- **[04-memory-model.md](./04-memory-model.md)** - 内存模型
- **[06-thread-model.md](./06-thread-model.md)** - 线程模型
- **[Part 12: 进阶主题](/docs/part12-advanced-topics/)** - 性能优化深入

## 📚 扩展阅读

- [EPICS Application Developer's Guide - Performance](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/Performance.html)
- [Linux Performance Tools](http://www.brendangregg.com/linuxperf.html)

---

**下一篇**: [09-design-patterns.md](./09-design-patterns.md) - 设计模式应用

**实践练习**:
1. 使用`top`测量IOC的CPU和内存占用
2. 使用`perf`分析CPU热点函数
3. 添加Monitor Deadband优化网络流量
4. 测量端到端延迟
