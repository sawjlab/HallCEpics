#define CAEN
/*******************************************************************************
devScaler.c
Device-support routines for Lecroy 1151 16-channel, 32-bit scaler

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
.01  6/26/93	tmm     Lecroy scaler
.02  1/16/95    tmm     v1.0 Joerger scaler
.03  6/11/96    tmm     v1.1 fixed test distinguishing VSC16 and VSC8
.04  6/28/96    tmm     use vsc_num_cards instead of MAX_SCALER_CARDS

*******************************************************************************/
/* version 1.2 */
#include	<vxWorks.h>
#include	<vme.h>
#include	<types.h>
#include	<stdioLib.h>
#include	<string.h>
#include	<math.h>
#include	<iv.h>

#include	<devLib.h> 
#include	<alarm.h>
/* #include	<dbRecType.h>*/
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<dbCommon.h>
#include	<dbScan.h>
#include	<recSup.h>
#include	<devSup.h>
#include	<drvSup.h>
#include	<dbScan.h>
#include	<special.h>
#include	<module_types.h>
#include	<scalerRecord.h>
#include        <epicsExport.h>
#include	"devScaler.h"

/*** Debug support ***/
#define PRIVATE_FUNCTIONS 0	/* normal:1, debug:0 */
#if PRIVATE_FUNCTIONS
#define STATIC static
#else
#define STATIC
#endif
#undef NODEBUG
#ifdef NODEBUG
#define Debug(l,f,v) ;
#else
#define Debug(l,FMT,V) {  if(l <= devScalerdebug) \
			{ printf("%s(%d):",__FILE__,__LINE__); \
			  printf(FMT,V); } }
#endif
volatile int devScalerdebug=6;

#define CARD_0_ADDRESS 0x40000000
#define CARD_ADDRESS_SPACE 0x100
#ifdef CAEN
#define RESET_OFFSET 0x50
#else
#define RESET_OFFSET 0x00
#endif
#define CTRL_OFFSET 0x04
#define DIRECTION_OFFSET 0x08
#ifdef CAEN
#define STATUS_ID_OFFSET 0x58
#define IRQ_VECTOR_OFFSET 0x04
#else
#define STATUS_ID_OFFSET 0x10
#define IRQ_VECTOR_OFFSET 0x10
#endif
#define IRQ_LEVEL_ENABLE_OFFSET 0x14
#define IRQ_MASK_OFFSET 0x18
#define IRQ_RESET_OFFSET 0x1C
#ifdef CAEN
#define REV_SERIAL_NO_OFFSET 0xFE
#define MODULE_TYPE_OFFSET 0xFC
#define MANUFACTURER_ID_OFFSET 0xFA
#define DATA_0_OFFSET 0x10
#else
#define REV_SERIAL_NO_OFFSET 0x20
#define MODULE_TYPE_OFFSET 0x24
#define MANUFACTURER_ID_OFFSET 0x28
#define DATA_0_OFFSET 0x80
#endif
#ifdef CAEN
#else
#endif
#define PRESET_0_OFFSET 0xC0
static long vsc_num_cards = 3;
#ifdef CAEN
/* STATIC void *vsc_addrs = (void *)0xf0da1800;*/
STATIC void *vsc_addrs = (void *)0xda1800;
#else
STATIC void *vsc_addrs = (void *)0x00d00000;
#endif
STATIC long	vsc_InterruptVector = 0;

STATIC long scaler_report(int level);
STATIC long scaler_init(int after);
STATIC long scaler_init_record(struct scalerRecord *psr);
STATIC long scaler_get_ioint_info(int cmd, struct dbCommon *p1, IOSCANPVT *p2);
STATIC long scaler_reset(int card);
STATIC long scaler_read(int card, volatile long **val);
STATIC long scaler_write_preset(int card, int signal, long val);
STATIC long scaler_arm(int card, int val);
STATIC long scaler_done(int card);

SCALERDSET devScaler = {
	7, 
	scaler_report,
	scaler_init,
	scaler_init_record,
	scaler_get_ioint_info,
	scaler_reset,
	scaler_read,
	scaler_write_preset,
	scaler_arm,
	scaler_done
};
epicsExportAddress(dset,devScaler);

