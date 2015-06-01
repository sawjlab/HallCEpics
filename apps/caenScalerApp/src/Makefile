TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
LIBRARY_IOC_vxWorks += caenScaler

DBD += caenScaler.dbd

DBDINC += scalerRecord

# specify all source files to be compiled and added to the library
INC += devScaler.h

caenScaler_SRCS += devScaler.c
caenScaler_SRCS += scalerRecord.c

caenScaler_SRCS += caenScaler_registerRecordDeviceDriver.cpp

# Add support from base/src/vxWorks if needed
#caenScaler_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

caenScaler_LIBS += $(EPICS_BASE_IOC_LIBS)

#PROD_IOC_vxWorks = caenScaler

# Build the main IOC entry point where needed
#caenScaler_SRCS_DEFAULT += caenScalerMain.cpp
#caenScaler_SRCS_vxWorks += -nil-

#===========================
include $(TOP)/configure/RULES

# Turn on debug() print statements
#USR_CPPFLAGS += -DDEBUG
