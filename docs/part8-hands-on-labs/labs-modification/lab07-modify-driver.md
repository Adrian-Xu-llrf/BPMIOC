# Lab 7: 修改驱动层函数

> **难度**: ⭐⭐⭐
> **时间**: 3小时
> **前置**: Lab 6, Part 4

## 🎯 实验目标

学会如何修改驱动层代码来支持新功能：

1. 理解驱动层的职责
2. 修改ReadData()函数支持新数据类型
3. 理解offset-type映射机制
4. 添加调试和错误处理
5. 测试驱动层修改

---

## 📚 背景知识

### 驱动层在三层架构中的位置

```
数据库层 (.db文件)
  Record: "$(P):Temperature"
  INP: "@TEMP:0"
    ↓
设备支持层 (devBPMMonitor.c)
  read_ai() {
      ReadData(offset, channel, type)  ← 调用驱动层
  }
    ↓
驱动层 (driverWrapper.c)  ← 本实验修改的部分
  ReadData() {
      // 根据offset和type读取硬件
      return value;
  }
    ↓
硬件层 (BPM_RFIn_ReadADC / Mock库)
```

---

### ReadData函数的作用

**当前实现** (driverWrapper.c:232):
```c
float ReadData(int offset, int channel, int type)
{
    float ret = 0.0;

    switch (offset) {
        case 0:  // AMP offset
            ret = BPM_RFIn_ReadADC(channel, type);
            break;
        case 1:  // XY offset
            ret = BPM_XY_Read(channel);
            break;
        // ...
    }

    return ret;
}
```

**问题**: 不支持温度读取（INP="@TEMP:0"）

---

## 🔧 实验任务

### 任务1: 添加温度读取支持

**需求**: 修改ReadData()支持`INP="@TEMP:0"`

---

### 步骤1: 理解当前offset映射

**查看driverWrapper.c中的offset定义**:

```bash
cd /home/user/BPMIOC/BPMmonitorApp/src

# 查看ReadData函数
grep -A 30 "float ReadData" driverWrapper.c
```

**当前offset映射**:
```c
offset=0 → AMP (RF幅度)
offset=1 → XY (位置)
offset=2 → PHS (相位)
offset=3 → BTN (按钮)
```

**我们添加**:
```c
offset=10 → TEMP (温度)
```

---

### 步骤2: 备份源文件

```bash
cp driverWrapper.c driverWrapper.c.backup

# 确认备份
ls -l driverWrapper.c*
```

---

### 步骤3: 添加温度读取函数

**在driverWrapper.c中添加温度读取函数**:

找到硬件函数部分（例如`BPM_RFIn_ReadADC`附近），添加：

```c
// 在文件顶部或适当位置添加

/**
 * @brief 读取FPGA温度
 * @return 温度值（摄氏度）
 */
float BPM_ReadTemperature(void)
{
    // 模拟实现：生成20-60度的随机温度
    static int init = 0;
    if (!init) {
        srand(time(NULL));
        init = 1;
    }

    // 基础温度 + 随机波动
    float base_temp = 45.0;
    float noise = (rand() % 200 - 100) / 10.0;  // -10 ~ +10度

    return base_temp + noise;
}
```

**注意**:
- 这是**模拟实现**，真实硬件需要读取温度传感器寄存器
- 如果使用Mock库，应该调用Mock库的温度函数

---

### 步骤4: 修改ReadData()函数

**找到ReadData()函数**，添加新的case：

```c
float ReadData(int offset, int channel, int type)
{
    float ret = 0.0;

    switch (offset) {
        case 0:  // AMP offset
            ret = BPM_RFIn_ReadADC(channel, type);
            break;

        case 1:  // XY offset
            ret = BPM_XY_Read(channel);
            break;

        case 2:  // PHS offset
            ret = BPM_RFIn_ReadADC(channel, type);
            break;

        case 3:  // BTN offset
            ret = BPM_BTN_Read(channel);
            break;

        case 10:  // TEMP offset ← 新添加
            ret = BPM_ReadTemperature();
            if (devBPMmonitorDebug > 0) {
                printf("ReadData: TEMP offset, temp=%.2f C\n", ret);
            }
            break;

        default:
            printf("ReadData: Unknown offset %d\n", offset);
            ret = 0.0;
            break;
    }

    return ret;
}
```

