/* pcodescan.c
   ============
   Author: J.Spaleta & R.J.Barnes
*/

/*
 ${license}
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>
#include "rtypes.h"
#include "option.h"
#include "rtime.h"
#include "dmap.h"
#include "limit.h"
#include "radar.h"
#include "rprm.h"
#include "iq.h"
#include "rawdata.h"
#include "fitblk.h"
#include "fitdata.h"
#include "fitacf.h"


#include "errlog.h"
#include "freq.h"
#include "tcpipmsg.h"

#include "rmsg.h"
#include "rmsgsnd.h"

#include "radarshell.h"

#include "build.h"
#include "global.h"
#include "reopen.h"
#include "setup.h"
#include "sync.h"

#include "site.h"
#include "sitebuild.h"
#include "siteglobal.h"

char *ststr=NULL;
char *libstr=NULL;
int  channel=0;

void *tmpbuf;
size_t tmpsze;



char progid[80]={"pcodecamp_bm3_15km"};
char progname[256];

int arg=0;
struct OptionData opt;

char *roshost=NULL;
char *droshost={"127.0.0.1"};

int tnum=4;      
struct TCPIPMsgHost task[4]={
  {"127.0.0.1",1,-1}, /* iqwrite */
  {"127.0.0.1",2,-1}, /* raw acfwrite */
  {"127.0.0.1",3,-1}, /* fit acf write */
  {"127.0.0.1",4,-1} /* rt server */
};


int latest_snd_data() {
  FILE    *latest_snd;
  char    data_path[100], data_filename[50], filename[80];
  char    *env_var;
  int64_t temp64;
  int32_t temp32;
  float   tempf;
  int64_t otime; 
  int     diff_secs,max_diff_secs; 
  struct  timeval current_time;
  int     freq_array_length=0,retval=0;
  int32_t *sound_freqs=NULL;
  float   **iscat_percent=NULL;
  float   max_scatter=-10;
  int     max_freq_bin=-1;
  int     i,j;
  int     max_freqs=0,max_beams=0;
  int     read_count=0,read_err=0;

  max_diff_secs=60*30;    
  gettimeofday(&current_time,NULL);
  otime=current_time.tv_sec-max_diff_secs;
  env_var=getenv("SD_SND_PATH");
  if( env_var==NULL )
    sprintf( data_path,"/data/ros/snd/");
  else
    snprintf(data_path,strlen(env_var)+10,"%s/", env_var);
  sprintf( data_filename, "latest_snd_data.%s", ststr);
  sprintf( filename, "%s%s.snd", data_path, data_filename);
  latest_snd= fopen(filename,"r");
  fprintf(stderr,"Read filename: %s %p\n",filename,latest_snd);
  if( latest_snd!=NULL ) {
    read_err=0;
    temp64=0;
    read_count=fread(&temp64,sizeof(int64_t),1,latest_snd);
    if(read_count!=1) read_err=1;
    otime=temp64;
    temp32=0;
    read_count=fread(&temp32,sizeof(int32_t),1,latest_snd);
    if(read_count!=1) read_err=1;
    freq_array_length=temp32;
    read_count=fread(&temp32,sizeof(int32_t),1,latest_snd);
    if(read_count!=1) read_err=1;
    max_freqs=temp32;
    read_count=fread(&temp32,sizeof(int32_t),1,latest_snd);
    if(read_count!=1) read_err=1;
    max_beams=temp32;
    if (read_err==0) {
      iscat_percent=(float**) malloc(max_freqs*sizeof(float*));
      if (iscat_percent!=NULL) {
        for (i=0; i < max_freqs;i++) {
          iscat_percent[i]=NULL;
          iscat_percent[i]= (float*) malloc(max_beams*sizeof(float));
        }
      }
      sound_freqs=malloc(freq_array_length*sizeof(int32_t));
      read_count=fread(sound_freqs,sizeof(int32_t),freq_array_length,latest_snd);
      if(read_count!=freq_array_length) read_err=1;
      for (i=0; i < max_freqs;i++) {
        for (j=0; j < max_beams;j++) {
          if(read_err==0) {
            read_count=fread(&tempf,sizeof(float),1,latest_snd);
            if(read_count!=1) read_err=1;
            else iscat_percent[i][j]=tempf;
          }
        }
      }
    }
    fclose(latest_snd);
  } else {
    fprintf(stderr,"Latest SND data: Open Error\n");
    return -1;
  }
  if (read_err) {
    fprintf(stderr,"Latest SND data: Read Error\n");
    return -1;
  }
  diff_secs=current_time.tv_sec-otime;
  if(diff_secs < max_diff_secs )
  {
    max_scatter=-10;
    max_freq_bin=-1;
    for( i=0; i< freq_array_length; i++ ) {
      if( iscat_percent[i][bmnum] >= max_scatter ) {
        max_scatter= iscat_percent[ i][bmnum];
        max_freq_bin=i;
      }
    }
    if(max_freq_bin >=0) {
      retval=sound_freqs[max_freq_bin];
    } else {
      fprintf(stderr,"SND Data Error\n");
      retval=-1;
    }
  } else {
    fprintf(stderr,"SND Data is too old to use\n");
    retval=-1;
  } 
  if(sound_freqs!=NULL) free(sound_freqs);
  if(iscat_percent!=NULL) {
    for (i=0; i < max_freqs;i++) {
      if(iscat_percent[i]!=NULL) {
        free(iscat_percent[i]);
      }
    } 
    free(iscat_percent);
  }
  return retval;
}


