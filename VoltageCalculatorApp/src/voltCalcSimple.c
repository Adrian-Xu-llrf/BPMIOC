/**
 * @file voltCalcSimple.c
 * @brief 简单的电压计算驱动（使用Channel Access Monitor）
 * @author 你的名字
 * @date 2025-12-06
 *
 * 这是一个完全独立的应用，通过CA Monitor订阅BPM的波形PV
 * 自动计算平均电压并提供给EPICS Record读取
 *
 * 优点：
 * 1. 完全不需要修改BPMmonitor的任何代码
 * 2. 独立运行，可以随时启动/停止
 * 3. 通过CA通信，解耦度高
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsMutex.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <dbAccess.h>
#include <recGbl.h>

#include <cadef.h>  /* Channel Access */

#define MAX_WAVEFORM_SIZE 10000
#define NUM_CHANNELS 8

/* 全局数据结构 */
typedef struct {
    /* Channel Access */
    chid waveformChid[NUM_CHANNELS];
    evid waveformEvid[NUM_CHANNELS];

    /* 配置参数 */
    int signalStart;
    int signalStop;
    int backgroundStart;
    int backgroundStop;

    /* 波形数据缓存 */
    double waveformData[NUM_CHANNELS][MAX_WAVEFORM_SIZE];
    int waveformLength[NUM_CHANNELS];

    /* 计算结果 */
    double avgVoltage[NUM_CHANNELS];

    /* 同步锁 */
    epicsMutexId lock;

    /* 配置信息 */
    char bpmPrefix[256];
    int initialized;

} VoltCalcGlobal;

static VoltCalcGlobal g_voltCalc = {0};

/**
 * @brief CA波形更新回调函数
 */
static void waveformMonitorCallback(struct event_handler_args args)
{
    int channel = (int)(long)args.usr;
    long count = args.count;
    double *pData = (double*)args.dbr;

    if (!g_voltCalc.initialized) return;
    if (channel < 0 || channel >= NUM_CHANNELS) return;

    epicsMutexLock(g_voltCalc.lock);

    /* 复制波形数据 */
    if (count > MAX_WAVEFORM_SIZE) count = MAX_WAVEFORM_SIZE;
    memcpy(g_voltCalc.waveformData[channel], pData, count * sizeof(double));
    g_voltCalc.waveformLength[channel] = count;

    /* 计算平均电压：信号平均值 - 本底平均值 */
    double signalSum = 0.0;
    double backgroundSum = 0.0;
    int signalCount = 0;
    int backgroundCount = 0;

    for (int i = 0; i < count; i++) {
        /* 信号区域 */
        if (i >= g_voltCalc.signalStart && i <= g_voltCalc.signalStop) {
            signalSum += g_voltCalc.waveformData[channel][i];
            signalCount++;
        }
        /* 本底区域 */
        if (i >= g_voltCalc.backgroundStart && i <= g_voltCalc.backgroundStop) {
            backgroundSum += g_voltCalc.waveformData[channel][i];
            backgroundCount++;
        }
    }

    /* 计算去本底后的平均电压 */
    if (signalCount > 0 && backgroundCount > 0) {
        double signalAvg = signalSum / signalCount;
        double backgroundAvg = backgroundSum / backgroundCount;
        g_voltCalc.avgVoltage[channel] = signalAvg - backgroundAvg;
    } else {
        g_voltCalc.avgVoltage[channel] = 0.0;
    }

    epicsMutexUnlock(g_voltCalc.lock);
}

/**
 * @brief 连接到BPM的波形PV并订阅更新
 */
static int connectWaveforms(void)
{
    int status;
    char pvName[256];

    /* 波形PV名称列表 */
    const char *wfNames[NUM_CHANNELS] = {
        "RF3Amp", "RF4Amp", "RF5Amp", "RF6Amp",
        "RF7Amp", "RF8Amp", "RF9Amp", "RF10Amp"
    };

    printf("voltCalc: Connecting to BPM waveforms...\n");

    /* 为每个通道创建连接和订阅 */
    for (int i = 0; i < NUM_CHANNELS; i++) {
        /* 构造PV名称：例如 "BPM:RF3Amp" */
        snprintf(pvName, sizeof(pvName), "%s:%s",
                 g_voltCalc.bpmPrefix, wfNames[i]);

        /* 创建通道 */
        status = ca_create_channel(pvName, NULL, NULL, 0,
                                   &g_voltCalc.waveformChid[i]);
        if (status != ECA_NORMAL) {
            printf("voltCalc: ERROR - Failed to create channel for %s: %s\n",
                   pvName, ca_message(status));
            return -1;
        }

        /* 订阅值变化 */
        status = ca_create_subscription(
            DBR_DOUBLE,           /* 数据类型 */
            0,                    /* 元素数量（0=全部） */
            g_voltCalc.waveformChid[i],  /* 通道ID */
            DBE_VALUE,            /* 事件掩码：值变化 */
            waveformMonitorCallback,  /* 回调函数 */
            (void*)(long)i,       /* 用户数据：通道索引 */
            &g_voltCalc.waveformEvid[i]  /* 事件ID */
        );

        if (status != ECA_NORMAL) {
            printf("voltCalc: ERROR - Failed to subscribe to %s: %s\n",
                   pvName, ca_message(status));
            return -1;
        }

        printf("voltCalc: Subscribed to %s\n", pvName);
    }

    /* 等待所有连接建立 */
    status = ca_pend_io(10.0);
    if (status != ECA_NORMAL) {
        printf("voltCalc: WARNING - Some connections may not be complete: %s\n",
               ca_message(status));
    }

    /* 启动CA事件处理线程 */
    ca_pend_event(0.001);

    return 0;
}

