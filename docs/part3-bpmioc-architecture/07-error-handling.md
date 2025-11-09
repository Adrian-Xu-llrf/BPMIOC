# 07: BPMIOC错误处理策略

> **难度**: ⭐⭐⭐☆☆
> **预计时间**: 50分钟
> **前置知识**: 01-architecture-overview.md, 02-data-flow.md

## 📋 本文目标

本文分析BPMIOC的错误处理机制，帮助你理解系统如何应对各种异常情况。

完成本文后，你将能够：
- ✅ 理解各层的错误处理策略
- ✅ 掌握EPICS的Alarm机制
- ✅ 知道如何调试错误
- ✅ 能够添加错误处理代码

## 🎯 错误处理总览

BPMIOC采用**分层错误处理**策略：

```
数据库层 ──> EPICS Alarm机制
              ├──> SEVR (Severity)
              ├──> STAT (Status)
              └──> 客户端报警

设备支持层 ──> 返回错误码
              ├──> return 0 (成功)
              ├──> return -1 (失败)
              └──> recGblRecordError()

驱动层 ──> 防御式编程
           ├──> 参数检查
           ├──> 返回默认值
           └──> 日志记录

硬件层 ──> 硬件库返回值检查
           ├──> dlopen/dlsym错误
           └──> 硬件通信错误
```

## 📊 各层错误处理策略

### 1. 硬件层错误处理

#### dlopen/dlsym错误

```c
// driverWrapper.c - InitDevice()

long InitDevice()
{
    void *handle;
    char *error;

    // 1. 打开动态库
    handle = dlopen(dll_filename, RTLD_LAZY);
    if (!handle) {
        // 错误处理：打印错误信息并返回失败
        fprintf(stderr, "Error: %s\n", dlerror());
        return -1;
    }

    // 2. 清除已有错误
    dlerror();

    // 3. 获取函数指针
    funcSystemInit = (int(*)(void))dlsym(handle, "SystemInit");

    // 4. 检查错误
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "Error: %s\n", error);
        dlclose(handle);
        return -1;
    }

    // 5. 调用初始化函数
    if (funcSystemInit() != 0) {
        fprintf(stderr, "SystemInit failed\n");
        return -1;
    }

    return 0;
}
```

**错误类型**：
- ❌ 库文件不存在：`cannot open shared object file`
- ❌ 函数符号不存在：`undefined symbol`
- ❌ 硬件初始化失败：`SystemInit() returned -1`

### 2. 驱动层错误处理

#### 防御式编程

```c
// driverWrapper.c

float ReadData(int offset, int channel, int type)
{
    // 1. 参数验证
    if (channel < 0 || channel > 7) {
        fprintf(stderr, "Invalid channel: %d\n", channel);
        return 0.0;  // 返回默认值
    }

    switch (offset)
    {
        case 0:  // RF信息
            if (type == AMP)
                return GetRFInfo(channel, 0);
            else if (type == PHASE)
                return GetRFInfo(channel, 1);
            else
                return 0.0;  // 未知type，返回0

        // ... 其他case

        default:
            // 未知offset，记录警告并返回0
            fprintf(stderr, "Unknown offset: %d\n", offset);
            return 0.0;
    }
}
```

#### 硬件通信错误

```c
// 推荐：添加返回值检查

float GetRFInfo(int channel, int type)
{
    float amp[8], phase[8], power[8];
    float vbpm, ibpm, vcri, icri;
    int pset;

    // 调用硬件库函数
    int ret = funcGetRfInfo(amp, phase, power,
                            &vbpm, &ibpm, &vcri, &icri, &pset);

    if (ret != 0) {
        // 硬件通信失败
        fprintf(stderr, "GetRfInfo failed: %d\n", ret);
        return 0.0;  // 返回默认值
    }

    if (type == 0)      // 幅度
        return amp[channel];
    else if (type == 1)  // 相位
        return phase[channel];
    else                // 功率
        return power[channel];
}
```

### 3. 设备支持层错误处理

#### init_record错误

