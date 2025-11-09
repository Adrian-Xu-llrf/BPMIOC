# 案例5: 故障排查实战

> **问题**: 生产环境IOC随机崩溃
> **时长**: 1天（复现2h + 分析4h + 修复1h + 验证1h）
> **难度**: ⭐⭐⭐⭐⭐
> **关键技术**: GDB、Valgrind、多线程调试、Core Dump分析

## 1. 问题现象

### 1.1 初始报告

**时间**: 2025-11-05 14:32
**报告人**: 运维工程师
**问题描述**:

> BPM IOC在生产环境运行72小时后崩溃，PV无法访问。
> 重启后正常，但24-48小时后再次崩溃。

**影响**:
- 束流监控中断
- 实验数据丢失
- 运维需要每天重启IOC

### 1.2 日志信息

```bash
# /var/log/syslog
Nov 5 14:32:15 beamline01 kernel: [123456.789] BPMmonitor[12345]: segfault at 0 ip 00007f8b4c2a1234 sp 00007f8b4c2a5678 error 4 in libBPMDriver.so[7f8b4c2a0000+10000]

# IOC控制台输出（通过screen查看）
epics> camonitor LLRF:BPM:RFIn_01_Amp
LLRF:BPM:RFIn_01_Amp  2025-11-05 14:32:14.234567  12.345
LLRF:BPM:RFIn_01_Amp  2025-11-05 14:32:14.334567  12.347
Segmentation fault (core dumped)
```

### 1.3 初步观察

- 崩溃时间不固定（24-72小时）
- 没有明显的触发条件
- Core dump文件已生成

## 2. 故障排查过程

### 2.1 分析Core Dump

#### 加载Core文件

```bash
# 查看core文件
ls -lh /var/crash/
-rw------- 1 root root 45M Nov  5 14:32 core.BPMmonitor.12345

# 使用GDB分析
gdb /opt/BPMmonitor/bin/linux-arm/BPMmonitor /var/crash/core.BPMmonitor.12345

(gdb) bt
#0  0x00007f8b4c2a1234 in BPM_RFIn_ReadADC () from ./libBPMDriver.so
#1  0x00000000004052a8 in AcquireThread (arg=0x0) at driverWrapper.c:156
#2  0x00007f8b4c3b1234 in start_thread () from /lib/arm-linux-gnueabihf/libpthread.so.0
#3  0x00007f8b4c4c5678 in clone () from /lib/arm-linux-gnueabihf/libc.so.6

(gdb) frame 1
#1  0x00000000004052a8 in AcquireThread (arg=0x0) at driverWrapper.c:156
156         g_data_buffer[offset][channel] = BPM_RFIn_ReadADC(channel, offset);

(gdb) print channel
$1 = 7

(gdb) print offset
$2 = 14    ← 注意：offset超出范围！

(gdb) print NUM_OFFSETS
$3 = 14

(gdb) info threads
  Id   Target Id         Frame
  3    Thread 0x7f8b4c2a5700 (LWP 12347) 0x00007f8b4c3b5678 in pthread_cond_wait
  2    Thread 0x7f8b4c3a6700 (LWP 12346) 0x00007f8b4c2a1234 in BPM_RFIn_ReadADC
* 1    Thread 0x7f8b4c4a7700 (LWP 12345) 0x00007f8b4c3b1234 in start_thread
```

**发现**:
- 崩溃发生在`AcquireThread`线程
- `offset = 14`，而`NUM_OFFSETS = 14`（数组越界！）
- 访问`g_data_buffer[14][7]`导致段错误

#### 检查源代码

```c
// driverWrapper.c:150-160
void* AcquireThread(void* arg) {
    while (g_running) {
        for (int ch = 0; ch < MAX_RF_CHANNELS; ch++) {
            for (int off = 0; off < NUM_OFFSETS; off++) {  // ← 应该是 off <= NUM_OFFSETS
                g_data_buffer[off][ch] = BPM_RFIn_ReadADC(ch, off);
            }
        }
        usleep(100000);
    }
}

// 数组定义
#define NUM_OFFSETS 14
static float g_data_buffer[NUM_OFFSETS][MAX_RF_CHANNELS];
```

**疑问**: 循环条件`off < NUM_OFFSETS`看起来是对的（0-13），为什么会访问到`offset=14`？

### 2.2 深入分析：竞态条件

#### 怀疑：并发修改

检查是否有其他地方修改了循环变量或数组：

