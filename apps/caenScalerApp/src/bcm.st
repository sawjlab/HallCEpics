/* bcm
   Calculate beam currents

   Stephen Wood, June 24, 1998

   2009.02.11 (saw) Add bcm4 for Moller BCM
*/

program bcm

option -r;
option +c;

int clock;
int bcm1;
int bcm2;
int bcm3;
int bcm4;
int bcm5;
int bcm6;
int clock_last;
int bcm1_last;
int bcm2_last;
int bcm3_last;
int bcm4_last;
int bcm5_last;
int bcm6_last;
int clock_dif;
int bcm1_dif;
int bcm2_dif;
int bcm3_dif;
int bcm4_dif;
int bcm5_dif;
int bcm6_dif;
int icount;

double ibcm1;
double ibcm2;
double ibcm3;
double ibcm4;
double ibcm5;
double ibcm6;
double ibcm1_off;
double ibcm2_off;
double ibcm3_off;
double ibcm4_off;
double ibcm5_off;
double ibcm6_off;
double ibcm1_gain;
double ibcm2_gain;
double ibcm3_gain;
double ibcm4_gain;
double ibcm5_gain;
double ibcm6_gain;
double iave1;
double iave2;
double iave3;
double iave4;
double iave5;
double iave6;
double iavetime;
double isum1;
double isum2;
double isum3;
double isum4;
double isum5;
double isum6;
double isumtime;

assign clock to "hcscaler.S1";
assign bcm1 to "hcscaler.S2";
assign bcm2 to "hcscaler.S3";
assign bcm3 to "hcscaler.S4";
assign bcm4 to "hcscaler.S5";
assign bcm5 to "hcscaler.S6";
assign bcm6 to "hcscaler.S7";

assign ibcm1 to "ibcm1";
assign ibcm2 to "ibcm2";
assign ibcm3 to "ibcm3";
assign ibcm4 to "ibcm4";
assign ibcm5 to "ibcm5";
assign ibcm6 to "ibcm6";
assign ibcm1_off to "ibcm1.AOFF";
assign ibcm2_off to "ibcm2.AOFF";
assign ibcm3_off to "ibcm3.AOFF";
assign ibcm4_off to "ibcm4.AOFF";
assign ibcm5_off to "ibcm5.AOFF";
assign ibcm6_off to "ibcm6.AOFF";
assign ibcm1_gain to "ibcm1.ASLO";
assign ibcm2_gain to "ibcm2.ASLO";
assign ibcm3_gain to "ibcm3.ASLO";
assign ibcm4_gain to "ibcm4.ASLO";
assign ibcm5_gain to "ibcm5.ASLO";
assign ibcm6_gain to "ibcm6.ASLO";

assign iave1 to "iave1";
assign iave2 to "iave2";
assign iave3 to "iave3";
assign iave4 to "iave4";
assign iave5 to "iave5";
assign iave6 to "iave6";
assign iavetime to "iavetime";

evflag ef_clock;
sync clock ef_clock;

monitor clock;

