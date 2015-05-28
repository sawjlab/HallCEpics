/* cpid.c */
/* share/src/rec @(#)recCpid.c	1.25     8/4/93 */

/* cpid.c - Record Support Routines for Pid records */
/*
 *      Current Author: Bob Dalesio
 *      Date:           06-27-95 
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1995,SURA 
 *
 * Modification Log:
 * -----------------
 * .00	lrd	6-21-95		started with the PID record -
 *				removed setpoint location stuff
 *				added output readback location
 *				added pid mode - change or position
 *				added output mode - normal, sequencial, manual, local
 *				make control calue changable
 * .01	lrd	10-1-95		handle possibility of normal/position having oval outside 
 *				- limits before coming into this state - verify current
 *				- value is in the limits before apply new changes
 *				process loop at scan rate - only pid stuff at slow rate
 *				apply monitor deadbands to outputchanges - not value changes
 * .02	lrd	12-13-95	make oval be orbv+dm
 *				do not post dm =0 when dt < mdt
 *				fix oval going over max on normal/position mode
 * .03	lrd	12-17-95	change type should post monitors when either ORBV or DM change
 *				fix negative change on a POSITION in NORMAL (it was subtracting it)
 *				use odm for NORMAL/CHANGE to keeep last computed DM for monitors
 * .04	lrd	12-22-95	compute dm in every mode - avoids wind-up of I term
 * .05	lrd	01-02-96	fix assignment of 0 to float to be 0.0
 * .06  mmk     01-04-96        re-arragne min, max, and dmin, dmax to fix the problem
 *                              of getting stuck if oval is greater than max by 
 *                              less than the value of dmin 
 * .07  mmk     02-09-96	added hopi and lopi, display limits for orbv
 */

#include	<vxWorks.h>
#include	<types.h>
#include	<stdioLib.h>
#include	<lstLib.h>
#include	<string.h>
/*since tickLib is not defined just define tickGet*/
unsigned long tickGet();

#include	<alarm.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<dbFldTypes.h>
#include	<errMdef.h>
#include	<recSup.h>
#define GEN_SIZE_OFFSET
#include	<cpidRecord.h>
#undef GEN_SIZE_OFFSET

#define		NORMAL		0
#define		MANUAL		1
#define		SEQUENCER	2
#define		LOCAL_CNTRL	3

#define		CHANGE		0
#define		POSITION	1

/* Create RSET - Record Support Entry Table*/
#define report NULL
#define initialize NULL
static long init_record();
static long process();
#define special NULL
static long get_value();
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
static long get_units();
static long get_precision();
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
static long get_graphic_double();
static long get_control_double();
static long get_alarm_double();
struct rset cpidRSET={
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
	get_alarm_double };


static void alarm();
static void monitor();
static long do_pid();


static long init_record(ppid,pass)
        struct cpidRecord	*ppid;
        int pass;
{
        /* Added for Channel Access Links */
        long status;

	ppid->udf = FALSE;
        if (pass==0) return(0);

	return(0);
}

static long process(ppid)
	struct cpidRecord	*ppid;
{
	long		 status;

	ppid->pact = TRUE;

	/* compute new output */
	status=do_pid(ppid);
	if(status==1) {
		ppid->pact = FALSE;
		return(0);
	}

	tsLocalTime(&ppid->time);

	/* check for alarms */
	alarm(ppid);

	/* check event list */
	monitor(ppid);

	/* process the forward scan link record */
	recGblFwdLink(ppid);

	ppid->pact=FALSE;
	return(status);
}

static long get_value(ppid,pvdes)
    struct cpidRecord		*ppid;
    struct valueDes	*pvdes;
{
    pvdes->field_type = DBF_FLOAT;
    pvdes->no_elements=1;
    (float *)(pvdes->pvalue) = &ppid->val;
    return(0);
}

static long get_units(paddr,units)
    struct dbAddr *paddr;
    char	  *units;
{
    struct cpidRecord	*ppid=(struct cpidRecord *)paddr->precord;

    strncpy(units,ppid->egu,sizeof(ppid->egu));
    return(0);
}

static long get_precision(paddr,precision)
    struct dbAddr *paddr;
    long	  *precision;
{
    struct cpidRecord	*ppid=(struct cpidRecord *)paddr->precord;

    *precision = ppid->prec;
    if(paddr->pfield == (void *)&ppid->val
    || paddr->pfield == (void *)&ppid->cval) return(0);
    recGblGetPrec(paddr,precision);
    return(0);
}


static long get_graphic_double(paddr,pgd)
    struct dbAddr *paddr;
    struct dbr_grDouble	*pgd;
{
    struct cpidRecord	*ppid=(struct cpidRecord *)paddr->precord;

    if(paddr->pfield==(void *)&ppid->val
    || paddr->pfield==(void *)&ppid->hihi
    || paddr->pfield==(void *)&ppid->high
    || paddr->pfield==(void *)&ppid->low
    || paddr->pfield==(void *)&ppid->lolo
    || paddr->pfield==(void *)&ppid->p
    || paddr->pfield==(void *)&ppid->i
    || paddr->pfield==(void *)&ppid->d
    || paddr->pfield==(void *)&ppid->cval){
        pgd->upper_disp_limit = ppid->hopr;
        pgd->lower_disp_limit = ppid->lopr;
    }else if(paddr->pfield==(void *)&ppid->orbv){
	pgd->upper_disp_limit = ppid->hopi;
	pgd->lower_disp_limit = ppid->lopi;
    } else recGblGetGraphicDouble(paddr,pgd);
    return(0);
}

static long get_control_double(paddr,pcd)
    struct dbAddr *paddr;
    struct dbr_ctrlDouble *pcd;
{
    struct cpidRecord	*ppid=(struct cpidRecord *)paddr->precord;

    if(paddr->pfield==(void *)&ppid->val
    || paddr->pfield==(void *)&ppid->hihi
    || paddr->pfield==(void *)&ppid->high
    || paddr->pfield==(void *)&ppid->low
    || paddr->pfield==(void *)&ppid->lolo
    || paddr->pfield==(void *)&ppid->p
    || paddr->pfield==(void *)&ppid->i
    || paddr->pfield==(void *)&ppid->d
    || paddr->pfield==(void *)&ppid->cval){
        pcd->upper_ctrl_limit = ppid->hopr;
        pcd->lower_ctrl_limit = ppid->lopr;
    }else if(paddr->pfield==(void *)&ppid->orbv){
	pcd->upper_ctrl_limit = ppid->hopi;
	pcd->lower_ctrl_limit = ppid->lopi;
    } else recGblGetControlDouble(paddr,pcd);
    return(0);
}
static long get_alarm_double(paddr,pad)
    struct dbAddr *paddr;
    struct dbr_alDouble	*pad;
{
    struct cpidRecord	*ppid=(struct cpidRecord *)paddr->precord;

    if(paddr->pfield==(void *)&ppid->val){
         pad->upper_alarm_limit = ppid->hihi;
         pad->upper_warning_limit = ppid->high;
         pad->lower_warning_limit = ppid->low;
         pad->lower_alarm_limit = ppid->lolo;
    } else recGblGetAlarmDouble(paddr,pad);
    return(0);
}

static void alarm(ppid)
    struct cpidRecord	*ppid;
{
	double		val;
	float		hyst, lalm, hihi, high, low, lolo;
	unsigned short	hhsv, llsv, hsv, lsv;

	if(ppid->udf == TRUE ){
 		recGblSetSevr(ppid,UDF_ALARM,INVALID_ALARM);
		return;
	}
	hihi = ppid->hihi; lolo = ppid->lolo; high = ppid->high; low = ppid->low;
	hhsv = ppid->hhsv; llsv = ppid->llsv; hsv = ppid->hsv; lsv = ppid->lsv;
	val = ppid->val; hyst = ppid->hyst; lalm = ppid->lalm;