```bash
grep -rn "NUM_OFFSETS\|g_data_buffer" BPMmonitorApp/src/

# 发现可疑代码：
devBPMMonitor.c:89:    int new_offset = NUM_OFFSETS + extra_offset;
devBPMMonitor.c:90:    float value = ReadData(new_offset, channel, 0);
```

查看代码：

```c
// devBPMMonitor.c:85-95
static long read_ai(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)prec->dpvt;

    // 新增的"扩展偏移"功能（开发者最近添加）
    int extra_offset = 0;
    if (strstr(prec->name, "Extended") != NULL) {
        extra_offset = 1;  // ← BUG: 会导致offset=14
    }

    int new_offset = pPvt->offset + extra_offset;
    float value = ReadData(new_offset, pPvt->channel, 0);
    // ...
}
```

**但这里只是读取，不会导致AcquireThread崩溃。继续查找...**

#### 使用Valgrind检测

在开发机上用Valgrind运行：

```bash
valgrind --leak-check=full --track-origins=yes \
         ../../bin/linux-x86_64/BPMmonitor st.cmd

# 输出
==12345== Invalid write of size 4
==12345== at 0x4052A8: AcquireThread (driverWrapper.c:156)
==12345== Address 0x7f8b4c300000 is 0 bytes after a block of size 448 alloc'd
==12345== at 0x4C2DB8F: malloc (in /usr/lib/valgrind/vgpreload_memcheck.so)

==12345== Thread 2:
==12345== Invalid read of size 4
==12345== at 0x405123: ReadData (driverWrapper.c:182)
==12345== by 0x405456: read_ai (devBPMMonitor.c:91)

# 进一步信息
==12345== Conditional jump or move depends on uninitialised value(s)
==12345== at 0x4052A0: AcquireThread (driverWrapper.c:154)
==12345== Uninitialised value was created by a heap allocation
```

**发现**: 有未初始化的值影响循环条件！

#### 深入GDB调试

设置条件断点：

```bash
gdb ../../bin/linux-x86_64/BPMmonitor

(gdb) break driverWrapper.c:156 if offset >= NUM_OFFSETS
Breakpoint 1 at 0x4052a8: file driverWrapper.c, line 156.

(gdb) run st.cmd
# ... 运行一段时间后 ...

Breakpoint 1, AcquireThread (arg=0x0) at driverWrapper.c:156
156         g_data_buffer[offset][channel] = BPM_RFIn_ReadADC(channel, offset);

(gdb) print offset
$1 = 14

(gdb) print &offset
$2 = (int *) 0x7f8b4c2a56f8

(gdb) watch -l offset
Hardware watchpoint 2: -location offset

(gdb) continue

# 触发watch point
Hardware watchpoint 2: -location offset
Old value = 13
New value = 14
0x00405678 in UpdateOffsetCount (count=15) at driverWrapper.c:225
225     NUM_OFFSETS = count;  // ← 找到了！
```

**根本原因找到**！

#### 发现真正的Bug

```c
// driverWrapper.c:220-230
// 动态更新offset数量的功能（最近添加）
void UpdateOffsetCount(int count) {
    if (count > 0 && count <= 20) {
        NUM_OFFSETS = count;  // ← BUG! NUM_OFFSETS是#define
        // 实际修改了某个内存位置！
    }
}
```

**问题**:
1. `NUM_OFFSETS`是`#define`宏，编译时替换为`14`
2. `NUM_OFFSETS = count;` 被编译为 `14 = count;` → **编译错误**？

等等，代码能编译通过吗？检查头文件：

```c
// driverWrapper.h
#define NUM_OFFSETS 14
extern int NUM_OFFSETS;  // ← 错误！与#define冲突

// driverWrapper.c
int NUM_OFFSETS = 14;  // 定义了全局变量
```

**真相大白**！
- 代码同时使用了`#define NUM_OFFSETS`和`int NUM_OFFSETS`
- 数组定义时用的是`#define`（14）
- 循环时用的是全局变量（初始值14）
- `UpdateOffsetCount`修改了全局变量为15
- 导致循环访问`g_data_buffer[14]`越界

### 2.3 竞态条件的完整链路

```
Time T0: IOC启动
  - 数组大小: g_data_buffer[14][8]  (#define)
  - 循环上界: NUM_OFFSETS = 14      (全局变量)

Time T1: 某个PV被访问，触发UpdateOffsetCount(15)
  - NUM_OFFSETS = 15

Time T2: AcquireThread下一次循环
  - for (int off = 0; off < 15; off++)  ← 15次
  - 访问g_data_buffer[14][x] ← 数组越界！
  - Segmentation Fault
```

