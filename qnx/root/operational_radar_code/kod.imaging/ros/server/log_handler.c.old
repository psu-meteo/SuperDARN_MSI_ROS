#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "global_server_variables.h"
#define PRE 0
#define POST 1
#define LOCK 1
#define UNLOCK 0

extern char *controlprogram_list_lock_buffer,*exit_lock_buffer,*coord_lock_buffer;
extern int verbose;

void logger(char **output_buffer_address,int post,int lock,char *message,int r,int c,int print)
{
  char msg_string[100],outstring[300],time_string[80];
  char *output_buffer;
  time_t ct;
  output_buffer=*output_buffer_address;
  if(print) printf("logger: input buffer %p\n",output_buffer);
  sprintf(msg_string,"");
  time(&ct);
  strftime(time_string,80,"%D-%T",localtime(&ct));
  if(post)
    strcat(msg_string,"POST : ");
  else 
    strcat(msg_string,"PRE  : ");
  strcat(msg_string,message);
  if(lock)
    strcat(msg_string," :   LOCK");
  else 
    strcat(msg_string," : UNLOCK");
  if(post && lock) {
    free(output_buffer);
    sprintf(outstring, "%s : %s : %d %d\n", time_string,msg_string,r,c);
    output_buffer = realloc(NULL, strlen(outstring)+1);
    strcpy(output_buffer, outstring);
  } else  {
    sprintf(outstring, "%s : %s : %d %d\n", time_string,msg_string,r,c);
    output_buffer = realloc(output_buffer, strlen(output_buffer)+strlen(outstring)+1);
    strcat(output_buffer, outstring);
  }
 *output_buffer_address=output_buffer;
  if(print) printf("logger: putput buffer %p\n",output_buffer);
}

void log_exit(void *arg) 
{
   pthread_t tid;
/* get the calling thread's ID */
   tid = pthread_self();

}

void *log_handler(void *arg)
{
   FILE *fp;
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_cleanup_push(log_exit,NULL);

   while (1) {
     sleep(10);
     fp=fopen("/tmp/controlprogram_lock", "w");  
     fprintf(fp,controlprogram_list_lock_buffer);
     fclose(fp);
     fp=fopen("/tmp/exit_lock", "w");  
     fprintf(fp,exit_lock_buffer);
     fclose(fp);
   }
   pthread_cleanup_pop(0);
   log_exit(NULL);
   pthread_exit(NULL);
};


