/* iwdscan_normalized.c
   ============
   Reluctant Author: J.Klein
   Merger of normalscan 1.10: R.J.Barnes & J.Spaleta
   and iwdscan 1.00: K. Krieger
*/

/*
 LICENSE AND DISCLAIMER
 
 Copyright (c) 2012 The Johns Hopkins University/Applied Physics Laboratory
 
 This file is part of the Radar Software Toolkit (RST).
 
 RST is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.
 
 RST is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with RST.  If not, see <http://www.gnu.org/licenses/>.
 
 
  
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
#include "rosmsg.h"
#include "tsg.h"

char *ststr=NULL;
char *libstr=NULL;

void *tmpbuf;
size_t tmpsze;

char progid[80]={"normalscan"};
char progname[256];

int arg=0;
struct OptionData opt;

char *roshost=NULL;
char *droshost={"127.0.0.1"};

int tnum=4;      
struct TCPIPMsgHost task[4]={
  {"127.0.0.1",1,-1}, /* iqwrite */
  {"127.0.0.1",2,-1}, /* rawacfwrite */
  {"127.0.0.1",3,-1}, /* fitacfwrite */
  {"127.0.0.1",4,-1}  /* rtserver */
};

int main(int argc,char *argv[]) {

  int ptab_7[7] = { 0, 9, 12, 20, 22, 26, 27};
  int lags_7[LAG_SIZE][2] = {
          {0,0},
          {26,27},
          {20,22},
          {9,12},
          {22,26},
          {22,27},
          {20,26},
          {20,27},
          {12,20},
          {0,9},
          {12,22},
          {9,20},
          {0,12},
          {9,22},
          {12,26},
          {12,27},

          {9,26},
          {9,27},

          {27,27}};

  int mppul_7 = 7;
  int mplgs_7 = 18;
  int mpinc_7 = 2400;
  int dmpinc_7 = 2400;
  int nmpinc_7 = 2400;
  int use_marker = 0; /* Default do not use marker */
  int marker_period = 15; /* Default every 15 beams marker period */
  int marker_counter = marker_period; /* Counter for the marker periods */


  int ptab_8[8] = {0,14,22,24,27,31,42,43};
  int lags_8[LAG_SIZE][2] = {
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

  int mppul_8 = 8;
  int mplgs_8 = 23;
  int mpinc_8 = 1500;
  int dmpinc_8 = 1500;
  int nmpinc_8 = 1500;

  char logtxt[1024];

  int exitpoll=0;
  int scannowait=0;
  int onesec=0; 
  int scnsc=10;
  int scnus=0;
  int elapsed_secs=0;
  int default_clrskip_secs=30;
  int clrskip_secs=-1;
  int do_clr_scan_start=0;
  int cpid=0;
  int startup=1;
  struct timeval t0,t1;

  int skip;
  int cnt=0;

  unsigned char fast=0;
  unsigned char discretion=0;

  int status=0,n;

  int beams=0;
  int total_scan_usecs=0;
  int total_integration_usecs=0;
  int fixfrq=-1;
  int temp = 0;

  printf("Size of int %d\n",(int)sizeof(int));
  printf("Size of long %d\n",(int)sizeof(long));
  printf("Size of long long %d\n",(int)sizeof(long long));
  printf("Size of struct TRTimes %d\n",(int)sizeof(struct TRTimes));
  printf("Size of struct SeqPRM %d\n",(int)sizeof(struct SeqPRM));
  printf("Size of struct RosData %d\n",(int)sizeof(struct RosData));
  printf("Size of struct DataPRM %d\n",(int)sizeof(struct DataPRM));
  printf("Size of Struct ControlPRM  %d\n",(int)sizeof(struct ControlPRM));
  printf("Size of Struct RadarPRM  %d\n",(int)sizeof(struct RadarPRM));
  printf("Size of Struct ROSMsg  %d\n",(int)sizeof(struct ROSMsg));
  printf("Size of Struct CLRFreq  %d\n",(int)sizeof(struct CLRFreqPRM));
  printf("Size of Struct TSGprm  %d\n",(int)sizeof(struct TSGprm));
  printf("Size of Struct SiteSettings  %d\n",(int)sizeof(struct SiteSettings));

  cp= 3385;
  intsc=1;
  intus=0;
  mppul= mppul_8;
  mplgs=mplgs_8;
  mpinc=mpinc_8;
  dmpinc=dmpinc_8;
  nrang=75;
  rsep=45;
  txpl=300;

  /* ========= PROCESS COMMAND LINE ARGUMENTS ============= */

  OptionAdd(&opt,"di",'x',&discretion);

  OptionAdd(&opt,"frang",'i',&frang);
  OptionAdd(&opt,"rsep",'i',&rsep);
  OptionAdd(&opt,"nrang",'i',&nrang);

  OptionAdd( &opt, "dt", 'i', &day);
  OptionAdd( &opt, "nt", 'i', &night);
  OptionAdd( &opt, "df", 'i', &dfrq);
  OptionAdd( &opt, "nf", 'i', &nfrq);
  OptionAdd( &opt, "fixfrq", 'i', &fixfrq);
  OptionAdd( &opt, "xcf", 'i', &xcnt);


  OptionAdd(&opt, "use_marker", 'i',&use_marker);
  OptionAdd(&opt, "marker_period",'i',&marker_period);

  OptionAdd(&opt,"ep",'i',&errlog.port);
  OptionAdd(&opt,"sp",'i',&shell.port); 

  OptionAdd(&opt,"bp",'i',&baseport); 

  OptionAdd(&opt,"ros",'t',&roshost);

  OptionAdd(&opt,"stid",'t',&ststr); 
  OptionAdd(&opt,"lib",'t',&libstr); 
 
  OptionAdd(&opt,"fast",'x',&fast);

  OptionAdd( &opt, "nowait", 'x', &scannowait);
  OptionAdd( &opt, "onesec", 'x', &onesec);
  OptionAdd(&opt,"sb",'i',&sbm);
  OptionAdd(&opt,"eb",'i',&ebm);
  OptionAdd(&opt,"c",'i',&cnum);
  OptionAdd( &opt, "clrskip", 'i', &clrskip_secs);
  OptionAdd( &opt, "clrscan", 'x', &do_clr_scan_start);
  OptionAdd( &opt, "cpid", 'i', &cpid);

   
  arg=OptionProcess(1,argc,argv,&opt,NULL);  

  if(use_marker != 0) {
        use_marker = 1;
  }
  if(marker_period < 0) {
       marker_period = 0;
  }
  
  marker_counter = marker_period;
 
  if (ststr==NULL) ststr= getenv("STSTR");
  if (libstr ==NULL ) libstr = getenv("LIBSTR");
  if (libstr == NULL) libstr=ststr;

  if (roshost==NULL) roshost=getenv("ROSHOST");
  if (roshost==NULL) roshost=droshost;



  OpsStart(ststr);

  status=SiteBuild(libstr,NULL); /* second argument is version string */
 
  if (status==-1) {
    fprintf(stderr,"Could not identify station.\n");
    exit(1);
  }
  
  ErrLog(errlog.sock,progname,logtxt);

  SiteStart(roshost,ststr);
  arg=OptionProcess(1,argc,argv,&opt,NULL);  

  printf("Station ID: %s  %d\n",ststr,stid);
  printf("baseport:%d\n",baseport);

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
  
 
  status=SiteSetupRadar();

  if(fixfrq==0) {
    sprintf(progname, "iwdscan (hacked..)");
  } else {
    sprintf(progname, "iwdscan (hacked..) (fixfrq)");
  }


  gettimeofday(&t0,NULL);
  elapsed_secs=clrskip_secs;
  gettimeofday(&t0,NULL);
  gettimeofday(&t1,NULL);


  printf("Initial Setup Complete: Station ID: %s  %d\n",ststr,stid);
  
  if (status !=0) {
    ErrLog(errlog.sock,progname,"Error locating hardware.");
    exit (1);
  }

  beams=abs(ebm-sbm)+1;

  if (discretion) cp= -cp;

  txpl=(rsep*20)/3;

  if (fast) sprintf(progname,"normalscan (fast)");
  else sprintf(progname,"normalscan");



  OpsLogStart(errlog.sock,progname,argc,argv);  

  OpsSetupTask(tnum,task,errlog.sock,progname);

  for (n=0;n<tnum;n++) {
    RMsgSndReset(task[n].sock);
    RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
  }

  printf("Preparing OpsFitACFStart Station ID: %s  %d\n",ststr,stid);
  
  OpsFitACFStart();
  
  printf("Preparing SiteTimeSeq Station ID: %s  %d\n",ststr,stid);


  printf("Entering Scan loop Station ID: %s  %d\n",ststr,stid);
  do {

    printf("Entering Site Start Scan Station ID: %s  %d\n",ststr,stid);
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
    if(do_clr_scan_start) startup=1; 
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
      } else {
        stfrq=nfrq;
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
            
      printf("Entering Site Start Intt Station ID: %s  %d\n",ststr,stid);
      SiteStartIntt(intsc,intus);
      gettimeofday(&t1,NULL);
      elapsed_secs=t1.tv_sec-t0.tv_sec;

      if(elapsed_secs<0) elapsed_secs=0;
      if((elapsed_secs >= clrskip_secs)||(startup==1)) {
        startup=0;
        ErrLog(errlog.sock,progname,"Doing clear frequency search.");

        sprintf(logtxt, "FRQ: %d %d", stfrq, frqrng);
        ErrLog(errlog.sock,progname, logtxt);

        if(fixfrq<0) {
          tfreq=SiteFCLR(stfrq,stfrq+frqrng);
        }
        t0.tv_sec=t1.tv_sec;
        t0.tv_usec=t1.tv_usec;
      } else {
        sprintf(logtxt,"Clear Search Skipped, next search in %d secs",(int)(clrskip_secs-elapsed_secs));
        ErrLog(errlog.sock,progname,logtxt);
      }
      sprintf(logtxt,"Transmitting on: %d (Noise=%g)",tfreq,noise);
      ErrLog(errlog.sock,progname,logtxt);
    
    if(use_marker) {
        ErrLog(errlog.sock,progname, "Using marker pulse sequence");
        if(marker_counter == marker_period) {
            mppul = mppul_7;
            mplgs = mplgs_7;
            mpinc = mpinc_7;
            tsgid = SiteTimeSeq(ptab_7);
            nave = SiteIntegrate(lags_7);
            if(nave < 0) {
                sprintf(logtxt, "Integration error:%d",nave);
                ErrLog(errlog.sock,progname,logtxt);
                exit(nave);
            }
            sprintf(logtxt,"Number of sequences [7]: %d",nave);
            ErrLog(errlog.sock,progname,logtxt);
            OpsBuildPrm(prm,ptab_7,lags_7);
            marker_counter = 0;
        } else {
            mppul = mppul_8;
            mplgs = mplgs_8;
            mpinc = mpinc_8;
            tsgid = SiteTimeSeq(ptab_8);
            nave = SiteIntegrate(lags_8);
            if(nave < 0) {
                sprintf(logtxt, "Integration error:%d",nave);
                ErrLog(errlog.sock,progname,logtxt);
                exit(nave);
            }
            sprintf(logtxt,"Number of sequences [8]: %d",nave);
            ErrLog(errlog.sock,progname,logtxt);
            OpsBuildPrm(prm,ptab_8,lags_8);
            marker_counter += 1;
        }
   } else {
        ErrLog(errlog.sock,progname, "Not using marker pulse sequence");
        tsgid = SiteTimeSeq(ptab_8);
        nave = SiteIntegrate(lags_8);
        if (nave < 0) {
            sprintf(logtxt, "Integration error:%d", nave);
            ErrLog(errlog.sock, progname, logtxt);
            exit(nave);
        }

        sprintf(logtxt, "Number of sequences: %d", nave);
        ErrLog(errlog.sock, progname, logtxt);

        OpsBuildPrm(prm, ptab_8, lags_8);
    }

      OpsBuildIQ(iq,&badtr);
      OpsBuildRaw(raw);

   
      FitACF(prm,raw,fblk,fit);
      
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
 
