/************************************************************************/
/*                                                                      */
/*                               C E B A F                              */
/*             Continuous Electron Beam Accelerator Facility.           */
/*                       Newport News, Virginia, USA.                   */
/*                                                                      */
/*      Copyright, 1994, SURA CEBAF.                                    */
/*                                                                      */
/*                                                                      */
/*      History                                                         */
/*      -------                                                         */
/*      .00 022694 db   Init Release                                    */
/*_begin                                                                */
/************************************************************************/
/*                                                                      */
/*      Title:  CEBAF CAEN Analogue Output Device Support                  */
/*      File:   devAiCaen.c                                               */
/*      Environment: UNIX, vxWorks                                      */
/*      Equipment: SUN, VME                                             */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*_end                                                                  */

/* NOTE: Specific to Short Integer Dataset Data.                        */


/*
 *      Original Author: Dave Barker
 *      Current Author:  Dave Barker
 *      Date:            02-26-94
 *
 *      Copyright 1994. SURA CEBAF.
 *
 *      The Experimental Physics and Industrial Control System (EPICS)
 *      is Copyrighted 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *
 */


#include	<vxWorks.h>
#include	<types.h>
#include	<stdioLib.h>
#include	<string.h>

#include	<alarm.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include    <recGbl.h>
#include    <recSup.h>
#include	<devSup.h>
#include	<link.h>
#include	<module_types.h>
#include	<aiRecord.h>
#include    <epicsExport.h>

#include    <vxWorks.h>
#include    <vxLib.h>

#include    "Caen.h"

#define DISABLE_INHIBIT 0
#define CONVERT         0
#define DO_NOT_CONVERT  2

#define REC_CHK_HW  0

#define READ_MOD_ID 0

static long init();
static long init_record();
static long read_ai();
extern void CAEN_reset();

static int  hardware_present;

extern char *CAEN_BASE_LADDR;

struct {
    long		number;
    DEVSUPFUN	report;
    DEVSUPFUN	init;
    DEVSUPFUN	init_record;
    DEVSUPFUN	get_ioint_info;
    DEVSUPFUN	read_ai;
    DEVSUPFUN	special_linconv;
} devAiCaen= {
    6,
    NULL,
    init,
    init_record,
    NULL,
    read_ai,
    NULL
};

epicsExportAddress(dset, devAiCaen);

/************************************************************************/
/*                                                                      */
/*    External Procedure: init                                          */
/*                                                                      */
/*    Function : This procedure is called twice by the EPICS system,    */
/*               once at the start of the IOC initialisation, and once  */
/*               after all database initialisation has completed. This  */
/*               procedure will only process the first time it is       */
/*               called. The CAEN interface to the High Voltage crates  */
/*               tested, and if present the initialisation proceeds.    */
/*                                                                      */
/*    Arguments: int after             Specifies the time of calling    */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
static long init(int after)
{
    int     status;
    short   result;
    short   buffer[12];
    char    mod_id[12];
    int     data;
    int     i;

    debug("devAiCaen init: Caen Initialization Routine Called, after = %d\n", after);

    if (after==0) {
        hardware_present = FALSE;

        /* Check to see if the CAEN V288 module is in the VME crate */
        if (CAEN_BASE_LADDR==0) {
            sysBusToLocalAdrs(0x39, (char *)CAEN_BASE_ADDR, &CAEN_BASE_LADDR);
        }
        status = vxMemProbe((char *)CAEN_BASE_LADDR, READ, 2, (char *)&data);
        if (status == OK ) {
            if (READ_MOD_ID) {
                /* Something is at CAEN_BASE_ADDRESS, now check that it is */
                /* a V288 module. */
                /* Send Read of Module Identifier */

                /* Reset board (direct access to driver support) */
                CAEN_reset();

                /* send command. Note this does not use the asyn task, it */
                /* directly access driver support. Used for init only.    */
                status = CAEN_send_raw_command(1,
                                               0,
                                               (short)CAEN_READ_MOD_ID_CMD,
                                               0);
                if (status == OK) {
                    /* read data */
                    status = CAEN_read_raw_data(1,0,CAEN_READ_MOD_ID_CMD, buffer, &result);
                    if (status != OK) {
                        debug("devAiCaen: Error, read_raw_data.\n");
                        return(1);
                    }
                    if (result != CAEN_OK) {
                        debug("devAiCaen: Result read_raw_data %d.\n",result);
                        return(1);
                    }


                    /* Module ID characters are stored in the first byte */
                    /* (bits 0-7) of each short word entry. */
                    for(i=0; i<CAEN_MOD_ID_SIZE; i++) {
                        mod_id[i]=(char)(buffer[i]&0xff);
                    }
                    mod_id[CAEN_MOD_ID_SIZE]='\0';

                    debug("devAiCaen: Read Module Id %s\n",mod_id);
                    if (strcmp(mod_id,CAEN_MODULE_ID_STRING) == 0) {
                        /* the board is present and correct. */
                        hardware_present = TRUE;

                        /* Now initialise the Tx pipes, Rx cache and start */
                        /* the Asynchronous Device Support Task. */
                        CAEN_init();
                    } else {
                        /* Wrong board in place */
                        debug("devAiCaen: Incorrect board detected!\n");
                    }
                } else {
                    /* error in CAENnet comms */
                    debug("devAiCaen: Error, send_command.\n");
                    return(ERROR);
                }
            } else {
                /* do not read mod id */
                CAEN_reset();
                hardware_present = TRUE;
                CAEN_init();
            }

        } /* end if memprobe */
        else {
            /* mem probe failed. */
            printf("devAiCaen: init: vxMemProbe failed.\n");
        }
    }   /* end after==0 */
    return(0);
}


