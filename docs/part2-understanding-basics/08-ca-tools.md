# 08 - Channel Access 工具

> **目标**: 掌握EPICS Channel Access命令行工具的使用
> **难度**: ⭐⭐☆☆☆
> **预计时间**: 35分钟

## 📋 学习目标

完成本节后，你将能够：
- ✅ 使用caget读取PV值
- ✅ 使用caput写入PV值
- ✅ 使用camonitor监控PV变化
- ✅ 使用cainfo查询PV元数据
- ✅ 在BPMIOC中应用这些工具进行调试和监控

## 🎯 Channel Access 概述

### 什么是Channel Access

**Channel Access (CA)** 是EPICS的网络协议，允许客户端通过网络访问IOC中的PV。

```
客户端（你的电脑）
    ↓
Channel Access (UDP 5064, TCP 5065)
    ↓
IOC（BPMIOC）
    ↓
PV值
```

### 核心工具

EPICS Base提供了4个核心命令行工具：

| 工具 | 用途 | 类比 |
|------|------|------|
| `caget` | 读取PV值 | 类似 `echo $VAR` |
| `caput` | 写入PV值 | 类似 `VAR=value` |
| `camonitor` | 监控PV变化 | 类似 `tail -f` |
| `cainfo` | 查询PV信息 | 类似 `file` 命令 |

## 📖 1. caget - 读取PV值

### 基本用法

```bash
# 最简单：只显示值
caget PV_NAME

# 示例
caget iLinac_007:BPM14And15:RFIn_01_Amp
# 输出: iLinac_007:BPM14And15:RFIn_01_Amp  1.234
```

### 常用选项

#### 1. 显示时间戳 (-t)

```bash
caget -t iLinac_007:BPM14And15:RFIn_01_Amp

# 输出: iLinac_007:BPM14And15:RFIn_01_Amp  2025-11-08 10:30:45.123456 1.234
```

#### 2. 显示报警状态 (-S)

```bash
caget -S iLinac_007:BPM14And15:Temperature1

# 输出（正常）:
# iLinac_007:BPM14And15:Temperature1  25.5 NO_ALARM

# 输出（报警）:
# iLinac_007:BPM14And15:Temperature1  75.2 MINOR HIGH
```

#### 3. 组合：时间戳+报警 (-tS)

```bash
caget -tS iLinac_007:BPM14And15:Temperature1

# 输出:
# iLinac_007:BPM14And15:Temperature1  2025-11-08 10:30:45.123456 25.5 NO_ALARM
```

#### 4. 读取多个PV

```bash
caget iLinac_007:BPM14And15:RFIn_01_Amp \
      iLinac_007:BPM14And15:RFIn_02_Amp \
      iLinac_007:BPM14And15:RFIn_03_Amp

# 输出:
# iLinac_007:BPM14And15:RFIn_01_Amp  1.234
# iLinac_007:BPM14And15:RFIn_02_Amp  2.456
# iLinac_007:BPM14And15:RFIn_03_Amp  1.789
```

#### 5. 读取波形数据 (-#)

```bash
# 读取前10个点
caget -# 10 iLinac_007:BPM14And15:RFIn_01_TrigWaveform

# 读取全部10000个点
caget -# 10000 iLinac_007:BPM14And15:RFIn_01_TrigWaveform

# 输出:
# iLinac_007:BPM14And15:RFIn_01_TrigWaveform 10000 1.234 1.235 1.236 ...
```

#### 6. 显示字段 (-d)

```bash
# 显示数据类型
caget -d DBR_DOUBLE iLinac_007:BPM14And15:RFIn_01_Amp
```

### caget选项速查表

| 选项 | 含义 | 示例 |
|------|------|------|
| (无) | 只显示值 | `caget PV` |
| `-t` | 显示时间戳 | `caget -t PV` |
| `-S` | 显示报警状态 | `caget -S PV` |
| `-tS` | 时间戳+报警 | `caget -tS PV` |
| `-a` | 显示所有字段 | `caget -a PV` |
| `-#` | 读取数组元素数 | `caget -# 100 WAVE_PV` |
| `-w` | 等待超时（秒） | `caget -w 10 PV` |
| `-n` | 不显示PV名称 | `caget -n PV` |

## ✍️ 2. caput - 写入PV值

