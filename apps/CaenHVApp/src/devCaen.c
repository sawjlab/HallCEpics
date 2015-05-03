#define SAWDBG 1
#define USE_VXMEMPROBE
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
/*      .00 022494 db   Init Release                                    */
/*      .01 112294 db   Tx of Set command checks the error code, if its */
/*                      not OK, will retry that command.                */
/*      .02 112394 db   Asyn task can be put into "idle" state. No      */
/*                      actual CaenNet comms, but all timestamps updated*/
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

#define DEBUG2 0
#define DEBUG3 0

#define ASYN_DEV_LOOP_DELAY  1
double sleep_time=0.001;
double spin_time=0.0001;

int SOS_CAEN_IDLE;		/* One of these gets set by a vxWorks Variable */
int HMS_CAEN_IDLE;		/* binary output record. */
int GEN_CAEN_IDLE;
int G0_CAEN_IDLE;
int TEMP_CAEN_IDLE;
int HKS_CAEN_IDLE;
int GEP_CAEN_IDLE;
#define CAEN_IDLE (SOS_CAEN_IDLE || HMS_CAEN_IDLE || GEN_CAEN_IDLE || G0_CAEN_IDLE || TEMP_CAEN_IDLE || HKS_CAEN_IDLE || GEP_CAEN_IDLE)
int CAEN_DEBUG;
int CAEN_RECORD_DEBUG;
int CAEN_NUM_TX = 0;
int CAEN_NUM_RX = 0;
int CAEN_TX_TO =0;
int CAEN_RX_TO =0;
int RESET_CRATE = 0;		/* Temporary Crate reset procedure */
char *CAEN_BASE_LADDR=0;

#include <stdlib.h>
#include <stdio.h>
#include <vxWorks.h>
#include <vxLib.h>
#include <taskLib.h>
#include <ioLib.h>
#include <fioLib.h>
#include <Caen.h>
#include <time.h>
#include <timers.h>
#define WATCHDOG
#ifdef WATCHDOG
#include <dbDefs.h>
#include <dbFldTypes.h>
#include <dbBase.h>
#include <dbAccess.h>
static struct dbAddr *DogPtr=0;

void setdogname(char *dogname)
{
    DogPtr = (struct dbAddr *) malloc(sizeof(struct dbAddr));
    dbNameToAddr(dogname, DogPtr);
}
#endif

void CAEN_write_tx_buf(short data);
void CAEN_write_tx_reg();
void CAEN_reset();
void CAEN_read_rx_buf(short *data);
void CAEN_read_status_reg(short *data);
void CAEN_spin(int iterations);

