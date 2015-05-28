/* bpm_proj

   Fit a set of BPMs and project to a point on the beamline.
   Based unix/root script of Buddhini Waidyawansa

   2010.11.14 (saw) Initial copy
*/

program bpm_proj

option -r;
option +c;

#define NBPMS 6
double x_bpm[NBPMS];
double y_bpm[NBPMS];

assign x_bpm to {"IPM3H07A.XPOS", "IPM3H07B.XPOS", "IPM3H07C.XPOS", 
    "IPM3H08.XPOS", "IPM3H09.XPOS", "IPM3H09B.XPOS"};
assign y_bpm to {"IPM3H07A.YPOS", "IPM3H07B.YPOS", "IPM3H07C.YPOS", 
    "IPM3H08.YPOS", "IPM3H09.YPOS", "IPM3H09B.YPOS"};

double targetX;
double targetY;
double tungstenX;
double tungstenY;
assign targetX to "qw:targetX";
assign targetY to "qw:targetY";
assign tungstenX to "qw:tungstenX";
assign tungstenY to "qw:tungstenY";

int i;
double weights[NBPMS];
#define ZZERO 138406000
double z_pos[NBPMS]; 

double fit_results[4];

// Qweak target is located at position 148.739 m. This was used as the position of the combined bpm
// Qweak tungsten plug is located at position  148.081 m.
// The location of the w-plug is based from an setimated distance from target center to US face of W-plug
// given by Greg Smith to be 65.8cm.     
#define TGTPOS (148739000.0-ZZERO)
#define TUNGSTENPOS (149397000.0-ZZERO)

monitor x_bpm;
monitor y_bpm;

evflag ev_change;
sync y_bpm[5] ev_change;

ss bpm {
  state init {
    when () {
      for(i=0;i<NBPMS;i++) {
	weights[i] = 1.0;
      }
      z_pos[0] = 138406000-ZZERO;
      z_pos[1] = 139363000-ZZERO;
      z_pos[2] = 140329000-ZZERO;	
      z_pos[3] = 143576000-ZZERO;
      z_pos[4] = 144803000-ZZERO;
      z_pos[5] = 147351000-ZZERO;
    } state gofit
  }
  state gofit {
    when (efTestAndClear(ev_change)) {
      dolsqfit(NBPMS,z_pos,x_bpm,weights,fit_results);
      targetX = fit_results[1]+fit_results[0]*TGTPOS;
      tungstenX = fit_results[1]+fit_results[0]*TUNGSTENPOS;

      dolsqfit(NBPMS,z_pos,y_bpm,weights,fit_results);
      targetY = fit_results[1]+fit_results[0]*TGTPOS;
      tungstenY = fit_results[1]+fit_results[0]*TUNGSTENPOS;
      
      pvPut(targetX);
      pvPut(targetY);
      pvPut(tungstenX);
      pvPut(tungstenY);

    } state gofit
  }
}

%{

  /* Take the difference between two 32 bit scalers */
  void dolsqfit(int npoints, double *x, double *y, double *w, double *results)
  {	
    double a,b,c,d,e,f;
    int i;
    a = b = c = d = e = f = 0.0;
    for(i=0;i<npoints;i++) {
      a+=x[i]*w[i];
      b+=w[i];
      c+=y[i]*w[i];
      d+=x[i]*x[i]*w[i];
      e+=x[i]*y[i]*w[i];
      f+=y[i]*y[i]*w[i];
    }
    results[0] = (e*b - c*a)/(d*b - a*a); // a (slope)
    results[1] = (d*c - e*a)/(d*b - a*a); // b (offset)
    results[2] = b/(d*b - a*a);           // erra
    results[3] = d/(d*b - a*a);           // errb
  }
}%