---

### 步骤5: 更新设备支持层的INP解析（可选）

**当前问题**: 设备支持层的init_record可能不识别`"@TEMP:0"`格式

**查看devBPMMonitor.c的init_record**:

```bash
cd /home/user/BPMIOC/BPMmonitorApp/src
grep -A 50 "init_record_ai" devBPMMonitor.c
```

**找到INP解析部分**:
```c
// 当前可能的解析
sscanf(param, "@%[^:]:%d ch=%d", type_str, &offset, &channel);
```

**修改为同时支持两种格式**:

```c
static long init_record_ai(aiRecord *prec)
{
    struct link *plink = &prec->inp;
    DevPvt *pPvt;
    char type_str[64];
    int nvals;

    // ... (前面的检查代码)

    // 解析INP字段
    char *param = plink->value.instio.string;

    // 尝试格式1: "@TYPE:offset ch=channel"
    nvals = sscanf(param, "@%[^:]:%d ch=%d",
                   type_str, &pPvt->offset, &pPvt->channel);

    if (nvals == 3) {
        // 成功解析格式1
    } else {
        // 尝试格式2: "@TYPE:offset" (无channel)
        nvals = sscanf(param, "@%[^:]:%d", type_str, &pPvt->offset);
        if (nvals == 2) {
            pPvt->channel = 0;  // 默认channel=0
        } else {
            printf("init_record_ai: Invalid INP format '%s'\n", param);
            free(pPvt);
            return S_db_badField;
        }
    }

    // type_str转换为type代码（如果需要）
    // AMP → type=0, XY → type=1, 等等

    // ... (后续代码)
}
```

**简化方案**: 直接使用`INP="@TEMP:10"`，其中10是offset

---

### 步骤6: 修改.db文件使用新offset

**编辑BPMMonitor.db**:

```bash
vim BPMmonitorApp/Db/BPMMonitor.db
```

**修改Temperature Record的INP字段**:

```
record(ai, "$(P):Temperature")
{
    field(DESC, "FPGA Temperature")
    field(DTYP, "BPMmonitor")
    field(INP,  "@TEMP:10")  # offset=10（温度）
    field(SCAN, "5 second")
    field(PREC, "1")
    field(EGU,  "C")
    field(HOPR, "100")
    field(LOPR, "0")
    field(HIHI, "85")
    field(HIGH, "75")
    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
}
```

**INP字段解析**:
- 设备支持层解析：`@TEMP:10` → offset=10
- 驱动层ReadData(10, ...) → case 10: BPM_ReadTemperature()

---

### 步骤7: 重新编译

```bash
cd /home/user/BPMIOC

# 清理
make clean

# 编译
make

# 检查编译错误
# 如果有错误，仔细阅读并修复
```

**常见编译错误**:
- 语法错误：检查括号、分号
- 未声明的函数：添加函数声明
- 类型不匹配：检查参数类型

---

### 步骤8: 测试温度PV

**启动IOC**:
```bash
cd /home/user/BPMIOC/iocBoot/iocBPMmonitor
./st.cmd
```

**在另一个终端测试**:
```bash
# 读取温度
caget iLinac_007:BPM14And15:Temperature

# 输出示例：
iLinac_007:BPM14And15:Temperature 48.3 C

# 监控温度变化
camonitor iLinac_007:BPM14And15:Temperature

# 输出：
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:15.123 48.3
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:20.456 47.1
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:25.789 49.6
...
```

**验证成功**: 温度值在20-60度范围内随机变化

---

### 步骤9: 添加调试信息

**在ReadData()中添加详细调试**:

```c
float ReadData(int offset, int channel, int type)
{
    float ret = 0.0;

    if (devBPMmonitorDebug > 1) {
        printf("ReadData: offset=%d, channel=%d, type=%d\n",
               offset, channel, type);
    }

    switch (offset) {
        // ... (其他case)

        case 10:  // TEMP offset
            ret = BPM_ReadTemperature();
            if (devBPMmonitorDebug > 0) {
                printf("ReadData: TEMP reading = %.2f C\n", ret);
            }
            break;

        default:
            printf("ReadData: ERROR - Unknown offset %d\n", offset);
            ret = 0.0;
            break;
    }

    if (devBPMmonitorDebug > 2) {
        printf("ReadData: returning %.2f\n", ret);
    }

    return ret;
}
```

