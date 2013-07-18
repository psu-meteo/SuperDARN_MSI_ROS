/* normalscan.c
   ============
   Author: R.J.Barnes & J.Spaleta
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
/* TODO:
 *   Redo the logic associated with boilerplate arguments for all tcp 
 *     connections so that we can remove stid,baseport,channel etc... 
 *     from the scheduler commandline. To ease readability, decrease 
 *     fragility of schedule writing and making it possible to ease 
 *     cross-site and mimic radar testing of common modes.
 *
 *     End goal: distribute a common schedule file, generated from the SD
 *       common schedule that works on all radars using the linux/qnx6 
 *       rst/ros system for which the program is designed.
 *       turn this: 
 *         2013 07 31 12 00   normalscan -stid kod -ep 43000 -sp 43001 -bp 43100 -fast -c 3
 *       into this:
 *         2013 07 31 12 00   normalscan -fast  
 *       by making the necessary site operational information discoverable from the running
 *       the environment.
 *
 *     JDS's suggested approach:
 *       Ideally we'd have a way to check environment variables for the
 *       boilerplate arguments at the start of program init. This would be
 *       best done as a new function in the ops library.  If the envvars exist
 *       use those values, else fill values with harmless defaults, then 
 *       reload the variables from commandline arguments if provided.  
 *
 *       At a minimum the stid and channel variables should be tastable from 
 *       the running environment from an Ops* function, which can then be 
 *       processed to to select the correct site library to dlopen. Once you
 *       have the correct site library loaded, specific tcp comm variables
 *       to use can be initialized with a Site* function. Controlprogram flow
 *       will have to be adjusted to make room for correct operation of an
 *       early Site* function. 
 *
 *    Convert existing cmdline option parsing from RST provided functions to
 *      standard GNU long option parsing.
 *
 *      Benefit: better tested option parsing code, which:
 *         1) correctly handles when an unknown argument is used.
 *         2) provides a standard way to provide a --usage and --help message
 *           to discover what options are supported.
 *
 *    Re-engineer Day/Night variable parsing to work with multi-site radars:
 *      Problem statement:
 *        Multi-site radars must use a common TR for both east and west radars
 *        but east and west radars will have different day/night settings in 
 *        their site library. Certain parameters like mpinc cannot be
 *        day/night switched as they impact the TR gate timing.
 *      Goal:
 *        Abstract Day/Night switch logic in this cp such that site operators
 *        can override the cp's request to day/night switch specific parameters.
 *        This way we can continue to add Day/Night parameter logic into the
 *        common controlprogram without disrupting site operations at
 *        multi-site radars. We should be able to day/night switch TR senstive
 *        parameters, we just have to be a little smarter about it.     
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
char *dfststr="tst";

void *tmpbuf;
size_t tmpsze;

char progid[80]={"normalscan"};
char progname[256];

int arg=0;
struct OptionData opt;

char *roshost=NULL;
char *droshost={"127.0.0.1"};

int baseport=44100;

struct TCPIPMsgHost errlog={"127.0.0.1",44100,-1};

struct TCPIPMsgHost shell={"127.0.0.1",44101,-1};

int tnum=4;      
struct TCPIPMsgHost task[4]={
  {"127.0.0.1",1,-1}, /* iqwrite */
  {"127.0.0.1",2,-1}, /* rawacfwrite */
  {"127.0.0.1",3,-1}, /* fitacfwrite */
  {"127.0.0.1",4,-1}  /* rtserver */
};

