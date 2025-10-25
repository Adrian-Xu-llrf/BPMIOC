/* driverWrapper.c */
/* Author:  Gao    Create Date:  02Nov2021 */
/* The last modified date:  04Apr2025 */
// 2025.04.04
// Revise waveform channel order;

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>  //The standard unix I/O, include sleep function.
#include <pthread.h>
#include <dlfcn.h>

#include <drvSup.h>
#include <epicsExport.h>

#include "driverWrapper.h"

typedef uint64_t U64;
typedef uint32_t U32;

#define DLL_FILE_NAME "liblowlevel.so"

#define buf_len 10000
#define trip_buf_len 100000
// #define adcRawdata_buf_len 13000

#define CSVfile_Path "/mnt/BPM_2bpmIn1Chassis_ioc/parameter/llrfparameters.csv"

static IOSCANPVT TriginScanPvt;
static IOSCANPVT TripBufferinScanPvt;
static IOSCANPVT ADCrawBufferinScanPvt;

static double parameters[9][7]={0};

// static float rf1amp[buf_len];
// static float rf2amp[buf_len];
static float rf3amp[buf_len];
static float rf4amp[buf_len];
static float rf5amp[buf_len];
static float rf6amp[buf_len];
static float rf7amp[buf_len];
static float rf8amp[buf_len];
static float rf9amp[buf_len];
static float rf10amp[buf_len];
// static float rf1phase[buf_len];
// static float rf2phase[buf_len];
static float rf3phase[buf_len];
static float rf4phase[buf_len];
static float rf5phase[buf_len];
static float rf6phase[buf_len];
static float rf7phase[buf_len];
static float rf8phase[buf_len];
static float rf9phase[buf_len];
static float rf10phase[buf_len];

static float X1[buf_len];
static float Y1[buf_len];
static float X2[buf_len];
static float Y2[buf_len];

// static float rf1amp_trip[trip_buf_len];
// static float rf2amp_trip[trip_buf_len];
// static float rf3amp_trip[trip_buf_len];
// static float rf4amp_trip[trip_buf_len];
// static float rf5amp_trip[trip_buf_len];
// static float rf6amp_trip[trip_buf_len];
// static float rf7amp_trip[trip_buf_len];
// static float rf8amp_trip[trip_buf_len];
// static float rf9amp_trip[trip_buf_len];
// static float rf10amp_trip[trip_buf_len];

static float HistoryX1[trip_buf_len];
static float HistoryY1[trip_buf_len];
static float HistoryX2[trip_buf_len];
static float HistoryY2[trip_buf_len];

// static float rf1phase_trip[trip_buf_len];
// static float rf2phase_trip[trip_buf_len];
// static float rf3phase_trip[trip_buf_len];
// static float rf4phase_trip[trip_buf_len];
// static float rf5phase_trip[trip_buf_len];
// static float rf6phase_trip[trip_buf_len];
// static float rf7phase_trip[trip_buf_len];
// static float rf8phase_trip[trip_buf_len];

// static int ADC1_rawdata[adcRawdata_buf_len];
// static int ADC2_rawdata[adcRawdata_buf_len];
// static int ADC3_rawdata[adcRawdata_buf_len];
// static int ADC4_rawdata[adcRawdata_buf_len];
// static int ADC5_rawdata[adcRawdata_buf_len];
// static int ADC6_rawdata[adcRawdata_buf_len];
// static int ADC7_rawdata[adcRawdata_buf_len];
// static int ADC8_rawdata[adcRawdata_buf_len];

// static float *rf1amp_trip, *rf2amp_trip, *rf3amp_trip, *rf4amp_trip, *rf5amp_trip, *rf6amp_trip, *rf7amp_trip, *rf8amp_trip, *rf1phase_trip, *rf2phase_trip, *rf3phase_trip, *rf4phase_trip, *rf5phase_trip, *rf6phase_trip, *rf7phase_trip, *rf8phase_trip;

static int ReadWfActionCounter=0;
// static int historyDataFlag=0;

static int pulseMode=0;
static int AVGStart=0;
static int AVGStop=0;

static int BackGroundStart=0;
static int BackGroundStop=0;

static int rf4_avg_volt=0;
static int rf5_avg_volt=0;
static int rf6_avg_volt=0;
static int rf7_avg_volt=0;
static int rf8_avg_volt=0;
static int rf9_avg_volt=0;
static int rf10_avg_volt=0;



static float X1_avg=0;
static float Y1_avg=0;
static float X2_avg=0;
static float Y2_avg=0;

static float ph_ch3=0;
static float ph_offset3=0;
static float ph_ch4=0;
static float ph_offset4=0;
static float ph_ch5=0;
static float ph_offset5=0;
static float ph_ch6=0;
static float ph_offset6=0;
static float ph_ch7=0;
static float ph_offset7=0;
static float ph_ch8=0;
static float ph_offset8=0;
static float ph_ch9=0;
static float ph_offset9=0;
static float ph_ch10=0;
static float ph_offset10=0;

static long long TAISecond=0;
static int TAINanoSecond=0;

static long InitDevice(); 

struct {
    long number;
    DRVSUPFUN report;
    DRVSUPFUN init;
} drWrapper = {
    2,
    NULL,
    InitDevice
};
epicsExportAddress(drvet, drWrapper);

