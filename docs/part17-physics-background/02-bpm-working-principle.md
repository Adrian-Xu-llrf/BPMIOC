# 02. BPM工作原理

> **目标**: 理解BPM如何测量束流位置
> **难度**: ⭐⭐
> **预计时间**: 20分钟

## 1. BPM基本概念

### 1.1 什么是BPM？

**BPM = Beam Position Monitor** (束流位置监测器)

**作用**: 非侵入式测量带电粒子束的横向位置 (x, y)

**类比**:
- 就像高速路上的地感线圈检测车辆位置
- 不接触粒子，通过电磁感应测量

### 1.2 为什么重要？

| 需求 | BPM的作用 |
|------|-----------|
| **轨道稳定** | 检测束流偏离 → 反馈校正 |
| **束流诊断** | 分析束流质量和问题 |
| **机器保护** | 位置超限 → 紧急shutdown |
| **实验质量** | 稳定的束流 → 稳定的实验数据 |

## 2. Button BPM结构

### 2.1 物理结构

```
                俯视图
        ┌──────────────────┐
        │                  │
        │  Button 1   Button 2
        │    (↑)        (→)  │
        │                  │
        │       ●          │  ● = 束流中心
        │     束流         │
        │                  │
        │  Button 4   Button 3
        │    (←)        (↓)  │
        │                  │
        └──────────────────┘

        侧视图
        ═══════╦═══════  真空管道
               ║
         ┌─────╨─────┐
         │  Button   │  电极 (金属片)
         │           │
         └───────────┘
               ║
               ▼ 到电子学
```

**关键组件**:
- 4个金属电极 (buttons) 均匀分布
- 真空管道
- 同轴电缆连接到电子学

### 2.2 工作原理

当束流通过BPM时：

```
束流经过
   ↓
电极上感应出镜像电荷
   ↓
镜像电荷产生电流脉冲
   ↓
电流信号通过电缆传输
   ↓
前端电子学放大、滤波
   ↓
ADC采样、数字处理
   ↓
计算位置 (x, y)
```

## 3. 位置计算原理

### 3.1 基本公式

**假设**: 束流在中心时，4个button信号相等

**位置偏离时**: 距离近的button信号强，远的弱

**归一化差分法**:

```
V1, V2, V3, V4 = 四个button的信号幅度

Σ = V1 + V2 + V3 + V4  (总和)

Δx = V1 + V2 - V3 - V4  (水平差分)
Δy = V1 + V4 - V2 - V3  (垂直差分)

x = K × (Δx / Σ)
y = K × (Δy / Σ)

K = 位置灵敏度系数 (通过标定确定)
```

### 3.2 数值示例

**场景**: 束流偏离中心向右上方

```
Button电压:
V1 = 12.0V  (右上)  ← 最强
V2 = 10.0V  (右下)
V3 = 7.0V   (左下)
V4 = 9.0V   (左上)

计算:
Σ = 12 + 10 + 7 + 9 = 38V

Δx = 12 + 10 - 7 - 9 = 6V  (正值 → 右侧)
Δy = 12 + 9 - 10 - 7 = 4V  (正值 → 上侧)

假设 K = 10mm (标定值):
x = 10 × (6/38) = 1.58mm  (右侧)
y = 10 × (4/38) = 1.05mm  (上侧)
```

### 3.3 代码实现

```python
def calculate_position(v1, v2, v3, v4, sensitivity=10.0):
    """
    计算束流位置

    Args:
        v1, v2, v3, v4: Button信号电压 (V)
        sensitivity: 位置灵敏度 (mm)

    Returns:
        (x, y): 位置坐标 (mm)
    """
    # 归一化差分
    sum_v = v1 + v2 + v3 + v4

    if sum_v < 1e-6:
        raise ValueError("总信号太弱，无法计算位置")

    delta_x = v1 + v2 - v3 - v4
    delta_y = v1 + v4 - v2 - v3

    x = sensitivity * (delta_x / sum_v)
    y = sensitivity * (delta_y / sum_v)

    return x, y

# 使用示例
v1, v2, v3, v4 = 12.0, 10.0, 7.0, 9.0
x, y = calculate_position(v1, v2, v3, v4)
print(f"位置: x={x:.2f}mm, y={y:.2f}mm")
# 输出: 位置: x=1.58mm, y=1.05mm
```

## 4. Stripline BPM

### 4.1 结构差异

```
        Button BPM          Stripline BPM
        ──────────          ─────────────
        ┌────┬────┐         ┌──────────┐
        │ ●1 │ ●2 │         │ ════════ │ 条状电极
        │    │    │         │          │
        │ ●4 │ ●3 │         │ ════════ │
        └────┴────┘         └──────────┘
        点状电极            条状电极 (长度>BPM直径)
```

**优点**:
- 更高的灵敏度
- 更好的高频响应
- 适合短脉冲束流

**缺点**:
- 结构复杂
- 成本高

## 5. RF-BPM vs. Beam Position

### 5.1 两种测量模式

| 模式 | 测量对象 | 频率 | 用途 |
|------|----------|------|------|
| **Beam Position** | 束流包络中心 | 0-几kHz | 慢轨道反馈 |
| **RF-BPM** | RF分量的位置 | RF频率(~500MHz) | 快速反馈 |

### 5.2 RF-BPM原理

```
束流信号 (时域)
    ↓
混频器 (与RF参考混频)
    ↓
I/Q解调
    ↓
提取RF频率分量的幅度和相位
    ↓
计算RF分量的位置
```

**BPMIOC中的体现**:

