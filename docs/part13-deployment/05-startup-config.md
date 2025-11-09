# 启动配置

> **目标**: 配置IOC自动启动和管理
> **难度**: ⭐⭐⭐
> **预计时间**: 1-2小时

## 📋 配置自动启动

### 方法1: Systemd服务（推荐）

创建 `/etc/systemd/system/bpmioc.service`:

```ini
[Unit]
Description=BPMIOC - BPM Monitor IOC
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/BPMmonitor/iocBoot/iocBPMmonitor
ExecStart=/opt/BPMmonitor/iocBoot/iocBPMmonitor/st.cmd
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

启用服务：

```bash
# 重新加载systemd
systemctl daemon-reload

# 启用自动启动
systemctl enable bpmioc

# 启动服务
systemctl start bpmioc

# 查看状态
systemctl status bpmioc

# 查看日志
journalctl -u bpmioc -f
```

### 方法2: Init脚本

创建 `/etc/init.d/bpmioc`:

```bash
#!/bin/sh
### BEGIN INIT INFO
# Provides:          bpmioc
# Required-Start:    $network
# Required-Stop:     $network
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: BPM Monitor IOC
### END INIT INFO

DAEMON=/opt/BPMmonitor/iocBoot/iocBPMmonitor/st.cmd
PIDFILE=/var/run/bpmioc.pid

case "$1" in
  start)
    echo "Starting BPMIOC..."
    start-stop-daemon --start --pidfile $PIDFILE --exec $DAEMON --background
    ;;
  stop)
    echo "Stopping BPMIOC..."
    start-stop-daemon --stop --pidfile $PIDFILE
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "Usage: /etc/init.d/bpmioc {start|stop|restart}"
    exit 1
    ;;
esac

exit 0
```

启用：

```bash
chmod +x /etc/init.d/bpmioc
update-rc.d bpmioc defaults
```

## 🔧 环境变量配置

创建 `/opt/BPMmonitor/env.sh`:

```bash
#!/bin/bash
# BPMIOC环境变量

export EPICS_HOST_ARCH=linux-arm
export EPICS_BASE=/opt/epics-base
export PATH=$EPICS_BASE/bin/$EPICS_HOST_ARCH:$PATH
export LD_LIBRARY_PATH=$EPICS_BASE/lib/$EPICS_HOST_ARCH:$LD_LIBRARY_PATH

# BPM驱动库路径
export BPM_DRIVER_LIB=/opt/BPMDriver/lib/libBPMDriver.so

# CA配置
export EPICS_CA_ADDR_LIST=192.168.1.255
export EPICS_CA_AUTO_ADDR_LIST=YES
```

在 `st.cmd` 中加载：

```bash
#!/opt/BPMmonitor/bin/linux-arm/BPMmonitor

# 加载环境变量
source /opt/BPMmonitor/env.sh

# IOC初始化
dbLoadDatabase("../../dbd/BPMmonitor.dbd")
BPMmonitor_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("../../db/BPMmonitor.db")
iocInit()
```

## 📝 日志配置

### 重定向日志到文件

修改systemd服务：

```ini
[Service]
...
StandardOutput=file:/var/log/bpmioc/ioc.log
StandardError=file:/var/log/bpmioc/error.log
```

### 日志轮转

创建 `/etc/logrotate.d/bpmioc`:

```
/var/log/bpmioc/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 0644 root root
}
```

## 🔗 相关文档

- [04-file-transfer.md](./04-file-transfer.md)
- [06-system-integration.md](./06-system-integration.md)
- [07-monitoring.md](./07-monitoring.md)