STATIC int scaler_total_cards;
STATIC struct scaler_state {
	int card_exists;
	int num_channels;
	int card_in_use;
	int count_in_progress; /* count in progress? */
	unsigned short ident; /* identification info for this card */
	volatile char *localaddr; /* address of this card */
	IOSCANPVT ioscanpvt;
	int done;
};
volatile STATIC struct scaler_state **scaler_state = 0;

/**************************************************
* scaler_report()
***************************************************/
STATIC long scaler_report(int level)
{
	int card;

	printf("*** scaler_report ***\n");
	if (vsc_num_cards <=0) {
#ifdef CAEN
		printf("    No CAEN scaler cards found.\n");
#else
		printf("    No Joerger VSCxx scaler cards found.\n");
#endif
	} else {
		for (card = 0; card < vsc_num_cards; card++) {
			if (scaler_state[card]) {
#ifdef CAEN
				printf("    CAEN %-2d card %d @ 0x%X, id: %d %s\n",
					scaler_state[card]->num_channels,
					card, 
					scaler_state[card]->localaddr, 
					scaler_state[card]->ident,
					scaler_state[card]->card_in_use ? "(in use)": "(NOT in use)");
#else
				printf("    Joerger VSC%-2d card %d @ 0x%X, id: %d %s\n",
					scaler_state[card]->num_channels,
					card, 
					scaler_state[card]->localaddr, 
					scaler_state[card]->ident,
					scaler_state[card]->card_in_use ? "(in use)": "(NOT in use)");
#endif
			}
		}
	}
	return (0);
}

/**************************************************
* scalerISR()
***************************************************/
STATIC int scalerISR(int card)
{
	volatile char *addr;
	volatile unsigned short *p16;

	if ((card+1) > scaler_total_cards) return(ERROR);

	addr = scaler_state[card]->localaddr;
	/* disable interrupts during processing */
	p16 = (unsigned short *)(addr+IRQ_LEVEL_ENABLE_OFFSET);
	*p16 &= (unsigned short) 0x7f;

	/* clear interrupt */
	p16 = (unsigned short *)(addr+IRQ_RESET_OFFSET);
	*p16 = (unsigned short)0;

	/* tell record support the hardware is done counting */
	scaler_state[card]->done = 1;

	/* get the record processed */
	scanIoRequest(scaler_state[card]->ioscanpvt);

	/* enable interrupts */
	p16 = (unsigned short *)(addr+IRQ_LEVEL_ENABLE_OFFSET);
	*p16 |= (unsigned short)0x80;
	return(0);
}


/**************************************************
* scalerISRSetup()
***************************************************/
STATIC int scalerISRSetup(int card)
{
	long status;
	volatile char *addr;
	volatile unsigned short *p16;
	int intLevel;
	
	Debug(5, "scalerISRSetup: Entry, card #%d\n", card);
	return(OK); /* No interrupts for now */
	if ((card+1) > scaler_total_cards) return(ERROR);
	addr = scaler_state[card]->localaddr;

	scanIoInit(&(scaler_state[card]->ioscanpvt));

	status = devConnectInterrupt(intVME, vsc_InterruptVector + card,
		(void *) &scalerISR, (void *) card);
	if (!RTN_SUCCESS(status)) {
	        errPrintf(status, __FILE__, __LINE__, "Can\'t connect to vector %d\n",
			  vsc_InterruptVector + card);
		return (ERROR);
	}

	/* get interrupt level from hardware, and enable that level in EPICS */
	p16 = (unsigned short *)(addr+IRQ_LEVEL_ENABLE_OFFSET);
	intLevel = (int) *p16 & 3;
	Debug(5, "scalerISRSetup: Interrupt level %d\n", intLevel);
	status = devEnableInterruptLevel(intVME, intLevel);
	if (!RTN_SUCCESS(status)) {
		errPrintf(status, __FILE__, __LINE__,
			  "Can\'t enable enterrupt level %d\n", intLevel);
		return (ERROR);
	}
	/* Write interrupt vector to hardware */
	p16 = (unsigned short *)(addr+IRQ_VECTOR_OFFSET);
	*p16 = (unsigned short) (vsc_InterruptVector + card);

	Debug(5, "scalerISRSetup: Exit, card #%d\n", card);
	return (OK);
}


