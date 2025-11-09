# Tutorial 3: 添加新硬件通道 - 扩展到10个RF输入

> **难度**: ⭐⭐⭐⭐
> **时间**: 1周（15-20小时）
> **前置**: Tutorial 1-2, Part 3-4

## 📖 项目概述

### 目标

将BPMIOC从8个RF输入通道扩展到10个，涵盖全链路修改。

### 需求

1. **功能需求**:
   - 支持RF输入通道9和10
   - 所有现有功能（幅度、相位、XY位置）都支持
   - 数据缓冲区扩展

2. **技术需求**:
   - 修改全局缓冲区大小
   - 批量添加PV
   - 更新INP/OUT通道范围

---

## 🔧 实现步骤

### 步骤1: 扩展全局缓冲区

**driverWrapper.c**:

```c
// 修改通道数定义
#define MAX_RF_CHANNELS  10  // 从8改为10

// 扩展缓冲区
float g_RFIn_Amp[MAX_RF_CHANNELS];
float g_RFIn_Phs[MAX_RF_CHANNELS];
```

---

### 步骤2: 批量添加PV

**BPMMonitor.db**:

```bash
# 使用脚本生成（或手动复制）
for i in 09 10; do
cat >> BPMMonitor.db << EOF
record(ai, "\$(P):RFIn_${i}_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=$((10#$i - 1))")
    field(PREC, "3")
    field(EGU,  "V")
}
EOF
done
```

---

### 步骤3: 验证数据采集

```c
// driverWrapper.c中验证通道号
if (channel < 0 || channel >= MAX_RF_CHANNELS) {
    printf("ERROR: Invalid channel %d\n", channel);
    return 0.0;
}
```

---

### 步骤4: 测试所有通道

```bash
# 测试通道9和10
caget iLinac_007:BPM14And15:RFIn_09_Amp
caget iLinac_007:BPM14And15:RFIn_10_Amp

# 监控所有通道
for i in {01..10}; do
    echo "Channel $i:"
    caget iLinac_007:BPM14And15:RFIn_${i}_Amp
done
```

---

## ✅ 学习成果

- ✅ 掌握了全链路修改流程
- ✅ 理解了缓冲区管理
- ✅ 学会了批量PV配置

**下一个教程**: Tutorial 4 - 从零构建温度监控IOC！