### 基本用法

```bash
# 基本写入
caput PV_NAME VALUE

# 示例
caput iLinac_007:BPM14And15:SetAlarmThreshold_Ch1 5.5

# 输出:
# Old : iLinac_007:BPM14And15:SetAlarmThreshold_Ch1  3.0
# New : iLinac_007:BPM14And15:SetAlarmThreshold_Ch1  5.5
```

### 常用选项

#### 1. 等待回调 (-c)

```bash
# 等待记录处理完成
caput -c iLinac_007:BPM14And15:TrigSoftWare 1

# 用于确保写入已被处理
```

#### 2. 等待并带超时 (-c -w)

```bash
# 等待最多10秒
caput -c -w 10 iLinac_007:BPM14And15:SetADCSampleNum 1000
```

#### 3. 写入字符串

```bash
# 字符串需要引号
caput iLinac_007:BPM14And15:Description "RF BPM Monitor System"
```

#### 4. 写入数组（波形）

```bash
# 写入多个值（用空格分隔）
caput iLinac_007:BPM14And15:SomeArray 1.0 2.0 3.0 4.0 5.0
```

### 实际应用示例

#### 示例1: 设置调试级别

```bash
# 设置驱动层调试级别为3（TRACE）
caput iLinac_007:BPM14And15:DebugLevel 3

# 验证设置
caget iLinac_007:BPM14And15:DebugLevel
# 输出: iLinac_007:BPM14And15:DebugLevel  3
```

#### 示例2: 触发软件事件

```bash
# 触发软件触发
caput iLinac_007:BPM14And15:TrigSoftWare 1

# 等待触发完成
caput -c iLinac_007:BPM14And15:TrigSoftWare 1
```

#### 示例3: 批量设置阈值

```bash
# 脚本方式批量设置
for i in {1..8}; do
    caput iLinac_007:BPM14And15:RFIn_0${i}_SetThresh_H 8.0
done
```

## 📊 3. camonitor - 监控PV变化

### 基本用法

```bash
# 监控单个PV（Ctrl+C停止）
camonitor iLinac_007:BPM14And15:RFIn_01_Amp

# 输出（持续）:
# iLinac_007:BPM14And15:RFIn_01_Amp  2025-11-08 10:30:45.100  1.234
# iLinac_007:BPM14And15:RFIn_01_Amp  2025-11-08 10:30:45.200  1.235
# iLinac_007:BPM14And15:RFIn_01_Amp  2025-11-08 10:30:45.300  1.237
# ...
```

### 常用选项

#### 1. 监控多个PV

```bash
camonitor iLinac_007:BPM14And15:RFIn_01_Amp \
          iLinac_007:BPM14And15:RFIn_02_Amp \
          iLinac_007:BPM14And15:Temperature1
```

#### 2. 限制监控次数 (-#)

```bash
# 监控100次后自动停止
camonitor -# 100 iLinac_007:BPM14And15:RFIn_01_Amp
```

#### 3. 显示报警 (-S)

```bash
camonitor -S iLinac_007:BPM14And15:Temperature1

# 输出:
# iLinac_007:BPM14And15:Temperature1  25.5 NO_ALARM
# iLinac_007:BPM14And15:Temperature1  71.2 MINOR HIGH  ← 报警触发！
```

#### 4. 简洁输出 (-g)

```bash
# 只显示值，不显示时间戳
camonitor -g10 iLinac_007:BPM14And15:RFIn_01_Amp

# 输出:
# iLinac_007:BPM14And15:RFIn_01_Amp  1.234
# iLinac_007:BPM14And15:RFIn_01_Amp  1.235
```

### 实际应用示例

#### 示例1: 监控RF信号质量

```bash
# 监控RF3的幅度、相位和功率
camonitor -tS \
    iLinac_007:BPM14And15:RFIn_01_Amp \
    iLinac_007:BPM14And15:RFIn_01_Phase \
    iLinac_007:BPM14And15:RF3Power
```

#### 示例2: 检测报警事件

```bash
# 只监控有报警状态的变化
camonitor -S iLinac_007:BPM14And15:Temperature1 | grep -v NO_ALARM
```

#### 示例3: 记录到文件