/***************************************************
* initialize all software and hardware
* scaler_init()
****************************************************/
STATIC long scaler_init(int after)
{
	volatile void *localaddr;
	unsigned long dummy, status;
	volatile unsigned long *p32;
	volatile unsigned short *p16;
	void *baseAddr, *probeAddr;
	int card;

	printf("In scaler_init\n");
	taskDelay(120);
	Debug(2,"scaler_init(): entry, after = %d\n", after);
	if (after) return(0);

	/* allocate scaler_state structures, array of pointers */
	if (scaler_state == NULL) {
    	scaler_state = (struct scaler_state **)
				calloc(1, vsc_num_cards * sizeof(struct scaler_state *));

		scaler_total_cards=0;
		for (card=0; card<vsc_num_cards; card++) {
		    scaler_state[card] = (struct scaler_state *)
					calloc(1, sizeof(struct scaler_state));
		}
	}

	/* Check out the hardware. */
	for (card=0; card<vsc_num_cards; card++) {
	  sysBusToLocalAdrs(0x39, (vsc_addrs + card*CARD_ADDRESS_SPACE),
			    &baseAddr);
	  /*		baseAddr = (void *)(vsc_addrs + card*CARD_ADDRESS_SPACE);*/
		probeAddr = baseAddr+DATA_0_OFFSET;

#ifndef CAEN
		/* Can we read from scaler data bank? */
		/*		status = locationProbe(atVMEA32, probeAddr);*/
		status = locationProbe(atVMEA32, probeAddr);
		if (status != S_dev_addressOverlap) {
			if (card==0) {
				errPrintf(status, __FILE__, __LINE__,
					"locationProbe failed: address 0x%x\n", probeAddr);
			}
			return(ERROR);
		}

		/* Can we reserve the required block of VME address space? */
		status = devRegisterAddress(__FILE__, atVMEA32, baseAddr,
			CARD_ADDRESS_SPACE, (void **)&localaddr);
		if (!RTN_SUCCESS(status)) {
			errPrintf(status, __FILE__, __LINE__,
				"Can\'t register 256-byte block at address 0x%x\n", baseAddr);
			return (ERROR);
		}
#else
		localaddr = baseAddr;
#endif
		/* Declare victory. */
		Debug(2,"scaler_init: we own 256 bytes starting at 0x%x\n", localaddr);
		scaler_total_cards++;
		scaler_state[card]->localaddr = localaddr;

		/* reset this card */
		p16 = (unsigned short *)(localaddr + RESET_OFFSET);
		*p16 = 0; /* any write to this address causes reset */

		/* get this card's identification */
		p16 = (unsigned short *)(localaddr + REV_SERIAL_NO_OFFSET);
		scaler_state[card]->ident = *p16;
		Debug(3,"scaler_init: Serial # = %d\n", scaler_state[card]->ident);
		scaler_state[card]->card_exists = 1;

		/* get this card's type (8 or 16 channels?) */
		Debug(2,"scaler_init: Init Address=0x%08.8x\n",localaddr);
		p16 = (unsigned short *)(localaddr + MODULE_TYPE_OFFSET);
#ifdef CAEN
		scaler_state[card]->num_channels = 16;
#else
		scaler_state[card]->num_channels = *p16 & 0x18;
#endif
		Debug(3,"scaler_init: nchan = %d\n", scaler_state[card]->num_channels);
	}

	Debug(3,"scaler_init: Total cards = %d\n\n",scaler_total_cards);
	Debug(3,"scaler_init: scalers initialized\n",0);
	return(0);
}

