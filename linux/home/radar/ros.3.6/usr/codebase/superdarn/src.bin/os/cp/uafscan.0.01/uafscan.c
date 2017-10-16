/* uafscan.c
   ============
   Author: R.J.Barnes & J.Spaleta & J.Klein
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
/* Includes provided by the OS environment */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <argtable2.h>
#include <zlib.h>
#include <math.h>

/* Includes provided by the RST */ 
#include "rtypes.h"
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
#include "tcpipmsg.h"
#include "rmsg.h"
#include "rmsgsnd.h"
#include "build.h"
#include "global.h"
#include "reopen.h"
#include "setup.h"
#include "sync.h"
#include "site.h"
#include "sitebuild.h"

/* sorry, included for checking sanity checking pcode sequences with --test (JTK)*/
#include "tsg.h" 
#include "maketsg.h"

/* Argtable define for argument error parsing */
#define ARG_MAXERRORS 30

#define MAX_INTEGRATIONS_PER_SCAN 100

int main(int argc,char *argv[]) {
  char progid[80]={"uafscan"};
  char progname[256]="uafscan";
  char modestr[32];

  char *roshost=NULL;
  char *droshost={"127.0.0.1"};

  char *ststr=NULL;

  char *libstr=NULL;
  char *verstr=NULL;
  char *beampattern=NULL;

  int status=0,n,i;
  int nerrors=0;
  int exitpoll=0;

  /* Variables need for interprocess communications */
  char logtxt[1024];
  void *tmpbuf;
  size_t tmpsze;
/*
  struct TCPIPMsgHost shell={"127.0.0.1",44101,-1};
*/
  int tnum=4;      
  struct TCPIPMsgHost task[4]={
    {"127.0.0.1",1,-1}, /* iqwrite */
    {"127.0.0.1",2,-1}, /* rawacfwrite */
    {"127.0.0.1",3,-1}, /* fitacfwrite */
    {"127.0.0.1",4,-1}  /* rtserver */
  };

/* Define the available barker codes for phasecoding*/
  int *bcode=NULL;
  int bcode1[1]={1};
  int bcode2[2]={1,-1};
  int bcode3[3]={1,1,-1};
  int bcode4[4]={1,1,-1,1};
  int bcode5[5]={1,1,1,-1,1};
  int bcode7[7]={1,1,1,-1,-1,1,-1};
  int bcode11[11]={1,1,1,-1,-1,-1,1,-1,-1,1,-1};
  int bcode13[13]={1,1,1,1,1,-1,-1,1,1,-1,1,-1,1};

  /* lists for parameters across a scan, need to send to usrp_server for swings to work.. */
  int32_t scan_clrfreq_bandwidth_list[MAX_INTEGRATIONS_PER_SCAN];
  int32_t scan_clrfreq_fstart_list[MAX_INTEGRATIONS_PER_SCAN];
  int32_t scan_beam_number_list[MAX_INTEGRATIONS_PER_SCAN];
  int32_t nBeams_per_scan = 0;
  int current_beam, iBeam;

  /* time sync of integration periods/ beams */
  int sync_scan;
  int time_now,  time_to_wait; /* times in ms for period synchronization */
  int *scan_times;  /* scan times in ms */

/* Pulse sequence Table */
  int ptab[8] = {0,14,22,24,27,31,42,43};

/* Lag sequence Table */
  int lags[LAG_SIZE][2] = {
    { 0, 0},		/*  0 */
    {42,43},		/*  1 */
    {22,24},		/*  2 */
    {24,27},		/*  3 */
    {27,31},		/*  4 */
    {22,27},		/*  5 */
    /* Lag 6 gap */
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
    /* Lag 23 gap */
    { 0,24},		/* 24 */
    {43,43}};		/* alternate lag-0  */

/* Integration period variables */
  int scnsc=120;
  int scnus=0;
  int total_scan_usecs=0;
  int total_integration_usecs=0;

  /* Variables for controlling clear frequency search */
  struct timeval t0,t1;
  int elapsed_secs=0;
  int default_clrskip_secs=30;
  int startup=1;

  /* XCF processing variables */
  int cnt=0;

  /* create commandline argument structs */
  /* First lets define a help argument */
  struct arg_lit  *al_help       = arg_lit0(NULL, "help", "Prints help infomation and then exits");
  /* Now lets define the keyword arguments */
  struct arg_lit  *al_debug      = arg_lit0(NULL, "debug","Enable debugging messages");
  struct arg_lit  *al_test       = arg_lit0(NULL, "test","Test-only, report parameter settings and exit without connecting to ros server");
  struct arg_lit  *al_discretion = arg_lit0(NULL, "di","Flag this is discretionary time operation"); 
  struct arg_lit  *al_fast       = arg_lit0(NULL, "fast","Flag this as fast 1-minute scan duration"); 
  struct arg_lit  *al_nowait     = arg_lit0(NULL, "nowait","Do not wait for minute scan boundary"); 
  struct arg_lit  *al_onesec     = arg_lit0(NULL, "onesec","Use one second integration times"); 
  struct arg_lit  *al_clrscan    = arg_lit0(NULL, "clrscan","Force clear frequency search at start of scan"); 
  /* Now lets define the integer valued arguments */
  struct arg_int  *ai_baud       = arg_int0(NULL, "baud", NULL,"Baud to use for phasecoded sequences"); /*OptionAdd( &opt, "baud", 'i', &nbaud);*/
  struct arg_int  *ai_tau        = arg_int0(NULL, "tau", NULL,"Lag spacing in usecs"); /*OptionAdd( &opt, "tau", 'i', &mpinc);*/
  struct arg_int  *ai_nrang      = arg_int0(NULL, "nrang", NULL,"Number of range cells"); /*OptionAdd(&opt,"nrang",'i',&nrang);*/
  struct arg_int  *ai_frang      = arg_int0(NULL, "frang", NULL,"Distance to first range cell in km"); /*OptionAdd(&opt,"frang",'i',&frang); */
  struct arg_int  *ai_rsep       = arg_int0(NULL, "rsep", NULL,"Range cell extent in km"); /*OptionAdd(&opt,"rsep",'i',&rsep); */
  struct arg_int  *ai_dt         = arg_int0(NULL, "dt", NULL,"UTC Hour indicating start of day time operation"); /*OptionAdd( &opt, "dt", 'i', &day); */
  struct arg_int  *ai_nt         = arg_int0(NULL, "nt", NULL,"UTC Hour indicating start of night time operation"); /*OptionAdd( &opt, "nt", 'i', &night); */
  struct arg_int  *ai_df         = arg_int0(NULL, "df", NULL,"Day time transmit frequency in kHz"); /*OptionAdd( &opt, "df", 'i', &dfrq); */
  struct arg_int  *ai_nf         = arg_int0(NULL, "nf", NULL,"Night time transmit frequency in KHz"); /*OptionAdd( &opt, "nf", 'i', &nfrq); */
  struct arg_int  *ai_fixfrq     = arg_int0(NULL, "fixfrq", NULL,"Fixes the transmit frequency of the radar to one frequency, in KHz"); /*OptionAdd( &opt, "fixfrq", 'i', &fixfrq); */
  struct arg_int  *ai_xcf        = arg_int0(NULL, "xcf", NULL,"Enable xcf, --xcf 1: for all sequences --xcf 2: for every other sequence, etc..."); /*OptionAdd( &opt, "xcf", 'i', &xcnt); */
  struct arg_int  *ai_ep         = arg_int0(NULL, "ep", NULL,"Local TCP port for errorlog process"); /*OptionAdd(&opt,"ep",'i',&errlog.port); */
  struct arg_int  *ai_sp         = arg_int0(NULL, "sp", NULL,"Local TCP port for radarshall process"); /*OptionAdd(&opt,"sp",'i',&shell.port); */
  struct arg_int  *ai_bp         = arg_int0(NULL, "bp", NULL,"Local TCP port for start of support task proccesses"); /*OptionAdd(&opt,"bp",'i',&baseport); */
  struct arg_int  *ai_sb         = arg_int0(NULL, "sb", NULL,"Limits the minimum beam to the given value."); /*OptionAdd(&opt,"sb",'i',&sbm); */
  struct arg_int  *ai_eb         = arg_int0(NULL, "eb", NULL,"Limits the maximum beam number to the given value."); /*OptionAdd(&opt,"eb",'i',&ebm); */
  struct arg_int  *ai_camp       = arg_int0(NULL, "camp",NULL,"Camping on one beam (camp overwrites sb and eb), or camping beam for themisscan beam pattern."); 
  struct arg_int  *ai_cnum       = arg_int0("c",  "cnum", NULL,"Radar Channel number, minimum value 1"); /*OptionAdd(&opt,"c",'i',&cnum); */
  struct arg_int  *ai_clrskip    = arg_int0(NULL, "clrskip",NULL,"Minimum number of seconds to skip between clear frequency search"); /*OptionAdd(&opt, "clrskip", 'i', &clrskip_secs); */
  struct arg_int  *ai_cpid       = arg_int0(NULL, "cpid",NULL,"Select control program ID number"); /*OptionAdd(&opt, "cpid", 'i', &cpid); */
  struct arg_int  *ai_meribm     = arg_int0(NULL, "meribm", NULL, "Only used for RBSP scan: meridional beam");
  struct arg_int  *ai_westbm     = arg_int0(NULL, "westbm", NULL, "Only used for RBSP scan: west beam");
  struct arg_int  *ai_eastbm     = arg_int0(NULL, "eastbm", NULL, "Only used for RBSP scan: east beam");

  /* Now lets define the string valued arguments */
  struct arg_str  *as_ros        = arg_str0(NULL, "ros", NULL,        "IP address of ROS server process"); /* OptionAdd(&opt,"ros",'t',&roshost); */
  struct arg_str  *as_ststr      = arg_str0(NULL, "stid", NULL,       "The station ID string. For example, use aze for azores east."); /* OptionAdd(&opt,"stid",'t',&ststr); */
  struct arg_str  *as_libstr     = arg_str0(NULL, "lib", NULL,        "The site library string. For example, use ros for for common libsite.ros"); 
  struct arg_str  *as_verstr     = arg_str0(NULL, "version", NULL,    "The site library version string. Defaults to: \"1\" "); 
  struct arg_str  *as_beampattern= arg_str0(NULL, "beampattern", NULL,"The beam pattern to use. (normal, themis, interleave, rbsp)"); 

  /* required end argument */
  struct arg_end  *ae_argend     = arg_end(ARG_MAXERRORS);

  /* create list of all arguement structs */
  void* argtable[] = {al_help,al_debug,al_test,al_discretion, al_fast, al_nowait, al_onesec, \
                      ai_baud, ai_tau, ai_nrang, ai_frang, ai_rsep, ai_dt, ai_nt, ai_df, ai_nf, ai_fixfrq, ai_xcf, ai_ep, ai_sp, ai_bp, ai_sb, ai_eb, ai_camp, ai_cnum, \
                      as_ros, as_ststr, as_libstr,as_verstr,as_beampattern, ai_clrskip,al_clrscan,ai_cpid, ai_meribm, ai_eastbm, ai_westbm, ae_argend};

/* END of variable defines */

/* Set default values of globally defined variables here*/
  cp     = 9000;    /*unused Alaska cpid, will be reset below  */
  intsc  = 7;
  intus  = 0;
  mppul  = 8;
  mplgs  = 23;
  mpinc  = 1500;
  dmpinc = 1500;
  nrang  = 75;
  rsep   = 45;
  txpl   = 300;
  nbaud  = 1;

/* Set default values for all the cmdline options */
  al_discretion->count = 0;
  al_fast->count = 0;
  al_nowait->count = 0;
  al_onesec->count = 0;
  al_clrscan->count = 0;
  al_debug->count = 0;
  ai_bp->ival[0] = 44100;
  ai_fixfrq->ival[0] = -1;
  ai_baud->ival[0] = nbaud;
  ai_tau->ival[0] = mpinc;
  ai_nrang->ival[0] = nrang;
  ai_frang->ival[0] = frang;
  ai_rsep->ival[0] = rsep;
  ai_dt->ival[0] = day;
  ai_nt->ival[0] = night;
  ai_df->ival[0] = dfrq;
  ai_nf->ival[0] = nfrq;
  ai_xcf->ival[0] = xcnt;
  ai_ep->ival[0] = errlog.port;
  ai_sp->ival[0] = shell.port;
  ai_sb->ival[0] = sbm;
  ai_eb->ival[0] = ebm;
  ai_cnum->ival[0] = cnum;
  ai_clrskip->ival[0] = -1;
  ai_cpid->ival[0] = 0;

 /* ========= PROCESS COMMAND LINE ARGUMENTS ============= */
  nerrors = arg_parse(argc,argv,argtable);

  if (nerrors > 0) {
    arg_print_errors(stdout,ae_argend,"uafscan");
    return -1;
  }
  
  if (argc == 1) {
    printf("No arguements found, try running %s with --help for more information.\n", progid);
    return -1;
  }

  /* print help */
  if(al_help->count > 0) {
    printf("UAFSCAN: One control program to emulate the behaviour of all control programs.\n Supported modes: ... \n\n");
    printf(" Usage: %s", progid);
    arg_print_syntax(stdout,argtable,"\n");
    arg_print_glossary(stdout,argtable,"  %-25s %s\n");
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return 0;
  }

  /* Set debug flag from command line arguement */
  if (al_debug->count) {
    debug = al_debug->count;
  }

  /* Load roshost argument here */
  if(strlen(as_ros->sval[0])) {
    roshost = malloc((strlen(as_ros->sval[0]) + 1) * sizeof(char));
    strcpy(roshost, as_ros->sval[0]);
  } else {
    roshost = getenv("ROSHOST");
    if (roshost == NULL) roshost = droshost;
  }

  /* Load station string argument here */
  if(strlen(as_ststr->sval[0])) {
    ststr = malloc((strlen(as_ststr->sval[0]) + 1) * sizeof(char));
    strcpy(ststr, as_ststr->sval[0]);
  } else {
    ststr = getenv("STSTR");
  }

  /* Load site library argument here */
  if(strlen(as_libstr->sval[0])) {
    libstr = malloc((strlen(as_libstr->sval[0]) + 1) * sizeof(char));
    strcpy(libstr, as_libstr->sval[0]);
  } else {
    libstr = getenv("LIBSTR");
    if (libstr == NULL) libstr=ststr;
  }
  if(strlen(as_verstr->sval[0])) {
    verstr = malloc((strlen(as_verstr->sval[0]) + 1) * sizeof(char));
    strcpy(verstr, as_verstr->sval[0]);
  } else {
    verstr = NULL;
  }
  if(strlen(as_beampattern->sval[0])) {
    beampattern = malloc((strlen(as_beampattern->sval[0]) + 1) * sizeof(char));
    strcpy(beampattern, as_beampattern->sval[0]);
  } else {
    beampattern = malloc(7 * sizeof(char));
    strcpy(beampattern, "normal");
  }


  printf("Requested :: ststr: %s libstr: %s verstr: %s\n",ststr,libstr,verstr);
/* This loads Radar Site information from hdw.dat files */
  OpsStart(ststr);

/* This loads Site library via dlopen and maps: site library specific functions into Site name space */
  status=SiteBuild(libstr,verstr); /* second argument is version string */
  if (status==-1) {
    fprintf(stderr,"Could not load requested site library\n");
    exit(1);
  }

  /* cnum is needed to know the config file name */
  if (ai_cnum->count) cnum = ai_cnum->ival[0];


/* Run SiteStart library function to load Site specific default values for global variables
 * This should be run before all options are parsed and before any task sockets are opened
 * arguments: host ip address, 3-letter station string
*/
  
  status=SiteStart(roshost,ststr);
  if (status==-1) {
    fprintf(stderr,"SiteStart failure\n");
    exit(1);
  }

/* load any provided argument values overriding default values provided by SiteStart */ 
  if (ai_xcf->count)  xcnt = ai_xcf->ival[0];
  if (ai_baud->count) nbaud = ai_baud->ival[0];
  if (ai_tau->count) mpinc = ai_tau->ival[0];
  if (ai_nrang->count) nrang = ai_nrang->ival[0];
  if (ai_frang->count) frang = ai_frang->ival[0];
  if (ai_rsep->count) rsep = ai_rsep->ival[0];
  if (ai_dt->count) day = ai_dt->ival[0];
  if (ai_nt->count) night = ai_nt->ival[0];
  if (ai_df->count) dfrq = ai_df->ival[0];
  if (ai_nf->count) nfrq = ai_nf->ival[0];
  if (ai_ep->count) errlog.port = ai_ep->ival[0];
  if (ai_sp->count) shell.port = ai_sp->ival[0];
  if (ai_sb->count) sbm = ai_sb->ival[0];
  if (ai_eb->count) ebm = ai_eb->ival[0];
  if (ai_bp->count) baseport=ai_bp->ival[0];

 /* ========= SET PARAMETER TO EMULATE OTHER CONTROL PROGRAMS ============= */

  /* NORMAL beam order  or CAMPING one one beam */
  if (strcmp(beampattern, "normal") == 0) {
      fprintf(stderr, "Initializing normal beam pattern...\n");
      nBeams_per_scan = abs(ebm-sbm)+1;
      current_beam = sbm;

      /* defaults for normalscan, adjusted   */  
      cp=150;
      scnsc = 120;
      scnus = 0;
      intsc=7;
      intus=0;

   /*   nrang=75; */
      txpl=300;

      sync_scan = 0; 
 
      /* FAST option */  
      if (al_fast->count) {     /* If fast option selected use 1 minute scan boundaries */
        cp    = 151;
        intsc = 3;
        intus = 500000; 
        scnsc = 60;
        scnus = 0;
        sprintf(modestr," (fast)");
        strncat(progname,modestr,strlen(modestr)+1);
      } 

      if (al_onesec->count) {    /* If onesec option selected , no longer wait for scan boundaries, activate clear frequency skip option*/
        cp    = 152;
        intsc = 1;
        intus = 0;
        scnsc = nBeams_per_scan+4;
        scnus = 0;
        sprintf(modestr," (onesec)");
        strncat(progname,modestr,strlen(modestr)+1);
        al_nowait->count = 1;
        if(ai_clrskip->ival[0] < 0)
            ai_clrskip->ival[0] = default_clrskip_secs;
      }
      

      if (ai_camp->count || nBeams_per_scan == 1) {   /* Camping Beam, no longer wait for scan boundaries, activate clear frequency skip option */ 
         fprintf(stderr, "Initializing one camping beam...\n");
         sprintf(modestr," (camp)");
         strncat(progname,modestr,strlen(modestr)+1);

         cp = 153;
         al_nowait->count = 1;
         if(ai_clrskip->ival[0] < 0)
             ai_clrskip->ival[0] = default_clrskip_secs;
         if (ai_camp->count){
             current_beam = ai_camp->ival[0];
             nBeams_per_scan = 1; 
         } 

         sprintf(logtxt,"uafscan configured for camping beam");
         ErrLog(errlog.sock,progname,logtxt);
         sprintf(logtxt," fast: %d onesec: %d cp: %d clrskip_secs: %d intsc: %d",al_fast->count,al_onesec->count,cp,ai_clrskip->ival[0],intsc);
         ErrLog(errlog.sock,progname,logtxt);

      }

      for (iBeam =0; iBeam < nBeams_per_scan; iBeam++){
         scan_beam_number_list[iBeam] = current_beam;
         current_beam += backward ? -1:1;
      }
  }

  /* INTERLEAVE(D) SCAN */
  else if (strcmp(beampattern, "interleave") == 0) {
     fprintf(stderr, "Initializing interleave beam pattern...\n");
     cp     = 191;                   /* using 191 per memorandum */
     intsc  = 3;                             /* integration period; not sure how critical this is */
     intus  = 0;                             /*  but can be changed here */
     scnsc  = 60;
     scnus  = 0;
     nrang  = 100;

     nBeams_per_scan = 16;

     int bmse[16] = { 0,4,8,12, 2,6,10,14, 1,5,9,13, 3,7,11,15 };
     int bmsw[16] = { 15,11,7,3, 13,9,5,1, 14,10,6,2, 12,8,4,0 };
     int *beampattern2take;
     if (strcmp(ststr,"mcm") == 0) {  /* TODO change back to kod */
        beampattern2take = bmse;             /* 1-min sequence */
     } else if (strcmp(ststr,"cvw") == 0) {
        beampattern2take = bmsw;             /* 1-min sequence */
     } else {
       printf("Error: Not intended for station %s\n", ststr);
       return (-1);
     }
     sync_scan  = 1; 
     scan_times = malloc(nBeams_per_scan * sizeof(int));

     for (iBeam =0; iBeam < nBeams_per_scan; iBeam++){
        scan_beam_number_list[iBeam] = beampattern2take[iBeam];
        scan_times[iBeam] = iBeam * (intsc * 1000 + intus/1000); /* in ms*/
    }
  }

  /* THEMISSCAN  */ 
  else if (strcmp(beampattern, "themis") == 0) {
     fprintf(stderr, "Initializing themis beam pattern...\n");
     scnsc  = 120;
     scnus  = 0;
     intsc  = 2;
     intus  = 600000;
     nrang  = 75;
/*     skip_time = 3.0l;  TODO first skip is calculated with 3s not int sc+us */

     nBeams_per_scan = 38;
     int camping_beam= 7; /* Default Camping Beam */

     if (ai_camp->count) 
        camping_beam = ai_camp->ival[0];
     

     /* Second within the 2min interval at which this beam is supposed to start */
     scan_times = ( int [38])    {
       0,   3,   6,   9,  12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,
      45,  48,  51,  54,  60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,
      93,  96,  99, 102, 105, 108, 111, 114 };
     sync_scan = 1;

     /* beams for forward and backward scanning radars; 
      *   -1 will be replaced by the selected camping beam */
     int forward_beams[ 38]=  {
       0,  -1,   1,  -1,   2,  -1,   3,  -1,   4,  -1,   5,  -1,   6,  -1,   7,
      -1,   8,  -1,   9,  -1,  10,  -1,  11,  -1,  12,  -1,  13,  -1,  14,  -1,
      15,  -1,  -1,  -1,  -1,  -1,  -1,  -1 };
     int backward_beams[ 38]= {
      15,  -1,  14,  -1,  13,  -1,  12,  -1,  11,  -1,  10,  -1,   9,  -1,   8,
      -1,   7,  -1,   6,  -1,   5,  -1,   4,  -1,   3,  -1,   2,  -1,   1,  -1,
       0,  -1,  -1,  -1,  -1,  -1,  -1,  -1 };
     
     int *beampattern2take;
     if (backward) 
        beampattern2take = backward_beams;
     else
        beampattern2take = forward_beams;

     for (iBeam =0; iBeam < nBeams_per_scan; iBeam++){
        if (beampattern2take[iBeam] == -1)
            scan_beam_number_list[iBeam] = camping_beam;
        else
            scan_beam_number_list[iBeam] = beampattern2take[iBeam];
        scan_times[iBeam] = scan_times[iBeam] * 1000; /* convert to ms */
     }

  }
  /* RPSP Scan */
  else if (strcmp(beampattern, "rbsp") == 0) { 
  /* Code is copied from:
        rbspscan.c   Author: Kevin Sterne  
        This code uses the 'Option 2' beam progression in which the west, meridional, and east beams are skipped in the regular field 
        of view scan.  As a bit of a trick, the beam progression goes for a forward radar:
                westbm, fovbm, meribm, eastbm, fovbm, fovbm, westbm, fovbm, meribm, eastbm, fovbm, fovbm, westbm, ...
        It was noticed that this kind of pattern still allows for 5 repetitions of the mini-scan beams for the traditional 16 beams radars.
        This code also does not synchronize the start of the beam sounding to an integer time interval.  
       
        This code is largely based off of the themisscan.c RCP.  The credits for this go to:  Author: J.Spaleta  With Modifications by M. McClorey
  */

     fprintf(stderr, "Initializing normal rbsp pattern...\n");

     cp=200;                 /* Semi-official cpid for rbspscan as of 26Oct2012 -KTS */
     scnsc = 120;
     scnus = 0;
     intsc=3;
     intus=200000;
     mppul=8;
     mplgs=23;
     mpinc=1500;
     dmpinc=1500;
     nrang=100;

     sync_scan = 0; 

     /* new variables for dynamically creating beam sequences */
     int isecond, ithird, tempF, tempB;                     /* used in beam progression */
     int *fbms;                                              /* forward  scanning beams */
     int *bbms;                                              /* backward scanning beams */
     int meribm=10;                                          /* meridional beam */
     int westbm=9;                                           /* west beam */
     int eastbm=11;                                          /* east beam */

     if (ai_meribm->count) meribm = ai_meribm->ival[0];
     if (ai_westbm->count) westbm = ai_westbm->ival[0];
     if (ai_eastbm->count) eastbm = ai_eastbm->ival[0];

     /* number of integration periods possible in scan time. basing this off of intsc and intus to reflect the integration
        used by the radar -KTS 25Sept2012 */
     nBeams_per_scan = floor((scnsc+scnus*1e-6)/(intsc+(intus*1e-6)));

     /* Makes sure there is an equal number of miniscan beams */
     /* Taking one off for buffer at the end of the scan */
     nBeams_per_scan = nBeams_per_scan - nBeams_per_scan%6 - 1;

     /* arrays for integration start times and beam sequences */
 /*     intgt = (int *)malloc(nBeams_per_scan*sizeof(int));*/
     fbms  = (int *)malloc(nBeams_per_scan*sizeof(int));
     bbms  = (int *)malloc(nBeams_per_scan*sizeof(int));

     /* If backward is set for West radar, start and end beams need to be reversed for the beam assigning code that follows until "End of Dartmouth Mods".  Usual SuperDARN
      * logic follows that for West radars sbm >= ebm and East radars sbm <= ebm. However, the code below for assigning the beam number arrays, fbms & bbms,
      * does not follow usual SuperDARN logic and always assumes sbm <= ebm.  -KTS 2/20/2012 */
     if (backward == 1) {
             i = sbm;
             sbm = ebm;
             ebm = i;
     }

     /* Creating beam progression arrays fbms and bbms. ASSUMES that forward scan radars will use west beam first,
      * and backward scan radars will use east beam first.  -KTS 09Oct2012 */

     tempB = ebm;
     tempF = sbm;
     for(i = 0; i<nBeams_per_scan; i++){
             isecond = (i-2)%6;
             ithird  = (i-3)%6;
             /* Every 0th, 6th, 12th, etc sounding is a mini-scan beam */
             if(i%6 == 0) {
                     fbms[i] = westbm;
                     bbms[i] = eastbm;
             /* Every 6th sounding starting with 2 is a mini-scan beam */
             } else if (isecond == 0) {
                     fbms[i] = meribm;
                     bbms[i] = meribm;
             /* Every 6th sounding starting with 3 is a mini-scan beam */
             } else if (ithird == 0) {
                     fbms[i] = eastbm;
                     bbms[i] = westbm;
             } else if (tempF<=ebm) {

                     /* Here is where mini-scan beams are skipped in the FoV scan.  The logic works out that if a 
                      start or end beam is in the mini-scan, then it must be treated differently.  -KTS 10Oct2012 */


             /* If tempF is any of the mini-scan beams, then increment it again */
                     if (tempF == westbm)
                             tempF++;
                     if (tempF == meribm)
                             tempF++;
                     if (tempF == eastbm)
                             tempF++;
                     fbms[i] = tempF;
                     tempF++;

             /* If tempB is any of the mini-scan beams, then decrement it again */
                     if (tempB == eastbm)
                             tempB--;
                     if (tempB == meribm)
                             tempB--;
                     if (tempB == westbm)
                             tempB--;
                     bbms[i] = tempB;
                     tempB--;

             } else {
            /* In case the calculation earlier gets messed up, set the beam number to something useful.  Otherwise, leaving the array value unassigned can be bad!!! -KTS 31Oct2012 */
                     bbms[i] = 7;
                     fbms[i] = 7;
             }
     }

     int *beampattern2take;
     if (backward) 
        beampattern2take = bbms;
     else
        beampattern2take = fbms;

     for (iBeam =0; iBeam < nBeams_per_scan; iBeam++){
        scan_beam_number_list[iBeam]    = beampattern2take[iBeam];
     }

  }
  else  {
    fprintf(stderr, "ERROR: Unknown beam pattern: %s. (Supported are: normal, interleave, rbsp or themis)\n", beampattern);
    return -1;
   }


  /* Print out details of beams */ 
  fprintf(stderr, "Sequence details: \n");
  for (iBeam =0; iBeam < nBeams_per_scan; iBeam++){
    fprintf(stderr, "  sequence %2d: beam: %2d, \n",iBeam, scan_beam_number_list[iBeam] );
  }




/* Open Connection to errorlog */  
  if ((errlog.sock=TCPIPMsgOpen(errlog.host,errlog.port))==-1) {    
    fprintf(stderr,"Error connecting to error log.\n Host: %s  Port: %d\n",errlog.host,errlog.port);
  }
/* Open Connection to radar shell */  
  if ((shell.sock=TCPIPMsgOpen(shell.host,shell.port))==-1) {    
    fprintf(stderr,"Error connecting to shell.\n");
  }

/* Open Connection to helper utilities like fitacfwrite*/  
  for (n=0;n<tnum;n++) task[n].port+=baseport;

/* Prep command string for tasks */ 
  strncpy(combf,progid,80);   
  OpsSetupCommand(argc,argv);
  OpsLogStart(errlog.sock,progname,argc,argv);  
  OpsSetupTask(tnum,task,errlog.sock,progname);
  for (n=0;n<tnum;n++) {
    RMsgSndReset(task[n].sock);
    RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
  }


  /* Initialize timing variables */
  elapsed_secs=0;
  gettimeofday(&t1,NULL);
  gettimeofday(&t0,NULL);
  

  if(nBeams_per_scan > 16) {  /* if number of beams in scan greater than legacy 16, recalculate beam dwell time to avoid over running scan boundary if scan boundary wait is active. */ 
      if (al_nowait->count==0 && al_onesec->count==0) {
        total_scan_usecs = (scnsc-3)*1E6+scnus;
        total_integration_usecs = total_scan_usecs/nBeams_per_scan;
        intsc = total_integration_usecs/1E6;
        intus = total_integration_usecs -(intsc*1E6);
      }
  }

  /* Configure phasecoded operation if nbaud > 1 */ 
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
      SiteExit(1);
  }
  pcode=(int *)malloc((size_t)sizeof(int)*mppul*nbaud);
  for(i=0;i<mppul;i++){
    for(n=0;n<nbaud;n++){
      pcode[i*nbaud+n]=bcode[n];
    }
  }

  /* Set special cpid if provided on commandline */
  if(ai_cpid->count > 0) 
     cp=ai_cpid->ival[0];

  /* Set cp to negative value indication discretionary period */
  if (al_discretion->count) 
     cp= -cp;


  /* Calculate tx pulse length setting from range separation */
  txpl = (nbaud*rsep*20)/3;

  /* Attempt to adjust mpinc to be a multiple of 10 and a muliple of txpl */
  if ((mpinc % txpl) || (mpinc % 10))  {
    ErrLog(errlog.sock,progname,"Error: mpinc not multiple of txpl... checking to see if it can be adjusted");
    sprintf(logtxt,"Initial: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep);
    ErrLog(errlog.sock,progname,logtxt);
    if((txpl % 10)==0) {

      ErrLog(errlog.sock,progname, "Attempting to adjust mpinc to correct");
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
  /* Check mpinc and if still invalid, exit with error */
  if ((mpinc % txpl) || (mpinc % 10) || (mpinc==0))  {
     sprintf(logtxt,"Error: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep);
     ErrLog(errlog.sock,progname,logtxt);
     exitpoll = 1;
     SiteExit(0);
  }

  if(al_test->count > 0) {
        
    fprintf(stdout,"Control Program Argument Parameters::\n");
    fprintf(stdout,"  xcf arg:: count: %d value: %d xcnt: %d\n",ai_xcf->count,ai_xcf->ival[0],xcnt);
    fprintf(stdout,"  baud arg:: count: %d value: %d nbaud: %d\n",ai_baud->count,ai_baud->ival[0],nbaud);
    fprintf(stdout,"  clrskip arg:: count: %d value: %d\n",ai_clrskip->count,ai_clrskip->ival[0]);
    fprintf(stdout,"  cpid: %d progname: \'%s\'\n",cp,progname);
    fprintf(stdout,"Scan Sequence Parameters::\n");
    fprintf(stdout,"  txpl: %d mpinc: %d nbaud: %d rsep: %d\n",txpl,mpinc,nbaud,rsep);
    fprintf(stdout,"  intsc: %d intus: %d scnsc: %d scnus: %d nowait: %d\n",intsc,intus,scnsc,scnus,al_nowait->count);
    fprintf(stdout,"  sbm: %d ebm: %d  nBeams_per_scan: %d\n",sbm,ebm,nBeams_per_scan);
    
    /* TODO: ADD PARAMETER CHECKING, SEE IF PCODE IS SANE AND WHATNOT */
   if(nbaud >= 1) {
        /* create tsgprm struct and pass to TSGMake, check if TSGMake makes something valid */
        /* checking with SiteTimeSeq(ptab); would be easier, but that talks to hardware..*/
        /* the job of aggregating a tsgprm from global variables should probably be a function in maketsg.c */
        int flag = 0;

        if (tsgprm.pat !=NULL) free(tsgprm.pat);
        if (tsgbuf !=NULL) TSGFree(tsgbuf);

        memset(&tsgprm,0,sizeof(struct TSGprm));   
        tsgprm.nrang   = nrang;
        tsgprm.frang   = frang;
        tsgprm.rsep    = rsep; 
        tsgprm.smsep   = smsep;
        tsgprm.txpl    = txpl;
        tsgprm.mppul   = mppul;
        tsgprm.mpinc   = mpinc;
        tsgprm.mlag    = 0;
        tsgprm.nbaud   = nbaud;
        tsgprm.stdelay = 18 + 2;
        tsgprm.gort    = 1;
        tsgprm.rtoxmin = 0;

        tsgprm.pat  = malloc(sizeof(int)*mppul);
        tsgprm.code = ptab;

        for (i=0;i<tsgprm.mppul;i++) 
           tsgprm.pat[i]=ptab[i];

        tsgbuf=TSGMake(&tsgprm,&flag);
        fprintf(stdout,"Sequence Parameters::\n");
        fprintf(stdout,"  lagfr: %d smsep: %d  txpl: %d\n",tsgprm.lagfr,tsgprm.smsep,tsgprm.txpl);
    
        if(tsgprm.smsep == 0 || tsgprm.lagfr == 0) {
            fprintf(stdout,"Sequence Parameters::\n");
            fprintf(stdout,"  lagfr: %d smsep: %d  txpl: %d\n",tsgprm.lagfr,tsgprm.smsep,tsgprm.txpl);
            fprintf(stdout,"WARNING: lagfr or smsep is zero, invalid timing sequence genrated from given baud/rsep/nrang/mpinc will confuse TSGMake and FitACF into segfaulting");
        }

        else {
            fprintf(stdout,"The phase coded timing sequence looks good\n");
        }
    } else {
        fprintf(stdout,"WARNING: nbaud needs to be  > 0\n");
    }

    if (nerrors > 0) {
        fprintf(stdout,"Errors found in commandline arguements: \n");
        arg_print_errors(stdout,ae_argend,"uafscan");
    }
    OpsFitACFStart();
 
    fprintf(stdout,"Test option enabled, exiting\n");
    return 0;
  }
  /* SiteSetupRadar, establish connection to usrp_server and do initial setup of memory buffers for raw samples */
  printf("Running SiteSetupRadar Station ID: %s  %d\n",ststr,stid);
  status=SiteSetupRadar();
  if (status !=0) {
    ErrLog(errlog.sock,progname,"Error connection to usrp_server.");
    exit (1);
  }

  printf("Preparing OpsFitACFStart Station ID: %s  %d\n",ststr,stid);
  OpsFitACFStart();


  printf("Preparing SiteTimeSeq Station ID: %s  %d\n",ststr,stid);
  tsgid=SiteTimeSeq(ptab);

  printf("Entering Scan loop Station ID: %s  %d\n",ststr,stid);
  do {
    /* reset clearfreq paramaters, in case daytime changed */
    for (iBeam =0; iBeam < nBeams_per_scan; iBeam++){
      scan_clrfreq_fstart_list[iBeam] = (int32_t) (OpsDayNight() == 1 ? dfrq : nfrq); 
      scan_clrfreq_bandwidth_list[iBeam] = frqrng;
      current_beam += backward ? -1:1;
    }

    /* Set iBeam for scan loop  */ 
    if(al_nowait->count==0) 
       iBeam = OpsFindSkip(scnsc,scnus);
    else 
       iBeam = 0;

    /* send stan data to usrp_sever */
    if (SiteStartScan(nBeams_per_scan, scan_beam_number_list, scan_clrfreq_fstart_list, scan_clrfreq_bandwidth_list, ai_fixfrq->ival[0], sync_scan, scan_times, scnsc, scnus, intsc, intus, iBeam) !=0){
         ErrLog(errlog.sock,progname,"Received error from usrp_server in ROS:SiteStartScan. Probably channel frequency issue in SetActiveHandler.");  
         sleep(1);
         continue;
    }
   /* OLD
     if (SiteStartScan(nBeams_per_scan, scan_beam_number_list, scan_clrfreq_fstart_list, scan_clrfreq_bandwidth_list, ai_fixfrq->ival[0], sync_scan, scan_times, scnsc, scnus, intsc, intus, iBeam) !=0) continue;
*/

    if (OpsReOpen(2,0,0) !=0) {
      ErrLog(errlog.sock,progname,"Opening new files.");
      for (n=0;n<tnum;n++) {
        RMsgSndClose(task[n].sock);
        RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
      }
    }

    scan=1;
    ErrLog(errlog.sock,progname,"Starting scan.");
    if(al_clrscan->count) startup=1;
    if (xcnt>0) {
      cnt++;
      if (cnt==xcnt) {
        xcf=1;
        cnt=0;
      } else xcf=0;
    } else xcf=0;




    /* Scan loop for sequences/beams  */
    do {  
      bmnum = scan_beam_number_list[iBeam];

      TimeReadClock( &yr, &mo, &dy, &hr, &mt, &sc, &us);


      /* THIS IS NOW IS USRP_SERVER */
      /* SYNC periods/beams */ /*
      if (sync_scan) {
          time_now     = ( (mt*60 + sc)*1000 + us/1000 ) % (scnsc*1000 + scnus/1000);
          time_to_wait = scan_times[iBeam] - time_now;
          if (time_to_wait > 0){
             printf("Sync periods: Waiting for %d ms ...", time_to_wait);
             usleep(time_to_wait);
             printf("done.\n");
          } else {
             printf("Sync periods: Not waiting, sinc periods is %d ms too late.", time_to_wait);
          }
      }  */

      /* TODO: JDS: You can not make any day night changes that impact TR gate timing at dual site locations. Care must be taken with day night operation*/ 
      stfrq = scan_clrfreq_fstart_list[iBeam];
      if(ai_fixfrq->ival[0]>0) {
        stfrq=ai_fixfrq->ival[0];
        tfreq=ai_fixfrq->ival[0];
        noise=0; 
      }

      ErrLog(errlog.sock,progname,"Starting Integration.");
      sprintf(logtxt," Int parameters:: rsep: %d mpinc: %d sbm: %d ebm: %d nrang: %d nbaud: %d scannowait: %d clrskip_secs: %d clrscan: %d cpid: %d",
              rsep,mpinc,sbm,ebm,nrang,nbaud,al_nowait->count,ai_clrskip->ival[0],al_clrscan->count,cp);
      ErrLog(errlog.sock,progname,logtxt);

      sprintf(logtxt,"Integrating beam:%d intt:%ds.%dus (%d:%d:%d:%d)",bmnum, intsc,intus,hr,mt,sc,us);
      ErrLog(errlog.sock,progname,logtxt);
            
      printf("Entering Site Start Intt Station ID: %s  %d\n",ststr,stid);
      SiteStartIntt(intsc,intus);
      gettimeofday(&t1,NULL);
      elapsed_secs=t1.tv_sec-t0.tv_sec;
      if (elapsed_secs<0) elapsed_secs=0;
      if ((elapsed_secs >= ai_clrskip->ival[0]) || (startup==1)) {
          startup = 0;
          ErrLog(errlog.sock,progname,"Doing clear frequency search.");  
          sprintf(logtxt, "FRQ: %d %d", stfrq, frqrng);
          ErrLog(errlog.sock,progname, logtxt);
  
          if (ai_fixfrq->ival[0]<=0) {
              tfreq=SiteFCLR(stfrq,stfrq+frqrng);
          }
          t0.tv_sec  = t1.tv_sec;
          t0.tv_usec = t1.tv_usec;
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

      /* Processing and sending data */ 
      OpsBuildPrm(prm,ptab,lags);    
      OpsBuildIQ(iq,&badtr);
      OpsBuildRaw(raw);
      FitACF(prm,raw,fblk,fit);
      
      msg.num   = 0;
      msg.tsize = 0;

      tmpbuf = RadarParmFlatten(prm,&tmpsze);
      RMsgSndAdd(&msg, tmpsze, tmpbuf, PRM_TYPE, 0); 

      tmpbuf=IQFlatten(iq, prm->nave, &tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,IQ_TYPE,0);

      RMsgSndAdd(&msg, sizeof(unsigned int)*2*iq->tbadtr, (unsigned char *) badtr, BADTR_TYPE, 0);
      RMsgSndAdd(&msg, strlen(sharedmemory)+1, (unsigned char *) sharedmemory, IQS_TYPE, 0);

      tmpbuf=RawFlatten(raw,prm->nrang,prm->mplgs,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,RAW_TYPE,0); 
 
      tmpbuf=FitFlatten(fit,prm->nrang,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,FIT_TYPE,0); 

      RMsgSndAdd(&msg,strlen(progname)+1,(unsigned char *) progname, NME_TYPE,0);   
     
     
      for (n=0;n<tnum;n++) RMsgSndSend(task[n].sock,&msg); 

      for (n=0;n<msg.num;n++) {
        if (msg.data[n].type==PRM_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==IQ_TYPE)  free(msg.ptr[n]);
        if (msg.data[n].type==RAW_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==FIT_TYPE) free(msg.ptr[n]); 
      }          

      if (exitpoll !=0) break;
      scan = 0;

      iBeam++;
      if (iBeam >= nBeams_per_scan) break;

    } while (1);

    if ((exitpoll==0) && (al_nowait->count==0)) {
      ErrLog(errlog.sock,progname,"Waiting for scan boundary.");
      SiteEndScan(scnsc,scnus);
    }
  } while (exitpoll==0);
  for (n=0;n<tnum;n++) RMsgSndClose(task[n].sock);
  
  /* free argtable and space allocated for arguements */
  arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
  free(ststr);
  free(roshost);
  
  ErrLog(errlog.sock,progname,"Ending program.");


  SiteExit(0);

  return 0;   
} 