static void (*funcGetRfInfo)(int channel, float* amp, float* phase);
static void (*funcGetDI)(int channel, int* value);
static void (*funcSetDO)(int channel, int value);
static int   (*funcGetFPGA_LED0_RBK)(void);
static int   (*funcGetFPGA_LED1_RBK)(void);
static void  (*funcSetOutputPulseEnable)(int value);
static void  (*funcSetInnerTrigEn)(int value);
static int   (*funcGetHistoryDataReady)(void);
static void (*funcSetHistoryTrigger)(int enable);
static void (*funcSetResetHistoryStorage)(int enable);
static void  (*funcSetTriggerExtractDataRatio)(float value);
static void  (*funcSetHistoryExtractDataRatio)(float value);//?
//static void (*funcGetHistoryData)(float *rf1amp,float *rf1phase,float *rf2amp,float *rf2phase,float *rf3amp,float *rf3phase,float *rf4amp,float *rf4phase,float *rf5amp,float *rf5phase,float *rf6amp,float *rf6phase,float *rf7amp,float *rf7phase,float *rf8amp,float *rf8phase);
//static void (*funcGetTriggerData)(float *rf1amp,float *rf1phase,float *rf2amp,float *rf2phase,float *rf3amp,float *rf3phase,float *rf4amp,float *rf4phase,float *rf5amp,float *rf5phase,float *rf6amp,float *rf6phase,float *rf7amp,float *rf7phase,float *rf8amp,float *rf8phase);
static void  (*funcSetSyncIQStartSign)(int value);
//static void (*funcGetTriggerAdcData)(int *ADC1_rawdata,int *ADC2_rawdata,int *ADC3_rawdata,int *ADC4_rawdata,int *ADC5_rawdata,int *ADC6_rawdata,int *ADC7_rawdata,int *ADC8_rawdata);
static int (*funcHistoryChannelDataReached)(void);
static void (*funcGetHistoryChannelData)(int channel,float *data);
//static int (*funcTriggerChannelDataReached)(void);
//static int (*funcADCChannelDataReached)(void);
//static void (*funcGetTriggerChannelData)(int channel,float *data);
//static void (*funcGetADCChannelData)(int channel,float *data);
static int (*funcTriggerAllDataReached)(void);
static void (*funcGetTriggerAllData)(int sel, int channel,float *data);
static int (*funcGetADclkState)(void);
static void  (*funcSetArmLedEnable)(int enable);
static void  (*funcSetFanLedStatus)(int value);
/*-------------------------BPM functions-----------------------------------*/
static int (*funcGetVcValue)(int channel);
static float (*funcGetBPMPhaseValue)(int channel);
static int (*funcGetxyPosition)(int channel);
static int (*funcGetVcSumValue)(int channel);
static int (*funcGetxyProtect)(int channel);
static void (*funcSetBPMk1)(int channel, float value);
static void (*funcSetBPMk2)(int channel, float value);
static void (*funcSetBPMk3)(int channel, float value);
static void (*funcSetBPMPhaseOffset)(int channel, float value);
static void (*funcSetBPMkxy)(int channel, int value);
static void (*funcSetBPMxyOffset)(int channel, int value);
static void (*funcSetBPMxyLimits)(int channel, int value);
static void (*funcSetReset)(int value);
static void (*funcSetBPMSumLimits)(int channel, int value);
static int (*funcGetSumProtect)(int channel);
static void  (*funcSetBPMProtectFilterTime)(float value);
//----------White Rabbit timestamp and err status test interface----------
static int (*funcGetWRStatus)(int ch);
static void (*funcSetWRCaputureDataTrigger)(void);
static void (*funcGetTimestampData)(int ch, long long *tm_utc, int *pps);
//----------White Rabbit timestamp and err status test interface----------
static void (*funcSetFreqControlWordtoDDS)(int value);
static void (*funcSetSelectExternelTrigger)(int value);
static void SetSysTime(void);

// calculate average voltage of each channel
static void calculateAvgVoltage(float *wfBuf, int ch_N, int length);

static long InitDevice()
{
	printf("## 7100-10ADC RK BPM IOC_20250830\n");
	printf("############################################################################\n");
	void *handle;
	int (*funcOpen)();

	handle = dlopen(DLL_FILE_NAME, RTLD_NOW);
	if (handle == NULL)
	{
		fprintf(stderr, "Failed to open libaray %s error:%s\n", DLL_FILE_NAME, dlerror());
		return -1;
	}

	funcOpen = dlsym(handle, "SystemInit");
	int result = funcOpen();
	if(result == 0)
	{
		printf("Open System success!\n");
	}

	funcGetRfInfo = dlsym(handle, "GetRfInfo");
	funcGetDI = dlsym(handle, "GetDI");
	funcSetDO = dlsym(handle, "SetDO");
	funcGetFPGA_LED0_RBK = dlsym(handle, "GetFPGA_LED0");
	funcGetFPGA_LED1_RBK = dlsym(handle, "GetFPGA_LED1");
	funcSetArmLedEnable = dlsym(handle, "SetArmLedEnable");
	funcSetFanLedStatus = dlsym(handle, "SetFanLedStatus");
	funcSetOutputPulseEnable = dlsym(handle, "SetOutputPulseEnable");  //RF pulse switch.
	funcSetInnerTrigEn = dlsym(handle, "SetInnerTrigEn");  //Select Inner or external Trig Enable to collect data.
	funcGetHistoryDataReady = dlsym(handle, "GetStorageDataReady");
	funcSetHistoryTrigger = dlsym(handle, "SetHistoryTrigger");
	funcSetResetHistoryStorage = dlsym(handle, "SetRsetDataStorage");
	funcSetTriggerExtractDataRatio = dlsym(handle, "SetTriggerExtractDataRatio");
	funcSetHistoryExtractDataRatio = dlsym(handle, "SetHistoryExtractDataRatio");
//	funcGetHistoryData = dlsym(handle, "GetHistoryData");
//	funcGetTriggerData = dlsym(handle, "GetTriggerData");
	funcSetSyncIQStartSign = dlsym(handle, "SetChangeStartIQSig");
//	funcGetTriggerAdcData = dlsym(handle, "GetTriggerAdcData");
	funcHistoryChannelDataReached = dlsym(handle, "HistoryChannelDataReached");
	funcGetHistoryChannelData = dlsym(handle, "GetHistoryChannelData");
//	funcTriggerChannelDataReached = dlsym(handle, "TriggerChannelDataReached");
//	funcADCChannelDataReached = dlsym(handle, "ADCChannelDataReached");
//	funcGetTriggerChannelData = dlsym(handle, "GetTriggerChannelData");
//	funcGetADCChannelData = dlsym(handle, "GetADCChannelData");
	funcTriggerAllDataReached = dlsym(handle, "TriggerAllDataReached");
	funcGetTriggerAllData = dlsym(handle, "GetTriggerAllData");
	funcGetADclkState = dlsym(handle, "GetPlBrokenState");
	funcGetVcValue = dlsym(handle, "GetVcValue");
	funcGetBPMPhaseValue = dlsym(handle, "GetBPMPhaseValue");
	funcGetxyPosition = dlsym(handle, "GetxyPosition");
	funcGetVcSumValue = dlsym(handle, "GetVcSumValue");
	funcGetxyProtect = dlsym(handle, "GetxyProtect");
	funcSetBPMk1 = dlsym(handle, "SetBPMk1");
	funcSetBPMk2 = dlsym(handle, "SetBPMk2");
	funcSetBPMk3 = dlsym(handle, "SetBPMk3");
	funcSetBPMPhaseOffset = dlsym(handle, "SetBPMPhaseOffset");
	funcSetBPMkxy = dlsym(handle, "SetBPMkxy");
	funcSetBPMxyOffset = dlsym(handle, "SetBPMxyOffset");
	funcSetBPMxyLimits = dlsym(handle, "SetBPMxyLimits");
	funcSetReset = dlsym(handle, "SetReset");
	funcSetBPMSumLimits = dlsym(handle, "SetBPMSumLimits");
	funcGetSumProtect = dlsym(handle, "GetSumProtect");
	funcSetBPMProtectFilterTime = dlsym(handle, "SetBPMProtectFilterTime");
	funcGetWRStatus = dlsym(handle, "GetWRStatus");
	funcSetWRCaputureDataTrigger = dlsym(handle, "SetWRCaputureDataTrigger");
	funcGetTimestampData = dlsym(handle, "GetTimestampData");
	funcSetFreqControlWordtoDDS = dlsym(handle, "SetFreqControlWordtoDDS");
	funcSetSelectExternelTrigger = dlsym(handle, "SetSelectExternelTrigger");

	pthread_t tidp1;
	if(pthread_create(&tidp1, NULL, pthread, NULL) == -1)
	{
		printf("create thread1 error!\n");
		return -1;
	}
	
	scanIoInit(&TriginScanPvt);
	scanIoInit(&TripBufferinScanPvt);
	scanIoInit(&ADCrawBufferinScanPvt);
	
	return 0;
}

