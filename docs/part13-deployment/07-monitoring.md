# 运行监控

> **目标**: 监控IOC运行状态和性能
> **难度**: ⭐⭐⭐
> **预计时间**: 半天

## 📋 监控方法

### 系统资源监控

#### CPU和内存

```bash
# 实时监控
top -p $(pidof BPMmonitor)

# 或使用htop
htop -p $(pidof BPMmonitor)

# 获取统计信息
ps aux | grep BPMmonitor
```

#### 网络连接

```bash
# 查看CA连接
netstat -an | grep 5064
netstat -an | grep 5065

# 查看连接数
netstat -an | grep 5064 | wc -l
```

### IOC状态监控

#### Channel Access统计

```bash
# 在IOC Shell中
epics> casr 2
```

#### Record扫描状态

```bash
epics> scanppl
```

### 自动化监控脚本

创建 `/opt/monitor/ioc_monitor.sh`:

```bash
#!/bin/bash
# IOC监控脚本

LOG_FILE="/var/log/bpmioc/monitor.log"

while true; do
    echo "=== $(date) ===" >> $LOG_FILE
    
    # CPU和内存
    ps aux | grep BPMmonitor | grep -v grep >> $LOG_FILE
    
    # 进程状态
    if pgrep BPMmonitor > /dev/null; then
        echo "IOC: Running" >> $LOG_FILE
    else
        echo "IOC: STOPPED - Restarting..." >> $LOG_FILE
        systemctl restart bpmioc
    fi
    
    # PV健康检查
    if timeout 5 caget LLRF:BPM:RFIn_01_Amp > /dev/null 2>&1; then
        echo "PV Access: OK" >> $LOG_FILE
    else
        echo "PV Access: FAILED" >> $LOG_FILE
    fi
    
    echo "" >> $LOG_FILE
    sleep 60
done
```

配置为systemd服务：

```ini
[Unit]
Description=IOC Monitor
After=bpmioc.service

[Service]
Type=simple
ExecStart=/opt/monitor/ioc_monitor.sh
Restart=always

[Install]
WantedBy=multi-user.target
```

## 📊 日志分析

### 查看系统日志

```bash
# Systemd日志
journalctl -u bpmioc -f

# 查看最近100行
journalctl -u bpmioc -n 100

# 查看特定时间范围
journalctl -u bpmioc --since "2025-11-09 14:00" --until "2025-11-09 15:00"
```

### 分析IOC日志

```bash
# 查找错误
grep -i error /var/log/bpmioc/ioc.log

# 统计错误次数
grep -c ERROR /var/log/bpmioc/ioc.log

# 查找性能警告
grep -i "slow" /var/log/bpmioc/ioc.log
```

## 🚨 告警配置

### Email告警

安装ssmtp：

```bash
apt-get install ssmtp

# 配置 /etc/ssmtp/ssmtp.conf
root=admin@example.com
mailhub=smtp.gmail.com:587
AuthUser=your@gmail.com
AuthPass=yourpassword
UseSTARTTLS=YES
```

告警脚本：

```bash
#!/bin/bash
# alert.sh

send_alert() {
    echo "Subject: IOC Alert: $1
    
$2" | ssmtp admin@example.com
}

# 使用
send_alert "IOC Stopped" "BPMIOC has stopped running on $(hostname)"
```

## 🔗 相关文档

- [06-system-integration.md](./06-system-integration.md)
- [08-troubleshooting.md](./08-troubleshooting.md)
- [Part 10: 02-logging.md](../part10-debugging-testing/02-logging.md)
