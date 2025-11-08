# 02 - 完整数据流分析

> **目标**: 深入追踪数据在BPMIOC三层架构中的完整流动
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 90分钟
> **前置知识**: 01-architecture-overview.md

## 📋 学习目标

完成本节后，你将能够：
- ✅ 追踪任何数据从硬件到客户端的完整路径
- ✅ 理解I/O中断和周期扫描的数据流差异
- ✅ 掌握读操作和写操作的不同流程
- ✅ 理解标量数据和波形数据的处理方式
- ✅ 能够定位数据流中的任何节点进行调试

## 🌊 1. 数据流概览

### 数据流的两个方向

```
读取方向（Read Path）:
硬件 → 驱动层 → 设备支持层 → 数据库层 → Channel Access → 客户端

写入方向（Write Path）:
客户端 → Channel Access → 数据库层 → 设备支持层 → 驱动层 → 硬件
```

### BPMIOC的数据类型

| 数据类型 | 示例PV | 记录类型 | 方向 | 更新方式 |
|---------|--------|---------|------|---------|
| 标量读取 | RF3Amp | ai | 读 | I/O Intr |
| 标量写入 | SetPhaseOffset | ao | 写 | Passive |
| 波形读取 | TrigWaveform | waveform | 读 | I/O Intr |
| 计算结果 | RF3Power | calc | 读 | 链接驱动 |
| 状态读取 | DeviceStatus | longin | 读 | Periodic |

## 🔍 2. 标量读取数据流（I/O Interrupt）

### 完整流程图

```
[硬件] BPM板卡
   ↓
   VME总线
   ↓
[硬件库] libBPMboard14And15.so::GetRfInfo()
   ↓ 返回8个通道的Amp/Phase/Power
   ↓
[驱动层] DataAcquisitionThread (pthread)
   ├→ 接收数据: Amp[0]=3.14, Phase[0]=45.6, ...
   ├→ 更新全局数组: Amp[], Phase[], Power[], ...
   └→ 触发扫描: scanIoRequest(ioScanPvt)
   ↓
[EPICS] I/O中断扫描线程被唤醒
   ↓
   对所有 SCAN="I/O Intr" 的记录:
   ↓
[数据库层] aiRecord: iLinac_007:BPM14And15:RFIn_01_Amp
   ├→ EPICS调用: devAiBPMmonitor::read_ai(pRec)
   ↓
[设备支持层] devBPMMonitor.c::read_ai()
   ├→ 取出DevPvt: offset=0, channel=0
   ├→ 调用驱动: ReadValue(0, 0, &value)
   ↓
[驱动层] driverWrapper.c::ReadValue()
   ├→ 识别offset=0 → AMP类型
   ├→ 读取全局数组: value = Amp[0] = 3.14
   ├→ 返回: status=0 (成功)
   ↓
[设备支持层] read_ai()继续
   ├→ 设置记录值: pRec->val = 3.14
   ├→ 返回: 0 (成功)
   ↓
[数据库层] EPICS记录处理
   ├→ 应用ASLO/AOFF转换 (如果有)
   ├→ 应用SMOO平滑 (如果有)
   ├→ 检查报警条件: val vs HIGH/HIHI
   ├→ 更新时间戳: epicsTimeStamp
   ├→ 处理FLNK链接 (如果有)
   └→ 通知CA客户端: 值已更新
   ↓
[Channel Access] CA服务器
   ├→ 广播更新: UDP通知所有订阅者
   ↓
[客户端] camonitor显示
   3.14
```

### 详细代码追踪

#### 阶段1: 硬件数据采集

**文件**: `driverWrapper.c`

