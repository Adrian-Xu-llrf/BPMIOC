# Week 5: 设备支持层（Device Support Layer）

> **学习目标**: 深入理解devBPMMonitor.c，掌握DSET机制
> **关键词**: DSET、init_record、read/write、DevPvt
> **难度**: ⭐⭐⭐⭐⭐
> **时间**: 15-20小时

## 📅 本周概览

```
周一-周三: 理论学习
  ├─ Part 5文档阅读
  ├─ devBPMMonitor.c代码分析
  └─ DSET机制理解

周四-周五: 动手实践
  ├─ Lab 8: 添加新的Record类型
  ├─ 修改init_record逻辑
  └─ 调试设备支持层

周末: 复习巩固
  ├─ 整理DSET笔记
  ├─ 绘制设备支持流程图
  └─ 规划Week 6
```

---

## 🎯 Week 5学习目标

### 知识目标
- [ ] 理解设备支持层在三层架构中的作用
- [ ] 掌握DSET（Device Support Entry Table）结构
- [ ] 理解init_record、read、write函数的实现
- [ ] 理解DevPvt私有数据结构的作用
- [ ] 理解INP/OUT字段的解析机制

### 技能目标
- [ ] 能读懂devBPMMonitor.c的全部代码
- [ ] 能追踪Record到驱动层的完整调用链
- [ ] 能添加新的Record类型支持
- [ ] 能修改init_record逻辑
- [ ] 能调试设备支持层的问题

---

## 📚 Day-by-Day Schedule

### Day 1 (Monday) - DSET概述

**学习内容** (2-3小时):
- ✅ Part 5: 01-overview.md - 设备支持层概述
- ✅ Part 5: 02-dset-structure.md - DSET结构详解
- ✅ EPICS官方文档: Device Support Guide

**实践任务**:
```bash
# 查看devBPMMonitor.c中的DSET定义
cd ~/BPMIOC/BPMmonitorApp/src
grep -A 10 "devAi.*=" devBPMMonitor.c
grep -A 10 "devAo.*=" devBPMMonitor.c
```

**理解要点**:
```c
// DSET结构
typedef struct {
    long      number;           // 函数数量
    DEVSUPFUN report;           // 报告函数
    DEVSUPFUN init;             // 全局初始化
    DEVSUPFUN init_record;      // Record初始化 ⭐核心
    DEVSUPFUN get_ioint_info;   // I/O中断信息 ⭐核心
    DEVSUPFUN read;             // 读取数据 ⭐核心
    DEVSUPFUN special_linconv;  // 线性转换
} devAi;
```

**思考问题**:
1. 为什么需要设备支持层？直接在数据库调用驱动层不行吗？
2. DSET中哪些函数是必须实现的？
3. init_record和read分别在什么时候被调用？

---

### Day 2 (Tuesday) - init_record实现

**学习内容** (2-3小时):
- ✅ Part 5: 03-init-record.md - init_record详解
- ✅ 阅读devBPMMonitor.c中的init_record_ai()函数

**代码分析**:
```c
// init_record_ai() 的主要工作
static long init_record_ai(aiRecord *prec)
{
    // 1. 分配DevPvt内存
    DevPvt *pPvt = (DevPvt *)malloc(sizeof(DevPvt));

    // 2. 解析INP字段 "@offset channel type"
    sscanf(prec->inp.value.instio.string, "@%d %d %d",
           &pPvt->offset, &pPvt->channel, &pPvt->type);

    // 3. 注册I/O中断
    scanIoInit(&pPvt->ioscanpvt);

    // 4. 保存到Record私有指针
    prec->dpvt = pPvt;

    return 0;
}
```

**实践任务**:
1. 在init_record_ai中添加printf，观察初始化顺序
2. 打印出每个Record的offset/channel/type
3. 理解DevPvt结构的作用

**思考问题**:
1. 为什么需要DevPvt结构？
2. scanIoInit的作用是什么？
3. INP字段"@0 3 0"表示什么意思？

---

### Day 3 (Wednesday) - read/write实现

**学习内容** (2-3小时):
- ✅ Part 5: 04-read-write.md - read/write函数详解
- ✅ 阅读read_ai()和write_ao()函数

**代码分析**:
```c
// read_ai() - 读取数据
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 调用驱动层ReadData()
    float value = ReadData(pPvt->offset,
                          pPvt->channel,
                          pPvt->type);

    // 更新Record值
    prec->val = value;
    prec->udf = 0;

    return 0;
}

// write_ao() - 写入数据
static long write_ao(aoRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 调用驱动层SetReg()
    SetReg(pPvt->channel, (int)prec->val);

    return 0;
}
```

**实践任务**:
1. 追踪一个ai Record的读取流程
2. 追踪一个ao Record的写入流程
3. 理解如何从Record获取DevPvt

---

### Day 4 (Thursday) - Lab 8实验

**实验**: 添加新的Record类型支持

**目标**: 添加对stringin Record的支持

**步骤**:

1. 在devBPMMonitor.c中定义stringin DSET:
```c
typedef struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
} devSi;

static devSi devSiMonitor = {
    6,
    NULL,
    NULL,
    init_record_si,
    get_ioint_info,
    read_si
};
epicsExportAddress(dset, devSiMonitor);
```

