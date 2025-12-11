/**
 * @file devVoltCalc.c
 * @brief Device Support for Voltage Calculator
 * @author 你的名字
 * @date 2025-12-06
 *
 * 提供EPICS Record和电压计算驱动之间的接口
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <alarm.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <dbScan.h>
#include <devSup.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <epicsExport.h>

/* 外部函数声明（来自voltCalcSimple.c） */
extern int voltCalcSetParam(const char *paramName, int value);
extern double voltCalcGetAvgVoltage(int channel);

/* ========== ai Record Device Support（读取平均电压） ========== */

static long init_ai_record(aiRecord *pai)
{
    /* 从INP字段解析通道号，格式：@CHANNEL */
    if (pai->inp.type != INST_IO) {
        recGblRecordError(S_db_badField, (void*)pai,
                         "devVoltCalc (init_record) Illegal INP field");
        return S_db_badField;
    }

    /* 解析通道号：@0 到 @7 */
    const char *parm = pai->inp.value.instio.string;
    if (parm[0] != '@') {
        recGblRecordError(S_db_badField, (void*)pai,
                         "devVoltCalc (init_record) Invalid INP format, expect @N");
        return S_db_badField;
    }

    int channel = atoi(parm + 1);
    if (channel < 0 || channel > 7) {
        recGblRecordError(S_db_badField, (void*)pai,
                         "devVoltCalc (init_record) Channel must be 0-7");
        return S_db_badField;
    }

    /* 将通道号存储在dpvt中 */
    pai->dpvt = (void*)(long)channel;

    return 0;
}

static long read_ai(aiRecord *pai)
{
    int channel = (int)(long)pai->dpvt;

    /* 从驱动获取平均电压 */
    double voltage = voltCalcGetAvgVoltage(channel);

    pai->val = voltage;
    pai->udf = 0;

    return 0;
}

/* ai Record的Device Support表 */
struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
} devAiVoltCalc = {
    6,
    NULL,
    NULL,
    init_ai_record,
    NULL,
    read_ai,
    NULL
};

epicsExportAddress(dset, devAiVoltCalc);

/* ========== ao Record Device Support（设置参数） ========== */

static long init_ao_record(aoRecord *pao)
{
    /* 从OUT字段解析参数名，格式：@PARAM_NAME */
    if (pao->out.type != INST_IO) {
        recGblRecordError(S_db_badField, (void*)pao,
                         "devVoltCalc (init_record) Illegal OUT field");
        return S_db_badField;
    }

    const char *parm = pao->out.value.instio.string;
    if (parm[0] != '@') {
        recGblRecordError(S_db_badField, (void*)pao,
                         "devVoltCalc (init_record) Invalid OUT format, expect @PARAM");
        return S_db_badField;
    }

    /* 复制参数名 */
    pao->dpvt = epicsStrDup(parm + 1);

    return 0;
}

static long write_ao(aoRecord *pao)
{
    const char *paramName = (const char*)pao->dpvt;
    int value = (int)pao->val;

    /* 设置参数 */
    if (voltCalcSetParam(paramName, value) != 0) {
        recGblSetSevr(pao, WRITE_ALARM, INVALID_ALARM);
        return -1;
    }

    pao->udf = 0;
    return 0;
}

/* ao Record的Device Support表 */
struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_ao;
    DEVSUPFUN special_linconv;
} devAoVoltCalc = {
    6,
    NULL,
    NULL,
    init_ao_record,
    NULL,
    write_ao,
    NULL
};

epicsExportAddress(dset, devAoVoltCalc);
