# Part 5.4: read/write函数详解

> **目标**: 深入理解read和write函数的实现
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 60分钟

## 📖 read/write函数的作用

- **read**: 从硬件读取数据，更新Record的值
- **write**: 将Record的值写入硬件

这两个函数在**Record被处理（process）时**调用。

---

## 🔍 read_ai函数详解

### 完整实现

```c
// 位置: BPMmonitorApp/src/devBPMMonitor.c

static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    float value;

    // 步骤1: 检查DevPvt是否有效
    if (pPvt == NULL) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 步骤2: 调用驱动层ReadData函数
    value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 步骤3: 更新Record值
    prec->val = value;

    // 步骤4: 清除未定义标志
    prec->udf = 0;

    return 0;  // 0表示成功，Record会继续处理
}
```

---

## 📝 read_ai逐步解析

### 步骤1: 获取DevPvt

```c
DevPvt *pPvt = (DevPvt *)prec->dpvt;
if (pPvt == NULL) {
    recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
    return -1;
}
```

**为什么检查NULL？**
- 如果init_record失败，dpvt可能是NULL
- 防止空指针解引用崩溃

**recGblSetSevr作用**:
- 设置Record的报警状态
- `READ_ALARM`: 读取错误
- `INVALID_ALARM`: 严重级别

---

### 步骤2: 调用驱动层

```c
value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);
```

**ReadData接口**（来自driverWrapper.c）:
```c
float ReadData(int offset, int channel, int type);
```

**参数来源**:
```
.db文件: INP="@0 3 0"
   ↓ init_record解析
DevPvt: {offset=0, channel=3, type=0}
   ↓ read_ai使用
ReadData(0, 3, 0)
   ↓
驱动层返回RF3的幅度值
```

---

### 步骤3: 更新Record值

```c
prec->val = value;
```

**VAL字段**:
- 这是ai Record的主要值字段
- 类型：`double`
- 客户端通过PV访问的就是这个值

**后续处理**:
```
prec->val = value;
   ↓
EPICS自动处理:
  - 应用ASLO/AOFF（模拟转换）
  - 应用ESLO/EOFF（工程单位转换）
  - 检查报警限值（HIHI/HIGH/LOW/LOLO）
  - 触发前向链接（FLNK）
  - 通知CA客户端
```

---

### 步骤4: 清除UDF标志

```c
prec->udf = 0;
```

**UDF** = **Undefined**（未定义标志）

**意义**:
- `udf = 1`: 值未初始化（默认值）
- `udf = 0`: 值已被设置

**为什么重要？**
```
Record刚创建时: udf = 1
   ↓
第一次read_ai成功: udf = 0
   ↓
客户端可以看到这个值是有效的
```

---

## ✍️ write_ao函数详解

### 完整实现

```c
static long write_ao(aoRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    int value;
    int ret;

    // 步骤1: 检查DevPvt
    if (pPvt == NULL) {
        recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM);
        return -1;
    }

    // 步骤2: 从Record获取值
    value = (int)prec->val;

    // 步骤3: 调用驱动层SetReg
    ret = SetReg(pPvt->channel, value);

    // 步骤4: 检查返回值
    if (ret != 0) {
        recGblSetSevr(prec, WRITE_ALARM, MAJOR_ALARM);
        return -1;
    }

    // 步骤5: 清除UDF
    prec->udf = 0;

    return 0;
}
```

---

## 📝 write_ao逐步解析

### 步骤1-2: 获取要写入的值

```c
DevPvt *pPvt = (DevPvt *)prec->dpvt;
value = (int)prec->val;
```

**数据流向**:
```
客户端: caput BPM:01:Reg10 12345
   ↓
EPICS处理:
  - 写入到prec->val
  - 应用ESLO/EOFF转换
  - 调用write_ao
   ↓
设备支持: value = prec->val
   ↓
驱动层: SetReg(10, 12345)
```

---

### 步骤3: 调用驱动层

```c
ret = SetReg(pPvt->channel, value);
```

**SetReg接口**（来自driverWrapper.c）:
```c
int SetReg(int addr, int value);
```

**参数**:
- `addr`: 寄存器地址（从pPvt->channel）
- `value`: 要写入的值

---

### 步骤4: 错误处理

