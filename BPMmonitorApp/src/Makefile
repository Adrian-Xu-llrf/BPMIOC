TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = BPMmonitor
# BPMmonitor.dbd will be created and installed
DBD += BPMmonitor.dbd

# BPMmonitor.dbd will be made up from these files:
BPMmonitor_DBD += base.dbd
BPMmonitor_DBD += devBPMMonitor.dbd

# Include dbd files from all support applications:
#BPMmonitor_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#BPMmonitor_LIBS += xxx

# BPMmonitor_registerRecordDeviceDriver.cpp derives from BPMmonitor.dbd
BPMmonitor_SRCS += BPMmonitor_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
BPMmonitor_SRCS_DEFAULT += BPMmonitorMain.cpp
BPMmonitor_SRCS_vxWorks += -nil-

BPMmonitor_SRCS += driverWrapper.c
BPMmonitor_SRCS += devBPMMonitor.c

# Add support from base/src/vxWorks if needed
#BPMmonitor_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# --- 修正开始 ---
# 链接 'dl' 和 'pthread' 作为系统库
# (我们删除了指向 /home/gao 的错误 _DIR 路径和 PROD_LIBS)
BPMmonitor_SYS_LIBS += dl
BPMmonitor_SYS_LIBS += pthread
# --- 修正结束 ---

# Finally link to the EPICS Base libraries
BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE