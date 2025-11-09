# 目标板设置

> **目标**: 配置ARM目标板运行环境
> **难度**: ⭐⭐⭐
> **预计时间**: 半天
> **前置知识**: Linux系统管理

## 📋 目标板准备

### 硬件准备

- ARM开发板（Raspberry Pi / Zynq / BeagleBone）
- SD卡/eMMC（≥8GB）
- 网线
- 串口线（调试）
- 电源

### 操作系统安装

#### Raspberry Pi

```bash
# 下载Raspbian Lite
wget https://downloads.raspberrypi.org/raspbian_lite_latest

# 烧录到SD卡
dd if=raspbian-lite.img of=/dev/sdX bs=4M status=progress
sync
```

#### Zynq (Petalinux)

参考Xilinx Petalinux文档创建镜像

### 网络配置

```bash
# 编辑网络接口
vi /etc/network/interfaces

auto eth0
iface eth0 inet static
    address 192.168.1.100
    netmask 255.255.255.0
    gateway 192.168.1.1
    dns-nameservers 8.8.8.8
```

### 安装依赖

```bash
# 更新包管理器
apt-get update

# 安装必需库
apt-get install -y \
    libreadline7 \
    libncurses5 \
    libstdc++6 \
    libc6 \
    openssh-server \
    rsync
```

### 创建运行环境

```bash
# 创建目录
mkdir -p /opt/BPMmonitor
mkdir -p /opt/epics-base
mkdir -p /var/log/bpmioc

# 创建用户（可选）
useradd -m -s /bin/bash ioc
```

## 🔗 相关文档

- [02-cross-compile.md](./02-cross-compile.md)
- [04-file-transfer.md](./04-file-transfer.md)