```c
void *DataAcquisitionThread(void *arg)
{
    // 局部变量用于接收硬件数据
    float localAmp[8], localPhase[8], localPower[8];
    float localVBPM[8], localIBPM[8];
    float localVCRI[8], localICRI[8];
    int   localPSET[8];

    while (1) {
        // 1. 调用硬件库获取所有RF通道数据
        //    这是一个阻塞调用，等待硬件准备好
        int status = funcGetRfInfo(
            localAmp,    // 8个通道的幅度
            localPhase,  // 8个通道的相位
            localPower,  // 8个通道的功率
            localVBPM,   // BPM电压
            localIBPM,   // BPM电流
            localVCRI,   // CRI电压
            localICRI,   // CRI电流
            localPSET    // 相位设置
        );

        if (status != 0) {
            printf("GetRfInfo failed: %d\n", status);
            epicsThreadSleep(0.1);
            continue;
        }

        // 2. 复制到全局数组（线程安全？后续讨论）
        memcpy(Amp, localAmp, 8 * sizeof(float));
        memcpy(Phase, localPhase, 8 * sizeof(float));
        memcpy(Power, localPower, 8 * sizeof(float));
        memcpy(VBPM, localVBPM, 8 * sizeof(float));
        memcpy(IBPM, localIBPM, 8 * sizeof(float));
        memcpy(VCRI, localVCRI, 8 * sizeof(float));
        memcpy(ICRI, localICRI, 8 * sizeof(float));
        memcpy(PSET, localPSET, 8 * sizeof(int));

        // 3. 触发I/O中断扫描
        //    这会唤醒EPICS的扫描线程
        scanIoRequest(ioScanPvt);

        // 4. 延时100ms后下一次采集
        epicsThreadSleep(0.1);  // 10 Hz采集率
    }

    return NULL;
}
```

**关键点**:
- `funcGetRfInfo`: 函数指针，指向硬件库函数
- 全局数组: `Amp[8]`, `Phase[8]`等，所有设备支持层共享
- `scanIoRequest()`: EPICS函数，触发所有注册的I/O中断记录

#### 阶段2: EPICS I/O中断扫描

当`scanIoRequest(ioScanPvt)`被调用时，EPICS会：

```c
// EPICS内部（简化）
void scanIoRequest(IOSCANPVT pvt)
{
    // 1. 标记pvt关联的所有记录需要处理
    mark_records_for_processing(pvt);

    // 2. 唤醒I/O中断扫描线程
    wake_up_scan_thread(pvt);
}

// I/O中断扫描线程
void io_interrupt_scan_thread(void *arg)
{
    while (1) {
        // 等待scanIoRequest()唤醒
        wait_for_scan_request();

        // 处理所有标记的记录
        for (each marked record) {
            process_record(record);
        }
    }
}
```

#### 阶段3: 记录处理

**EPICS调用**:
```c
// EPICS内部
void process_record(dbCommon *precord)
{
    if (precord->rtype == ai) {
        aiRecord *pai = (aiRecord *)precord;

        // 调用设备支持层的read函数
        long status = pai->dset->read_ai(pai);

        if (status == 0) {
            // 读取成功，继续处理
            apply_conversions(pai);
            check_alarms(pai);
            update_timestamp(pai);
            process_forward_link(pai);
            notify_ca_clients(pai);
        }
    }
}
```

#### 阶段4: 设备支持层读取

**文件**: `devBPMMonitor.c`

```c
static long read_ai(aiRecord *pRec)
{
    // 1. 获取私有数据（init_ai_record时保存的）
    DevPvt *pPvt = (DevPvt *)pRec->dpvt;

    if (pPvt == NULL) {
        recGblSetSevr(pRec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 2. 准备接收缓冲区
    double value = 0.0;

    // 3. 调用驱动层读取函数
    int status = ReadValue(
        pPvt->offset,   // 例如: 0 (AMP:0)
        pPvt->channel,  // 例如: 0 (通道0 = RF3)
        &value          // 输出指针
    );

    // 4. 检查状态
    if (status != 0) {
        recGblSetSevr(pRec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 5. 设置记录值
    pRec->val = value;

    // 6. 返回成功
    return 0;  // 告诉EPICS继续处理记录
}
```

**DevPvt结构**:
```c
typedef struct {
    int offset;      // 数据类型编号 (AMP:0, PHASE:1, ...)
    int channel;     // 通道号 (0-7)
    char type_str[16]; // 类型字符串 "AMP", "PHASE", ...
} DevPvt;

// 例如，对于 INP="@AMP:0 ch=0"
// DevPvt { offset=0, channel=0, type_str="AMP" }
```

#### 阶段5: 驱动层读取

