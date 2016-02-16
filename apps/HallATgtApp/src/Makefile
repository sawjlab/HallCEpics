TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
#DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += HallATgtSupport

# Compile and add the code to the support library
#HallATgtSupport_SRCS += xxxRecord.c
#HallATgtSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
# This is required for Windows DLL builds.
#HallATgtSupport_SRCS += dbSubExample.c
HallATgtSupport_SRCS += HallATgtHello.c
#HallATgtSupport_SRCS += initTrace.c

HallATgtSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build the IOC application
PROD_IOC = HallATgt

# HallATgt.dbd will be created and installed
DBD += HallATgt.dbd

# HallATgt.dbd will include these files:
HallATgt_DBD += base.dbd
#HallATgt_DBD += xxxSupport.dbd
#HallATgt_DBD += dbSubExample.dbd
HallATgt_DBD += HallATgtHello.dbd
#HallATgt_DBD += initTrace.dbd

# HallATgt_registerRecordDeviceDriver.cpp derives from HallATgt.dbd
HallATgt_SRCS += HallATgt_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
HallATgt_SRCS_DEFAULT += HallATgtMain.cpp
HallATgt_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#HallATgt_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
HallATgt_LIBS += HallATgtSupport

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into HallATgtSupport
#    sncExample_SNCFLAGS += +r
#    HallATgt_DBD += sncExample.dbd
#    HallATgtSupport_SRCS += sncExample.stt
#    HallATgtSupport_LIBS += seq pv
#    HallATgt_LIBS += seq pv

    # Build sncProgram as a standalone program
#    PROD_HOST += sncProgram
#    sncProgram_SNCFLAGS += +m
#    sncProgram_SRCS += sncProgram.st
#    sncProgram_LIBS += seq pv
#    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link IOC to the EPICS Base libraries
HallATgt_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

