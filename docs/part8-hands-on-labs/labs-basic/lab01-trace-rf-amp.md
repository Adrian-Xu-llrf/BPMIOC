# 实验1: 追踪RF振幅数据流

> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 1-2小时
> **前置条件**: 完成Part 1，BPMIOC能运行

## 🎯 实验目标

通过本实验，你将：
- ✅ 理解RF振幅从硬件到PV的完整数据流
- ✅ 学会使用调试输出追踪代码执行
- ✅ 掌握数据流的每个环节
- ✅ 能够绘制完整的数据流图

## 📋 实验准备

### 前置知识
- [ ] 理解PV、Record、IOC概念 (Part 2)
- [ ] 理解三层架构 (Part 1/01-introduction)
- [ ] 能使用caget/camonitor

### 工具准备
- [ ] BPMIOC已编译并能运行
- [ ] 文本编辑器 (vim/nano/vscode)
- [ ] 两个终端窗口

## 📝 实验步骤

### 步骤1: 确定目标PV (5分钟)

我们要追踪的PV: `iLinac_007:BPM14And15:RF3Amp`

**先测试PV是否可访问**:
```bash
# 终端1: 启动IOC
cd ~/BPMIOC/iocBoot/iocBPMmonitor
./st.cmd

# 终端2: 测试PV
caget iLinac_007:BPM14And15:RF3Amp
```

预期输出:
```
iLinac_007:BPM14And15:RF3Amp    1.234567
```

**记录当前值**: _________

### 步骤2: 在数据库中查找PV定义 (10分钟)

```bash
cd ~/BPMIOC/BPMmonitorApp/Db
grep "RF3Amp" BPMMonitor.db
```

你会找到类似这样的记录:
```
record(ai, "$(P):RF3Amp") {
    field(DTYP, "BPMMonitor")
    field(INP,  "@AMP:0 ch=0")
    field(SCAN, "I/O Intr")
    field(PREC, "3")
    field(EGU,  "V")
    field(DESC, "RF3 Amplitude")
}
```

**分析记录**:
- **记录类型**: ai (Analog Input)
- **设备类型**: BPMMonitor
- **INP字段**: `@AMP:0 ch=0`
  - TYPE: `AMP`
  - offset: `0`
  - channel: `0`
- **SCAN**: I/O Intr (I/O中断扫描)

**问题1**: 这个记录的EGU字段是什么含义？
<details>
<summary>答案</summary>
EGU = Engineering Units，工程单位，这里是"V"（伏特）
</details>

**问题2**: SCAN字段为"I/O Intr"是什么意思？
<details>
<summary>答案</summary>
I/O中断扫描，当驱动层调用scanIoRequest时，这个记录会被处理
</details>

### 步骤3: 在设备支持层添加调试输出 (15分钟)

编辑设备支持层代码:
```bash
cd ~/BPMIOC/BPMmonitorApp/src
cp devBPMMonitor.c devBPMMonitor.c.bak  # 备份
vim devBPMMonitor.c
```

**3.1 在 `init_ai_record()` 函数中添加调试**

找到 `static long init_ai_record(struct aiRecord *prec)` 函数，在解析参数后添加:

```c
static long init_ai_record(struct aiRecord *prec)
{
    // ... 原有代码 ...

    // 仅为RF3Amp添加调试输出
    if (strcmp(prec->name, "iLinac_007:BPM14And15:RF3Amp") == 0) {
        printf("\n=== [DEVICE SUPPORT] init_ai_record ===\n");
        printf("  Record Name: %s\n", prec->name);
        printf("  INP String:  %s\n", pinstio->string);
        printf("  Parsed -> type: %s, offset: %d, channel: %d\n",
               pPvt->type_str, pPvt->offset, pPvt->channel);
        printf("=======================================\n\n");
    }

    return 0;
}
```

**3.2 在 `read_ai()` 函数中添加调试**

找到 `static long read_ai(struct aiRecord *prec)` 函数:

```c
static long read_ai(struct aiRecord *prec)
{
    BPMMonitorPvt *pPvt = (BPMMonitorPvt *)prec->dpvt;

    // 调用驱动层读取
    double value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 仅为RF3Amp添加调试（且每10次打印一次，避免刷屏）
    static int read_count = 0;
    if (strcmp(prec->name, "iLinac_007:BPM14And15:RF3Amp") == 0) {
        read_count++;
        if (read_count % 10 == 0) {
            printf("[DEVICE SUPPORT] read_ai: %s = %.6f V\n",
                   prec->name, value);
        }
    }

    prec->val = value;
    prec->udf = FALSE;

    return 2;
}
```

### 步骤4: 在驱动层添加调试输出 (15分钟)

编辑驱动层代码:
```bash
cp driverWrapper.c driverWrapper.c.bak  # 备份
vim driverWrapper.c
```

**4.1 在 `ReadData()` 函数中添加调试**

找到 `double ReadData(int offset, int channel, int type)` 函数:

```c
double ReadData(int offset, int channel, int type)
{
    double value = 0.0;
    static int read_count = 0;

    switch(offset) {
        case OFFSET_AMP:  // offset = 0
            if (channel >= 0 && channel < 8) {
                value = Amp[channel];

                // 仅为channel 0 (RF3) 添加调试
                if (channel == 0) {
                    read_count++;
                    if (read_count % 10 == 0) {
                        printf("[DRIVER] ReadData(OFFSET_AMP, ch=%d): Amp[0]=%.6f\n",
                               channel, value);
                    }
                }
            }
            break;

        // ... 其他case保持不变 ...
    }

    return value;
}
```

**4.2 在 `my_thread()` 函数中添加调试**

找到 `static void my_thread(void *arg)` 函数，在更新Amp后添加:

```c
static void my_thread(void *arg)
{
    static double sim_time = 0.0;
    static int loop_count = 0;

    while (1) {
        if (use_simulation) {
            // 模拟数据生成
            for (int i = 0; i < 8; i++) {
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);
            }

            // 每10次循环打印一次 (channel 0)
            loop_count++;
            if (loop_count % 10 == 0) {
                printf("[DRIVER] my_thread: Amp[0]=%.6f (simulated)\n", Amp[0]);
            }

            sim_time += 0.1;
        } else {
            // ... 真实硬件代码 ...
        }

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.1);
    }
}
```

### 步骤5: 重新编译 (5分钟)

```bash
cd ~/BPMIOC
make clean
make -j$(nproc)
```

验证编译成功:
```bash
echo $?  # 应该输出 0
```

### 步骤6: 运行并观察输出 (10分钟)

**终端1: 启动IOC**
```bash
cd iocBoot/iocBPMmonitor
./st.cmd
```

**观察输出**，你应该看到：

```
=== BPM Monitor Driver Initialization ===
WARNING: Using SIMULATION mode
Starting iocInit
...
=== [DEVICE SUPPORT] init_ai_record ===
  Record Name: iLinac_007:BPM14And15:RF3Amp
  INP String:  @AMP:0 ch=0
  Parsed -> type: AMP, offset: 0, channel: 0
=======================================
...
iocRun: All initialization complete
epics>
[DRIVER] my_thread: Amp[0]=1.234567 (simulated)
[DRIVER] ReadData(OFFSET_AMP, ch=0): Amp[0]=1.234567
[DEVICE SUPPORT] read_ai: iLinac_007:BPM14And15:RF3Amp = 1.234567 V
...
```

**终端2: 监控PV**
```bash
camonitor iLinac_007:BPM14And15:RF3Amp
```

观察值的变化是否与调试输出一致。

### 步骤7: 绘制数据流图 (15分钟)

根据观察到的输出，在纸上或电脑上绘制完整的数据流图：

```
┌─────────────────────────────────────────────────┐
│  模拟硬件 (my_thread 后台线程)                   │
│  [DRIVER] my_thread: Amp[0]=1.234567           │
├─────────────────────────────────────────────────┤
│  每100ms:                                       │
│  1. 生成模拟数据: Amp[i] = 1.0 + 0.5*sin(t)    │
│  2. 存入全局缓冲: Amp[0] = 1.234567            │
│  3. 触发I/O中断: scanIoRequest(ioScanPvt)      │
└──────────────┬──────────────────────────────────┘
               │ scanIoRequest触发
               ▼
┌─────────────────────────────────────────────────┐
│  EPICS 扫描器                                   │
│  识别到ioScanPvt信号                            │
│  查找所有SCAN="I/O Intr"的记录                  │
└──────────────┬──────────────────────────────────┘
               │ 触发记录处理
               ▼
┌─────────────────────────────────────────────────┐
│  设备支持层 (devBPMMonitor.c)                   │
│  read_ai(prec)                                  │
├─────────────────────────────────────────────────┤
│  1. 从prec->dpvt获取私有数据:                   │
│     type="AMP", offset=0, channel=0            │
│  2. 调用驱动层:                                 │
│     value = ReadData(0, 0, TYPE_AMP)           │
│  3. 更新记录值:                                 │
│     prec->val = 1.234567                       │
│  [DEVICE SUPPORT] read_ai: ...RF3Amp = 1.234567│
└──────────────┬──────────────────────────────────┘
               │ 调用ReadData()
               ▼
┌─────────────────────────────────────────────────┐
│  驱动层 (driverWrapper.c)                       │
│  ReadData(offset=0, channel=0, type=TYPE_AMP)  │
├─────────────────────────────────────────────────┤
│  switch(offset) {                               │
│    case OFFSET_AMP:                             │
│      value = Amp[0];  // 读取全局缓冲          │
│  }                                              │
│  [DRIVER] ReadData: Amp[0]=1.234567            │
│  return 1.234567;                               │
└──────────────┬──────────────────────────────────┘
               │ 返回值
               ▼
┌─────────────────────────────────────────────────┐
│  Record处理完成                                 │
│  prec->val = 1.234567                          │
└──────────────┬──────────────────────────────────┘
               │ Channel Access 通知
               ▼
┌─────────────────────────────────────────────────┐
│  网络客户端                                     │
│  camonitor 收到值更新通知                       │
│  显示: iLinac_007:BPM14And15:RF3Amp  1.234567  │
└─────────────────────────────────────────────────┘
```

