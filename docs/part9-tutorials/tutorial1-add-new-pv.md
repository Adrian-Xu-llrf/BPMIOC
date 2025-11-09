# Tutorial 1: 添加新PV - 信噪比监控

> **难度**: ⭐⭐⭐
> **时间**: 1周（10-15小时）
> **前置**: Part 4-8

## 📖 项目概述

### 目标

为BPMIOC添加一个新的PV: `$(P):RFIn_01_SNR`，用于监控RF输入通道1的信噪比（Signal-to-Noise Ratio）。

### 需求

1. **功能需求**:
   - 实时计算RF通道1的SNR
   - SNR单位：dB
   - 更新频率：1秒
   - 报警：SNR < 20dB时触发MINOR报警

2. **技术需求**:
   - 修改数据库层（.db文件）
   - 修改驱动层（添加SNR计算）
   - 无需修改设备支持层（复用现有）
   - 支持Mock库模拟

---

## 🎯 学习目标

完成本教程后，你将学会：

- ✅ 端到端添加新PV的完整流程
- ✅ 在驱动层实现数据处理逻辑
- ✅ 配置Record字段（报警、精度、单位）
- ✅ 使用Mock库模拟新数据
- ✅ 测试和验证新功能

---

## 📊 架构设计

### 数据流

```
RF ADC数据（幅度）
    ↓
驱动层计算SNR
    ↓
ReadData(offset=SNR_OFFSET)
    ↓
设备支持层read_ai()
    ↓
Record: $(P):RFIn_01_SNR
    ↓
CA客户端显示
```

### SNR计算公式

```
SNR (dB) = 20 * log10(Signal_Amp / Noise_Amp)

其中：
- Signal_Amp: RF信号幅度（从offset=0读取）
- Noise_Amp: 假设为固定值或测量值（简化为0.1V）
```

---

## 🔧 实现步骤

### 步骤1: 定义新的offset

**编辑driverWrapper.c**:

```c
// 在文件顶部添加新的offset定义
#define OFFSET_AMP   0
#define OFFSET_XY    1
#define OFFSET_PHS   2
#define OFFSET_BTN   3
#define OFFSET_SNR   20  // 新增：SNR offset
```

---

### 步骤2: 实现SNR计算函数

**在driverWrapper.c中添加**:

```c
/**
 * @brief 计算RF信号的信噪比
 * @param channel RF通道号（0-7）
 * @return SNR值（dB）
 */
float CalculateSNR(int channel)
{
    float signal_amp, noise_amp, snr_db;
    
    // 读取信号幅度
    signal_amp = BPM_RFIn_ReadADC(channel, 0);
    
    // 噪声幅度（简化：假设为固定值）
    // 实际应用中可能需要测量或估算
    noise_amp = 0.1;  // 假设噪声为0.1V
    
    // 避免除零
    if (noise_amp < 0.001) {
        noise_amp = 0.001;
    }
    
    // 计算SNR (dB)
    snr_db = 20.0 * log10(signal_amp / noise_amp);
    
    if (devBPMmonitorDebug > 0) {
        printf("CalculateSNR: ch=%d, signal=%.3f, noise=%.3f, SNR=%.2f dB\n",
               channel, signal_amp, noise_amp, snr_db);
    }
    
    return snr_db;
}
```

**注意**: 需要包含`<math.h>`并链接`libm`

---

### 步骤3: 修改ReadData函数

**在driverWrapper.c的ReadData()中添加case**:

```c
float ReadData(int offset, int channel, int type)
{
    float ret = 0.0;

    switch (offset) {
        case OFFSET_AMP:  // 0
            ret = BPM_RFIn_ReadADC(channel, type);
            break;

        case OFFSET_XY:   // 1
            ret = BPM_XY_Read(channel);
            break;

        case OFFSET_PHS:  // 2
            ret = BPM_RFIn_ReadADC(channel, type);
            break;

        case OFFSET_BTN:  // 3
            ret = BPM_BTN_Read(channel);
            break;

        case OFFSET_SNR:  // 20 - 新增
            ret = CalculateSNR(channel);
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

### 步骤4: 修改Makefile添加数学库

**编辑BPMmonitorApp/src/Makefile**:

```makefile
# 系统库依赖
BPMmonitor_SYS_LIBS += dl
BPMmonitor_SYS_LIBS += pthread
BPMmonitor_SYS_LIBS += m        # 新增：libm（数学库，提供log10）
```

---

### 步骤5: 添加SNR Record到数据库

**编辑BPMmonitorApp/Db/BPMMonitor.db**，在文件末尾添加：

```
#================================================
# 信噪比监控
#================================================

