# Week 3: BPMIOC架构深入理解

> **时间**: 第3周（15-20小时）
> **目标**: 理解BPMIOC三层架构和驱动层
> **难度**: ⭐⭐⭐⭐☆

## 📅 本周目标

- ✅ 理解BPMIOC三层架构
- ✅ 深入理解驱动层（driverWrapper.c）
- ✅ 掌握动态库加载（dlopen/dlsym）
- ✅ 理解Offset系统
- ✅ 完成数据流追踪实验

## 📚 学习内容

### Day 1-2: 三层架构理解

**学习材料**：
- Part 3: 01-architecture-overview.md
- Part 3: 02-data-flow.md
- Part 3: 03-initialization-sequence.md

**学习要点**：
- 三层架构设计理念
- 完整数据流（硬件→驱动→设备支持→数据库→CA）
- 系统初始化序列

**实践**：
```bash
# 追踪RF3Amp的完整数据流
# 1. 查看数据库定义
grep "RF3Amp" ~/BPMIOC/BPMmonitorApp/Db/BPMMonitor.db

# 2. 查看设备支持层调用
grep "read_ai" ~/BPMIOC/BPMmonitorApp/src/devBPMMonitor.c

# 3. 查看驱动层实现
grep "ReadData" ~/BPMIOC/BPMmonitorApp/src/driverWrapper.c
```

### Day 3-4: 驱动层深入

**学习材料**：
- Part 4: README.md
- Part 4: 01-overview.md
- Part 4: 04-initdevice.md

**学习要点**：
- driverWrapper.c结构
- InitDevice()初始化流程
- pthread数据采集线程
- ReadData()函数实现

**实践**：
```bash
# 在InitDevice添加调试输出
# 编辑driverWrapper.c，添加printf
cd ~/BPMIOC/BPMmonitorApp/src
vim driverWrapper.c

# 重新编译
cd ~/BPMIOC
make

# 运行观察输出
cd iocBoot/iocBPMmonitor
./st.cmd
```

### Day 5: Offset系统和内存模型

**学习材料**：
- Part 3: 04-memory-model.md
- Part 3: 05-offset-system.md

**学习要点**：
- 全局缓冲区设计
- Offset系统工作原理
- 如何添加新offset

### Weekend: 实验4 + 架构总结

**实验4**: lab04-understand-init.md
- 理解初始化过程
- 添加日志追踪

**总结**：
- 绘制三层架构图
- 绘制完整数据流图
- 整理Offset映射表

## ✅ Week 3 检查点

- [ ] 能画出三层架构图
- [ ] 能追踪任意PV的完整数据流
- [ ] 理解InitDevice()的5个步骤
- [ ] 理解Offset系统的设计
- [ ] 完成实验4

## 🔗 相关资源

- [Part 3: BPMIOC架构](../part3-bpmioc-architecture/)
- [Part 4: 驱动层](../part4-driver-layer/)

---

**下一周**: [05-week4.md](./05-week4.md) - 驱动层修改实践