/**
 * @brief 初始化电压计算驱动
 * @param bpmPrefix BPM IOC的PV前缀，例如 "BPM:SYS0:1"
 */
int voltCalcInit(const char *bpmPrefix)
{
    int status;

    if (g_voltCalc.initialized) {
        printf("voltCalc: Already initialized\n");
        return 0;
    }

    if (!bpmPrefix || strlen(bpmPrefix) == 0) {
        printf("voltCalc: ERROR - Invalid BPM prefix\n");
        return -1;
    }

    printf("=====================================\n");
    printf("Voltage Calculator Initialization\n");
    printf("=====================================\n");

    /* 保存配置 */
    strncpy(g_voltCalc.bpmPrefix, bpmPrefix, sizeof(g_voltCalc.bpmPrefix) - 1);

    /* 初始化默认参数 */
    g_voltCalc.signalStart = 0;
    g_voltCalc.signalStop = 100;
    g_voltCalc.backgroundStart = 1600;
    g_voltCalc.backgroundStop = 1650;

    /* 创建互斥锁 */
    g_voltCalc.lock = epicsMutexCreate();
    if (!g_voltCalc.lock) {
        printf("voltCalc: ERROR - Failed to create mutex\n");
        return -1;
    }

    /* 初始化Channel Access上下文 */
    status = ca_context_create(ca_enable_preemptive_callback);
    if (status != ECA_NORMAL) {
        printf("voltCalc: ERROR - Failed to create CA context: %s\n",
               ca_message(status));
        return -1;
    }

    /* 连接到BPM波形 */
    if (connectWaveforms() != 0) {
        printf("voltCalc: ERROR - Failed to connect waveforms\n");
        return -1;
    }

    g_voltCalc.initialized = 1;

    printf("voltCalc: Initialization complete\n");
    printf("  BPM Prefix: %s\n", bpmPrefix);
    printf("  Signal Range: [%d, %d]\n",
           g_voltCalc.signalStart, g_voltCalc.signalStop);
    printf("  Background Range: [%d, %d]\n",
           g_voltCalc.backgroundStart, g_voltCalc.backgroundStop);
    printf("=====================================\n");

    return 0;
}

/**
 * @brief 设置参数
 */
int voltCalcSetParam(const char *paramName, int value)
{
    if (!g_voltCalc.initialized) {
        printf("voltCalc: ERROR - Not initialized\n");
        return -1;
    }

    epicsMutexLock(g_voltCalc.lock);

    if (strcmp(paramName, "signalStart") == 0) {
        g_voltCalc.signalStart = value;
    } else if (strcmp(paramName, "signalStop") == 0) {
        g_voltCalc.signalStop = value;
    } else if (strcmp(paramName, "backgroundStart") == 0) {
        g_voltCalc.backgroundStart = value;
    } else if (strcmp(paramName, "backgroundStop") == 0) {
        g_voltCalc.backgroundStop = value;
    } else {
        epicsMutexUnlock(g_voltCalc.lock);
        printf("voltCalc: ERROR - Unknown parameter: %s\n", paramName);
        return -1;
    }

    epicsMutexUnlock(g_voltCalc.lock);

    printf("voltCalc: Set %s = %d\n", paramName, value);
    return 0;
}

/**
 * @brief 获取平均电压结果
 */
double voltCalcGetAvgVoltage(int channel)
{
    double result = 0.0;

    if (!g_voltCalc.initialized) return 0.0;
    if (channel < 0 || channel >= NUM_CHANNELS) return 0.0;

    epicsMutexLock(g_voltCalc.lock);
    result = g_voltCalc.avgVoltage[channel];
    epicsMutexUnlock(g_voltCalc.lock);

    return result;
}

/* ========== IOC Shell命令注册 ========== */

static const iocshArg voltCalcInitArg0 = {"BPM PV prefix", iocshArgString};
static const iocshArg *voltCalcInitArgs[] = {&voltCalcInitArg0};
static const iocshFuncDef voltCalcInitDef = {"voltCalcInit", 1, voltCalcInitArgs};

static void voltCalcInitCallFunc(const iocshArgBuf *args)
{
    voltCalcInit(args[0].sval);
}

static const iocshArg voltCalcSetParamArg0 = {"Parameter name", iocshArgString};
static const iocshArg voltCalcSetParamArg1 = {"Value", iocshArgInt};
static const iocshArg *voltCalcSetParamArgs[] = {
    &voltCalcSetParamArg0,
    &voltCalcSetParamArg1
};
static const iocshFuncDef voltCalcSetParamDef = {
    "voltCalcSetParam", 2, voltCalcSetParamArgs
};

static void voltCalcSetParamCallFunc(const iocshArgBuf *args)
{
    voltCalcSetParam(args[0].sval, args[1].ival);
}

static void voltCalcRegister(void)
{
    iocshRegister(&voltCalcInitDef, voltCalcInitCallFunc);
    iocshRegister(&voltCalcSetParamDef, voltCalcSetParamCallFunc);
}

epicsExportRegistrar(voltCalcRegister);

/* 导出函数供Device Support使用 */
epicsExportAddress(int, voltCalcSetParam);
epicsExportAddress(double, voltCalcGetAvgVoltage);
