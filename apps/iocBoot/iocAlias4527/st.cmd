#!../../bin/linux-x86_64/Alias4527

## You may have to change Alias4527 to something else
## everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/Alias4527.dbd"
Alias4527_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate "db/SY4527_crate13.substitutions"
dbLoadTemplate "db/SY4527_crate14.substitutions"

## Set this to see messages from mySub
#var mySubDebug 1

## Run this to trace the stages of iocInit
#traceIocInit

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncExample, "user=cvxwrksHost"