	/* alarm condition hihi */
	if (hhsv && (val >= hihi || ((lalm==hihi) && (val >= hihi-hyst)))){
	        if (recGblSetSevr(ppid,HIHI_ALARM,ppid->hhsv)) ppid->lalm = hihi;
		return;
	}

	/* alarm condition lolo */
	if (llsv && (val <= lolo || ((lalm==lolo) && (val <= lolo+hyst)))){
	        if (recGblSetSevr(ppid,LOLO_ALARM,ppid->llsv)) ppid->lalm = lolo;
		return;
	}

	/* alarm condition high */
	if (hsv && (val >= high || ((lalm==high) && (val >= high-hyst)))){
	        if (recGblSetSevr(ppid,HIGH_ALARM,ppid->hsv)) ppid->lalm = high;
		return;
	}

	/* alarm condition low */
	if (lsv && (val <= low || ((lalm==low) && (val <= low+hyst)))){
	        if (recGblSetSevr(ppid,LOW_ALARM,ppid->lsv)) ppid->lalm = low;
		return;
	}

	/* we get here only if val is out of alarm by at least hyst */
	ppid->lalm = val;
	return;
}

static void monitor(ppid)
    struct cpidRecord	*ppid;
{
	unsigned short	monitor_mask;
	float		delta,out;

        monitor_mask = recGblResetAlarms(ppid);
        /* value being monitored is either the change in output or absolute output */
	if (ppid->omod == CHANGE){
		if (ppid->pmod == NORMAL)	out = ppid->orbv + ppid->odm;
		else				out = ppid->orbv + ppid->dm;
	}else{
		out = ppid->oval;
	}

        /* check for value change */
        delta = ppid->mlst - out;
        if(delta<0.0) delta = -delta;
        if (delta > ppid->mdel) {
                /* post events for value change */
                monitor_mask |= DBE_VALUE;
                /* update last value monitored */
                ppid->mlst = out;
        }

        /* check for archive change */
        delta = ppid->alst - out;
        if(delta<0.0) delta = -delta;
        if (delta > ppid->adel) {
                /* post events on value field for archive change */
                monitor_mask |= DBE_LOG;
                /* update last archive value monitored */
                ppid->alst = out;
        }

        /* send out all monitors  for value changes*/
        if (monitor_mask){
                db_post_events(ppid,&ppid->oval,monitor_mask);
		db_post_events(ppid,&ppid->ct,monitor_mask);
		db_post_events(ppid,&ppid->dm,monitor_mask);
		db_post_events(ppid,&ppid->outv,monitor_mask);
		if (ppid->pmod == NORMAL){
			db_post_events(ppid,&ppid->p,monitor_mask);
			db_post_events(ppid,&ppid->i,monitor_mask);
			db_post_events(ppid,&ppid->d,monitor_mask);
			db_post_events(ppid,&ppid->err,monitor_mask);
			db_post_events(ppid,&ppid->derr,monitor_mask);
			db_post_events(ppid,&ppid->dt,monitor_mask);
		}
        }
	if (ppid->lmod != ppid->pmod){
		db_post_events(ppid,&ppid->pmod,DBE_LOG | DBE_VALUE);
		ppid->lmod = ppid->pmod;
	}
	if (ppid->lovl != ppid->val)
	{
		db_post_events(ppid,&ppid->val,DBE_LOG | DBE_VALUE);
		ppid->lovl = ppid->val;
	}
        return;
}

