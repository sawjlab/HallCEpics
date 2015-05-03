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
/*      Title:  CEBAF CAEN Device Support Initialisation.               */
/*      File:   drvCaenInit.c                                           */
/*      Environment: UNIX, vxWorks                                      */
/*      Equipment: SUN, VME                                             */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*_end                                                                  */



#include <vxWorks.h>
#include <stdioLib.h>
#include <fioLib.h>
#include <ioLib.h>
#include <taskLib.h>
#include <string.h>
#include <taskwd.h>
#include <pipeDrv.h>

#include <Caen.h>

#include <time.h>

#define SPAWN_ASYN_TASK  1

extern int CAEN_DEBUG;

int read_line();

void CAEN_dev_sup_respawn(epicsThreadId tid)
{
    epicsThreadId result;

    taskwdRemove(tid);
    taskDelete((int)tid);
    /* Spawn the asychronous task */
    result = taskSpawn(CAEN_DEV_SUP_TASK_NAME,
                       CAEN_DEV_SUP_TASK_PRI,
                       CAEN_DEV_SUP_TASK_OPT,
                       CAEN_DEV_SUP_TASK_SS,
                       (FUNCPTR)CAEN_DEV_SUP_TASK_ENTRY,0,1,2,3,4,5,6,7,8,9);
    if((int)result != ERROR) {
        taskwdInsert(result, (VOIDFUNCPTR) CAEN_dev_sup_respawn, (void *)result);
    } else {
        printf("CAEN Dev Sup: Error in spawning task %s\n",
               CAEN_DEV_SUP_TASK_NAME);
    }
    printf("Spawned %s\n",CAEN_DEV_SUP_TASK_NAME);
}

