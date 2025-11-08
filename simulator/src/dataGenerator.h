/**
 * dataGenerator.h - Configurable Data Generator Header
 *
 * 提供灵活的数据生成功能，支持从配置文件加载参数
 *
 * Author: BPMIOC Simulator
 * Date: 2025-11-08
 */

#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 波形配置结构
 ******************************************************************************/

/**
 * 单个通道的波形配置
 */
typedef struct {
    // 基本参数
    double frequency;       // 外包络频率 (Hz)
    double amplitude;       // 幅度 (V)
    double offset;          // 直流偏移 (V)
    double phase_offset;    // 相位偏移 (radians)

    // 噪声配置
    double noise_level;     // 噪声水平 (0.0-1.0, 相对于幅度)
    int noise_type;         // 0=高斯, 1=均匀

    // 谐波配置
    int enable_harmonics;   // 是否包含谐波
    double harmonic2_amp;   // 2次谐波幅度 (相对于基波)
    double harmonic3_amp;   // 3次谐波幅度

    // 调制
    int enable_modulation;  // 是否启用调制
    double mod_frequency;   // 调制频率 (Hz)
    double mod_depth;       // 调制深度 (0.0-1.0)

    // 相位参数
    double phase_drift_rate;  // 相位漂移速率 (deg/s)
    double phase_jitter;      // 相位抖动 (deg RMS)
} ChannelConfig;

/**
 * 全局模拟器配置
 */
typedef struct {
    // 通道配置
    ChannelConfig channels[8];

    // 采样参数
    double sample_rate;     // 采样率 (Hz)
    int waveform_points;    // 波形点数

    // 全局效果
    double global_drift;    // 全局幅度漂移速率 (%/min)
    double temperature_coeff;  // 温度系数 (%/C)

    // 触发配置
    int trigger_mode;       // 0=自由, 1=边沿
    double trigger_level;   // 触发电平 (V)
    int trigger_slope;      // 0=上升沿, 1=下降沿

    // 时间相关
    double start_time;      // 开始时间 (s)
    double current_time;    // 当前时间 (s)
} SimulatorConfig;

/*******************************************************************************
 * API函数
 ******************************************************************************/

/**
 * 初始化数据生成器
 * @return 0=成功, -1=失败
 */
int DataGen_Init(void);

/**
 * 从配置文件加载参数
 * @param filename 配置文件路径
 * @return 0=成功, -1=失败
 */
int DataGen_LoadConfig(const char *filename);

/**
 * 保存配置到文件
 * @param filename 配置文件路径
 * @return 0=成功, -1=失败
 */
int DataGen_SaveConfig(const char *filename);

/**
 * 获取配置（用于修改）
 * @return 配置结构指针
 */
SimulatorConfig* DataGen_GetConfig(void);

/**
 * 生成单个通道的单个采样点
 * @param channel 通道号 (0-7)
 * @param time 时间 (s)
 * @param value [out] 生成的值
 * @return 0=成功, -1=失败
 */
int DataGen_GenerateSample(int channel, double time, float *value);

/**
 * 生成所有通道的RF数据（一次快照）
 * @param Amp [out] 8通道幅度
 * @param Phase [out] 8通道相位
 * @param Power [out] 8通道功率
 * @return 0=成功, -1=失败
 */
int DataGen_GenerateRfSnapshot(float *Amp, float *Phase, float *Power);

/**
 * 生成完整波形
 * @param channel 通道号 (0-7)
 * @param buffer [out] 波形缓冲区
 * @param npts [out] 生成的点数
 * @param max_pts 最大点数
 * @return 0=成功, -1=失败
 */
int DataGen_GenerateWaveform(int channel, float *buffer, int *npts, int max_pts);

/**
 * 重置时间
 */
void DataGen_ResetTime(void);

/**
 * 设置当前时间
 * @param time 时间 (s)
 */
void DataGen_SetTime(double time);

/**
 * 推进时间
 * @param delta_time 时间增量 (s)
 */
void DataGen_AdvanceTime(double delta_time);

/**
 * 获取当前时间
 * @return 当前时间 (s)
 */
double DataGen_GetTime(void);

/*******************************************************************************
 * 辅助函数
 ******************************************************************************/

/**
 * 生成高斯噪声
 * @param mean 均值
 * @param stddev 标准差
 * @return 噪声值
 */
double DataGen_GaussianNoise(double mean, double stddev);

/**
 * 生成均匀噪声
 * @param min 最小值
 * @param max 最大值
 * @return 噪声值
 */
double DataGen_UniformNoise(double min, double max);

#ifdef __cplusplus
}
#endif

#endif /* DATA_GENERATOR_H */
