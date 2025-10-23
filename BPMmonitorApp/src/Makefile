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

dl_DIR += /home/gao/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/lib
PROD_LIBS += dl
pthread_DIR += /home/gao/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/lib
PROD_LIBS += pthread

# Add support from base/src/vxWorks if needed
#BPMmonitor_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

