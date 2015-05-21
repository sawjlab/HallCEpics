TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Build an IOC support library

# Build the IOC application
PROD_IOC_vxWorks = vmec8

# vmec8.dbd will be created and installed
DBD += vmec8.dbd

# vmec8.dbd will include these files:
vmec8_DBD += base.dbd
vmec8_DBD += CaenHV.dbd

# Build the main IOC entry point where needed
vmec8_SRCS_DEFAULT += vmec8Main.cpp
vmec8_SRCS_vxWorks += vmec8_registerRecordDeviceDriver.cpp

# Add support from base/src/vxWorks if needed
vmec8_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
vmec8_LIBS += CaenHV

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    vmec8_LIBS += seq pv
endif

# Finally link IOC to the EPICS Base libraries
vmec8_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