int main(int argc,char *argv[]) {

  int ptab[8] = {0,14,22,24,27,31,42,43};
  int *bcode;
  int bcode2[2]={1,-1};
  int bcode3[3]={1,1,-1};
  int bcode4[4]={1,1,-1,1};
  int bcode5[5]={1,1,1,-1,1};
  int bcode7[7]={1,1,1,-1,-1,1,-1};
  int bcode11[11]={1,1,1,-1,-1,-1,1,-1,-1,1,-1};
  int bcode13[13]={1,1,1,1,1,-1,-1,1,1,-1,1,-1,1};

  int lags[LAG_SIZE][2] = {
    { 0, 0},		/*  0 */
    {42,43},		/*  1 */
    {22,24},		/*  2 */
    {24,27},		/*  3 */
    {27,31},		/*  4 */
    {22,27},		/*  5 */

    {24,31},		/*  7 */
    {14,22},		/*  8 */
    {22,31},		/*  9 */
    {14,24},		/* 10 */
    {31,42},		/* 11 */
    {31,43},		/* 12 */
    {14,27},		/* 13 */
    { 0,14},		/* 14 */
    {27,42},		/* 15 */
    {27,43},		/* 16 */
    {14,31},		/* 17 */
    {24,42},		/* 18 */
    {24,43},		/* 19 */
    {22,42},		/* 20 */
    {22,43},		/* 21 */
    { 0,22},		/* 22 */

    { 0,24},		/* 24 */

    {43,43}};		/* alternate lag-0  */

    char logtxt[1024];

  int exitpoll=0;
  int scannowait=1; 
  int scnsc=120;
  int scnus=0;
  int elapsed_secs=0,clrskip_secs=10,startup=1;
  int skip;
  int cnt=0;

  unsigned char fast=0;
  unsigned char onesec=0;
  unsigned char discretion=0;

  int status=0,n,i;
  struct timeval t0,t1;

  int beams=0;
  int total_scan_usecs=0;
  int total_integration_usecs=0;
  int fixfrq=-1;
  int snd_freq=-1;

  cp=9150;
  intsc=7;
  intus=0;
  mppul=8;
  mplgs=23;
  mpinc=1500;
  nrang=225;
  rsep=15;
  nbaud=5;

  debug=0;
  /* ========= PROCESS COMMAND LINE ARGUMENTS ============= */

  OptionAdd(&opt,"di",'x',&discretion);

  OptionAdd(&opt,"frang",'i',&frang);
  OptionAdd(&opt,"rsep",'i',&rsep);

  OptionAdd( &opt, "dt", 'i', &day);
  OptionAdd( &opt, "nt", 'i', &night);
  OptionAdd( &opt, "sf", 'i', &stfrq);
  OptionAdd( &opt, "df", 'i', &dfrq);
  OptionAdd( &opt, "nf", 'i', &nfrq);
  OptionAdd( &opt, "fixfrq", 'i', &fixfrq);
  OptionAdd( &opt, "xcf", 'i', &xcnt);
  OptionAdd( &opt, "baud", 'i', &nbaud);
  OptionAdd( &opt, "tau", 'i', &mpinc);
  OptionAdd( &opt, "rangeres", 'i', &rsep);
  OptionAdd( &opt, "ranges", 'i', &nrang);


  OptionAdd(&opt,"ep",'i',&errlog.port);
  OptionAdd(&opt,"sp",'i',&shell.port); 

  OptionAdd(&opt,"bp",'i',&baseport); 

  OptionAdd(&opt,"ros",'t',&roshost);

  OptionAdd(&opt,"stid",'t',&ststr); 
  OptionAdd(&opt,"lib",'t',&libstr); 
  OptionAdd(&opt,"c",'i',&channel); 

 
  OptionAdd(&opt,"fast",'x',&fast);
  OptionAdd(&opt,"onesec",'x',&onesec);
  OptionAdd(&opt,"nowait",'x',&scannowait);

  OptionAdd(&opt,"sb",'i',&sbm);
  OptionAdd(&opt,"eb",'i',&ebm);

  OptionAdd( &opt, "clrskip", 'i', &clrskip_secs);

  arg=OptionProcess(1,argc,argv,&opt,NULL);  

  if (ststr==NULL) ststr = getenv("STSTR");
  if (libstr==NULL) libstr = getenv("LIBSTR");
  if (libstr==NULL) libstr=ststr;

  if (roshost==NULL) roshost=getenv("ROSHOST");
  if (roshost==NULL) roshost=droshost;

  if(nbaud==2) bcode=bcode2;
  if(nbaud==3) bcode=bcode3;
  if(nbaud==4) bcode=bcode4;
  if(nbaud==5) bcode=bcode5;
  if(nbaud==7) bcode=bcode7;
  if(nbaud==11) bcode=bcode11;
  if(nbaud==13) bcode=bcode13;
  pcode=(int *)malloc((size_t)sizeof(int)*mppul*nbaud);
  for(i=0;i<mppul;i++){
    for(n=0;n<nbaud;n++){
      pcode[i*nbaud+n]=bcode[n];
    }
  }
  



  printf("Station String: %s\n",ststr);


  OpsStart(ststr);

  status=SiteBuild(libstr,NULL); /* second argument is version string */

  if (status==-1) {
    fprintf(stderr,"Could not identify station.\n");
    exit(1);
  }

  SiteStart(roshost,ststr);

  arg=OptionProcess(1,argc,argv,&opt,NULL);  

 
  strncpy(combf,progid,80);   
  for (n=0;n<tnum;n++) task[n].port+=baseport;
  if ((errlog.sock=TCPIPMsgOpen(errlog.host,errlog.port))==-1) {    
    fprintf(stderr,"Error connecting to error log.\n Host: %s Port: %d\n",errlog.host,errlog.port);
  }

  if ((shell.sock=TCPIPMsgOpen(shell.host,shell.port))==-1) {    
    fprintf(stderr,"Error connecting to shell.\n");
  }
 
  OpsSetupCommand(argc,argv);
  OpsSetupShell();
   
  RadarShellParse(&rstable,"sbm l ebm l dfrq l nfrq l dfrang l nfrang l dmpinc l nmpinc l frqrng l xcnt l",                        
                  &sbm,&ebm,                              
                  &dfrq,&nfrq,                  
                  &dfrang,&nfrang,                            
                  &dmpinc,&nmpinc,                            
                  &frqrng,&xcnt);      
  ebm=3;
  sbm=3; 
  if(channel>0) cnum=channel; 
  status=SiteSetupRadar();
  gettimeofday(&t0,NULL);
  elapsed_secs=clrskip_secs; 
 
  if (status !=0) {
    ErrLog(errlog.sock,progname,"Error locating hardware.");
    exit (1);
  }


  if (fast) {
     cp=9151;
     scnsc=60;
     scnus=0;
     intsc=3;
     intus=0;
  }
  if (onesec) {
     cp=9152;
     scnsc=17;
     scnus=141000;
     intsc=1;
     intus=0;
  }

  beams=abs(ebm-sbm)+1;
  if(beams > 16) {
    if (scannowait==0) {
      total_scan_usecs=(scnsc-3)*1E6+scnus;
      total_integration_usecs=total_scan_usecs/beams;
      intsc=total_integration_usecs/1E6;
      intus=total_integration_usecs -(intsc*1E6);
    }
  }
  if (discretion) cp= -cp;

  txpl=(nbaud*rsep*20)/3;

  if (onesec) sprintf(progname,"pcodecamp_bm3_15km (onesec)");
  else { 
    if (fast) sprintf(progname,"pcodecamp_bm3_15km (fast)");
    else sprintf(progname,"pcodescan_bm3_15km");
  }
  OpsLogStart(errlog.sock,progname,argc,argv);  

  OpsSetupTask(tnum,task,errlog.sock,progname);
  for (n=0;n<tnum;n++) {
    RMsgSndReset(task[n].sock);
    RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
  }

  
  OpsFitACFStart();

  tsgid=SiteTimeSeq(ptab);

  do {
  
    if (SiteStartScan() !=0) continue;
    
    if (OpsReOpen(2,0,0) !=0) {
      ErrLog(errlog.sock,progname,"Opening new files.");
      for (n=0;n<tnum;n++) {
        RMsgSndClose(task[n].sock);
        RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
      }
    }

    scan=1;
    
    ErrLog(errlog.sock,progname,"Starting scan.");
   
    if (xcnt>0) {
      cnt++;
      if (cnt==xcnt) {
        xcf=1;
        cnt=0;
      } else xcf=0;
    } else xcf=0;

    skip=OpsFindSkip(scnsc,scnus);
    
    if (backward) {
      bmnum=sbm-skip;
      if (bmnum<ebm) bmnum=sbm;
    } else {
      bmnum=sbm+skip;
      if (bmnum>ebm) bmnum=sbm;
    }

    do {

      TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
      
      if (OpsDayNight()==1) {
        stfrq=dfrq;
        /*mpinc=dmpinc;*/
        frang=dfrang;
      } else {
        stfrq=nfrq;
        /*mpinc=nmpinc;*/
        frang=nfrang;
      }        
      if(fixfrq>0) {
        stfrq=fixfrq;
        tfreq=fixfrq;
        noise=0;
      }

      sprintf(logtxt,"Integrating beam:%d intt:%ds.%dus (%d:%d:%d:%d)",bmnum,
                      intsc,intus,hr,mt,sc,us);
      ErrLog(errlog.sock,progname,logtxt);

      ErrLog(errlog.sock,progname,"Starting Integration.");
            
      SiteStartIntt(intsc,intus);
      gettimeofday(&t1,NULL);
      elapsed_secs=t1.tv_sec-t0.tv_sec;
      if(elapsed_secs<0) elapsed_secs=0;
      if((elapsed_secs >= clrskip_secs) || (startup==1)) {
        startup=0; 

        snd_freq=latest_snd_data();  

        if (snd_freq > 0 ) {
          sprintf(logtxt, "Using Latest Sounding Data Optimal stfrq: %d", snd_freq);
          ErrLog(errlog.sock,progname, logtxt);
          stfrq=snd_freq;
        } else {
          sprintf(logtxt, "Bad Sounding Data, using existing stfrq: %d", stfrq);
          ErrLog(errlog.sock,progname, logtxt);

        }
        ErrLog(errlog.sock,progname,"Doing clear frequency search."); 
   
        sprintf(logtxt, "FRQ: %d %d", stfrq, frqrng);
        ErrLog(errlog.sock,progname, logtxt);

            
        printf("FRQ: %d %d", stfrq, frqrng);
        if(fixfrq<0) {
          tfreq=SiteFCLR(stfrq,stfrq+frqrng);
        } 
        t0.tv_sec=t1.tv_sec;
        t0.tv_usec=t1.tv_usec;

      } 
      sprintf(logtxt,"Transmitting on: %d (Noise=%g)",tfreq,noise);
      ErrLog(errlog.sock,progname,logtxt);

    
      nave=SiteIntegrate(lags);   
      if (nave<0) {
        sprintf(logtxt,"Integration error:%d",nave);
        ErrLog(errlog.sock,progname,logtxt); 
        continue;
      }
      sprintf(logtxt,"Number of sequences: %d",nave);
      ErrLog(errlog.sock,progname,logtxt);

      OpsBuildPrm(prm,ptab,lags);

      OpsBuildIQ(iq,&badtr);
      
      OpsBuildRaw(raw);
      
      FitACF(prm,raw,fblk,fit);
      
  
      /* write out data here */
      
      msg.num=0;
      msg.tsize=0;

      tmpbuf=RadarParmFlatten(prm,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,
		 PRM_TYPE,0);

      tmpbuf=IQFlatten(iq,prm->nave,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,IQ_TYPE,0);

      RMsgSndAdd(&msg,sizeof(unsigned int)*2*iq->tbadtr,
                 (unsigned char *) badtr,BADTR_TYPE,0);

      RMsgSndAdd(&msg,strlen(sharedmemory)+1,(unsigned char *) sharedmemory,
                 IQS_TYPE,0);

      tmpbuf=RawFlatten(raw,prm->nrang,prm->mplgs,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,RAW_TYPE,0);

      tmpbuf=FitFlatten(fit,prm->nrang,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,FIT_TYPE,0);


      RMsgSndAdd(&msg,strlen(progname)+1,(unsigned char *) progname,
		 NME_TYPE,0);



      for (n=0;n<tnum;n++) RMsgSndSend(task[n].sock,&msg);

      for (n=0;n<msg.num;n++) {
        if (msg.data[n].type==PRM_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==IQ_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==RAW_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==FIT_TYPE) free(msg.ptr[n]);
      }


      RadarShell(shell.sock,&rstable);

      if (exitpoll !=0) break;
      scan=0;
      if (bmnum==ebm) break;
      if (backward) bmnum--;
      else bmnum++;

    } while (1);

    ErrLog(errlog.sock,progname,"Waiting for scan boundary."); 
  
    if ((exitpoll==0) && (scannowait==0)) SiteEndScan(scnsc,scnus);

  } while (exitpoll==0);
  
  
  for (n=0;n<tnum;n++) RMsgSndClose(task[n].sock);
  

  ErrLog(errlog.sock,progname,"Ending program.");


  SiteExit(0);

  return 0;   
} 
 
