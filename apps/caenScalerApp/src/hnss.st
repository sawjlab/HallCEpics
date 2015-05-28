/* hnss

   HNSS hodoscope rates

   Stephen Wood, February 14, 2000
*/

program hnss

option -r;
option +c;

int clock;
int hnss1;
int hnss2;
int hnss3;
int hnss4;
int hnss5;
int hnss6;
int hnss7;
int hnss8;
int hnss9;
int hnss10;
int hnss11;
int hnss12;

int clock_last;
int hnss1_last;
int hnss2_last;
int hnss3_last;
int hnss4_last;
int hnss5_last;
int hnss6_last;
int hnss7_last;
int hnss8_last;
int hnss9_last;
int hnss10_last;
int hnss11_last;
int hnss12_last;

int clock_dif;
int hnss1_dif;
int hnss2_dif;
int hnss3_dif;
int hnss4_dif;
int hnss5_dif;
int hnss6_dif;
int hnss7_dif;
int hnss8_dif;
int hnss9_dif;
int hnss10_dif;
int hnss11_dif;
int hnss12_dif;

int hnss_update_period;

assign clock to "scaler.S1";
assign hnss1 to "scaler.S9";
assign hnss2 to "scaler.S10";
assign hnss3 to "scaler.S11";
assign hnss4 to "scaler.S12";
assign hnss5 to "scaler.S13";
assign hnss6 to "scaler.S14";
assign hnss7 to "scaler.S15";
assign hnss8 to "scaler.S16";
assign hnss9 to "scaler.S5";
assign hnss10 to "scaler.S6";
assign hnss11 to "scaler.S7";
assign hnss12 to "scaler.S8";
assign hnss_update_period to "blm_update_period";

double rhnss1;
double rhnss2;
double rhnss3;
double rhnss4;
double rhnss5;
double rhnss6;
double rhnss7;
double rhnss8;
double rhnss9;
double rhnss10;
double rhnss11;
double rhnss12;

assign rhnss1 to "rhnss1";
assign rhnss2 to "rhnss2";
assign rhnss3 to "rhnss3";
assign rhnss4 to "rhnss4";
assign rhnss5 to "rhnss5";
assign rhnss6 to "rhnss6";
assign rhnss7 to "rhnss7";
assign rhnss8 to "rhnss8";
assign rhnss9 to "rhnss9";
assign rhnss10 to "rhnss10";
assign rhnss11 to "rhnss11";
assign rhnss12 to "rhnss12";


evflag ef_clock;
sync clock ef_clock;

int skipcount;

monitor clock;

