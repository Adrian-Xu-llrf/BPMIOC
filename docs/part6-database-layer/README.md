# Part 6: 数据库层详解

> **目标**: 深入理解EPICS数据库和PV配置
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 2周
> **前置知识**: Part 2, Part 3, Part 5

## 📋 本部分概述

Part 6详细分析数据库层，包括.db文件的编写、Record配置、PV命名规范等。

主要内容：
- BPMMonitor.db 完整解析
- Record类型和字段详解
- 扫描机制配置
- Alarm和链接配置
- st.cmd启动脚本

完成本部分后，你将能够：
- ✅ 理解.db文件结构
- ✅ 能够配置各种Record
- ✅ 掌握PV命名规范
- ✅ 能够编写启动脚本

## 📚 核心文档

| 文档 | 描述 | 状态 |
|------|------|------|
| README.md | 本文档 | ✅ |
| 01-db-file-structure.md | .db文件结构 | 📝 |
| 02-record-configuration.md | Record配置 | 📝 |
| 03-pv-naming.md | PV命名规范 | 📝 |
| 04-scan-mechanisms.md | 扫描机制 | 📝 |
| 05-startup-script.md | st.cmd详解 | 📝 |

## 🎯 学习要点

### 典型Record配置

```
record(ai, "LLRF:BPM:RF3Amp")
{
    field(DTYP, "BPM")           # 设备类型
    field(INP,  "@0:3")          # 输入参数
    field(SCAN, "I/O Intr")      # 扫描机制
    field(PREC, "3")             # 精度
    field(EGU,  "V")             # 工程单位
    field(HIHI, "5.0")           # 高高限
    field(HHSV, "MAJOR")         # 高高报警级别
}
```

### PV命名规范

```
LLRF:BPM:RF3Amp
  │    │    │   └─> 数据类型（Amp/Phase）
  │    │    └─────> 通道号（RF3）
  │    └──────────> 设备名（BPM）
  └───────────────> 系统名（LLRF）
```

## 🔗 相关文档

- **Part 2**: EPICS基础
- **Part 5**: 设备支持层
- **Part 8**: 实验室

## 📚 参考资源

- [Database Definition Guide](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/DatabaseDefinition.html)
- [IOC Shell Commands](https://epics.anl.gov/base/R3-15/6-docs/AppDevGuide/IOCShell.html)

---

**源码**: `BPMmonitorApp/Db/BPMMonitor.db`
