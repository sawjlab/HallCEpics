TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Build an IOC support library

# Build the IOC application
PROD_IOC_vxWorks = vmec15

# vmec15.dbd will be created and installed
DBD += vmec15.dbd

# vmec15.dbd will include these files:
vmec15_DBD += base.dbd
vmec15_DBD += CaenHV.dbd

# Build the main IOC entry point where needed
vmec15_SRCS_DEFAULT += vmec15Main.cpp
vmec15_SRCS_vxWorks += vmec15_registerRecordDeviceDriver.cpp

# Add support from base/src/vxWorks if needed
vmec15_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
vmec15_LIBS += caenScaler

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    vmec15_LIBS += seq pv
endif

# Finally link IOC to the EPICS Base libraries
vmec15_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

