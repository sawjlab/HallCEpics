TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
LIBRARY_IOC_vxWorks += CaenHV

DBD += CaenHV.dbd

# specify all source files to be compiled and added to the library
INC += Caen.h

CaenHV_SRCS += devCaen.c
CaenHV_SRCS += drvCaen.c
CaenHV_SRCS += devAiCaen.c
CaenHV_SRCS += devAoCaen.c
CaenHV_SRCS += devBiCaen.c
CaenHV_SRCS += devBoCaen.c
CaenHV_SRCS += drvCaenInit.c
CaenHV_SRCS += HVTaskWatchDog.c

CaenHV_SRCS += CaenHV_registerRecordDeviceDriver.cpp


# subroutines
CaenHV_SRCS += statSub.c
 
# Add support from base/src/vxWorks if needed
#CaenHV_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

CaenHV_LIBS += $(EPICS_BASE_IOC_LIBS)

#PROD_IOC_vxWorks = CaenHV

# Build the main IOC entry point where needed
#CaenHV_SRCS_DEFAULT += CaenHVMain.cpp
#CaenHV_SRCS_vxWorks += -nil-

#===========================
include $(TOP)/configure/RULES

# Turn on debug() print statements
#USR_CPPFLAGS += -DDEBUG