/* The following functions can only be used in driver layer.***********/ 
static float GetRFInfo(int channel, int type);

static float GetDI(int channel);

static void SetDO(int channel, int value);

static float GetFPGA_LED0_RBK();

static float GetFPGA_LED1_RBK();

static void SetSysLedEnable(int enable);

static void SetFanLedStat(int value);

static void SetPulsecw(unsigned short value);

static void  SetInnerTrigEn(int value);

static float GetHistoryDataReady();

static int SetHistoryTrigger(int enable);

static void  SetResetHistoryStorage(int value);

static void  SetTriggerExtractDataRatio(float value);

static void  SetHistoryExtractDataRatio(float value);

// static void GetHistoryData(float *rf1amp,float *rf1phase,float *rf2amp,float *rf2phase,float *rf3amp,float *rf3phase,float *rf4amp,float *rf4phase,float *rf5amp,float *rf5phase,float *rf6amp,float *rf6phase,float *rf7amp,float *rf7phase,float *rf8amp,float *rf8phase);

// static void GetTriggerData(float* rf1amp, float* rf1phase, float* rf2amp, float* rf2phase, float* rf3amp, float* rf3phase, 
			// float* rf4amp, float* rf4phase, float* rf5amp, float* rf5phase, float* rf6amp, float* rf6phase, 
			// float* rf7amp, float* rf7phase, float* rf8amp, float* rf8phase);

// static void GetTriggerAdcData(int *ADC1_rawdata,int *ADC2_rawdata,int *ADC3_rawdata,int *ADC4_rawdata,int *ADC5_rawdata,int *ADC6_rawdata,int *ADC7_rawdata,int *ADC8_rawdata);

static void  SetSyncIQStartSign(int value);

static int HistoryDataUploadReady(void);

static void GetHistoryDataFromSingleCh(int channel,float *data);

static void  ReadCSVparametersfile(int value);

static void SetOffset(int row, double value);

static void GetSysTime(void);

// static void copyArray(float *dmaBuf, float *wfBuf, int length);

static void copyArray(float *dmaBuf, float *wfBuf, int ch_N, int length);

static void copyXYArray(float *dmaBuf, float *wfBuf, int ch_N, int length);

static void copyHistoryArray(float *dmaBuf, float *wfBuf, int ch_N, int length);

static void copyHistoryXYArray(float *dmaBuf, float *wfBuf, int ch_N, int length);

static void copyArray2Power(float *dmaBuf, float *wfBuf, int length, int Ch_N);

static void copyPhArray(float *dmaBuf, float *wfBuf, int ch_N, int length);

// static void copyADCrawData(int *dmaBuf, float *wfBuf, int length);

/*-----------------------BPM function--------------------------*/
static int GetVabcdValue(int channel);

static int GetXYPosition(int channel);

static int GetVsumValue(int channel);

static void SetBPMk1(int channel, float value);

static void SetBPMk2(int channel, float value);

static void SetBPMkxy(int channel, float value);

static void SetBPMxyOffset(int channel, int value);

static void SetBPMxyLimits(int channel, int value);

static void SetBPMSumLimits(int channel, int value);

static float GetPhOnFlattop(int channel);

static void SetFastIntlkFilterTime(float value);

static void SetDDSMode(int value);

static void SelectTriggerSource(int value);

void *pthread()
{
	while(1)
	{
//		funcTriggerChannelDataReached();
		funcTriggerAllDataReached();
		scanIoRequest(TriginScanPvt);
		funcGetTimestampData(1, &TAISecond, &TAINanoSecond);
		funcSetWRCaputureDataTrigger();
		usleep(100000);
//		GetTriggerData(rf1amp,rf1phase,rf2amp,rf2phase,rf3amp,rf3phase,rf4amp,rf4phase,rf5amp,rf5phase,rf6amp,rf6phase,rf7amp,rf7phase,rf8amp,rf8phase);
//		GetTriggerAdcData(ADC1_rawdata, ADC2_rawdata, ADC3_rawdata, ADC4_rawdata, ADC5_rawdata, ADC6_rawdata, ADC7_rawdata, ADC8_rawdata);
		// usleep(200000);
		// funcADCChannelDataReached();
		// scanIoRequest(ADCrawBufferinScanPvt);
	}
}

IOSCANPVT devGetInTrigScanPvt()
{
	return TriginScanPvt;
}

IOSCANPVT devGetInTripBufferScanPvt()
{
	return TripBufferinScanPvt;
}

IOSCANPVT devGetInADCrawBufferScanPvt()
{
	return ADCrawBufferinScanPvt;
}

