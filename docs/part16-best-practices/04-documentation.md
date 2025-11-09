# 文档编写

> **目标**: 编写清晰完整的文档
> **难度**: ⭐⭐⭐
> **预计时间**: 1-2天

## 文档类型

### README.md

```markdown
# BPMIOC - BPM Monitor IOC

## 概述
BPMIOC是用于LLRF系统BPM监控的EPICS IOC应用。

## 功能特性
- 支持8路RF输入信号采集
- 实时幅度和相位监控
- 寄存器读写
- 波形数据采集

## 快速开始

### 安装
\`\`\`bash
cd BPMmonitor
make
\`\`\`

### 运行
\`\`\`bash
cd iocBoot/iocBPMmonitor
./st.cmd
\`\`\`

### 测试
\`\`\`bash
caget LLRF:BPM:RFIn_01_Amp
\`\`\`

## 依赖
- EPICS Base 3.15.6+
- Mock库（PC开发）或真实驱动（硬件）

## 文档
- [架构设计](docs/DESIGN.md)
- [API参考](docs/API.md)
- [部署指南](docs/DEPLOYMENT.md)

## 许可证
MIT License
```

### API文档

使用Doxygen生成：

```bash
# Doxyfile
PROJECT_NAME = "BPMIOC"
INPUT = BPMmonitorApp/src
RECURSIVE = YES
GENERATE_HTML = YES
EXTRACT_ALL = YES
```

生成文档：

```bash
doxygen Doxyfile
```

### 用户手册

```markdown
# BPMIOC用户手册

## 1. 介绍
### 1.1 系统概述
### 1.2 功能列表

## 2. 安装部署
### 2.1 系统要求
### 2.2 安装步骤
### 2.3 配置说明

## 3. 使用指南
### 3.1 启动IOC
### 3.2 访问PV
### 3.3 常见操作

## 4. 故障排查
### 4.1 常见问题
### 4.2 错误代码
### 4.3 联系支持

## 5. API参考
### 5.1 函数列表
### 5.2 数据结构
### 5.3 示例代码
```

## 代码注释

### 函数文档

```c
/**
 * @brief 读取RF输入信号数据
 * 
 * 该函数从指定的通道读取RF信号数据，根据offset参数返回
 * 幅度、相位或其他参数。
 * 
 * @param offset 数据类型偏移
 *               - OFFSET_AMP (0): 幅度 (dBm)
 *               - OFFSET_PHA (2): 相位 (度)
 *               - OFFSET_Q (4): Q值
 * @param channel 通道号，范围0-7
 * @param type 数据类型，当前未使用，传0
 * 
 * @return 读取的数据值
 * @retval 0.0 通道号无效时返回
 * 
 * @note 该函数线程安全，使用内部缓冲区
 * @warning channel参数必须在0-7范围内
 * 
 * @see SetReg
 * @since v1.0
 * 
 * @par 示例:
 * @code
 * float amp = ReadData(OFFSET_AMP, 0, 0);
 * printf("Channel 0 amplitude: %.3f dBm\n", amp);
 * @endcode
 */
float ReadData(int offset, int channel, int type);
```

## 变更日志

### CHANGELOG.md

```markdown
# Changelog

All notable changes to this project will be documented in this file.

## [1.1.0] - 2025-11-09
### Added
- SNR calculation feature
- Support for 10 RF channels
- Temperature monitoring

### Changed
- Improved performance by 50%
- Updated EPICS Base to 3.15.6

### Fixed
- Memory leak in data acquisition thread
- Phase calculation error

## [1.0.0] - 2025-10-01
### Added
- Initial release
- 8 RF channel support
- Basic monitoring functions
```

## 🔗 相关文档

- [01-coding-standards.md](./01-coding-standards.md)
- [05-code-review.md](./05-code-review.md)
