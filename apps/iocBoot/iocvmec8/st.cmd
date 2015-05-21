## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script
cd "/net/cdaqfs/home/cvxwrks/saw/mystuff/iocBoot/iocvmec7"

< cdCommands
< ../nfsCommands

cd topbin

## You may have to change vmec7 to something else
## everywhere it appears in this file
ld 0,0, "vmec8.munch"
rebootHookAdd(epicsExitCallAtExits)

## Register all support components
cd top
dbLoadDatabase "dbd/vmec8.dbd"
vmec8_registerRecordDeviceDriver pdbbase

#epicsEnvSet( "EPICS_CA_ADDR_LIST", "129.57.255.13")

## Load record instances

dbLoadTemplate "db/CaenHV_crate11.substitutions"
dbLoadTemplate "db/CaenHV_crate12.substitutions"

## Set this to see messages from mySub
#mySubDebug = 1

## Run this to trace the stages of iocInit
#traceIocInit

cd startup
iocInit

## Start any sequence programs
#seq &sncExample, "user=cvxwrks"
