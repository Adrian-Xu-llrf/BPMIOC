/**
 * @file voltCalcDriver.h
 * @brief 电压计算驱动头文件
 * @author 你的名字
 * @date 2025-12-06
 *
 * 这是一个完全独立的应用，通过Channel Access读取BPM波形数据
 * 并计算平均电压，不需要修改BPMmonitor的任何代码
 */

#ifndef VOLT_CALC_DRIVER_H
#define VOLT_CALC_DRIVER_H

#include <epicsTypes.h>
#include <asynDriver.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 驱动函数声明 */
int voltCalcConfigure(const char *portName, const char *bpmPrefix);

/* 参数索引 */
typedef enum {
    PARAM_SIGNAL_START,      /* 信号区起始点 */
    PARAM_SIGNAL_STOP,       /* 信号区结束点 */
    PARAM_BACKGROUND_START,  /* 本底区起始点 */
    PARAM_BACKGROUND_STOP,   /* 本底区结束点 */
    PARAM_AVG_VOLTAGE_0,     /* RF3平均电压 */
    PARAM_AVG_VOLTAGE_1,     /* RF4平均电压 */
    PARAM_AVG_VOLTAGE_2,     /* RF5平均电压 */
    PARAM_AVG_VOLTAGE_3,     /* RF6平均电压 */
    PARAM_AVG_VOLTAGE_4,     /* RF7平均电压 */
    PARAM_AVG_VOLTAGE_5,     /* RF8平均电压 */
    PARAM_AVG_VOLTAGE_6,     /* RF9平均电压 */
    PARAM_AVG_VOLTAGE_7,     /* RF10平均电压 */
    PARAM_LAST
} voltCalcParam_t;

#ifdef __cplusplus
}
#endif

#endif /* VOLT_CALC_DRIVER_H */