int main(int argc,char *argv[]) {

  int ptab[8] = {0,14,22,24,27,31,42,43};
  int *bcode=NULL;
  int bcode1[1]={1};
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
  int scannowait=0;
  int onesec=0; 
  int scnsc=120;
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

  int status=0,n,i;

  int beams=0;
  int total_scan_usecs=0;
  int total_integration_usecs=0;
  int fixfrq=-1;

/*
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
*/

  cp=150;
  intsc=7;
  intus=0;
  mppul=8;
  mplgs=23;
  mpinc=1500;
  nrang=75;
  rsep=45;
  txpl=300;
  nbaud=1;

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
  OptionAdd( &opt, "baud", 'i', &nbaud);
  OptionAdd( &opt, "tau", 'i', &mpinc);

  OptionAdd(&opt,"ep",'i',&errlog.port);
  OptionAdd(&opt,"sp",'i',&shell.port); 

  OptionAdd(&opt,"bp",'i',&baseport); 

  OptionAdd(&opt,"ros",'t',&roshost);

  OptionAdd(&opt,"stid",'t',&ststr); 
 
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
 
  if (ststr==NULL) ststr=dfststr;

  if (roshost==NULL) roshost=getenv("ROSHOST");
  if (roshost==NULL) roshost=droshost;

  if ((errlog.sock=TCPIPMsgOpen(errlog.host,errlog.port))==-1) {    
    fprintf(stderr,"Error connecting to error log.\n");
  }

  if ((shell.sock=TCPIPMsgOpen(shell.host,shell.port))==-1) {    
    fprintf(stderr,"Error connecting to shell.\n");
  }

  for (n=0;n<tnum;n++) task[n].port+=baseport;

  OpsStart(ststr);

  status=SiteBuild(ststr,NULL); /* second argument is version string */
 
  if (status==-1) {
    fprintf(stderr,"Could not identify station.\n");
    exit(1);
  }

  SiteStart(roshost);
  arg=OptionProcess(1,argc,argv,&opt,NULL);  

  printf("Station ID: %s  %d\n",ststr,stid);


  strncpy(combf,progid,80);   
 
  OpsSetupCommand(argc,argv);
/*  
 *  FIXME: JDS: RadarShell has a problem with multi-site and multi-channel setups. Disabling for now
*/
/*
  OpsSetupShell();
   
  RadarShellParse(&rstable,"sbm l ebm l dfrq l nfrq l frqrng l xcnt l",                        
                  &sbm,&ebm,                              
                  &dfrq,&nfrq,                  
                  &frqrng,&xcnt);      
*/  
 
  status=SiteSetupRadar();
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
  if (fast) {
    cp=151;
    intsc=3;
    intus=500000;
    scnsc=60;
    scnus=0;
    sprintf(progname,"normalscan (fast)");
  } else {
    scnsc=120;
    scnus=0;
    sprintf(progname,"normalscan");
  }
  if (onesec) {
    cp=152;
    intsc=1;
    intus=0;
    scnsc=beams+4;
    scnus=0;
    sprintf(progname,"normalscan (onesec)");
    scannowait=1;
    if(clrskip_secs < 0) clrskip_secs=default_clrskip_secs;
  }
  if(beams==1) {
    /* Camping Beam */
    sprintf(progname,"normalscan (camp)");
    scannowait=1;
    if(clrskip_secs < 0) clrskip_secs=default_clrskip_secs;
    cp=153;
    sprintf(logtxt,"Normalscan configured for camping beam");
    ErrLog(errlog.sock,progname,logtxt);
    sprintf(logtxt," fast: %d onesec: %d cp: %d clrskip_secs: %d intsc: %d",fast,onesec,cp,clrskip_secs,intsc);
    ErrLog(errlog.sock,progname,logtxt);
  }
  if(beams > 16) {
      if (scannowait==0 && onesec==0) {
        total_scan_usecs=(scnsc-3)*1E6+scnus;
        total_integration_usecs=total_scan_usecs/beams;
        intsc=total_integration_usecs/1E6;
        intus=total_integration_usecs -(intsc*1E6);
      }
  }
  switch(nbaud) {
    case 1:
      bcode=bcode1;
    case 2:
      bcode=bcode2;
      break;
    case 3:
      bcode=bcode3;
      break;
    case 4:
      bcode=bcode4;
      break;
    case 5:
      bcode=bcode5;
      break;
    case 7:
      bcode=bcode7;
      break;
    case 11:
      bcode=bcode11;
      break;
    case 13:
      bcode=bcode13;
      break;
    default:
      ErrLog(errlog.sock,progname,"Error: Unsupported nbaud requested, exiting");
      SiteExit(0);
  }
  pcode=(int *)malloc((size_t)sizeof(int)*mppul*nbaud);
  for(i=0;i<mppul;i++){
    for(n=0;n<nbaud;n++){
      pcode[i*nbaud+n]=bcode[n];
    }
  }


  if(cpid > 0) cp=cpid;

  if (discretion) cp= -cp;

  txpl=(nbaud*rsep*20)/3;

  if ((mpinc % txpl) || (mpinc % 10))  {
    sprintf(logtxt,"Error: mpinc not multiple of txpl... checking to see if it can be adjusted"); 
    ErrLog(errlog.sock,progname,logtxt);
    sprintf(logtxt,"Initial: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep); 
    ErrLog(errlog.sock,progname,logtxt);
    if((txpl % 10)==0) {

      sprintf(logtxt,"Attempting to adjust mpinc to correct"); 
      ErrLog(errlog.sock,progname,logtxt);
      if (mpinc < txpl) mpinc=txpl;
      int minus_remain=mpinc % txpl;
      int plus_remain=txpl -(mpinc % txpl);
      if (plus_remain > minus_remain)
        mpinc = mpinc - minus_remain;
      else
         mpinc = mpinc + plus_remain;
      if (mpinc==0) mpinc = mpinc + plus_remain; 

    }
  } 
  if ((mpinc % txpl) || (mpinc % 10) || (mpinc==0))  {
     sprintf(logtxt,"Error: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep); 
     ErrLog(errlog.sock,progname,logtxt);
     exitpoll=1;
     SiteExit(0);
  }
  sprintf(logtxt,"Adjusted: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep); 
  ErrLog(errlog.sock,progname,logtxt);
  
  OpsLogStart(errlog.sock,progname,argc,argv);  

  OpsSetupTask(tnum,task,errlog.sock,progname);

  for (n=0;n<tnum;n++) {
    RMsgSndReset(task[n].sock);
    RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
  }

  printf("Preparing OpsFitACFStart Station ID: %s  %d\n",ststr,stid);
  
  OpsFitACFStart();
  
  printf("Preparing SiteTimeSeq Station ID: %s  %d\n",ststr,stid);

  tsgid=SiteTimeSeq(ptab);

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

    if(scannowait==0) skip=OpsFindSkip(scnsc,scnus);
    else skip=0;

    if (backward) {
      bmnum=sbm-skip;
      if (bmnum>ebm) bmnum=sbm;
    } else {
      bmnum=sbm+skip;
      if (bmnum>ebm) bmnum=sbm;
    }

    do {
      if (backward) {
        if (bmnum>sbm) bmnum=sbm;
        if (bmnum<ebm) bmnum=ebm;
      } else {
        if (bmnum<sbm) bmnum=sbm;
        if (bmnum>ebm) bmnum=ebm;
      }

      TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
      /*
       * JDS: 201307 
       * Day/Night parameter switching for any parameters associated with 
       *  the pulse transmit sequencing can _NOT_ safely be done at multi-site radars. 
       *  East and West radars may have different day/night settings and controlprograms
       *  must use the same TR gate signal for correct operation.
       *  Any parameter which causes the TR gate timing to shift will lead to incorrect operation
       *  at multi-site radars.
       *
       *  Examples of parameters that should not day/night switch on multi-site radars:
       *    mpinc,txpl,rsep,ptab,lags,pcode,nbaud 
       *  Examples of paramters that can be day/night switched on multi-site radars:
       *    tfreq,nrang,frang,sbm,ebm
       *  Any variables added to the day/night switch logic below should be tested on multi-site
       *  radars.   
      */  
      /* 
       * FIXME: Find a way to ask the site environment if its okay to day/night switch specific variables 
      */
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

      ErrLog(errlog.sock,progname,"Starting Integration.");
      sprintf(logtxt," Int parameters:: rsep: %d mpinc: %d sbm: %d ebm: %d nrang: %d nbaud: %d scannowait: %d clrskip_secs: %d do_clr_scan_start: %d cpid: %d",
              rsep,mpinc,sbm,ebm,nrang,nbaud,scannowait,clrskip_secs,do_clr_scan_start,cp);
      ErrLog(errlog.sock,progname,logtxt);

      sprintf(logtxt,"Integrating beam:%d intt:%ds.%dus (%d:%d:%d:%d) %d %d",bmnum,
              intsc,intus,hr,mt,sc,us,sbm,ebm);
      ErrLog(errlog.sock,progname,logtxt);

            
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
          tfreq=SiteFCLR(stfrq-frqrng/2,stfrq+frqrng/2);
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
/*  
 *  FIXME: JDS: RadarShell has a problem with multi-site and multi-channel setups. Disabling for now
*/
/*
      RadarShell(shell.sock,&rstable);
*/
      if (exitpoll !=0)  break;
      scan=0;
      if (bmnum==ebm)  break;
      if (backward) bmnum--;
      else bmnum++;
    } while (1);

    if ((exitpoll==0) && (scannowait==0)) {
      ErrLog(errlog.sock,progname,"Waiting for scan boundary."); 
      SiteEndScan(scnsc,scnus);
    }
  } while (exitpoll==0);
  
  
  for (n=0;n<tnum;n++) RMsgSndClose(task[n].sock);
  

  ErrLog(errlog.sock,progname,"Ending program.");


  SiteExit(0);

  return 0;   
} 
 