**启用调试**:
在IOC Shell中：
```bash
epics> var devBPMmonitorDebug 1

# 观察温度读取的调试输出
ReadData: TEMP reading = 48.30 C
ReadData: TEMP reading = 47.10 C
...
```

---

## 📝 任务2: 添加温度控制功能

### 子任务2.1: 添加风扇控制寄存器

**需求**: 支持写入风扇控制寄存器（offset=20）

**添加SetReg()的case**:

```c
int SetReg(int offset, float value)
{
    if (devBPMmonitorDebug > 0) {
        printf("SetReg: offset=%d, value=%.2f\n", offset, value);
    }

    switch (offset) {
        case 0:  // 原有寄存器
            // ... 现有代码
            break;

        case 20:  // 风扇控制 ← 新添加
            printf("SetReg: Setting fan speed to %.0f%%\n", value);
            // 真实硬件：写入风扇控制寄存器
            // BPM_WriteFanSpeed((int)value);

            // 模拟：只是打印
            if (value < 0 || value > 100) {
                printf("SetReg: ERROR - Fan speed out of range (0-100)\n");
                return -1;
            }
            break;

        default:
            printf("SetReg: Unknown offset %d\n", offset);
            return -1;
    }

    return 0;
}
```

---

### 子任务2.2: 添加风扇控制PV

**在BPMMonitor.db中添加**:

```
record(ao, "$(P):Fan_Speed")
{
    field(DESC, "Fan Speed Control")
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:20")  # offset=20
    field(DRVH, "100")
    field(DRVL, "0")
    field(PREC, "0")
    field(EGU,  "%")
    field(PINI, "YES")
    field(VAL,  "50")  # 初始值50%
}
```

**测试**:
```bash
# 读取当前风扇速度
caget iLinac_007:BPM14And15:Fan_Speed
# 输出：iLinac_007:BPM14And15:Fan_Speed 50 %

# 设置风扇速度
caput iLinac_007:BPM14And15:Fan_Speed 75

# IOC输出：
SetReg: offset=20, value=75.00
SetReg: Setting fan speed to 75%

# 尝试非法值
caput iLinac_007:BPM14And15:Fan_Speed 120
# IOC输出：
SetReg: ERROR - Fan speed out of range (0-100)
```

---

## 🔍 深入理解

### offset-type映射机制

**为什么使用offset？**

```
设计目标：一个ReadData()函数支持多种数据类型

方案：用offset区分数据类型
  offset=0  → RF幅度
  offset=1  → XY位置
  offset=10 → 温度
  offset=20 → 风扇控制
```

**数据流**:
```
.db文件:
  field(INP, "@TEMP:10")
    ↓
设备支持层init_record():
  解析"@TEMP:10" → pPvt->offset = 10
    ↓
设备支持层read_ai():
  ReadData(pPvt->offset, ...) → ReadData(10, ...)
    ↓
驱动层ReadData():
  switch (offset) {
      case 10: return BPM_ReadTemperature();
  }
```

---

### 真实硬件 vs. 模拟

**模拟实现** (当前Lab):
```c
float BPM_ReadTemperature(void)
{
    return 45.0 + (rand() % 200 - 100) / 10.0;
}
```

**真实硬件实现**:
```c
float BPM_ReadTemperature(void)
{
    // 读取FPGA温度传感器寄存器
    uint32_t raw = ReadFPGAReg(TEMP_SENSOR_REG_ADDR);

    // 转换为摄氏度（假设公式）
    float temp = (raw * 503.975 / 4096.0) - 273.15;

    return temp;
}
```

**Mock库实现** (使用simulator/):
```c
float BPM_ReadTemperature(void)
{
    return ReadTemperature(0);  // 调用Mock库
}
```

---

## ⚠️ 常见错误

### 错误1: offset冲突

**现象**: 温度值是RF幅度

**原因**: offset=10已被其他类型使用