2. 实现init_record_si():
```c
static long init_record_si(stringinRecord *prec)
{
    // 类似init_record_ai
    DevPvt *pPvt = (DevPvt *)malloc(sizeof(DevPvt));
    sscanf(prec->inp.value.instio.string, "@%d %d %d",
           &pPvt->offset, &pPvt->channel, &pPvt->type);
    prec->dpvt = pPvt;
    return 0;
}
```

3. 实现read_si():
```c
static long read_si(stringinRecord *prec)
{
    // 读取系统状态信息
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 假设读取系统状态字符串
    const char *status = "System OK";
    strncpy(prec->val, status, sizeof(prec->val));
    prec->udf = 0;

    return 0;
}
```

4. 在数据库中添加record:
```
record(stringin, "BPM:01:System:Status") {
    field(DTYP, "BPM Monitor")
    field(INP,  "@0 0 0")
    field(SCAN, "1 second")
}
```

5. 编译测试:
```bash
make
cd ~/BPMIOC/iocBoot/iocBPMMonitor
./st.cmd
caget BPM:01:System:Status
```

---

### Day 5 (Friday) - 深入理解I/O中断

**学习内容** (2-3小时):
- ✅ Part 5: 05-devpvt.md - DevPvt和I/O中断

**理解要点**:
```c
// get_ioint_info - 关键！
static long get_ioint_info(int cmd, aiRecord *prec,
                          IOSCANPVT *ppvt)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;

    // 返回这个Record的ioscanpvt
    *ppvt = pPvt->ioscanpvt;

    return 0;
}
```

**I/O中断流程**:
```
pthread线程 (driverWrapper.c)
    ↓
TriggerAllDataReached()
    ↓
scanIoRequest(TriginScanPvt)  ← 触发I/O中断
    ↓
EPICS扫描线程被唤醒
    ↓
处理所有SCAN="I/O Intr"的Record
    ↓
调用read_ai()读取新数据
```

**实践任务**:
1. 在scanIoRequest前后添加时间戳
2. 测量从触发到Record更新的延迟
3. 理解为什么I/O中断比周期扫描高效

---

### Weekend - 复习和总结

**Saturday** (3-4小时):

1. **复习本周内容**:
   - 回顾Part 5所有文档
   - 整理DSET相关笔记

2. **绘制流程图**:
```
Record初始化流程:
iocInit()
  → dbInitRecords()
    → init_record_ai()
      → malloc DevPvt
      → 解析INP字段
      → scanIoInit()

Record读取流程:
scanIoRequest()
  → 扫描线程唤醒
    → process(aiRecord)
      → read_ai()
        → ReadData()
          → GetRFInfo()
```

3. **完成Lab 8的扩展**:
   - 添加longout Record支持（寄存器写入）
   - 添加waveform Record支持

**Sunday** (2-3小时):

1. **Week 5检查点测试**:
```c
// 能回答以下问题吗？
Q1: DSET是什么？有什么作用？
Q2: init_record什么时候被调用？做什么工作？
Q3: DevPvt结构存储什么信息？
Q4: I/O中断扫描的触发流程是什么？
Q5: 如何添加新的Record类型支持？
```

2. **规划Week 6**:
   - 预习Part 6: 数据库层
   - 准备.db文件学习环境

---

## ✅ Week 5检查点

完成Week 5后，你应该能够：

**理论理解**:
- [ ] 完全理解DSET结构和作用
- [ ] 理解init_record/read/write的调用时机
- [ ] 理解I/O中断扫描机制
- [ ] 理解DevPvt私有数据的作用

**代码能力**:
- [ ] 能读懂devBPMMonitor.c的全部代码
- [ ] 能追踪Record→设备支持→驱动层的调用链
- [ ] 能添加新的Record类型支持
- [ ] 能修改init_record逻辑

**调试能力**:
- [ ] 知道在哪里添加调试信息
- [ ] 能判断问题在设备支持层还是驱动层
- [ ] 能使用GDB调试设备支持代码

---

## 📝 本周学习笔记模板

```markdown
# Week 5学习笔记

## DSET结构
- 作用：
- 关键函数：
  - init_record:
  - read:
  - get_ioint_info:

## DevPvt结构
```c
typedef struct {
    int offset;        // 用途：
    int channel;       // 用途：
    int type;          // 用途：
    IOSCANPVT ioscanpvt;  // 用途：
} DevPvt;
```

## Record处理流程
[画流程图]

## Lab 8收获
- 添加了什么Record类型：
- 遇到的问题：
- 解决方案：
- 学到的经验：

## 疑问和思考
1.
2.
```

---

## 🔗 相关资源

### 文档
- [Part 5: 设备支持层](../part5-device-support-layer/)
- [Part 4: 驱动层](../part4-driver-layer/)
- [Part 8: Lab 8](../part8-hands-on-labs/)

### 源码
- `BPMmonitorApp/src/devBPMMonitor.c` - 主文件
- `BPMmonitorApp/Db/BPMMonitor.db` - 数据库配置

### EPICS文档
- [Device Support Guide](https://epics.anl.gov/base/R3-15/6-docs/DeviceSupport.html)
- [Record Reference Manual](https://epics.anl.gov/base/R3-15/6-docs/RecordReference.html)

---

## 🎯 下一步

完成Week 5后，进入Week 6学习数据库层！

👉 [07-week6.md](./07-week6.md) - Week 6: 数据库进阶

---

**Week 5加油！** 设备支持层是IOC的核心桥梁，掌握它会让你对EPICS的理解更上一层楼！ 💪
