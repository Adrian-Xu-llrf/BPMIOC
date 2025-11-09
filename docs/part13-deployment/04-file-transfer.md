# 文件传输

> **目标**: 将编译好的文件传输到目标板
> **难度**: ⭐⭐
> **预计时间**: 1小时

## 📋 传输方法

### 方法1: SCP (推荐)

```bash
# 传输整个bin目录
scp -r bin/linux-arm root@192.168.1.100:/opt/BPMmonitor/bin/

# 传输dbd和db
scp -r dbd db root@192.168.1.100:/opt/BPMmonitor/

# 传输iocBoot
scp -r iocBoot root@192.168.1.100:/opt/BPMmonitor/
```

### 方法2: RSYNC

```bash
# 同步整个目录（增量传输）
rsync -avz --progress \
    bin/linux-arm dbd db iocBoot \
    root@192.168.1.100:/opt/BPMmonitor/
```

### 方法3: NFS挂载（开发测试）

PC端：
```bash
# 安装NFS服务器
sudo apt-get install nfs-kernel-server

# 配置导出
echo "/opt/BPMmonitor 192.168.1.100(rw,sync,no_subtree_check)" >> /etc/exports

# 重启NFS
sudo systemctl restart nfs-kernel-server
```

ARM板端：
```bash
# 挂载NFS
mount -t nfs 192.168.1.1:/opt/BPMmonitor /opt/BPMmonitor
```

### 传输EPICS Base库

```bash
# 传输库文件
scp -r /opt/epics-base/lib/linux-arm root@192.168.1.100:/opt/epics-base/lib/

# 或只传输必需的库
scp /opt/epics-base/lib/linux-arm/*.so* root@192.168.1.100:/opt/epics-base/lib/linux-arm/
```

## 📦 打包部署

```bash
# 创建部署包
cd /opt/BPMmonitor
tar czf bpmioc-deploy.tar.gz \
    bin/linux-arm \
    dbd \
    db \
    iocBoot

# 传输
scp bpmioc-deploy.tar.gz root@192.168.1.100:/tmp/

# 在目标板解压
ssh root@192.168.1.100
cd /opt/BPMmonitor
tar xzf /tmp/bpmioc-deploy.tar.gz
```

## 🔗 相关文档

- [03-target-setup.md](./03-target-setup.md)
- [05-startup-config.md](./05-startup-config.md)
