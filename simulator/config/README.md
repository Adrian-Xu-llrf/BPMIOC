# Simulator Configuration Files

本目录包含BPMIOC模拟器的配置文件。

## 配置文件列表

| 文件 | 模式 | 用途 |
|------|------|------|
| `sim_sine.conf` | 正弦波 (MODE=0) | 基础测试、平滑数据流 |
| `sim_random.conf` | 随机噪声 (MODE=1) | 稳定性测试、噪声容忍度 |
| `sim_replay.conf` | 文件回放 (MODE=2) | 真实数据验证（待实现） |
| `fault_injection.conf` | 故障注入 | 异常测试（待实现） |

## 配置文件格式

### 全局配置

```ini
[Global]
sample_rate = 100000000     # 采样率 (Hz)
waveform_points = 10000     # 波形点数
global_drift = 0.100        # 全局漂移率 (%/min)
```

### 通道配置

```ini
[Channel_0]
frequency = 0.500           # 外包络频率 (Hz)
amplitude = 1.000           # 幅度 (V)
offset = 4.000              # 直流偏移 (V)
phase_offset = 0.0          # 相位偏移 (度)
noise_level = 0.020         # 噪声水平 (0.0-1.0，相对幅度)
harmonics = 1               # 启用谐波 (0/1)
harmonic2_amp = 0.100       # 2次谐波幅度（相对于基波）
```

## 使用方法

### 方法1：环境变量

```bash
# 设置模拟模式
export BPMIOC_SIM_MODE=0              # 0=正弦, 1=随机, 2=回放

# 指定配置文件
export BPMIOC_SIM_CONFIG=./config/sim_sine.conf

# 运行IOC
./st.cmd
```

### 方法2：st.cmd内设置

```bash
# 在st.cmd中添加
epicsEnvSet("BPMIOC_SIM_MODE", "0")
epicsEnvSet("BPMIOC_SIM_CONFIG", "$(TOP)/simulator/config/sim_sine.conf")
```

## 参数说明

### 频率参数

- **frequency**: 外包络频率，控制幅度变化速度
  - 推荐范围: 0.1 - 2.0 Hz
  - 太低: 数据变化慢，不易观察
  - 太高: 可能超过IOC更新率

### 幅度参数

- **amplitude**: 正弦波幅度（峰值）
  - 典型值: 0.5 - 2.0 V
  - 最终值 = offset ± amplitude

- **offset**: 直流偏移（中心值）
  - 典型值: 3.0 - 5.0 V
  - 模拟RF信号的工作点

### 噪声参数

- **noise_level**: 噪声强度
  - 0.02 = 2% (低噪声，正常工作)
  - 0.10 = 10% (高噪声，测试稳定性)
  - 0.20 = 20% (极端噪声)

### 谐波参数

- **harmonics**: 是否包含谐波
  - 0 = 纯正弦波
  - 1 = 包含2次和3次谐波

- **harmonic2_amp**: 2次谐波相对幅度
  - 0.10 = 10% (轻微失真)
  - 0.30 = 30% (明显失真)

## 示例场景

### 场景1：正常工作条件

```bash
export BPMIOC_SIM_MODE=0
export BPMIOC_SIM_CONFIG=./config/sim_sine.conf
```

- 低噪声 (2%)
- 平滑变化
- 适合算法验证

### 场景2：恶劣环境

```bash
export BPMIOC_SIM_MODE=1
export BPMIOC_SIM_CONFIG=./config/sim_random.conf
```

- 高噪声 (10%+)
- 较大漂移
- 适合稳定性测试

### 场景3：快速测试

```bash
# 使用默认配置（不指定配置文件）
export BPMIOC_SIM_MODE=0
unset BPMIOC_SIM_CONFIG
```

## 自定义配置

1. 复制现有配置文件：
```bash
cp sim_sine.conf my_config.conf
```

2. 编辑参数

3. 使用自定义配置：
```bash
export BPMIOC_SIM_CONFIG=./config/my_config.conf
```

## 注意事项

⚠️ **配置文件不是必需的** - 如果不指定，使用内置默认值

⚠️ **模式优先级** - BPMIOC_SIM_MODE环境变量优先于配置文件

⚠️ **路径** - 配置文件路径可以是绝对路径或相对路径

⚠️ **格式** - 必须是标准INI格式，# 开头为注释