ss bcm {
  state init {
    when (efTestAndClear(ef_clock)) {
      pvGet(bcm1);
      pvGet(bcm2);
      pvGet(bcm3);
      pvGet(bcm4);
      pvGet(bcm5);
      pvGet(bcm6);
      ibcm1 = 0.0;
      ibcm2 = 0.0;
      ibcm3 = 0.0;
      ibcm4 = 0.0;
      ibcm5 = 0.0;
      ibcm6 = 0.0;
      pvPut(ibcm1);
      pvPut(ibcm2);
      pvPut(ibcm3);
      pvPut(ibcm4);
      pvPut(ibcm5);
      pvPut(ibcm6);
      clock_last = clock;
      bcm1_last = bcm1;
      bcm2_last = bcm2;
      bcm3_last = bcm3;
      bcm4_last = bcm4;
      bcm4_last = bcm5;
      bcm4_last = bcm6;
      /* Average current */
      icount = 0;
      isum1 = 0.0;
      isum2 = 0.0;
      isum3 = 0.0;
      isum4 = 0.0;
      isum5 = 0.0;
      isum6 = 0.0;
      isumtime = 0.0;
    } state load_calibration
  }
  state load_calibration {
    when () {
      pvGet(ibcm1_off);
      pvGet(ibcm2_off);
      pvGet(ibcm3_off);
      pvGet(ibcm4_off);
      pvGet(ibcm5_off);
      pvGet(ibcm6_off);
      pvGet(ibcm1_gain);
      pvGet(ibcm2_gain);
      pvGet(ibcm3_gain);
      pvGet(ibcm4_gain);
      pvGet(ibcm5_gain);
      pvGet(ibcm6_gain);
    } state calculate
  }
  state calculate {
    when (efTestAndClear(ef_clock)) {
      pvGet(bcm1);
      pvGet(bcm2);
      pvGet(bcm3);
      pvGet(bcm4);
      pvGet(bcm5);
      pvGet(bcm6);
      clock_dif = scaldif(clock,clock_last);
      bcm1_dif = scaldif(bcm1,bcm1_last);
      bcm2_dif = scaldif(bcm2,bcm2_last);
      bcm3_dif = scaldif(bcm3,bcm3_last);
      bcm4_dif = scaldif(bcm4,bcm4_last);
      bcm5_dif = scaldif(bcm5,bcm5_last);
      bcm6_dif = scaldif(bcm6,bcm6_last);
      /*      printf("%ld %ld %ld\n",clock_dif,bcm3,bcm3_last);*/
      /* printf("%ld %ld %ld %ld\n",clock_dif,bcm1_dif,bcm2_dif,bcm3_dif);*/
      clock_last = clock;
      bcm1_last = bcm1;
      bcm2_last = bcm2;
      bcm3_last = bcm3;
      bcm4_last = bcm4;
      bcm5_last = bcm5;
      bcm6_last = bcm6;
      ibcm1 = ibcm1_gain * (bcm1_dif/ (clock_dif / 1000000.0) - ibcm1_off);
      ibcm2 = ibcm2_gain * (bcm2_dif/ (clock_dif / 1000000.0) - ibcm2_off);
      ibcm3 = ibcm3_gain * (bcm3_dif/ (clock_dif / 1000000.0) - ibcm3_off);
      ibcm4 = ibcm4_gain * (bcm4_dif/ (clock_dif / 1000000.0) - ibcm4_off);
      ibcm5 = ibcm5_gain * (bcm5_dif/ (clock_dif / 1000000.0) - ibcm5_off);
      ibcm6 = ibcm6_gain * (bcm6_dif/ (clock_dif / 1000000.0) - ibcm6_off);
/*	printf("%f %ld %f %f\n",ibcm1, bcm2_dif, ibcm2_off, ibcm2_gain);*/
      pvPut(ibcm1);
      pvPut(ibcm2);
      pvPut(ibcm3);
      pvPut(ibcm4);
      pvPut(ibcm5);
      pvPut(ibcm6);
      /* Average current */
	isum1 += ibcm1;
	isum2 += ibcm2;
	isum3 += ibcm3;
	isum4 += ibcm4;
	isum5 += ibcm5;
	isum6 += ibcm6;
      isumtime += clock_dif/1000000.0;
      icount++;
      if(icount >= 30) {
	iave1 = isum1/icount;
	iave2 = isum2/icount;
	iave3 = isum3/icount;
	iave4 = isum4/icount;
	iave5 = isum5/icount;
	iave6 = isum6/icount;
	iavetime = isumtime;
	pvPut(iave1);
	pvPut(iave2);
	pvPut(iave3);
	pvPut(iave4);
	pvPut(iave5);
	pvPut(iave6);
	pvPut(iavetime);
        icount = 0;
        isum1 = 0.0;
        isum2 = 0.0;
        isum3 = 0.0;
        isum4 = 0.0;
        isum5 = 0.0;
        isum6 = 0.0;
        isumtime = 0.0;
	/* special code to put average currents to LabView for gen */
	/*        if(pushCurrent(iavetime,iave1,iave2,iave3)==-1) 
	  pushCurrent(iavetime,iave1,iave2,iave3); *//* Try one more time */
      }
    } state calculate
  }
}
%{

  /* Take the difference between two 32 bit scalers */
  int scaldif(unsigned int new, unsigned int old)
    {
      int dif;

      if(new > old) {
	return((int) (new-old));
      } else {
	return((int) (~old + 1 + new));
      }
    }

}%