/************************************************************************/
/*                                                                      */
/*    External Procedure: CAEN_init.                                    */
/*                                                                      */
/*    Function : This procedure initialises the CAEN device support.    */
/*               First the Cache is created by reading the file         */
/*               CAEN.config. The Tx pipes are created, and finally,    */
/*               the asynchronous task is spawned which processes the   */
/*               asyn device support.                                   */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  : the error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
int CAEN_init_table()
{
  /* This is the initialisation procedure for the CAEN Device */
  /* Support. It initializes a data cache of HV data.         */
  /* The data cache will be filled in as records are initialized */
  /* Note, this procedure should be called once, and once only. */


  if(CAEN_TABLE_INITIALISED==TRUE) {
    /* already initialised */
    if(DEBUG)printf("drvCaenInit: abort init, already done.\n");
    return(CAEN_OK);
  }

  /* Init the table */
  /* Should probably do this with a linked list */
  caen_table[0].crate = -1;
  CAEN_TABLE_INITIALISED=TRUE;
  return(CAEN_OK);
}
int CAEN_init_channel(int crate, int channel)
{
  int tindex=0;
  int channel_found=0;
  struct status_data_type *status_data_ptr;
  struct param_data_type  *param_data_ptr;

  for(tindex=0;tindex<MAX_NUMBER_OF_HV_CHANNELS;tindex++) {
    if(caen_table[tindex].crate < 0) {
      break;
    }
    if((caen_table[tindex].crate == crate) &&
       (caen_table[tindex].channel == channel)) {
      channel_found == 1;
      break;
    }
  }
  if(!channel_found) {
    if(MAX_NUMBER_OF_HV_CHANNELS-tindex>=2) {
      caen_table[tindex].crate = crate;
      caen_table[tindex].channel = channel;
      caen_table[tindex].group = 0;

      /* allocate memory for status data */
      status_data_ptr =
	(struct status_data_type *)malloc(sizeof(CAEN_STATUS_DATA));
      if(status_data_ptr == NULL) {
	/* Error: could not malloc */
	printf("CAEN Dev Sup: Error in malloc\n");
	/* do not try again */
	CAEN_INITIALISED=TRUE;
	return(CAEN_ERROR);
      }

      /* zero the data */
      status_data_ptr->v_mon  = 0;
      status_data_ptr->i_mon  = 0;
      status_data_ptr->status = 0;

      /* attache cache data to the table */
      caen_table[tindex].status_ptr=
	(struct status_data_type *)status_data_ptr;

      /* zero the time stamp and set LAM */
      caen_table[tindex].status_time_stamp=0;
      caen_table[tindex].status_stale_out_period=
	CAEN_STATUS_LONG_STALE_OUT_PERIOD;
      caen_table[tindex].status_LAM=5;

      /* allocate memory for param data */
      param_data_ptr =
	(struct param_data_type *)malloc(sizeof(CAEN_PARAM_DATA));
      if(param_data_ptr == NULL) {
	/* Error: could not malloc */
	printf("CAEN Dev Sup: Error in malloc\n");
	/* do not try again */
	CAEN_INITIALISED=TRUE;
	return(CAEN_ERROR);
      }

      /* zero the data */
      param_data_ptr->name[0]= '\0';
      param_data_ptr->v0_set = 0;
      param_data_ptr->v1_set = 0;
      param_data_ptr->i0_set = 0;
      param_data_ptr->i1_set = 0;
      param_data_ptr->v_max  = 0;
      param_data_ptr->ramp_up = 0;
      param_data_ptr->ramp_down = 0;
      param_data_ptr->trip   = 0;
      param_data_ptr->hv     = 0;
      param_data_ptr->passwd = 0;
      param_data_ptr->power_down = 0;
      param_data_ptr->on_enable = 0;
      param_data_ptr->pon = 0;

      /* attache cache data to the table */
      caen_table[tindex].param_ptr=
	(struct param_data_type *)param_data_ptr;

      /* zero the time stamp and LAM */
      caen_table[tindex].param_time_stamp=0;
      caen_table[tindex].param_stale_out_period=
	CAEN_PARAM_LONG_STALE_OUT_PERIOD;
      caen_table[tindex].param_LAM=5;

      caen_table[tindex+1].crate = -1;
    } else {
      printf("CAEN Dev Sup: Maximum number of channels, %d, exceeded\n",
	     MAX_NUMBER_OF_HV_CHANNELS-1);
      return(CAEN_ERROR);
    }
  }
  return(CAEN_OK);
}
int CAEN_init_task()
{
  int result;
  int fd;

  if(CAEN_INITIALISED==TRUE) {
    /* already initialised */
    if(DEBUG)printf("drvCaenInit: abort init, already done.\n");
    return(CAEN_OK);
  }

  /* Create the two pipes which send the transmit messages */
  /* to the asynchronous task. */
  /* HV OFF message Tx pipe */
  result = pipeDevCreate(HV_OFF_TX_PIPE_NAME,
			 HV_OFF_PIPE_MAX_MSG,
			 sizeof(CAEN_TX_MSG));
  if(result != OK) {	
    /* error, abort initialisation */
    printf("CAEN Dev Sup: Error in creating pipe %s\n",
	   HV_OFF_TX_PIPE_NAME);
    CAEN_INITIALISED=TRUE;
    return(CAEN_ERROR);
  }

  /* HV other message Tx pipe */
  result = pipeDevCreate(HV_TX_PIPE_NAME,
			 HV_TX_PIPE_MAX_MSG,
			 sizeof(CAEN_TX_MSG));
  if(result != OK) {
    /* error, abort initialisation */
    printf("CAEN Dev Sup: Error in creating pipe %s\n",
	   HV_TX_PIPE_NAME);
    CAEN_INITIALISED=TRUE;
    return(CAEN_ERROR);
  }

  /* Open the pipes for read/write */
  fd = open(HV_OFF_TX_PIPE_NAME,O_RDWR,0);
  if (fd==ERROR) {
    /* error, abort initialisation */
    printf("CAEN Dev Sup: Error in opening pipe %s\n",
	   HV_OFF_TX_PIPE_NAME);
    CAEN_INITIALISED=TRUE;
    return(CAEN_ERROR);
  }
  HV_OFF_TX_PIPE_FD = fd;

  fd = open(HV_TX_PIPE_NAME,O_RDWR,0);
  if (fd==ERROR) {
    /* error, abort initialisation */
    printf("CAEN Dev Sup: Error in opening pipe %s\n",
	   HV_TX_PIPE_NAME);
    CAEN_INITIALISED=TRUE;
    return(CAEN_ERROR);
  }
  HV_TX_PIPE_FD = fd;

  CAEN_INITIALISED=TRUE;

  if(SPAWN_ASYN_TASK) {
        /* Spawn the asychronous task */
        debug("drvCaenInit: Spawning task\n");
        result = taskSpawn(CAEN_DEV_SUP_TASK_NAME,
                           CAEN_DEV_SUP_TASK_PRI,
                           CAEN_DEV_SUP_TASK_OPT,
                           CAEN_DEV_SUP_TASK_SS,
                           (FUNCPTR)CAEN_DEV_SUP_TASK_ENTRY,0,1,2,3,4,5,6,7,8,9);
        if(result == ERROR) {
            printf("CAEN Dev Sup: Error in spawning task %s\n",
                   CAEN_DEV_SUP_TASK_NAME);
            CAEN_INITIALISED=TRUE;
            return(CAEN_ERROR);
        }
    } else {
        taskwdInsert((epicsThreadId)result, (VOIDFUNCPTR)CAEN_dev_sup_respawn, (void *)result);
    }

    CAEN_INITIALISED=TRUE;
    return(CAEN_OK);
}
#if 0
int CAEN_init_old()
{
    struct status_data_type *status_data_ptr;
    struct param_data_type  *param_data_ptr;
    int result;
    int table_index;
    int fd;
    FILE *config;
    int crate;
    int channel;
    int group;
    char label[32];
    char line[156];

    /* This is the initialisation procedure for the CAEN Device */
    /* Support. It creates a data cache of HV data based on     */
    /* a configuration file.                                    */
    /* Note, this procedure should be called once, and once only. */

    if(CAEN_INITIALISED==TRUE) {
        /* already initialised */
        if(DEBUG)printf("drvCaenInit: abort init, already done.\n");
        return(CAEN_OK);
    }

    /* Init the table */
    table_index=0;
    caen_table[table_index].crate = -1;

    /* Read the config file */

    /* USING STREAMS */
    config = fopen(CAEN_CONFIG_FILE_NAME,"r");
    if (config == (FILE *)NULL) {
        printf("CAEN Dev Sup: Error in opening file %s\n",
               CAEN_CONFIG_FILE_NAME);
        CAEN_INITIALISED=TRUE;
        fclose(config);
        return(CAEN_ERROR);
    }
    if(DEBUG)printf("drvCaenInit: config file opened.\n");

    /* Scan the file, for each entry create an entry in the */
    /* cache table, and allocate its memory. */

    /* Lines with # as their first character are commented lines. */

    result=0;
    while(result!=EOF) {
        result = read_line(config,line);
        if(result != EOF) {
            if ( strchr(line,'#') ==0 ) {
                /* not a commented line, decode label, crate, channel & group*/
                sscanf(line,"%s %d %d %d",label,&crate,&channel,&group);
                if(DEBUG)printf("drvCaenInit: file entry %s %d %d %d\n",
                                    label,crate,channel,group);
                /* read data, create entry in caen table */
                caen_table[table_index].crate = (short)crate;
                caen_table[table_index].channel = (short)channel;
                caen_table[table_index].group   = (short)group;

                /* allocate memory for status data */
                status_data_ptr =
                    (struct status_data_type *)malloc(sizeof(CAEN_STATUS_DATA));
                if(status_data_ptr == NULL) {
                    /* Error: could not malloc */
                    printf("CAEN Dev Sup: Error in malloc\n");
                    /* do not try again */
                    CAEN_INITIALISED=TRUE;
                    return(CAEN_ERROR);
                }

                /* zero the data */
                status_data_ptr->v_mon  = 0;
                status_data_ptr->i_mon  = 0;
                status_data_ptr->status = 0;

                /* attache cache data to the table */
                caen_table[table_index].status_ptr=
                    (struct status_data_type *)status_data_ptr;

                /* zero the time stamp and set LAM */
                caen_table[table_index].status_time_stamp=0;
                caen_table[table_index].status_stale_out_period=
                    CAEN_STATUS_LONG_STALE_OUT_PERIOD;
                caen_table[table_index].status_LAM=5;

                /* allocate memory for param data */
                param_data_ptr =
                    (struct param_data_type *)malloc(sizeof(CAEN_PARAM_DATA));
                if(param_data_ptr == NULL) {
                    /* Error: could not malloc */
                    printf("CAEN Dev Sup: Error in malloc\n");
                    /* do not try again */
                    CAEN_INITIALISED=TRUE;
                    return(CAEN_ERROR);
                }

                /* zero the data */
                param_data_ptr->name[0]= '\0';
                param_data_ptr->v0_set = 0;
                param_data_ptr->v1_set = 0;
                param_data_ptr->i0_set = 0;
                param_data_ptr->i1_set = 0;
                param_data_ptr->v_max  = 0;
                param_data_ptr->ramp_up = 0;
                param_data_ptr->ramp_down = 0;
                param_data_ptr->trip   = 0;
                param_data_ptr->hv     = 0;
                param_data_ptr->passwd = 0;
                param_data_ptr->power_down = 0;
                param_data_ptr->on_enable = 0;
                param_data_ptr->pon = 0;

                /* attache cache data to the table */
                caen_table[table_index].param_ptr=
                    (struct param_data_type *)param_data_ptr;

                /* zero the time stamp and LAM */
                caen_table[table_index].param_time_stamp=0;
                caen_table[table_index].param_stale_out_period=
                    CAEN_PARAM_LONG_STALE_OUT_PERIOD;
                caen_table[table_index].param_LAM=5;


                /* increment table pointer, init next entry to null */
                table_index++;
                caen_table[table_index].crate=-1;

            } /* end if read ok */
            else {
                /* line is commented */
                printf("drvCaenInit: %s\n",line);
            }
        }
    }   /* end while */

    fclose (config);
    /* end of cache table creation. */


    /* Create the two pipes which send the transmit messages */
    /* to the asynchronous task. */
    /* HV OFF message Tx pipe */
    result = pipeDevCreate(HV_OFF_TX_PIPE_NAME,
                           HV_OFF_PIPE_MAX_MSG,
                           sizeof(CAEN_TX_MSG));
    if(result != OK) {
        /* error, abort initialisation */
        printf("CAEN Dev Sup: Error in creating pipe %s\n",
               HV_OFF_TX_PIPE_NAME);
        CAEN_INITIALISED=TRUE;
        return(CAEN_ERROR);
    }

    /* HV other message Tx pipe */
    result = pipeDevCreate(HV_TX_PIPE_NAME,
                           HV_TX_PIPE_MAX_MSG,
                           sizeof(CAEN_TX_MSG));
    if(result != OK) {
        /* error, abort initialisation */
        printf("CAEN Dev Sup: Error in creating pipe %s\n",
               HV_TX_PIPE_NAME);
        CAEN_INITIALISED=TRUE;
        return(CAEN_ERROR);
    }

    /* Open the pipes for read/write */
    fd = open(HV_OFF_TX_PIPE_NAME,O_RDWR,0);
    if (fd==ERROR) {
        /* error, abort initialisation */
        printf("CAEN Dev Sup: Error in opening pipe %s\n",
               HV_OFF_TX_PIPE_NAME);
        CAEN_INITIALISED=TRUE;
        return(CAEN_ERROR);
    }
    HV_OFF_TX_PIPE_FD = fd;

    fd = open(HV_TX_PIPE_NAME,O_RDWR,0);
    if (fd==ERROR) {
        /* error, abort initialisation */
        printf("CAEN Dev Sup: Error in opening pipe %s\n",
               HV_TX_PIPE_NAME);
        CAEN_INITIALISED=TRUE;
        return(CAEN_ERROR);
    }
    HV_TX_PIPE_FD = fd;

    if(SPAWN_ASYN_TASK) {
        /* Spawn the asychronous task */
        debug("drvCaenInit: Spawning task\n");
        result = taskSpawn(CAEN_DEV_SUP_TASK_NAME,
                           CAEN_DEV_SUP_TASK_PRI,
                           CAEN_DEV_SUP_TASK_OPT,
                           CAEN_DEV_SUP_TASK_SS,
                           (FUNCPTR)CAEN_DEV_SUP_TASK_ENTRY,0,1,2,3,4,5,6,7,8,9);
        if(result == ERROR) {
            printf("CAEN Dev Sup: Error in spawning task %s\n",
                   CAEN_DEV_SUP_TASK_NAME);
            CAEN_INITIALISED=TRUE;
            return(CAEN_ERROR);
        }
    } else {
        taskwdInsert((epicsThreadId)result, (VOIDFUNCPTR)CAEN_dev_sup_respawn, (void *)result);
    }

    CAEN_INITIALISED=TRUE;
    return(CAEN_OK);
}

#endif



/************************************************************************/
/*                                                                      */
/*    Internal Procedure: read_line                                     */
/*                                                                      */
/*    Function : This procedure reads the next line from the HV         */
/*               configuration file. If the end of file is detected,    */
/*               an EOF is returned. Otherwise zero is returned.        */
/*                                                                      */
/*    Arguments:                                                        */
/*                                                                      */
/*    Returns  : the error status.                                      */
/*                                                                      */
/*    Copyright, 1994, SURA CEBAF.                                      */
/*                                                                      */
/************************************************************************/
int read_line(fp,line)
FILE *fp;
char *line;
{
    char c;
    int  rd;
    int index=0;

    line[index]='\0';

    rd=getc(fp);
    c=(char)rd;
    while( (rd != EOF)&&(c != '\n')) {
        line[index]=(char)c;
        index++;
        rd=getc(fp);
        c=(char)rd;
    }
    line[index]='\0';

    if(c==(char)EOF) {
        return (EOF);
    } else {
        return (0);
    }
}
