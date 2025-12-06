# 波形平均电压计算模块说明

**作者**: [你的名字]
**日期**: 2025-12-06
**目的**: 计算去除本底的波形平均电压

---

## 📋 模块概述

这是一个**独立的新增功能模块**，用于计算各通道波形的平均电压（去除本底）。该模块已经与原有的 `driverWrapper.c` 代码**完全隔离**，不会影响师兄的原有代码逻辑。

---

## 📁 新增文件

### 1. `voltageCalculator.h`
- **功能**: 模块头文件，定义公共接口
- **类型**: 全新文件
- **依赖**: 无外部依赖

### 2. `voltageCalculator.c`
- **功能**: 模块实现文件，包含所有平均电压计算逻辑
- **类型**: 全新文件
- **依赖**: 仅依赖标准库 `<stdio.h>`

### 3. `VOLTAGE_CALCULATOR_README.md`
- **功能**: 本说明文档
- **类型**: 全新文件

---

## 🔧 修改的文件

### 1. `driverWrapper.c` (师兄的代码)
**所有修改都添加了清晰的注释标记，便于识别**

#### 修改位置汇总：
1. **第 23 行** - 添加头文件包含
   ```c
   #include "voltageCalculator.h"  /* 新增模块：波形平均电压计算（你的代码） */
   ```

2. **第 111-112 行** - 移除新增的静态变量（已移至新模块）
   ```c
   /* ========== 新增变量已移至 voltageCalculator.c 模块 ========== */
   /* 移除的变量：BackGroundStart, BackGroundStop, rf3~rf10_avg_volt */
   ```

3. **第 205 行** - 移除函数声明（已移至新模块）
   ```c
   /* ========== 新增函数声明已移至 voltageCalculator.h 模块 ========== */
   ```

4. **第 288-289 行** - 在初始化函数中调用模块初始化
   ```c
   /* ========== 初始化新增的电压计算模块（你的代码） ========== */
   voltCalc_init();
   ```

5. **第 552-554 行** - ReadData 函数中的 case 34
   ```c
   /* ========== 新增：读取平均电压（你的代码） ========== */
   case 34:
       return voltCalc_getAvgVoltage(channel);
   ```

6. **第 672-677 行** - SetReg 函数中同步 AVGStart/AVGStop
   ```c
   case 20:
       AVGStart = val_tmp;
       voltCalc_setSignalStart(val_tmp);  /* 同步到新模块 */
       break;
   case 21:
       AVGStop = val_tmp;
       voltCalc_setSignalStop(val_tmp);  /* 同步到新模块 */
       break;
   ```

7. **第 692-698 行** - SetReg 函数中的 case 27 和 28
   ```c
   /* ========== 新增：设置本底范围（你的代码） ========== */
   case 27:
       voltCalc_setBackgroundStart(val_tmp);
       break;
   case 28:
       voltCalc_setBackgroundStop(val_tmp);
       break;
   ```

8. **第 747-787 行** - readWaveform 函数中的计算调用
   ```c
   /* ========== 新增：在读取波形时计算平均电压（你的代码） ========== */
   case 11-18:
       // 每个 case 中调用 voltCalc_calculateAvgVoltage()
   ```

9. **第 1482-1485 行** - 文件末尾说明
   ```c
   /* 注意：calculateAvgVoltage 函数已经移到 voltageCalculator.c 模块中 */
   /* 这是你新增的功能，现在已经与原有代码隔离 */
   ```

### 2. `Makefile`
**第 34 行** - 添加新模块到编译列表
```makefile
BPMmonitor_SRCS += voltageCalculator.c  # 新增模块：波形平均电压计算
```

---

## ✅ 对师兄代码的影响评估

### 完全不影响的部分（99%的代码）
- ✅ 所有原有的函数逻辑
- ✅ 所有原有的变量
- ✅ 所有原有的数据流
- ✅ BPM位置计算
- ✅ 波形采集
- ✅ 历史数据
- ✅ 触发逻辑
- ✅ 其他所有现有功能

### 轻微触碰的部分（接口调用）
在 `driverWrapper.c` 中只有以下几处**接口级别**的调用：
1. 初始化时调用 `voltCalc_init()` - 不影响原有初始化逻辑
2. 读取波形时调用 `voltCalc_calculateAvgVoltage()` - 仅计算新增的平均电压值
3. 设置参数时调用 `voltCalc_setXXX()` - 仅设置新增模块的参数

**这些调用都是单向的（从 driverWrapper 调用新模块），不会反向影响原有逻辑。**

---

## 🎯 功能说明

### 核心功能
计算每个通道（RF3-RF10）的平均电压，公式为：
```
平均电压 = 信号区平均值 - 本底区平均值
```

### 参数设置
- **信号区范围**: 通过 PV `SetAvgStartPosition` 和 `SetAvgStopPosition` 设置
- **本底区范围**: 通过 PV `SetBaselineStartPosition` 和 `SetBaselineStopPosition` 设置

### 结果读取
通过 PV 读取各通道的平均电压：
- BPM1: `Va1p_volt_avg`, `Vb1p_volt_avg`, `Vc1p_volt_avg`, `Vd1p_volt_avg`
- BPM2: `Va2p_volt_avg`, `Vb2p_volt_avg`, `Vc2p_volt_avg`, `Vd2p_volt_avg`

---

## 🛡️ 如何向师兄保证代码安全

### 1. 代码隔离保证
- ✅ 所有新增逻辑都在独立的 `.c/.h` 文件中
- ✅ 对师兄代码的修改仅限于**明确标记的接口调用**
- ✅ 没有修改任何原有功能的逻辑
- ✅ 所有修改都有清晰的中文注释标记

### 2. 回退容易
如果需要移除这个功能，只需：
1. 删除 `voltageCalculator.c` 和 `voltageCalculator.h`
2. 在 `driverWrapper.c` 中删除所有带 `/* ========== 新增 ... ========== */` 注释的代码
3. 从 `Makefile` 中移除 `voltageCalculator.c` 这一行
4. 从数据库文件中删除 `BPMCal.db`

### 3. 测试建议
在师兄的系统上测试时：
1. 先备份原有代码
2. 编译新代码
3. 运行IOC，验证原有功能不受影响
4. 测试新增的平均电压功能

---

## 📊 代码统计

| 项目 | 数量 |
|------|------|
| 新增文件 | 3 个 |
| 修改文件 | 2 个 |
| 新增代码行数 | ~150 行（独立文件中） |
| 师兄代码修改行数 | ~20 行（仅接口调用） |
| 师兄代码删除行数 | ~60 行（移至新模块） |

---

## 📝 版本历史

| 日期 | 版本 | 说明 |
|------|------|------|
| 2025-12-06 | v1.0 | 完成代码隔离，创建独立模块 |
| 之前 | v0.x | 功能代码直接写在 driverWrapper.c 中 |

---

## 👨‍💻 联系方式

如果师兄有任何疑问或需要说明，可以：
1. 查看所有带注释标记的修改
2. 对比新旧版本的差异
3. 直接联系我进行说明

---

**总结**: 这个模块是完全独立的，对师兄的原有代码影响**最小化**，且所有修改都有清晰标记，方便审查和维护。
