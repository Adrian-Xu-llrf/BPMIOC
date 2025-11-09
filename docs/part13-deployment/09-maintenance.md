# 维护升级

> **目标**: IOC系统维护和版本升级
> **难度**: ⭐⭐⭐
> **预计时间**: 按需

## 📋 日常维护

### 备份

#### 配置备份

```bash
#!/bin/bash
# backup.sh

BACKUP_DIR="/backup/bpmioc"
DATE=$(date +%Y%m%d)

mkdir -p $BACKUP_DIR

# 备份配置
tar czf $BACKUP_DIR/config-$DATE.tar.gz \
    /opt/BPMmonitor/iocBoot \
    /opt/BPMmonitor/db \
    /etc/systemd/system/bpmioc.service

# 备份日志
cp /var/log/bpmioc/ioc.log $BACKUP_DIR/ioc-$DATE.log

# 删除30天前的备份
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete
```

定期执行：

```bash
# 添加到crontab
0 2 * * * /opt/scripts/backup.sh
```

### 日志清理

```bash
# 手动清理
find /var/log/bpmioc -name "*.log" -mtime +7 -delete

# 或使用logrotate（已在05-startup-config.md配置）
```

### 系统更新

```bash
# 更新操作系统
apt-get update
apt-get upgrade

# 重启IOC服务
systemctl restart bpmioc
```

## 🔄 版本升级

### 升级流程

#### 1. 准备阶段

```bash
# 备份当前版本
cd /opt/BPMmonitor
tar czf /backup/bpmioc-backup-$(date +%Y%m%d).tar.gz .

# 记录当前版本
./bin/linux-arm/BPMmonitor --version > /backup/version.txt
```

#### 2. 编译新版本

在PC上：

```bash
cd /opt/BPMmonitor
git pull
make clean
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

#### 3. 传输新版本

```bash
# 停止IOC
ssh root@192.168.1.100 'systemctl stop bpmioc'

# 传输新文件
scp -r bin/linux-arm dbd db root@192.168.1.100:/opt/BPMmonitor/

# 启动IOC
ssh root@192.168.1.100 'systemctl start bpmioc'
```

#### 4. 验证

```bash
# 检查IOC状态
ssh root@192.168.1.100 'systemctl status bpmioc'

# 测试PV访问
caget LLRF:BPM:RFIn_01_Amp
```

#### 5. 回滚（如果需要）

```bash
# 停止IOC
ssh root@192.168.1.100 'systemctl stop bpmioc'

# 恢复备份
ssh root@192.168.1.100 'cd /opt/BPMmonitor && tar xzf /backup/bpmioc-backup-YYYYMMDD.tar.gz'

# 启动IOC
ssh root@192.168.1.100 'systemctl start bpmioc'
```

## 📊 健康检查

### 每周检查

- [ ] IOC运行时间（uptime）
- [ ] 内存使用情况
- [ ] 磁盘空间
- [ ] 日志错误统计
- [ ] PV访问测试
- [ ] 性能指标记录

### 每月检查

- [ ] 系统更新
- [ ] 配置审查
- [ ] 备份验证
- [ ] 文档更新

## 🔧 自动化维护脚本

```bash
#!/bin/bash
# maintenance.sh - 自动化维护脚本

echo "=== IOC Maintenance Report $(date) ===" | tee /var/log/maintenance.log

# 1. IOC状态
echo "1. IOC Status:" | tee -a /var/log/maintenance.log
systemctl status bpmioc | grep Active | tee -a /var/log/maintenance.log

# 2. 资源使用
echo "2. Resource Usage:" | tee -a /var/log/maintenance.log
ps aux | grep BPMmonitor | grep -v grep | tee -a /var/log/maintenance.log

# 3. 磁盘空间
echo "3. Disk Space:" | tee -a /var/log/maintenance.log
df -h /opt /var/log | tee -a /var/log/maintenance.log

# 4. 日志统计
echo "4. Error Count (last 24h):" | tee -a /var/log/maintenance.log
journalctl -u bpmioc --since "24 hours ago" | grep -c ERROR | tee -a /var/log/maintenance.log

# 5. PV测试
echo "5. PV Test:" | tee -a /var/log/maintenance.log
if timeout 5 caget LLRF:BPM:RFIn_01_Amp > /dev/null 2>&1; then
    echo "  PASS" | tee -a /var/log/maintenance.log
else
    echo "  FAIL - Alert sent" | tee -a /var/log/maintenance.log
    # 发送告警
    echo "PV access failed" | mail -s "IOC Alert" admin@example.com
fi

echo "================================" | tee -a /var/log/maintenance.log
```

定期执行：

```bash
# 每周一上午9点执行
0 9 * * 1 /opt/scripts/maintenance.sh
```

## 🔗 相关文档

- [05-startup-config.md](./05-startup-config.md)
- [07-monitoring.md](./07-monitoring.md)
- [08-troubleshooting.md](./08-troubleshooting.md)

---

**总结**: Part 13完成！你已掌握BPMIOC从开发到部署的完整流程。
