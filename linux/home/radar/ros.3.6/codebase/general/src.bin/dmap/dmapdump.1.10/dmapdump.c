/* dmapdump.c
   ==========
   Author: R.J.Barnes
*/


/*
 (c) 2010 JHU/APL & Others - Please Consult LICENSE.superdarn-rst.3.1-beta-18-gf704e97.txt for more information.
 
 
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include "rtypes.h"
#include "option.h"
#include "dmap.h"
#include "hlpstr.h"




struct OptionData opt;
int arg=0;

int main(int argc,char *argv[]) {
  struct DataMap *ptr;
  struct DataMapScalar *s;
  struct DataMapArray *a;
  char **tmp;
  unsigned char dflg=0;
  unsigned char zflg=0;
  unsigned char help=0;
  unsigned char option=0;


  FILE *fp=NULL;
  gzFile zfp=0;
  int c;
  int x,n;

  OptionAdd(&opt,"-help",'x',&help);
  OptionAdd(&opt,"-option",'x',&option);
  OptionAdd(&opt,"z",'x',&zflg);

  OptionAdd(&opt,"d",'x',&dflg);

  
  arg=OptionProcess(1,argc,argv,&opt,NULL); 
  if (help==1) {
    OptionPrintInfo(stdout,hlpstr);
    exit(0);
  }
  if (option==1) {
    OptionDump(stdout,&opt);
    exit(0);
  }

  if (arg !=argc) {
    if (zflg) {
      zfp=gzopen(argv[arg],"r");
      if (zfp==0) {
        fprintf(stderr,"File not found.\n");
        exit(-1);
      }
    } else {
      fp=fopen(argv[arg],"r");
      if (fp==NULL) {
        fprintf(stderr,"File not found.\n");
        exit(-1);
      }
    }  
  } else {
    if (zflg) zfp=gzdopen(fileno(stdin),"r");
    else fp=stdin; 
  }
    
  while(1) {
    if (zflg) ptr=DataMapReadZ(zfp);
    else ptr=DataMapFread(fp);

    if (ptr==NULL) break;

    fprintf(stdout,"scalars:\n");
    for (c=0;c<ptr->snum;c++) {
      s=ptr->scl[c];
      switch (s->type) {
        case DATACHAR:
        fprintf(stdout,"\tchar");
        break;
        case DATASHORT:
        fprintf(stdout,"\tshort");
        break;
        case DATAINT:
        fprintf(stdout,"\tint");
        break;
        case DATAFLOAT:
        fprintf(stdout,"\tfloat");
        break;
        case DATADOUBLE:
        fprintf(stdout,"\tdouble");
        break;
        case DATASTRING:
        fprintf(stdout,"\tstring");
        break;
      }
      fprintf(stdout,"\t%c%s%c",'"',s->name,'"');
      fprintf(stdout," = ");
      switch (s->type) {
        case DATACHAR:
        fprintf(stdout,"%d",*(s->data.cptr));
        break;
        case DATASHORT:
        fprintf(stdout,"%d",*(s->data.sptr));
        break;
        case DATAINT:
        fprintf(stdout,"%d",*(s->data.iptr));
        break;
        case DATAFLOAT:
        fprintf(stdout,"%f",*(s->data.fptr));
        break;
        case DATADOUBLE:
        fprintf(stdout,"%lf",*(s->data.dptr));
        break;
        case DATASTRING:
	tmp=(char **) s->data.vptr;
        fprintf(stdout,"%c%s%c",'"',*tmp,'"');
        break;
      }
      fprintf(stdout,"\n");
    }
    fprintf(stdout,"arrays:\n");
    for (c=0;c<ptr->anum;c++) {
      a=ptr->arr[c];
      switch (a->type) {
        case DATACHAR:
        fprintf(stdout,"\tchar");
        break;
        case DATASHORT:
        fprintf(stdout,"\tshort");
        break;
        case DATAINT:
        fprintf(stdout,"\tint");
        break;
        case DATAFLOAT:
        fprintf(stdout,"\tfloat");
        break;
        case DATADOUBLE:
        fprintf(stdout,"\tdouble");
        break;
        case DATASTRING:
        fprintf(stdout,"\tstring");
        break;
      }
      fprintf(stdout,"\t%c%s%c",'"',a->name,'"');
      fprintf(stdout," ");
      for (x=0;x<a->dim;x++) fprintf(stdout,"[%d]",a->rng[a->dim-1-x]);
      if (dflg) {
        fprintf(stdout,"=");
        n=1;
        for (x=0;x<a->dim;x++) n=a->rng[x]*n;
        for (x=0;x<n;x++) {
	  if (x % a->rng[0]==0) fprintf(stdout,"\n\t\t");
          else if (x !=0) fprintf(stdout,",\t");
          switch (a->type) {
            case DATACHAR:
            fprintf(stdout,"%d",a->data.cptr[x]);
            break;
            case DATASHORT:
            fprintf(stdout,"%d",a->data.sptr[x]);
            break;
            case DATAINT:
            fprintf(stdout,"%d",a->data.iptr[x]);
            break;
            case DATAFLOAT:
            fprintf(stdout,"%f",a->data.fptr[x]);
            break;
            case DATADOUBLE:
            fprintf(stdout,"%lf",a->data.dptr[x]);
            break;	    
            case DATASTRING:
            tmp=(char **) a->data.vptr;
            fprintf(stdout,"%c%s%c",'"',tmp[x],'"');
            break;
	    
          }
        }  
        fprintf(stdout,"\n");
      } else fprintf(stdout,"\n");
    }  
    DataMapFree(ptr);

  }
  if (zflg) gzclose(zfp);
  else fclose(fp);
  return 0;  


}
