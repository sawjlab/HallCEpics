/**
 * Status bit masking
 * 
 * Trip     = 0;
 * Ovr Curr = 1;
 * HV Max   = 2;
 * Ovr Volt = 3;
 * Und Volt = 4;
 * Ramp Dwn = 5;
 * Ramp Up  = 6;
 * HV On    = 7;
 * HV Off   = 8;

 * Channel Status from table 6.19 in the SY403 manual
 * --------------------------------------------------------
 * Bits |Val0                   |Val1               |OutVal
 * 0..1 |Don't Care             |Don't Care         |na
 * 2    |Channel not Present    |Channel Present    |na
 * 3..7 |Don't Care             |Don't Care         |na
 * 8    |                       |HV Max             |8
 * 9    |                       |Trip               |9
 * 10   |                       |Overvoltage        |10
 * 11   |                       |Undervoltage       |11
 * 12   |                       |Overcurrent        |12
 * 13   |                       |Down               |13
 * 14   |                       |Up                 |14
 * 15   |Channel Off            |Channel On         |0 or 15
 */

#include <registryFunction.h>
#include <subRecord.h>
#include <epicsExport.h>

#include "Caen.h"

#define inp  ((int) psub->a)
#define out  ((int) psub->val)

int ix;
int index;

long statSubInit(struct subRecord *psub) {
    return(0);
}

/* The order of the compares was taken from the original sequencer. */
long statSub(struct subRecord *psub) {
    if (inp & 0x0004) {                 /* Channel present */
        if (inp & 0x0200) {             /* Trip */
            out = 9;
        } else if (inp & 0x1000) {      /* Overcurrent */
            out = 12;
        } else if (inp & 0x0100) {      /* HV Max */
            out = 8;
        } else if (inp & 0x0400) {      /* Overvoltage */
            out = 10;
        } else if (inp & 0x0800) {      /* Undervoltage */
            out = 11;
        } else if (inp & 0x2000) {      /* Down */
            out = 13;
        } else if (inp & 0x4000) {      /* Up */
            out = 14;
        } else if (inp & 0x8000) {      /* Channel On */
            out = 15;
        } else if ((inp & 0x8000)==0) { /* Channel Off */
            out = 0;
        }
    } else {                            /* Channel not present */
        out = 4;
    }
    
    for (ix=0; ix<64; ix++) {
        CAEN_get_table_index(36,ix,&index);
        if (index>=0) {
            caen_table[index].status_stale_out_period=1000;
        }
    }
    return(0);
}

epicsRegisterFunction(statSubInit);
epicsRegisterFunction(statSub);