record(ai, "$(P):RFIn_01_SNR")
{
    field(DESC, "RF Input 1 Signal-to-Noise Ratio")
    field(DTYP, "BPMmonitor")
    field(INP,  "@SNR:20 ch=0")     # offset=20, channel=0
    field(SCAN, "1 second")         # 每秒更新
    field(PREC, "2")                # 2位小数
    field(EGU,  "dB")               # 单位：分贝
    field(HOPR, "60")               # 显示上限
    field(LOPR, "0")                # 显示下限
    
    # 报警配置
    field(LOW,  "20")               # 低限：20 dB
    field(LOLO, "10")               # 低低限：10 dB
    field(LSV,  "MINOR")            # 低限报警级别
    field(LLSV, "MAJOR")            # 低低限报警级别
    
    field(PINI, "YES")              # 初始化时处理
}

# 可选：为所有8个通道添加SNR
record(ai, "$(P):RFIn_02_SNR")
{
    field(DESC, "RF Input 2 SNR")
    field(DTYP, "BPMmonitor")
    field(INP,  "@SNR:20 ch=1")
    field(SCAN, "1 second")
    field(PREC, "2")
    field(EGU,  "dB")
    field(LOW,  "20")
    field(LSV,  "MINOR")
    field(PINI, "YES")
}

# ... 其他通道类似
```

---

### 步骤6: 更新Mock库支持SNR（可选）

**如果使用simulator/，编辑simulator/src/mock_bpm.c**:

```c
// 添加SNR模拟函数（可选）
float MockCalculateSNR(int channel)
{
    // 模拟SNR：25-35 dB范围随机波动
    float base_snr = 30.0;
    float variation = (rand() % 200 - 100) / 20.0;  // ±5 dB
    return base_snr + variation;
}
```

---

### 步骤7: 编译

```bash
cd /home/user/BPMIOC

# 清理旧编译产物
make clean

# 重新编译
make

# 检查编译输出
# 应该看到：
# Compiling driverWrapper.c
# Linking BPMmonitor
```

**常见编译错误**:

1. **未定义log10**:
   ```
   undefined reference to 'log10'
   ```
   **解决**: 确保添加了`BPMmonitor_SYS_LIBS += m`

2. **未包含math.h**:
   ```
   implicit declaration of function 'log10'
   ```
   **解决**: 在driverWrapper.c顶部添加`#include <math.h>`

---

### 步骤8: 测试

**启动IOC**:

```bash
cd /home/user/BPMIOC/iocBoot/iocBPMmonitor
./st.cmd
```

**在另一个终端测试**:

```bash
# 1. 检查PV是否存在
dbl | grep SNR

# 输出应包含：
# iLinac_007:BPM14And15:RFIn_01_SNR
# iLinac_007:BPM14And15:RFIn_02_SNR

# 2. 读取SNR值
caget iLinac_007:BPM14And15:RFIn_01_SNR

# 输出示例：
# iLinac_007:BPM14And15:RFIn_01_SNR 28.45 dB

# 3. 监控SNR变化
camonitor -t iLinac_007:BPM14And15:RFIn_01_SNR

# 输出：
# 2024-11-09 12:30:01.123 iLinac_007:BPM14And15:RFIn_01_SNR 28.45
# 2024-11-09 12:30:02.125 iLinac_007:BPM14And15:RFIn_01_SNR 29.12
# 2024-11-09 12:30:03.127 iLinac_007:BPM14And15:RFIn_01_SNR 27.89

# 4. 测试报警（如果SNR降到20以下）
caget -t iLinac_007:BPM14And15:RFIn_01_SNR

# 如果SNR < 20：
# iLinac_007:BPM14And15:RFIn_01_SNR 2024-11-09 12:30:05.123 18.23 LOW MINOR
```

---

### 步骤9: 调试（如果需要）

**启用调试信息**:

在IOC Shell中：
```bash
epics> var devBPMmonitorDebug 1
```

**观察输出**:
```
CalculateSNR: ch=0, signal=5.234, noise=0.100, SNR=34.37 dB
ReadData: offset=20, channel=0, returning 34.37
```