### 步骤8: 验证理解 (10分钟)

回答以下问题以验证你的理解：

**Q1**: RF3Amp的值从哪里来？
<details>
<summary>答案</summary>
从全局缓冲区Amp[0]，由my_thread()后台线程每100ms更新一次
</details>

**Q2**: 为什么使用I/O中断扫描而不是定时扫描？
<details>
<summary>答案</summary>
I/O中断更高效，数据更新时立即触发，避免轮询浪费CPU
</details>

**Q3**: scanIoRequest()的作用是什么？
<details>
<summary>答案</summary>
通知EPICS扫描器，所有使用该ioScanPvt的"I/O Intr"记录需要被处理
</details>

**Q4**: 数据流经过了几层？
<details>
<summary>答案</summary>
3层：驱动层(driverWrapper.c) → 设备支持层(devBPMMonitor.c) → 数据库层(BPMMonitor.db)
</details>

**Q5**: 如果后台线程停止，会发生什么？
<details>
<summary>答案</summary>
PV值不再更新，因为没有scanIoRequest触发，记录不会被处理
</details>

## ✅ 验证方法

### 成功标准
- ✅ 能看到完整的调试输出链
- ✅ 调试输出的值与camonitor显示一致
- ✅ 能绘制出完整的数据流图
- ✅ 能回答上面的5个问题

### 输出示例

正确的输出应该是这样的循环：
```
[DRIVER] my_thread: Amp[0]=1.234567 (simulated)
[DRIVER] ReadData(OFFSET_AMP, ch=0): Amp[0]=1.234567
[DEVICE SUPPORT] read_ai: iLinac_007:BPM14And15:RF3Amp = 1.234567 V
(camonitor显示: 1.234567)

(1秒后...)

[DRIVER] my_thread: Amp[0]=1.345678 (simulated)
[DRIVER] ReadData(OFFSET_AMP, ch=0): Amp[0]=1.345678
[DEVICE SUPPORT] read_ai: iLinac_007:BPM14And15:RF3Amp = 1.345678 V
(camonitor显示: 1.345678)
```

## 💡 深入思考

### 思考题1: 性能优化
**Q**: 为什么不直接在my_thread()中更新PV，而要经过这么多层？

**A**: 分层设计的好处：
- **解耦**: 驱动层不依赖EPICS数据库
- **复用**: 同一驱动可以被多个Record使用
- **灵活**: 可以轻松更换硬件或Record类型
- **标准**: 遵循EPICS设计模式

### 思考题2: 数据一致性
**Q**: Amp[]是全局变量，会不会有竞争条件？

**A**: 在当前实现中：
- 写线程: my_thread()
- 读线程: EPICS扫描线程
- 潜在问题: 读写可能同时发生

**改进**: 使用epicsMutex加锁（高级主题）

### 思考题3: 扫描周期
**Q**: 后台线程100ms扫描一次，PV实际更新频率是多少？

**A**: 取决于：
- 后台线程周期: 100ms
- 数据变化频率: 每次都变
- I/O中断触发: 每100ms一次
- **实际频率**: 10 Hz (每秒10次)

## 🔬 扩展练习

### 练习1: 追踪其他PV
重复步骤2-7，追踪：
- `RF3Phase` (相位)
- `RF3Power` (功率)

观察它们的数据流有什么不同？

### 练习2: 修改扫描周期
修改my_thread()的休眠时间从0.1秒改为1秒：
```c
epicsThreadSleep(1.0);  // 原来是0.1
```

观察PV更新频率的变化。

### 练习3: 添加数据处理
在ReadData()中添加一个简单的滤波：
```c
case OFFSET_AMP:
    static double last_value = 0;
    double raw_value = Amp[channel];
    value = 0.7 * last_value + 0.3 * raw_value;  // 低通滤波
    last_value = value;
    break;
```

观察PV值的变化是否更平滑。

## 📝 实验报告模板

```markdown
# 实验1: 追踪RF振幅数据流 - 实验报告

## 基本信息
- 姓名：
- 日期：
- 用时：

## 实验结果
### 1. 数据流图
[插入你绘制的数据流图]

### 2. 调试输出
```
[粘贴关键的调试输出]
```

### 3. 问题回答
Q1: ...
A1: ...

Q2: ...
A2: ...

[依次回答所有问题]

## 实验心得
1. 我学到了：
2. 困难之处：
3. 解决方法：
4. 疑问：

## 扩展练习
[记录你完成的扩展练习]
```

## 🔗 相关资源

- [Part 3: 数据流分析](../../part3-bpmioc-architecture/04-data-flow.md)
- [Part 4: 驱动层详解](../../part4-driver-layer/)
- [Part 5: 设备支持层](../../part5-device-support-layer/)

---

**恭喜完成实验1！** 🎉

这是理解BPMIOC最重要的实验之一。完成这个实验后，你已经掌握了：
- ✅ 完整的数据流
- ✅ 三层架构的交互
- ✅ I/O中断机制
- ✅ 调试方法

**下一步**: 尝试 [lab02-modify-scan.md](./lab02-modify-scan.md) 修改扫描周期！
