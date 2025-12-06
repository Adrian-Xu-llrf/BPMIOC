# 平均电压计算说明

本文档概述 `avg_voltage` 模块与 `driverWrapper` 中相关调用方式，便于快速了解改动范围与使用方法。

## 模块职责
- 独立封装平均计算窗口与背景窗口的配置。
- 内部维护每个波形通道的平均电压结果并对外提供查询接口。
- 通过布尔开关控制是否执行平均计算，默认关闭以保持旧行为。

## 关键 API
- `configure_avg_window(start, stop, background_start, background_stop)`: 设置主窗口与背景窗口位置。
- `get_avg_window(...)`: 读取当前窗口配置。
- `set_avg_enabled(enabled)` / `is_avg_enabled()`: 打开或关闭平均计算。
- `update_avg_voltage(ch_id, data, len)`: 针对指定通道在最新波形数据上执行计算并返回结果。
- `get_avg_voltage(ch_id)`: 读取已保存的平均值（不触发重新计算）。

> `ch_id` 与硬件通道一一对应，当前在 `driverWrapper` 中使用的编号为 0、2、4、6、8、10、12、14。

## 寄存器映射
`driverWrapper` 中的 `SetReg` 调用通过以下 offset 配置平均计算行为：
- 20：平均窗口起点
- 21：平均窗口终点
- 27：背景窗口起点
- 28：背景窗口终点
- 29：平均计算使能开关（0 关闭，非 0 开启）

`BPMMonitor.db` 暴露了 `SetAVGStartPosition`、`SetAVGStopPosition`、`EnableAvgCalc` 等 PV，可直接通过 EPICS 进行调试。

## 数据路径
- 在 `readData` / `readWaveform` 中，当开关打开时，会在波形复制后调用 `update_avg_voltage(...)` 更新对应通道的平均值。
- 上层读取平均结果时通过 `case 34`（`get_avg_voltage`）返回，无需接触旧的全局变量。

## 兼容性
- 关闭平均开关时不会修改内部平均值，保持与改动前一致的数据路径。
- 窗口参数在计算时会自动裁剪到当前波形长度，避免越界。
