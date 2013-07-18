/* refresh.c
   =========
   Author: R.J.Barnes
*/

/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "rtime.h"
#include "log_info.h"
#include "schedule.h"

int then=-1;
extern time_t tval[SCHED_MAX_FILES];


int test_refresh(struct scd_blk *ptr) {

  struct stat buf;
  int yr,mo,dy,hr,mt,sc,us;
  int nowsec,now,rc,i;
  TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
  
  nowsec=hr*3600+mt*60+sc;
  now=nowsec/ptr->refresh;

  /* test to see if the schedule has been altered */
  for(i=0;i<ptr->num_files;i++) {
    rc=stat(ptr->files[i],&buf);
    if (rc!=0) {
      fprintf(stdout,"Refresh Error\n");
      fflush(stdout);
      return -1;
    }
    if (tval[i]==-1) tval[i]=buf.st_mtime;
    if (buf.st_mtime !=tval[i]) {
      tval[i]=buf.st_mtime;
      then=now;
      fprintf(stdout,"Schedule Refresh Needed\n");
      fflush(stdout);
      return 1;
    }


  /* okay check whether we're on a refresh boundary */
  
    if (then==-1) then=now;
    if (then !=now) {
      tval[i]=buf.st_mtime;
      then=now;
      fprintf(stdout,"Reached Refresh Boundary\n");
      return 1;
    }   
  }
  return 0;
}
