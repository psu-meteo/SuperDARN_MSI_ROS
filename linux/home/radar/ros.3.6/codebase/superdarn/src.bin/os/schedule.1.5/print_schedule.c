/* print_schedule.c
   ================
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


void print_schedule(struct scd_blk *ptr) {/* prints out the schedule */
  int i,c;
  char txt[256];
  double current_time;
  int yr,mo,dy,hr,mt,sc,us;
  TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
  current_time=TimeYMDHMSToEpoch(yr,mo,dy,hr,mt,sc);
  if (ptr->num==0) {
    log_info(1,"No schedule file loaded");
    return;
  }
  
  for(i=0;i<ptr->num_files;i++) {
    sprintf(txt,"Schedule file %s loaded",ptr->files[i]);
    log_info(1,txt);
  }
  sprintf(txt,"Command path -> %s",ptr->path);
  log_info(1,txt);
  if (ptr->entry[0].stime==-1) {
    sprintf(txt,"Default Program -> %s\n",ptr->entry[0].command);
    log_info(1,txt);
  }
  if ((ptr->cnt >=0) && (ptr->cnt<ptr->num)) {
    int yr,mo,dy,hr,mt;
    double sc;
    log_info(1,"Pending programs :\n");    		 
    for (c=ptr->cnt+1;c<ptr->num;c++) {
      if (ptr->entry[c].stime==-1) continue;
      if(current_time < ptr->entry[c].stime) {
        TimeEpochToYMDHMS(ptr->entry[c].stime,&yr,&mo,&dy,&hr,&mt,&sc);
        sprintf(txt,"%d : %d %d %d : %d %d :: %d %d -> %s",c,yr,mo,dy,hr,mt,
	          ptr->entry[c].priority,ptr->entry[c].duration_minutes,ptr->entry[c].command);
        log_info(1,txt);       
      }
    } 
  } else log_info(1,"There are no pending programs");
  sprintf(txt,"\nCurrent program ->%s",ptr->command);
  log_info(1,txt);
  sprintf(txt,"Schedule reloaded every %d seconds",ptr->refresh);
  
  log_info(1,txt);
}  			

void print_entries(struct scd_blk *ptr) {/* prints out all entries*/
  int c;
  char txt[256];
  double current_time;
  int yr,mo,dy,hr,mt,sc,us;
  TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
  current_time=TimeYMDHMSToEpoch(yr,mo,dy,hr,mt,sc);
  
  sprintf(txt,"Full List of Schedule Entries:");
  log_info(1,txt);
  if ((ptr->cnt >=0) && (ptr->cnt<ptr->num)) {
    int yr,mo,dy,hr,mt;
    double sc;
    for (c=0;c<ptr->num;c++) {
        TimeEpochToYMDHMS(ptr->entry[c].stime,&yr,&mo,&dy,&hr,&mt,&sc);
        sprintf(txt,"%d : %d %d %d : %d %d :: %d %d -> %s",c,yr,mo,dy,hr,mt,
	          ptr->entry[c].priority,ptr->entry[c].duration_minutes,ptr->entry[c].command);
        log_info(1,txt);       
    } 
  } else log_info(1,"no schedule entries");
  log_info(1,"\n");
}
