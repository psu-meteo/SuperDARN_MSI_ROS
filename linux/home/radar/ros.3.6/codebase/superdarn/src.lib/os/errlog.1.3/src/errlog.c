/* errlog.c
   ========
   Author: R.J.Barnes
*/

/*
 LICENSE AND DISCLAIMER
 
 Copyright (c) 2012 The Johns Hopkins University/Applied Physics Laboratory
 
 This file is part of the Radar Software Toolkit (RST).
 
 RST is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.
 
 RST is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with RST.  If not, see <http://www.gnu.org/licenses/>.
 
 
 
*/

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tcpipmsg.h"
#include "errlog.h"

char str[128];

char *ErrLogStrTime() {
  time_t clock;
  struct tm *gmt;
  struct timespec err_tm;
  int stat;
  
  char *mos[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char *dys[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    
  stat = clock_gettime(CLOCK_REALTIME, &err_tm);
  
  gmt = gmtime(&err_tm.tv_sec); 
  sprintf(str,"%s %s %d %02d:%02d:%02d,%03d %d",dys[gmt->tm_wday],mos[gmt->tm_mon],gmt->tm_mday,gmt->tm_hour,gmt->tm_min,gmt->tm_sec,(int)(err_tm.tv_nsec/1e6),1900+gmt->tm_year);
  
  return str;
}

int ErrLog(int sock,char *name,char *buffer) {

  int msg,s;
  size_t nlen,blen;

  nlen=strlen(name)+1;
  blen=strlen(buffer)+1;

  msg=ERROR_MSG;

  s=TCPIPMsgSend(sock,&msg,sizeof(int));

  if (s !=sizeof(int)) {
    fprintf(stderr,"WARNING: Error not logged\n");
    fprintf(stderr,"%s: %d : %s :%s\n",ErrLogStrTime(),getpid(),name,buffer);
    return -1;
  }

  s=TCPIPMsgSend(sock,&nlen,sizeof(size_t));
  s=TCPIPMsgSend(sock,&blen,sizeof(size_t));

  s=TCPIPMsgSend(sock,name,nlen);
  s=TCPIPMsgSend(sock,buffer,blen);

  s=TCPIPMsgRecv(sock,&msg,sizeof(int));

  fprintf(stderr,"%s: %d : %s :%s\n",ErrLogStrTime(),getpid(),name,buffer);

  if ((s !=sizeof(int))  || (msg !=ERROR_OK)) {
    fprintf(stderr,"WARNING: Error not logged\n");
    return -1;
  }
  return 0;
}






