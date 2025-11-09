# 故障排查

> **目标**: 快速诊断和解决部署问题
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 按需

## 📋 常见问题

### 问题1: IOC启动失败

**症状**：
```
./st.cmd
Segmentation fault
```

**排查步骤**：

```bash
# 1. 检查文件架构
file bin/linux-arm/BPMmonitor
# 应该是: ELF 32-bit ARM

# 2. 检查库依赖
ldd bin/linux-arm/BPMmonitor
# 查看是否有 "not found"

# 3. 设置库路径
export LD_LIBRARY_PATH=/opt/epics-base/lib/linux-arm:$LD_LIBRARY_PATH

# 4. 使用GDB调试
gdb bin/linux-arm/BPMmonitor
(gdb) run st.cmd
(gdb) backtrace
```

### 问题2: PV无法访问

**症状**：
```
caget LLRF:BPM:RFIn_01_Amp
Channel connect timed out
```

**排查步骤**：

```bash
# 1. 检查IOC是否运行
ps aux | grep BPMmonitor

# 2. 检查网络连通性
ping 192.168.1.100

# 3. 检查CA端口
netstat -an | grep 5064
netstat -an | grep 5065

# 4. 检查防火墙
iptables -L

# 5. 测试本地访问（在ARM板上）
caget LLRF:BPM:RFIn_01_Amp

# 6. 检查CA配置
echo $EPICS_CA_ADDR_LIST
```

### 问题3: 硬件通信失败

**症状**：
```
ERROR: InitDevice: Failed to load library
```

**排查步骤**：

```bash
# 1. 检查驱动库
ls -l /opt/BPMDriver/lib/libBPMDriver.so
file /opt/BPMDriver/lib/libBPMDriver.so

# 2. 检查硬件设备
ls -l /dev/mem
ls -l /dev/uio*

# 3. 检查权限
chmod 666 /dev/mem
chmod 666 /dev/uio0

# 4. 测试硬件访问
./test_hardware
```

### 问题4: 性能不佳

**症状**：
- caget延迟高（>100ms）
- 数据更新慢

**排查步骤**：

```bash
# 1. 检查CPU占用
top -p $(pidof BPMmonitor)

# 2. 检查网络延迟
ping 192.168.1.100

# 3. 检查IOC日志
grep -i "slow" /var/log/bpmioc/ioc.log

# 4. 性能分析
perf record -p $(pidof BPMmonitor) sleep 10
perf report
```

## 🔧 调试技巧

### 远程GDB调试

PC端：

```bash
# 启动gdbserver（在ARM板上）
gdbserver :2345 /opt/BPMmonitor/bin/linux-arm/BPMmonitor st.cmd
```

PC端：

```bash
# 连接gdbserver
arm-linux-gnueabihf-gdb /opt/BPMmonitor/bin/linux-arm/BPMmonitor
(gdb) target remote 192.168.1.100:2345
(gdb) break InitDevice
(gdb) continue
```

### 串口调试

```bash
# 连接串口
screen /dev/ttyUSB0 115200

# 或使用minicom
minicom -D /dev/ttyUSB0 -b 115200
```

## 📚 故障排查清单

- [ ] 检查文件架构匹配
- [ ] 检查库依赖完整
- [ ] 检查网络连通性
- [ ] 检查防火墙规则
- [ ] 检查进程运行状态
- [ ] 检查日志错误信息
- [ ] 检查硬件设备状态
- [ ] 检查权限设置
- [ ] 测试性能指标

## 🔗 相关文档

- [06-system-integration.md](./06-system-integration.md)
- [07-monitoring.md](./07-monitoring.md)
- [Part 10: 01-gdb-debugging.md](../part10-debugging-testing/01-gdb-debugging.md)