**文件**: `driverWrapper.c`

```c
int ReadValue(int offset, int channel, void *pValue)
{
    // 1. 验证参数
    if (channel < 0 || channel >= 8) {
        printf("ReadValue: invalid channel %d\n", channel);
        return -1;
    }

    // 2. 根据offset类型读取全局数组
    switch (offset) {
        case 0:  // AMP:0
            *(double *)pValue = (double)Amp[channel];
            break;

        case 1:  // PHASE:1
            *(double *)pValue = (double)Phase[channel];
            break;

        case 2:  // POWER:2
            *(double *)pValue = (double)Power[channel];
            break;

        case 3:  // VBPM:3
            *(double *)pValue = (double)VBPM[channel];
            break;

        case 4:  // IBPM:4
            *(double *)pValue = (double)IBPM[channel];
            break;

        // ... 更多offset类型

        default:
            printf("ReadValue: unknown offset %d\n", offset);
            return -1;
    }

    // 3. 返回成功
    return 0;
}
```

**数据流总结**:
```
硬件: Amp值 = 3.14 V
  ↓ funcGetRfInfo()
驱动层全局数组: Amp[0] = 3.14
  ↓ scanIoRequest()
EPICS扫描线程: 处理记录
  ↓ read_ai()
设备支持层: ReadValue(0, 0, &value)
  ↓
驱动层: value = Amp[0] = 3.14
  ↓
设备支持层: pRec->val = 3.14
  ↓
EPICS: 处理、报警、时间戳
  ↓
CA: 通知客户端
  ↓
客户端: 显示 3.14
```

### 时序分析

```
时间轴 (假设T=0时刻硬件数据更新):

T=0ms:
  硬件库: GetRfInfo()返回新数据
  驱动层: 更新Amp[0]=3.14
  驱动层: 调用scanIoRequest()

T=0.1ms:
  EPICS: I/O扫描线程被唤醒
  EPICS: 开始处理第一个记录

T=0.2ms:
  设备支持层: read_ai()被调用
  驱动层: ReadValue()返回3.14
  设备支持层: pRec->val = 3.14

T=0.3ms:
  EPICS: 应用转换和报警检查
  EPICS: 更新时间戳

T=0.5ms:
  CA服务器: 通知所有订阅客户端

T=1ms:
  客户端: camonitor显示新值

总延迟: ~1ms (从硬件到客户端)
```

## 📊 3. 周期扫描数据流（Periodic Scan）

### 与I/O中断的区别

```
I/O Interrupt:
  硬件驱动 → 立即触发记录处理
  优点: 低延迟，数据驱动
  缺点: 需要硬件支持

Periodic Scan:
  定时器 → 定期触发记录处理
  优点: 简单，不依赖硬件事件
  缺点: 固定延迟，可能采样过快/过慢
```

### 流程图

```
[EPICS] 扫描线程 (.5 second定时器)
   ↓
   每500ms触发一次
   ↓
[数据库层] aiRecord: SCAN=".5 second"
   ↓
   调用 devAiBPMmonitor::read_ai(pRec)
   ↓
[设备支持层] read_ai()
   ↓
   调用 ReadValue(offset, channel, &value)
   ↓
[驱动层] ReadValue()
   ↓
   读取全局数组 (当前缓存的值)
   ↓
   返回值可能是:
   - 100ms前的数据 (如果硬件更新周期=100ms)
   - 刚更新的数据 (运气好)
   - 500ms前的数据 (如果硬件更新慢)
```

### 示例

假设硬件每100ms更新一次数据：

```
时间轴:
T=0ms:    硬件更新 → Amp[0]=3.14
T=100ms:  硬件更新 → Amp[0]=3.20
T=200ms:  硬件更新 → Amp[0]=3.18
T=300ms:  硬件更新 → Amp[0]=3.22
T=400ms:  硬件更新 → Amp[0]=3.19
T=500ms:  EPICS周期扫描触发 → 读取Amp[0]=3.19
T=600ms:  硬件更新 → Amp[0]=3.21
T=700ms:  硬件更新 → Amp[0]=3.17
T=800ms:  硬件更新 → Amp[0]=3.23
T=900ms:  硬件更新 → Amp[0]=3.20
T=1000ms: EPICS周期扫描触发 → 读取Amp[0]=3.20
```

