#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include "rtime.h"
#include "schedule.h"
#include "log_info.h"

int init_schedule(struct scd_blk *ptr) {
  ptr->num=0;
  ptr->cnt=0;
  ptr->default_set=0;
  ptr->entry[0].stime=-1;
  strcpy(ptr->entry[0].command,"NO DEFAULT SELECTED YET!!!");

  return 0;
}
