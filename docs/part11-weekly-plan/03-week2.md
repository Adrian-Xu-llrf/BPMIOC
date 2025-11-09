# Week 2: EPICS基础

> **时间**: 第2周（15-20小时）
> **目标**: 深入理解EPICS核心概念
> **难度**: ⭐⭐⭐☆☆

## 📅 本周目标

完成本周后，你应该能够：
- ✅ 理解EPICS的核心概念（PV、Record、CA）
- ✅ 掌握常用的Channel Access工具
- ✅ 理解Record类型和字段
- ✅ 完成基础实验2-3

## 📚 学习内容

### Day 1-2: EPICS核心概念（理论）

**学习材料**：
- Part 2: 01-what-is-epics.md
- Part 2: 02-key-concepts.md
- Part 2: 03-your-first-ioc.md

**学习要点**：
- PV（Process Variable）是什么
- Record的作用
- CA（Channel Access）协议
- IOC架构

**实践**：
```bash
# 启动IOC
cd ~/BPMIOC/iocBoot/iocBPMmonitor
./st.cmd

# 使用CA工具
caget LLRF:BPM:RF3Amp
caput LLRF:BPM:SetOffset 0.5
camonitor LLRF:BPM:X1
```

**检查点**：
- [ ] 能解释PV是什么
- [ ] 能用caget/caput访问PV
- [ ] 理解IOC的作用

### Day 3-4: Record类型和扫描机制

**学习材料**：
- Part 2: 04-record-types.md
- Part 2: 05-scanning-basics.md
- Part 2: 08-ca-tools.md

**学习要点**：
- ai/ao/bi/bo/waveform等Record类型
- 扫描机制（Passive, I/O Intr, Periodic）
- CA工具详解

**实践**：
```bash
# 查看Record详细信息
dbpr LLRF:BPM:RF3Amp 3

# 监控PV变化
camonitor LLRF:BPM:RF3Amp

# Python访问
python3 -c "
import epics
pv = epics.PV('LLRF:BPM:RF3Amp')
print(f'Value: {pv.get()}')
"
```

**检查点**：
- [ ] 理解5种基本Record类型
- [ ] 理解I/O Intr扫描
- [ ] 能用Python访问PV

### Day 5: Links和数据库文件

**学习材料**：
- Part 2: 06-links-and-forwarding.md
- Part 2: 09-database-files.md

**学习要点**：
- CP/PP/MS链接
- 数据库文件格式
- Record关联

**实践**：
```bash
# 查看.db文件
cat ~/BPMIOC/BPMmonitorApp/Db/BPMMonitor.db | less

# 查看PV链接
dbpr LLRF:BPM:RF3Amp 4
```

### Weekend: 实验2-3 + 复习

**实验2**: lab02-modify-scan.md
- 修改扫描机制
- 观察行为变化

**实验3**: lab03-add-debug.md
- 添加调试输出
- 追踪数据流

**复习**：
- 回顾本周笔记
- 整理问题清单
- 准备下周学习

## ✅ Week 2 检查点

完成以下检查，确认本周学习效果：

### 理论理解
- [ ] 能解释EPICS架构（IOC、CA、PV）
- [ ] 理解5种基本Record类型的区别
- [ ] 理解3种扫描机制
- [ ] 理解CP/PP链接的作用

### 实践能力
- [ ] 能用caget/caput/camonitor操作PV
- [ ] 能用dbpr查看Record详细信息
- [ ] 能用Python访问EPICS PV
- [ ] 完成实验2-3

### 问题排查
- [ ] 能解决"PV not found"问题
- [ ] 能查看IOC日志
- [ ] 能使用iocsh命令

## 📊 学习进度

在 [10-progress-tracker.md](./10-progress-tracker.md) 中记录：

```markdown
## Week 2: EPICS基础 ✅
- [x] Day 1: EPICS核心概念 (2025-11-10)
- [x] Day 2: CA工具使用 (2025-11-11)
- [x] Day 3: Record类型 (2025-11-12)
- [x] Day 4: 扫描机制 (2025-11-13)
- [x] Day 5: Links和数据库 (2025-11-14)
- [x] Weekend: 实验2-3 + 复习 (2025-11-15/16)

收获：
- 理解了EPICS的核心概念
- 掌握了CA工具的使用
- 完成了基础实验

问题：
- CP链接和PP链接的区别还不够清晰
- Python pyepics库的高级用法需要进一步学习
```

## 💡 学习建议

1. **理论与实践结合**：每学完一个概念就立即实践
2. **做好笔记**：记录关键概念和命令
3. **多问为什么**：不要只记住，要理解原理
4. **对比总结**：对比不同Record类型、扫描机制的区别

## 🔗 相关资源

- [Part 2: EPICS基础](../part2-understanding-basics/)
- [Part 8: 基础实验](../part8-hands-on-labs/labs-basic/)
- [EPICS官方文档](https://epics-controls.org)

---

**下一周**: [04-week3.md](./04-week3.md) - 驱动层理解

**本周时间**: 15-20小时
- 工作日: 2-3小时/天 × 5 = 10-15小时
- 周末: 3-4小时/天 × 2 = 6-8小时
