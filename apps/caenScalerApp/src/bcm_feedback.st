/* bcm_feedback
   Handle asymmetry calculation averaging and signals for communication
   with MCC.

   Stephen Wood, September 25, 2001

   Take an average for N samples with beam above some threshold.  Beam
   must have been above the threshold for one sample before and one sample
   after the N samples.

   HC:bcm_select will select which bcm to use.
   HC:bcm_threshold will be a threshold (in whatever units ibcm1/2/3 are)
	we use to determine if the beam is on.

   HC:Q_ONOFF if 0, don't increment STAMP, but keep computing asymmetries
   HC:Q_STAMP increment whenever a new asymetry is found.

   HC:q_samples   Number of samples to take before computing an asymmetry

*/

program bcm_feedback

%%#include <math.h>
%%#include <time.h>

option -r;
option -c;

/* EPICS variables */
double q_asy;
double dq_asy;
int q_onoff;
int q_stamp;
string q_stamp_asc;
double rwp_slope;
double rwp_slope_in;
double rwp_slope_out;
int bcm_select;
double bcm_threshold;
int period;
double q_asy1;
double dq_asy1;
double q_asy2;
double dq_asy2;

double ibcm1;
double ibcm2;
double current_asy1;
double current_asy2;

int half_wave_plate;

assign q_asy to "HC:Q_ASY";
assign dq_asy to "HC:DQ_ASY";
assign q_asy1 to "HC:q_asy1";
assign dq_asy1 to "HC:dq_asy1";
assign q_asy2 to "HC:q_asy2";
assign dq_asy2 to "HC:dq_asy2";
assign q_onoff to "HC:Q_ONOFF";
assign q_stamp to "HC:Q_STAMP";
assign q_stamp_asc to "HC:Q_STAMP_ASC";
assign rwp_slope to "HC:RWP_SLOPE";
assign rwp_slope_in to "HC:RWP_SLOPE_IN";
assign rwp_slope_out to "HC:RWP_SLOPE_OUT";
assign bcm_select to "HC:bcm_select";
assign bcm_threshold to "HC:bcm_threshold";
assign period to "HC:qint_period";

assign ibcm1 to "ibcm1";
assign ibcm2 to "ibcm2";
assign current_asy1 to "current_asy1";
assign current_asy2 to "current_asy2";
assign half_wave_plate to "IGL1I00DI24_24M";
/*assign half_wave_plate to "hallc:bcm2";*/

monitor current_asy1;
monitor current_asy2;
monitor ibcm2;
monitor period;
evflag ef_asy1;
sync current_asy1 ef_asy1;
evflag ef_asy2;
sync current_asy2 ef_asy2;
evflag ef_period;
sync period ef_period;
evflag ef_ibcm2;
sync ibcm2 ef_ibcm2;

monitor bcm_threshold;
evflag ef_bcm_threshold;
sync bcm_threshold ef_bcm_threshold;
monitor bcm_select;
evflag ef_bcm_select;
sync bcm_select ef_bcm_select;

monitor half_wave_plate;
evflag ef_half_wave_plate;
sync half_wave_plate ef_half_wave_plate;

monitor rwp_slope;
evflag ef_rwp_slope;
sync rwp_slope ef_rwp_slope;

/* Local variables */
double samples[1000];
int sample_count;
int sample_max;
int count_above_threshold;
double asy1_sum;
double asy2_sum;
double asy1_sum2;
double asy2_sum2;

