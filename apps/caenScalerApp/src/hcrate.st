/* halomon

   Hall C rates

   Stephen Wood, July 22, 2005
*/

program hcrate

option -r;
option +c;

int clock;

double rate[16];
int hcscaler[16];
int hcscaler_last[16];

int hc_update_period;

assign hcscaler[0] to "hcscaler.S1";
assign hcscaler[1] to "hcscaler.S2";
assign hcscaler[2] to "hcscaler.S3";
assign hcscaler[3] to "hcscaler.S4";
assign hcscaler[4] to "hcscaler.S5";
assign hcscaler[5] to "hcscaler.S6";
assign hcscaler[6] to "hcscaler.S7";
assign hcscaler[7] to "hcscaler.S8";
assign hcscaler[8] to "hcscaler.S9";
assign hcscaler[9] to "hcscaler.S10";
assign hcscaler[10] to "hcscaler.S11";
assign hcscaler[11] to "hcscaler.S12";
assign hcscaler[12] to "hcscaler.S13";
assign hcscaler[13] to "hcscaler.S14";
assign hcscaler[14] to "hcscaler.S15";
assign hcscaler[15] to "hcscaler.S16";

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

assign clock to "hcscaler.S1";  /* 1 MhZ */
assign hc_update_period to "hcscaler_update_period";


evflag ef_clock;
sync clock ef_clock;

int skipcount;
int clock_last;
int clock_dif;
int i;

monitor clock;

ss hcmon {
  state init {
    when (efTestAndClear(ef_clock)) {
      clock_last = clock;
      for(i=0;i<16;i++) {
	pvGet(hcscaler[i]);
	hcscaler_last[i] = hcscaler[i];
	rate[i] = 0;
	pvPut(rate[i]);
	hc_update_period=1;
	pvPut(hc_update_period);
      }
    } state calculate
  }
  state calculate {
    when (efTestAndClear(ef_clock) && skipcount++ >= hc_update_period) {
      skipcount = 1;
      for(i=0;i<16;i++) {
	pvGet(hcscaler[i]);
      }
      clock_dif = scaldif(clock,clock_last);
      if(clock_dif > 0) {
	for(i=0;i<16;i++) {
	  rate[i] = (scaldif(hcscaler[i],hcscaler_last[i])/(clock_dif/1000000.0));
	}
      } else {
	for(i=0;i<16;i++) {
	  rate[i] = 0;
	}
      }
/* printf("%f %d %d %d\n",rhalo, halo, halo_last, clock_dif); */
      clock_last = clock;
      for(i=0;i<16;i++) {
	hcscaler_last[i]=hcscaler[i];
	pvPut(rate[i]);
      }
      pvGet(hc_update_period);
      if(hc_update_period > 60) {
	hc_update_period = 60;
	pvPut(hc_update_period);
      }
    } state calculate
  }
}
