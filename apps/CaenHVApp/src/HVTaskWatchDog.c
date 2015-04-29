#include <vxWorks.h>
#include <taskLib.h>
#include <stdlib.h>
#include <stdioLib.h>
#include <dbDefs.h>
#include <dbFldTypes.h>
#include <dbBase.h>
#include <dbAccess.h>

#define MAXTASKS 200

struct dbAddr *dogPtr=0;

void HVTaskWatchDog()
{
    int idList[200];
    int ntasks;
    int i;
    int nsuspended;
    long val;

    while(1) {

        ntasks = taskIdListGet(idList,MAXTASKS);

        nsuspended=0;

        for(i=0; i<ntasks; i++) {
            if(taskIsSuspended(idList[i])) {
                nsuspended++;
            }
        }
        if(nsuspended==0) {
            val = 1;
            dbPutField(dogPtr, DBR_LONG, &val, sizeof(val));
        } else {
            printf("%d suspended tasks\n",nsuspended);
            val = 0;
            dbPutField(dogPtr, DBR_LONG, &val, sizeof(val));
            break;
        }
        taskDelay(20*60);
    }
}

void startHVTaskWatchDog(char *dogname)
{
    dogPtr = (struct dbAddr *) malloc(sizeof(struct dbAddr));
    dbNameToAddr(dogname, dogPtr);

    taskSpawn("Suspend Watchdog",
              200,
              0,
              10000,
              (FUNCPTR)HVTaskWatchDog,0,1,2,3,4,5,6,7,8,9);
}
