/*******************************************************************************
recScaler.c
Record-support routines for Joerger 8-channel, 32-bit scaler

Original Author: Tim Mooney
Date: 1/16/95

Experimental Physics and Industrial Control System (EPICS)

Copyright 1995, the University of Chicago Board of Governors.

This software was produced under U.S. Government contract
W-31-109-ENG-38 at Argonne National Laboratory.

Initial development by:
	The X-ray Optics Group
	Experimental Facilities Division
	Advanced Photon Source
	Argonne National Laboratory

Modification Log:
-----------------
.01  6/26/93	tmm     Lecroy-scaler record
.02  1/16/95    tmm     Joerger-scaler
.03  8/28/95    tmm     Added .vers (code version) and .card (VME-card number)
                        fields 
.04  2/8/96     tmm     v1.7:  Fixed bug: was posting CNT field several times
                        when done.
.05  2/21/96    tmm     v1.71:  precision of vers field is 2
.06  6/5/96     tmm     v1.8: precision defaults to PREC field
*******************************************************************************/
#define VERSION 1.8

#include	<vxWorks.h>
#include	<types.h>
#include	<stdioLib.h>
#include	<lstLib.h>
#include	<string.h>
#include	<wdLib.h>

#include	<alarm.h>
#include	<callback.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<dbFldTypes.h>
#include	<errMdef.h>
#include	<recSup.h>
#include	<devSup.h>
#include	<special.h>
#define GEN_SIZE_OFFSET
#include	<scalerRecord.h>
#undef GEN_SIZE_OFFSET
#include	<choiceScaler.h>
#include	"devScaler.h"

#ifdef NODEBUG
#define Debug(l,FMT,V) ;
#else
#define Debug(l,FMT,V) {  if(l <= recScalerdebug) \
			{ printf("%s(%d):",__FILE__,__LINE__); \
			  printf(FMT,V); } }
#endif
volatile int recScalerdebug = 0;

#define DN2UP 1
#define MIN(a,b) (a)<(b)?(a):(b)
#define MAX(a,b) (a)>(b)?(a):(b)


/* Create RSET - Record Support Entry Table*/
#define report NULL
#define initialize NULL
static long init_record();
static long process();
static long special();
#define get_value NULL
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
#define get_units NULL
static long get_precision();
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
#define get_graphic_double NULL
#define get_control_double NULL
#define get_alarm_double NULL

struct rset scalerRSET = {
	RSETNUMBER,
	report,
	initialize,
	init_record,
	process,
	special,
	get_value,
	cvt_dbaddr,
	get_array_info,
	put_array_info,
	get_units,
	get_precision,
	get_enum_str,
	get_enum_strs,
	put_enum_str,
	get_graphic_double,
	get_control_double,
	get_alarm_double
};

static void alarm();
static void monitor();
static void updateCounts(struct scalerRecord *pscal);

/*** callback stuff ***/
struct callback {
	CALLBACK	callback;
	struct dbCommon *precord;
	WDOG_ID wd_id;
};

static void myCallback(struct callback *pcallback)
{
    struct dbCommon *precord=pcallback->precord;

    dbScanLock(precord);
    updateCounts((struct scalerRecord *)precord);
    dbScanUnlock(precord);
}

static void myCallback1(struct callback *pcallback)
{
    struct scalerRecord *pscal = (struct scalerRecord *)pcallback->precord;

	if (pscal->scan) scanOnce((void *)pscal);
}


static long init_record(pscal,pass)
struct scalerRecord *pscal;
int pass;
{
  /*	int i,j;*/
	long status;
	static card=0;
	SCALERDSET *pdset = (SCALERDSET *)(pscal->dset);
	struct callback *pcallback, *pcallback1;

	Debug(5, "init_record: pass = %d\n", pass);
	if (pass == 0) {
		pscal->vers = VERSION;
		return (0);
	}

	/* Gotta have a .val field.  Make its value reproducible. */
	pscal->val = 0;

	/*** setup callback stuff (note: array of 2 callback structures) ***/
	/* first callback to implement periodic updates */
	pcallback = (struct callback *)(calloc(2,sizeof(struct callback)));
	pscal->dpvt = (void *)pcallback;
	callbackSetCallback(myCallback,&pcallback->callback);
	pcallback->precord = (struct dbCommon *)pscal;
	pcallback->wd_id = wdCreate();
	/* second callback to implement delay */
	pcallback1 = (struct callback *)&(pcallback[1]);
	callbackSetCallback(myCallback1,&pcallback1->callback);
	pcallback1->precord = (struct dbCommon *)pscal;
	pcallback1->wd_id = wdCreate();

	/* Check that we have everything we need. */
	if (!(pdset = (SCALERDSET *)(pscal->dset)))
	{
		recGblRecordError(S_dev_noDSET,(void *)pscal, "scaler: init_record");
		return(S_dev_noDSET);
	}

	Debug(2, "init_record: calling dset->init_record\n", 0);
	pscal->out.value.vmeio.card = card++; /* SAW Hack */
	printf("Card = %d\n",pscal->out.value.vmeio.card);
	if (pdset->init_record)
	{
		status=(*pdset->init_record)(pscal);
		Debug(3, "init_record: dset->init_record returns %d\n", status);
		if (status) {
			pscal->card = -1;
			return (status);
		}
		pscal->card = pscal->out.value.vmeio.card;
	}

	/* convert between time and clock ticks */
	if (pscal->tp) {
		/* convert time to clock ticks */
		pscal->pr1 = (long) (pscal->tp * pscal->freq);
	} else if (pscal->pr1 && pscal->freq) {
		/* convert clock ticks to time */
		pscal->tp = (double)(pscal->pr1 / pscal->freq);
	}
	return(0);
}


