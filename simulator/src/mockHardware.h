/**
 * mockHardware.h - Mock Hardware Library Header
 *
 * 提供与真实硬件库(libBPMboard14And15.so)完全兼容的接口
 * 用于PC上的开发和测试，无需真实FPGA硬件
 *
 * Author: BPMIOC Simulator
 * Date: 2025-11-08
 */

#ifndef MOCK_HARDWARE_H
#define MOCK_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 系统初始化和配置
 ******************************************************************************/

/**
 * 初始化模拟硬件系统
 * @return 0=成功, -1=失败
 */
int SystemInit(void);

/**
 * 获取硬件板卡状态
 * @return 0=未初始化, 1=正常, -1=错误
 */
int GetBoardStatus(void);

/**
 * 获取固件版本
 * @param version 版本字符串缓冲区
 * @param maxlen 缓冲区最大长度
 * @return 0=成功, -1=失败
 */
int GetFirmwareVersion(char *version, int maxlen);

/**
 * 系统复位
 * @return 0=成功, -1=失败
 */
int SystemReset(void);

/*******************************************************************************
 * RF数据采集 - 核心功能
 ******************************************************************************/

/**
 * 获取所有RF通道信息 - 最重要的数据采集函数
 *
 * @param Amp   [out] 8个通道的幅度值 (V)
 * @param Phase [out] 8个通道的相位值 (度)
 * @param Power [out] 8个通道的功率值 (W)
 * @param VBPM  [out] BPM电压 (V)
 * @param IBPM  [out] BPM电流 (A)
 * @param VCRI  [out] CRI电压 (V)
 * @param ICRI  [out] CRI电流 (A)
 * @param PSET  [out] 相位设置值 (整数)
 * @return 0=成功, -1=失败
 */
int GetRfInfo(float *Amp, float *Phase, float *Power,
              float *VBPM, float *IBPM,
              float *VCRI, float *ICRI,
              int *PSET);

/**
 * 获取单个通道的幅度
 * @param channel 通道号 (0-7)
 * @param value [out] 幅度值
 * @return 0=成功, -1=失败
 */
int GetChannelAmplitude(int channel, float *value);

/**
 * 获取单个通道的相位
 * @param channel 通道号 (0-7)
 * @param value [out] 相位值
 * @return 0=成功, -1=失败
 */
int GetChannelPhase(int channel, float *value);

/**
 * 获取单个通道的功率
 * @param channel 通道号 (0-7)
 * @param value [out] 功率值
 * @return 0=成功, -1=失败
 */
int GetChannelPower(int channel, float *value);

/*******************************************************************************
 * 波形数据采集
 ******************************************************************************/

/**
 * 获取触发波形数据
 *
 * @param channel 通道号 (0-7)
 * @param buffer [out] 波形数据缓冲区
 * @param npts [out] 实际采样点数
 * @return 0=成功, -1=失败
 */
int GetTrigWaveform(int channel, float *buffer, int *npts);

/**
 * 获取连续波形数据
 * @param channel 通道号 (0-7)
 * @param buffer [out] 波形数据缓冲区
 * @param npts [out] 实际采样点数
 * @return 0=成功, -1=失败
 */
int GetContinuousWaveform(int channel, float *buffer, int *npts);

/*******************************************************************************
 * 参数设置 - 写操作
 ******************************************************************************/

/**
 * 设置相位偏移
 * @param channel 通道号 (0-7)
 * @param value 相位偏移值 (-180 ~ +180度)
 * @return 0=成功, -1=失败
 */
int SetPhaseOffset(int channel, float value);

/**
 * 设置幅度偏移
 * @param channel 通道号 (0-7)
 * @param value 幅度偏移值 (V)
 * @return 0=成功, -1=失败
 */
int SetAmpOffset(int channel, float value);

/**
 * 设置功率阈值
 * @param channel 通道号 (0-7)
 * @param value 功率阈值 (W)
 * @return 0=成功, -1=失败
 */
int SetPowerThreshold(int channel, float value);

