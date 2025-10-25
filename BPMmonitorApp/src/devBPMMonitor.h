/* devBPMMonitor.h */
/* Author:  Gao    Create Date:  02Nov2021 */
/* The last modified date:  23Dec2021 */

#ifndef _devBPMMonitor_H
#define _devBPMMonitor_H

static long devGetInTrigInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt);
static long devGetInTripBufferInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt);
static long devGetInADCrawBufferInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt);
static int devIoParse();

#endif