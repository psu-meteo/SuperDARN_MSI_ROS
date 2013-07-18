/* set_schedule.c
   ==============
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

int set_schedule(struct scd_blk *ptr) {
  int yr,mo,dy,hr,mt,sc,us;
  double current_time,etime;
  int c,inscope,max_prio,best_entry;
  TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
  current_time=TimeYMDHMSToEpoch(yr,mo,dy,hr,mt,sc);
  best_entry=0;
  max_prio=0;  

/*
  fprintf(stdout, "\n\n\nset_schedule %d\n",max_prio);
*/
  for (c=0;c<ptr->num;c++) {
/*
    fprintf(stdout, "%d :: %d :: %d -> %s\n\n",
      c,ptr->entry[c].duration_minutes,ptr->entry[c].priority,ptr->entry[c].command);
*/
    inscope=0;
    /*First check if entry is in scope*/
    if(current_time>=ptr->entry[c].stime) {
      if(ptr->entry[c].duration_minutes>0) {
        etime=ptr->entry[c].stime+ptr->entry[c].duration_minutes*60.;
        if(etime >= current_time) {
/*
          fprintf(stdout,"Entry %d duration inscope\n",c);
*/
          inscope=1;
        } else {
/*
           fprintf(stdout,"Entry %d out of scope\n",c);
*/
          inscope=0;
        }
      }  else {
/*
        fprintf(stdout,"Entry %d no duration inscope\n",c);
*/
        inscope=1;
      }
    } else {
      /*
       * This entry starts in the future.
       * all subsequent entries are in the future
       */
/*
      fprintf(stdout,"Entry %d out of scope in the future\n",c);
*/
      inscope=0;
    }
/*
    fprintf(stdout,"cprio: %d maxprio: %d\n",ptr->entry[c].priority,max_prio);
*/
    if(inscope) {
      /*Select highest priority inscope*/
      if(ptr->entry[c].priority >=max_prio){
        max_prio=ptr->entry[c].priority;
        best_entry=c;
      }

/*      fprintf(stdout,"Entry %d %d entry prio: %d max_prio: %d\n",c,best_entry,ptr->entry[c].priority,max_prio);*/
    }
  }
/*
  fprintf(stdout,"\n\n\nBest entry: ");
  fprintf(stdout, "%d : %s\n\n",best_entry,ptr->entry[best_entry].command);
*/
  return best_entry;  
}
