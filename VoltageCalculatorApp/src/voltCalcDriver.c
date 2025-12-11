/**
 * @file voltCalcDriver.c
 * @brief 电压计算驱动实现（基于asynPortDriver）
 * @author 你的名字
 * @date 2025-12-06
 *
 * 使用EPICS的asyn框架，通过Channel Access订阅BPM波形数据
 * 实现平均电压计算功能
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <asynPortDriver.h>
#include <cadef.h>  /* Channel Access */

#include "voltCalcDriver.h"

#define MAX_WAVEFORM_SIZE 10000

/* 驱动私有数据结构 */
typedef struct {
    const char *portName;
    asynUser *pasynUser;

    /* Channel Access连接 */
    chid waveformChid[8];     /* 8个波形通道 */
    evid waveformEvid[8];     /* 事件订阅ID */

    /* 参数存储 */
    int signalStart;
    int signalStop;
    int backgroundStart;
    int backgroundStop;

    /* 波形数据缓存 */
    double waveformData[8][MAX_WAVEFORM_SIZE];
    int waveformLength[8];

    /* 计算结果 */
    double avgVoltage[8];

    /* 同步 */
    epicsMutexId lock;

    /* asyn参数索引 */
    int paramIndex[PARAM_LAST];

} voltCalcPvt;

/* 静态全局变量 */
static voltCalcPvt *pPvt = NULL;

/* Channel Access回调函数 */
static void waveformCallback(struct event_handler_args args)
{
    if (!pPvt) return;

    int channel = (int)(long)args.usr;
    chtype type = args.type;
    long count = args.count;

    if (type != DBR_DOUBLE) return;

    epicsMutexLock(pPvt->lock);

    /* 复制波形数据 */
    if (count > MAX_WAVEFORM_SIZE) count = MAX_WAVEFORM_SIZE;
    memcpy(pPvt->waveformData[channel], args.dbr, count * sizeof(double));
    pPvt->waveformLength[channel] = count;

    /* 计算平均电压 */
    double signalSum = 0.0;
    double backgroundSum = 0.0;
    int signalCount = 0;
    int backgroundCount = 0;

    for (int i = 0; i < count; i++) {
        if (i >= pPvt->signalStart && i <= pPvt->signalStop) {
            signalSum += pPvt->waveformData[channel][i];
            signalCount++;
        }
        if (i >= pPvt->backgroundStart && i <= pPvt->backgroundStop) {
            backgroundSum += pPvt->waveformData[channel][i];
            backgroundCount++;
        }
    }

    if (signalCount > 0 && backgroundCount > 0) {
        double signalAvg = signalSum / signalCount;
        double backgroundAvg = backgroundSum / backgroundCount;
        pPvt->avgVoltage[channel] = signalAvg - backgroundAvg;
    } else {
        pPvt->avgVoltage[channel] = 0.0;
    }

    /* 更新asyn参数 */
    pasynManager->getAddr(pPvt->pasynUser, &channel);
    asynPortDriver *pDriver = (asynPortDriver*)pPvt->pasynUser->userPvt;
    if (pDriver) {
        pDriver->setDoubleParam(PARAM_AVG_VOLTAGE_0 + channel,
                                pPvt->avgVoltage[channel]);
        pDriver->callParamCallbacks();
    }

    epicsMutexUnlock(pPvt->lock);
}

/* 连接到BPM的波形PV */
static int connectWaveforms(const char *bpmPrefix)
{
    int status;
    char pvName[256];

    const char *wfNames[8] = {
        "rf3amp_wf", "rf4amp_wf", "rf5amp_wf", "rf6amp_wf",
        "rf7amp_wf", "rf8amp_wf", "rf9amp_wf", "rf10amp_wf"
    };

    /* 初始化Channel Access */
    status = ca_context_create(ca_disable_preemptive_callback);
    if (status != ECA_NORMAL) {
        printf("voltCalc: ca_context_create failed: %s\n",
               ca_message(status));
        return -1;
    }

    /* 为每个通道创建连接 */
    for (int i = 0; i < 8; i++) {
        snprintf(pvName, sizeof(pvName), "%s:%s", bpmPrefix, wfNames[i]);

        status = ca_create_channel(pvName, NULL, NULL, 0,
                                   &pPvt->waveformChid[i]);
        if (status != ECA_NORMAL) {
            printf("voltCalc: Failed to create channel for %s: %s\n",
                   pvName, ca_message(status));
            return -1;
        }

        /* 订阅波形数据更新 */
        status = ca_create_subscription(DBR_DOUBLE, 0,
                                       pPvt->waveformChid[i],
                                       DBE_VALUE, waveformCallback,
                                       (void*)(long)i,
                                       &pPvt->waveformEvid[i]);
        if (status != ECA_NORMAL) {
            printf("voltCalc: Failed to subscribe to %s: %s\n",
                   pvName, ca_message(status));
            return -1;
        }

        printf("voltCalc: Subscribed to %s\n", pvName);
    }

    /* 等待连接建立 */
    status = ca_pend_io(5.0);
    if (status != ECA_NORMAL) {
        printf("voltCalc: ca_pend_io failed: %s\n", ca_message(status));
        return -1;
    }

    return 0;
}

/* 配置函数 - 在IOC启动脚本中调用 */
int voltCalcConfigure(const char *portName, const char *bpmPrefix)
{
    if (!portName || !bpmPrefix) {
        printf("voltCalc: Invalid arguments\n");
        return -1;
    }

    /* 分配私有数据结构 */
    pPvt = (voltCalcPvt*)calloc(1, sizeof(voltCalcPvt));
    if (!pPvt) {
        printf("voltCalc: Out of memory\n");
        return -1;
    }

    pPvt->portName = epicsStrDup(portName);

    /* 初始化参数 */
    pPvt->signalStart = 0;
    pPvt->signalStop = 0;
    pPvt->backgroundStart = 1600;
    pPvt->backgroundStop = 1650;

    /* 创建互斥锁 */
    pPvt->lock = epicsMutexCreate();

    /* 连接到BPM波形 */
    if (connectWaveforms(bpmPrefix) != 0) {
        free((void*)pPvt->portName);
        epicsMutexDestroy(pPvt->lock);
        free(pPvt);
        pPvt = NULL;
        return -1;
    }

    printf("voltCalc: Driver configured successfully\n");
    printf("  Port name: %s\n", portName);
    printf("  BPM prefix: %s\n", bpmPrefix);

    return 0;
}

/* IOC shell命令注册 */
static const iocshArg voltCalcConfigureArg0 = {"Port name", iocshArgString};
static const iocshArg voltCalcConfigureArg1 = {"BPM PV prefix", iocshArgString};
static const iocshArg *voltCalcConfigureArgs[] = {
    &voltCalcConfigureArg0,
    &voltCalcConfigureArg1
};

static const iocshFuncDef voltCalcConfigureDef = {
    "voltCalcConfigure",
    2,
    voltCalcConfigureArgs
};

static void voltCalcConfigureCallFunc(const iocshArgBuf *args)
{
    voltCalcConfigure(args[0].sval, args[1].sval);
}

static void voltCalcRegister(void)
{
    iocshRegister(&voltCalcConfigureDef, voltCalcConfigureCallFunc);
}

epicsExportRegistrar(voltCalcRegister);
