## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script
cd "/net/cdaqfs/home/cvxwrks/EpicsLL/apps/iocBoot/iocvmec7"

< cdCommands
< ../nfsCommands

cd topbin

## You may have to change vmec7 to something else
## everywhere it appears in this file
ld 0,0, "vmec7.munch"
rebootHookAdd(epicsExitCallAtExits)

## Register all support components
cd top
dbLoadDatabase "dbd/vmec7.dbd"
vmec7_registerRecordDeviceDriver pdbbase

#epicsEnvSet( "EPICS_CA_ADDR_LIST", "129.57.255.13")

## Load record instances

dbLoadTemplate "db/CaenHV_crate2.substitutions"
dbLoadTemplate "db/CaenHV_crate3.substitutions"
dbLoadTemplate "db/CaenHV_crate4.substitutions"
dbLoadTemplate "db/CaenHV_crate5.substitutions"

## Set this to see messages from mySub
#mySubDebug = 1

## Run this to trace the stages of iocInit
#traceIocInit

cd startup
iocInit

## Start any sequence programs
#seq &sncExample, "user=cvxwrks"
