/* halomon

   Hall C rates

   Stephen Wood, July 22, 2005
*/

program hcrate

option -r;
option +c;

#define CLOCKRATE 1000000
#define CLOCKCHAN 2
#define TOLERANCE 0.0001
#define NTIMES 5

double rate[16];
int hcscaler[16];
int hcscaler_last[16];

int hc_update_period;
struct epicsTimeStamp timestamp;
struct epicsTimeStamp last_timestamp;
int use_timestamp;
double time_interval;
int process;

assign process to "hcscaler0.S4";
assign hcscaler[0] to "hcscaler0.S1";
assign hcscaler[1] to "hcscaler0.S2";
assign hcscaler[2] to "hcscaler0.S3";
assign hcscaler[3] to "hcscaler0.S4";
assign hcscaler[4] to "hcscaler0.S5";
assign hcscaler[5] to "hcscaler0.S6";
assign hcscaler[6] to "hcscaler0.S7";
assign hcscaler[7] to "hcscaler0.S8";
assign hcscaler[8] to "hcscaler0.S9";
assign hcscaler[9] to "hcscaler0.S10";
assign hcscaler[10] to "hcscaler0.S11";
assign hcscaler[11] to "hcscaler0.S12";
assign hcscaler[12] to "hcscaler0.S13";
assign hcscaler[13] to "hcscaler0.S14";
assign hcscaler[14] to "hcscaler0.S15";
assign hcscaler[15] to "hcscaler0.S16";

assign rate[0] to "hcrate1";
assign rate[1] to "hcrate2";
assign rate[2] to "hcrate3";
assign rate[3] to "hcrate4";
assign rate[4] to "hcrate5";
assign rate[5] to "hcrate6";
assign rate[6] to "hcrate7";
assign rate[7] to "hcrate8";
assign rate[8] to "hcrate9";
assign rate[9] to "hcrate10";
assign rate[10] to "hcrate11";
assign rate[11] to "hcrate12";
assign rate[12] to "hcrate13";
assign rate[13] to "hcrate14";
assign rate[14] to "hcrate15";
assign rate[15] to "hcrate16";

/* assign hc_update_period to "hcscaler_update_period";*/
double clockrate;

evflag ef_clock;
sync process ef_clock;
monitor process;

int skipcount;
int clock_last;
int clock_dif;
int good_count;
int i;
int initted=0;


ss hcmon {
  state init {
    when (efTestAndClear(ef_clock)) {
      clockrate = CLOCKRATE;
      for(i=0;i<16;i++) {
	pvGet(hcscaler[i]);
	hcscaler_last[i] = hcscaler[i];
	rate[i] = 0;
	/*	pvPut(rate[i]);*/
	hc_update_period=1;
	/*	pvPut(hc_update_period);*/
      }
      clock_last = hcscaler[CLOCKCHAN];
      last_timestamp = pvTimeStamp(hcscaler[0]);
      use_timestamp = 1; /* Use two to never try scaler clock */
      good_count = 0;
      initted = 0;
    } state calculate
  }
  state calculate {
    when (efTestAndClear(ef_clock) && skipcount++ >= hc_update_period) {
      skipcount = 1;
      for(i=0;i<16;i++) {
	pvGet(hcscaler[i]);
      }
      if(use_timestamp>0) {
	timestamp = pvTimeStamp(hcscaler[0]);
	time_interval = timestamp.secPastEpoch-last_timestamp.secPastEpoch+
	      (timestamp.nsec/1000000000.0
	       -last_timestamp.nsec/10000000.0);
	if(time_interval > 0.0) {
	  for(i=0;i<16;i++) {
	    rate[i] = scaldif(hcscaler[i],hcscaler_last[i]);
	  }
	} else {
	  for(i=0;i<16;i++) {
	    rate[i] = 0;
	  }
	}
	if(use_timestamp<2) {
	  if(fabs(rate[CLOCKCHAN]/clockrate-1.0) <= TOLERANCE) {
	    good_count++;
	    if(good_count >= NTIMES) {
	      use_timestamp = 0;
	      printf("Switching to scaler channel %d as %d Hz clock\n",
		     CLOCKCHAN, CLOCKRATE);
	    }
	  } else {
	    good_count = 0;
	  }
	}
      } else {
	clock_dif = scaldif(hcscaler[CLOCKCHAN],clock_last);
	if(clock_dif > 0) {
	  for(i=0;i<16;i++) {
	    rate[i] = (scaldif(hcscaler[i],hcscaler_last[i])/(clock_dif/clockrate));
	  }
	} else {
	  for(i=0;i<16;i++) {
	    rate[i] = 0;
	  }
	}
	clock_last = hcscaler[CLOCKCHAN];
      }
      for(i=0;i<16;i++) {
	hcscaler_last[i]=hcscaler[i];
      }
      if(initted) {
        for(i=0;i<16;i++) {
	pvPut(rate[i]);	       
	}
      } else {
	initted=1;
      }
      /*      pvGet(hc_update_period);
      if(hc_update_period > 60) {
	hc_update_period = 60;
	pvPut(hc_update_period);
	}*/
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
