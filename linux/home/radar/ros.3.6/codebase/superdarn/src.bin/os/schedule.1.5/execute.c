/* execute.c
   =========
   Author: R.J.Barnes
*/

/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "schedule.h"

#define TIME_OUT 10

int resetflg=0;

void terminate_signal_handler(int sig_number) { /* null handler for time out */
  resetflg=1;
}

void terminate(pid_t prog_id) {
  
  
  sigset_t set;
  struct sigaction act;
  struct sigaction oldact; 

  struct itimerval timer;

  sigemptyset(&set);
  sigaddset(&set,SIGALRM);
 
  act.sa_flags=0;
  act.sa_mask=set;
  act.sa_handler=terminate_signal_handler;
  sigaction(SIGALRM,&act,&oldact);

  /* kill off a running program */

  timer.it_interval.tv_sec=0;
  timer.it_interval.tv_usec=0;
  timer.it_value.tv_sec=TIME_OUT;
  timer.it_value.tv_usec=0;

  setitimer(ITIMER_REAL,&timer,NULL);

  kill(prog_id,SIGUSR1);
  waitpid(prog_id,NULL,0);
  kill(prog_id,SIGINT);
  waitpid(prog_id,NULL,0);
  kill(prog_id,SIGINT);
  waitpid(prog_id,NULL,0);
  kill(prog_id,SIGINT);
  waitpid(prog_id,NULL,0);
  
  if (resetflg==1) {  
    fprintf(stderr,"killing task.\n");
    kill(prog_id,SIGUSR1);
    waitpid(prog_id,NULL,0);
    kill(prog_id,SIGINT);
    waitpid(prog_id,NULL,0);
    kill(prog_id,SIGINT);
    waitpid(prog_id,NULL,0);
    kill(prog_id,SIGINT);
    waitpid(prog_id,NULL,0);
    kill(prog_id,SIGKILL);
    waitpid(prog_id,NULL,0);
    resetflg=0;
  }

  timer.it_value.tv_sec=0;
  timer.it_value.tv_usec=0;
  setitimer(ITIMER_REAL,&timer,NULL);

  sigaction(SIGALRM,&oldact,NULL);

}
   


int execute(char *path,char *prog) {
  int pid,i;
  char command[2*SCHED_LINE_LENGTH+1];
  int s=0; 
  char *name;
  char *argv[256];

  /* build the argument vector */

  sprintf(command,"%s/%s",path,prog);
  name=strtok(command," ");
  argv[0]=basename(name);
  for (i=1;(argv[i]=strtok(NULL," ")) !=NULL;i++);
  pid=fork();

  if (pid==0) { 
    s=execv(name,argv);
    exit(0);
  }
  return pid;   

}
  
  