```c
// devBPMMonitor.c

long init_record_ai(aiRecord *prec)
{
    struct instio *pinstio;
    recordpara_t *recordpara;

    // 1. 检查设备类型
    if (prec->inp.type != INST_IO) {
        recGblRecordError(S_db_badField, (void *)prec,
                          "devAiBPM (init_record) Illegal INP field");
        return S_db_badField;
    }

    // 2. 分配内存
    recordpara = (recordpara_t *)malloc(sizeof(recordpara_t));
    if (!recordpara) {
        recGblRecordError(S_db_noMemory, (void *)prec,
                          "devAiBPM (init_record) malloc failed");
        return S_db_noMemory;
    }

    // 3. 解析参数
    pinstio = (struct instio *)&(prec->inp.value);
    char *parm = pinstio->string;

    if (!parm || strlen(parm) == 0) {
        recGblRecordError(S_db_badField, (void *)prec,
                          "devAiBPM (init_record) Empty INP string");
        free(recordpara);
        return S_db_badField;
    }

    // ... 继续初始化

    return 0;  // 成功
}
```

**EPICS错误码**：

| 错误码 | 值 | 含义 |
|--------|---|------|
| `S_db_badField` | 错误 | 字段值不合法 |
| `S_db_noMemory` | 错误 | 内存分配失败 |
| `S_db_notFound` | 错误 | 未找到资源 |

#### read/write错误

```c
long read_ai(aiRecord *prec)
{
    recordpara_t *priv = (recordpara_t *)prec->dpvt;

    // 1. 检查私有数据
    if (!priv) {
        // 设置Record为INVALID状态
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 2. 读取数据
    float value = ReadData(priv->offset, priv->channel, priv->type);

    // 3. 检查数据有效性（示例）
    if (isnan(value) || isinf(value)) {
        // 设置Record为INVALID状态
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 4. 赋值
    prec->val = value;

    return 0;  // 成功
}
```

### 4. 数据库层错误处理：EPICS Alarm机制

#### Alarm字段

每个Record都有Alarm相关字段：

```c
// aiRecord结构（部分）
typedef struct aiRecord {
    epicsEnum16  sevr;   // 当前Severity
    epicsEnum16  stat;   // 当前Status
    epicsEnum16  nsta;   // 新Status
    epicsEnum16  nsev;   // 新Severity

    double       hihi;   // 高高限
    double       high;   // 高限
    double       low;    // 低限
    double       lolo;   // 低低限

    epicsEnum16  hhsv;   // HIHI Severity
    epicsEnum16  hsv;    // HIGH Severity
    epicsEnum16  lsv;    // LOW Severity
    epicsEnum16  llsv;   // LOLO Severity
} aiRecord;
```

#### Severity级别

| Severity | 值 | 颜色 | 含义 |
|----------|---|------|------|
| `NO_ALARM` | 0 | 绿色 | 正常 |
| `MINOR_ALARM` | 1 | 黄色 | 次要报警 |
| `MAJOR_ALARM` | 2 | 红色 | 重要报警 |
| `INVALID_ALARM` | 3 | 白色 | 数据无效 |

#### Alarm配置示例

```
# BPMMonitor.db

record(ai, "LLRF:BPM:RF3Amp")
{
    field(DTYP, "BPM")
    field(INP,  "@0:3")
    field(SCAN, "I/O Intr")

    # Alarm配置
    field(HIHI, "5.0")      # 高高限：5.0V
    field(HIGH, "4.5")      # 高限：4.5V
    field(LOW,  "0.5")      # 低限：0.5V
    field(LOLO, "0.1")      # 低低限：0.1V

    field(HHSV, "MAJOR")    # 超过HIHI为MAJOR报警
    field(HSV,  "MINOR")    # 超过HIGH为MINOR报警
    field(LSV,  "MINOR")    # 低于LOW为MINOR报警
    field(LLSV, "MAJOR")    # 低于LOLO为MAJOR报警
}
```

#### Alarm状态