#if 1
/************************************************************************/
/*                                                                      */
/*    External Procedure: current_time                                  */
/*                                                                      */
/*    Function : This procedure returns the current time in mS.         */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  : The current time                                       */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
static unsigned long current_time()
{
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME,&tp);
    return(tp.tv_sec*1000+(tp.tv_nsec/1000000));

}
#endif

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_send_raw_command                         */
/*                                                                      */
/*    Function : This procedure issues the correct sequence of VME      */
/*               commands to the V288 CAEN VME board which will result  */
/*               in the requested command being sent. Note, some        */
/*               commands (e.g. HV on) set a bit in a CAEN command word.*/
/*               Not all command types are supported, this procedure is */
/*               mainly used by the device support during initialisation*/
/*               to verify that the hardware configuration is correct   */
/*               before creating the asynchronous task and its cache.   */
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
int CAEN_send_raw_command(crate, channel, command, value)
short  crate;
short  channel;
short  command;
short  value;
{
    short op_code;
    short status;
    short temp;
    unsigned short error_code;
    int   loop_counter;
    int   timedout;
    int   Tx_counter;
    int   Tx_successful;
    short send_value;

    op_code=0;

    if ( (command < CAEN_WRITE_V0_CMD) ||
            ((command>CAEN_RESET_TRIPS_CMD)&&(command<CAEN_READ_MOD_ID_CMD))||
            (command > CAEN_READ_PON_CMD)) {
        if (DEBUG)printf("devCaen.c: Command not supported: %d\n",command);
        return(OK);
    }

    /* Try to Send a Set command CAEN_SET_CMD_RETRIES number of times. */
    /* If Crate is constantly busy, abort the command. */

    Tx_counter=0;
    Tx_successful = 0;

    while ( (Tx_successful == 0 ) && (Tx_counter < CAEN_SET_CMD_RETRIES)) {
        /* Try to set the command until successful, or timeout */
        status = CAEN_STATUS_ERROR;
        loop_counter=0;
        while (status!=(short)CAEN_STATUS_OK) {
            /* Send the controller's Id to the Tx buffer */
            CAEN_write_tx_buf((short)CAEN_CONTROLLER_ID);

            /* Send the destination crate id to the Tx buffer */
            CAEN_write_tx_buf(crate);

            /* Send the operation code to the Tx buffer */
            switch (command) {
            case CAEN_WRITE_V0_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_V0_SET_VAL);
                break;

            case CAEN_WRITE_V1_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_V1_SET_VAL);
                break;

            case CAEN_WRITE_I0_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_I0_SET_VAL);
                break;

            case CAEN_WRITE_I1_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_I1_SET_VAL);
                break;

            case CAEN_WRITE_VMAX_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_VMAX_SET_VAL);
                break;

            case CAEN_WRITE_RUP_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_RUP_SET_VAL);
                break;

            case CAEN_WRITE_RDWN_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_RDWN_SET_VAL);
                break;

            case CAEN_WRITE_TRIP_CMD:
                op_code = (short)(((channel&0xff)<<8)+CAEN_WRITE_TRIP_SET_VAL);
                break;

            case CAEN_WRITE_HV_CMD:
            case CAEN_WRITE_PASSWD_CMD:
            case CAEN_WRITE_PDWN_CMD:
            case CAEN_WRITE_ON_EN_CMD:
            case CAEN_WRITE_PON_CMD:
            case CAEN_WRITE_HV_OFF_CMD:
                op_code = (short)( ((channel&0xff)<<8)+CAEN_WRITE_FLAG_SET_VAL);
                break;

            case CAEN_READ_MOD_ID_CMD:
                /* Set opcode for read module id */
                op_code = (short)CAEN_READ_MOD_ID;
                break;

            case CAEN_RESET_TRIPS_CMD:
                /* Set opcode to reset the trips in the crate */
                op_code = (short)CAEN_RESET_TRIPS;
                break;

            case CAEN_READ_V_MON_CMD:
            case CAEN_READ_I_MON_CMD:
            case CAEN_READ_STATUS_CMD:
                /* Set opcode for read module id */
                op_code = (short)( ((channel&0xff)<<8) + CAEN_READ_CH_STATUS);
                break;

            case CAEN_READ_NAME_CMD:
            case CAEN_READ_V0_CMD:
            case CAEN_READ_V1_CMD:
            case CAEN_READ_I0_CMD:
            case CAEN_READ_I1_CMD:
            case CAEN_READ_VMAX_CMD:
            case CAEN_READ_RUP_CMD:
            case CAEN_READ_RDWN_CMD:
            case CAEN_READ_TRIP_CMD:
            case CAEN_READ_HV_CMD:
            case CAEN_READ_PASSWD_CMD:
            case CAEN_READ_PDWN_CMD:
            case CAEN_READ_ON_EN_CMD:
            case CAEN_READ_PON_CMD:
                /* Set opcode for read module id */
                op_code = (short)( ((channel&0xff)<<8) + CAEN_READ_CH_PARAMS);
                break;

            default:
                /* Invalid command type */
                if (DEBUG)printf("devCaen.c: Invalid command %d\n",command);
                /* reset VME */
                CAEN_reset();
                break;

            } /* end switch */

            /* write op code to Tx buffer */
            CAEN_write_tx_buf((short)op_code);

            /* If Command is a Write, process value to send and send it */
            if ( (command<=CAEN_RESET_TRIPS_CMD)&&
                    (command>=CAEN_WRITE_V0_CMD)) {
                /* write cmd, process the value field for those commands */
                /* which set a bit in the value word. */
                switch(command) {
                case CAEN_WRITE_HV_CMD:
                    /* Value for this command is a bit in the final value*/
                    temp=value;
                    send_value=0;
                    /* Set HV Mask and HV Flag */
                    send_value |= CAEN_HV_MASK;
                    if (temp != 0) {
                        /* switch HV on */
                        send_value |= CAEN_HV_ON_FLAG;
                    } else {
                        /* switch HV off */
                        send_value |= CAEN_HV_OFF_FLAG;
                    }
                    break;
                case CAEN_WRITE_HV_OFF_CMD:
                    /* Send a switch HV off command, regardless of the value.*/
                    send_value=0;
                    send_value|=CAEN_HV_MASK;
                    send_value|=CAEN_HV_OFF_FLAG;
                    break;
                case CAEN_WRITE_PASSWD_CMD:
                    /* Value for this command is a bit in the final value*/
                    temp=value;
                    send_value=0;
                    /* Set Passwd Mask and Flag */
                    send_value |= CAEN_PASSWD_MASK;
                    if (temp != 0) {
                        /* switch Passwd req on */
                        send_value |= CAEN_PASSWD_REQ_FLAG;
                    } else {
                        /* switch passwd not req */
                        send_value |= CAEN_PASSWD_NREQ_FLAG;
                    }
                    break;
                case CAEN_WRITE_PDWN_CMD:
                    /* Value for this command is a bit in the final value*/
                    temp=value;
                    send_value=0;
                    /* Set power down mode Mask and Flag */
                    send_value |= CAEN_PDWN_MASK;
                    if (temp != 0) {
                        /* switch pdwn to ramp down */
                        send_value |= CAEN_PDWN_RDWN_FLAG;
                    } else {
                        /* switch pdwn to kill. */
                        send_value |= CAEN_PDWN_KILL_FLAG;
                    }
                    break;
                case CAEN_WRITE_ON_EN_CMD:
                    /* Value for this command is a bit in the final value*/
                    temp=value;
                    send_value=0;
                    /* Set on enable Mask and Flag */
                    send_value |= CAEN_ON_EN_MASK;
                    if (temp != 0) {
                        /* switch on to enabled */
                        send_value |= CAEN_ON_EN_FLAG;
                    } else {
                        /* switch on to disabled */
                        send_value |= CAEN_ON_DIS_FLAG;
                    }
                    break;
                case CAEN_WRITE_PON_CMD:
                    /* Value for this command is a bit in the final value*/
                    temp=value;
                    send_value=0;
                    /* Set on at poweron mode */
                    send_value |= CAEN_PON_MASK;
                    if (temp != 0) {
                        /* switch pon to enabled */
                        send_value |= CAEN_PON_ON_FLAG;
                    } else {
                        /* switch pon to disabled */
                        send_value |= CAEN_PON_OFF_FLAG;
                    }
                    break;
                default:
                    send_value=value;
                    break;
                } /* end switch */
                CAEN_write_tx_buf((short)send_value);
            } /* end if write cmd. */

            /* Check status register */
            CAEN_read_status_reg(&status);

            loop_counter++;

            if (loop_counter > CAEN_TX_TIMEOUT) {
                printf("devCaen.c: Snd Raw Cmd TIMEOUT\n");
                CAEN_TX_TO++;
                CAEN_reset();
                return((int)CAEN_STATUS_ERROR);
            }

            /* pause before trying again */
            CAEN_spin(1);

        }  /* end while status error and not timed out */

        /* Send the command */
        CAEN_write_tx_reg();

        /* Check status register */
        CAEN_read_status_reg(&status);
        if (status != (short)CAEN_STATUS_OK) {
            printf("devCaen.c: Snd Raw Cmd Tx Reg Error\n");
            return((int)status);
        }

        /* Poll the rx buffer, if it does not respond within */
        /* CAEN_RX_TIMEOUT iterations, return with error */

        status=CAEN_STATUS_ERROR;
        loop_counter=0;
        timedout=FALSE;
        while ( (status != (short)CAEN_STATUS_OK) && (timedout==FALSE)) {
            CAEN_spin(1);

            /* Read the Error Code */
            CAEN_read_rx_buf(&error_code);

            /* Check status register */
            CAEN_read_status_reg(&status);

            loop_counter++;
            if (loop_counter>=CAEN_RX_TIMEOUT) {
                if (status != (short)CAEN_STATUS_OK) timedout=TRUE;
            }
        } /* end while */

        if (timedout == TRUE) {
            if (DEBUG) printf("devdevCaen.c: Rd Raw Data TIMEDOUT\n");
            CAEN_RX_TO++;
            CAEN_reset();
            return((int)CAEN_TIMEOUT_ERROR);
        }

        /* Have read error_code */

        if (error_code == CAEN_OK ) {
            /* module not busy */
            Tx_successful = 1;
        } else if (error_code == CAEN_MODULE_BUSY) {
            Tx_counter++;
        } else {
            if (DEBUG) printf("CAEN_send_raw_command error %x\n",error_code);
            return((int)CAEN_ERROR);
        }
    } /* End of Tx_successful loop */

    if (Tx_successful == 1) {
        CAEN_NUM_TX++;
    }

    /* Command Sent */
    return(OK);

}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_read_raw_data                            */
/*                                                                      */
/*    Function : This procedure issues the correct sequence of VME      */
/*               commands to the V288 CAEN VME board which will result  */
/*               in the requested data being read off the Rx buffer.    */
/*               The number of words to be read is dependent upon the   */
/*               data structure the command is in, e.g. status data or  */
/*               params data. All of the data structure will be read,   */
/*               and the cache entry for that cr,ch will be updated     */
/*               with the data, and the timestamp updated.              */
/*               This procedure accesses the driver support directly,   */
/*               it is preceded by a CAEN_send_raw_command() call,      */
/*               requests the data which is procedure reads.            */
/*                                                                      */
/*    Arguments: short crate         channel crate id.                  */
/*               short channel       channel id.                        */
/*               short *buffer       buffer where data is to be put,    */
/*                                   data may be 1-n words in length.   */
/*               short *result       result of read.                    */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
int CAEN_read_raw_data(crate, channel, command, buffer, result)
short crate;
short channel;
short command;
short buffer[];
short *result;
{
    short data;
    short status;
    short rx_data[32]; /* local buffer of all data read from crate */
    int   data_count;
    int   i;
    int   res;
    int   table_index;
    struct status_data_type *status_ptr;
    struct param_data_type  *param_ptr;

    /* NOTE: this procedure stores its data into the local cache, the */
    /* only exceptions are CAEN_READ_MOD_ID_CMD and READ_STATUS_CMD. */
    /* the former is not stored in the cache, the second is, but is  */
    /* used for record initialization before the cache is complete.  */

    *result = (short)CAEN_OK;

    /* Don't Poll the rx buffer, this polling has already been done by
       CAEN_send_raw_command  */

    /* Read data word-by-word from the rx buffer, counting as */
    /* it proceeds. The first invalid entry is indicated by   */
    /* an error status. If the total number of words read does */
    /* not match what the command expects raise an error. */
    data_count=0;
    status = CAEN_STATUS_OK;
    while ( status == (short)CAEN_STATUS_OK) {
        /* Read the Data word */
        CAEN_read_rx_buf(&data);

        /* Check status register */
        CAEN_read_status_reg(&status);

        if (status == (short)CAEN_STATUS_OK) {
            rx_data[data_count]=data;
            data_count++;
        }
    } /* end while */

    /* Check data_count and extract requested word(s) from buffer */
    if (command == CAEN_READ_MOD_ID_CMD) {
        /* expect CAEN_MOD_ID_SIZE words */
        if (data_count != CAEN_MOD_ID_SIZE) {
            /* did not get all of the data. error */
            printf("devCaen.c: Rd Raw Data, Inv Num Data read (mod id)\n");
            return(ERROR);
        }
        /* mod id consists of 11 words. */
        /* copy data into buffer */
        for (i=0; i<11; i++) {
            buffer[i]=(short)rx_data[i];
        }

    } else if ( (command >= CAEN_READ_V_MON_CMD) &&
                (command <= CAEN_READ_STATUS_CMD)) {
        /* expect CAEN_CH_STATUS_SIZE words */
        if (data_count != CAEN_CH_STATUS_SIZE) {
            /* did not get all of the data. error */
            printf("devCaen.c: Rd Raw Data, Inv Num Data read (ch status)\n");
#ifdef SAWDBG
            printf("Data count = %d/%d\n",data_count,CAEN_CH_STATUS_SIZE);
            for (i=0; i<((data_count < 2*CAEN_CH_STATUS_SIZE) ?
                         data_count : 2*CAEN_CH_STATUS_SIZE); i++) {
                printf("%d ",rx_data[i]);
            }
            printf("\n");
#endif
            return(ERROR);
        }

        /* Update the Status Cache entry for this crate, channel. */

        CAEN_get_cache_status_ptr(crate,
                                  channel,
                                  &status_ptr,
                                  &table_index,
                                  &res);
        if (res == CAEN_NOT_FOUND) {
            /* invalid crate,chan */
            printf("devCaen.c:A: Rd Raw Data, inv crate,chann addr\n");
            return(ERROR);
        }

        /* update cache data */
        (status_ptr->v_mon) =(int)((rx_data[0]<<16)+rx_data[1]);
        (status_ptr->i_mon) =rx_data[2];
        (status_ptr->status)=rx_data[3];
        if (DEBUG) printf("(%d %d): %d %d %d\n",crate,channel
                              ,(int)(status_ptr->v_mon)
                              ,(int)(status_ptr->i_mon)
                              ,(int)(status_ptr->status));

        /* update cache time stamp */
        caen_table[table_index].status_time_stamp = current_time();


        /* return requested data is command is for status */
        if (command==CAEN_READ_STATUS_CMD) {
            buffer[0]=(short)rx_data[CAEN_READ_STATUS_POS];
        }
    } else if ( (command >= CAEN_READ_NAME_CMD) &&
                (command <= CAEN_READ_PON_CMD)) {
        /* expect CAEN_CH_PARMS_SIZE words */
        if (data_count != CAEN_CH_PARMS_SIZE) {
            /* did not get all of the data. error */
            printf("devCaen.c: Rd Raw Data, Inv Num Data read (ch params)\n");
#ifdef SAWDBG
            printf("Data count = %d/%d\n",data_count,CAEN_CH_PARMS_SIZE);
            for (i=0; i<((data_count < 2*CAEN_CH_PARMS_SIZE) ?
                         data_count : 2*CAEN_CH_PARMS_SIZE); i++) {
                printf("%d ",rx_data[i]);
            }
            printf("\n");
#endif
            return(ERROR);
        }

        /* Update the Params Cache entry for this crate, channel. */

        CAEN_get_cache_param_ptr(crate,
                                 channel,
                                 &param_ptr,
                                 &table_index,
                                 &res);
        if (res == CAEN_NOT_FOUND) {
            /* invalid crate,chan */
            printf("devCaen.c:B: Rd Raw Data, inv crate,chann addr\n");
            return(ERROR);
        }

        /* update param data */
        for (i=0; i<6; i++) {
            (param_ptr->name[2*i])=(char)(rx_data[i]&0xff);
            (param_ptr->name[2*i+1])=(char)(rx_data[i]&0xff00);
        }
        (param_ptr->v0_set) =(int)((rx_data[6]<<16)+rx_data[7]);
        (param_ptr->v1_set) =(int)((rx_data[8]<<16)+rx_data[9]);
        (param_ptr->i0_set) =rx_data[10];
        (param_ptr->i1_set) =rx_data[11];
        (param_ptr->v_max)  =rx_data[12];
        (param_ptr->ramp_up) =rx_data[13];
        (param_ptr->ramp_down) =rx_data[14];
        (param_ptr->trip) =rx_data[15];
        (param_ptr->hv) =  (short)(rx_data[16]&CAEN_HV_MASK);
        (param_ptr->passwd) =  (short)(rx_data[16]&CAEN_PASSWD_MASK);
        (param_ptr->power_down) =  (short)(rx_data[16]&CAEN_PDWN_MASK);
        (param_ptr->on_enable) =  (short)(rx_data[16]&CAEN_ON_EN_MASK);
        (param_ptr->pon) =  (short)(rx_data[16]&CAEN_PON_MASK);

        /* update cache time stamp */
        caen_table[table_index].param_time_stamp = current_time();

    } else if ( command == CAEN_READ_BSY_CMD) {
        /* expect CAEN_BSY_STATUS_SIZE words */
        if (data_count != CAEN_BSY_STATUS_SIZE) {
            /* did not get all of the data. error */
            return(ERROR);
        }
    } else {
        /* invalid command type */
        return(ERROR);
    }

    return(OK);
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_update_cache                             */
/*                                                                      */
/*    Function : This procedure issues the correct sequence of VME      */
/*               commands to the V288 CAEN VME board which will result  */
/*               in the requested data being requested and read.        */
/*               The data of type status or params is read and the      */
/*               cache entry for this crate,channel address is updated  */
/*               with this data. The time stamp is then made current.   */
/*                                                                      */
/*    Arguments: short crate         channel crate id.                  */
/*               short channel       channel id.                        */
/*               short data_type     the type of data (status, param)   */
/*                                                                      */
/*    Returns  : The error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
int CAEN_update_cache(crate, channel, data_type)
short crate;
short channel;
short data_type;
{
    short data;
    short  result;
    int   status;
    int   res;
    int   table_index;
    struct status_data_type *status_ptr;
    struct param_data_type  *param_ptr;

    if (data_type == CAEN_STATUS_DATA_TYPE) {
        /* if not in idle state, read data */
        if (!CAEN_IDLE) {
            /* Send command to read all status data */
            status=CAEN_send_raw_command(crate,
                                         channel,
                                         CAEN_READ_STATUS_CMD,
                                         0);
            if (status != OK) {
                if (DEBUG)printf("CAEN_update_cache: status: \
			   CAEN_send_raw_command %d\n",status);
                return(status);
            }

            status=CAEN_read_raw_data(crate,
                                      channel,
                                      CAEN_READ_STATUS_CMD,
                                      &data,
                                      &result);
            if (status != OK) {
                if (DEBUG)printf("CAEN_update_cache: status: \
			   CAEN_read_raw_data %d\n",status);
                return(status);
            }
            if (result != (short)CAEN_OK) {
                if (result == (short)CAEN_TIMEOUT_ERROR) {
                    if (DEBUG)printf("Rx TO\n");
                } else {
                    if (DEBUG)printf("CAEN_update_cache: result: CAEN_read_raw_data %d\n", result);
                    return((int)result);
                }
            }
        } else { /* in idle mode, update the time stamp to keep records honest. */
            CAEN_get_cache_status_ptr(crate,
                                      channel,
                                      &status_ptr,
                                      &table_index,
                                      &res);
            if (res == CAEN_NOT_FOUND) {
                /* invalid crate,chan */
                printf("devCaen.c:C: Rd Raw Data, inv crate,chann addr\n");
                return(ERROR);
            }

            /* update cache time stamp */
            caen_table[table_index].status_time_stamp = current_time();
        }
    } else if (data_type == CAEN_PARAM_DATA_TYPE) {
        /* if not in idle state, read data */
        if (!CAEN_IDLE) {
            /* Send command to read all param data */
            status=CAEN_send_raw_command(crate,
                                         channel,
                                         CAEN_READ_NAME_CMD,
                                         0);
            if (status != OK) {
                if (DEBUG)printf("CAEN_update_cache: param: \
			   CAEN_send_raw_command %d\n",status);
                return(status);
            }

            status=CAEN_read_raw_data(crate,
                                      channel,
                                      CAEN_READ_NAME_CMD,
                                      &data,
                                      &result);
            if (status != OK) {
                if (DEBUG)printf("CAEN_update_cache: status: \
			   CAEN_read_raw_data %d\n",status);
                return(status);
            }
            if (result != (short)CAEN_OK) {
                if (result == (short)CAEN_TIMEOUT_ERROR) {
                    if (DEBUG)printf("Rx TO\n");
                } else {
                    if (DEBUG)printf("CAEN_update_cache: result: \
			     CAEN_read_raw_data %d\n",result);
                    return((int)result);
                }
            }
        } else { /* in idle mode, update the time stamp to keep records honest. */
            CAEN_get_cache_param_ptr(crate,
                                     channel,
                                     &param_ptr,
                                     &table_index,
                                     &res);
            if (res == CAEN_NOT_FOUND) {
                /* invalid crate,chan */
                printf("devCaen.c:D: Rd Raw Data, inv crate,chann addr\n");
                return(ERROR);
            }

            /* update cache time stamp */
            caen_table[table_index].param_time_stamp = current_time();

        }
    } else {
        printf("Update Cache: invalid data type\n");
        return(ERROR);
    }

    return(OK);
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_set_stale_out_period.                    */
/*                                                                      */
/*    Function : This procedure set the stale out period for a cache    */
/*               entry.                                                 */
/*                                                                      */
/*    Arguments: short  crate          Crate ID .                       */
/*               short  channel        Channel ID.                      */
/*               short  data_type      0=status, 1=param.               */
/*               long   stale_out_period   Period in milliseconds.      */
/*                                                                      */
/*    Returns  : The current time                                       */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_set_staleout(crate,channel,data_type,period)
short  crate;
short  channel;
short  data_type;
long   period;
{
    int result;
    int table_index;
    struct status_data_type *status_ptr;

    /* search for table entry with this crate, channel */
    CAEN_get_cache_status_ptr(crate,
                              channel,
                              &status_ptr,
                              &table_index,
                              &result);
    if (result != CAEN_FOUND) {
        printf("CAEN_set_stale_out_period: Invalid crate,chan address\n");
        return;
    }

    if (data_type == CAEN_STATUS_DATA_TYPE) {
        /* set status stale out period */
        caen_table[table_index].status_stale_out_period = (long)period;
    } else if (data_type == CAEN_PARAM_DATA_TYPE) {
        /* set param stale out period */
        caen_table[table_index].param_stale_out_period = (long)period;
    } else {
        printf("CAEN_set_stale_out_period: Invalid data type\n");
    }


}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_get_table_index                          */
/*                                                                      */
/*    Function : This procedure searches the CAEN cache table for an    */
/*               entry with the supplied crate, channel parameters.     */
/*               It returns the entries index. Otherwise, a neg index.  */
/*                                                                      */
/*    Arguments: short  crate          Crate ID .                       */
/*               short  channel        Channel ID.                      */
/*               short  *index         returned index.                  */
/*                                                                      */
/*    Returns  : The current time                                       */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_get_table_index(crate,channel,index)
short  crate;
short  channel;
int    *index;
{

    int i;

    *index= -1;
    i=0;
    while ( caen_table[i].crate > 0) {
        if ( (caen_table[i].crate == crate)&&
                (caen_table[i].channel == channel)) {
            /* Found entry */
            *index=i;
            return;
        }
        i++;
    }

}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_dev_sup                                  */
/*                                                                      */
/*    Function : This procedure is the asynchronous part of the CAEN    */
/*               device support. It runs under its own task at a lower  */
/*               priority to the EPICS system.                          */
/*               It receives Tx msgs from EPICS via the two pipes       */
/*               HV_OFF_TX_PIPE_FD and HV_TX_PIPE_FD. It keeps a local  */
/*               cache of the HV data current by periodically reading   */
/*               all channel data.                                      */
/*               During the processing loop, this task does the following*/
/*               1) check if a msg is in HV_OFF_TX_PIPE_FD, if so       */
/*                  service the request and end.                        */
/*               2) check if a msg is in HV_TX_PIPE_FD, if so           */
/*                  service the request and end.                        */
/*               3) Check if an update is necessary, if so read the data*/
/*                  from the next unread channel.                       */
/*               4) if none of the above, then do nothing.              */
/*               5) end of loop.                                        */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  :                                                        */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_dev_sup()
{
    int status;
    int num_bytes;
    struct caen_tx_msg_type msg_buffer;
    struct status_data_type *ptr;
    int   index;
    int   search;
    int      read_count;
    int      chan_loop;
    int      channel_num;
    int      LAM_serviced;
    int      last_LAM;
    int      read_channel;
    int      all_done;
    int      retry_count;
#ifdef WATCHDOG
    long DogVal=1;
    int watchdog_counter=0;
#endif
    int loops=0;
    unsigned long lastlooptime=0;
    read_count=0;
    chan_loop =0;
    last_LAM = 0;
    read_channel=0;

    /* Go Into Processing Loop */
    while (1) {
#ifdef WATCHDOG
        if (DogPtr) {
            if (watchdog_counter++ > 1000) {
                dbPutField(DogPtr, DBR_LONG, &DogVal, sizeof(DogVal));
                watchdog_counter=0;
                /*	  printf("dog wagged\n");*/
            }
        }
#endif

        if (RESET_CRATE) {		/* A hack to reset trips */
            int temp1[10],temp2;
            printf("Attempting to reset trips in crate %d\n",RESET_CRATE);
            CAEN_send_raw_command(RESET_CRATE,0,CAEN_RESET_TRIPS_CMD,0);
            CAEN_read_raw_data(RESET_CRATE,0,CAEN_RESET_TRIPS_CMD,&temp1,&temp2);
            RESET_CRATE=0;
        }
        /* if (DEBUG)printf("Asyn Task: start of loop\n"); */
        /* Check HV OFF CMD Tx buffer */
        status = ioctl(HV_OFF_TX_PIPE_FD,FIONREAD,(int)&num_bytes);
        if (status == ERROR) {
            /* pipe error */
            printf("CAEN Task: pipe error reading pipe %s.\n",
                   HV_OFF_TX_PIPE_NAME);
            exit(0);
        }

        if (num_bytes > 0) {
            /* HV OFF TX MSG(S) AWAITING TRANSMISSION */

            if (DEBUG)printf("Asyn Task: hv off cmd\n");
            /* Loop over all messages (to prevent inf loop, stop */
            /* when MAX_NUMBER_OF_HV_CHANNELS has been processed)*/
            channel_num=0;
            while ( (num_bytes>= sizeof(CAEN_TX_MSG)) &&
                    (channel_num < MAX_NUMBER_OF_HV_CHANNELS)) {

                /* Check that there are full message(s) in pipe */
                if ((num_bytes%sizeof(CAEN_TX_MSG))!=0) {
                    /* corrupted messages */
                    printf("CAEN Task:HVOFF CMD pipe does not have complete msg(s)\n");
                    exit(0);
                }

                /* pipe has msg(s) for transmission */
                /* Read msg */
                status = read(HV_OFF_TX_PIPE_FD,
                              (void *)&msg_buffer,
                              sizeof(msg_buffer));

                if (status < sizeof(msg_buffer)) {
                    /* invalid data */
                    printf("CAEN Task: read from HV OFF CMD pipe incomplete.\n");
                    exit(0);
                }


                if (DEBUG) {
                    printf("CAEN Task: received Tx message for HV OFF.");
                    printf("crate %d channel %d\n",msg_buffer.crate,msg_buffer.channel);
                }

                /* If not in idle state, Send the message */
                if (!CAEN_IDLE) {
                    /* Due to the extreme importance that this command is */
                    /* processed. Try CAEN_OFF_RETRIES times */
                    retry_count=0;
                    status=ERROR;
                    while ( (status!=OK)&&(retry_count<CAEN_HV_OFF_RETRIES)) {
                        status = CAEN_send_raw_command (msg_buffer.crate,
                                                        msg_buffer.channel,
                                                        msg_buffer.command,
                                                        msg_buffer.value);
                        retry_count++;
                    }
                    if (status != OK) {
                        printf("CAEN Task: Send HV OFF cmd fail after %d tries.\n",
                               retry_count);
                        /* exit(0); */
                    }
                }

                /* To Fix the bug that the Caen VME returns OK but the
                 * off command actually is not effective if many off commands
                 * are fired as quickly as possible, delay before sending
                 * the next.
                 */

                CAEN_spin(20);

                /* recalc number of bytes in pipe. */
                status = ioctl(HV_OFF_TX_PIPE_FD,FIONREAD,(int)&num_bytes);
                if (status == ERROR) {
                    /* pipe error */
                    printf("CAEN Task: pipe error reading pipe %s.\n",
                           HV_OFF_TX_PIPE_NAME);
                    exit(0);
                }

                channel_num++;
            } /* end while */

        } /* end of HV OFF processing */
        else {

            /* No HV OFF cmds in pipe, so Check HV Tx pipe */
            status = ioctl(HV_TX_PIPE_FD,FIONREAD,(int)&num_bytes);
            if (status == ERROR) {
                /* pipe error */
                printf("CAEN Task: pipe error reading pipe %s.\n",
                       HV_TX_PIPE_NAME);
                exit(0);
            }

            if (num_bytes > 0) {
                /* CAEN TX CMD(S) FOUND */

                if (DEBUG)printf("Asyn Task: Tx cmd\n");

                /* Loop over next MAX_NUM_TX_CMD_PER_LOOP messages */
                channel_num=0;
                while ( (num_bytes>= sizeof(CAEN_TX_MSG)) &&
                        (channel_num < MAX_NUM_TX_CMD_PER_LOOP)) {

                    /* Check that there are full message(s) in pipe */
                    if ((num_bytes%sizeof(CAEN_TX_MSG))!=0) {
                        /* corrupted messages */
                        printf("CAEN Task: HV CMD pipe does not have full msg\n");
                        exit(0);
                    }

                    /* pipe has msg(s) for transmission */
                    /* Read msg */
                    status = read(HV_TX_PIPE_FD,
                                  (void *)&msg_buffer,
                                  sizeof(msg_buffer));

                    if (status < sizeof(msg_buffer)) {
                        /* invalid data */
                        printf("CAEN Task: read from HV TX CMD pipe incomplete.\n");
                        exit(0);
                    }


                    if (DEBUG) {
                        printf("CAEN Task: received Tx message .");
                        printf("crate %d channel %d cmd %d\n",
                               msg_buffer.crate,
                               msg_buffer.channel,
                               msg_buffer.command);
                    }

                    /* If not in idle state, Send the message */
                    if (!CAEN_IDLE) {
                        /* Send the message */
                        status = CAEN_send_raw_command (msg_buffer.crate,
                                                        msg_buffer.channel,
                                                        msg_buffer.command,
                                                        msg_buffer.value);
                        if (status != OK) {
                            printf("CAEN Task: Send Tx cmd fail: cr=%d ch=%d cmd=%d val=%d status=%d\n",
                                   msg_buffer.crate,
                                   msg_buffer.channel,
                                   msg_buffer.command,
                                   msg_buffer.value,
                                   status);
                            /*exit(0);*/
                        }
                    }

                    /* If the command was a write command, then set the LAM */
                    /* for the corresponding PARAM data set such that the   */
                    /* new value is cached ready for database read. */
                    if ( (msg_buffer.command>=CAEN_WRITE_V0_CMD) &&
                            (msg_buffer.command<=CAEN_RESET_TRIPS_CMD) ) {
                        /* get cache table index */
                        CAEN_get_cache_status_ptr((short)msg_buffer.crate,
                                                  (short)msg_buffer.channel,
                                                  &ptr,
                                                  &index,
                                                  &search);
                        if (search != CAEN_FOUND) {
                            printf("CAEN Task: Cache table search fail (%d,%d)\n",
                                   msg_buffer.crate,msg_buffer.channel);
                        } else {
                            /* Set param LAM */
                            /* Sometimes a single lam 1/60th second after cmd */
                            /* is not read (timeout). So issue two LAMS per cmd */
                            caen_table[index].param_LAM=caen_table[index].param_LAM+2;
                        }
                    }

                    /* recalc number of bytes in pipe. */
                    status = ioctl(HV_OFF_TX_PIPE_FD,FIONREAD,(int)&num_bytes);
                    if (status == ERROR) {
                        /* pipe error */
                        printf("CAEN Task: pipe error reading pipe %s.\n",
                               HV_OFF_TX_PIPE_NAME);
                        exit(0);
                    }

                    channel_num++;
                } /* end while Tx loop */

            } /* end of  Tx processing */
            else {
                /* No TX cmds of any kind */
                /* scan the entire cache table, starting from the last */
                /* LAM which was serviced, and service the next LAM found */
                chan_loop = last_LAM + 1;
                LAM_serviced = FALSE;
                all_done=FALSE;
                while ( (all_done!=TRUE) && (LAM_serviced != TRUE)) {
                    if (DEBUG2)printf("Asyn Task: LAMloop, %d ",chan_loop);
                    /* Check if LAM is raised. if this is a valid table entry*/
                    if (caen_table[chan_loop].crate >= 0) {
                        if ((caen_table[chan_loop].status_LAM>0)||
                                (caen_table[chan_loop].param_LAM>0)) {
                            /* chan has at least one LAM raised, service all LAM(s)*/
                            if (caen_table[chan_loop].status_LAM>0) {
                                status=CAEN_update_cache(caen_table[chan_loop].crate,
                                                         caen_table[chan_loop].channel,
                                                         CAEN_STATUS_DATA_TYPE);
                                if (status != OK) {
                                    printf("CAEN Task: Update Status Cache error.\n");
                                    /* exit(0); */
                                }
                                caen_table[chan_loop].status_LAM=
                                    caen_table[chan_loop].status_LAM-1;
                                if (DEBUG2)printf("CAEN. Serviced status LAM (%d,%d)\n",
                                                      caen_table[chan_loop].crate,
                                                      caen_table[chan_loop].channel);
                                /* cache updated for this channel data type */
                            }
                            if (caen_table[chan_loop].param_LAM>0) {
                                status=CAEN_update_cache(caen_table[chan_loop].crate,
                                                         caen_table[chan_loop].channel,
                                                         CAEN_PARAM_DATA_TYPE);
                                if (status != OK) {
                                    printf("CAEN Task: Update Param Cache error.\n");
                                    /* exit(0); */
                                }
                                caen_table[chan_loop].param_LAM=
                                    caen_table[chan_loop].param_LAM-1;
                                if (DEBUG2)printf("CAEN. Serviced param LAM (%d,%d)\n",
                                                      caen_table[chan_loop].crate,
                                                      caen_table[chan_loop].channel);
                                /* cache updated for this channel data type */
                            }
                            LAM_serviced=TRUE;
                            last_LAM=chan_loop;
                        }/* end of LAM servicing for this channel */
                    }
                    if (!LAM_serviced) {
                        if (chan_loop==last_LAM) {
                            all_done=TRUE;
                        } else {
                            if (caen_table[chan_loop].crate <0) {
                                chan_loop=0;
                            } else {
                                chan_loop++;
                                if (caen_table[chan_loop].crate<0) {
                                    /* found end of table, reset to start of table */
                                    chan_loop=0;
                                }
                            }
                        }
                    }
                } /* End of LAM while loop */

                if (DEBUG2) {
                    if (LAM_serviced)
                        printf("Asyn Task: LAMloopend: LAM serviced %d\n",LAM_serviced);
                }
                if (LAM_serviced == FALSE) {
                    /* No LAMS were raised, check next channel in queue */
                    /* for data updates. */
                    read_channel++;
                    if (caen_table[read_channel].crate<0) {
                        /* end of table */
		        read_channel=0;
			if(DEBUG3>0) {
			  int newtime=current_time();
			  loops++;
			  printf("%f\n",(newtime-lastlooptime)/1000.0);
			  lastlooptime=newtime;
			}
                    }
		    if(DEBUG3>1) {
		      printf("%d ",read_channel);
		    }

                    /* Check if status data is stale. */
                    if (caen_table[read_channel].status_time_stamp +
                            caen_table[read_channel].status_stale_out_period
                            < current_time()) {
                        if (DEBUG2)printf("Asyn Task: status update (%d,%d)\n",caen_table[read_channel].crate,
                                              caen_table[read_channel].channel);
                        /* Channel status needs an update */
                        status = CAEN_update_cache(caen_table[read_channel].crate,
                                                   caen_table[read_channel].channel,
                                                   CAEN_STATUS_DATA_TYPE);
                        if (status != OK) {
                            printf("CAEN Task: update status cache error. (%d,%d)\n",
                                   caen_table[read_channel].crate,
                                   caen_table[read_channel].channel);
                            /* exit(0); */
                        }
                    } else if (caen_table[read_channel].status_time_stamp
                               > current_time()) {
                        /* Correct for clock wrap around */
                        caen_table[read_channel].status_time_stamp = 0;
                    }


                    /* Check if param data is stale. */
                    if (caen_table[read_channel].param_time_stamp +
                            caen_table[read_channel].param_stale_out_period
                            < current_time()) {
                        if (DEBUG2)printf("Asyn Task: param update (%d,%d)\n",caen_table[read_channel].crate,
                                              caen_table[read_channel].channel);
                        /* Channel param needs an update */
                        status = CAEN_update_cache(caen_table[read_channel].crate,
                                                   caen_table[read_channel].channel,
                                                   CAEN_PARAM_DATA_TYPE);
                        if (status != OK) {
                            printf("CAEN Task: update param cache error (%d,%d).\n",
                                   caen_table[read_channel].crate,
                                   caen_table[read_channel].channel);
                            /* exit(0); */
                        }
                    } else if (caen_table[read_channel].param_time_stamp
                               > current_time()) {
                        /* Correct for clock wrap around */
                        caen_table[read_channel].param_time_stamp = 0;
                    }

                } /* end of channel read processing. */
            } /* End if Tx. */
        } /* End if HV OFF TX */
	/*        taskDelay(ASYN_DEV_LOOP_DELAY);*/
	epicsThreadSleep(sleep_time);
    } /* End inf loop */
}

