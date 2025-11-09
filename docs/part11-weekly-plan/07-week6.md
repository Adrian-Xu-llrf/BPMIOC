# Week 6: 数据库进阶（Database Layer）

> **学习目标**: 掌握.db文件设计，理解Record配置
> **关键词**: .db文件、Record字段、PV设计
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 15-20小时

## 📅 本周概览

```
周一-周三: 理论学习
  ├─ Part 6文档阅读
  ├─ .db文件语法学习
  └─ PV设计原则

周四-周五: 动手实践
  ├─ Lab 9: 设计新的PV结构
  ├─ Lab 10: 完整功能实现
  └─ 数据库优化

周末: 复习巩固
  ├─ PV设计总结
  └─ 准备Week 7
```

---

## 🎯 Week 6学习目标

### 知识目标
- [ ] 理解.db文件的语法和结构
- [ ] 掌握ai/ao/longin/longout/waveform等Record类型
- [ ] 理解INP/OUT/SCAN/FLNK等字段的作用
- [ ] 掌握Record链接机制（CP/PP/MS）
- [ ] 理解如何设计PV结构

### 技能目标
- [ ] 能独立设计一组完整的PV
- [ ] 能为新功能设计数据库配置
- [ ] 能优化PV更新效率
- [ ] 能调试数据库配置问题

---

## 📚 Day-by-Day Schedule

### Day 1-2 - .db文件语法

**学习内容**:
- ✅ Part 6: 01-overview.md
- ✅ Part 6: 02-record-types.md
- ✅ 分析BPMMonitor.db文件

**Record基本语法**:
```
record(ai, "BPM:01:RF3:Amp") {
    field(DTYP, "BPM Monitor")    # 设备类型
    field(INP,  "@0 3 0")         # 输入链接
    field(SCAN, "I/O Intr")       # 扫描机制
    field(EGU,  "V")              # 工程单位
    field(PREC, "3")              # 精度
    field(FLNK, "BPM:01:RF3:RMS") # 前向链接
}
```

---

### Day 3-4 - Lab 9: PV设计

**实验**: 为RF功率监控设计完整PV

**需求**:
- 监控4个RF通道的功率（计算值：Amp²）
- 功率超过阈值时报警
- 记录历史最大值

**设计**:
```
# 1. 功率计算（calc record）
record(calc, "BPM:01:RF3:Power") {
    field(CALC, "A*A")
    field(INPA, "BPM:01:RF3:Amp CP")
    field(EGU,  "W")
    field(PREC, "3")
    field(SCAN, "Passive")
    field(FLNK, "BPM:01:RF3:PowerCheck")
}

# 2. 阈值检查（ai + HIHI alarm）
record(ai, "BPM:01:RF3:PowerCheck") {
    field(INP,  "BPM:01:RF3:Power MS")
    field(HIHI, "10.0")
    field(HHSV, "MAJOR")
    field(HIGH, "8.0")
    field(HSV,  "MINOR")
}

# 3. 历史最大值（sel record）
record(sel, "BPM:01:RF3:PowerMax") {
    field(INPA, "BPM:01:RF3:Power CP")
    field(INPB, "BPM:01:RF3:PowerMax")
    field(SELM, "High Signal")
}
```

---

### Day 5 - Lab 10: 完整功能实现

**实验**: 实现波形平均功能

```
record(waveform, "BPM:01:RF3:WaveRaw") {
    field(DTYP, "BPM Monitor")
    field(INP,  "@6 0 0")
    field(FTVL, "FLOAT")
    field(NELM, "10000")
    field(SCAN, ".1 second")
}

record(aSub, "BPM:01:RF3:WaveAvg") {
    field(SNAM, "waveformAverage")
    field(INPA, "BPM:01:RF3:WaveRaw")
    field(FTA,  "FLOAT")
    field(NOA,  "10000")
    field(OUTA, "BPM:01:RF3:WaveAvgData PP")
}
```

---

## ✅ Week 6检查点

**能力测试**:
- [ ] 能设计一组完整的PV（5个以上Record）
- [ ] 能使用calc/sel/aSub等高级Record
- [ ] 能配置报警阈值
- [ ] 能实现Record间的链接

---

## 🎯 下一步

👉 [08-week7.md](./08-week7.md) - Week 7: 综合开发

---

**Week 6加油！** 数据库层是用户直接看到的界面，设计好PV结构非常重要！ 🎨