/**
 * 设置采样率
 * @param rate 采样率 (Hz)
 * @return 0=成功, -1=失败
 */
int SetSampleRate(int rate);

/**
 * 设置触发模式
 * @param mode 0=内触发, 1=外触发
 * @return 0=成功, -1=失败
 */
int SetTriggerMode(int mode);

/**
 * 设置触发电平
 * @param level 触发电平 (V)
 * @return 0=成功, -1=失败
 */
int SetTriggerLevel(float level);

/*******************************************************************************
 * BPM特定功能
 ******************************************************************************/

/**
 * 获取束流位置
 * @param x [out] X方向位置 (mm)
 * @param y [out] Y方向位置 (mm)
 * @return 0=成功, -1=失败
 */
int GetBeamPosition(float *x, float *y);

/**
 * 获取束流强度
 * @param intensity [out] 强度值
 * @return 0=成功, -1=失败
 */
int GetBeamIntensity(float *intensity);

/**
 * 设置BPM增益
 * @param gain 增益值
 * @return 0=成功, -1=失败
 */
int SetBPMGain(float gain);

/*******************************************************************************
 * 温度和电源监控
 ******************************************************************************/

/**
 * 获取板卡温度
 * @param sensor 传感器编号 (0-3)
 * @param temp [out] 温度值 (摄氏度)
 * @return 0=成功, -1=失败
 */
int GetTemperature(int sensor, float *temp);

/**
 * 获取电源电压
 * @param rail 电源轨编号 (0=+5V, 1=+3.3V, 2=+12V)
 * @param voltage [out] 电压值 (V)
 * @return 0=成功, -1=失败
 */
int GetPowerVoltage(int rail, float *voltage);

/**
 * 获取电源电流
 * @param rail 电源轨编号
 * @param current [out] 电流值 (A)
 * @return 0=成功, -1=失败
 */
int GetPowerCurrent(int rail, float *current);

/*******************************************************************************
 * 诊断和调试
 ******************************************************************************/

/**
 * 设置调试级别
 * @param level 0=OFF, 1=ERROR, 2=INFO, 3=DEBUG, 4=TRACE
 * @return 0=成功, -1=失败
 */
int SetDebugLevel(int level);

/**
 * 获取错误信息
 * @param buffer [out] 错误信息缓冲区
 * @param maxlen 缓冲区最大长度
 * @return 0=成功, -1=失败
 */
int GetLastError(char *buffer, int maxlen);

/**
 * 自检
 * @return 0=通过, -1=失败
 */
int SelfTest(void);

/*******************************************************************************
 * 校准功能
 ******************************************************************************/

/**
 * 执行校准
 * @param channel 通道号 (0-7, -1=所有通道)
 * @return 0=成功, -1=失败
 */
int Calibrate(int channel);

/**
 * 加载校准数据
 * @param filename 校准文件路径
 * @return 0=成功, -1=失败
 */
int LoadCalibration(const char *filename);

/**
 * 保存校准数据
 * @param filename 校准文件路径
 * @return 0=成功, -1=失败
 */
int SaveCalibration(const char *filename);

/*******************************************************************************
 * 模拟器特有功能 (真实硬件库中不存在)
 ******************************************************************************/

/**
 * 设置模拟模式
 * @param mode 0=正弦波, 1=随机噪声, 2=文件回放
 * @return 0=成功, -1=失败
 */
int Mock_SetSimulationMode(int mode);

/**
 * 加载回放数据文件
 * @param filename 数据文件路径
 * @return 0=成功, -1=失败
 */
int Mock_LoadDataFile(const char *filename);

/**
 * 设置故障注入
 * @param fault_type 故障类型
 * @param enable 1=启用, 0=禁用
 * @return 0=成功, -1=失败
 */
int Mock_SetFault(int fault_type, int enable);

/**
 * 获取模拟器统计信息
 * @param buffer [out] 统计信息缓冲区
 * @param maxlen 缓冲区最大长度
 * @return 0=成功, -1=失败
 */
int Mock_GetStatistics(char *buffer, int maxlen);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_HARDWARE_H */