```bash
# 记录1小时的数据
camonitor -tS iLinac_007:BPM14And15:RFIn_01_Amp > rf3_amp_log.txt &
PID=$!

# 1小时后停止
sleep 3600
kill $PID
```

## 🔍 4. cainfo - 查询PV元数据

### 基本用法

```bash
cainfo iLinac_007:BPM14And15:RFIn_01_Amp
```

**输出示例**:
```
iLinac_007:BPM14And15:RFIn_01_Amp
    State:            connected
    Host:             localhost:5064
    Data type:        DBF_DOUBLE
    Element count:    1
    Native data type: DBF_DOUBLE
    Access:           read, write
    Status:           NO_ALARM
    Severity:         NO_ALARM
    Precision:        3
    Units:            V
    Value:            1.234
    Lo disp limit:    0
    Hi disp limit:    10
    Lo alarm limit:   0.5
    Lo warn limit:    1
    Hi warn limit:    8
    Hi alarm limit:   9
```

### 有用的信息

从cainfo输出中可以了解：
- **State**: PV是否在线（connected/disconnected）
- **Host**: IOC的地址和端口
- **Data type**: 数据类型（DOUBLE, LONG, STRING等）
- **Access**: 访问权限（read, write, read/write）
- **Units**: 工程单位（V, C, W等）
- **Limits**: 显示限值和报警限值

### 实际应用

```bash
# 检查PV是否存在
cainfo iLinac_007:BPM14And15:NonExistentPV

# 输出:
# Channel connect timed out: 'iLinac_007:BPM14And15:NonExistentPV' not found.

# 检查PV是否可写
cainfo iLinac_007:BPM14And15:SetAlarmThreshold_Ch1 | grep Access
# 输出: Access:           read, write
```

## 🛠️ 5. 环境变量配置

### 重要的CA环境变量

| 变量 | 含义 | 默认值 | 示例 |
|------|------|--------|------|
| `EPICS_CA_ADDR_LIST` | CA服务器地址列表 | 广播 | `localhost 192.168.1.100` |
| `EPICS_CA_AUTO_ADDR_LIST` | 自动发现 | YES | `NO` |
| `EPICS_CA_MAX_ARRAY_BYTES` | 最大数组大小 | 16384 | `1000000` |
| `EPICS_CA_CONN_TMO` | 连接超时（秒） | 30 | `10` |
| `EPICS_CA_BEACON_PERIOD` | Beacon周期（秒） | 15 | `15` |

### 配置示例

```bash
# 配置CA连接到特定IOC
export EPICS_CA_ADDR_LIST="localhost"
export EPICS_CA_AUTO_ADDR_LIST=NO

# 增大数组传输限制（用于大波形）
export EPICS_CA_MAX_ARRAY_BYTES=10000000  # 10MB

# 添加到~/.bashrc使其永久生效
echo 'export EPICS_CA_ADDR_LIST="localhost"' >> ~/.bashrc
echo 'export EPICS_CA_AUTO_ADDR_LIST=NO' >> ~/.bashrc
```

### 检查配置

```bash
# 查看当前CA配置
env | grep EPICS_CA

# 输出:
# EPICS_CA_ADDR_LIST=localhost
# EPICS_CA_AUTO_ADDR_LIST=NO
# EPICS_CA_MAX_ARRAY_BYTES=10000000
```

## 🧪 6. 实战练习：BPMIOC监控脚本

### 练习1: 创建RF监控脚本

创建一个脚本监控所有8个RF通道：

```bash
vim monitor_rf.sh
```

```bash
#!/bin/bash

PREFIX="iLinac_007:BPM14And15"

echo "=== RF Monitor ==="
echo "Press Ctrl+C to stop"
echo ""

while true; do
    clear
    echo "=== RF Amplitude and Phase ==="
    date
    echo ""

    for i in {01..08}; do
        amp=$(caget -n -tS ${PREFIX}:RFIn_${i}_Amp)
        phase=$(caget -n ${PREFIX}:RFIn_${i}_Phase)

        printf "RF%-2d: Amp=%s  Phase=%6.1f°\n" $((i)) "$amp" "$phase"
    done

    sleep 1
done
```

```bash
chmod +x monitor_rf.sh
./monitor_rf.sh
```

### 练习2: 报警检测脚本

```bash
vim check_alarms.sh
```

