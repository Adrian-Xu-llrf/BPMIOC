/* devBPMMonitor.c */
/* Device support module */
/* Author:  Gao    Create Date:  02Nov2021 */
/* The last modified date:  23Dec2021 */
  
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <devLib.h>
#include <cantProceed.h>
#include <epicsExport.h>
#include <epicsMath.h>
#include <epicsTypes.h>

#include <aiRecord.h>
#include <aoRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <waveformRecord.h>

#include "driverWrapper.h"
#include "devBPMMonitor.h"

typedef enum strtype{
	NONE = 0, 
	REG,
	STRING,
	AMP,
	PHASE,
	POWER,
	ARRAY
}strtype_t;

typedef struct {
	strtype_t type;
	unsigned short offset;
	unsigned short channel;
}recordpara_t;

/* ai ***************************************************************/
static long init_record_ai(aiRecord *);
static long read_ai(aiRecord *);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devAi =
{
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai,
    NULL
};
epicsExportAddress(dset, devAi);

/* ao ***************************************************************/
static long init_record_ao(aoRecord *);
static long write_ao(aoRecord *);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
    DEVSUPFUN special_linconv;
} devAo =
{
    6,
    NULL,
    NULL,
    init_record_ao,
    NULL,
    write_ao,
    NULL
};
epicsExportAddress(dset, devAo);

/* bi ***************************************************************/
static long init_record_bi(biRecord *);
static long read_bi(biRecord *);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devBi =
{
    6,
    NULL,
    NULL,
    init_record_bi,
    NULL,
    read_bi,
    NULL
};
epicsExportAddress(dset, devBi);

/* bo ***************************************************************/
static long init_record_bo(boRecord *);
static long write_bo(boRecord *);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
    DEVSUPFUN special_linconv;
} devBo =
{
    6,
    NULL,
    NULL,
    init_record_bo,
    NULL,
    write_bo,
    NULL
};
epicsExportAddress(dset, devBo);

/* Trigger waveform ***************************************************************/
static long init_record_wf(waveformRecord *);
static long read_wf(waveformRecord *);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devTrigWaveform =
{
    6,
    NULL,
    NULL,
    init_record_wf,
    devGetInTrigInfo,
    read_wf,
    NULL
};
epicsExportAddress(dset, devTrigWaveform);

/* Trip History buffer waveform ***************************************************************/

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devHistoryWaveform =
{
    6,
    NULL,
    NULL,
    init_record_wf,
    devGetInTripBufferInfo,
    read_wf,
    NULL
};
epicsExportAddress(dset, devHistoryWaveform);

/* ADC raw data waveform ***************************************************************/

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devADCRawDataWaveform =
{
    6,
    NULL,
    NULL,
    init_record_wf,
    devGetInADCrawBufferInfo,
    read_wf,
    NULL
};
epicsExportAddress(dset, devADCRawDataWaveform);

/***********************************************************************
 *   Routine to parse IO arguments
 **********************************************************************/ 
static int devIoParse(char *string, recordpara_t *recordpara) 
{
	int nchar;
	char typeName[10] = "";
	char *pchar = string, separator;
//	pchar=strchr(string,':');
//	sscanf(string, "%[ADDRSTRING]", typestr);	
//	sscanf(pchar,":%u",&offset);

    /* Get type name */
    nchar = strcspn(pchar, ":");
    strncpy(typeName, pchar, nchar);
    typeName[nchar] = '\0';
    pchar += nchar;
    separator = *pchar++;	
	if (strcmp(typeName, "REG") == 0)
		recordpara->type = REG;	
	else if (strcmp(typeName, "STRING") == 0)
		recordpara->type = STRING;
	else if (strcmp(typeName, "AMP") == 0)
		recordpara->type = AMP;
	else if (strcmp(typeName, "PHASE") == 0)
		recordpara->type = PHASE;
	else if(strcmp(typeName, "POWER") == 0)
		recordpara->type = POWER;
	else if(strcmp(typeName, "ARRAY") == 0)
		recordpara->type = ARRAY;
	else
		recordpara->type = NONE;

    /* Check offset */
    if (separator == ':')
    {
        recordpara->offset = strtol(pchar, &pchar, 0);
        separator = *pchar++;
    }
    else
    {
        recordpara->offset = 0;
    }	

    /* Check channel */
	while(*pchar == ' ' || *pchar == '\t')
		separator = *pchar++;
	if(*pchar == '\0'){
//		nchar = 0;
//		pchar = NULL;
	}else{
		nchar = strcspn(pchar, "=");
		pchar += nchar;
		separator = *pchar++;
		recordpara->channel = strtol(pchar, &pchar, 0);
//		nchar = 0;
//		pchar = NULL;
	}
	
	return 0;
 }

