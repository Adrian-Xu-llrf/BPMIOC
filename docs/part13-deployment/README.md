# Part 13: 部署上线

> **目标**: 掌握从PC开发到真实硬件部署的完整流程
> **难度**: ⭐⭐⭐⭐☆
> **预计时间**: 1-2周
> **前置知识**: Part 1-10, Linux系统管理基础

## 📋 本部分概述

Part 13提供从开发环境到生产环境的完整部署指南。

主要内容：
- 交叉编译配置
- 目标板环境搭建
- 文件传输和部署
- 系统集成和测试
- 运行监控和维护

完成本部分后，你将能够：
- ✅ 配置交叉编译环境
- ✅ 部署IOC到ARM板
- ✅ 集成真实硬件
- ✅ 监控运行状态
- ✅ 快速排查故障
- ✅ 执行系统维护和升级

## 📚 核心文档

| 文档 | 描述 | 状态 |
|------|------|------|
| README.md | 本文档 | ✅ |
| **[01-deployment-overview.md](./01-deployment-overview.md)** | 部署概览 | ✅ |
| **[02-cross-compile.md](./02-cross-compile.md)** | 交叉编译详解 | ✅ |
| **[03-target-setup.md](./03-target-setup.md)** | 目标板设置 | ✅ |
| **[04-file-transfer.md](./04-file-transfer.md)** | 文件传输 | ✅ |
| **[05-startup-config.md](./05-startup-config.md)** | 启动配置 | ✅ |
| **[06-system-integration.md](./06-system-integration.md)** | 系统集成 | ✅ |
| **[07-monitoring.md](./07-monitoring.md)** | 运行监控 | ✅ |
| **[08-troubleshooting.md](./08-troubleshooting.md)** | 故障排查 | ✅ |
| **[09-maintenance.md](./09-maintenance.md)** | 维护升级 | ✅ |

## 🎯 部署流程概览

### 完整部署流程

```
阶段1: 准备阶段
  ├── 安装交叉编译工具链
  ├── 配置EPICS Base交叉编译
  └── 配置BPMIOC交叉编译

阶段2: 编译阶段
  ├── 交叉编译EPICS Base
  ├── 交叉编译BPMIOC
  └── 验证编译产物

阶段3: 目标板准备
  ├── 安装操作系统
  ├── 配置网络
  ├── 安装依赖库
  └── 创建运行环境

阶段4: 部署阶段
  ├── 传输文件
  ├── 配置启动脚本
  ├── 设置权限
  └── 测试运行

阶段5: 集成阶段
  ├── 连接真实硬件
  ├── 验证硬件通信
  ├── 测试PV访问
  └── 性能调优

阶段6: 上线阶段
  ├── 配置自动启动
  ├── 设置监控
  ├── 备份配置
  └── 文档记录
```

## 🔧 快速开始

### 最小化部署步骤

**假设**：
- 开发PC：Ubuntu 20.04 x86_64
- 目标板：Raspberry Pi 4 / Zynq-7000 ARM板
- 网络：192.168.1.0/24

**步骤**：

```bash
# 1. 安装交叉编译工具链
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# 2. 配置BPMIOC交叉编译
cd /opt/BPMmonitor
# 编辑 configure/CONFIG_SITE
echo "CROSS_COMPILER_TARGET_ARCHS = linux-arm" >> configure/CONFIG_SITE

# 3. 交叉编译
make CROSS_COMPILER_TARGET_ARCHS=linux-arm

# 4. 传输到目标板
scp -r bin/linux-arm dbd db iocBoot root@192.168.1.100:/opt/BPMmonitor/

# 5. 在目标板上运行
ssh root@192.168.1.100
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor
./st.cmd

# 6. 测试
caget LLRF:BPM:RFIn_01_Amp
```

## 📊 部署检查清单

### 准备阶段

- [ ] 安装交叉编译工具链
- [ ] EPICS Base交叉编译成功
- [ ] BPMIOC交叉编译成功
- [ ] 所有依赖库已准备