| Status | 值 | 含义 |
|--------|---|------|
| `NO_ALARM` | 0 | 正常 |
| `READ_ALARM` | 1 | 读取错误 |
| `WRITE_ALARM` | 2 | 写入错误 |
| `HIHI_ALARM` | 3 | 超过HIHI限 |
| `HIGH_ALARM` | 4 | 超过HIGH限 |
| `LOLO_ALARM` | 5 | 低于LOLO限 |
| `LOW_ALARM` | 6 | 低于LOW限 |
| `STATE_ALARM` | 7 | 状态报警 |
| `COS_ALARM` | 8 | 变化速率报警 |
| `COMM_ALARM` | 9 | 通信错误 |
| `TIMEOUT_ALARM` | 10 | 超时 |
| `HWLIMIT_ALARM` | 11 | 硬件限位 |
| `CALC_ALARM` | 12 | 计算错误 |
| `SCAN_ALARM` | 13 | 扫描错误 |
| `LINK_ALARM` | 14 | 链接错误 |
| `SOFT_ALARM` | 15 | 软件定义报警 |
| `BAD_SUB_ALARM` | 16 | 子程序错误 |
| `UDF_ALARM` | 17 | 未定义值 |
| `DISABLE_ALARM` | 18 | 禁用状态 |
| `SIMM_ALARM` | 19 | 仿真模式 |
| `READ_ACCESS_ALARM` | 20 | 读权限错误 |
| `WRITE_ACCESS_ALARM` | 21 | 写权限错误 |

## 🔍 错误检测和诊断

### 1. 查看PV的Alarm状态

```bash
# 方法1：caget查看Severity和Status
$ caget LLRF:BPM:RF3Amp.SEVR LLRF:BPM:RF3Amp.STAT
LLRF:BPM:RF3Amp.SEVR NO_ALARM
LLRF:BPM:RF3Amp.STAT NO_ALARM

# 如果有报警
$ caget LLRF:BPM:RF3Amp.SEVR LLRF:BPM:RF3Amp.STAT
LLRF:BPM:RF3Amp.SEVR MAJOR
LLRF:BPM:RF3Amp.STAT HIHI_ALARM

# 方法2：使用-a查看所有字段
$ caget -a LLRF:BPM:RF3Amp

# 方法3：CS-Studio / Phoebus GUI
# 报警的PV会显示为黄色（MINOR）或红色（MAJOR）
```

### 2. IOC日志查看

```bash
# 查看IOC控制台输出
# 错误信息会打印到stderr

# 示例输出
Error: cannot open shared object file: libBPMboard14And15.so
devAiBPM (init_record) Illegal INP field
SystemInit failed
```

### 3. 使用dbpr调试

```bash
# 在IOC shell中
iocsh> dbpr LLRF:BPM:RF3Amp 3

# 输出（级别3）：
ASG:                DESC: RF3 Amplitude
DISA: 0             DISP: 0             DISS: NO_ALARM
DISV: 1             NAME: LLRF:BPM:RF3Amp
SEVR: MAJOR         STAT: HIHI_ALARM    ← 报警状态！
TPRO: 0             VAL: 5.2            ← 当前值
HIHI: 5.0           HIGH: 4.5           ← 限值
LOW: 0.5            LOLO: 0.1
```

## 🛠️ 添加自定义错误处理

### 示例1：添加数据范围检查

```c
// driverWrapper.c

float ReadData(int offset, int channel, int type)
{
    float value;

    switch (offset)
    {
        case 0:  // RF信息
            value = GetRFInfo(channel, type);

            // 范围检查
            if (value < 0.0 || value > 10.0) {
                fprintf(stderr, "WARNING: RF value out of range: %.2f\n", value);
                // 可选：截断到合理范围
                value = (value < 0.0) ? 0.0 : 10.0;
            }

            return value;

        // ... 其他case
    }
}
```

### 示例2：添加硬件通信重试

