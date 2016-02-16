#!../../bin/linux-x86_64/HallATgt

## You may have to change HallATgt to something else
## everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/HallATgt.dbd"
HallATgt_registerRecordDeviceDriver pdbbase

epicsEnvSet("EPICS_CA_ADDR_LIST","129.57.255.11")

## Load record instances
dbLoadRecords("db/HallATgtMirror.db")

## Set this to see messages from mySub
#var mySubDebug 1

## Run this to trace the stages of iocInit
#traceIocInit

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncExample, "user=cvxwrksHost"
