/* bcm
   Calculate beam currents

   Stephen Wood, June 24, 1998
*/

program bcm_gen

option -r;
option +c;

int clockm;
int clockp;
int bcm1m;
int bcm2m;
int bcm1p;
int bcm2p;
int clockm_last;
int clockp_last;
int bcm1m_last;
int bcm2m_last;
int bcm1p_last;
int bcm2p_last;
int clockm_dif;
int clockp_dif;
int bcm1m_dif;
int bcm2m_dif;
int bcm1p_dif;
int bcm2p_dif;
int icountm;
int icountp;

double ibcm1m;
double ibcm2m;
double ibcm1p;
double ibcm2p;
double ibcm1_off;
double ibcm2_off;
double ibcm1_gain;
double ibcm2_gain;
/*
double iave1m;
double iave2m;
double iave1p;
double iave2p;
double iavetimem;
double iavetimep;
*/
double isum1m;
double isum2m;
double isum1p;
double isum2p;
double isumtimem;
double isumtimep;
double time_asy;
double current_asy1;
double current_asy2;
double clocksum;

assign clockm to "hcscalerm.S1";
assign bcm1m to "hcscalerm.S2";
assign bcm2m to "hcscalerm.S3";
assign clockp to "hcscalerp.S1";
assign bcm1p to "hcscalerp.S2";
assign bcm2p to "hcscalerp.S3";

assign ibcm1m to "ibcm1m";
assign ibcm2m to "ibcm2m";
assign ibcm1p to "ibcm1p";
assign ibcm2p to "ibcm2p";
assign ibcm1_off to "ibcm1.AOFF";
assign ibcm2_off to "ibcm2.AOFF";
assign ibcm1_gain to "ibcm1.ASLO";
assign ibcm2_gain to "ibcm2.ASLO";
assign time_asy to "time_asy";
assign current_asy1 to "current_asy1";
assign current_asy2 to "current_asy2";
assign clockm_dif to "clockm_dif";
assign clockp_dif to "clockp_dif";

/*assign iave1m to "iave1m";
assign iave2m to "iave2m";
assign iave1p to "iave1p";
assign iave2p to "iave2p";*/

/*assign iavetimem to "iavetimem";
assign iavetimep to "iavetimep";*/

evflag ef_clockp;
sync clockp ef_clockp;
evflag ef_clockm;
sync clockm ef_clockm;

monitor clockp;
monitor clockm;

ss bcm_gen {
  state init {
    when (efTest(ef_clockm)||efTest(ef_clockp)) {
      efClear(ef_clockm);
      efClear(ef_clockp);
      pvGet(bcm1m);
      pvGet(bcm2m);
      pvGet(bcm1p);
      pvGet(bcm2p);
      ibcm1m = 0.0;
      ibcm2m = 0.0;
      ibcm1p = 0.0;
      ibcm2p = 0.0;
      pvPut(ibcm1m);/* May not save these in records */
      pvPut(ibcm2m);
      pvPut(ibcm1p);
      pvPut(ibcm2p);
      /*      pvGet(clockm);*/
      clockm_last = clockm;
      clockp_last = clockp;
      bcm1m_last = bcm1m;
      bcm2m_last = bcm2m;
      bcm1p_last = bcm1p;
      bcm2p_last = bcm2p;
      /* Average current */
      icountm = 0;
      icountp = 0;
      isum1m = 0.0;
      isum2m = 0.0;
      isum1p = 0.0;
      isum2p = 0.0;
      isumtimem = 0.0;
      isumtimep = 0.0;
    } state load_calibration
  }
  state load_calibration {
    when () {
      pvGet(ibcm1_off);
      pvGet(ibcm2_off);
      pvGet(ibcm1_gain);
      pvGet(ibcm2_gain);
    } state calculate
  }
  state calculate {
    when (efTest(ef_clockm)||efTest(ef_clockp)) {
      efClear(ef_clockm);
      efClear(ef_clockp);
      pvGet(bcm1m);
      pvGet(bcm2m);
      pvGet(bcm1p);
      pvGet(bcm2p);
      /*      pvGet(clockm);*/
      clockm_dif = scaldif(clockm,clockm_last);
      clockp_dif = scaldif(clockp,clockp_last);
      bcm1m_dif = scaldif(bcm1m,bcm1m_last);
      bcm2m_dif = scaldif(bcm2m,bcm2m_last);
      bcm1p_dif = scaldif(bcm1p,bcm1p_last);
      bcm2p_dif = scaldif(bcm2p,bcm2p_last);
      clockm_last = clockm;
      clockp_last = clockp;
      bcm1m_last = bcm1m;
      bcm2m_last = bcm2m;
      bcm1p_last = bcm1p;
      bcm2p_last = bcm2p;
      if(clockm_dif > 0) {
	ibcm1m = ibcm1_gain * (bcm1m_dif/ (clockm_dif / 1000000.0) - ibcm1_off);
	ibcm2m = ibcm2_gain * (bcm2m_dif/ (clockm_dif / 1000000.0) - ibcm2_off);
	/*			printf("clockm = %d\n",clockm_dif);*/
      } /*else {
	ibcm1m = -1;
	ibcm2m = -1;
      }*/
      if(clockp_dif > 0) {
	ibcm1p = ibcm1_gain * (bcm1p_dif/ (clockp_dif / 1000000.0) - ibcm1_off);
	ibcm2p = ibcm2_gain * (bcm2p_dif/ (clockp_dif / 1000000.0) - ibcm2_off);
	/*		printf("clockp = %d\n",clockp_dif);*/
      }/* else {
	ibcm1p = -1;
	ibcm2p = -1;
      }*/
      pvPut(ibcm1m);
      pvPut(ibcm2m);
      pvPut(ibcm1p);
      pvPut(ibcm2p);
      if(ibcm1m > 0 && ibcm1p > 0) {
	current_asy1 = (ibcm1p-ibcm1m)/(ibcm1p+ibcm1m);
	current_asy2 = (ibcm2p-ibcm2m)/(ibcm2p+ibcm2m);
      } else {
	current_asy1 = -1;
	current_asy2 = -1;
      }	
      if(clockp_dif > 0 && clockm_dif > 0) {
	clocksum = (clockp_dif+clockm_dif);
	time_asy = (clockp_dif-clockm_dif)/clocksum;
      } /*else {
	time_asy = -1;
      }*/
      pvPut(current_asy1);
      pvPut(current_asy2);
      pvPut(time_asy);
      pvPut(clockm_dif);
      pvPut(clockp_dif);
      /* Average current */
      /*	isum1 += ibcm1;
	isum2 += ibcm2;
      isumtime += clock_dif/1000000.0;
      icount++;
      if(icount >= 30) {
	iave1 = isum1/icount;
	iave2 = isum2/icount;
	iavetime = isumtime;
	pvPut(iave1);
	pvPut(iave2);
	pvPut(iavetime);
        icount = 0;
        isum1 = 0.0;
        isum2 = 0.0;
        isumtime = 0.0;
	*/
	/* special code to put average currents to LabView */
      /*        pushCurrent(iavetime,iave1,iave2,iave3);
      }*/
    } state calculate
  }
}
