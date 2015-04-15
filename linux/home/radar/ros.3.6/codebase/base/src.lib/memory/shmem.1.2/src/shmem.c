/* shmem.c
   =======
   Author R.J.Barnes
   J.T.Klein, added shared memory size function to help with IQ buffer (11/2013)
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


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

int ShMemSizeFd(int fd); /* this should go in a header file.. find out how to use build system to include ../include/shmem.h */

unsigned char *ShMemAlloc(char *memname,int size,int flags,int unlink,
			  int *fdptr) {

  int s;
  unsigned char *p;
  int fd;

  if (unlink) {
    s=shm_unlink(memname); /* ignore errors on this */
  }
  
  fd=shm_open(memname,flags,0777);
  if (fd==-1) {
    return NULL;
  }

  if (flags & O_CREAT) {
    s=ftruncate(fd,size);
    if (s==-1) {
      close(fd);
      return NULL;
    }
  } 

  p=mmap(0,size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
  if ((int) p==-1) {
    close(fd);
    return NULL;
  }

  if (fdptr !=NULL) *fdptr=fd;
  return p;

}

int ShMemFree(unsigned char *p,char *memname,int size,int unlink,int fd) {
  int s=0;
  munmap(p,size);
  close(fd);
  if (unlink) s=shm_unlink(memname);
  return s;
}

/* return size of shared memory in bytes from the name */ 
int ShMemSizeName(char *memname) {
  int bufsize, fd;
  fd=shm_open(memname,O_RDONLY,0777);
  bufsize = ShMemSizeFd(fd);
  close(fd);
  return bufsize;
}

/* return size of shared memory in bytes from the file descriptor */
int ShMemSizeFd(int fd) {
  /* may errors if optimization is not enabled (?!?!?!) */
  struct stat filestat;
  int t;
  int memsize = -1;
  
  if(fd > 0) {
    t = fstat(fd, &filestat);
    memsize = (int) filestat.st_size;
  }
  
  return memsize; 
}