float ReadData(int offset, int channel, int type)
{
	switch(offset)
	{
		case 0:
			if(type == 3){
//				SetSysTime();
				return GetRFInfo(channel, 0);  //Return amp value.
			}else if(type == 4){
//				if(pulseMode==0)
					return GetRFInfo(channel, 1);  //Return phase value.
//				else{
//					if((channel != 0) && (channel != 1)){
//						float ph_t;
//						ph_t = GetPhOnFlattop(channel);
//						if(ph_t>180)
//							return (ph_t - 360);
//						else if(ph_t<-180)
//							return (ph_t + 360);
//						else
//							return ph_t;
//					}else{
//						return GetRFInfo(channel, 1); 
//					}
//				}
			}else{
				return GetRFInfo(channel, 0);  //Return amp value.
			}
		case 1:
			return GetDI(channel);
		case 2:
			return GetFPGA_LED0_RBK();
		case 3:
			return GetFPGA_LED1_RBK();
		case 4:
			return GetHistoryDataReady();
		case 5:
			return GetVabcdValue(channel);
		case 6:
//			return funcGetBPMPhaseValue(channel);
			if(pulseMode==0)
//					return GetRFInfo(channel, 1);  //Return phase value.
				return funcGetBPMPhaseValue(channel-2);	
			else{
				if((channel != 0) && (channel != 1)){
					float ph_t;
					ph_t = GetPhOnFlattop(channel);
					if(ph_t>180)
						return (ph_t - 360);
					else if(ph_t<-180)
						return (ph_t + 360);
					else
						return ph_t;
				}else{
					return GetRFInfo(channel, 1); 
				}
			}
		case 7:
			return GetXYPosition(channel);
		case 8:
			return GetVsumValue(channel);
		case 9:
			return funcGetxyProtect(channel);
		case 10:
			return (GetVabcdValue(0) - GetVabcdValue(2));
		case 11:
			return (GetVabcdValue(1) - GetVabcdValue(3));
		case 12:
			return (GetVabcdValue(4) - GetVabcdValue(6));
		case 13:
			return (GetVabcdValue(5) - GetVabcdValue(7));
		case 14:
			return (GetVabcdValue(0)+GetVabcdValue(1)+GetVabcdValue(2)+GetVabcdValue(3));
		case 15:
			return (GetVabcdValue(4)+GetVabcdValue(5)+GetVabcdValue(6)+GetVabcdValue(7));
		case 16:
			return (GetVabcdValue(0) + GetVabcdValue(2));
		case 17:
			return (GetVabcdValue(1) + GetVabcdValue(3));
		case 18:
			return (GetVabcdValue(4) + GetVabcdValue(6));
		case 19:
			return (GetVabcdValue(5) + GetVabcdValue(7));
		case 20:
			return ((float)(GetVabcdValue(0) - GetVabcdValue(2)) / (float)(GetVabcdValue(0) + GetVabcdValue(2)));
		case 21:
			return ((float)(GetVabcdValue(1) - GetVabcdValue(3)) / (float)(GetVabcdValue(1) + GetVabcdValue(3)));
		case 22:
			return ((float)(GetVabcdValue(4) - GetVabcdValue(6)) / (float)(GetVabcdValue(4) + GetVabcdValue(6)));
		case 23:
			return ((float)(GetVabcdValue(5) - GetVabcdValue(7)) / (float)(GetVabcdValue(5) + GetVabcdValue(7)));
		case 24:
			return (((float)GetVabcdValue(channel) / 1.28E+6) * sqrt(2));
		case 25:
			return (((float)(GetVabcdValue(0) - GetVabcdValue(2)) / 1.28E+6) * sqrt(2));
		case 26:
			return (((float)(GetVabcdValue(1) - GetVabcdValue(3)) / 1.28E+6) * sqrt(2));
		case 27:
			return (((float)(GetVabcdValue(4) - GetVabcdValue(6)) / 1.28E+6) * sqrt(2));
		case 28:
			return (((float)(GetVabcdValue(5) - GetVabcdValue(7)) / 1.28E+6) * sqrt(2));
		case 29:
			if(pulseMode==0)
				return ((float)GetXYPosition(channel)/1E+6);
			else
			{
				if(channel==0)
					return X1_avg/1000;
				else if(channel==1)
					return Y1_avg/1000;
				else if(channel==2)
					return X2_avg/1000;
				else if(channel==3)
					return Y2_avg/1000;
			}	
		case 30:
			return funcGetADclkState();
		case 31:
			return funcGetSumProtect(channel);
		case 32:
			if(pulseMode==0)
				return (funcGetBPMPhaseValue(1)+funcGetBPMPhaseValue(2)+funcGetBPMPhaseValue(3))/3;
			else
			{
				return ((ph_ch4+ph_ch5+ph_ch6)/3);
			}
		case 33:
			if(pulseMode==0)
				return (funcGetBPMPhaseValue(5)+funcGetBPMPhaseValue(6)+funcGetBPMPhaseValue(7))/3;
			else
			{
				return ((ph_ch8+ph_ch9+ph_ch10)/3);
			}
		case 34:
			switch(channel){
				case 0:return rf3_avg_volt;
				case 1:return rf4_avg_volt;
				case 2:return rf5_avg_volt;
				case 3:return rf6_avg_volt;
				case 4:return rf7_avg_volt;
				case 5:return rf8_avg_volt;
				case 6:return rf9_avg_volt;
				case 7:return rf10_avg_volt;
				default: return 0;


			}
		case 93:
			return funcGetWRStatus(channel);
		default:
			printf("Call ReadData function with Unknown offset value.\n");
			return 0;
			break;
	}
}

double amp2power(float amp, int ch_N)
{
	double dBm=0,power=0,offset=0,a=0,b=0,n=0,Vrms=0;
	Getparameters(ch_N,1,&offset);
	Getparameters(ch_N,2,&a);
	Getparameters(ch_N,3,&b);
	if(amp<=0)	amp=0;
	else if(amp>=32767)	amp=32767;
	Vrms=a*amp - b;  //This formula was decided by the calibration data.
	dBm=10*log10(Vrms*Vrms*20) + offset;
	n=dBm/10-6;
	power=pow(10,n);  //unit is KW
	return power;
}

void SetReg(int offset, int channel, float val)
{
	int val_tmp;
	val_tmp = (int)val;
	unsigned short val_tmp2;
	
	double val_tmp3;
	switch(offset)
	{
		case 0:
//			val_tmp = (int)val;
			SetDO(channel, val_tmp);
			break;
		case 1:
			val_tmp2 = (unsigned short)val;
			SetPulsecw(val_tmp2);
			break;
		case 2:
//			val_tmp = (int)val;
			SetInnerTrigEn(val_tmp);
			break;
		case 3:
//			val_tmp = (int)val;
			SetHistoryTrigger(val_tmp);
			break;
		case 4:
//			val_tmp = (int)val;
			SetResetHistoryStorage(val_tmp);
			break;
		case 5:
			SetTriggerExtractDataRatio(val);
			break;
		case 6:
			SetHistoryExtractDataRatio(val);
			break;
		case 7:
//			val_tmp = (int)val;
			SetSyncIQStartSign(val_tmp);
			break;
		case 8:
			val_tmp3 = (double)val;
			SetOffset(channel,val_tmp3);
			break;
		case 9:
			ReadCSVparametersfile(val_tmp);
			break;
		case 10:
			SetBPMk1(channel, val);
			break;
		case 11:
			SetBPMk2(channel, val);
			break;
		case 12:
			funcSetBPMk3(channel, val);
			break;
		case 13:
			funcSetBPMPhaseOffset(channel, val);
			if(channel==0)
				ph_offset3 = val;
			else if(channel==1)
				ph_offset4 = val;
			else if(channel==2)
				ph_offset5 = val;
			else if(channel==3)
				ph_offset6 = val;
			else if(channel==4)
				ph_offset7 = val;
			else if(channel==5)
				ph_offset8 = val;
			else if(channel==6)
				ph_offset9 = val;
			else if(channel==7)
				ph_offset10 = val;
			break;
		case 14:
			SetBPMkxy(channel, val);
			break;
		case 15:
			SetBPMxyOffset(channel, val_tmp);
			break;
		case 16:
			SetBPMxyLimits(channel, val_tmp);
			break;
		case 17:
			funcSetReset(val_tmp);
			break;
		case 18:
			SetBPMSumLimits(channel, val_tmp);
			break;
		case 19:
			pulseMode = val_tmp;
			break;
		case 20:
			AVGStart = val_tmp;
			break;
		case 21:
			AVGStop = val_tmp;
			break;
		case 22:
			SetFastIntlkFilterTime(val);
			break;
		case 23:
			SetDDSMode(val_tmp);
			break;
		case 24:
			SetSysLedEnable(val_tmp);	
			break;
		case 25:
			SetFanLedStat(val_tmp);
			break;
		case 26:
			SelectTriggerSource(val_tmp);
			break;
		case 27:
			BackGroundStart(val_tmp);
			break;
		case 28:
			BackGroundStop(val_tmp);
			break;
		default:
			printf("Call SetReg function with Unknown offset value.\n");	
			break;
	}
}

