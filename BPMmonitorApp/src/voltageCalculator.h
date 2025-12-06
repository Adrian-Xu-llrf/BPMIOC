/**
 * @file voltageCalculator.h
 * @brief 波形平均电压计算模块（去除本底）
 * @author 你的名字
 * @date 2025
 *
 * 这个模块是新增的功能，用于计算去除本底的波形平均电压
 * 与原有的 driverWrapper.c 代码隔离，便于维护和管理
 */

#ifndef VOLTAGE_CALCULATOR_H
#define VOLTAGE_CALCULATOR_H

/**
 * @brief 初始化电压计算模块
 */
void voltCalc_init(void);

/**
 * @brief 设置本底波形的起始位置
 * @param start 起始点位置
 */
void voltCalc_setBackgroundStart(int start);

/**
 * @brief 设置本底波形的结束位置
 * @param stop 结束点位置
 */
void voltCalc_setBackgroundStop(int stop);

/**
 * @brief 设置信号波形的起始位置（从原有代码继承）
 * @param start 起始点位置
 */
void voltCalc_setSignalStart(int start);

/**
 * @brief 设置信号波形的结束位置（从原有代码继承）
 * @param stop 结束点位置
 */
void voltCalc_setSignalStop(int stop);

/**
 * @brief 计算指定通道的平均电压（信号 - 本底）
 * @param wfBuf 波形数据缓冲区
 * @param ch_N 通道编号 (0,2,4,6,8,10,12,14 对应 RF3-RF10)
 * @param length 波形数据长度
 */
void voltCalc_calculateAvgVoltage(float *wfBuf, int ch_N, int length);

/**
 * @brief 获取指定通道的平均电压
 * @param channel 通道号 (0-7 对应 RF3-RF10)
 * @return 平均电压值
 */
float voltCalc_getAvgVoltage(int channel);

#endif /* VOLTAGE_CALCULATOR_H */