## 3. 修复方案

### 3.1 立即修复（Hotfix）

```c
// driverWrapper.h - 移除歧义
// #define NUM_OFFSETS 14  ← 删除
extern const int NUM_OFFSETS;  // 改为常量

// driverWrapper.c
const int NUM_OFFSETS = 14;  // 常量，不可修改

// 如果需要动态offset，使用不同的变量名
static int g_active_offset_count = 14;

void* AcquireThread(void* arg) {
    while (g_running) {
        int offset_count = g_active_offset_count;  // 本地副本
        for (int ch = 0; ch < MAX_RF_CHANNELS; ch++) {
            for (int off = 0; off < offset_count && off < NUM_OFFSETS; off++) {
                g_data_buffer[off][ch] = BPM_RFIn_ReadADC(ch, off);
            }
        }
        usleep(100000);
    }
}
```

### 3.2 根本解决（重构）

```c
// driverWrapper.h
#define MAX_OFFSETS 20  // 最大支持的offset数量

typedef struct {
    int num_offsets;
    float data[MAX_OFFSETS][MAX_RF_CHANNELS];
    pthread_mutex_t mutex;
} DataBuffer;

// driverWrapper.c
static DataBuffer g_buffer = {
    .num_offsets = 14,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

void* AcquireThread(void* arg) {
    while (g_running) {
        pthread_mutex_lock(&g_buffer.mutex);
        int count = g_buffer.num_offsets;
        pthread_mutex_unlock(&g_buffer.mutex);

        for (int ch = 0; ch < MAX_RF_CHANNELS; ch++) {
            for (int off = 0; off < count; off++) {
                pthread_mutex_lock(&g_buffer.mutex);
                g_buffer.data[off][ch] = BPM_RFIn_ReadADC(ch, off);
                pthread_mutex_unlock(&g_buffer.mutex);
            }
        }
        usleep(100000);
    }
}

void UpdateOffsetCount(int count) {
    if (count > 0 && count <= MAX_OFFSETS) {
        pthread_mutex_lock(&g_buffer.mutex);
        g_buffer.num_offsets = count;
        pthread_mutex_unlock(&g_buffer.mutex);
    }
}
```

### 3.3 添加防御性代码

```c
float ReadData(int offset, int channel, int type) {
    // 边界检查
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        errlogPrintf("ERROR: Invalid channel %d (max %d)\n",
                    channel, MAX_RF_CHANNELS);
        return 0.0;
    }

    if (offset < 0 || offset >= MAX_OFFSETS) {
        errlogPrintf("ERROR: Invalid offset %d (max %d)\n",
                    offset, MAX_OFFSETS);
        return 0.0;
    }

    pthread_mutex_lock(&g_buffer.mutex);
    float value = g_buffer.data[offset][channel];
    pthread_mutex_unlock(&g_buffer.mutex);

    return value;
}
```

## 4. 验证和测试

### 4.1 单元测试

```c
// test/test_boundary.c
#include "unity.h"
#include "driverWrapper.h"

void test_ReadData_boundary(void) {
    // 测试边界条件
    float value;

    // 正常范围
    value = ReadData(0, 0, 0);
    TEST_ASSERT_FLOAT_WITHIN(100.0, 10.0, value);

    value = ReadData(MAX_OFFSETS - 1, MAX_RF_CHANNELS - 1, 0);
    TEST_ASSERT_FLOAT_WITHIN(100.0, 10.0, value);

    // 越界测试
    value = ReadData(MAX_OFFSETS, 0, 0);  // offset越界
    TEST_ASSERT_EQUAL_FLOAT(0.0, value);

    value = ReadData(0, MAX_RF_CHANNELS, 0);  // channel越界
    TEST_ASSERT_EQUAL_FLOAT(0.0, value);

    value = ReadData(-1, 0, 0);  // 负数
    TEST_ASSERT_EQUAL_FLOAT(0.0, value);
}

void test_UpdateOffsetCount_boundary(void) {
    // 正常更新
    UpdateOffsetCount(10);
    TEST_ASSERT_EQUAL(10, GetOffsetCount());

    // 越界拒绝
    UpdateOffsetCount(MAX_OFFSETS + 1);
    TEST_ASSERT_EQUAL(10, GetOffsetCount());  // 未改变

    UpdateOffsetCount(-1);
    TEST_ASSERT_EQUAL(10, GetOffsetCount());  // 未改变
}
```

### 4.2 压力测试

