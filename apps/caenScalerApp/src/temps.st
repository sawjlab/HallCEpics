/* temps
   Measure temperatures of the exit window cooling and doghouse
*/

program temps

option -r;
option +c;

int channel[13];

assign channel to {"qw:exitwindowsupply.AOFF", "qw:exitwindowreturn.AOFF", "qw:doghouse.AOFF"
    ,"qw:bellowstemp1.AOFF","qw:bellowstemp2.AOFF","qw:bellowstemp3.AOFF","qw:bellowstemp4.AOFF"
    ,"qw:bellowstemp5.AOFF","qw:bellowstemp6.AOFF","qw:bellowstemp7.AOFF","qw:bellowstemp8.AOFF"
    ,"qw:tungstenplugsupply.AOFF","qw:tungstenplugreturn.AOFF"};

double value[13];
assign value to {"qw:exitwindowsupply", "qw:exitwindowreturn", "qw:doghouse"
    ,"qw:bellowstemp1","qw:bellowstemp2","qw:bellowstemp3","qw:bellowstemp4"
    ,"qw:bellowstemp5","qw:bellowstemp6","qw:bellowstemp7","qw:bellowstemp8"
    ,"qw:tungstenplugsupply","qw:tungstenplugreturn"};

int nchannel=13;
int i;

%{
  void qwtemp_initialize();
  void qwtemp_getvalues(int nchannel, int *channel, double *value);
}%

ss sstemps {
  state init {
    when() {
      printf("%d %d %d %d %d %d %d %d %d %d %d\n",channel[0], channel[1],channel[2]
	     ,channel[3],channel[4],channel[5],channel[6]
	     ,channel[7],channel[8],channel[9],channel[10],channel[11]
	     ,channel[12]);
      qwtemp_initialize();
      for(i=0;i<nchannel;i++) {
	pvGet(channel[i]);
      }
    } state run
  }
  state run {
    when(delay(0.1)) {
      qwtemp_getvalues(nchannel, channel, value);
      for(i=0;i<nchannel;i++) {
	pvPut(value[i]);
      }
    } state run
  }
}
