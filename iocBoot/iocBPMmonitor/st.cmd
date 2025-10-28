#!../../bin/linux-arm/BPMmonitor

## You may have to change BPMmonitor to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase("../../dbd/BPMmonitor.dbd",0,0)
BPMmonitor_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("../../db/BPMMonitor.db","P=iLinac_007:BPM14And15, P1=iLinac_007:BPM14, P2=iLinac_007:BPM15")
dbLoadRecords("../../db/BPMCal.db","P=iLinac_007:BPM14And15, P1=iLinac_007:BPM14, P2=iLinac_007:BPM15")

iocInit()

## Start any sequence programs
#seq sncBPMmonitor,"user=gao"
