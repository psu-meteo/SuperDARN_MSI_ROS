/* test_schedule.c
   ===============
   Author: R.J.Barnes
*/

/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "rtime.h"
#include "log_info.h"
#include "schedule.h"

int test_schedule(struct scd_blk *ptr) {
  int yr,mo,dy,hr,mt,sc,us;
  double current_time;
  TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
  current_time=TimeYMDHMSToEpoch(yr,mo,dy,hr,mt,sc);
  if (ptr->cnt<0) return 0;  
  if (ptr->cnt==ptr->num) return 0;
  if (current_time>=ptr->entry[ptr->cnt].stime) return 1;
  return 0;
}
