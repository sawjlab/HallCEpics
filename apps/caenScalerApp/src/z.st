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

option -r;
option +c;

/* EPICS variables */
double q_asy;
double dq_asy;
int q_onoff;
int q_stamp;
double rwp_slope;
int bcm_select;
double bcm_threshold;
double q_asy1;
double dq_asy1;
double q_asy2;
double dq_asy2;

double ibcm1;
double ibcm2;
double current_asy1;
double current_asy2;

assign q_asy to "HC:Q_ASY";
assign dq_asy to "HC:DQ_ASY";
assign q_asy1 to "HC:q_asy1";
assign dq_asy1 to "HC:dq_asy1";
assign q_asy2 to "HC:q_asy2";
assign dq_asy2 to "HC:dq_asy2";
assign q_onoff to "HC:Q_ONOFF";
assign q_stamp to "HC:Q_STAMP";
assign rwp_slope to "HC:RWP_SLOPE";
assign bcm_select to "HC:bcm_select";
assign bcm_threshold to "HC:bcm_threshold";

assign ibcm1 to "ibcm1";
assign ibcm2 to "ibcm2";
assign current_asy1 to "current_asy1";
assign current_asy2 to "current_asy2";

monitor current_asy1;
monitor current_asy2;
monitor ibcm1;
evflag ef_asy1;
sync current_asy1 ef_asy1;
evflag ef_asy2;
sync current_asy2 ef_asy2;

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
    when (efTest(ef_asy1)||efTest(ef_asy2)) {
	efClear(ef_asy1);
	efClear(ef_asy2);
	sample_max = 60;
	q_stamp = 0;
	pvPut(q_stamp);
	q_onoff = 0;
	pvPut(q_onoff);
/* Set some defaults for things now.  Will later read a file on startup */
	bcm_select = 1;
	pvPut(bcm_select);
	bcm_threshold = 20;
	pvPut(bcm_threshold);
    } state reset
  }
  state reset {
    when () {
      sample_count = 0;
      asy1_sum = asy1_sum2 = asy2_sum = asy2_sum2 = 0;
      count_above_threshold = 0;
    } state wait_for_steady_beam
  }
  state wait_for_steady_beam
    when(count_above_threshold >= 3) {
	printf("Starting to calculate\n");
    } state calculate
    when(ibcm1 >= bcm_threshold) {
      count_above_threshold++;
    } state wait_for_steady_beam
    when(ibcm1 < bcm_threshold) {
    } state reset
  }
  state calculate {
    when(ibcm1 < bcm_threshold) {
    } state reset
    when (efTest(ef_asy1)||efTest(ef_asy2)) {
      efClear(ef_asy1);
      efClear(ef_asy2);
      asy1_sum += current_asy1;
      asy1_sum2 += current_asy1*current_asy1;
      asy2_sum += current_asy2;
      asy2_sum2 += current_asy2*current_asy2;
      sample_count++;
	printf("%d: %f %f %f\n",sample_count,current_asy1,
		asy1_sum,asy1_sum2);
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
	q_stamp += 1;
	pvPut(q_asy);
	pvPut(dq_asy);
	pvPut(q_asy1);
	pvPut(dq_asy1);
	pvPut(q_asy2);
	pvPut(dq_asy2);

	pvPut(q_stamp);

/* Reset counters */

	asy1_sum = asy1_sum2 = asy2_sum = asy2_sum2 = 0;
	sample_count = 0;
      }
    } state calculate
  }
}
