#!../../bin/linux-arm/BPMmonitor

## You may have to change BPMmonitor to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase("../../dbd/BPMmonitor.dbd",0,0)
BPMmonitor_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("../../db/BPMMonitor.db","P=BPM")

iocInit()

## Start any sequence programs
#seq sncBPMmonitor,"user=gao"
