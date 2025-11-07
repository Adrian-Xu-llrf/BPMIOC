# Week 1: 环境搭建 + 快速复现

> **本周目标**: 让BPMIOC在模拟模式下运行起来
> **学习时间**: 15-20小时
> **难度**: ⭐⭐☆☆☆

## 🎯 本周学习目标

完成本周学习后，你将能够：
- ✅ 安装并配置EPICS Base
- ✅ 编译BPMIOC项目
- ✅ 在模拟模式下运行IOC
- ✅ 使用CA工具访问PV
- ✅ 用Python读写PV
- ✅ 理解PV、Record、IOC等基本概念

## 📅 每日计划

### Day 1 (周一): EPICS Base安装 [3小时]

#### 上午 (1.5小时)
```
09:00-09:30  📖 阅读 Part 1/01-introduction.md
             理解BPMIOC是什么

09:30-10:00  📖 阅读 Part 1/02-prerequisites.md
             检查系统要求

10:00-10:30  💻 安装依赖包
             Ubuntu: apt install build-essential...
```

#### 下午 (1.5小时)
```
14:00-15:30  💻 安装EPICS Base
             📖 跟随 Part 1/03-install-epics.md
             - 下载 base-3.15.6
             - 编译 make -j$(nproc)
             - 配置环境变量
             - 验证安装: which softIoc

15:30-15:45  ✅ 检查点
             运行第一个测试IOC
             caget test:value
```

**作业**:
- [ ] EPICS Base编译成功
- [ ] 环境变量配置正确
- [ ] softIoc能正常运行
- [ ] CA工具能访问测试PV

**笔记**:
```
记录遇到的问题：
1. 问题描述
2. 解决方法
3. 参考资料
```

---

### Day 2 (周二): 编译BPMIOC [2.5小时]

#### 上午 (1.5小时)
```
09:00-09:30  📖 阅读 Part 1/04-clone-and-compile.md
             理解编译流程

09:30-10:30  💻 克隆和配置
             git clone BPMIOC
             vim configure/RELEASE
             (修改EPICS_BASE路径)

10:30-11:00  💻 第一次编译尝试
             make clean
             make -j$(nproc)
             (可能失败,这是正常的)
```

#### 下午 (1小时)
```
14:00-15:00  💻 启用模拟模式
             📖 Part 1/05-enable-simulation.md
             - 备份driverWrapper.c
             - 修改InitDevice()
             - 修改my_thread()
             - 重新编译
```

**作业**:
- [ ] BPMIOC代码已克隆
- [ ] configure/RELEASE配置正确
- [ ] 模拟模式代码已添加
- [ ] BPMIOC编译成功

**笔记**:
```
记录修改的文件和位置：
- driverWrapper.c: 第XX行添加use_simulation
- ...
```

---

### Day 3 (周三): 运行IOC [2小时]

#### 上午 (1小时)
```
09:00-09:30  💻 第一次运行IOC
             📖 Part 1/06-first-run.md
             cd iocBoot/iocBPMmonitor
             ./st.cmd

09:30-10:00  💻 IOC Shell实验
             epics> dbl
             epics> dbgf "...:RF3Amp"
             epics> dbpr "...:RF3Amp" 2
```

#### 下午 (1小时)
```
14:00-15:00  💻 验证PV访问
             📖 Part 1/07-verify-pvs.md

             新终端:
             caget ...:RF3Amp
             camonitor ...:RF3Amp
             camonitor ...:RF3Phase
```

**作业**:
- [ ] IOC能成功启动
- [ ] 看到"SIMULATION mode"消息
- [ ] dbl能列出所有PV
- [ ] caget能读取到数值
- [ ] camonitor显示值在变化

**实验任务**:
1. 监控所有8个RF通道的振幅
2. 观察值的变化规律（正弦波）
3. 截图保存

---

### Day 4 (周四): EPICS基础概念 [2.5小时]

#### 上午 (1.5小时)
```
09:00-09:30  📖 Part 2/01-what-is-epics.md
             EPICS是什么

09:30-10:30  📖 Part 2/02-key-concepts.md
             核心概念详解:
             - PV (Process Variable)
             - Record
             - IOC
             - Channel Access
             - Device Support
```

#### 下午 (1小时)
```
14:00-15:00  📖 Part 2/04-record-types.md
             记录类型入门:
             - ai (Analog Input)
             - ao (Analog Output)
             - calc (Calculation)
             - waveform
```

**作业**:
- [ ] 理解PV、Record、IOC的关系
- [ ] 能用自己的话解释Channel Access
- [ ] 知道ai、ao等记录类型的用途

**思考题**:
1. PV和Record是什么关系？
2. 为什么需要Device Support？
3. Channel Access是如何工作的？

---

### Day 5 (周五): 创建第一个IOC [2小时]

#### 上午 (1小时)
```
09:00-10:00  💻 创建简单IOC
             📖 Part 2/03-your-first-ioc.md

             创建温度监测IOC:
             - 编写database文件
             - 创建启动脚本
             - 运行IOC
```

#### 下午 (1小时)
```
14:00-15:00  💻 实验: 扩展IOC
             添加功能:
             - 3个温度传感器
             - 1个平均温度计算
             - 1个温差计算
             - 1个高温告警
```

**作业**:
- [ ] 创建了自己的第一个IOC
- [ ] 理解数据库文件的结构
- [ ] 能添加新的记录
- [ ] 能用calc记录做计算