ss hnss {
  state init {
    when (efTestAndClear(ef_clock)) {
      pvGet(hnss1);
      pvGet(hnss2);
      pvGet(hnss3);
      pvGet(hnss4);
      pvGet(hnss5);
      pvGet(hnss6);
      pvGet(hnss7);
      pvGet(hnss8);
      pvGet(hnss9);
      pvGet(hnss10);
      pvGet(hnss11);
      pvGet(hnss12);
      clock_last = clock;
      hnss1_last = hnss1;
      hnss2_last = hnss2;
      hnss3_last = hnss3;
      hnss4_last = hnss4;
      hnss5_last = hnss5;
      hnss6_last = hnss6;
      hnss7_last = hnss7;
      hnss8_last = hnss8;
      hnss9_last = hnss9;
      hnss10_last = hnss10;
      hnss11_last = hnss11;
      hnss12_last = hnss12;
      rhnss1 = 0.0;
      rhnss2 = 0.0;
      rhnss3 = 0.0;
      rhnss4 = 0.0;
      rhnss5 = 0.0;
      rhnss6 = 0.0;
      rhnss7 = 0.0;
      rhnss8 = 0.0;
      rhnss9 = 0.0;
      rhnss10 = 0.0;
      rhnss11 = 0.0;
      rhnss12 = 0.0;
      pvPut(rhnss1);
      pvPut(rhnss2);
      pvPut(rhnss3);
      pvPut(rhnss4);
      pvPut(rhnss5);
      pvPut(rhnss6);
      pvPut(rhnss7);
      pvPut(rhnss8);
      pvPut(rhnss9);
      pvPut(rhnss10);
      pvPut(rhnss11);
      pvPut(rhnss12);
      skipcount = 1;
      hnss_update_period = 10;
      pvPut(hnss_update_period);
    } state calculate
  }
  state calculate {
    when (efTestAndClear(ef_clock) && skipcount++ >= hnss_update_period/2) {
      skipcount = 1;
      pvGet(hnss1);
      pvGet(hnss2);
      pvGet(hnss3);
      pvGet(hnss4);
      pvGet(hnss5);
      pvGet(hnss6);
      pvGet(hnss7);
      pvGet(hnss8);
      pvGet(hnss9);
      pvGet(hnss10);
      pvGet(hnss11);
      pvGet(hnss12);
      clock_dif = scaldif(clock,clock_last);
      if(clock_dif > 0) {
	rhnss1 = (scaldif(hnss1,hnss1_last)/(clock_dif/1000000.0))/100000.;
	rhnss2 = (scaldif(hnss2,hnss2_last)/(clock_dif/1000000.0))/100000.;
	rhnss3 = (scaldif(hnss3,hnss3_last)/(clock_dif/1000000.0))/100000.;
	rhnss4 = (scaldif(hnss4,hnss4_last)/(clock_dif/1000000.0))/100000.;
	rhnss5 = (scaldif(hnss5,hnss5_last)/(clock_dif/1000000.0))/100000.;
	rhnss6 = (scaldif(hnss6,hnss6_last)/(clock_dif/1000000.0))/100000.;
	rhnss7 = (scaldif(hnss7,hnss7_last)/(clock_dif/1000000.0))/100000.;
	rhnss8 = (scaldif(hnss8,hnss8_last)/(clock_dif/1000000.0))/100000.;
	rhnss9 = (scaldif(hnss9,hnss9_last)/(clock_dif/1000000.0))/100000.;
	rhnss10 = (scaldif(hnss10,hnss10_last)/(clock_dif/1000000.0))/100000.;
	rhnss11 = (scaldif(hnss11,hnss11_last)/(clock_dif/1000000.0))/100000.;
	rhnss12 = (scaldif(hnss12,hnss12_last)/(clock_dif/1000000.0))/100000.;
      } else {
	rhnss1 = 0.0;
	rhnss2 = 0.0;
	rhnss3 = 0.0;
	rhnss4 = 0.0;
	rhnss5 = 0.0;
	rhnss6 = 0.0;
	rhnss7 = 0.0;
	rhnss8 = 0.0;
	rhnss9 = 0.0;
	rhnss10 = 0.0;
	rhnss11 = 0.0;
	rhnss12 = 0.0;
      }
      clock_last = clock;
      hnss1_last = hnss1;
      hnss2_last = hnss2;
      hnss3_last = hnss3;
      hnss4_last = hnss4;
      hnss5_last = hnss5;
      hnss6_last = hnss6;
      hnss7_last = hnss7;
      hnss8_last = hnss8;
      hnss9_last = hnss9;
      hnss10_last = hnss10;
      hnss11_last = hnss11;
      hnss12_last = hnss12;
      pvPut(rhnss1);
      pvPut(rhnss2);
      pvPut(rhnss3);
      pvPut(rhnss4);
      pvPut(rhnss5);
      pvPut(rhnss6);
      pvPut(rhnss7);
      pvPut(rhnss8);
      pvPut(rhnss9);
      pvPut(rhnss10);
      pvPut(rhnss11);
      pvPut(rhnss12);
      pvGet(hnss_update_period);
      if(hnss_update_period > 60) {
	hnss_update_period = 60;
	pvPut(hnss_update_period);
      }
    } state calculate
  }
}
