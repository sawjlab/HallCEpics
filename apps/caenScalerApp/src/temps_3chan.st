/* temps
   Measure temperatures of the exit window cooling and doghouse
*/

program temps

option -r;
option +c;

int channel[3];

assign channel to {"qw:exitwindowsupply.AOFF", "qw:exitwindowreturn.AOFF", "qw:doghouse.AOFF"};

double value[3];
assign value to {"qw:exitwindowsupply", "qw:exitwindowreturn", "qw:doghouse"};

int nchannel=3;
int i;

%{
  void qwtemp_initialize();
  void qwtemp_getvalues(int nchannel, int *channel, double *value);
}%

ss sstemps {
  state init {
    when() {
      printf("%d %d %d\n",channel[0], channel[1],channel[2]);
      qwtemp_initialize();
      for(i=0;i<nchannel;i++) {
	pvGet(channel[i]);
      }
    } state run
  }
  state run {
    when(delay(2.0)) {
      qwtemp_getvalues(nchannel, channel, value);
      for(i=0;i<nchannel;i++) {
	pvPut(value[i]);
      }
    } state run
  }
}