---

## 📊 验证清单

### 功能验证

- [ ] PV存在于数据库（`dbl | grep SNR`）
- [ ] 能够读取SNR值（`caget`）
- [ ] SNR值在合理范围内（15-40 dB）
- [ ] 每秒更新一次（`camonitor -t`）
- [ ] 单位显示正确（dB）
- [ ] 精度为2位小数

### 报警验证

- [ ] SNR < 20 dB时触发LOW MINOR报警
- [ ] SNR < 10 dB时触发LOLO MAJOR报警
- [ ] 报警恢复正常（SNR回升时）

### 代码质量

- [ ] 添加了足够的注释
- [ ] 代码遵循项目风格
- [ ] 无编译警告
- [ ] 无内存泄漏（如果使用动态内存）

---

## 🚀 扩展挑战

### 挑战1: 动态噪声估算

替换固定噪声值，实现真实的噪声测量：

```c
float EstimateNoise(int channel)
{
    // 读取多个采样点
    float samples[100];
    for (int i = 0; i < 100; i++) {
        samples[i] = BPM_RFIn_ReadADC(channel, 0);
        usleep(100);  // 100us间隔
    }
    
    // 计算标准差作为噪声估计
    float mean = 0.0, variance = 0.0;
    for (int i = 0; i < 100; i++) {
        mean += samples[i];
    }
    mean /= 100.0;
    
    for (int i = 0; i < 100; i++) {
        variance += (samples[i] - mean) * (samples[i] - mean);
    }
    variance /= 100.0;
    
    return sqrt(variance);
}
```

---

### 挑战2: SNR历史记录

使用waveform Record记录SNR历史：

```
record(waveform, "$(P):RFIn_01_SNR_History")
{
    field(DTYP, "Soft Channel")
    field(FTVL, "FLOAT")
    field(NELM, "3600")  # 1小时历史（1秒间隔）
}

# 使用subArray或Python脚本定期更新
```

---

### 挑战3: 多通道SNR平均

添加calc Record计算所有通道的平均SNR：

```
record(calc, "$(P):SNR_Average")
{
    field(SCAN, "1 second")
    field(INPA, "$(P):RFIn_01_SNR CP")
    field(INPB, "$(P):RFIn_02_SNR CP")
    field(INPC, "$(P):RFIn_03_SNR CP")
    field(INPD, "$(P):RFIn_04_SNR CP")
    field(CALC, "(A+B+C+D)/4")
    field(PREC, "2")
    field(EGU,  "dB")
}
```

---

## 📚 相关知识

### SNR的重要性

**信噪比（SNR）**是衡量信号质量的重要指标：

- **高SNR（>30 dB）**: 信号清晰，噪声低
- **中等SNR（20-30 dB）**: 可接受的信号质量
- **低SNR（<20 dB）**: 信号质量差，可能影响测量精度

### 在加速器中的应用

在BPM（Beam Position Monitor）系统中：
- SNR影响位置测量精度
- 低SNR可能表示：
  - 束流强度低
  - 探测器性能下降
  - 电缆或连接器问题
  - 电磁干扰

---

## ✅ 学习成果

完成本教程后，你已经掌握了：

1. **端到端PV添加流程**:
   - 驱动层修改（添加offset和计算函数）
   - 数据库配置（添加Record）
   - Makefile修改（添加库依赖）

2. **数据处理实现**:
   - 在驱动层实现计算逻辑
   - 使用数学库函数（log10）
   - 错误处理和边界检查

3. **测试和验证**:
   - 使用CA工具测试
   - 验证功能和报警
   - 调试技巧

4. **实际项目经验**:
   - 需求分析
   - 设计实现
   - 测试部署

---

## 🎯 总结

### 关键点

1. **Offset机制**: 通过offset区分不同数据类型
2. **数据流**: 驱动层→设备支持层→Record→CA
3. **配置与代码**: .db文件（配置）+ .c文件（实现）

### 下一步

完成Tutorial 1后，你可以：
- 继续Tutorial 2，学习更复杂的offset扩展
- 应用相同方法添加其他PV（如THDI、峰峰值等）
- 优化SNR计算算法

---

**恭喜完成Tutorial 1！** 你已掌握端到端添加新PV的完整流程！💪

**下一个教程**: Tutorial 2 - 实现新的Offset类型，学习更深入的架构扩展！
