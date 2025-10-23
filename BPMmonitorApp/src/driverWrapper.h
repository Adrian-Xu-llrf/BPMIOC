/* driverWrapper.h */
/* Author:  Gao    Create Date:  01Nov2021 */
/* The last modified date:  16Dec2021 */

#ifndef _driverWrapper_H
#define _driverWrapper_H

#include <dbScan.h>

/* The following functions will be called from upper layer.**************/
IOSCANPVT devGetInTrigScanPvt();

IOSCANPVT devGetInTripBufferScanPvt();

IOSCANPVT devGetInADCrawBufferScanPvt();

void* pthread();

float ReadData(int offset, int channel, int type);

void SetReg(int offset, int channel, float val);

// void readWaveform(int offset, int ch_N, unsigned int nelem, float* data);
void readWaveform(int offset, int ch_N, unsigned int nelem, float* data, long long *TAI_S, int *TAI_nS);

void  Getparameters(int row,int column,double* data);

double amp2power(float amp, int ch_N);

#endif