# Lab 11: I/O中断扫描实现

> **难度**: ⭐⭐⭐⭐
> **时间**: 4小时
> **前置**: Lab 1-10, Part 6.4

## 🎯 实验目标

实现I/O中断扫描机制，实现硬件事件驱动的低延迟数据采集。

---

## 📚 背景知识

### I/O中断 vs. 周期扫描

```
周期扫描（.5 second）:
  - 每0.5秒扫描一次
  - 延迟：0-500ms
  - CPU占用高

I/O中断扫描：
  - 硬件触发时立即扫描
  - 延迟：<1ms
  - CPU占用低（按需处理）
```

---

## 🔧 实验任务

### 任务: 实现触发信号的I/O中断扫描

---

### 步骤1: 驱动层创建IOSCANPVT

**在driverWrapper.c中**:

```c
#include <scanIo.h>

// 全局I/O scan结构
IOSCANPVT TriginScanPvt;

// 初始化
int drvBPMmonitorInit(void)
{
    // 初始化scan结构
    scanIoInit(&TriginScanPvt);

    // 创建触发监控线程
    pthread_t tid;
    pthread_create(&tid, NULL, BPM_Trigin_thread, NULL);

    return 0;
}

// 触发监控线程
void *BPM_Trigin_thread(void *arg)
{
    while (1) {
        // 等待硬件触发（模拟：每1秒触发一次）
        sleep(1);

        // 触发I/O中断扫描！
        scanIoRequest(TriginScanPvt);

        if (devBPMmonitorDebug > 0) {
            printf("BPM_Trigin_thread: Trigger event!\n");
        }
    }

    return NULL;
}
```

---

### 步骤2: 设备支持层连接IOSCANPVT

**在devBPMMonitor.c的get_ioint_info中**:

```c
extern IOSCANPVT TriginScanPvt;  // 声明外部变量

static long get_ioint_info(int cmd, aiRecord *prec, IOSCANPVT *ppvt)
{
    *ppvt = TriginScanPvt;  // 返回driver的IOSCANPVT
    return 0;
}
```

---

### 步骤3: .db文件使用I/O Intr扫描

```
record(ai, "$(P):RFIn_Trig_Amp")
{
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(SCAN, "I/O Intr")  ← I/O中断扫描
    field(PREC, "3")
}
```

---

### 步骤4: 测试I/O中断

**监控PV**:
```bash
camonitor -t iLinac_007:BPM14:RFIn_Trig_Amp

# 输出（每秒更新一次，由硬件触发）:
2024-11-09 12:30:01.123 iLinac_007:BPM14:RFIn_Trig_Amp 5.234
2024-11-09 12:30:02.125 iLinac_007:BPM14:RFIn_Trig_Amp 5.231
2024-11-09 12:30:03.124 iLinac_007:BPM14:RFIn_Trig_Amp 5.240
```

**对比周期扫描**:
```
周期扫描（.5s）: 更新可能在0-500ms之间的任意时刻
I/O中断扫描: 更新在触发后<1ms内
```

---

## 🔍 深入理解

### scanIoRequest流程

```
硬件触发
  ↓
BPM_Trigin_thread检测到
  ↓
scanIoRequest(TriginScanPvt)
  ↓
EPICS唤醒I/O中断扫描线程
  ↓
处理所有注册到TriginScanPvt的Record
  ├─ Record 1: read_ai()
  ├─ Record 2: read_ai()
  └─ Record 3: read_ai()
```

---

## 🚀 扩展挑战

### 挑战1: 多个IOSCANPVT

支持不同的触发源：

```c
IOSCANPVT Trigger1ScanPvt;  // 触发源1
IOSCANPVT Trigger2ScanPvt;  // 触发源2

// Record可以选择不同的触发源
// 通过INP字段区分
```

### 挑战2: 真实GPIO触发

使用GPIO中断替代sleep(1)模拟。

---

## ✅ 学习检查点

- [ ] 理解scanIoInit和scanIoRequest
- [ ] 能够创建触发监控线程
- [ ] 理解get_ioint_info的作用
- [ ] 能够配置I/O Intr扫描

---

**恭喜完成Lab 11！** 你已掌握I/O中断机制！💪