/*********  Support for "I/O Intr" for input records ******************/ 
static long devGetInTrigInfo(int cmd, dbCommon * record,
				  IOSCANPVT * ppvt) 
{
//	recordpara_t * p = record->dpvt;
	*ppvt = devGetInTrigScanPvt();
	return 0;
}

/*********  Support for "I/O Intr" for input records ******************/ 
static long devGetInTripBufferInfo(int cmd, dbCommon * record,
				  IOSCANPVT * ppvt) 
{
//	recordpara_t * p = record->dpvt;
	*ppvt = devGetInTripBufferScanPvt();
	return 0;
}

/*********  Support for "I/O Intr" for input records ******************/ 
static long devGetInADCrawBufferInfo(int cmd, dbCommon * record,
				  IOSCANPVT * ppvt) 
{
//	recordpara_t * p = record->dpvt;
	*ppvt = devGetInADCrawBufferScanPvt();
	return 0;
}

/* ai ***************************************************************/ 
static long init_record_ai(aiRecord *record) 
{
	recordpara_t *priv;
//	int status;
	priv = (recordpara_t *)callocMustSucceed(1, sizeof(recordpara_t),"init_record_ai");
	devIoParse(record->inp.value.instio.string, priv);
	record->dpvt = priv;
	return 0;
}

static long read_ai(aiRecord *record) 
{
	recordpara_t *priv = (recordpara_t *)record->dpvt;
	int ch_N;
	float amp;
	double value;
	switch (priv->type)
	{
		case POWER:
			amp = ReadData(priv->offset, priv->channel, priv->type);
			ch_N = priv->channel + 1;
			value = amp2power(amp, ch_N);
			break;
		case REG:
			value = ReadData(priv->offset, priv->channel, priv->type);
			break;
		case AMP:
			value = ReadData(priv->offset, priv->channel, priv->type);
			break;
		case PHASE:
			value = ReadData(priv->offset, priv->channel, priv->type);
			break;
		default:
			value = 0;
			break;
	}
	record->val = value;	
	record->udf = FALSE;
	return 2;
}

/* ao ***************************************************************/ 
static long init_record_ao(aoRecord *record) 
{
	recordpara_t *priv;
	priv = (recordpara_t *)callocMustSucceed(1, sizeof(recordpara_t),"init_record_ao");
	devIoParse(record->out.value.instio.string, priv);
	record->dpvt = priv;
	return 2;		/* preserve whatever is in the VAL field */
}

 static long write_ao(aoRecord *record) 
{
	recordpara_t *priv = (recordpara_t *)record->dpvt;
	SetReg(priv->offset, priv->channel, record->val);
	return 0;
}

/* bi ***************************************************************/ 
static long init_record_bi(biRecord *record) 
{
	recordpara_t *priv;
//	int status;
	priv = (recordpara_t *)callocMustSucceed(1, sizeof(recordpara_t),"init_record_bi");
	devIoParse(record->inp.value.instio.string, priv);
	record->dpvt = priv;
	return 0;
}

static long read_bi(biRecord *record) 
{
	recordpara_t *priv = (recordpara_t *)record->dpvt;
	float value;
	value = ReadData(priv->offset, priv->channel, priv->type);
//	record->val = (int)value;	
//	record->udf = FALSE;
//	return 2;
	record->rval = (int)value;
	return 0;
}

/* bo ***************************************************************/ 
static long init_record_bo(boRecord *record) 
{
	recordpara_t *priv;
	priv = (recordpara_t *)callocMustSucceed(1, sizeof(recordpara_t),"init_record_ao");
	devIoParse(record->out.value.instio.string, priv);
	record->dpvt = priv;
	return 2;		/* preserve whatever is in the VAL field */
}

 static long write_bo(boRecord *record) 
{
	recordpara_t *priv = (recordpara_t *)record->dpvt;
	float value;
	value = (float)record->val;
	SetReg(priv->offset, priv->channel, value);
	return 0;
}

/* waveform *********************************************************/

static long init_record_wf(waveformRecord *record)
{
	recordpara_t *priv;
	priv = (recordpara_t *)callocMustSucceed(1, sizeof(recordpara_t),"init_record_wf");
	devIoParse(record->inp.value.instio.string, priv);
/* 	printf("recordpara->type:%d\n", priv->type);
	printf("recordpara->offset:%d\n", priv->offset); */
	record->dpvt = priv;
	return 0;
}

static long read_wf(waveformRecord *record)
{
	long long TaiSec = 0;
	int TaiNSec = 0;
	recordpara_t *priv = (recordpara_t *)record->dpvt;
	readWaveform(priv->offset, priv->channel, record->nelm, record->bptr, &TaiSec, &TaiNSec);
	record->time.secPastEpoch=(epicsUInt32)TaiSec;
	record->time.nsec=(epicsUInt32)TaiNSec;
/* 	printf("recordpara->type:%d\n", priv->type);
	printf("recordpara->offset:%d\n", priv->offset); */
	record->nord = record->nelm;
	return 0;
}