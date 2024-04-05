/* load_schedule.c
   ===============
   Author: R.J.Barnes
*/

/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include "rtime.h"
#include "schedule.h"
#include "log_info.h"

extern int verbose;

int default_priority=0;
int max_duration_minutes=0;
int has_priority=0,has_duration=0;

char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str - 1;
    char *endp = NULL;

    if( str == NULL )
            return NULL;

    if( str[0] == '\0' )
            return str;

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address
     * the first non-whitespace characters from
     * each end.
     */
    while( isspace(*(++frontp)) );
    while( isspace(*(--endp)) && endp != frontp );

    if( str + len - 1 != endp )
            *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
            *str = '\0';

    /* Shift the string so that it starts at str so
     * that if it's dynamically allocated, we can
     * still free it on the returned pointer.  Note
     * the reuse of endp to mean the front of the
     * string buffer now.
     */
    endp = str;
    if( frontp != str )
    {
            while( *frontp ) *endp++ = *frontp++;
            *endp = '\0';
    }


    return str;
}

int parse_schedule_line(char *line,struct scd_blk *ptr) {
 
  char *token,*endptr,*ttok;
  long val;
  int year,month,day,hour,minute,i;
  errno=0;
/*  char txt[128];*/
  if ((token=strtok(line,DELIM))==NULL) return -1; /* year/default */
  
  if (strcmp(token,"default")==0) {
    /* default entry */
    if ((token=strtok(NULL,""))==NULL) return -1; /* command */
    strcpy(ptr->entry[0].command,token);
    ptr->entry[0].stime=-1;
    ptr->default_set=1;
    if(ptr->num==0) ptr->num++;
    return 1;
  } else if (strcmp(token,"path")==0) {
    /* the path variable */
    if ((token=strtok(NULL,""))==NULL) return -1; /* command */
    strcpy(ptr->path,token);
  } else if (strcmp(token,"channel")==0) {
    /* Set the CHANSTR envvar */
    if ((token=strtok(NULL,""))==NULL) return -1; /* string */
    ttok=trim(token);
    fprintf(stdout,"Setting the CHANSTR envvar: \'%s\'\n",ttok);
    setenv("CHANSTR",ttok,1);
  } else if (strcmp(token,"sitelib")==0) {
    /* Set the CHANSTR envvar */
    if ((token=strtok(NULL,""))==NULL) return -1; /* string */
    ttok=trim(token);
    fprintf(stdout,"Setting the LIBSTR envvar: \'%s\'\n",ttok);
    setenv("LIBSTR",ttok,1);
  } else if (strcmp(token,"stationid")==0) {
    /* Set the STSTR envvar */
    if ((token=strtok(NULL,""))==NULL) return -1; /* string */
    ttok=trim(token);
    fprintf(stdout,"Setting the STSTR envvar: \'%s\'\n",ttok);
    setenv("STSTR",ttok,1);
  } else if (strcmp(token,"priority")==0) {
    if ((token=strtok(NULL,""))==NULL) return -1; /* minutes */
    default_priority=atoi(token);
    has_priority=1;
  } else if (strcmp(token,"duration")==0) {
    if ((token=strtok(NULL,""))==NULL) return -1; /* minutes */
    max_duration_minutes=atoi(token);
    has_duration=1;
  } else {
    if (ptr->num==0) ptr->num=1;
    ptr->entry[ptr->num].priority=0;
    ptr->entry[ptr->num].duration_minutes=-1;
    /* extract time data */
	year=atoi(token);
    if ((token=strtok(NULL,DELIM))==NULL) return -1; /* month */
	month=atoi(token);
    if ((token=strtok(NULL,DELIM))==NULL) return -1; /* day */
    day=atoi(token);
    if ((token=strtok(NULL,DELIM))==NULL) return -1; /* hour */
	hour=atoi(token);
    if ((token=strtok(NULL,DELIM))==NULL) return -1; /* minute */
	minute=atoi(token);
    ptr->entry[ptr->num].stime=TimeYMDHMSToEpoch(year,month,day,hour,minute,0);
    if (ptr->entry[ptr->num].stime==-1) return -1;
    if(has_priority) {
      if ((token=strtok(NULL,DELIM))==NULL) return -1; /* duration in minutes */
	val = strtol(token,&endptr, 10);
        if ( (strlen(endptr) > 0) || (errno!=0) ) {
	  ptr->entry[ptr->num].duration_minutes=max_duration_minutes ;
          /* fprintf(stderr,"Entry: %d Using fallback duration: %d\n",ptr->num,max_duration_minutes); */
          if(errno!=0) {
            fprintf(stderr,"Entry: %d Unknown ERROR!!!\n",ptr->num);
          }
        } else {
	  ptr->entry[ptr->num].duration_minutes=val;
        }
      if ((token=strtok(NULL,DELIM))==NULL) return -1; /* priority */
        errno=0;
	val = strtol(token,&endptr, 10);
        if ( (strlen(endptr) > 0) || (errno!=0) ) {
	  ptr->entry[ptr->num].priority=default_priority ;
        } else {
	  ptr->entry[ptr->num].priority=val;
        }
    }

    if ((token=strtok(NULL,""))==NULL) return -1; /* command */
        
    /* strip leading spaces from command */
    for (i=0;(token[i] !=0) && ((token[i]==' ') || (token[i]=='\t'));i++); 
    if (token[i]==0) return -1;   
    strcpy(ptr->entry[ptr->num].command,token+i);
    ptr->num++;
    if( ptr->num>=SCHED_MAX_ENTRIES )  {
      log_info(0,"Exceeded Max number of scheduled items! wrapping around and overwriting.");
      ptr->num=1;
    }
  }
  return 0;	
}
				 