static long process(pscal)
struct scalerRecord *pscal;
{
	int i, card, preset_value, keepon;
	long *pscaler = &(pscal->s1);
	long *ppreset = &(pscal->pr1);
	volatile long *pdata = NULL;
	short *pdir = &pscal->d1;
	short *pgate = &pscal->g1;
	SCALERDSET *pdset = (SCALERDSET *)(pscal->dset);

	Debug(5, "process: entry\n", 0);
	pscal->pact = TRUE;
	pscal->udf = FALSE;

	card = pscal->out.value.vmeio.card;

	/* If we're being called as a result of a done-counting interrupt, */
	/* (*pdset->done)(card) will return TRUE */
	if ((*pdset->done)(card)) pscal->cnt = 0;

	if (pscal->cnt != pscal->pcnt)
	{
		if (pscal->cnt)
		{
			/*** start counting ***/
			/* disarm, disable interrupt generation, reset disarm-on-cout, */
			/* clear mask register, clear direction register, clear counters */
			(*pdset->reset)(card);
			for (i=0; i<pscal->nch; i++) {
				pdir[i] = pgate[i];
				if (pgate[i]) {
					Debug(5, "process: writing preset: %d.\n", ppreset[i]);
					(*pdset->write_preset)(card, i, ppreset[i]);
				}
			}
			(*pdset->arm)(card, 1);
		}
		else
		{
			/* stop counting */
			(*pdset->arm)(card, 0);
		}
		pscal->pcnt = pscal->cnt;
		Debug(2, "process: posting done flag (%d)\n", pscal->cnt);
		db_post_events(pscal,&(pscal->cnt),DBE_VALUE);
	}

	/* read and display scalers */
	updateCounts(pscal);

	/* done counting? */
	if (!pscal->cnt)
	{
		recGblGetTimeStamp(pscal);
		alarm(pscal);
		monitor(pscal);
		recGblFwdLink(pscal);
	}

	pscal->pact = FALSE;
	return(0);
}


static void updateCounts(struct scalerRecord *pscal)
{
	int i, keepon, called_by_process;
	int card = pscal->out.value.vmeio.card;
	long *pscaler = &(pscal->s1);
	long *ppreset = &(pscal->pr1);
	/*	short *pgate = &(pscal->g1);*/
	volatile long *pdata = NULL;
	short *pdir = &pscal->d1;
	long counts[MAX_SCALER_CHANNELS];
	SCALERDSET *pdset = (SCALERDSET *)(pscal->dset);
	struct callback *pcallback = (struct callback *)pscal->dpvt;

	called_by_process = (pscal->pact == TRUE);
	if (!called_by_process) {
		if (pscal->cnt)
			pscal->pact = TRUE;
		else
			return;
	}

	/* read scalers (get pointer to actual VME-resident scaler-data array) */
	(*pdset->read)(card, &pdata);

	Debug(4, "updateCounts: posting scaler values\n", 0);
	/* post scaler values */
	for (i=0; i<pscal->nch; i++)
	{
#if DN2UP
		counts[i] = pdir[i] ? (ppreset[i] - pdata[i]) : pdata[i];
#else
		counts[i] = pdata[i];
#endif
		if (counts[i] != pscaler[i])
		{
			pscaler[i] = counts[i];
			db_post_events(pscal,&(pscaler[i]),DBE_VALUE);
			if (i==0)
			{
				/* convert clock ticks to time */
				pscal->t = pscaler[i] / pscal->freq;
				db_post_events(pscal,&(pscal->t),DBE_VALUE);
			}
		}
	}

#if 0 /* following code disabled in v1.7 */
	/* done? (If we're not interrupt driven, it's our job to check done.) */
	for (i=0, keepon=1; i<pscal->nch && keepon; i++) {
		if (pgate[i] && pdir[i] && (pdata[i] < 0)) {
			pscal->cnt = pscal->pcnt = 0;
			Debug(2, "updateCounts: posting done flag (%d)\n", pscal->cnt);
			db_post_events(pscal,&(pscal->cnt),DBE_VALUE);
			keepon = 0;
		}
	}
#endif

	if (pscal->cnt) {
		/* arrange to call this routine again after user-specified time */
		Debug(4, "updateCounts: arranging for callback\n", 0);
		callbackSetPriority(pscal->prio,&pcallback->callback);
		if (pscal->rate > .01) {
			i = vxTicksPerSecond / pscal->rate; /* ticks between updates */
			i = MAX(1,i);
			wdStart(pcallback->wd_id,i,(FUNCPTR)callbackRequest,(int)pcallback);
		}
	}

	if (!called_by_process) pscal->pact = FALSE;
}