```bash
# 72小时压力测试
./run_ioc.sh &
IOC_PID=$!

# 监控IOC状态
for i in {1..72}; do
    sleep 3600  # 1小时
    if ! kill -0 $IOC_PID 2>/dev/null; then
        echo "ERROR: IOC crashed after $i hours"
        exit 1
    fi

    # 检查PV
    camonitor -c 100 LLRF:BPM:RFIn_01_Amp > /dev/null
    if [ $? -ne 0 ]; then
        echo "ERROR: PV not accessible after $i hours"
        exit 1
    fi

    echo "Hour $i: IOC running OK"
done

echo "SUCCESS: 72-hour test passed"
```

### 4.3 Valgrind验证

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ../../bin/linux-x86_64/BPMmonitor st.cmd

# 运行10分钟后Ctrl+C

# 检查输出
==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 1,234 allocs, 1,234 frees, 456,789 bytes allocated
==12345==
==12345== All heap blocks were freed -- no leaks are possible
==12345==
==12345== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

## 5. 部署和回滚计划

### 5.1 部署步骤

```bash
# 1. 备份当前版本
ssh root@beamline01
cd /opt/BPMmonitor
tar czf BPMmonitor_v1.0_backup_$(date +%Y%m%d).tar.gz .

# 2. 停止IOC
systemctl stop bpmioc

# 3. 部署新版本
scp -r bin/linux-arm root@beamline01:/opt/BPMmonitor/
scp -r db root@beamline01:/opt/BPMmonitor/

# 4. 测试启动
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor
./st.cmd
# 验证PV可访问后Ctrl+C

# 5. 启动服务
systemctl start bpmioc
systemctl status bpmioc

# 6. 监控
camonitor LLRF:BPM:RFIn_*_Amp
```

### 5.2 监控指标

部署后持续监控：
- IOC进程存活（每分钟检查）
- PV可访问性（每10秒测试）
- 内存占用（每小时记录）
- Core dump生成（实时报警）

## 6. 经验教训

### ✅ 做得好的

1. **Core Dump保存**
   - 系统配置了core dump自动保存
   - 提供了关键的崩溃现场信息

2. **系统化排查**
   - GDB → Valgrind → 源码分析
   - 逐步缩小问题范围

3. **添加测试**
   - 边界条件测试
   - 72小时压力测试

### ❌ 教训

1. **代码审查不严**
   - `#define`和全局变量同名未被发现
   - Code Review应检查此类问题

2. **缺少边界检查**
   - 数组访问未检查边界
   - 应该添加断言和防御性代码

3. **测试不充分**
   - 部署前未进行长时间测试
   - 未测试动态配置场景

### 💡 改进措施

1. **静态分析**
   ```bash
   # 使用cppcheck检测
   cppcheck --enable=all --inconclusive src/
   # 会检测到变量重定义
   ```

2. **运行时检查**
   ```c
   #ifdef DEBUG
   #define ASSERT_RANGE(val, min, max) \
       assert((val) >= (min) && (val) < (max))
   #else
   #define ASSERT_RANGE(val, min, max)
   #endif

   g_buffer.data[off][ch] = value;
   ASSERT_RANGE(off, 0, MAX_OFFSETS);
   ASSERT_RANGE(ch, 0, MAX_RF_CHANNELS);
   ```

3. **持续监控**
   ```python
   # 监控脚本
   import epics
   import time

   pv = epics.PV('LLRF:BPM:Heartbeat')
   while True:
       if not pv.connected:
           send_alert("IOC disconnected!")
       time.sleep(10)
   ```

## 7. 总结

### 问题根因

`#define`宏和同名全局变量并存，导致：
- 数组定义使用宏值（14）
- 循环使用变量值（动态修改为15）
- 数组越界访问导致段错误

### 修复效果

| 指标 | 修复前 | 修复后 |
|------|--------|--------|
| 运行时间 | 24-72h崩溃 | >720h无崩溃 ✅ |
| 内存错误 | Valgrind报错 | 0错误 ✅ |
| 边界保护 | 无 | 完整检查 ✅ |

### 关键经验

1. **善用工具**: GDB + Valgrind + Core Dump
2. **系统化排查**: 不要猜测，用数据说话
3. **防御性编程**: 边界检查、断言、错误处理
4. **充分测试**: 单元测试 + 集成测试 + 压力测试

## 🔗 相关资源

- [Part 10: GDB调试](../part10-debugging-testing/01-gdb-debugging.md)
- [Part 10: 性能工具](../part10-debugging-testing/03-performance-tools.md)
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
- [Core Dump分析指南](https://linux-audit.com/understand-and-configure-core-dumps-work-on-linux/)
