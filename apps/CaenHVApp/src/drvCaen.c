/************************************************************************/
/*                                                                      */
/*                               C E B A F                              */
/*             Continuous Electron Beam Accelerator Facility.           */
/*                       Newport News, Virginia, USA.                   */
/*                                                                      */
/*      Copyright, 1993, SURA CEBAF.                                    */
/*                                                                      */
/*                                                                      */
/*      History                                                         */
/*      -------                                                         */
/*      .00 030194 db   Init Release                                    */
/*_begin                                                                */
/************************************************************************/
/*                                                                      */
/*      Title:  CEBAF CAEN Device Support                               */
/*      File:   devCaen.c                                               */
/*      Environment: UNIX, vxWorks                                      */
/*      Equipment: SUN, VME                                             */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*_end                                                                  */

/* #define DEBUG CAEN_RECORD_DEBUG */

#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Caen.h"

/* External variables declared in this module */
struct caen_table_type caen_table[MAX_NUMBER_OF_HV_CHANNELS];
int    CAEN_INITIALISED = FALSE;
int    HV_OFF_TX_PIPE_FD;
int    HV_TX_PIPE_FD;



/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_send_command                             */
/*                                                                      */
/*    Function : This procedure issues the command to the Tx pipe for   */
/*               asynchronous processing. If the command is a HV OFF    */
/*               command, then it is sent to the HV OFF pipe for        */
/*               immediate processing.                                  */
/*                                                                      */
/*    Arguments: short  crate        Destination CAEN crate number.     */
/*               short  channel      Destination CAEN HV channel num.   */
/*               short  command      CAEN Dev Sup Command Type.         */
/*               short value.        16 bit value to be sent.           */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
int CAEN_send_command(crate, channel, command, value)
short  crate;
short  channel;
short  command;
short  value;
{
    struct caen_tx_msg_type caen_msg;
    int result;

    debug("drvCaen CAEN_send_command: Crate %hd, Channel %hd, Cmd %hd\n",
          crate, channel, command);

    /* construct the message. */
    caen_msg.crate   =  (short)crate;
    caen_msg.channel =  (short)channel;
    caen_msg.command =  (short)command;
    caen_msg.value   =  (short)value;

    /* write message to correct pipe */

    /* Check if command is HV and value is off */
    if ( ((command == (short)CAEN_WRITE_HV_CMD)&&
            (value == 0)) ||
            (command==(short)CAEN_WRITE_HV_OFF_CMD)) {
        /* write command to HV OFF pipe */
        result = write(HV_OFF_TX_PIPE_FD,&caen_msg,sizeof(caen_msg));
        if(result == ERROR) {
            return(result);
        }
    } else {
        /* write cmd to normal tx pipe */
        result = write(HV_TX_PIPE_FD,&caen_msg,sizeof(caen_msg));
        if(result == ERROR) {
            return(result);
        }
    }
    return(OK);
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_read_data                                */
/*                                                                      */
/*    Function : This procedure reads the requested data from the local */
/*               cache. Values are all returned as an integer, or an    */
/*               integer array. Short values are set in bits 0-15, char */
/*               in bits 0-7.                                           */
/*                                                                      */
/*    Arguments: short crate         Crate id.                          */
/*               short channel       Channel number.                    */
/*               short command       CAEN Dev Sup Command Type.         */
/*               int   *ptr          Pointer to where data is to be     */
/*                                   read. Data may be shorts or chars. */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
int CAEN_read_data(crate, channel, command, ptr)
short crate;
short channel;
short command;
int   *ptr;
{
    struct status_data_type *status_ptr;
    struct param_data_type  *param_ptr;
    int  result;
    int  table_index;


    if(DEBUG)printf("CAEN_READ_DATA.(Cr %d Ch %d Co %d)\n",
                        crate,channel,command);
    /* Search cache for entry with with crate, channel ID */

    /* If data is in the status entry, search for the status_ptr */
    /* using crate,channel as key. Otherwise search for param_ptr */

    if( (command>CAEN_READ_MOD_ID_CMD)&&(command<CAEN_READ_NAME_CMD) ) {
        /* required data is in status part of cache */
        CAEN_get_cache_status_ptr(crate,
                                  channel,
                                  (struct status_data_type **)&status_ptr,
                                  &table_index,
                                  &result);
        if(result != CAEN_FOUND) {
            /* invalid address */
            return(result);
        }
    } else if( (command>CAEN_READ_STATUS_CMD)&&(command<CAEN_READ_VMAX_B0_CMD)) {
        /* required data is in param part of cache */
        CAEN_get_cache_param_ptr(crate,
                                 channel,
                                 (struct param_data_type **)&param_ptr,
                                 &table_index,
                                 &result);
        if(result != CAEN_FOUND) {
            /* invalid address */
            return(result);
        }
    } else {
        /* invalid command */
        printf("CAEN_read_data: Invalid command type %d\n",command);
        return(CAEN_ERROR);
    }

    /* cache present, copy word(s) from cache to buffer */

    switch (command) {

    case CAEN_READ_V_MON_CMD:
        /* read v mon, two words, MSW first */
        *ptr = (int)status_ptr->v_mon;
        break;

    case CAEN_READ_I_MON_CMD:
        /* read i mon, one word */
        *ptr=(int)(status_ptr->i_mon);
        break;

    case CAEN_READ_STATUS_CMD:
        /* read status, one word */
        *ptr=(int)(status_ptr->status);
        break;

    case CAEN_READ_NAME_CMD:
        /* read channel name ,pointer to string, 12 characters */
        bcopy(param_ptr->name,(char *)ptr,12);
        break;

    case CAEN_READ_V0_CMD:
        /* read V0 set, two words, MSW first */
        *ptr=(int)(param_ptr->v0_set);
        break;

    case CAEN_READ_V1_CMD:
        /* read V1 set, two words, MSW first */
        *ptr=(int)(param_ptr->v1_set);
        break;

    case CAEN_READ_I0_CMD:
        /* read I0 set, one word */
        *ptr=(int)(param_ptr->i0_set);
        break;

    case CAEN_READ_I1_CMD:
        /* read I1 set, one word */
        *ptr=(int)(param_ptr->i1_set);
        break;

    case CAEN_READ_VMAX_CMD:
        /* read V max, one word */
        *ptr=(int)(param_ptr->v_max);
        break;

    case CAEN_READ_RUP_CMD:
        /* read ramp up, one word */
        *ptr=(int)(param_ptr->ramp_up);
        break;

    case CAEN_READ_RDWN_CMD:
        /* read ramp down, one word */
        *ptr=(int)(param_ptr->ramp_down);
        break;

    case CAEN_READ_TRIP_CMD:
        /* read trip, one word */
        *ptr=(int)(param_ptr->trip);
        break;

    case CAEN_READ_HV_CMD:
        /* read hv status (on,off), one word*/
        *ptr=(int)(param_ptr->hv);
        break;

    case CAEN_READ_PASSWD_CMD:
        /* read passwd required, one word*/
        *ptr=(int)(param_ptr->passwd);
        break;

    case CAEN_READ_PDWN_CMD:
        /* read power down mode, one word*/
        *ptr=(int)(param_ptr->power_down);
        break;

    case CAEN_READ_ON_EN_CMD:
        /* read on enable, one word*/
        *ptr=(int)(param_ptr->on_enable);
        break;

    case CAEN_READ_PON_CMD:
        /* read pon, one word*/
        *ptr=(int)(param_ptr->pon);
        break;

    default:
        return(CAEN_ERROR);
        break;

    } /* end case */

    return(CAEN_OK);

}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_get_cache_status_ptr                     */
/*                                                                      */
/*    Function : This procedure scans the CAEN cache table searching    */
/*               for an entry with the specified crate and channel ids. */
/*               It returns the pointer to the status data table.       */
/*                                                                      */
/*    Arguments: short crate         Crate id.                          */
/*               short channel       Channel number.                    */
/*               status_data_type *ptr returned pointer to status data. */
/*               int   *index        Table index for found entry        */
/*               int   *result       Result of search.                  */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/


void CAEN_get_cache_status_ptr(crate, channel, ptr, index, result)
short crate;
short channel;
struct status_data_type **ptr;
int   *index;
int   *result;
{
    int loop_counter;
    int entry_found;
    int end_of_table;

    if(DEBUG)printf("CAEN_get_cache_status_ptr: crate %d chan %d\n",
                        crate,channel);
    /* Search the CAEN cache table, terminating on an */
    /* entry with a negative crate id */
    *ptr=NULL;
    *index=0;
    *result = CAEN_NOT_FOUND;

    loop_counter=0;
    entry_found=FALSE;
    end_of_table=FALSE;
    while((loop_counter<MAX_NUMBER_OF_HV_CHANNELS)&&
            (entry_found==FALSE)&&
            (end_of_table==FALSE)) {
        if( caen_table[loop_counter].crate < 0) {
            /* found end of table */
            end_of_table=TRUE;
            if(DEBUG)
                printf("CAEN_find_cache_entry: EOT: entry %d\n", loop_counter);
        } else if( (caen_table[loop_counter].crate==crate)&&
                   (caen_table[loop_counter].channel==channel)) {
            /* Found entry, Check data is valid */
            *ptr = caen_table[loop_counter].status_ptr;
            *index=loop_counter;
            entry_found=TRUE;
            if(DEBUG)
                printf("CAEN_get_cache_status_ptr: found entry (%p)\n", ptr);
#if 0
            if(current_time() >
                    (caen_table[loop_counter].status_time_stamp +
                     CAEN_DATA_INV_PERIOD)) {
                /* cached data is out of date, warn record */
                *result=CAEN_INVALID_DATA;
                return;
            } else {
#endif
                *result=CAEN_FOUND;
#if 0
            }
#endif
        } else {
            /* increment loop counter */
            loop_counter++;
        }
    } /* end while */

    if(loop_counter == MAX_NUMBER_OF_HV_CHANNELS) {
        /* ran off end of table without finding anything */
        if(DEBUG)printf("CAEN_get_cache_status_ptr: error, at boundary\n");
    }
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_get_cache_param_ptr                      */
/*                                                                      */
/*    Function : This procedure scans the CAEN cache table searching    */
/*               for an entry with the specified crate and channel ids. */
/*               It returns the pointer to the param  data table.       */
/*                                                                      */
/*    Arguments: short crate         Crate id.                          */
/*               short channel       Channel number.                    */
/*               param_data_type *ptr returned pointer to param data.   */
/*               int   *index        table index of found entry.        */
/*               int   *result       Result of search.                  */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_get_cache_param_ptr(crate, channel, ptr, index, result)
short crate;
short channel;
struct param_data_type **ptr;
int   *index;
int   *result;
{
    int loop_counter;
    int entry_found;
    int end_of_table;

    if(DEBUG)printf("CAEN_get_cache_param_ptr: crate %d chan %d\n",
                        crate,channel);
    /* Search the CAEN cache table, terminating on an */
    /* entry with a neg crate id */
    *ptr=NULL;
    *index=0;
    *result = CAEN_NOT_FOUND;

    loop_counter=0;
    entry_found=FALSE;
    end_of_table=FALSE;
    while((loop_counter<MAX_NUMBER_OF_HV_CHANNELS)&&
            (entry_found==FALSE)&&
            (end_of_table==FALSE)) {
        if( caen_table[loop_counter].crate < 0) {
            /* found end of table */
            end_of_table=TRUE;
            if(DEBUG)
                printf("CAEN_find_cache_entry: EOT: entry %d\n", loop_counter);
        } else if( (caen_table[loop_counter].crate==crate)&&
                   (caen_table[loop_counter].channel==channel)) {
            /* Found entry, Check data is valid */
            *ptr = caen_table[loop_counter].param_ptr;
            *index=loop_counter;
            entry_found=TRUE;
            if(DEBUG)
                printf("CAEN_get_cache_param_ptr: found entry (%p)\n", ptr);
#if 0
            if(current_time() >
                    (caen_table[loop_counter].param_time_stamp +
                     CAEN_DATA_INV_PERIOD)) {
                /* cached data is out of date, warn record */
                *result=CAEN_INVALID_DATA;
                return;
            } else {
#endif
                *result=CAEN_FOUND;
#if 0
            }
#endif
        } else {
            /* increment loop counter */
            loop_counter++;
        }
    } /* end while */

    if(loop_counter == MAX_NUMBER_OF_HV_CHANNELS) {
        /* ran off end of table without finding anything */
        if(DEBUG)printf("CAEN_get_cache_param_ptr: error, at boundary\n");
    }
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_cache                                    */
/*                                                                      */
/*    Function : This procedure prints a summary of the cache.          */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  :                                                        */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_cache()
{
    int index;

    printf("\n\n");
    printf(" S U R A                           C E B A F\n\n");
    printf("        C A E N Device Suport Cache Table\n\n");
    printf("              D Barker. 1 Mar 94.\n");
    printf("(c)SURA CEBAF 1994.\n\n");

    printf("# Cr Ch     Status Info           Param Info\n");
    printf("        (Ptr    TS   SO    LAM)   (Ptr    TS   SO   LAM)\n");
    index=0;
    while(caen_table[index].crate >=0) {
        printf("%3d %2d %2d %2d 0x%6x %8d %5d %d  0x%6x %8d %5d %d\n",
               index,
               caen_table[index].crate,
               caen_table[index].channel,
               caen_table[index].group,
               caen_table[index].status_ptr,
               caen_table[index].status_time_stamp,
               caen_table[index].status_stale_out_period,
               caen_table[index].status_LAM,
               caen_table[index].param_ptr,
               caen_table[index].param_time_stamp,
               caen_table[index].param_stale_out_period,
               caen_table[index].param_LAM);
        index++;
    }
    printf("\n");
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_group                                    */
/*                                                                      */
/*    Function : This procedure prints a summary of the cache.          */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  :                                                        */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_group(group)
short group;
{
    int index;

    printf("\n\n");
    printf(" S U R A                           C E B A F\n\n");
    printf("        C A E N Device Suport Cache Table\n\n");
    printf("              D Barker. 1 Mar 94.\n");
    printf("(c)SURA CEBAF 1994.\n\n");
    printf("Cache information for group %d\n\n",group);;

    printf("# Cr Ch     Status Info           Param Info\n");
    printf("        (Ptr    TS   SO    LAM)   (Ptr    TS   SO   LAM)\n");
    index=0;
    while(caen_table[index].crate >=0) {
        if(caen_table[index].group==group) {
            printf("%3d %2d %2d %2d 0x%6x %8d %5d %d  0x%6x %8d %5d %d\n",
                   index,
                   caen_table[index].crate,
                   caen_table[index].channel,
                   caen_table[index].group,
                   caen_table[index].status_ptr,
                   caen_table[index].status_time_stamp,
                   caen_table[index].status_stale_out_period,
                   caen_table[index].status_LAM,
                   caen_table[index].param_ptr,
                   caen_table[index].param_time_stamp,
                   caen_table[index].param_stale_out_period,
                   caen_table[index].param_LAM);
        }
        index++;
    }
    printf("\n");
}

