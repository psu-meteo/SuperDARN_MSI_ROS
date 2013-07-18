/* schedule.h
   ==========
   Author: R.J.Barnes
*/


/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/


/* data structures for my scheduler routine */

#define SCHED_LINE_LENGTH 1024
#define SCHED_MAX_ENTRIES 1024
#define DELIM " \t\n"
#define SCHED_MAX_FILES  10
#define FILENAME_MAX_LENGTH 256 
/* each entry consists of time at which a command starts 
   + the load command */

struct scd_entry {
  int priority;
  int duration_minutes;
  double stime;
  char command[SCHED_LINE_LENGTH];
};

struct scd_blk {
  char files[SCHED_MAX_FILES][FILENAME_MAX_LENGTH];
  int num_files;
  int num; /* number of scheduled events */
  int cnt;
  char path[SCHED_LINE_LENGTH];
  struct scd_entry entry[SCHED_MAX_ENTRIES];
  char command[SCHED_LINE_LENGTH];
  int default_set;
  int refresh;
  pid_t pid;
}; 
 
  
  
void print_schedule(struct scd_blk *ptr);
void print_entries(struct scd_blk *ptr);
int set_schedule(struct scd_blk *ptr);
int load_schedule(FILE *fp,struct scd_blk *ptr);
int init_schedule(struct scd_blk *ptr);
int test_schedule(struct scd_blk *ptr);