void readWaveform(int offset, int ch_N, unsigned int nelem, float* data, long long *TAI_S, int *TAI_nS)
{
	*TAI_S = (TAISecond-631152000-8*60*60);
	*TAI_nS = (TAINanoSecond*16);
	switch(offset)
	{
		case 1:
			funcGetTriggerAllData(0, 0, data);
//			*TAI_S = (TAISecond-631152000);
//			*TAI_nS = (TAINanoSecond*16);
			break;
		case 2:
			funcGetTriggerAllData(0, 1, data);
//			*TAI_S = (TAISecond-631152000);
//			*TAI_nS = (TAINanoSecond*16);
			break;
		case 3:
			funcGetTriggerAllData(0, 2, data);
			break;
		case 4:
			funcGetTriggerAllData(0, 3, data);
			break;
		case 5:
			funcGetTriggerAllData(0, 4, data);
			break;
		case 6:
			funcGetTriggerAllData(0, 5, data);
			break;
		case 7:
			funcGetTriggerAllData(0, 6, data);
			break;
		case 8:
			funcGetTriggerAllData(0, 7, data);
			break;
//		case 9:
//			funcGetTriggerAllData(0, 8, data);
//			break;
//		case 10:
//			funcGetTriggerAllData(0, 9, data);
//			break;
		case 11:
			copyArray(rf3amp, data, 0, nelem);
			calculateAvgVoltage(data,0,nelem);
//			funcGetTriggerAllData(1, 0, data);
			break;
		case 12:
			copyArray(rf4amp, data, 2, nelem);
			calculateAvgVoltage(data,2,nelem);
//			funcGetTriggerAllData(1, 2, data);
			break;
		case 13:
			copyArray(rf5amp, data, 4, nelem);
			calculateAvgVoltage(data,4,nelem);
//			funcGetTriggerAllData(1, 4, data);
			break;
		case 14:
			copyArray(rf6amp, data, 6, nelem);
			calculateAvgVoltage(data,6,nelem);
//			funcGetTriggerAllData(1, 6, data);
			break;
		case 15:
			copyArray(rf7amp, data, 8, nelem);
			calculateAvgVoltage(data,8,nelem);
//			funcGetTriggerAllData(1, 8, data);
			break;
		case 16:
			copyArray(rf8amp, data, 10, nelem);
			calculateAvgVoltage(data,10,nelem);
//			funcGetTriggerAllData(1, 10, data);
			break;
		case 17:
			copyArray(rf9amp, data, 12, nelem);
			calculateAvgVoltage(data,12,nelem);
//			funcGetTriggerAllData(1, 12, data);
			break;
		case 18:
			copyArray(rf10amp, data, 14, nelem);
			calculateAvgVoltage(data,14,nelem);
//			funcGetTriggerAllData(1, 14, data);
			break;
//		case 19:
//			copyArray(rf9amp, data, 16, nelem);
//			funcGetTriggerAllData(1, 16, data);
//			break;
//		case 20:
//			copyArray(rf10amp, data, 18, nelem);
//			funcGetTriggerAllData(1, 18, data);
//			break;
		case 21:
			copyPhArray(rf3phase, data, 1, nelem);
//			funcGetTriggerAllData(1, 1, data);
			break;
		case 22:
			copyPhArray(rf4phase, data, 3, nelem);
//			funcGetTriggerAllData(1, 3, data);
			break;
		case 23:
//			funcGetTriggerAllData(1, 5, data);
			copyPhArray(rf5phase, data, 5, nelem);
			break;
		case 24:
//			funcGetTriggerAllData(1, 7, data);
			copyPhArray(rf6phase, data, 7, nelem);
			break;
		case 25:
//			funcGetTriggerAllData(1, 9, data);
			copyPhArray(rf7phase, data, 9, nelem);
			break;
		case 26:
//			funcGetTriggerAllData(1, 11, data);
			copyPhArray(rf8phase, data, 11, nelem);
			break;
		case 27:
//			funcGetTriggerAllData(1, 13, data);
			copyPhArray(rf9phase, data, 13, nelem);
			break;
		case 28:
//			funcGetTriggerAllData(1, 15, data);
			copyPhArray(rf10phase, data, 15, nelem);
			break;
//		case 29:
//			funcGetTriggerAllData(1, 17, data);
//			copyPhArray(rf9phase, data, 17, nelem);
//			break;
//		case 30:
//			funcGetTriggerAllData(1, 19, data);
//			copyPhArray(rf10phase, data, 19, nelem);
//			break;
		case 31:
			GetHistoryDataFromSingleCh(0, data);
			break;
		case 32:
			GetHistoryDataFromSingleCh(2, data);
			break;
		case 33:
			GetHistoryDataFromSingleCh(4, data);
			break;
		case 34:
			GetHistoryDataFromSingleCh(6, data);
			break;
		case 35:
			GetHistoryDataFromSingleCh(8, data);
			break;
		case 36:
			GetHistoryDataFromSingleCh(10, data);
			break;
		case 37:
			GetHistoryDataFromSingleCh(12, data);
			break;
		case 38:
			GetHistoryDataFromSingleCh(14, data);
			break;
//		case 39:
//			GetHistoryDataFromSingleCh(16, data);
//			break;
//		case 40:
//			GetHistoryDataFromSingleCh(18, data);
//			break;
		case 41:
			GetHistoryDataFromSingleCh(1, data);
			break;
		case 42:
			GetHistoryDataFromSingleCh(3, data);
			break;
		case 43:
			GetHistoryDataFromSingleCh(5, data);
			break;
		case 44:
			GetHistoryDataFromSingleCh(7, data);
			break;
		case 45:
			GetHistoryDataFromSingleCh(9, data);
			break;
		case 46:
			GetHistoryDataFromSingleCh(11, data);
			break;
		case 47:
			GetHistoryDataFromSingleCh(13, data);
			break;
		case 48:
			GetHistoryDataFromSingleCh(15, data);
			break;
//		case 49:
//			GetHistoryDataFromSingleCh(17, data);
//			break;
//		case 50:
//			GetHistoryDataFromSingleCh(19, data);
//			break;
//		case 53:
//			copyArray(rf3amp, data, 4, nelem);
//			break;
//		case 54:
//			copyArray(rf4amp, data, 6, nelem);
//			break;
//		case 55:
//			copyArray(rf5amp, data, 8, nelem);
//			break;
//		case 56:
//			copyArray(rf6amp, data, 10, nelem);
//			break;
//		case 57:
//			copyArray(rf7amp, data, 12, nelem);
//			break;
//		case 58:
//			copyArray(rf8amp, data, 14, nelem);
//			break;
//		case 59:
//			copyArray(rf9amp, data, 16, nelem);
//			break;
//		case 60:
//			copyArray(rf10amp, data, 18, nelem);
//			break;
		case 61:
			copyXYArray(X1, data, 16, nelem);
			break;
		case 62:
			copyXYArray(Y1, data, 17, nelem);
			break;
		case 63:
			copyXYArray(X2, data, 18, nelem);
			break;
		case 64:
			copyXYArray(Y2, data, 19, nelem);
			break;
		case 65:
			funcGetTriggerAllData(1, 20, data);
			break;
		case 66:
			funcGetTriggerAllData(1, 21, data);
			break;
//		case 73:
//			copyHistoryArray(rf3amp_trip, data, 4, nelem);
//			break;
//		case 74:
//			copyHistoryArray(rf4amp_trip, data, 6, nelem);
//			break;
//		case 75:
//			copyHistoryArray(rf5amp_trip, data, 8, nelem);
//			break;
//		case 76:
//			copyHistoryArray(rf6amp_trip, data, 10, nelem);
//			break;
//		case 77:
//			copyHistoryArray(rf7amp_trip, data, 12, nelem);
//			break;
//		case 78:
//			copyHistoryArray(rf8amp_trip, data, 14, nelem);
//			break;
//		case 79:
//			copyHistoryArray(rf9amp_trip, data, 16, nelem);
//			break;
//		case 80:
//			copyHistoryArray(rf10amp_trip, data, 18, nelem);
//			break;
		case 81:
			copyHistoryXYArray(HistoryX1, data, 16, nelem);
			break;
		case 82:
			copyHistoryXYArray(HistoryY1, data, 17, nelem);
			break;
		case 83:
			copyHistoryXYArray(HistoryX2, data, 18, nelem);
			break;
		case 84:
			copyHistoryXYArray(HistoryY2, data, 19, nelem);
			break;
		case 85:
			GetHistoryDataFromSingleCh(20, data);
			break;
		case 86:
			GetHistoryDataFromSingleCh(21, data);
			break;		
		default:
			printf("Call readWaveform function with Unknown offset value.\n");		
			break;
	}
}