int cmp_scd(const void *a,const void *b) {
  struct scd_entry *as;
  struct scd_entry *ab;

   as=(struct scd_entry *) a;
   ab=(struct scd_entry *) b;
  
   if (as->stime<ab->stime) return -1;
   if (as->stime>ab->stime) return 1;

   if (as->stime==ab->stime) { 
     if (as->priority>ab->priority) return -1;
     if (as->priority<ab->priority) return 1;
   }
   return 0;
}

int load_schedule(FILE *fp,struct scd_blk *ptr) {
  int chr;
  int count=0;
  char txt[256];
  char line[SCHED_LINE_LENGTH];
  int default_present=0;
  rewind(fp);
  default_priority=0;
  max_duration_minutes=0;
  has_priority=0;
  has_duration=0;    
  while ( ( (chr=fgetc(fp)) != EOF) && 
          (ptr->num<SCHED_MAX_ENTRIES) ) {
    switch (chr) {
     case '#' :
	    /* comment so ignore rest of line */
		while ( ((chr=fgetc(fp)) != EOF) && (chr !='\n'));  
		break;
	 case '\n' :
	    /* ignore new lines */
		break;
	 case ' ' :
	   /* ignore leading spaces */
	   break;		
	 default : 
	   /* build string */
	   line[0]=(char) chr;
	   count=1;
	   while ( (count<SCHED_LINE_LENGTH) && ( (chr=fgetc(fp)) != EOF) && 
	   			(chr !='\n') ) {
		 line[count]=(char) chr;		
	     count++;
	   }
	   line[count]=0;
	   if (parse_schedule_line(line,ptr)==1) default_present=1;
	   break;
	}	
  }
 	
  if (ptr->num==0) {
    log_info(0,"No programs in schedule file");
    return 0;
  }
/*
  if (default_present==0) {
    if(ptr->default_set==0) {
      log_info(0,"No default program in schedule file");
      return 0;
    }
  }
*/
  if (ptr->num >= SCHED_MAX_ENTRIES) {
    sprintf(txt,"Schedule truncated : > %d entries",SCHED_MAX_ENTRIES);
    log_info(0,txt);
  }
  /* sort the events into order */
  if(verbose) {
    sprintf(txt,"Pre-Sort schedule entry list");
    log_info(0,txt);
    print_entries(ptr);
    sprintf(txt,"Sorting scheduled events by start time and priority\n");
    log_info(0,txt);
  }
  qsort(ptr->entry,ptr->num,sizeof(struct scd_entry),cmp_scd);						    
  if(verbose) {
    sprintf(txt,"Post-Sort schedule entry list");
    log_info(0,txt);
    print_entries(ptr);
  }
/*
  sprintf(txt,"Num Entries:  %d",ptr->num);
  log_info(0,txt);
  int i;
  for(i=0;i<ptr->num;i++) {
	sprintf(txt,"Sch Entry %d  stime: %lf pr: %d cmd: %s",
          i,ptr->entry[i].stime,ptr->entry[i].priority,
          ptr->entry[i].command);
	log_info(0,txt);
  }
*/
  return 0;
}