/* A discrete form of the PID algorithm is as follows
 B
 * M(n) = KP*E(n) 
 *      + KI*SUMi(E(i)*dT(i))
 *	+ KD*(E(n) -E(n-1))/dT(n) + Mr
 * where
 *	M(n)	Value of manipulated variable at nth sampling instant
 *	KP,KI,KD Proportional, Integral, and Differential Gains
 *		NOTE: KI is inverse of normal definition of KI
 *	E(n)	Error at nth sampling instant
 *	SUMi	Sum from i=0 to i=n
 *	dT(n)	Time difference between n-1 and n
 *	Mr midrange adjustment
 *
 * Taking first difference yields
 * delM(n) = KP*(E(n)-E(n-1)) + KI * E(n)*dT(n)
 *		+ KD*((E(n)-E(n-1))/dT(n) - (E(n-1)-E(n-2))/dT(n-1))
 * or using variables defined in following
 * dm = kp*de + ki*e*dt + kd*(de/dt - dep/dtp)
 */
static long do_pid(ppid)
struct cpidRecord     *ppid;
{
	long		status,nRequest;
	unsigned long	ctp;	/*clock ticks previous	*/
	unsigned long	ct;	/*clock ticks		*/
	unsigned short	out_rb;	/* output readback exists */
	float		cval;	/*actual value		*/
	float		orbv;	/* output readback value */
	float		dt;	/*delta time (seconds)	*/
	float		dtp;	/*previous dt		*/
	float		kp,ki,kd;/*gains		*/
	float		e;	/*error			*/
	float		ep;	/*previous error	*/
	float		de;	/*change in error	*/
	float		dep;	/*prev change in error	*/
	float		dm;	/*change in manip variable */
	float		mdm;	/*adjusted change in manip variable */
	float		p;	/*proportional contribution*/
	float		i;	/*integral contribution*/
	float		d;	/*derivative contribution*/
	float           last;   /*last requested output value */
	unsigned short	local_req,mode;

        /* fetch the controlled value */
        if (ppid->cvl.type != DB_LINK) { /* nothing to control*/
                if (recGblSetSevr(ppid,SOFT_ALARM,INVALID_ALARM)) return(0);
	}
        nRequest=1;
        if(dbGetLink(&(ppid->cvl),DBR_FLOAT, &cval,0,0)!=NULL) {
                recGblSetSevr(ppid,LINK_ALARM,INVALID_ALARM);
                return(0);
        }else if (cval != ppid->cval){
		ppid->cval = cval;
		db_post_events(ppid,&ppid->cval,DBE_VALUE|DBE_LOG);
	}

	/* fetch the output readback value */
	out_rb = 0;
        if (ppid->orbl.type == DB_LINK) { /* nothing to control*/
        	nRequest=1;
        	if(dbGetLink(&(ppid->orbl),DBR_FLOAT,
		  &orbv,0,0)!=NULL) {
        	        recGblSetSevr(ppid,LINK_ALARM,INVALID_ALARM);
        	        return(0);
        	}else if (orbv != ppid->orbv){
			ppid->orbv = orbv;
			db_post_events(ppid,&ppid->orbv,DBE_VALUE|DBE_LOG);
		}
		out_rb = 1;
	}

	/* determine the mode */
	mode = NORMAL;
	if (ppid->smod) mode = SEQUENCER;
	if (ppid->mmod) mode = MANUAL;
        if (ppid->loc.type == DB_LINK) { /* no local/remote switch */
        	nRequest=1;
        	if(dbGetLink(&(ppid->loc),DBR_FLOAT,
		  &local_req,0,0)!=NULL) {
        	        recGblSetSevr(ppid,LINK_ALARM,INVALID_ALARM);
        	}else{
			if (!local_req) mode = LOCAL_CNTRL;
		}
	}

	/* on the first scan - no time is set - make setpoint = controlled value */
	if (ppid->time.secPastEpoch == 0){
		ppid->val = cval;
		ppid->oval = ppid->orbv;
		db_post_events(ppid,&ppid->val,DBE_VALUE|DBE_LOG);
	}

	if (ppid->udf == TRUE ) {
                recGblSetSevr(ppid,UDF_ALARM,INVALID_ALARM);
                return(0);
	}

