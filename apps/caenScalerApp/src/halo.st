/* halomon

   G9 Halo rates

   Stephen Wood, February 14, 2000
*/

program halomon

option -r;
option +c;

int clock;
int halo;

int clock_last;
int halo_last;

int clock_dif;

int halo_update_period;

assign clock to "hcscaler.S1";
assign halo to "hcscaler.S16";
assign halo_update_period to "halo_update_period";

double rhalo;

assign rhalo to "g0_pivot_halo";

evflag ef_clock;
sync clock ef_clock;

int skipcount;

monitor clock;

ss halomon {
  state init {
    when (efTestAndClear(ef_clock)) {
      pvGet(halo);
      clock_last = clock;
      halo_last = halo;
      rhalo = 0.0;
      pvPut(rhalo);
      skipcount = 1;
      halo_update_period = 2;
      pvPut(halo_update_period);
    } state calculate
  }
  state calculate {
    when (efTestAndClear(ef_clock) && skipcount++ >= halo_update_period/2) {
      skipcount = 1;
      pvGet(halo);
      clock_dif = scaldif(clock,clock_last);
      if(clock_dif > 0) {
	rhalo = (scaldif(halo,halo_last)/(clock_dif/1000000.0));
      } else {
	rhalo = 0.0;
      }
/*	printf("%f %d %d %d\n",rhalo, halo, halo_last, clock_dif);*/
      clock_last = clock;
      halo_last = halo;
      pvPut(rhalo);
      pvGet(halo_update_period);
      if(halo_update_period > 60) {
	halo_update_period = 60;
	pvPut(halo_update_period);
      }
    } state calculate
  }
}
