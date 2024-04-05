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


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
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

  int ptab[8] = {0,14,22,24,27,31,42,43};

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
 
  int scnsc=120;
  int scnus=0;
  int skip;
  int cnt=0;
  int i;
  unsigned char fast=0;
  unsigned char discretion=0;

  int status=0,n;

  int beams=0;
  int total_scan_usecs=0;
  int total_integration_usecs=0;
  int fixfrq=-1;

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

  cp=0;
  intsc=7;
  intus=0;
  mppul=8;
  mplgs=23;
  mpinc=1500;
  dmpinc=1500;
  nrang=75;
  rsep=45;
  txpl=300;

  /* ========= PROCESS COMMAND LINE ARGUMENTS ============= */

  OptionAdd(&opt,"di",'x',&discretion);

  OptionAdd(&opt,"frang",'i',&frang);
  OptionAdd(&opt,"rsep",'i',&rsep);

  OptionAdd( &opt, "dt", 'i', &day);
  OptionAdd( &opt, "nt", 'i', &night);
  OptionAdd( &opt, "df", 'i', &dfrq);
  OptionAdd( &opt, "nf", 'i', &nfrq);
  OptionAdd( &opt, "fixfrq", 'i', &fixfrq);
  OptionAdd( &opt, "xcf", 'i', &xcnt);

  OptionAdd(&opt,"ep",'i',&errlog.port);
  OptionAdd(&opt,"sp",'i',&shell.port); 

  OptionAdd(&opt,"bp",'i',&baseport); 

  OptionAdd(&opt,"ros",'t',&roshost);

  OptionAdd(&opt,"stid",'t',&ststr); 
  OptionAdd(&opt,"lib",'t',&libstr); 
 
  OptionAdd(&opt,"fast",'x',&fast);

  OptionAdd( &opt, "nowait", 'x', &scannowait);
  OptionAdd(&opt,"sb",'i',&sbm);
  OptionAdd(&opt,"eb",'i',&ebm);
  OptionAdd(&opt,"c",'i',&cnum);

   
  arg=OptionProcess(1,argc,argv,&opt,NULL);  
 
  if (ststr==NULL) ststr= getenv("STSTR");
  if (libstr==NULL) libstr = getenv("LIBSTR");
  if (libstr==NULL) libstr=ststr;

  if (roshost==NULL) roshost=getenv("ROSHOST");
  if (roshost==NULL) roshost=droshost;



  OpsStart(ststr);

  status=SiteBuild(libstr,NULL); /* second argument is version string */
 
  if (status==-1) {
    fprintf(stderr,"Could not identify station.\n");
    exit(1);
  }

  arg=OptionProcess(1,argc,argv,&opt,NULL);  

  printf("Station ID: %s  %d\n",ststr,stid);
  SiteStart("",ststr);
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
   
  
  if (fast) sprintf(progname,"noopscan (fast)");
  else sprintf(progname,"noopscan");
  i=0;
  do {
     printf("Hey this is a noop loop : %d\n",i);
     sleep(1);
     if(i>100) exitpoll=1;
     i++;
  } while (exitpoll==0);

  
  SiteExit(0);

  return 0;   
} 
 