/* Directly place the Caen driver support file drvdevCaen.c here, since
I (saw) find the device support/driver support separation artificial
and confusing */

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
/*      .00 022494 db   Init Release                                    */
/*_begin                                                                */
/************************************************************************/
/*                                                                      */
/*      Title:  CEBAF CAEN Driver Support Procedure File.               */
/*      File:   RF.c                                                    */
/*      Environment: UNIX, vxWorks                                      */
/*      Equipment: SUN, VME                                             */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*_end                                                                  */

/*
 *      Original Author: Dave Barker
 *      Current Author:  Dave Barker
 *      Date:            2-24-94
 *
 *      Copyright 1993. SURA CEBAF.
 *
 *      The Experimental Physics and Industrial Control System (EPICS)
 *      is Copyrighted 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *
 */

/*#include "Caen.h"*/

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_write_tx_buf                             */
/*                                                                      */
/*    Function : This procedure writes a 16 bit value to the CAEN       */
/*               transmission buffer.                                   */
/*                                                                      */
/*    Arguments: short  data     value to write.                        */
/*                                                                      */
/*    Returns  : The error status of the procedure.                     */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_write_tx_buf(short data)
{
#ifdef USE_VXMEMPROBE
    int status;
    /* write data to tx buffer */
    status = vxMemProbe((char *)CAEN_BASE_LADDR+CAEN_TX_BUF,
                        WRITE,
                        2,
                        (char *)&data);
#else
    /* write data to tx buffer */
    *(short *)(CAEN_BASE_LADDR+CAEN_TX_BUF)=data;
#endif
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_write_tx_reg                             */
/*                                                                      */
/*    Function : This procedure writes to the CAEN                      */
/*               transmission buffer. The causes the Tx buffer contents */
/*               to be sent to their destination.                       */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  : The error status of the procedure.                     */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_write_tx_reg()
{
#ifdef USE_VXMEMPROBE
    int status;
    short data;
    data = 0;
    status = vxMemProbe((char *)(CAEN_BASE_LADDR+CAEN_TX_REG), WRITE,
                        2, (char *)&data);
#else
    /* write zero to tx buffer */
    *(short *)(CAEN_BASE_LADDR+CAEN_TX_REG)=0;
#endif
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_reset                                    */
/*                                                                      */
/*    Function : This procedure resets the VME CAEN communications      */
/*               board. This ensures that the board is in the state     */
/*               ready for the next command.                            */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  : The error status of the procedure.                     */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_reset()
{
#ifdef USE_VXMEMPROBE
    int status;
    short data;
    data = 0;
    status = vxMemProbe((char *)(CAEN_BASE_LADDR+CAEN_RESET_REG), WRITE,
                        2, (char *)&data);
#else
    /* write zero to reset register. */
    *(short *)(CAEN_BASE_LADDR+CAEN_RESET_REG)=0;
#endif
    /* now spin for 100 micro seconds to allow board */
    /* to reset */
    CAEN_spin(1);
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_read_rx_buf                              */
/*                                                                      */
/*    Function : This procedure reads a 16 bit value from the CAEN      */
/*               receive buffer.                                        */
/*                                                                      */
/*    Arguments: short *data     pointer to data.                       */
/*                                                                      */
/*    Returns  : The error status of the procedure.                     */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_read_rx_buf(short *data)
{
#ifdef USE_VXMEMPROBE
    int status;
    status = vxMemProbe((char *)(CAEN_BASE_LADDR+CAEN_RX_BUF), READ,
                        2, (char *)data);
#else
    /* read the rx buffer. */
    *data=(short)*(short *)(CAEN_BASE_LADDR+CAEN_RX_BUF);
#endif
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_read_status_reg                          */
/*                                                                      */
/*    Function : This procedure reads a 16 bit value from the CAEN      */
/*               status register.                                       */
/*                                                                      */
/*    Arguments: short *data     pointer to data.                       */
/*                                                                      */
/*    Returns  : The error status of the procedure.                     */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_read_status_reg(short *data)
{
#ifdef USE_VXMEMPROBE
    int status;
    status = vxMemProbe((char *)(CAEN_BASE_LADDR+CAEN_STATUS_REG), READ,
                        2, (char *)data);
#else
    /* read the status register. */
    *data=(short)*(short *)(CAEN_BASE_LADDR+CAEN_STATUS_REG);
#endif
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_spin.                                    */
/*                                                                      */
/*    Function : This procedure delays the task by 100 micro seconds.   */
/*               This is necessary after a reset.                       */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  :                                                        */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
void CAEN_spin(int iterations)
{
    int i,j;

    /*    for (j=0; j<iterations; j++) {
        for (i=0; i<800; i++);
	}*/
    epicsThreadSleep(spin_time*iterations);
}