**问题**: 周期扫描只能采样到部分数据（500ms间隔 > 100ms更新）

**解决方案**: 使用I/O中断扫描，每次硬件更新都触发处理

## ✍️ 4. 标量写入数据流

### 完整流程图

```
[客户端] caput命令
   ↓
   caput iLinac_007:BPM14And15:SetPhaseOffset 10.5
   ↓
[Channel Access] CA客户端库
   ↓
   TCP连接到IOC的CA服务器
   ↓
[Channel Access] CA服务器 (IOC内)
   ↓
   找到对应的记录: aoRecord
   ↓
[数据库层] aoRecord: iLinac_007:BPM14And15:SetPhaseOffset
   ├→ 接收新值: 10.5
   ├→ 应用转换 (ASLO/AOFF/ESLO/EOFF)
   ├→ 检查限制 (DRVH/DRVL)
   ├→ 调用设备支持层: write_ao(pRec)
   ↓
[设备支持层] devBPMMonitor.c::write_ao()
   ├→ 取出DevPvt: offset=15, channel=0
   ├→ 准备写入值: value = pRec->val = 10.5
   ├→ 调用驱动: WriteValue(15, 0, &value)
   ↓
[驱动层] driverWrapper.c::WriteValue()
   ├→ 识别offset=15 → PHASE_OFFSET类型
   ├→ 调用硬件函数: funcSetPhaseOffset(0, 10.5)
   ↓
[硬件库] libBPMboard14And15.so::SetPhaseOffset()
   ├→ 准备VME命令
   ├→ 写入硬件寄存器
   ↓
[硬件] BPM板卡
   ├→ 更新相位偏移寄存器 = 10.5
   ↓
   返回成功
   ↓
[驱动层] WriteValue()返回0
   ↓
[设备支持层] write_ao()返回0
   ↓
[数据库层] EPICS标记写入成功
   ↓
[Channel Access] CA服务器通知客户端
   ↓
[客户端] caput显示: Old=0.0, New=10.5
```

### 详细代码

#### 阶段1: 客户端写入

```bash
$ caput iLinac_007:BPM14And15:SetPhaseOffset 10.5
Old : iLinac_007:BPM14And15:SetPhaseOffset  0
New : iLinac_007:BPM14And15:SetPhaseOffset  10.5
```

#### 阶段2: 数据库层记录定义

```
record(ao, "$(P):SetPhaseOffset")
{
    field(DESC, "Set Phase Offset")
    field(SCAN, "Passive")          # 写入时才处理
    field(DTYP, "BPMmonitor")
    field(OUT,  "@PHASE_OFFSET:15 ch=0")  # offset=15
    field(PREC, "2")
    field(EGU,  "deg")
    field(DRVH, "180")              # 最大值
    field(DRVL, "-180")             # 最小值
}
```

#### 阶段3: 设备支持层写入

**文件**: `devBPMMonitor.c`

```c
static long write_ao(aoRecord *pRec)
{
    // 1. 获取私有数据
    DevPvt *pPvt = (DevPvt *)pRec->dpvt;

    if (pPvt == NULL) {
        recGblSetSevr(pRec, WRITE_ALARM, INVALID_ALARM);
        return -1;
    }

    // 2. 获取要写入的值 (已经过EPICS转换)
    double value = pRec->val;

    // 3. 调用驱动层写入函数
    int status = WriteValue(
        pPvt->offset,   // 15 (PHASE_OFFSET:15)
        pPvt->channel,  // 0
        &value          // 10.5
    );

    // 4. 检查状态
    if (status != 0) {
        recGblSetSevr(pRec, WRITE_ALARM, INVALID_ALARM);
        return -1;
    }

    // 5. 返回成功
    return 0;
}
```

#### 阶段4: 驱动层写入

**文件**: `driverWrapper.c`

