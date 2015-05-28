/* 
   inputRegister

   Background job that reads an input register and wakes up watchdog records
   whenever a bit in the input register goes high.

   This helper routine is used because I can't figure out how to wake up
   bo records with HIGH fields without also flipping them back to zero when
   the input (through DOL) goes low again.

*/

#define WATCHDOG_RECORD_NAME "HCDOG_bit"
#define BIT_RECORD_NAME "HC_bit"
#define REGISTERADDRESS 0xffff0ed0
#define REGSIZE 16
#define MAX(A,B) (A>B?A:B)

#include <stdio.h>
#include <string.h>
#include <taskLib.h>
#include <dbAccess.h>
#include <dbAddr.h>

void inputRegisterScan();

void startInputRegisterScan()
{
  taskSpawn("inputRegisterTask",195,VX_FP_TASK,20000,
	    (FUNCPTR) *inputRegisterScan,0,0,0,0,0,0,0,0,0,0);
}
void inputRegisterScan()
{
  volatile long *inpreg=(long *) REGISTERADDRESS;
  long regvalue;
  long lastregvalue;
  long watchdogmask=0;
  long bitmask=0;
  struct dbAddr bitPtr[REGSIZE];
  struct dbAddr wdPtr[REGSIZE];
  char *s;
  long mask;
  int bit;
  long value;

  printf("Starting input register scanning at 0x%x\n",(unsigned int) inpreg);
  printf("Initial state 0x%lx\n",*inpreg & 0xffff);

  /* Discover which bit's have corresponding EPICS record and
     cache the db addresses for those records */
  mask = 1;
  s = (char *) malloc(MAX(strlen(WATCHDOG_RECORD_NAME),strlen(BIT_RECORD_NAME))+4);
  for(bit=0;bit<REGSIZE;bit++) {
    sprintf(s,"%s%d",BIT_RECORD_NAME,bit);
    bitPtr[bit].precord=0;
    dbNameToAddr(s, &bitPtr[bit]);
    if(bitPtr[bit].precord) {
      bitmask |= mask;
    }
    sprintf(s,"%s%d",WATCHDOG_RECORD_NAME,bit);
    wdPtr[bit].precord=0;
    dbNameToAddr(s, &wdPtr[bit]);
    if(wdPtr[bit].precord) {
      watchdogmask |= mask;
    }
    mask <<= 1;
  }
  for(bit=0;bit<REGSIZE;bit++) {
    value = 0;
    if(bitPtr[bit].precord) {	/* Set all Bits to zero */
      dbPutField(&bitPtr[bit],DBR_LONG, &value, sizeof(value));
    }
    value = 1;
    if(wdPtr[bit].precord) {	/* Start all Watchdog timers */
      dbPutField(&wdPtr[bit],DBR_LONG, &value, sizeof(value));
    }
  }

  lastregvalue = 0;
  printf("Bit mask = 0x%lx, Watchdog mask = 0x%lx\n",bitmask,watchdogmask);

  for(;;) {
    regvalue = *inpreg;
    mask = 1;
    for(bit=0;bit<REGSIZE;bit++) {
      if((mask & bitmask) != 0) { /* Write changed bits */
	if((regvalue & mask) != (lastregvalue & mask)) {
	  value = ((regvalue & mask) != 0);
	  dbPutField(&bitPtr[bit],DBR_LONG, &value, sizeof(value));
	}
      }
      if((mask & watchdogmask) != 0) {
	if(((regvalue & mask) != 0) && ((lastregvalue & mask) == 0)) {
	  value = 1;
	  dbPutField(&wdPtr[bit],DBR_LONG, &value, sizeof(value));
	}
      }
      mask <<= 1;
    }
    lastregvalue = regvalue;
    taskDelay(30);
  }
}



