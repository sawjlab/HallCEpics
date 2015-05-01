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
/*      Title:  CEBAF CAEN Driver and Device Support Header File  */
/*      File:   Caen.h                                                  */
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
 *      Copyright 1994. SURA CEBAF.
 *
 *      The Experimental Physics and Industrial Control System (EPICS)
 *      is Copyrighted 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *
 */

#ifndef DEBUG
#define DEBUG 0
#endif

#define debug(...) \
    do { if (DEBUG) fprintf(stderr, ##__VA_ARGS__); } while (0)

/* Procedure definitions */

/* devCaen.c */
extern int  CAEN_send_command();
extern int  CAEN_read_data();
extern void CAEN_get_cache_status_ptr();
extern void CAEN_get_cache_param_ptr();

/* devCaenInit.c */
/*extern int  CAEN_init();*/
extern int  CAEN_init_table();
extern int  CAEN_init_task();
extern int CAEN_init_channle(int crate, int channel);

/* Caen.c */
extern int CAEN_send_raw_command();
extern int CAEN_read_raw_data();
extern int CAEN_update_cache();
/*extern long current_time();*/
extern void CAEN_set_staleout();
extern void CAEN_dev_sup();

/* External variable declaration */
extern int HV_OFF_TX_PIPE_FD;
extern int HV_TX_PIPE_FD;
extern int CAEN_INITIALISED;
extern int CAEN_TABLE_INITIALISED;
extern int CAEN_DEBUG;
extern int CAEN_RECORD_DEBUG;
extern int CAEN_TX_TO;
extern int CAEN_RX_TO;
extern int CAEN_NUM_TX;
extern int CAEN_NUM_RX;

/* General defintions */
/* #define CAEN_RX_TIMEOUT     2000 */
#define CAEN_RX_TIMEOUT     4000
/* #define CAEN_TX_TIMEOUT     2000 */
#define CAEN_TX_TIMEOUT     4000
#define MAX_NUMBER_OF_HV_CHANNELS 1000
#define MAX_NUM_TX_CMD_PER_LOOP   1

/* Data block types which are read atomically */
#define  CAEN_STATUS_DATA_TYPE        0
#define  CAEN_PARAM_DATA_TYPE         1

/* Cache stale out periods */
#define  CAEN_STATUS_LONG_STALE_OUT_PERIOD   30000  /* 30 Sec */
#define  CAEN_STATUS_SHORT_STALE_OUT_PERIOD  1000   /* 1 Sec */
#define  CAEN_PARAM_LONG_STALE_OUT_PERIOD    30000  /* 30 Sec */
#define  CAEN_PARAM_SHORT_STALE_OUT_PERIOD   5000   /* 5 Sec */


/* CAEN Dev Sup Task definitions */
#define  CAEN_DEV_SUP_TASK_NAME    "CAEN"
#define  CAEN_DEV_SUP_TASK_PRI     200
#define  CAEN_DEV_SUP_TASK_OPT     0
#define  CAEN_DEV_SUP_TASK_SS      10000
#define  CAEN_DEV_SUP_TASK_ENTRY   CAEN_dev_sup

/* Tx Pipe names and parameters */
#define HV_OFF_TX_PIPE_NAME  "/pipe/hv_off_tx"
#define HV_TX_PIPE_NAME      "/pipe/hv_tx"
#define HV_OFF_PIPE_MAX_MSG  MAX_NUMBER_OF_HV_CHANNELS
#define HV_TX_PIPE_MAX_MSG   4*MAX_NUMBER_OF_HV_CHANNELS

/* Search result definitions */
#define CAEN_FOUND           1
#define CAEN_NOT_FOUND       0
#define CAEN_INVALID_DATA    2

/* Cache data invalid timeout */
#define CAEN_DATA_INV_PERIOD  60000

/* Number of times HV OFF is attempted until give up */
#define CAEN_HV_OFF_RETRIES   10

/* Number of times BUSY status will be read before giving up */
/*#define CAEN_SET_CMD_RETRIES 3*/
#define CAEN_SET_CMD_RETRIES 6

/* Config file definitions */
#define CAEN_CONFIG_FILE_NAME "CAEN.config"

/* VME Addresses */
/* Used by Device Support */
/*#define CAEN_BASE_ADDR   0xf0f00000*/
/*#define CAEN_BASE_ADDR   0xe0f00000*/
#define CAEN_BASE_ADDR   0xf00000
#define CAEN_TX_BUF      0x0
#define CAEN_RX_BUF      0x0
#define CAEN_STATUS_REG  0x02
#define CAEN_TX_REG      0x04
#define CAEN_RESET_REG   0x06
#define CAEN_INT_VEC_REG 0x08   /* not used */

/* CAEN Driver Support Defs */
#define CAEN_CONTROLLER_ID     0x0001
#define CAEN_HV_MASK           0x0800
#define CAEN_PASSWD_MASK       0x1000
#define CAEN_PDWN_MASK         0x2000
#define CAEN_ON_EN_MASK        0x4000
#define CAEN_PON_MASK          0x8000
#define CAEN_HV_ON_FLAG        0x0008
#define CAEN_HV_OFF_FLAG       0x0000
#define CAEN_PASSWD_REQ_FLAG   0x0010
#define CAEN_PASSWD_NREQ_FLAG  0x0000
#define CAEN_PDWN_RDWN_FLAG    0x0020
#define CAEN_PDWN_KILL_FLAG    0x0000
#define CAEN_ON_EN_FLAG        0x0040
#define CAEN_ON_DIS_FLAG       0x0000
#define CAEN_PON_ON_FLAG       0x0080
#define CAEN_PON_OFF_FLAG      0x0000

/* CAEN Status register values */
#define CAEN_STATUS_OK         0xfffe
#define CAEN_STATUS_ERROR      0xffff

/* CAEN Device Command types. */
/* Write CMDS */
#define CAEN_WRITE_V0_CMD        1
#define CAEN_WRITE_V1_CMD        2
#define CAEN_WRITE_I0_CMD        3
#define CAEN_WRITE_I1_CMD        4
#define CAEN_WRITE_VMAX_CMD      5
#define CAEN_WRITE_RUP_CMD       6
#define CAEN_WRITE_RDWN_CMD      7
#define CAEN_WRITE_TRIP_CMD      8
#define CAEN_WRITE_HV_CMD        9
#define CAEN_WRITE_PASSWD_CMD    10
#define CAEN_WRITE_PDWN_CMD      11
#define CAEN_WRITE_ON_EN_CMD     12
#define CAEN_WRITE_PON_CMD       13
#define CAEN_WRITE_HV_OFF_CMD    14
#define CAEN_RESET_TRIPS_CMD     15

/* Read CMDS */
#define CAEN_READ_MOD_ID_CMD     20
#define CAEN_READ_V_MON_CMD      21
#define CAEN_READ_I_MON_CMD      22
#define CAEN_READ_STATUS_CMD     23
#define CAEN_READ_NAME_CMD       24
#define CAEN_READ_V0_CMD         25
#define CAEN_READ_V1_CMD         26
#define CAEN_READ_I0_CMD         27
#define CAEN_READ_I1_CMD         28
#define CAEN_READ_VMAX_CMD       29
#define CAEN_READ_RUP_CMD        30
#define CAEN_READ_RDWN_CMD       31
#define CAEN_READ_TRIP_CMD       32
#define CAEN_READ_HV_CMD         33
#define CAEN_READ_PASSWD_CMD     34
#define CAEN_READ_PDWN_CMD       35
#define CAEN_READ_ON_EN_CMD      36
#define CAEN_READ_PON_CMD        37
#define CAEN_READ_VMAX_B0_CMD    38
#define CAEN_READ_VMAX_B1_CMD    39
#define CAEN_READ_VMAX_B2_CMD    40
#define CAEN_READ_VMAX_B3_CMD    41
#define CAEN_READ_IMAX_B0_CMD    42
#define CAEN_READ_IMAX_B1_CMD    43
#define CAEN_READ_IMAX_B2_CMD    44
#define CAEN_READ_IMAX_B3_CMD    45
#define CAEN_READ_VRES_B0_CMD    46
#define CAEN_READ_VRES_B1_CMD    47
#define CAEN_READ_VRES_B2_CMD    48
#define CAEN_READ_VRES_B3_CMD    49
#define CAEN_READ_IRES_B0_CMD    50
#define CAEN_READ_IRES_B1_CMD    51
#define CAEN_READ_IRES_B2_CMD    52
#define CAEN_READ_IRES_B3_CMD    53
#define CAEN_READ_VSF_B0_CMD     54
#define CAEN_READ_VSF_B1_CMD     55
#define CAEN_READ_VSF_B2_CMD     56
#define CAEN_READ_VSF_B3_CMD     57
#define CAEN_READ_ISF_B0_CMD     58
#define CAEN_READ_ISF_B1_CMD     59
#define CAEN_READ_ISF_B2_CMD     60
#define CAEN_READ_ISF_B3_CMD     61
#define CAEN_READ_BSY_CMD        62


/* CAEN CAENnet Command types */
#define CAEN_READ_MOD_ID      0x0
#define CAEN_READ_CH_STATUS   0x01      /* Requires Channel ID */
#define CAEN_READ_CH_PARAMS   0x02      /* Requires Channel ID */
#define CAEN_READ_BRD_CHARS   0x03      /* Requires Channel ID */
#define CAEN_READ_BSY_STATUS  0x00ff
#define CAEN_WRITE_V0_SET_VAL  0x10       /* Requires Channel ID */
#define CAEN_WRITE_V1_SET_VAL  0x11       /* Requires Channel ID */
#define CAEN_WRITE_I0_SET_VAL  0x12       /* Requires Channel ID */
#define CAEN_WRITE_I1_SET_VAL  0x13       /* Requires Channel ID */
#define CAEN_WRITE_VMAX_SET_VAL 0x14      /* Requires Channel ID */
#define CAEN_WRITE_RUP_SET_VAL  0x15      /* Requires Channel ID */
#define CAEN_WRITE_RDWN_SET_VAL 0x16      /* Requires Channel ID */
#define CAEN_WRITE_TRIP_SET_VAL 0x17      /* Requires Channel ID */
#define CAEN_WRITE_FLAG_SET_VAL 0x18      /* Requires Channel ID */
#define CAEN_RESET_TRIPS        0x32
/* CAEN VME Module ID String */
/* #define CAEN_MODULE_ID_STRING   "SY403 V1.44" */
#define CAEN_MODULE_ID_STRING   "SY403 V1.45"

/* CAEN Read Error Codes */
#define CAEN_OK                      0x0000
#define CAEN_CHANNEL_NONEXISTENT     (unsigned short) 0xffff
#define CAEN_MODULE_BUSY             (unsigned short) 0xff00
#define CAEN_CODE_INVALID            (unsigned short) 0xff01
#define CAEN_VALUE_OUT_OF_RANGE      (unsigned short) 0xff02
#define CAEN_CHANNEL_NOT_PRESENT     (unsigned short) 0xff03
#define CAEN_NO_DATA                 (unsigned short) 0xfffd
#define CAEN_CONTROLLER_INVALID      (unsigned short) 0xfffe

/* general error status */
#define CAEN_ERROR                   1
#define CAEN_TIMEOUT_ERROR           2

/* Expected number of words each read command should return */
#define CAEN_MOD_ID_SIZE              11
#define CAEN_CH_STATUS_SIZE            4
#define CAEN_CH_PARMS_SIZE            17
#define CAEN_BRD_DATA_SIZE            24
#define CAEN_BSY_STATUS_SIZE           1

/* Definitions of the starting position of read data in */
/* the receive data buffer. (Starting at zero) */
#define CAEN_MOD_ID_POS           0
#define CAEN_READ_V_MON_POS       0  /* MSW */
#define CAEN_READ_I_MON_POS       2
#define CAEN_READ_STATUS_POS      3
#define CAEN_READ_NAME_POS        0
#define CAEN_READ_V0_POS          6  /* MSW */
#define CAEN_READ_V1_POS          8  /* MSW */
#define CAEN_READ_I0_POS         10
#define CAEN_READ_I1_POS         11
#define CAEN_READ_VMAX_POS       12
#define CAEN_READ_RUP_POS        13
#define CAEN_READ_RDWN_POS       14
#define CAEN_READ_TRIP_POS       15
#define CAEN_READ_HV_POS         16 /* bit of word */
#define CAEN_READ_PASSWD_POS     16 /* bit of word */
#define CAEN_READ_PDWN_POS       16 /* bit of word */
#define CAEN_READ_ON_EN_POS      16 /* bit of word */
#define CAEN_READ_PON_POS        16 /* bit of word */
#define CAEN_READ_VMAX_B0_POS     0
#define CAEN_READ_VMAX_B1_POS     1
#define CAEN_READ_VMAX_B2_POS     2
#define CAEN_READ_VMAX_B3_POS     3
#define CAEN_READ_IMAX_B0_POS     4
#define CAEN_READ_IMAX_B1_POS     5
#define CAEN_READ_IMAX_B2_POS     6
#define CAEN_READ_IMAX_B3_POS     7
#define CAEN_READ_VRES_B0_POS     8
#define CAEN_READ_VRES_B1_POS     9
#define CAEN_READ_VRES_B2_POS    10
#define CAEN_READ_VRES_B3_POS    11
#define CAEN_READ_IRES_B0_POS    12
#define CAEN_READ_IRES_B1_POS    13
#define CAEN_READ_IRES_B2_POS    14
#define CAEN_READ_IRES_B3_POS    15
#define CAEN_READ_VSF_B0_POS     16
#define CAEN_READ_VSF_B1_POS     17
#define CAEN_READ_VSF_B2_POS     18
#define CAEN_READ_VSF_B3_POS     19
#define CAEN_READ_ISF_B0_POS     20
#define CAEN_READ_ISF_B1_POS     21
#define CAEN_READ_ISF_B2_POS     22
#define CAEN_READ_ISF_B3_POS     23
#define CAEN_READ_BSY_POS         0


/* Message structure between Dev Sup and Asyn task */
typedef struct   caen_tx_msg_type {
    short crate;
    short channel;
    short command;
    short value;
} CAEN_TX_MSG;

typedef struct  caen_table_type {
    short crate;
    short channel;
    short group;
    struct status_data_type *status_ptr;
    unsigned long   status_time_stamp;
    unsigned long   status_stale_out_period;
    int    status_LAM;
    struct param_data_type *param_ptr;
    unsigned long   param_time_stamp;
    unsigned long   param_stale_out_period;
    int    param_LAM;
} CAEN_TAB;

/* Cache Data table structure */
typedef struct  status_data_type {
    unsigned int    v_mon;
    unsigned short  i_mon;
    unsigned short  status;
} CAEN_STATUS_DATA;

/* Cache Data table structure */
typedef struct  param_data_type {
    char            name[12];
    unsigned int    v0_set;
    unsigned int    v1_set;
    unsigned short  i0_set;
    unsigned short  i1_set;
    unsigned short  v_max;
    unsigned short  ramp_up;
    unsigned short  ramp_down;
    unsigned short  trip;
    unsigned short  hv;
    unsigned short  passwd;
    unsigned short  power_down;
    unsigned short  on_enable;
    unsigned short  pon;
} CAEN_PARAM_DATA;

extern struct caen_table_type caen_table[];