```c
// BPM数据结构
typedef struct {
    // Beam Position模式
    float position_x;      // 低频位置
    float position_y;

    // RF-BPM模式
    float rf_amplitude;    // RF分量幅度
    float rf_phase;        // RF分量相位
    float rf_position_x;   // RF位置
    float rf_position_y;
} BPMData;
```

## 6. BPM性能指标

### 6.1 关键参数

| 参数 | 典型值 | 物理意义 |
|------|--------|----------|
| **位置分辨率** | 1-10μm | 能分辨的最小位移 |
| **动态范围** | ±10mm | 能测量的位置范围 |
| **带宽** | DC-1MHz | 能响应的频率范围 |
| **线性度** | <1% | 位置与信号的线性关系 |

### 6.2 影响因素

```
测量精度受以下因素影响:

1. 电子学噪声
   SNR越高 → 精度越好

2. 标定精度
   K值不准 → 系统误差

3. 机械稳定性
   BPM位置漂移 → 测量错误

4. 束流强度
   信号太弱 → 噪声主导
```

## 7. BPM数据处理流程

### 7.1 完整链路

```
┌──────────────┐
│   BPM电极    │  4路模拟信号
└──────┬───────┘
       ↓
┌──────────────┐
│  前端放大器  │  放大、滤波
└──────┬───────┘
       ↓
┌──────────────┐
│   ADC采样    │  数字化 (14-16bit, MSPS)
└──────┬───────┘
       ↓
┌──────────────┐
│  FPGA处理    │  I/Q解调、FIR滤波、位置计算
└──────┬───────┘
       ↓
┌──────────────┐
│  BPM IOC     │  数据封装、PV发布
└──────┬───────┘
       ↓
┌──────────────┐
│  上层应用    │  轨道反馈、数据分析
└──────────────┘
```

### 7.2 FPGA中的位置计算

```verilog
// 伪代码：FPGA中的位置计算逻辑
module position_calculator(
    input [15:0] adc_ch1,
    input [15:0] adc_ch2,
    input [15:0] adc_ch3,
    input [15:0] adc_ch4,
    output [31:0] position_x,
    output [31:0] position_y
);

    // 求和
    wire [17:0] sum = adc_ch1 + adc_ch2 + adc_ch3 + adc_ch4;

    // 差分
    wire [16:0] diff_x = (adc_ch1 + adc_ch2) - (adc_ch3 + adc_ch4);
    wire [16:0] diff_y = (adc_ch1 + adc_ch4) - (adc_ch2 + adc_ch3);

    // 除法 (定点数)
    // position = (diff / sum) * sensitivity
    assign position_x = (diff_x << 16) / sum * SENSITIVITY;
    assign position_y = (diff_y << 16) / sum * SENSITIVITY;

endmodule
```

## 8. 实际应用中的考虑

### 8.1 标定 (Calibration)

**问题**: 如何确定灵敏度系数 K？

**方法1**: 移动束流标定
- 使用校正磁铁移动束流
- 记录位置与BPM读数的关系
- 拟合得到 K

**方法2**: 离线标定
- 使用精密移动台和信号源
- 模拟束流信号
- 测量响应曲线

### 8.2 软件中的标定处理

```python
# 标定数据存储
calibration = {
    'BPM01': {
        'sensitivity_x': 10.5,  # mm
        'sensitivity_y': 10.3,  # mm
        'offset_x': 0.05,       # mm (机械对中误差)
        'offset_y': -0.03,      # mm
    },
    # ... 其他BPM
}

def apply_calibration(raw_x, raw_y, bpm_name):
    """应用标定参数"""
    cal = calibration[bpm_name]

    x = raw_x * cal['sensitivity_x'] + cal['offset_x']
    y = raw_y * cal['sensitivity_y'] + cal['offset_y']

    return x, y
```

### 8.3 数据质量检查

```c
// IOC中的数据质量检查
int ValidateBPMData(BPMData *data) {
    // 检查1: 信号总和是否足够
    float sum = data->v1 + data->v2 + data->v3 + data->v4;
    if (sum < MIN_SIGNAL_SUM) {
        errlogPrintf("BPM: Signal too weak, sum=%.2f\n", sum);
        return -1;
    }

    // 检查2: 位置是否在合理范围
    if (fabs(data->position_x) > BPM_APERTURE ||
        fabs(data->position_y) > BPM_APERTURE) {
        errlogPrintf("BPM: Position out of range, x=%.2f, y=%.2f\n",
                    data->position_x, data->position_y);
        return -1;
    }

    // 检查3: 位置变化是否过快
    float dx = data->position_x - prev_x;
    float dy = data->position_y - prev_y;
    float velocity = sqrt(dx*dx + dy*dy) / SAMPLE_PERIOD;

    if (velocity > MAX_BEAM_VELOCITY) {
        errlogPrintf("BPM: Position change too fast, v=%.2f mm/s\n",
                    velocity);
        return -1;
    }

    return 0;
}
```

## 9. 小结

### 关键要点

1. **BPM = 非侵入式位置测量**
   - 通过电磁感应检测
   - 4个button信号 → 计算位置

2. **归一化差分法**
   - 消除束流强度影响
   - x = K × (Δx / Σ)

3. **软件开发者关注点**
   - 理解数据来源（ADC → FPGA → IOC）
   - 实现标定和数据质量检查
   - 性能优化（高速采样）

4. **物理约束**
   - 位置范围: ±BPM孔径
   - 分辨率: μm级
   - 更新率: kHz级

### 下一步

继续学习 [03-RF信号基础](./03-rf-signal-basics.md) 了解RF信号的幅度、相位等参数。

## 📚 延伸阅读

- [BPM Technology Overview (CERN)](https://indico.cern.ch/event/390748/)
- [Beam Position Monitors (维基百科)](https://en.wikipedia.org/wiki/Beam_position_monitor)
