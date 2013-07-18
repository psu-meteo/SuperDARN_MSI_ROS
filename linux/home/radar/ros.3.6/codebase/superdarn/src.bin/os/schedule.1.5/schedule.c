/* schedule.c
   ==========
   Author: R.J.Barnes
*/

/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/

 
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "option.h"

#include "tcpipmsg.h"
#include "errlog.h"

#include "log_info.h"
#include "schedule.h"
#include "refresh.h"
#include "check_program.h"

#include "errstr.h"
#include "hlpstr.h"


unsigned char verbose=0;
struct OptionData opt;
struct OptionText *scdfilelist;

struct scd_blk schedule;
struct scd_blk save_sched;

char *logname=NULL;
char dlogname[]="scdlog";
time_t  tval[SCHED_MAX_FILES];

void child_handler(int signum) {
  pid_t pid;
  if (schedule.pid !=-1) pid=wait(NULL);
  schedule.pid=-1;
}

int main(int argc,char *argv[]) {

  unsigned char dyflg=0;
  unsigned char hrflg=0;
  unsigned char tmtflg=0;
  unsigned char mtflg=0;

  int i,scderr,arg;

  unsigned char help=0; 
  unsigned char option=0; 

  char logtxt[256];

  FILE *fp;
  int modify_flag,tick=0;

  sigset_t set;
  struct sigaction act;
  struct sigaction oldact; 

  for(i=0;i<SCHED_MAX_FILES;i++) {
    tval[i]=-1;
  }
  schedule.refresh=10*60;

  OptionAdd(&opt,"-help",'x',&help);
  OptionAdd(&opt,"-option",'x',&option);

  OptionAdd(&opt,"d",'x',&dyflg);
  OptionAdd(&opt,"h",'x',&hrflg);
  OptionAdd(&opt,"t",'x',&tmtflg);
  OptionAdd(&opt,"m",'x',&mtflg);
  OptionAdd(&opt,"v",'x',&verbose);

  OptionAdd(&opt,"l",'t',&logname);
  OptionAdd(&opt,"f",'a',&scdfilelist);
  arg=OptionProcess(1,argc,argv,&opt,NULL);

  if (help==1) {
    OptionPrintInfo(stdout,hlpstr);
    exit(0);
  }

  if (option==1) {
    OptionDump(stdout,&opt);
    exit(0);
  }
/*
  if (argc==arg) {
    OptionPrintInfo(stdout,errstr);
    exit(0);
  }
*/
  if(scdfilelist==NULL) {
    fprintf(stdout,"Use -f option to set atleast one schedule file\n");
    exit(-1);

  }
  if(scdfilelist->num>SCHED_MAX_FILES) scdfilelist->num=SCHED_MAX_FILES;
  if(verbose) {
    sprintf(logtxt,"Num Schedule Files: %d\n",scdfilelist->num);
    log_info(0,logtxt);
  }
  sigemptyset(&set);

  act.sa_flags=0;
  act.sa_mask=set;
  act.sa_handler=&child_handler;
  sigaction(SIGCHLD,&act,&oldact);

  if (dyflg) schedule.refresh=24*3600;
  if (hrflg) schedule.refresh=3600;
  if (tmtflg) schedule.refresh=10*60;
  if (mtflg) schedule.refresh=60;
  if (logname==NULL) logname=dlogname;
  schedule.num_files=0;
  schedule.num=0;
  schedule.pid=-1;
  schedule.cnt=0;
  strcpy(schedule.command,"Null"); 
  strcpy(schedule.path,"/");

  init_schedule(&schedule);

  scderr=0;
  for(i=0;i<scdfilelist->num;i++) {
    sprintf(logtxt,"Reading schedule file: %s",scdfilelist->txt[i]);
    log_info(0,logtxt);
    fp=fopen(scdfilelist->txt[i],"r");  
    if (fp==NULL) {
      sprintf(logtxt,"Schedule file not found: %s",scdfilelist->txt[i]);
      log_info(0,logtxt);
      scderr++;
    } else {
      if (load_schedule(fp,&schedule) !=0) {
        sprintf(logtxt,"Error reading schedule file: %s",scdfilelist->txt[i]);
        log_info(0,logtxt);
        scderr++;
        exit(-1);
      } else {
        strncpy(schedule.files[i],scdfilelist->txt[i],FILENAME_MAX_LENGTH);
        schedule.files[i][FILENAME_MAX_LENGTH-1]='\0';
        schedule.num_files++;
      }
      fclose(fp);
    }
  }
  if (schedule.default_set!=1) {
    sprintf(logtxt,"Error no default program set in any schedule file");
    log_info(0,logtxt);
    exit(-1);
  }
  if(schedule.num_files==0) {
    sprintf(logtxt,"No valid schedule file found");
    log_info(0,logtxt);
    exit(-1);
  }
  if (schedule.num==0) {
    log_info(0,"Warning: No scheduled programs");
  }

  schedule.cnt=set_schedule(&schedule);
  sleep(1);
  check_program(&schedule,schedule.cnt);
  print_schedule(&schedule);

  do {
      if (schedule.pid ==-1) {
        log_info(0,"Control program not running - Failed or Died\n");
        log_info(0," Waiting 5 seconds and trying to restart\n");
        sleep(5);
        check_program(&schedule,schedule.cnt);
      }
      if (test_refresh(&schedule) != 0) {
        struct stat nstat1[SCHED_MAX_FILES], nstat2[SCHED_MAX_FILES];
        log_info(0, " Refreshing schedule file.");
        for(i=0;i<schedule.num_files;i++) {
          stat(schedule.files[i], &nstat1[i]);
	}
        for(i=0;i<schedule.num_files;i++) {
          stat(schedule.files[i], &nstat2[i]);
	}
        modify_flag=0; 
        for(i=0;i<schedule.num_files;i++) {
          if(nstat1[i].st_mtime != nstat2[i].st_mtime) {
            log_info(0, "A Schedule File is being modified.");
            modify_flag=1; 
          }
        }
        if(modify_flag==0) {
          memcpy(&save_sched, &schedule, sizeof(schedule));
          init_schedule(&schedule);
          for(i=0;i<schedule.num_files;i++) {
            sprintf(logtxt,"Rloading schedule file: %s",schedule.files[i]);
            log_info(0, logtxt);
            fp = fopen(schedule.files[i], "r");
            if (fp == NULL) {
               sprintf(logtxt, "Schedule file %s not found", schedule.files[i]);
               log_info(0, logtxt);
            } else {
              if (load_schedule(fp, &schedule) != 0) {
                log_info(0, "Error reading updated schedule file");
                memcpy(&schedule, &save_sched, sizeof(schedule));
              }
              fclose(fp);
            }
          }
          if (schedule.default_set!=1) {
            sprintf(logtxt,"Error no default program set in any schedule file");
            log_info(0,logtxt);
            exit(-1);
          }
          schedule.cnt = set_schedule(&schedule);
          check_program(&schedule, schedule.cnt);
          print_schedule(&schedule);
        }
      }

      if (test_schedule(&schedule) !=0) {
        check_program(&schedule,schedule.cnt);
        schedule.cnt=set_schedule(&schedule);
      }
      sleep(1);
      tick++;
      tick=tick % 10;

  } while(1);

 
  return 0;

}
   

 






