/************************************************************************/
/*                                                                      */
/*    External Procedure: init_record.                                  */
/*                                                                      */
/*    Function : This procedure is called once for every instance of an */
/*               CAEN analogue output record. The procedure verifies the*/
/*               record's high voltage channel is available,            */
/*               and that the access permission is correct.             */
/*                                                                      */
/*    Arguments: struct boRecord *pbo   Pointer to record data.         */
/*                                                                      */
/*    Returns  : error status of the procedure.                         */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
static long init_record(struct aiRecord	*pai)
{
    char    *parm;      /* parameters from INP */
    int     parms;      /* parameter count */
    short   crate, channel, command;
    short   buffer[8];
    short   result;
    int     status;

    debug("devAiCaen: init record started for record %s\n", pai->name);

    /* If no hardware is present, do not initialise */
    if (!hardware_present) {
        debug("devAiCaen: Record initialization failed, no h/w\n");
        return(DO_NOT_CONVERT);
    }

    switch (pai->inp.type) {
    case (INST_IO):
        parm = pai->inp.value.instio.string;
        debug("\tparm = %s\n", parm);
        parms = sscanf(parm, "H%hd V%hd C%hd", &crate, &channel, &command);

        debug("\tCrate %hd, Channel %hd, Cmd %hd\n",
              crate, channel, command);
        if (REC_CHK_HW) {
            /* Check HV channel exists */
            status = CAEN_send_raw_command(crate,
                                           channel,
                                           CAEN_READ_STATUS_CMD,
                                           (short)0);
            if (status != OK) {
                debug("devAiCaen: rec_init snd raw cmd\n");
                return(1);
            }
            status = CAEN_read_raw_data(crate,
                                        channel,
                                        CAEN_READ_STATUS_CMD,
                                        buffer,
                                        &result);
            debug("status = %d, buffer=%hd %hd %hd %hd %hd %hd %hd %hd\n",
                  status, buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],
                  buffer[5],buffer[6],buffer[7]);

            if (status != OK) {
                /* Read error */
                debug("devAiCaen: rec_init rd raw data\n");
                return(1);
            }
            if (result != CAEN_OK) {
                if (result== (short)CAEN_CHANNEL_NONEXISTENT) {
                    /* Address is invalid for h/w setup */
                    recGblRecordError(S_db_badField,(void *)pai,
                                      "devAiCaen (init_record) invalid CAEN channel");
                    return(S_db_badField);
                } else {
                    /* some other bothersome error. */
                    debug("devAiCaen: rec_init rd result %d \n",result);
                    return(1);
                }
            }
        } /* end hw check */

        /* This is an input record, check that the command number */
        /* lies in the output command range. */
        if ( (command < CAEN_READ_MOD_ID_CMD) ||
                (command > CAEN_READ_PON_CMD)) {
            /* Command is not a read command */
            recGblRecordError(S_db_badField,(void *)pai,
                              "devAiCaen (init_record) CAEN access violation");
            return(S_db_badField);
        }
        /* For this dev support, only allow lin conv scaling */
        if (pai->eguf > 0) pai->eslo = pai->eguf;
        break;
    default:
        recGblRecordError(S_db_badField,(void *)pai,
                          "devAiCaen (init_record) INP type not CAEN_HV");
        return(S_db_badField);
        break;
    }   /* end switch */
    return(0);
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: read_ai                                       */
/*                                                                      */
/*    Function : This procedure sends the CAENnet command sequence which*/
/*               will service the command specified by the record's     */
/*               hardware address.                                      */
/*                                                                      */
/*    Arguments: struct boRecord *pbo   Pointer to the record data.     */
/*                                                                      */
/*    Returns  : The error status of the procecdure.                    */
/*                                                                      */
/*    Copyright, 1993, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
static long read_ai(struct aiRecord	*pai)
{
    char    *parm;
    int     parms;
    short   crate, channel, command;
    int     status;

    /* If no CAEN interface is present, do not write */
    if (!hardware_present) {
        return(DO_NOT_CONVERT);
    }

    parm = pai->inp.value.instio.string;
    parms = sscanf(parm, "H%hd V%hd C%hd", &crate, &channel, &command);

    /* For starters, only allow CAEN_READ_STATUS_CMD  */
    /* and CAEN_READ_V_MON_CMD */

    /* send command */
    status = CAEN_read_data(crate, channel, command, &(pai->rval));
    debug("devAiCaen read_ai: pai->rval %d\n", pai->rval);

    if (status != OK) {
        if (status == CAEN_INVALID_DATA) {
            /* Cache data is out of date, perhaps comms to Caen lost. */
            /* Set record into invalid error state */
            recGblSetSevr(pai,(short)SOFT_ALARM,(short)INVALID_ALARM);
        } else {
            debug("devAiCaen: read_ai send cmd error\n");
            return(status);
        }
    }

    /* if no error and record is invalid, reset */
    if ((pai->stat == (short)SOFT_ALARM)&&(pai->sevr==(short)INVALID_ALARM)) {
        recGblSetSevr(pai,(short)SOFT_ALARM,(short)NO_ALARM);
    }

    return(OK);
}