ss bcm_feedback {
  state init {
    when () {
	period = 120;
	pvPut(period);
	sample_max = period/2;
	q_stamp = 0;
	pvPut(q_stamp);
	q_onoff = 0;
	pvPut(q_onoff);
/* Set some defaults for things now.  Will later read a file on startup */
	bcm_select = 2;
	pvPut(bcm_select);
	bcm_threshold = 30;
	pvPut(bcm_threshold);

	rwp_slope_in = 0;
	rwp_slope_out = 0;
	pvPut(rwp_slope_in);
	pvPut(rwp_slope_out);
    } state reset
  }
  state reset {
    when () {
      sample_count = 0;
      asy1_sum = asy1_sum2 = asy2_sum = asy2_sum2 = 0;
      count_above_threshold = 0;
    } state wait_for_steady_beam
  }
  state half_wave_plate_change {
    when() {
	if(half_wave_plate == 0) { /* IN */
	    rwp_slope = rwp_slope_in;
	} else {	    
	    rwp_slope = rwp_slope_out;
	}
	pvPut(rwp_slope);
    } state reset
  }
  state copy_new_rwp_slope {
    when () {
	if(half_wave_plate == 0) { /* IN */
	    rwp_slope_in = rwp_slope;
	    pvPut(rwp_slope_in);
	} else {	    
	    rwp_slope_out = rwp_slope;
	    pvPut(rwp_slope_out);
	}
	pvPut(rwp_slope);
    } state reset
  }
  state wait_for_steady_beam {
    when(efTestAndClear(ef_half_wave_plate)) {
	printf("Half Wave Plate=%d\n",half_wave_plate);
    } state half_wave_plate_change
    when(efTestAndClear(ef_rwp_slope)) {
	printf("New RWP_SLOPE\n");
    } state copy_new_rwp_slope
    when(count_above_threshold >= 3) {
	printf("Starting to integrate\n");
    } state calculate
    when(efTestAndClear(ef_period)) {
	sample_max = period/2;
    } state wait_for_steady_beam
    when(efTestAndClear(ef_ibcm2)) {
        if(ibcm2 > bcm_threshold) {
	    count_above_threshold++;
	} else {
	    count_above_threshold = 0;
        }
    } state wait_for_steady_beam
  }
  state calculate {
    when(efTestAndClear(ef_half_wave_plate)) {
	printf("Half Wave Plate=%d\n",half_wave_plate);
    } state half_wave_plate_change
    when(efTestAndClear(ef_rwp_slope)) {
	printf("New RWP_SLOPE\n");
    } state copy_new_rwp_slope
    when(ibcm2 < bcm_threshold) {
	printf("%f below threshold of %f\n",ibcm2,bcm_threshold);
    } state reset
    when(efTestAndClear(ef_period)) {
	sample_max = period/2;
    } state reset
    when (efTest(ef_asy1)&&efTest(ef_asy2)) {
      efClear(ef_asy1);
      efClear(ef_asy2);
      asy1_sum += current_asy1;
      asy1_sum2 += current_asy1*current_asy1;
      asy2_sum += current_asy2;
      asy2_sum2 += current_asy2*current_asy2;
      sample_count++;
/*      printf("%d: %f %f %f\n",sample_count,current_asy1,
		asy1_sum,asy1_sum2);*/
      if(sample_count >= sample_max) {
	q_asy1 = asy1_sum / sample_count;
	dq_asy1 = sqrt((asy1_sum2 - sample_count*q_asy1*q_asy1)
			/(sample_count*(sample_count-1)));
/*	printf("%f %f %f %f\n",asy1_sum,asy1_sum2,q_asy1, dq_asy1);*/
	q_asy2 = asy2_sum / sample_count;
	dq_asy2 = sqrt((asy2_sum2 - sample_count*q_asy2*q_asy2)
			/(sample_count*(sample_count-1)));
/* Convert to ppm */
	q_asy1 *= 1000000;
	dq_asy1 *= 1000000;
	q_asy2 *= 1000000;
	dq_asy2 *= 1000000;

	if(bcm_select <= 1) {
	    q_asy = q_asy1;
	    dq_asy = dq_asy1;
	} else {
	    q_asy = q_asy2;
	    dq_asy = dq_asy2;
	}
	q_stamp = getunixtime(q_stamp_asc);
	q_stamp += 1;
	pvPut(q_asy);
	pvPut(dq_asy);
	pvPut(q_asy1);
	pvPut(dq_asy1);
	pvPut(q_asy2);
	pvPut(dq_asy2);

	pvPut(q_stamp);
	pvPut(q_stamp_asc);
/* Reset counters */

	asy1_sum = asy1_sum2 = asy2_sum = asy2_sum2 = 0;
	sample_count = 0;
        pvGet(bcm_threshold);
      }
    } state calculate
  }
}
%{
#define TS_EPOCH_SEC_PAST_1970 7305*86400 /* 1/1/90 20 yr (5 leap) of seconds */
  unsigned long getunixtime(char *q_stamp_asc)
  {
    TS_STAMP now;
    char nowText[32];
    unsigned long unixtime;

    (void) tsLocalTime(&now);
/* Convert from EPICS EPOCH to UNIX EPOCH */
    unixtime = now.secPastEpoch + TS_EPOCH_SEC_PAST_1970;
    tsStampToText(&now, TS_TEXT_MONDDYYYY, q_stamp_asc);
    return(unixtime);
  }
}%

