#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ioLib.h>
#include <semLib.h>

int qwtemp_fd=-1;
int qwtemp_ttynum=1;
int QWTEMP_DEBUG=0;
#define NTCCHANS 12

void qwtemp_initialize()
{
  char RS232_name[20];
  int status;
  
  if(qwtemp_ttynum>=16) {
    sprintf(RS232_name,"/tyIP/%d",qwtemp_ttynum-16);
  } else {
    sprintf(RS232_name,"/tyCo/%d",qwtemp_ttynum);
  }
  
  qwtemp_fd = open(RS232_name, O_RDWR, 0777);
  
  status = ioctl(qwtemp_fd, FIOBAUDRATE, 9600);
}

void qwtemp_monitor()
{
  /* Test program */
  int channel[NTCCHANS];
  int i;
  double value[NTCCHANS];
  for(i=0;i<NTCCHANS;i++) {
    channel[i]=i+3;		      	
  }
  qwtemp_initialize();
  while(1) {
    qwtemp_getvalues(NTCCHANS,channel,value);
    for(i=0;i<NTCCHANS;i++) {
      printf("%d: %f  ",channel[i],value[i]);
    }
    printf("\n");
    taskDelay(60);
  }
}

void qwtemp_getvalues(int nchannel, int *channel, double *value)
{
  char bufout[200];
  char bufin[200];
  int lenout;
  double val;
  int i;

  for(i=0;i<nchannel;i++) {
    /* Set Units */
#if 0
    sprintf(bufout,"UNIT %d,CENT\n",channel[i]);
    if(QWTEMP_DEBUG) {
      printf("%s",bufout);
    }
    lenout = strlen(bufout);
    write(qwtemp_fd, bufout, lenout);
    qwtempReadReply(qwtemp_fd, bufin);
#endif    
    /* Get Value */
    sprintf(bufout,"UNIT %d,CENT;MEAS? %d\n",channel[i],channel[i]);
    if(QWTEMP_DEBUG) {
      printf("%s",bufout);
    }
    lenout = strlen(bufout);
    write(qwtemp_fd,bufout,lenout);
    qwtempReadReply(qwtemp_fd, bufin);
    
    if(QWTEMP_DEBUG) {
      printf("|%s|\n",bufin);
    }
    val = atof(bufin);
    value[i] = val;
    if(QWTEMP_DEBUG) {
      printf("%.2f  ",val);
    }
  }
}	
 
qwtempReadReply(int fd,char *buff)
{
  int i, wait, nbytes, status;
 
  wait = 12;
  nbytes = 0;
  while(wait-- > 0){ /* Wait up to 3 seconds for return */
    status = ioctl(fd,FIONREAD,(int) &nbytes);
    if(nbytes>0) {
      break;
    }
    taskDelay(sysClkRateGet()/12);
  }
  if(nbytes==0){
    buff[0] = 0;
    /*    fprintf(stdout,"No reply!\n");*/
    return;
  }
   
  read(fd,buff,nbytes);
 
  wait = 4;                     /* Wait up to 1/2 second more */
  while(wait-- > 0 && buff[nbytes-1] != '\n'){
    taskDelay(sysClkRateGet()/12); /* One last chance to get everything */
    status = ioctl(fd,FIONREAD,(int) &i);
    if(i>0) {
      read(fd,&buff[nbytes],i);
      /*printf("Read %d more bytes\n",i);*/
      nbytes += i;
    }
  }
  buff[nbytes] = '\0';
  return;
}	