```c
int WriteValue(int offset, int channel, void *pValue)
{
    double value = *(double *)pValue;

    // 验证通道
    if (channel < 0 || channel >= 8) {
        return -1;
    }

    // 根据offset类型调用硬件函数
    switch (offset) {
        case 15:  // PHASE_OFFSET:15
            return funcSetPhaseOffset(channel, (float)value);

        case 16:  // AMPLITUDE_OFFSET:16
            return funcSetAmpOffset(channel, (float)value);

        // ... 更多写入类型

        default:
            printf("WriteValue: unknown offset %d\n", offset);
            return -1;
    }
}
```

#### 阶段5: 硬件函数

**硬件库**: `libBPMboard14And15.so`

```c
int SetPhaseOffset(int channel, float offset)
{
    // 1. 验证参数
    if (channel < 0 || channel >= 8) {
        return -1;
    }

    if (offset < -180.0 || offset > 180.0) {
        return -1;
    }

    // 2. 计算寄存器地址
    uint32_t base_addr = 0x10000000;  // VME基地址
    uint32_t reg_offset = 0x100 + channel * 0x10;  // 每个通道偏移
    uint32_t addr = base_addr + reg_offset;

    // 3. 转换为硬件格式（假设16位，单位0.1度）
    int16_t hw_value = (int16_t)(offset * 10.0);

    // 4. VME写入
    int status = vme_write16(addr, hw_value);

    // 5. 返回状态
    return status;
}
```

## 🌊 5. 波形数据流

### 波形 vs 标量

```
标量 (ai):
  - 单个值: 3.14
  - 简单快速
  - 实时监控

波形 (waveform):
  - 数组: [1.2, 3.4, 5.6, ..., 1000个点]
  - 大量数据
  - 历史分析、FFT等
```

### 波形读取流程

```
[硬件] 触发采集 → 10000点波形数据
   ↓
[硬件库] GetTrigWaveform(channel, buffer, &npts)
   ↓ 返回10000个采样点
   ↓
[驱动层] DataAcquisitionThread
   ├→ 分配全局缓冲区: WaveformBuf[channel][10000]
   ├→ 复制数据: memcpy(WaveformBuf[ch], buffer, npts*sizeof(float))
   ├→ 触发扫描: scanIoRequest(ioScanPvt)
   ↓
[EPICS] I/O中断扫描
   ↓
[数据库层] waveform记录
   ↓
[设备支持层] read_waveform()
   ├→ 调用驱动: ReadWaveform(offset, channel, buffer, &npts)
   ↓
[驱动层] ReadWaveform()
   ├→ 复制数据: memcpy(buffer, WaveformBuf[ch], npts*sizeof(float))
   ├→ 设置点数: *npts = 10000
   ↓
[设备支持层] read_waveform()
   ├→ 设置: pRec->bptr = buffer
   ├→ 设置: pRec->nord = npts
   ↓
[Channel Access] 传输10000点数组
   ↓
[客户端] 接收波形数组
```

### 数据库定义

```
record(waveform, "$(P):RFIn_01_TrigWaveform")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@TRIG_WAVEFORM:20 ch=0")
    field(FTVL, "FLOAT")          # 元素类型: float
    field(NELM, "10000")          # 最大元素数
}
```

### 设备支持层代码

```c
static long read_waveform(waveformRecord *pRec)
{
    DevPvt *pPvt = (DevPvt *)pRec->dpvt;
    int npts = 0;

    // 调用驱动读取波形
    int status = ReadWaveform(
        pPvt->offset,       // 20 (TRIG_WAVEFORM)
        pPvt->channel,      // 0
        pRec->bptr,         // 缓冲区指针 (EPICS分配的)
        &npts               // 输出: 实际点数
    );

    if (status == 0) {
        pRec->nord = npts;  // 设置实际读取的点数
        return 0;
    } else {
        pRec->nord = 0;
        return -1;
    }
}
```

### 驱动层代码

```c
int ReadWaveform(int offset, int channel, void *buffer, int *npts)
{
    if (offset == 20) {  // TRIG_WAVEFORM
        // 复制全局缓冲区到记录缓冲区
        memcpy(buffer, WaveformBuf[channel], 10000 * sizeof(float));
        *npts = 10000;
        return 0;
    }

    return -1;
}
```

### 性能考虑