**挑战任务**:
创建一个数据记录器IOC，包含：
- 4个ai记录（传感器）
- 1个calc记录（总和）
- 1个calc记录（平均值）
- 使用SCAN字段设置不同扫描周期

---

### Weekend (周末): 实践和复习 [7小时]

#### 周六 (4小时)

**上午 (2小时)**
```
09:00-10:00  📖 复习本周内容
             - 回顾笔记
             - 整理不理解的概念
             - 查阅FAQ

10:00-11:00  📖 Part 2/05-scanning-basics.md
             扫描机制详解
             📖 Part 2/06-ca-tools.md
             CA工具使用
```

**下午 (2小时)**
```
14:00-16:00  🧪 Part 8/labs-basic/lab01
             实验1: 追踪RF振幅数据流

             任务:
             1. 在devBPMMonitor.c添加调试输出
             2. 在driverWrapper.c添加调试输出
             3. 重新编译运行
             4. 观察完整数据流
             5. 绘制数据流图
```

#### 周日 (3小时)

**上午 (1.5小时)**
```
09:00-10:30  📖 Part 2/07-python-epics.md
             Python访问EPICS

             💻 安装pyepics:
             pip install pyepics

             💻 编写Python脚本:
             - 读取RF振幅
             - 监控PV变化
             - 绘制波形图
```

**下午 (1.5小时)**
```
14:00-15:30  📝 总结和规划
             1. 填写进度追踪表
             2. 整理本周笔记
             3. 列出遗留问题
             4. 规划下周学习
```

**作业**:
- [ ] 完成lab01实验
- [ ] 能用Python读写PV
- [ ] 画出RF3Amp的完整数据流图
- [ ] 总结本周学习成果

---

## ✅ 本周检查清单

### 环境搭建
- [ ] EPICS Base 3.15.6已安装
- [ ] 环境变量配置正确
- [ ] softIoc命令可用
- [ ] CA工具(caget/caput)可用

### BPMIOC
- [ ] 代码已克隆
- [ ] 模拟模式已启用
- [ ] 编译成功
- [ ] IOC能启动
- [ ] 在模拟模式下运行

### PV访问
- [ ] 能用dbl列出PV
- [ ] 能用dbgf读取PV
- [ ] 能用caget访问PV
- [ ] 能用camonitor监控PV
- [ ] 能用Python访问PV

### 概念理解
- [ ] 理解PV是什么
- [ ] 理解Record是什么
- [ ] 理解IOC是什么
- [ ] 理解Channel Access是什么
- [ ] 知道ai、ao、calc等记录类型

### 实践能力
- [ ] 创建了第一个简单IOC
- [ ] 完成了lab01实验
- [ ] 画出了数据流图
- [ ] 能修改数据库文件

## 📊 学习成果

本周结束时，你应该有：

### 文档
- [ ] EPICS安装笔记
- [ ] BPMIOC编译笔记
- [ ] 遇到的问题和解决方案
- [ ] 概念理解笔记

### 代码
- [ ] 修改后的driverWrapper.c (模拟模式)
- [ ] 你的第一个IOC项目
- [ ] Python访问脚本

### 图表
- [ ] RF3Amp数据流图
- [ ] BPMIOC三层架构图 (手绘)
- [ ] EPICS基本概念关系图

## 💡 学习建议

### 本周重点
1. **不要着急**: 环境搭建可能遇到各种问题，耐心解决
2. **做好笔记**: 记录每个步骤和遇到的问题
3. **理解概念**: PV、Record、IOC这些基本概念很重要
4. **动手实践**: 必须自己敲命令，不要只看

### 常见问题
- Q: EPICS编译很慢？
  A: 正常，使用 `make -j$(nproc)` 并行编译

- Q: BPMIOC编译失败？
  A: 检查configure/RELEASE路径，查看08-troubleshooting.md

- Q: IOC启动但PV访问不到？
  A: 检查网络，ping localhost，查看CA配置

- Q: 概念太抽象理解不了？
  A: 继续往下学，做实验后会更清楚

## 📝 每日笔记模板

```markdown
# Week 1 Day X - [日期]

## 今日目标
- [ ] 目标1
- [ ] 目标2

## 学习内容
- 主题1: [笔记]
- 主题2: [笔记]

## 动手实践
- 任务1: [结果]
- 任务2: [结果]

## 遇到的问题
1. 问题: [描述]
   解决: [方法]

## 今日收获
- 学到了XXX
- 理解了XXX
- 做出了XXX

## 明日计划
- [ ] 任务1
- [ ] 任务2
```

## 🔗 相关资源

### 本周主要文档
- [Part 1: 快速复现](../part1-quick-reproduction/)
- [Part 2: 理解基础](../part2-understanding-basics/)
- [Part 8: lab01](../part8-hands-on-labs/labs-basic/lab01-trace-rf-amp.md)

### 参考资料
- [EPICS官方文档](https://epics-controls.org/)
- [FAQ](../part18-appendix/faq.md)
- [故障排查](../part18-appendix/troubleshooting-guide.md)

---

## 下周预告: Week 2

下周我们将：
- 深入学习EPICS数据库
- 理解BPMIOC架构
- 完成更多实验
- 开始修改代码

**继续加油！** 🚀完成第一周后，你已经迈出了重要的一步！