static long special(paddr,after)
struct dbAddr *paddr;
int	after;
{
	struct scalerRecord *pscal = (struct scalerRecord *)(paddr->precord);
	int special_type = paddr->special;
	int i;
	unsigned short *pdir, *pgate;
	long *ppreset;
	struct callback *pcallback = (struct callback *)pscal->dpvt;
	struct callback *pcallback1 = (struct callback *)&(pcallback[1]);

	if (!after) return (0);

	if (paddr->pfield == (void *) &pscal->cnt) {
		/* Scan record if it's not Passive.  (If it's Passive, it will get */
		/* scanned automatically, since .cnt is a Process-Passive field */

		/* arrange to call this routine again after user-specified time */
		callbackSetPriority(pscal->prio,&pcallback1->callback);
		i = vxTicksPerSecond * pscal->dly; /* ticks to delay */
		if (i <= 0) {
			if (pscal->scan) scanOnce((void *)pscal);
		}
		else {
			wdStart(pcallback1->wd_id,i,(FUNCPTR)callbackRequest,
				(int)pcallback1);
		}
	}
	else if (paddr->pfield == (void *) &pscal->tp) {
		/* convert time to clock ticks */
		pscal->pr1 = (long) (pscal->tp * pscal->freq);
		db_post_events(pscal,&(pscal->pr1),DBE_VALUE);
		pscal->d1 = pscal->g1 = 1;
		db_post_events(pscal,&(pscal->d1),DBE_VALUE);
		db_post_events(pscal,&(pscal->g1),DBE_VALUE);
	}
	else if (paddr->pfield == (void *) &pscal->pr1) {
		/* convert clock ticks to time */
		pscal->tp = (double)(pscal->pr1 / pscal->freq);
		db_post_events(pscal,&(pscal->tp),DBE_VALUE);
		if (pscal->tp > 0) {
			pscal->d1 = pscal->g1 = 1;
			db_post_events(pscal,&(pscal->d1),DBE_VALUE);
			db_post_events(pscal,&(pscal->g1),DBE_VALUE);
		}
	}
	else if ((paddr->pfield >= (void *) (&pscal->pr2) ) && 
			 (paddr->pfield <= (void *) (&pscal->pr16))) {
		i = (paddr->pfield - (void *)&(pscal->pr1)) / sizeof(long);
		Debug(4, "special: channel %d preset\n", i);
		pdir = (unsigned short *) &(pscal->d1);
		pgate = (unsigned short *) &(pscal->g1);
		ppreset = (long *) &(pscal->pr1);
		if (ppreset[i] > 0) {
			pdir[i] = pgate[i] = 1;
			db_post_events(pscal,&(pdir[i]),DBE_VALUE);
			db_post_events(pscal,&(pgate[i]),DBE_VALUE);
		}
	}
	else if (paddr->pfield == (void *) &pscal->rate) {
		pscal->rate = MIN(60.,MAX(0.,pscal->rate));
		db_post_events(pscal,&(pscal->tp),DBE_VALUE);
	}
	else if ((paddr->pfield >= (void *) (&pscal->g1) ) && 
			 (paddr->pfield <= (void *) (&pscal->g16))) {
		/* If user set gate field, make sure preset counter has some */
		/* reasonable value. */
		i = (int)((paddr->pfield - (void *)&(pscal->g1)) / sizeof(short));
		Debug(4, "special: channel %d gate\n", i);
		ppreset = (long *) &(pscal->pr1);
		pgate = (unsigned short *) &(pscal->g1);
		if (pgate[i] && (ppreset[i] == 0)) {
			ppreset[i] = 1000;
			db_post_events(pscal,&(ppreset[i]),DBE_VALUE);
		}
	}

	return(0);
}

static long get_precision(paddr, precision)
struct dbAddr *paddr;
long          *precision;
{
	struct scalerRecord *pscal = (struct scalerRecord *) paddr->precord;

	*precision = pscal->prec;
	if (paddr->pfield == (void *) &pscal->vers) {
		*precision = 2;
	} else if (paddr->pfield >= (void *) &pscal->val) {
		*precision = pscal->prec;
	} else {
		recGblGetPrec(paddr, precision);	/* Field is in dbCommon */
	}
	return (0);
}


static void alarm(pscal)
struct scalerRecord *pscal;
{
	if(pscal->udf == TRUE ){
		recGblSetSevr(pscal,UDF_ALARM,INVALID_ALARM);
		return;
	}
	return;
}

static void monitor(pscal)
struct scalerRecord *pscal;
{
	unsigned short monitor_mask;

	monitor_mask = recGblResetAlarms(pscal);

	monitor_mask|=(DBE_VALUE|DBE_LOG);

	/* check all value fields for changes */
	return;
}