	/* compute time difference */
	ctp = ppid->ct;
	ct = tickGet();
	if (ppid->time.secPastEpoch == 0){
		dt=0.0;
	} else {
		if(ctp<ct) {
			dt = (float)(ct-ctp);
		}else { /* clock has overflowed */
			dt = (unsigned long)(0xffffffff) - ctp;
			dt = dt + ct + 1;
		}
		dt = dt/vxTicksPerSecond;
	}

	/* compute the change in output in any mode */
	ppid->dm = dm = 0.0;
	if (dt >= ppid->mdt){
		dtp = ppid->dt;
		kp = ppid->kp;
		ki = ppid->ki;
		kd = ppid->kd;
		ep = ppid->err;
		dep = ppid->derr;
		e = ppid->val - ppid->cval;	/* setpoint - readback */
		de = e - ep;
		p = kp*de;
		i = ki*e*dt;
		if(dtp>0.0 && dt>0.0) d = kd*(de/dt - dep/dtp);
		else d = 0.0;
		dm = p + i + d;

		/* update record*/
		ppid->dt   = dt;
		ppid->ct  = ct;
		ppid->err  = e;
		ppid->derr = de;
		ppid->p  = p;
		ppid->i  = i;
		ppid->d  = d;

	        if (mode == NORMAL){
		
			/* correct for min and max */
            		if (ppid->omod == CHANGE) last = ppid->orbv;
			else                      last = ppid->oval;

			if(dm + last > ppid->max) dm = ppid->max - last;
			else if(dm + last < ppid->min) dm = ppid->min - last;
				 
                        /* correct for dmin and dmax */
	        	mdm = dm < 0.0? -dm:dm;
	        	if(mdm > ppid->dmax) mdm = ppid->dmax;
	        	else if(mdm < ppid->dmin) mdm = 0;
	        	dm = dm < 0.0? -mdm:mdm;

			if(ppid->omod == CHANGE){
				ppid->dm = dm;
				ppid->odm = ppid->dm;	/* for monitors when dm is not calculated */
						/* needs to be done here to get clamps    */
                                }
                        else{ 
				ppid->oval += dm;
				ppid->dm = dm;
                                } 
						
	        }
	}

	if (mode == MANUAL){
		if (ppid->omod == CHANGE){
			dm = ppid->mval - ppid->orbv;
			if (dm < 0.0) dm = -dm;
			/* dmin is the only constraint in manual mode */
			if (dm < ppid->dmin) ppid->dm = 0.0;
			else                 ppid->dm = ppid->mval - ppid->orbv;
		}else{
			dm = ppid->oval - ppid->mval;
			if (dm < 0.0) dm = -dm;
			if (dm >= ppid->dmin) 
				ppid->oval = ppid->mval;
		}
	}else if (mode == LOCAL_CNTRL){
		if (ppid->omod == CHANGE) ppid->dm = 0.0;
		else ppid->oval = ppid->orbv;
	}else if (mode == SEQUENCER) {
		if (ppid->omod == CHANGE){
			dm = ppid->sval - ppid->orbv;
			if (dm < 0.0) dm = -dm;
			/* dmin is the only constraint in sequencer mode */
			if (dm < ppid->dmin) ppid->dm = 0.0;
			else                 ppid->dm = ppid->sval - ppid->orbv;
		}else{
			dm = ppid->oval - ppid->sval;
			if (dm < 0.0) dm = -dm;
			if (dm >= ppid->dmin) ppid->oval = ppid->sval;
		}
	}
	nRequest = 1;
	if (ppid->omod == CHANGE){
		if (mode == NORMAL) ppid->oval = ppid->orbv + ppid->odm;
		else		    ppid->oval = ppid->orbv + ppid->dm;
		ppid->outv = ppid->dm;
	}else{
		ppid->dm = ppid->oval - ppid->mlst;
		ppid->outv = ppid->oval;
	}
	dbPutLink(&(ppid->out),DBR_FLOAT,&(ppid->outv),&nRequest);

	if (mode != ppid->pmod){
		ppid->pmod = mode;
	}

	return(0);
}
