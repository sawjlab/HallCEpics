TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE


LIBRARY_IOC_vxWorks += bcmSupport

DBD += bcmSupport.dbd

#bcmSupport_DBD += base.dbd
#bcmSupport_DBD += caenScaler.dbd

bcmSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# bcm_registerRecordDeviceDriver.cpp derives from bcm.dbd
#bcm_SRCS += bcm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
#bcm_SRCS_DEFAULT += bcmMain.cpp
bcm_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#bcm_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
#bcm_LIBS += $(EPICS_BASE_IOC_LIBS)

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
#ifneq ($(SNCSEQ),)
    # Build sncExample into bcmSupport
  bcmSupport_DBD += bcmSNC.dbd
  bcmSupport_SRCS += hcrate.st
#  bcmSupport_SRCS += bcm_registerRecordDeviceDriver.cpp

#    hcrate_SNCFLAGS += +r
#    bcm_SNCFLAGS += +r
#    bcm_SRCS += hcrate.st
#endif


# Finally link IOC to the EPICS Base libraries
bcm_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