/***************************************************
* scaler_init_record()
****************************************************/
STATIC long scaler_init_record(struct scalerRecord *psr)
{
	int card = psr->out.value.vmeio.card;
	int status;

	printf("In scaler_init_record\n");
	taskDelay(120);
	/* out must be an VME_IO */
	printf("Out type =%d, should be %d\n",psr->out.type,VME_IO);
	psr->out.type = VME_IO;
	switch (psr->out.type)
	{
	case (VME_IO) : break;
	default:
		recGblRecordError(S_dev_badBus,(void *)psr,
			"devScaler (init_record) Illegal OUT Bus Type");
		return(S_dev_badBus);
	}

	Debug(5,"scaler_init_record: card %d\n", card);
	if (!scaler_state[card]->card_exists)
	{
		recGblRecordError(S_dev_badCard,(void *)psr,
		    "devScaler (init_record) card does not exist!");
		return(S_dev_badCard);
    }

	if (scaler_state[card]->card_in_use)
	{
		recGblRecordError(S_dev_badSignal,(void *)psr,
		    "devScaler (init_record) card already in use!");
		return(S_dev_badSignal);
    }
	scaler_state[card]->card_in_use = 1;
	psr->nch = scaler_state[card]->num_channels;

	/* setup interrupt handler */
	status = scalerISRSetup(card);

	return(0);
}


/***************************************************
* scaler_get_ioint_info()
****************************************************/
STATIC long scaler_get_ioint_info(
	int cmd,
	struct dbCommon *prec,
	IOSCANPVT *ppvt)
{
	struct scalerRecord *psr = (struct scalerRecord *)prec;
	int card = psr->out.value.vmeio.card;

	Debug(5,"scaler_get_ioint_info: cmd = %d\n", cmd);
	*ppvt = scaler_state[card]->ioscanpvt;
	return(0);
}

/***************************************************
* scaler_reset()
****************************************************/
STATIC long scaler_reset(int card)
{
	volatile char *addr;
	volatile unsigned short *pdata;

	Debug(5,"scaler_reset: card %d\n", card);
	if ((card+1) > scaler_total_cards) return(ERROR);
	addr = scaler_state[card]->localaddr;

#ifndef CAEN
	/* disable interrupt */
	pdata = (unsigned short *)(addr + IRQ_LEVEL_ENABLE_OFFSET);
	*pdata &= (unsigned short) 0x7f;
#endif
	/* reset board */
	pdata = (unsigned short *)(addr + RESET_OFFSET);
	*pdata = 0;
	return(0);
}

/***************************************************
* scaler_read()
* return pointer to array of scaler values (on the card)
****************************************************/
STATIC long scaler_read(int card, volatile long **val)
{
	volatile long *pdata;

	Debug(8,"scaler_read: card %d\n", card);
	if ((card+1) > scaler_total_cards) return(ERROR);
	pdata = (long *) (scaler_state[card]->localaddr + DATA_0_OFFSET);
	*val = pdata;
	Debug(8,"scaler_read: ...returns %x (address on VME card)\n", *val);
	Debug(7,"scaler_read: ...(chan 0 = 0x%x)\n", *pdata);
	return(0);
}

/***************************************************
* scaler_write_preset()
****************************************************/
STATIC long scaler_write_preset(int card, int signal, long val)
{
	volatile char *addr;
	volatile long *p32;
	volatile unsigned short *p16;
	short mask;

	Debug(5,"scaler_write_preset: card %d\n", card);
	Debug(5,"scaler_write_preset: signal %d\n", signal);
	Debug(5,"scaler_write_preset: val = %d\n", val);

	if ((card+1) > scaler_total_cards) return(ERROR);

	addr = scaler_state[card]->localaddr;
	p32 = (volatile long *) (addr + PRESET_0_OFFSET);

	p32[signal] = val;

	/* make the preset scaler count down */
	p16 = (unsigned short *) (addr + DIRECTION_OFFSET);
	mask = *p16;
	mask |= 1<<signal;
	Debug(5,"scaler_write_preset: up/down mask = 0x%x\n", mask);
	*p16 = mask;

	/* enable IRQ from preset channel */
	p16 = (unsigned short *) (addr + IRQ_MASK_OFFSET);
	mask = *p16;
	mask |= 1<<signal;
	Debug(5,"scaler_write_preset: IRQ mask = 0x%x\n", mask);
	*p16 = mask;
	return(0);
}