**解决**: 选择未使用的offset

---

### 错误2: 未重新编译

**现象**: 修改代码后PV值未变化

**原因**: 忘记重新编译和重启IOC

**解决**: `make clean && make && 重启IOC`

---

### 错误3: 函数未声明

**编译错误**:
```
driverWrapper.c: undefined reference to 'BPM_ReadTemperature'
```

**原因**: 函数声明和定义位置不对

**解决**: 确保函数在调用前声明

---

### 错误4: 数据类型不匹配

**编译警告**:
```
warning: implicit conversion from 'double' to 'float'
```

**解决**: 使用正确的类型cast
```c
float noise = (float)(rand() % 200 - 100) / 10.0f;
```

---

## ✅ 学习检查点

- [ ] 理解驱动层在三层架构中的作用
- [ ] 能够修改ReadData()函数
- [ ] 理解offset映射机制
- [ ] 能够添加新的硬件函数
- [ ] 能够修改SetReg()支持写入
- [ ] 理解真实硬件和模拟的区别
- [ ] 能够添加调试信息

---

## 🚀 扩展挑战

### 挑战1: 添加电压监控

添加多个电压监控点：

```c
// 添加函数
float BPM_ReadVoltage(int channel)
{
    float voltages[] = {3.3, 5.0, 12.0, -12.0};
    if (channel < 0 || channel >= 4) return 0.0;

    // 添加小波动
    return voltages[channel] + (rand() % 100 - 50) / 1000.0;
}

// ReadData添加case
case 11:  // VOLTAGE offset
    ret = BPM_ReadVoltage(channel);
    break;
```

.db文件：
```
record(ai, "$(P):Voltage_3V3") {
    field(INP, "@VOLT:11 ch=0")
    field(PREC, "3")
    field(EGU, "V")
}
record(ai, "$(P):Voltage_5V") {
    field(INP, "@VOLT:11 ch=1")
    field(PREC, "3")
    field(EGU, "V")
}
```

---

### 挑战2: 实现数据缓存

优化ReadData()，避免重复读取硬件：

```c
typedef struct {
    float value;
    time_t timestamp;
} CachedData;

CachedData temp_cache = {0.0, 0};

float BPM_ReadTemperature_Cached(void)
{
    time_t now = time(NULL);

    // 缓存有效期：5秒
    if (now - temp_cache.timestamp < 5) {
        return temp_cache.value;  // 返回缓存值
    }

    // 读取新值
    temp_cache.value = BPM_ReadTemperature();
    temp_cache.timestamp = now;

    return temp_cache.value;
}
```

---

### 挑战3: 添加错误处理

完善ReadData()的错误处理：

```c
float ReadData(int offset, int channel, int type)
{
    float ret = 0.0;
    int error = 0;

    // 参数检查
    if (channel < 0 || channel > 15) {
        printf("ReadData: ERROR - Invalid channel %d\n", channel);
        return -999.0;  // 错误值
    }

    switch (offset) {
        case 10:  // TEMP offset
            ret = BPM_ReadTemperature();

            // 合理性检查
            if (ret < -40.0 || ret > 125.0) {
                printf("ReadData: ERROR - Temperature out of range: %.2f\n", ret);
                error = 1;
                ret = -999.0;
            }
            break;

        // ...
    }

    return ret;
}
```

---

## 📚 相关知识

- **Part 4.7**: ReadData详解
- **Part 4.8**: SetReg详解
- **Part 4.13**: 故障排查
- **Lab 3**: 添加调试信息
- **Lab 8**: 添加新Record类型支持

---

## 🎯 总结

本实验学习了如何修改驱动层支持新功能：

1. **添加硬件函数**: `BPM_ReadTemperature()`
2. **修改ReadData()**: 添加新的offset case
3. **修改SetReg()**: 支持写入控制寄存器
4. **配合.db文件**: 使用新的offset

**关键理解**:
- 驱动层是硬件访问的抽象层
- offset机制实现了灵活的数据类型映射
- 修改驱动层需要同步修改.db文件

**下一步**: Lab 8学习如何在设备支持层添加新Record类型支持！

---

**恭喜完成Lab 7！** 你已经掌握了驱动层修改技能！ 💪
