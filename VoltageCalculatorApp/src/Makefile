TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = VoltageCalculator

# VoltageCalculator.dbd will be created and installed
DBD += VoltageCalculator.dbd

# VoltageCalculator.dbd will be made up from these files:
VoltageCalculator_DBD += base.dbd
VoltageCalculator_DBD += devVoltCalc.dbd

# VoltageCalculator_registerRecordDeviceDriver.cpp derives from VoltageCalculator.dbd
VoltageCalculator_SRCS += VoltageCalculator_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
VoltageCalculator_SRCS_DEFAULT += VoltageCalculatorMain.cpp
VoltageCalculator_SRCS_vxWorks += -nil-

# Add all the support libraries needed by this IOC
VoltageCalculator_LIBS += $(EPICS_BASE_IOC_LIBS)

# Source files
VoltageCalculator_SRCS += voltCalcSimple.c
VoltageCalculator_SRCS += devVoltCalc.c

# Link with Channel Access client library
VoltageCalculator_LIBS += ca
VoltageCalculator_LIBS += Com

# System libraries
VoltageCalculator_SYS_LIBS += pthread

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
