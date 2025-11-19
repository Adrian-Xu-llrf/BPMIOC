# Week 4 - 完整项目实战

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 完成一个完整的 BPM IOC 项目

---

## Day 1-2: 项目规划（4小时）

### 项目需求

实现一个完整的 BPM 监控系统：

**功能需求**：
1. 4通道幅度采集（ai record）
2. 4通道相位采集（ai record）
3. 波形数据采集（waveform record）
4. X/Y 位置计算（calc record）
5. 增益/偏置设置（ao record）
6. I/O 中断驱动
7. 日志和报警

### 项目结构

```
BPMComplete/
├── configure/
├── BPMCompleteApp/
│   ├── Db/
│   │   ├── BPM.db
│   │   ├── BPMCalc.db
│   │   └── Makefile
│   ├── src/
│   │   ├── devBPMMonitor.c
│   │   ├── devBPMMonitor.h
│   │   ├── drvBPMWrapper.c
│   │   ├── BPMComplete.dbd
│   │   └── Makefile
│   └── Makefile
├── iocBoot/
│   └── iocBPMComplete/
│       ├── st.cmd
│       └── envPaths
└── Makefile
```

---

## Day 3-5: 代码实现（6小时）

### 1. 数据库文件（BPM.db）

```
# === 原始数据采集 ===
record(ai, "BPM:CH$(CH):Amp") {
    field(DESC, "Channel $(CH) Amplitude")
    field(DTYP, "BPMmonitor")
    field(INP, "@AMP:0 ch=$(CH)")
    field(SCAN, "I/O Intr")
    field(EGU, "V")
    field(PREC, "4")
    field(HIHI, "1.0")
    field(HIGH, "0.9")
    field(LOW, "0.1")
    field(LOLO, "0.05")
    field(HHSV, "MAJOR")
    field(HSV, "MINOR")
    field(LSV, "MINOR")
    field(LLSV, "MAJOR")
}

record(ai, "BPM:CH$(CH):Phase") {
    field(DESC, "Channel $(CH) Phase")
    field(DTYP, "BPMmonitor")
    field(INP, "@PHASE:0 ch=$(CH)")
    field(SCAN, "I/O Intr")
    field(EGU, "deg")
    field(PREC, "2")
}

record(waveform, "BPM:CH$(CH):Wave") {
    field(DESC, "Channel $(CH) Waveform")
    field(DTYP, "BPMmonitor")
    field(INP, "@WAVE:0 ch=$(CH)")
    field(FTVL, "FLOAT")
    field(NELM, "1024")
    field(SCAN, "I/O Intr")
}
```

### 2. 计算数据库（BPMCalc.db）

```
# X 位置计算
record(calc, "BPM:X:Pos") {
    field(DESC, "X Position")
    field(CALC, "(A-B)/(A+B)*D")
    field(INPA, "BPM:CH0:Amp CP MS")
    field(INPB, "BPM:CH2:Amp CP MS")
    field(INPD, "BPM:X:Scale")
    field(EGU, "mm")
    field(PREC, "3")
}

# Y 位置计算
record(calc, "BPM:Y:Pos") {
    field(DESC, "Y Position")
    field(CALC, "(A-B)/(A+B)*D")
    field(INPA, "BPM:CH1:Amp CP MS")
    field(INPB, "BPM:CH3:Amp CP MS")
    field(INPD, "BPM:Y:Scale")
    field(EGU, "mm")
    field(PREC, "3")
}

# 刻度因子
record(ao, "BPM:X:Scale") {
    field(DESC, "X Scale Factor")
    field(VAL, "10.0")
    field(EGU, "mm")
    field(PREC, "2")
}

record(ao, "BPM:Y:Scale") {
    field(DESC, "Y Scale Factor")
    field(VAL, "10.0")
    field(EGU, "mm")
    field(PREC, "2")
}

# 总功率
record(calc, "BPM:TotalPower") {
    field(DESC, "Total Power")
    field(CALC, "A+B+C+D")
    field(INPA, "BPM:CH0:Amp CP MS")
    field(INPB, "BPM:CH1:Amp CP MS")
    field(INPC, "BPM:CH2:Amp CP MS")
    field(INPD, "BPM:CH3:Amp CP MS")
    field(EGU, "V")
    field(PREC, "4")
}
```

### 3. Device Support（参考 Week2）

### 4. Driver Support（参考 Week3）

---

## Day 6: 测试和调试（2小时）

### 编译

```bash
$ cd BPMComplete
$ make clean
$ make
```

### 运行 IOC

```bash
$ cd iocBoot/iocBPMComplete
$ ./st.cmd
```

### 测试命令

```bash
# 1. 读取所有通道
$ caget BPM:CH{0,1,2,3}:Amp

# 2. 读取位置
$ caget BPM:X:Pos BPM:Y:Pos

# 3. 监控
$ camonitor BPM:CH0:Amp BPM:X:Pos

# 4. 设置刻度
$ caput BPM:X:Scale 15.0

# 5. 读取波形
$ caget -# 1024 BPM:CH0:Wave
```

---

## Day 7: 总结和优化（2小时）

### 性能优化

1. **减少CPU占用**
   - 优化扫描频率
   - 使用 I/O Intr 而非周期扫描

2. **内存优化**
   - 合理设置 waveform NELM
   - 及时释放资源

3. **网络优化**
   - 使用 CP MS 而非 CP
   - 减少不必要的链接

### 错误处理

```c
// 在 Device Support 中
static long read_ai(aiRecord *prec) {
    DevicePrivate *pPvt = (DevicePrivate *)prec->dpvt;

    if (!pPvt) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }

    float value = ReadHardware(pPvt->channel);

    if (value < 0) {
        recGblSetSevr(prec, HW_LIMIT_ALARM, MAJOR_ALARM);
        return -1;
    }

    prec->val = value;
    prec->udf = FALSE;
    return 2;
}
```

### 添加日志

```c
#include "errlog.h"

errlogPrintf("BPM: Channel %d read error\n", channel);
```

---

## 最终检查清单

- [ ] 所有 PV 可以正常读写
- [ ] I/O 中断工作正常
- [ ] 计算结果正确
- [ ] 报警功能正常
- [ ] 无内存泄漏
- [ ] 无编译警告
- [ ] 代码有注释
- [ ] 有 README 文档

---

## 完整学习路径总结

### Month 1: C 语言基础
- ✅ 基础语法
- ✅ 指针
- ✅ 结构体和函数
- ✅ 综合练习

### Month 2: Linux 和 EPICS
- ✅ Linux 基础命令
- ✅ 编译工具和 Makefile
- ✅ EPICS Base 安装
- ✅ 第一个 IOC

### Month 3: IOC 开发
- ✅ Database 文件
- ✅ Device Support
- ✅ Driver Support
- ✅ 完整项目

---

## 下一步学习建议

1. **深入学习**
   - 研究 BPMIOC 完整代码
   - 理解 EPICS 源码
   - 学习高级 Record 类型

2. **实践项目**
   - 连接真实硬件
   - 开发自己的应用
   - 参与开源项目

3. **扩展知识**
   - 学习 Python EPICS 接口
   - 学习 EDM/CS-Studio GUI
   - 学习 Archiver

---

## 恭喜！

你已经完成了从零基础到 EPICS IOC 开发的完整学习路径！

现在你可以：
- 💻 阅读和理解 EPICS 代码
- 🔧 开发自己的 Device/Driver Support
- 🚀 创建完整的 IOC 应用
- 📊 集成 FPGA 硬件到 EPICS 系统

继续保持学习和实践！🎉
