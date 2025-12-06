/**
 * @file voltageCalculator.c
 * @brief 波形平均电压计算模块实现（去除本底）
 * @author 你的名字
 * @date 2025
 *
 * 这个模块是新增的功能，用于计算去除本底的波形平均电压
 * 与原有的 driverWrapper.c 代码隔离，便于维护和管理
 */

#include <stdio.h>
#include "voltageCalculator.h"

/* ========== 模块内部变量 ========== */

/* 本底波形范围 */
static int BackGroundStart = 0;
static int BackGroundStop = 0;

/* 信号波形范围（与原有代码共享的参数） */
static int SignalStart = 0;
static int SignalStop = 0;

/* 各通道的平均电压值 (RF3-RF10) */
static float rf3_avg_volt = 0;
static float rf4_avg_volt = 0;
static float rf5_avg_volt = 0;
static float rf6_avg_volt = 0;
static float rf7_avg_volt = 0;
static float rf8_avg_volt = 0;
static float rf9_avg_volt = 0;
static float rf10_avg_volt = 0;

/* ========== 公共接口函数 ========== */

/**
 * @brief 初始化电压计算模块
 */
void voltCalc_init(void)
{
    BackGroundStart = 0;
    BackGroundStop = 0;
    SignalStart = 0;
    SignalStop = 0;

    rf3_avg_volt = 0;
    rf4_avg_volt = 0;
    rf5_avg_volt = 0;
    rf6_avg_volt = 0;
    rf7_avg_volt = 0;
    rf8_avg_volt = 0;
    rf9_avg_volt = 0;
    rf10_avg_volt = 0;

    printf("Voltage Calculator Module Initialized\n");
}

/**
 * @brief 设置本底波形的起始位置
 */
void voltCalc_setBackgroundStart(int start)
{
    BackGroundStart = start;
}

/**
 * @brief 设置本底波形的结束位置
 */
void voltCalc_setBackgroundStop(int stop)
{
    BackGroundStop = stop;
}

/**
 * @brief 设置信号波形的起始位置
 */
void voltCalc_setSignalStart(int start)
{
    SignalStart = start;
}

/**
 * @brief 设置信号波形的结束位置
 */
void voltCalc_setSignalStop(int stop)
{
    SignalStop = stop;
}

/**
 * @brief 计算指定通道的平均电压（信号 - 本底）
 * @param wfBuf 波形数据缓冲区
 * @param ch_N 通道编号 (0,2,4,6,8,10,12,14 对应 RF3-RF10)
 * @param length 波形数据长度
 */
void voltCalc_calculateAvgVoltage(float *wfBuf, int ch_N, int length)
{
    int i = 0;
    float signal_sum = 0;       // 有效信号之和
    float background_sum = 0;   // 本底信号之和
    int signal_count = 0;       // 有效信号点数
    int background_count = 0;   // 本底信号点数
    float avg_volt = 0;         // 平均电压

    /* 遍历波形数据，分别累加信号区和本底区的值 */
    for (i = 0; i < length; i++) {
        if (i >= SignalStart && i <= SignalStop) {
            signal_sum += wfBuf[i];
        }
        if (i >= BackGroundStart && i <= BackGroundStop) {
            background_sum += wfBuf[i];
        }
    }

    signal_count = SignalStop - SignalStart + 1;
    background_count = BackGroundStop - BackGroundStart + 1;

    /* 计算平均电压 = 信号平均值 - 本底平均值 */
    if (signal_count > 0 && background_count > 0) {
        avg_volt = signal_sum / signal_count - background_sum / background_count;
    }
    else {
        avg_volt = 0;
    }

    /* 根据通道编号存储结果 */
    switch (ch_N) {
        case 0:  rf3_avg_volt = avg_volt; break;
        case 2:  rf4_avg_volt = avg_volt; break;
        case 4:  rf5_avg_volt = avg_volt; break;
        case 6:  rf6_avg_volt = avg_volt; break;
        case 8:  rf7_avg_volt = avg_volt; break;
        case 10: rf8_avg_volt = avg_volt; break;
        case 12: rf9_avg_volt = avg_volt; break;
        case 14: rf10_avg_volt = avg_volt; break;
        default:
            printf("voltCalc: Unknown channel %d\n", ch_N);
            break;
    }
}

/**
 * @brief 获取指定通道的平均电压
 * @param channel 通道号 (0-7 对应 RF3-RF10)
 * @return 平均电压值
 */
float voltCalc_getAvgVoltage(int channel)
{
    switch (channel) {
        case 0: return rf3_avg_volt;
        case 1: return rf4_avg_volt;
        case 2: return rf5_avg_volt;
        case 3: return rf6_avg_volt;
        case 4: return rf7_avg_volt;
        case 5: return rf8_avg_volt;
        case 6: return rf9_avg_volt;
        case 7: return rf10_avg_volt;
        default:
            return 0;
    }
}