static void copyArray(float *dmaBuf, float *wfBuf, int ch_N, int length)
{
	int i;
//	funcGetTriggerChannelData(ch_N, dmaBuf);
	funcGetTriggerAllData(1, ch_N, dmaBuf);
	for(i=0; i<length; ++i){
		wfBuf[i] = ((float)dmaBuf[i] / 1.28E+6) * sqrt(2);
	}
}

static void copyPhArray(float *dmaBuf, float *wfBuf, int ch_N, int length)
{
	int i;
	funcGetTriggerAllData(1, ch_N, dmaBuf);
	for(i=0; i<length; ++i){
		wfBuf[i] = (float)dmaBuf[i];
		if(i==AVGStop){
			if(ch_N == 1){
				ph_ch3 = (float)dmaBuf[i];
			}
			else if(ch_N == 3){
				ph_ch4 = (float)dmaBuf[i];
			}
			else if(ch_N == 5){
				ph_ch5 = (float)dmaBuf[i];
			}
			else if(ch_N == 7){
				ph_ch6 = (float)dmaBuf[i];
			}
			else if(ch_N == 9){
				ph_ch7 = (float)dmaBuf[i];
			}
			else if(ch_N == 11){
				ph_ch8 = (float)dmaBuf[i];
			}
			else if(ch_N == 13){
				ph_ch9 = (float)dmaBuf[i];
			}
			else if(ch_N == 15){
				ph_ch10 = (float)dmaBuf[i];
			}
		}
	}
}

static void copyXYArray(float *dmaBuf, float *wfBuf, int ch_N, int length)
{
	int i;
	float sum=0;
	int totalPoints;
//	funcGetTriggerChannelData(ch_N, dmaBuf);
	funcGetTriggerAllData(1, ch_N, dmaBuf);
	for(i=0; i<length; ++i){
		wfBuf[i] = ((float)dmaBuf[i] / 1000);
		if(i>=AVGStart && i<=AVGStop)
		{
			sum += ((float)dmaBuf[i] / 1000);
		}
	}
	totalPoints = AVGStop - AVGStart + 1;
	if(ch_N == 16)
		X1_avg = sum/totalPoints;
    else if(ch_N == 17)
		Y1_avg = sum/totalPoints;
	else if(ch_N == 18)
		X2_avg = sum/totalPoints;
	else if(ch_N == 19)
		Y2_avg = sum/totalPoints;
}

static void copyHistoryArray(float *dmaBuf, float *wfBuf, int ch_N, int length)
{
	int i;
//	funcGetTriggerChannelData(ch_N, dmaBuf);
	GetHistoryDataFromSingleCh(ch_N, dmaBuf);
	for(i=0; i<length; ++i){
		wfBuf[i] = ((float)dmaBuf[i] / 1.28E+6) * sqrt(2);
	}
}

static void copyHistoryXYArray(float *dmaBuf, float *wfBuf, int ch_N, int length)
{
	int i;
//	funcGetTriggerChannelData(ch_N, dmaBuf);
	GetHistoryDataFromSingleCh(ch_N, dmaBuf);
	for(i=0; i<length; ++i){
		wfBuf[i] = ((float)dmaBuf[i] / 1000);
	}
}

// static void copyADCrawData(int *dmaBuf, float *wfBuf, int length)
// {
	// int i;
	// for(i=0; i<length; ++i){
		// wfBuf[i] = dmaBuf[i];
	// }
// }

