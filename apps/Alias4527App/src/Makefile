TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
#DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += Alias4527Support

# Compile and add the code to the support library
#Alias4527Support_SRCS += xxxRecord.c
#Alias4527Support_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
# This is required for Windows DLL builds.
#Alias4527Support_SRCS += dbSubExample.c
Alias4527Support_SRCS += Alias4527Hello.c
#Alias4527Support_SRCS += initTrace.c

Alias4527Support_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build the IOC application
PROD_IOC = Alias4527

# Alias4527.dbd will be created and installed
DBD += Alias4527.dbd

# Alias4527.dbd will include these files:
Alias4527_DBD += base.dbd
#Alias4527_DBD += xxxSupport.dbd
#Alias4527_DBD += dbSubExample.dbd
Alias4527_DBD += Alias4527Hello.dbd
#Alias4527_DBD += initTrace.dbd

# Alias4527_registerRecordDeviceDriver.cpp derives from Alias4527.dbd
Alias4527_SRCS += Alias4527_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
Alias4527_SRCS_DEFAULT += Alias4527Main.cpp
Alias4527_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#Alias4527_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
Alias4527_LIBS += Alias4527Support

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into Alias4527Support
    sncExample_SNCFLAGS += +r
    Alias4527_DBD += sncExample.dbd
    Alias4527Support_SRCS += sncExample.stt
    Alias4527Support_LIBS += seq pv
    Alias4527_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link IOC to the EPICS Base libraries
Alias4527_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