```
数据量:
  10000点 × 4字节/点 = 40 KB

传输时间 (100 Mbps以太网):
  40 KB / 12.5 MB/s ≈ 3.2 ms

总延迟:
  硬件采集: ~10 ms
  数据复制: ~0.5 ms
  EPICS处理: ~1 ms
  CA传输:   ~3.2 ms
  总计:     ~15 ms
```

## 🔗 6. 计算记录数据流

### Calc记录用途

```
场景：计算RF功率
  功率 = 幅度² × 负载电阻

输入：RF3Amp (ai记录)
输出：RF3Power (calc记录)
公式：P = A² × 50Ω
```

### 数据流

```
[硬件] Amp = 3.14 V
   ↓
[驱动层] Amp[0] = 3.14
   ↓ scanIoRequest()
   ↓
[数据库层] RFIn_01_Amp (ai)
   ├→ 处理记录
   ├→ val = 3.14
   ├→ FLNK → RF3Power  ← 转发链接
   ↓
[数据库层] RF3Power (calc)
   ├→ INPA = RFIn_01_Amp.VAL = 3.14
   ├→ CALC = "A*A*50"
   ├→ 计算: val = 3.14 × 3.14 × 50 = 493.38
   ├→ 不需要设备支持层！（纯数据库层处理）
   ↓
[Channel Access] 通知客户端
   ↓
[客户端] RF3Power = 493.38 W
```

### 记录定义

```
# 输入：幅度
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, "I/O Intr")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
    field(FLNK, "$(P):RF3Power")  # ← 处理完后触发calc记录
}

# 计算：功率
record(calc, "$(P):RF3Power")
{
    field(SCAN, "Passive")              # ← 被FLNK触发
    field(CALC, "A*A*50")               # ← P = A²R
    field(INPA, "$(P):RFIn_01_Amp CP")  # ← 链接到ai记录
    field(PREC, "2")
    field(EGU,  "W")
}
```

### 数据流特点

- **无设备支持层**: calc记录纯软件计算
- **链接驱动**: FLNK或CP链接触发处理
- **实时计算**: 输入更新立即计算输出
- **零硬件开销**: 不访问驱动层或硬件

## 🔄 7. 多PV联动数据流

### 场景：RF通道切换

```
用户需求：
  1. 选择RF通道 (0-7)
  2. 显示该通道的Amp/Phase/Power
  3. 自动更新
```

### 实现方式

```
[客户端] caput SelectChannel 3
   ↓
[数据库层] SelectChannel (mbbo记录)
   ├→ val = 3
   ├→ FLNK → UpdateChannel
   ↓
[数据库层] UpdateChannel (calcout记录)
   ├→ CALC = "A"
   ├→ INPA = "SelectChannel"
   ├→ OUT  = "ChannelAmp.PROC PP"  ← 触发处理
   ├→ OOPT = "Every Time"
   ↓
[数据库层] ChannelAmp (ai记录)
   ├→ 强制重新处理
   ├→ 调用 read_ai()
   ├→ 读取 Amp[3]
   ↓
同时触发:
   ├→ ChannelPhase → 读取 Phase[3]
   └→ ChannelPower → 读取 Power[3]
```

### 记录定义

```
# 通道选择
record(mbbo, "$(P):SelectChannel")
{
    field(ZRST, "RF3")
    field(ONST, "RF4")
    field(TWST, "RF5")
    # ... up to FFST
    field(FLNK, "$(P):ChannelAmp.PROC")
}

# 动态显示幅度
record(ai, "$(P):ChannelAmp")
{
    field(SCAN, "Passive")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")  # ch从SelectChannel动态获取
    field(FLNK, "$(P):ChannelPhase.PROC")
}

# 动态显示相位
record(ai, "$(P):ChannelPhase")
{
    field(SCAN, "Passive")
    field(DTYP, "BPMmonitor")
    field(INP,  "@PHASE:1 ch=0")
    field(FLNK, "$(P):ChannelPower.PROC")
}

# 动态显示功率
record(ai, "$(P):ChannelPower")
{
    field(SCAN, "Passive")
    field(DTYP, "BPMmonitor")
    field(INP,  "@POWER:2 ch=0")
}
```

## 🐛 8. 数据流调试