```c
// 推荐：添加重试机制

#define MAX_RETRIES 3

float GetRFInfo(int channel, int type)
{
    float amp[8], phase[8], power[8];
    int retry;

    for (retry = 0; retry < MAX_RETRIES; retry++) {
        int ret = funcGetRfInfo(amp, phase, power, ...);

        if (ret == 0) {
            // 成功
            break;
        }

        fprintf(stderr, "GetRfInfo retry %d/%d\n", retry+1, MAX_RETRIES);
        usleep(10000);  // 等待10ms后重试
    }

    if (retry == MAX_RETRIES) {
        fprintf(stderr, "GetRfInfo failed after %d retries\n", MAX_RETRIES);
        return 0.0;  // 返回默认值
    }

    return (type == 0) ? amp[channel] : phase[channel];
}
```

### 示例3：添加Alarm处理回调

```c
// 在.db中添加Forward Link

record(ai, "LLRF:BPM:RF3Amp")
{
    field(DTYP, "BPM")
    field(INP,  "@0:3")
    field(SCAN, "I/O Intr")

    field(HIHI, "5.0")
    field(HHSV, "MAJOR")

    # 报警时Forward到处理Record
    field(FLNK, "LLRF:BPM:RF3Amp_AlarmHandler")
}

# 报警处理Record
record(calcout, "LLRF:BPM:RF3Amp_AlarmHandler")
{
    field(INPA, "LLRF:BPM:RF3Amp.SEVR")
    field(CALC, "A >= 2")  # MAJOR or INVALID

    # 如果报警，发送通知
    field(OOPT, "When Non-zero")
    field(OUT,  "LLRF:BPM:AlarmNotify PP")
}
```

## ✅ 错误处理最佳实践

### 1. 分层处理原则

```
硬件层错误 → 驱动层处理（日志+默认值）
    ↓
驱动层错误 → 设备支持层处理（返回错误码）
    ↓
设备支持层错误 → 数据库层Alarm
    ↓
数据库层Alarm → 客户端GUI显示
```

### 2. 错误日志规范

```c
// 推荐格式
fprintf(stderr, "[%s:%d] ERROR: %s\n", __FILE__, __LINE__, message);

// 示例
fprintf(stderr, "[driverWrapper.c:123] ERROR: dlopen failed: %s\n", dlerror());
```

### 3. 优雅降级

```c
// 当硬件不可用时，仍能运行（模拟模式）

if (dlopen(...) == NULL) {
    fprintf(stderr, "WARNING: Hardware library not found, using simulation mode\n");
    use_simulation = 1;
}

float GetRFInfo(...) {
    if (use_simulation) {
        return simulate_rf_data();  // 返回模拟数据
    } else {
        return funcGetRfInfo(...);  // 返回真实数据
    }
}
```

## 🎯 学习检查点

完成本文后，你应该能够回答：

1. **错误处理策略**：
   - [ ] 各层如何处理错误？
   - [ ] 什么时候使用返回错误码？什么时候使用Alarm？
   - [ ] 如何添加重试机制？

2. **EPICS Alarm**：
   - [ ] Severity有哪些级别？
   - [ ] 如何配置HIHI/HIGH/LOW/LOLO？
   - [ ] 如何查看PV的Alarm状态？

3. **调试**：
   - [ ] 如何查看IOC错误日志？
   - [ ] 如何使用dbpr调试Record？
   - [ ] 硬件库加载失败的常见原因？

## 🔗 相关文档

- **[02-data-flow.md](./02-data-flow.md)** - 理解数据流
- **[06-thread-model.md](./06-thread-model.md)** - 线程模型
- **[Part 10: 调试与测试](/docs/part10-debugging-testing/)** - 详细调试技巧

## 📚 扩展阅读

- [EPICS Application Developer's Guide - Chapter 14: Database Monitoring](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/DatabaseMonitoring.html)
- [EPICS Record Reference Manual - Alarm Specification](https://epics.anl.gov/base/R3-15/6-docs/RecordReference.html#Alarm%20Specification)

---

**下一篇**: [08-performance-analysis.md](./08-performance-analysis.md) - 性能分析

**实践练习**:
1. 为`LLRF:BPM:X1`添加HIHI/LOLO报警配置
2. 修改ReadData添加参数范围检查
3. 故意破坏硬件库路径，观察错误信息
4. 使用caget查看所有PV的Alarm状态