```bash
#!/bin/bash

PREFIX="iLinac_007:BPM14And15"

echo "Checking for alarms..."

# 检查所有RF通道
for i in {01..08}; do
    status=$(caget -tS ${PREFIX}:RFIn_${i}_Amp | awk '{print $NF}')

    if [ "$status" != "NO_ALARM" ]; then
        echo "⚠️  ALARM: RFIn_${i}_Amp is in $status state!"
    fi
done

# 检查温度
for i in {1..4}; do
    temp_status=$(caget -tS ${PREFIX}:Temperature${i} 2>/dev/null | awk '{print $NF}')

    if [ "$temp_status" != "NO_ALARM" ]; then
        echo "🌡️  ALARM: Temperature${i} is in $temp_status state!"
    fi
done

echo "Alarm check complete."
```

### 练习3: 性能测试脚本

测量CA响应时间：

```bash
#!/bin/bash

PREFIX="iLinac_007:BPM14And15"
PV="${PREFIX}:RFIn_01_Amp"
COUNT=100

echo "Testing CA latency for $PV"
echo "Reading $COUNT times..."

start_time=$(date +%s.%N)

for i in $(seq 1 $COUNT); do
    caget -n $PV > /dev/null
done

end_time=$(date +%s.%N)
total_time=$(echo "$end_time - $start_time" | bc)
avg_time=$(echo "scale=3; $total_time / $COUNT * 1000" | bc)

echo "Total time: ${total_time} seconds"
echo "Average latency: ${avg_time} ms per read"
```

## 📊 7. CA工具组合技巧

### 技巧1: 批量读取+格式化

```bash
# 读取所有RF幅度并计算总和
sum=0
for i in {01..08}; do
    val=$(caget -n iLinac_007:BPM14And15:RFIn_${i}_Amp)
    sum=$(echo "$sum + $val" | bc)
done
echo "Total RF amplitude: $sum V"
```

### 技巧2: 监控+邮件报警

```bash
# 监控温度，超限时发送邮件
camonitor -S iLinac_007:BPM14And15:Temperature1 | \
while read line; do
    if echo "$line" | grep -q "MAJOR"; then
        echo "$line" | mail -s "BPMIOC Temperature Alarm" admin@example.com
    fi
done
```

### 技巧3: 数据采集+绘图

```bash
# 采集1分钟数据并绘图
camonitor -# 600 -g10 iLinac_007:BPM14And15:RFIn_01_Amp | \
    awk '{print NR, $2}' > data.txt

# 使用gnuplot绘图
gnuplot <<EOF
set terminal png
set output 'rf3_amp.png'
set title 'RF3 Amplitude vs Time'
set xlabel 'Sample'
set ylabel 'Amplitude (V)'
plot 'data.txt' with lines
EOF
```

## 🔗 相关文档

- [Part 2: 02-key-concepts.md](./02-key-concepts.md) - Channel Access概念
- [Part 12: 01-css-opi.md](../../part12-gui/01-css-opi.md) - 图形界面
- [EPICS Channel Access Reference Manual](https://epics.anl.gov/base/R3-15/6-docs/CAref.html)

## 📝 总结

### 核心工具速查

| 任务 | 命令 |
|------|------|
| 读取值 | `caget PV` |
| 读取+时间戳 | `caget -t PV` |
| 读取+报警 | `caget -tS PV` |
| 写入值 | `caput PV VALUE` |
| 等待写入完成 | `caput -c PV VALUE` |
| 监控变化 | `camonitor PV` |
| 监控N次 | `camonitor -# N PV` |
| 查询信息 | `cainfo PV` |
| 读取波形 | `caget -# COUNT WAVE_PV` |

### 常用组合

```bash
# 快速检查IOC状态
caget -tS iLinac_007:BPM14And15:*Amp

# 监控并记录
camonitor -tS PV > log.txt &

# 测试连接
cainfo PV | head -3

# 批量操作
for pv in PV1 PV2 PV3; do caget $pv; done
```

### 下一步

- [09-database-files.md](./09-database-files.md) - 数据库文件语法
- [Part 8: Labs](../part8-hands-on-labs/) - 实践练习
- [Part 12: GUI](../../part12-gui/) - 图形界面工具

---

**🎉 恭喜！** 你已经掌握了EPICS Channel Access工具的使用！
