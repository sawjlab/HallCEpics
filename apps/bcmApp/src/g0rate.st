/* halomon

   G0 rates

   Stephen Wood, February 14, 2000
*/

program g0rate

option -r;
option +c;

int clock;

/*
#define v2foffset 204312
#define v2fscale 400000.0

Dave says vactual = vreadout*2.4439-0.018
*/
#define v2foffset 207258.11
#define v2fscale 163672.818

double rate[48];
int g0scaler[48];
int g0scaler_last[48];
double voltage[16];

int g0_update_period;

assign g0scaler[0] to "g0scaler.S1";
assign g0scaler[1] to "g0scaler.S2";
assign g0scaler[2] to "g0scaler.S3";
assign g0scaler[3] to "g0scaler.S4";
assign g0scaler[4] to "g0scaler.S5";
assign g0scaler[5] to "g0scaler.S6";
assign g0scaler[6] to "g0scaler.S7";
assign g0scaler[7] to "g0scaler.S8";
assign g0scaler[8] to "g0scaler.S9";
assign g0scaler[9] to "g0scaler.S10";
assign g0scaler[10] to "g0scaler.S11";
assign g0scaler[11] to "g0scaler.S12";
assign g0scaler[12] to "g0scaler.S13";
assign g0scaler[13] to "g0scaler.S14";
assign g0scaler[14] to "g0scaler.S15";
assign g0scaler[15] to "g0scaler.S16";

assign g0scaler[16] to "g0scaler1.S1";
assign g0scaler[17] to "g0scaler1.S2";
assign g0scaler[18] to "g0scaler1.S3";
assign g0scaler[19] to "g0scaler1.S4";
assign g0scaler[20] to "g0scaler1.S5";
assign g0scaler[21] to "g0scaler1.S6";
assign g0scaler[22] to "g0scaler1.S7";
assign g0scaler[23] to "g0scaler1.S8";
assign g0scaler[24] to "g0scaler1.S9";
assign g0scaler[25] to "g0scaler1.S10";
assign g0scaler[26] to "g0scaler1.S11";
assign g0scaler[27] to "g0scaler1.S12";
assign g0scaler[28] to "g0scaler1.S13";
assign g0scaler[29] to "g0scaler1.S14";
assign g0scaler[30] to "g0scaler1.S15";
assign g0scaler[31] to "g0scaler1.S16";

assign g0scaler[32] to "g0scaler2.S1";
assign g0scaler[33] to "g0scaler2.S2";
assign g0scaler[34] to "g0scaler2.S3";
assign g0scaler[35] to "g0scaler2.S4";
assign g0scaler[36] to "g0scaler2.S5";
assign g0scaler[37] to "g0scaler2.S6";
assign g0scaler[38] to "g0scaler2.S7";
assign g0scaler[39] to "g0scaler2.S8";
assign g0scaler[40] to "g0scaler2.S9";
assign g0scaler[41] to "g0scaler2.S10";
assign g0scaler[42] to "g0scaler2.S11";
assign g0scaler[43] to "g0scaler2.S12";
assign g0scaler[44] to "g0scaler2.S13";
assign g0scaler[45] to "g0scaler2.S14";
assign g0scaler[46] to "g0scaler2.S15";
assign g0scaler[47] to "g0scaler2.S16";

assign rate[0] to "g0rate1";
assign rate[1] to "g0rate2";
assign rate[2] to "g0rate3";
assign rate[3] to "g0rate4";
assign rate[4] to "g0rate5";
assign rate[5] to "g0rate6";
assign rate[6] to "g0rate7";
assign rate[7] to "g0rate8";
assign rate[8] to "g0rate9";
assign rate[9] to "g0rate10";
assign rate[10] to "g0rate11";
assign rate[11] to "g0rate12";
assign rate[12] to "g0rate13";
assign rate[13] to "g0rate14";
assign rate[14] to "g0rate15";
assign rate[15] to "g0rate16";
assign rate[16] to "g0rate17";
assign rate[17] to "g0rate18";
assign rate[18] to "g0rate19";
assign rate[19] to "g0rate20";
assign rate[20] to "g0rate21";
assign rate[21] to "g0rate22";
assign rate[22] to "g0rate23";
assign rate[23] to "g0rate24";
assign rate[24] to "g0rate25";
assign rate[25] to "g0rate26";
assign rate[26] to "g0rate27";
assign rate[27] to "g0rate28";
assign rate[28] to "g0rate29";
assign rate[29] to "g0rate30";
assign rate[30] to "g0rate31";
assign rate[31] to "g0rate32";

assign voltage[0] to "g0voltage1";
assign voltage[1] to "g0voltage2";
assign voltage[2] to "g0voltage3";
assign voltage[3] to "g0voltage4";
assign voltage[4] to "g0voltage5";
assign voltage[5] to "g0voltage6";
assign voltage[6] to "g0voltage7";
assign voltage[7] to "g0voltage8";
assign voltage[8] to "g0voltage9";
assign voltage[9] to "g0voltage10";
assign voltage[10] to "g0voltage11";
assign voltage[11] to "g0voltage12";
assign voltage[12] to "g0voltage13";
assign voltage[13] to "g0voltage14";
assign voltage[14] to "g0voltage15";
assign voltage[15] to "g0voltage16";

assign clock to "g0scaler.S9";  /* 4 MhZ */
assign g0_update_period to "g0scaler_update_period";


evflag ef_clock;
sync clock ef_clock;

int skipcount;
int clock_last;
int clock_dif;
int i;

monitor clock;

ss g0mon {
  state init {
    when (efTestAndClear(ef_clock)) {
      clock_last = clock;
      for(i=0;i<48;i++) {
	pvGet(g0scaler[i]);
	g0scaler_last[i] = g0scaler[i];
	if(i<32) {
	  rate[i] = 0;
	  pvPut(rate[i]);
	}
	g0_update_period=1;
	pvPut(g0_update_period);
      }
    } state calculate
  }
  state calculate {
    when (efTestAndClear(ef_clock) && skipcount++ >= g0_update_period) {
      skipcount = 1;
      for(i=0;i<48;i++) {
	pvGet(g0scaler[i]);
      }
      clock_dif = scaldif(clock,clock_last);
      if(clock_dif > 0) {
	for(i=0;i<48;i++) {
	  rate[i] = (scaldif(g0scaler[i],g0scaler_last[i])/(clock_dif/4000000.0));
	}
      } else {
	for(i=0;i<48;i++) {
	  rate[i] = 0;
	}
      }
/*	printf("%f %d %d %d\n",rhalo, halo, halo_last, clock_dif);*/
      clock_last = clock;
      for(i=0;i<48;i++) {
	g0scaler_last[i]=g0scaler[i];
	if(i<32) {
	  pvPut(rate[i]);
	}
      }
      for(i=0;i<16;i++) {
        voltage[i] = (rate[32+i]-v2foffset)/v2fscale;
        pvPut(voltage[i]);
      }
      pvGet(g0_update_period);
      if(g0_update_period > 60) {
	g0_update_period = 60;
	pvPut(g0_update_period);
      }
    } state calculate
  }
}
