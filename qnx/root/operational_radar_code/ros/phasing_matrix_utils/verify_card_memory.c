#include <sys/types.h> 
#ifdef __QNX__
  #include <hw/inout.h>
  #include <sys/socket.h>
  #include <sys/neutrino.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
#endif
#include <signal.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "global_server_variables.h"
#include "priority.h"
#include "include/plx_functions.h"
#include "include/plx_defines.h"
#include "utils.h"
#include "rtypes.h"

 
int verbose=0;
int configured=1;
 
int main(int argc, char *argv[]){
  // DECLARE AND INITIALIZE ANY NECESSARY VARIABLES
    unsigned int mmap_io_ptr,IOBASE;
    int	pci_handle,IRQ;
    int i,temp;
    int return_val;
    int32_t data=-1,radar=1,card=-1,type=1,maddr=-1,phasecode=0,attencode=0;
    int32_t *mdata=NULL;
    int32_t verify_phasecode=0,verify_attencode=0;
    for(i = 1; i < argc; i++) {  /* Skip argv[0] (program name). */
      if (strcmp(argv[i], "-m") == 0){
        i++;
        maddr = atoi(argv[i]); 
      } 
      if (strcmp(argv[i], "-c") == 0){
        i++;
        card = atoi(argv[i]); 
      } 
      if (strcmp(argv[i], "-p") == 0){
        i++;
        phasecode = atoi(argv[i]); 
        verify_phasecode = 1 ; 
      } 
      if (strcmp(argv[i], "-a") == 0){
        i++;
        attencode = atoi(argv[i]); 
        verify_attencode = 1 ; 
      } 
      if (strcmp(argv[i], "-v") == 0){
        i++;
        verbose += atoi(argv[i]); 
      } 
      if (strcmp(argv[i], "-r") == 0){
        i++;
        radar = atoi(argv[i]); 
      } 
      if (strcmp(argv[i], "-old") == 0){
        type = 0; 
      } 
    }
    if (argc < 2 || maddr < 0 || card < 0  ) {
      fprintf(stdout,"%s called with no arguments\n",argv[0]);
      fprintf(stdout,"  Required argument -m memory address\n");
      fprintf(stdout,"  Required argument -c card\n");
      fprintf(stdout,"  Optional argument -p phasecode\n");
      fprintf(stdout,"  Optional argument -a attencode\n");
      fprintf(stdout,"  Optional argument -r radar number, 1 or 2 for dual site. Default is 1\n");
      fprintf(stdout,"  Optional argument -old: use for McM phasing cards\n");
      fprintf(stdout,"  Optional argument -v increment verbosity flag\n");
      return -1;
    } else {
      if (verbose > 0 ) fprintf(stdout,"Selected Radar: %d Maddr: %d Card: %d Phase: %d Atten: %d\n",radar,card,maddr,phasecode,attencode);
    }
#ifdef __QNX__       
    // SET THE SYSTEM CLOCK RESOLUTION AND GET THE START TIME OF THIS PROCESS 
        if(configured) {
      // OPEN THE PLX9052 AND GET LOCAL BASE ADDRESSES 
	  temp=_open_PLX9052(&pci_handle, &mmap_io_ptr, &IRQ, verbose);
          IOBASE=mmap_io_ptr;
          if(temp==-1){
                if (verbose > 1) fprintf(stderr, "       PLX9052 configuration failed");
          }
          else{
                if (verbose > 1) fprintf(stderr, "       PLX9052 configuration successful!\n");
          }
    // INITIALIZE DIO BOARD
          // GROUP 0 - PortA=output, PortB=output, PortClo=output, PortChi=output
          out8(IOBASE+CNTRL_GRP_0,0x80);
          // GROUP 1 - PortAinput, PortB=input, PortClo=input, PortChi=output
          out8(IOBASE+CNTRL_GRP_1,0x93);
          out8(IOBASE+PA_GRP_0,0x00);
          out8(IOBASE+PB_GRP_0,0x00);
          out8(IOBASE+PC_GRP_0,0x00);
          out8(IOBASE+PA_GRP_1,0x00);
          out8(IOBASE+PB_GRP_1,0x00);
          if (DEVICE_ID==0x0c78) {
          // GROUP 2 - PortA=output, PortB=output, PortClo=output, PortChi=output
            out8(IOBASE+CNTRL_GRP_2,0x80);
          // GROUP 3 - PortAinput, PortB=input, PortClo=input, PortChi=output
            out8(IOBASE+CNTRL_GRP_3,0x93);
            out8(IOBASE+PA_GRP_2,0x00);
            out8(IOBASE+PB_GRP_2,0x00);
            out8(IOBASE+PC_GRP_2,0x00);
            out8(IOBASE+PA_GRP_3,0x00);
            out8(IOBASE+PB_GRP_3,0x00);
            out8(IOBASE+CNTRL_GRP_4,0x80);
            out8(IOBASE+PC_GRP_4,0x00);
            out8(IOBASE+PA_GRP_4,0x00);
            out8(IOBASE+PB_GRP_4,0xff);
          }
        } else {
          printf("Not configured\n");
        }
#endif
    // Set Beam 
        return_val=0;
        _select_card(IOBASE,radar,card); 
        _select_beam(IOBASE,radar,card,verbose); 

        mdata=&data;
        return_val+=_verify_data(IOBASE,radar,card,maddr,attencode,ATTEN,type,mdata); 
        fprintf(stdout,"Attencode:: ");
        fprintf(stdout," In-Memory: %6d ",data);
        if(verify_attencode==1) {
          fprintf(stdout,"Expected: %6d",attencode);
          if(data!=attencode) return_val+=ATTENERR;
        }
        fprintf(stdout,"\n");

        mdata=&data;
        return_val+=_verify_data(IOBASE,radar,card,maddr,phasecode,SWITCHES,type,mdata); 
        fprintf(stdout,"Phasecode:: ");
        fprintf(stdout," In-Memory: %6d ",data);
        if(verify_phasecode==1) {
          fprintf(stdout,"Expected: %6d",phasecode);
          if(data!=phasecode) return_val+=ATTENERR;
        }
        fprintf(stdout,"\n");

        return return_val;
}