### 目标板准备

- [ ] 操作系统安装完成
- [ ] 网络配置正确
- [ ] SSH访问正常
- [ ] 必要的系统库已安装
- [ ] 存储空间充足（>1GB）

### 部署阶段

- [ ] 文件传输完整
- [ ] 文件权限正确
- [ ] 环境变量配置
- [ ] st.cmd可执行
- [ ] IOC能成功启动

### 验证阶段

- [ ] PV可以访问
- [ ] 硬件通信正常
- [ ] 数据值正确
- [ ] 性能满足要求
- [ ] 无错误日志

### 生产准备

- [ ] 自动启动配置
- [ ] 监控脚本部署
- [ ] 日志轮转配置
- [ ] 备份方案就绪
- [ ] 文档更新完成

## 🌐 支持的目标平台

| 平台 | 架构 | 状态 | 说明 |
|------|------|------|------|
| **Raspberry Pi 4** | ARM Cortex-A72 | ✅ | 推荐用于开发测试 |
| **Zynq-7000** | ARM Cortex-A9 + FPGA | ✅ | 生产环境常用 |
| **BeagleBone Black** | ARM Cortex-A8 | ✅ | 低成本方案 |
| **i.MX6** | ARM Cortex-A9 | ✅ | 工业级应用 |
| **RTEMS** | 实时操作系统 | 🚧 | 需要专门配置 |

## 🔍 常见部署场景

### 场景1: 开发测试部署

**目标**：快速部署到测试板进行功能验证

**特点**：
- 使用NFS挂载，无需每次传输文件
- 开启调试符号
- 详细日志输出

### 场景2: 生产环境部署

**目标**：稳定可靠的生产部署

**特点**：
- 本地存储，独立运行
- 优化编译，剥离调试符号
- 自动启动，异常重启
- 完整监控和日志

### 场景3: 批量部署

**目标**：部署到多个相同的目标板

**特点**：
- 统一镜像
- 自动化脚本
- 远程管理

## 🛠️ 工具和脚本

### 部署脚本示例

```bash
#!/bin/bash
# deploy.sh - BPMIOC自动部署脚本

TARGET_IP="192.168.1.100"
TARGET_USER="root"
TARGET_DIR="/opt/BPMmonitor"

echo "=== BPMIOC Deployment Script ==="

# 1. 交叉编译
echo "[1/4] Cross compiling..."
make clean
make CROSS_COMPILER_TARGET_ARCHS=linux-arm

# 2. 打包
echo "[2/4] Packaging..."
tar czf bpmioc-arm.tar.gz bin/linux-arm dbd db iocBoot

# 3. 传输
echo "[3/4] Transferring to target..."
scp bpmioc-arm.tar.gz ${TARGET_USER}@${TARGET_IP}:/tmp/

# 4. 部署
echo "[4/4] Deploying on target..."
ssh ${TARGET_USER}@${TARGET_IP} <<'EOF'
cd /tmp
tar xzf bpmioc-arm.tar.gz -C /opt/BPMmonitor/
chmod +x /opt/BPMmonitor/iocBoot/iocBPMmonitor/st.cmd
systemctl restart bpmioc
EOF

echo "=== Deployment Complete ==="
```

## 📚 参考资源

- **EPICS交叉编译**: https://epics.anl.gov/base/R3-15/6-docs/README.Cross
- **ARM工具链**: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain
- **Buildroot**: https://buildroot.org/
- **Yocto Project**: https://www.yoctoproject.org/

## 🔗 相关文档

- **[Part 7: 构建系统](../part7-build-system/)** - Makefile和交叉编译基础
- **[Part 10: 调试与测试](../part10-debugging-testing/)** - 远程调试
- **[Part 19: 模拟器教程](../part19-simulator-tutorial/)** - PC开发环境

---

**开始部署**: 从 [01-deployment-overview.md](./01-deployment-overview.md) 开始学习！
