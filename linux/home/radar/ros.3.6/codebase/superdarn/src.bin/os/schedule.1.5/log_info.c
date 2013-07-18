/* log_info.c
   ==========
   Author: R.J.Barnes
*/

/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern char *logname;

void log_info(int flg,char *str) {
  FILE *fp;
  char *date;
  char logpath[256];
  time_t ltime;
  struct tm *time_of_day;
 
  time(&ltime);  
  time_of_day=gmtime(&ltime);

  date=asctime(time_of_day);  
  date[strlen(date)-1]=':';
  if (flg==0) fprintf(stderr,date);
  fprintf(stderr,str);
  fprintf(stderr,"\n");
  
  sprintf(logpath,"%s.%.4d%.2d%.2d",logname,1900+
          time_of_day->tm_year,time_of_day->tm_mon+1,
          time_of_day->tm_mday);
  fp=fopen(logpath,"a");
  if (fp !=NULL) {
    if (flg==0) fprintf(fp,date);
    fprintf(fp,str);
    fprintf(fp,"\n");
    fclose(fp);
  }
}