```c
if (ret != 0) {
    recGblSetSevr(prec, WRITE_ALARM, MAJOR_ALARM);
    return -1;
}
```

**好的实践**:
- 检查驱动层返回值
- 设置适当的报警
- 返回错误状态

---

## 🌊 read_waveform函数

### Waveform特殊处理

```c
static long read_waveform(waveformRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    int nread;

    if (pPvt == NULL) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 调用ReadWaveform读取数组数据
    nread = ReadWaveform(pPvt->offset,    // 波形类型
                        pPvt->channel,     // 通道
                        (float *)prec->bptr, // 数据缓冲区
                        prec->nelm);       // 最大元素数

    if (nread < 0) {
        recGblSetSevr(prec, READ_ALARM, MAJOR_ALARM);
        return -1;
    }

    // 更新实际读取的元素数
    prec->nord = nread;
    prec->udf = 0;

    return 0;
}
```

**关键字段**:
- `prec->bptr`: 指向数据缓冲区
- `prec->nelm`: 最大元素数（Number of ELeMents）
- `prec->nord`: 实际读取数（Number of elements ReaD）

---

## 🔄 read/write的调用时机

### 调用流程

```
1. I/O中断触发
   scanIoRequest(ioscanpvt)
      ↓
   EPICS扫描线程唤醒
      ↓
   对所有SCAN="I/O Intr"的Record:
      process(prec)
         ↓
         read_ai(prec)  ← 在这里调用
         ↓
         处理数据（报警检查、转换等）
         ↓
         触发FLNK
         ↓
         通知CA客户端

2. 周期扫描
   Timer到期
      ↓
   对所有SCAN=".1 second"的Record:
      process(prec)
         ↓
         read_ai(prec)

3. 用户写入
   caput PV_NAME value
      ↓
   EPICS CA服务器
      ↓
   dbPutField()
      ↓
   process(aoRecord *prec)
      ↓
      write_ao(prec)  ← 在这里调用
```

---

## ⚠️ 常见错误

### 错误1: 忘记清除UDF

```c
// ❌ 错误
static long read_ai(aiRecord *prec) {
    // ...
    prec->val = value;
    return 0;
    // 忘记: prec->udf = 0;
}
// 结果：PV永远显示UDF状态
```

---

### 错误2: 不检查返回值

```c
// ❌ 错误
static long write_ao(aoRecord *prec) {
    SetReg(pPvt->channel, value);  // 不检查返回值
    return 0;  // 总是返回成功
}

// ✅ 正确
int ret = SetReg(pPvt->channel, value);
if (ret != 0) {
    recGblSetSevr(prec, WRITE_ALARM, MAJOR_ALARM);
    return -1;
}
```

---

### 错误3: 类型转换错误

```c
// ❌ 错误：精度丢失
prec->val = (int)value;  // 如果value是浮点数

// ✅ 正确
prec->val = value;  // 保持精度
```

---

## 🎨 高级技巧

### 技巧1: 添加数据验证

```c
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    float value;

    value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    // 检查数据有效性
    if (isnan(value) || isinf(value)) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    // 范围检查
    if (value < -10.0 || value > 10.0) {
        // 记录警告，但继续处理
        printf("Warning: Value out of normal range: %.2f\n", value);
    }

    prec->val = value;
    prec->udf = 0;

    return 0;
}
```

---

### 技巧2: 性能测量

```c
static long read_ai(aiRecord *prec)
{
    DevPvt *pPvt = (DevPvt *)prec->dpvt;
    struct timeval start, end;
    float value;

    gettimeofday(&start, NULL);

    value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 +
                    (end.tv_usec - start.tv_usec) / 1000.0;

    if (elapsed > 10.0) {  // 超过10ms
        printf("Slow read detected: %.2f ms\n", elapsed);
    }

    prec->val = value;
    prec->udf = 0;

    return 0;
}
```

---

## ✅ 学习检查点

- [ ] 理解read_ai的4个步骤
- [ ] 理解write_ao的5个步骤
- [ ] 知道何时调用read/write
- [ ] 理解UDF标志的作用
- [ ] 能够添加错误处理和数据验证

---

## 🎯 下一步

- **[05-devpvt.md](./05-devpvt.md)** - DevPvt管理详解

---

**关键**: read/write是设备支持的**核心功能**，连接Record和驱动层！💡
