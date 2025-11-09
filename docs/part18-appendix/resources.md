# 外部资源 (Resources)

精选的EPICS和加速器控制相关学习资源。

## 目录
- [官方文档](#官方文档)
- [在线教程](#在线教程)
- [书籍推荐](#书籍推荐)
- [视频课程](#视频课程)
- [社区和论坛](#社区和论坛)
- [开发工具](#开发工具)
- [相关项目](#相关项目)

---

## 官方文档

### EPICS官方

| 资源 | 链接 | 说明 |
|------|------|------|
| **EPICS官网** | https://epics-controls.org/ | 官方主页 |
| **EPICS文档中心** | https://docs.epics-controls.org/ | 完整文档集合 |
| **Application Developer's Guide** | https://docs.epics-controls.org/en/latest/appdevguide/ | 应用开发指南 |
| **Record Reference Manual** | https://docs.epics-controls.org/en/latest/recordref/ | Record类型参考 |
| **GitHub仓库** | https://github.com/epics-base/epics-base | 源代码 |

### EPICS Base文档

- **3.15 Documentation**: https://epics.anl.gov/base/R3-15/6-docs/
- **7.0 Documentation**: https://docs.epics-controls.org/en/latest/

### EPICS Module文档

| Module | 链接 | 说明 |
|--------|------|------|
| **asyn** | https://epics.anl.gov/modules/soft/asyn/ | 异步设备支持 |
| **StreamDevice** | https://paulscherrerinstitute.github.io/StreamDevice/ | 基于流的设备支持 |
| **calc** | https://epics.anl.gov/modules/soft/calc/ | 计算模块 |
| **autosave** | https://epics.anl.gov/modules/soft/autosave/ | 自动保存/恢复 |

---

## 在线教程

### EPICS教程

**EPICS官方教程**
- Training Materials: https://epics-controls.org/resources-and-support/documents/training/
- Getting Started Guide: https://docs.epics-controls.org/en/latest/getting-started/

**第三方教程**
- **EPICS for Dummies**: https://www.aps.anl.gov/epics/docs/
- **EPICS at Diamond Light Source**: https://www.diamond.ac.uk/Instruments/Support/Software/EPICS.html

### 加速器物理

**入门课程**
- **CERN Accelerator School**: https://cas.web.cern.ch/
  - 免费的在线课程和讲义

- **US Particle Accelerator School**: https://uspas.fnal.gov/
  - 暑期学校和在线课程

**BPM和LLRF**
- **BPM Tutorial (CERN)**: https://indico.cern.ch/event/390748/
- **LLRF系统介绍**: https://accelconf.web.cern.ch/

---

## 书籍推荐

### EPICS相关

虽然EPICS专门书籍较少，但以下资源很有价值：

**在线书籍**:
- **EPICS IOC Application Developer's Guide** (官方，免费)
  - https://docs.epics-controls.org/en/latest/appdevguide/

**相关书籍**:
- **"Embedded Linux Systems with the Yocto Project"** - Rudolf J. Streif
  - 嵌入式Linux开发（用于IOC部署）

### 加速器相关

| 书名 | 作者 | 说明 |
|------|------|------|
| **Handbook of Accelerator Physics and Engineering** | A. Chao, M. Tigner | 加速器物理百科全书 |
| **RF Linear Accelerators** | T. Wangler | RF加速原理 |
| **Beam Instrumentation and Diagnostics** | P. Strehl | 束流诊断 |
| **The Physics of Particle Accelerators** | K. Wille | 加速器物理入门 |

### 控制系统

- **"Feedback Control of Dynamic Systems"** - Franklin, Powell, Emami-Naeini
  - 反馈控制理论（用于LLRF）

- **"Real-Time Systems"** - Jane W. S. Liu
  - 实时系统设计

---

## 视频课程

### YouTube资源

**EPICS相关**:
- Search: "EPICS IOC tutorial"
- Search: "Channel Access tutorial"

**加速器物理**:
- **CERN Lectures on YouTube**
  - https://www.youtube.com/c/CERN
  - 搜索 "accelerator physics"

- **Fermilab Lectures**
  - https://www.youtube.com/user/fermilab
  - 搜索 "particle accelerator"

### 在线课程平台

- **Coursera**: 搜索 "Control Systems", "Embedded Systems"
- **edX**: 搜索 "Particle Physics"

---

## 社区和论坛

### EPICS社区

**邮件列表** (最活跃的支持渠道):
- **tech-talk**: tech-talk@aps.anl.gov
  - 技术讨论和问题求助
  - 订阅: https://epics.anl.gov/tech-talk/

- **core-talk**: core-talk@aps.anl.gov
  - EPICS Core开发讨论

**Slack**:
- EPICS Slack Workspace: https://epics-controls.slack.com/
  - 需要邀请加入

**GitHub**:
- EPICS Organization: https://github.com/epics-base
  - Issues和Discussions

### 加速器社区

- **Accelerator Physics Forum**: https://www.physics.stackexchange.com/
  - 标签: accelerator-physics

- **CERN Indico**: https://indico.cern.ch/
  - 会议、讲座、培训

---

## 开发工具

### IDE和编辑器

| 工具 | 说明 | 链接 |
|------|------|------|
| **VS Code** | 轻量级IDE，有EPICS插件 | https://code.visualstudio.com/ |
| **Eclipse** | EPICS推荐的IDE | https://www.eclipse.org/ |
| **Vim/Emacs** | 终端编辑器 | - |

**VS Code插件**:
- C/C++ (Microsoft)
- Makefile Tools
- Remote - SSH (远程开发)

### EPICS GUI工具

| 工具 | 类型 | 说明 | 链接 |
|------|------|------|------|
| **CS-Studio** | 现代GUI | 基于Eclipse | https://controlssoftware.sns.ornl.gov/css/ |
| **Phoebus** | 现代GUI | CS-Studio继任者 | https://github.com/ControlSystemStudio/phoebus |
| **MEDM** | 传统GUI | Motif EDM | https://epics.anl.gov/extensions/medm/ |
| **caQtDM** | Qt GUI | 基于Qt | https://github.com/caqtdm/caqtdm |

### CA命令行工具

这些工具随EPICS Base安装：

```bash
caget     # 读取PV
caput     # 写入PV
camonitor # 监控PV变化
cainfo    # 查看PV信息
casw      # CA网络搜索
```

### 调试工具

| 工具 | 用途 | 命令 |
|------|------|------|
| **GDB** | 源码级调试 | `gdb program` |
| **Valgrind** | 内存泄漏检测 | `valgrind --leak-check=full` |
| **strace** | 系统调用跟踪 | `strace -p PID` |
| **perf** | 性能分析 | `perf record -g -p PID` |

### 网络工具

```bash
wireshark    # 抓包分析CA协议
tcpdump      # 命令行抓包
netstat      # 查看网络连接
```

---

## 相关项目

### 开源IOC示例

| 项目 | 说明 | 链接 |
|------|------|------|
| **epics-docker** | EPICS Docker镜像 | https://github.com/epics-containers/epics-docker |
| **motorMotorSim** | 电机模拟器IOC | https://github.com/epics-modules/motor |
| **ADSimDetector** | 区域探测器模拟器 | https://github.com/areaDetector/ADSimDetector |

### 加速器控制系统

| 系统 | 实验室 | 说明 |
|------|--------|------|
| **EPICS** | ANL, SLAC等 | 美国主流 |
| **TANGO** | ESRF等 | 欧洲主流 |
| **DOOCS** | DESY | 德国XFEL |

### 数据归档

- **Archiver Appliance**: https://slacmshankar.github.io/epicsarchiver_docs/
  - EPICS数据归档解决方案

- **InfluxDB + Grafana**:
  - https://www.influxdata.com/
  - https://grafana.com/

---

## 学习路径建议

### 初学者

```
1. 阅读官方Getting Started
   ↓
2. 完成EPICS Tutorial
   ↓
3. 构建简单的IOC
   ↓
4. 学习CA工具
   ↓
5. 编写设备支持
```

**推荐资源**:
- Application Developer's Guide
- Getting Started Guide
- 本项目Part 1-2

### 进阶开发者

```
1. 深入Record Reference
   ↓
2. 学习asyn框架
   ↓
3. 性能优化技巧
   ↓
4. 分布式系统设计
   ↓
5. 源码阅读
```

**推荐资源**:
- EPICS Base源码
- tech-talk归档
- 本项目Part 12-14

### 加速器物理背景

```
1. CERN School基础课程
   ↓
2. BPM/LLRF原理
   ↓
3. 结合EPICS实现
   ↓
4. 实际项目经验
```

**推荐资源**:
- 本项目Part 17
- CERN Indico讲座
- Handbook of Accelerator Physics

---

## 获取帮助

### 提问前的准备

1. **搜索已有答案**
   - Google: "EPICS [your problem]"
   - tech-talk归档搜索

2. **准备信息**
   - EPICS版本: `echo $EPICS_BASE`
   - 操作系统: `uname -a`
   - 完整错误信息
   - 最小可复现示例

3. **选择合适的渠道**
   - 技术问题 → tech-talk
   - Bug报告 → GitHub Issues
   - 快速讨论 → Slack

### 提问模板

```
标题: [简洁描述问题]

环境:
- EPICS Base版本:
- 操作系统:
- 相关模块:

问题描述:
[详细描述]

复现步骤:
1. ...
2. ...

期望行为:
[...]

实际行为:
[...]

相关代码/配置:
```cpp
// 代码片段
```

已尝试的解决方案:
- ...
```

---

## 保持更新

### 订阅邮件列表

```bash
# 订阅tech-talk
# 发送邮件到: tech-talk-request@aps.anl.gov
# 主题: subscribe
```

### 关注GitHub

- Star epics-base仓库
- Watch releases
- 订阅notifications

### 参加会议

- **EPICS Collaboration Meeting** (每年2次)
  - 春季和秋季
  - https://epics-controls.org/meetings-and-events/

- **加速器会议**
  - IPAC (International Particle Accelerator Conference)
  - PAC (Particle Accelerator Conference)

---

**祝学习顺利！** 记得收藏这些资源，它们会在你的EPICS开发之路上持续提供帮助。