static void copyArray2Power(float *dmaBuf, float *wfBuf, int length, int Ch_N)
{
	double dBm=0,power=0,offset=0,a=0,b=0,n=0,amp=0,Vrms=0;
	int i;
	Getparameters(Ch_N,1,&offset);
	Getparameters(Ch_N,2,&a);
	Getparameters(Ch_N,3,&b);
	for(i=0; i<length; ++i){
		amp = (double)dmaBuf[i];
		Vrms=a*amp - b;
		dBm=10*log10(Vrms*Vrms*20)+offset;
		n=dBm/10-6;
		power=pow(10,n);
		wfBuf[i]=(float)power;	
	}
}

static float GetPhOnFlattop(int channel)
{
	if(channel==2)
		return (ph_ch3);
	else if(channel==3)
		return (ph_ch4);
	else if(channel==4)
		return (ph_ch5);
	else if(channel==5)
		return (ph_ch6);
	else if(channel==6)
		return (ph_ch7);
	else if(channel==7)
		return (ph_ch8);
	else if(channel==8)
		return (ph_ch9);
	else if(channel==9)
		return (ph_ch10);
//		return (ph_ch10+ph_offset10);
	else
		return 0;
}

static float GetRFInfo(int channel, int type)
{
	float amp, phase;
	funcGetRfInfo(channel, &amp, &phase);
	if(type == 0){
		return amp;
	}else{
		return phase;
	}
}

static float GetDI(int channel)
{
	int value;
	funcGetDI(channel, &value);
	return (float)value;
}

static void SetDO(int channel, int value)
{
	GetSysTime();
	printf("Set DO %d value to %d\n", channel, value);
	funcSetDO(channel, value);
	printf("Set DO finished!\n");
}

static float GetFPGA_LED0_RBK()
{
	int value;
	value = funcGetFPGA_LED0_RBK();
	return (float)value;
}

static float GetFPGA_LED1_RBK()
{
	int value;
	value = funcGetFPGA_LED1_RBK();
	return value;
}

static void SetSysLedEnable(int enable)
{
	GetSysTime();
	printf("Set Sys LED on front panel enable --> %d\n", enable);
	funcSetArmLedEnable(enable);	
}

static void SetFanLedStat(int value)
{
	GetSysTime();
	printf("Set Fan LED state on front panel to --> %d\n", value);
	funcSetFanLedStatus(value);
}

static void SetPulsecw(unsigned short value)
{
	GetSysTime();
	printf("Set Pulse CW --> %d\n", value);
	funcSetOutputPulseEnable(value);
}

static void  SetInnerTrigEn(int value)
{
	GetSysTime();
	printf("Select Inner or external Trig signal to collect Trigger waveform --> %d\n", value);
	funcSetInnerTrigEn(value);
}

static float   GetHistoryDataReady()
{
	int value;
	value = funcGetHistoryDataReady();
	return value;
}

static int SetHistoryTrigger(int enable)
{
	GetSysTime();
	printf("IOC try to read History waveform.\n");	
//	printf("Set History Trigger to 1\n");	
//	printf("History Trigger have set to -->%d\n", enable);
//	historyDataFlag = HistoryDataUploadReady();
	if(enable == 1 && ReadWfActionCounter == 0)
	{
		ReadWfActionCounter = 1;		
		funcSetHistoryTrigger(enable);
		GetSysTime();
		printf("finish setting history waveform trigger method --> %d!\n", enable);
//		printf("Begin Get History data!\n");
		// rf1amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf2amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf3amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf4amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf5amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf6amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf7amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf8amp_trip = malloc(trip_buf_len * sizeof(float));
		// rf1phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf2phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf3phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf4phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf5phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf6phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf7phase_trip = malloc(trip_buf_len * sizeof(float));
		// rf8phase_trip = malloc(trip_buf_len * sizeof(float));
		GetSysTime();
		printf("Start to Get History data!\n");
//		int historyDataReady=0;
//		historyDataReady = HistoryDataUploadReady();
		HistoryDataUploadReady();
		// while(historyDataReady == 1 && historyDataFlag == 0)
		// {
			// historyDataReady = HistoryDataUploadReady();
			// usleep(100000);
		// } 
//		GetHistoryData(rf1amp_trip,rf1phase_trip,rf2amp_trip,rf2phase_trip,rf3amp_trip,rf3phase_trip,rf4amp_trip,rf4phase_trip,rf5amp_trip,rf5phase_trip,rf6amp_trip,rf6phase_trip,rf7amp_trip,rf7phase_trip,rf8amp_trip,rf8phase_trip);
		scanIoRequest(TripBufferinScanPvt);
		ReadWfActionCounter = 0;
		GetSysTime();
		printf("All history data have been read from FPGA and stored in ARM buffer.\n");
		return 0;
	}
	GetSysTime();
	printf("The last run of read History waveform didn't finish or the read command value is 0, exit.\n");
	return 1;
}

static void SetResetHistoryStorage(int value)
{
	GetSysTime();
	printf("Reset history data buffer --> %d\n", value);
	funcSetResetHistoryStorage(value);	
//	printf("History Trigger have set to 1\n");
}

static void  SetTriggerExtractDataRatio(float value)
{
	GetSysTime();
	printf("Set Data Ratio of Trigger data --> %f\n", value);
	funcSetTriggerExtractDataRatio(value);
}

static void  SetHistoryExtractDataRatio(float value)
{
	GetSysTime();
	printf("Set Data Ratio of History Data --> %f\n", value);
	funcSetHistoryExtractDataRatio(value);
}



// static void GetHistoryData(float *rf1amp,float *rf1phase,float *rf2amp,float *rf2phase,float *rf3amp,float *rf3phase,float *rf4amp,float *rf4phase,float *rf5amp,float *rf5phase,float *rf6amp,float *rf6phase,float *rf7amp,float *rf7phase,float *rf8amp,float *rf8phase)
// {
// //	GetSysTime();
// //	printf("Start to Get History data!\n");
	// funcGetHistoryData(rf1amp,rf1phase,rf2amp,rf2phase,rf3amp,rf3phase,rf4amp,rf4phase,rf5amp,rf5phase,rf6amp,rf6phase,rf7amp,rf7phase,rf8amp,rf8phase);
	// GetSysTime();
	// printf("Get History data from FPGA has finished!\n");
// }

// static void GetTriggerData(float* rf1amp, float* rf1phase, float* rf2amp, float* rf2phase, float* rf3amp, float* rf3phase, 
			// float* rf4amp, float* rf4phase, float* rf5amp, float* rf5phase, float* rf6amp, float* rf6phase, 
			// float* rf7amp, float* rf7phase, float* rf8amp, float* rf8phase)
// {	
	// funcGetTriggerData(rf1amp,rf1phase,rf2amp,rf2phase,rf3amp,rf3phase,rf4amp,rf4phase,rf5amp,rf5phase,rf6amp,rf6phase,rf7amp,rf7phase,rf8amp,rf8phase);
