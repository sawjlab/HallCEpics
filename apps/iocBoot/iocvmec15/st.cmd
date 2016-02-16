## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script
cd "/net/cdaqfs/home/cvxwrks/EpicsLL/apps/iocBoot/iocvmec15"

< cdCommands
< ../nfsCommands

cd topbin

## You may have to change vmec15 to something else
## everywhere it appears in this file
ld 0,0, "vmec15.munch"
rebootHookAdd(epicsExitCallAtExits)

## Register all support components
cd top
dbLoadDatabase "dbd/vmec15.dbd"
vmec15_registerRecordDeviceDriver pdbbase

#epicsEnvSet( "EPICS_CA_ADDR_LIST", "129.57.255.13")

## Load record instances

dbLoadRecords("db/scaler.template","prefix=hc,card=0")
#dbLoadRecords("db/scaler.template","prefix=hc,card=1")
#dbLoadRecords("db/scaler.template","prefix=hc,card=2")

dbLoadTemplate("db/bcm.substitutions")
dbLoadRecords("db/ozone.db","prefix=hc,chan=9")

## Set this to see messages from mySub
#mySubDebug = 1

## Run this to trace the stages of iocInit
#traceIocInit

cd startup
iocInit

## Start any sequence programs
#seq &sncExample, "user=cvxwrks"
seq &hcrate