### 8.1 添加打印追踪

**驱动层** (`driverWrapper.c`):
```c
void *DataAcquisitionThread(void *arg)
{
    while (1) {
        funcGetRfInfo(Amp, Phase, ...);

        printf("[DRIVER] Acquired: Amp[0]=%.3f, Phase[0]=%.3f\n",
               Amp[0], Phase[0]);

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.1);
    }
}

int ReadValue(int offset, int channel, void *pValue)
{
    printf("[DRIVER] ReadValue(offset=%d, ch=%d)\n", offset, channel);

    switch (offset) {
        case 0:
            *(double *)pValue = (double)Amp[channel];
            printf("[DRIVER]   → returning %.3f\n", *(double *)pValue);
            break;
    }
    return 0;
}
```

**设备支持层** (`devBPMMonitor.c`):
```c
static long read_ai(aiRecord *pRec)
{
    DevPvt *pPvt = (DevPvt *)pRec->dpvt;
    double value;

    printf("[DEVSUP] read_ai for %s (offset=%d, ch=%d)\n",
           pRec->name, pPvt->offset, pPvt->channel);

    int status = ReadValue(pPvt->offset, pPvt->channel, &value);

    printf("[DEVSUP]   → got value=%.3f, status=%d\n", value, status);

    pRec->val = value;
    return status;
}
```

**输出示例**:
```
[DRIVER] Acquired: Amp[0]=3.140, Phase[0]=45.600
[DEVSUP] read_ai for iLinac_007:BPM14And15:RFIn_01_Amp (offset=0, ch=0)
[DRIVER] ReadValue(offset=0, ch=0)
[DRIVER]   → returning 3.140
[DEVSUP]   → got value=3.140, status=0
```

### 8.2 使用EPICS内置调试

```bash
# IOC shell
dbpf iLinac_007:BPM14And15:RFIn_01_Amp.TPRO 1  # 启用trace

# 输出：
UDF is TRUE
SEVR is INVALID STAT is UNKNOWN
devAiBPMmonitor::read_ai called
record processed
val = 3.140
```

### 8.3 监控数据流

```bash
# 终端1: 监控原始PV
camonitor iLinac_007:BPM14And15:RFIn_01_Amp

# 终端2: 监控计算PV
camonitor iLinac_007:BPM14And15:RF3Power

# 终端3: 查看时间戳
caget -tS iLinac_007:BPM14And15:RFIn_01_Amp

# 输出:
iLinac_007:BPM14And15:RFIn_01_Amp 2025-11-08 10:23:45.123456 3.14
```

## ✅ 学习检查点

完成本节后，你应该能够：

### 读取流程
- [ ] 能画出I/O中断数据流的完整流程图
- [ ] 能解释scanIoRequest()的作用和时机
- [ ] 理解周期扫描和I/O中断的数据流差异
- [ ] 知道全局数组在哪一层、何时更新

### 写入流程
- [ ] 能画出caput到硬件的完整路径
- [ ] 理解WriteValue()和ReadValue()的对称性
- [ ] 知道DRVH/DRVL在哪里起作用

### 特殊流程
- [ ] 理解波形数据和标量数据的处理差异
- [ ] 理解calc记录如何避免访问硬件
- [ ] 能够解释FLNK链接如何驱动数据流

### 调试能力
- [ ] 能够在三层中添加调试输出
- [ ] 能够使用EPICS工具追踪数据流
- [ ] 能够定位数据流断点

## 🚀 下一步

现在你已经理解了数据流，接下来：

👉 [03-initialization-sequence.md](./03-initialization-sequence.md) - 详细了解系统如何初始化

或者深入某个主题：
- [04-memory-model.md](./04-memory-model.md) - 内存模型和全局变量
- [06-thread-model.md](./06-thread-model.md) - 线程模型和同步

---

**💡 思考题**:

1. 为什么I/O中断比周期扫描延迟更低？
2. 如果两个ai记录同时读取Amp[0]，会有竞态条件吗？
3. 波形数据为什么不用CP链接而是用I/O中断？

**⏱️ 实践建议**: 用GDB或printf在三层中设置断点，实际追踪一次RF3Amp的完整数据流。