// }

// static void GetTriggerAdcData(int *ADC1_rawdata,int *ADC2_rawdata,int *ADC3_rawdata,int *ADC4_rawdata,int *ADC5_rawdata,int *ADC6_rawdata,int *ADC7_rawdata,int *ADC8_rawdata)
// {
	// funcGetTriggerAdcData(ADC1_rawdata, ADC2_rawdata, ADC3_rawdata, ADC4_rawdata, ADC5_rawdata, ADC6_rawdata, ADC7_rawdata, ADC8_rawdata);
// }

static void SetSyncIQStartSign(int value)
{
	GetSysTime();
	printf("IOC Set Sync the Start Sign of IQ Signal --> %d\n", value);
	funcSetSyncIQStartSign(value);
}

static int HistoryDataUploadReady(void)
{
	return funcHistoryChannelDataReached();
}

static void GetHistoryDataFromSingleCh(int channel,float *data)
{
	funcGetHistoryChannelData(channel, data);
}

static int GetVabcdValue(int channel)
{
	return funcGetVcValue(channel);
}

static int GetXYPosition(int channel)
{
	return funcGetxyPosition(channel);
}

static int GetVsumValue(int channel)
{
	return funcGetVcSumValue(channel);
}

static void SetBPMk1(int channel, float value)
{
	funcSetBPMk1(channel, value);
}

static void SetBPMk2(int channel, float value)
{
	funcSetBPMk2(channel, value);
}

static void SetBPMkxy(int channel, float value)
{
	int val;
	if(channel == 4 || channel == 5)
	{
		val = (int)(value*32767);
	}else{
		val = (int)(value*1E+6);
	}
	funcSetBPMkxy(channel, val);
	printf("The Kxy or Ksum has been set to %d.\n", val);
}

static void SetBPMxyOffset(int channel, int value)
{
	int val;
	val = (value*1E+6);
	funcSetBPMxyOffset(channel, val);
	printf("The xy offset of (%d) has been set to %d.\n", channel, val);
}

static void SetBPMxyLimits(int channel, int value)
{
	int val;
	val = (value*1000);
	funcSetBPMxyLimits(channel, val);
	printf("The xy limit of (%d) has been set to %d.\n", channel, val);
}

static void SetBPMSumLimits(int channel, int value)
{
	int val;
	val = (value*1);
	funcSetBPMSumLimits(channel, val);
	printf("The Sum limit of (%d) has been set to %d.\n", channel, val);
}

static void SetFastIntlkFilterTime(float value)
{
	funcSetBPMProtectFilterTime(value);
	printf("The Fast Interlock Filter has been set to %f us.\n", value);
}

static void SetDDSMode(int value)
{
	GetSysTime();
	printf("IOC Set DDS mode to --> %d\n", value);
	funcSetFreqControlWordtoDDS(value);
}

static void SelectTriggerSource(int value)
{
	GetSysTime();
	printf("IOC Set Select Trigger Source of %d.\n", value);
	funcSetSelectExternelTrigger(value);
}

static void  ReadCSVparametersfile(int value)
{
	FILE *fp = NULL;
	int i=0;
	char *line,*record;
	char buffer[1024];
	if(value==1)
	{
		GetSysTime();
		printf("Read CSV parameters file-->%d\n",value);
		fp = fopen(CSVfile_Path,"r");
		if(fp!=NULL)
		{
			GetSysTime();
			printf("Open CSV parameters file successed!\n");
			fseek(fp,0,SEEK_SET);
			while((line=fgets(buffer,sizeof(buffer),fp))!=NULL)
			{
				int j=0;
				record=strtok(line,",");
				while(record!=NULL)
				{
					if(j!=1 && j!=5){
						parameters[i][j]=atof(record);
						record=strtok(NULL,",");
					}else{
						
						record=strtok(NULL,",");
					}
					j++;
				}
				i++;
			}
			fclose(fp);
			fp=NULL;
			int j;
			for(i=0;i<9;i++)
			{
				for(j=0;j<7;j++){
					printf("%e\t",parameters[i][j]);
				}
				printf("\n");
			}
		}
		else{
			GetSysTime();
			printf("Failed to read csv file!\n");
		}
	}
}

void  Getparameters(int row,int column,double* data)
{
	*data=parameters[row][column];
}

static void SetOffset(int row, double value)
{
	parameters[row][1]=value;
	GetSysTime();
	printf("The offset %d has been set to %f\n", row, value);
}

static void SetSysTime(void)
{
	struct timespec ts;
	long long TaiSec = 0;
	int TaiNSec = 0;
	funcGetTimestampData(0, &TaiSec, &TaiNSec);
	ts.tv_sec = TaiSec;
	ts.tv_nsec = (TaiNSec*16);
	clock_settime(CLOCK_REALTIME, &ts);	
}

static void GetSysTime(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);//获取1970-1-1到现在的时间结果保存到tv中
	uint64_t sec=tv.tv_sec;
//	uint64_t min=tv.tv_sec/60;
	struct tm cur_tm;//保存转换后的时间结果
	localtime_r((time_t*)&sec,&cur_tm);
	char cur_time[20];
	snprintf(cur_time,20,"%d-%02d-%02d %02d:%02d:%02d",cur_tm.tm_year+1900,cur_tm.tm_mon+1,cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min,cur_tm.tm_sec);
	printf("%s ",cur_time);		
}


static void calculateAvgVoltage(float *wfBuf, int ch_N, int length)
{
	int i=0;
	float signal_sum = 0;  // sum of effective signal
	float background_sum = 0; // sum of background signal
	int signal_count = 0; // count of effective signal
	int background_count = 0; // count of background signal
	float avg_volt = 0; // average voltage

	for (i=0;i<length;i++){
		if(i>=AVGStart && i<=AVGStop){
			signal_sum += wfBuf[i];

		}
		if(i>=BackGroundStart && i<=BackGroundStop)
		{
			background_sum += wfBuf[i];

		}	
	}
	signal_count = AVGStop - AVGStart + 1;
	background_count = BackGroundStop - BackGroundStart + 1;

	if (signal_count > 0 && background_count > 0){
		avg_volt = signal_sum / signal_count - background_sum / background_count;

	}
	else{
		avg_volt = 0;
	}

	switch(ch_N){
		case 0: rf3_avg_volt = avg_volt; break;
		case 2: rf4_avg_volt = avg_volt; break;
		case 4: rf5_avg_volt = avg_volt; break;
		case 6: rf6_avg_volt = avg_volt; break;
		case 8: rf7_avg_volt = avg_volt; break;
		case 10: rf8_avg_volt = avg_volt; break;
		case 12: rf9_avg_volt = avg_volt; break;
		case 14: rf10_avg_volt = avg_volt; break;
	
	}
	
}