/***************************************************
* scaler_arm()
* Make scaler ready to count.  If ARM output is connected
* to ARM input, and GATE input permits, the scaler will
* actually start counting.
****************************************************/
STATIC long scaler_arm(int card, int val)
{
	volatile char *addr;
	volatile unsigned short *pdata;
	short ctrl_data;

	Debug(5,"scaler_arm: card %d\n", card);
	Debug(5,"scaler_arm: val = %d\n", val);
	if ((card+1) > scaler_total_cards) return(ERROR);
	addr = scaler_state[card]->localaddr;

	/* disable interrupt */
	pdata = (unsigned short *) (addr + IRQ_LEVEL_ENABLE_OFFSET);
	*pdata &= (unsigned short) 0x7f;

	if (val) {
		/* write the interrupt vector to the board */
		pdata = (unsigned short *) (addr + IRQ_VECTOR_OFFSET);
		*pdata = (unsigned short) (vsc_InterruptVector + card);

		/* enable interrupt-when-done */
		pdata = (unsigned short *) (addr + IRQ_LEVEL_ENABLE_OFFSET);
		*pdata |= (unsigned short) 0x80;
	}

	/* arm scaler */
	pdata = (unsigned short *) (addr + CTRL_OFFSET);
	ctrl_data = *pdata;
	if (val) ctrl_data |= 1; else ctrl_data &= 0x0E;
	*pdata = ctrl_data;
	Debug(5,"scaler_arm: ctrl reg => 0x%x\n", *pdata & 0xf);
	return(0);
}


/***************************************************
* scaler_done()
****************************************************/
STATIC long scaler_done(int card)
{
	if ((card+1) > scaler_total_cards) return(ERROR);

	if (scaler_state[card]->done) {
		scaler_state[card]->done = 0;
		return(1);
	} else {
		return(0);
	}
}


/*****************************************************
* VSCSetup()
* User (startup file) calls this function to configure
* us for the hardware.
*****************************************************/
VSCSetup(int num_cards,	/* maximum number of cards in crate */
	   void *addrs,		/* Base Address(0x100-0xffffff00, 256-byte boundary) */
	   unsigned vector)	/* noninterrupting(0), valid vectors(64-255) */
{
	printf("In VCSetup\n");
	taskDelay(120);
	vsc_num_cards = num_cards;
	vsc_addrs = addrs;
	vsc_InterruptVector = vector;
}

/* debugging function */
scaler_show(int card)
{
	volatile char *addr = scaler_state[card]->localaddr;
	volatile unsigned short *p16;
	volatile unsigned long *p32;
	int i;

	printf("scaler_show: card %d\n", card);
	p16 = (unsigned short *) (addr+CTRL_OFFSET);
	printf("scaler_show: ctrl reg = 0x%x\n", *p16&0xf);
	p16 = (unsigned short *) (addr+DIRECTION_OFFSET);
	printf("scaler_show: dir reg = 0x%x\n", *p16);
	p16 = (unsigned short *) (addr+STATUS_ID_OFFSET);
	printf("scaler_show: irq vector = 0x%x\n", *p16&0xff);
	p16 = (unsigned short *) (addr+IRQ_LEVEL_ENABLE_OFFSET);
	printf("scaler_show: irq level/enable = 0x%x\n", *p16&0xff);
	p16 = (unsigned short *) (addr+IRQ_MASK_OFFSET);
	printf("scaler_show: irq mask reg = 0x%x\n", *p16);
	p16 = (unsigned short *) (addr+MODULE_TYPE_OFFSET);
	printf("scaler_show: module type = 0x%x\n", *p16&0xff);
	p32 = (unsigned long *) (addr + DATA_0_OFFSET);
	for (i=0; i<scaler_state[card]->num_channels; i++) {
		printf("    scaler_show: channel %d counts = %ld\n", i, p32[i]);
	}
	printf("scaler_show: scaler_state[card]->done = %d\n", scaler_state[card]->done);
	return(0);
}
