#!../../bin/linux-x86_64/VoltageCalculator

## You may have to change VoltageCalculator to something else
## everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/VoltageCalculator.dbd"
VoltageCalculator_registerRecordDeviceDriver pdbbase

## ========== 配置电压计算驱动 ========== ##
## 参数：BPM IOC的PV前缀
## 例如：voltCalcInit("BPM:SYS0:1")
## 这里需要根据实际的BPM IOC配置修改
voltCalcInit("BPM")

## Load record instances
## 参数：PV前缀（用于电压计算器的PV）
dbLoadRecords("db/voltCalc.db","P=VOLTCALC")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=user"
