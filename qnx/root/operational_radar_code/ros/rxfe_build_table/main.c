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
#include "control_program.h"
#include "global_server_variables.h"
#include "priority.h"
#include "include/plx_functions.h"
#include "include/plx_defines.h"
#include "utils.h"
#define ANTENNAS 20

int sock,msgsock;
int verbose=0;
int configured=1;

unsigned long rxfe_addresses[256];

static unsigned char reverse_addr(unsigned char x) {
  unsigned char h; 
  int i=0;
  for(h=i=0; i< 5; i++) {
    h=( h << 1 ) + (x & 1);
    x>>=1;
  }
  return h;
}

int main(int argc, char *argv[]){

    // DECLARE AND INITIALIZE ANY NECESSARY VARIABLES

	// counter and temporary variables
	int	i,j,addr,card,code;
        int     ifmode,att1,att2,att3,att4,radar,amp1,amp2,amp3; 
	int 	temp,temp1,result;
	// pci, io, and memory variables
	unsigned int	 mmap_io_ptr,IOBASE;
	int		pci_handle,IRQ;
	// timing related variables
        struct timeval tv;
	struct	timespec	start, stop, sleep;
        struct RXFESettings  settings;
        char filename[80];
#ifdef __QNX__       
	struct	 _clockperiod 	new, old;
	int	clockresolution;
#endif

#ifdef __QNX__       
    // SET THE SYSTEM CLOCK RESOLUTION AND GET THE START TIME OF THIS PROCESS 
	// set the system clock resolution to 10 us

	new.nsec=10000;
	new.fract=0;
	temp=ClockPeriod(CLOCK_REALTIME,&new,0,0);
	if(temp==-1) 	perror("Unable to change system clock resolution");
	temp=ClockPeriod(CLOCK_REALTIME,0,&old,0);
	if(temp==-1) 	perror("Unable to read sytem time");
	clockresolution=old.nsec;
	if (verbose > 0) printf("System clock resolution is %d ns\n",clockresolution);

	i=0;

        if(configured) {
      // OPEN THE PLX9052 AND GET LOCAL BASE ADDRESSES 
	  temp=_open_PLX9052(&pci_handle, &mmap_io_ptr, &IRQ, 1);
          IOBASE=mmap_io_ptr;
          if(temp==-1){
                if (verbose > 0) fprintf(stderr, "       PLX9052 configuration failed");
          }
          else{
                if (verbose > 0) fprintf(stderr, "       PLX9052 configuration successful!\n");
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
          }

          if(argc==1) {
            for(ifmode=0;ifmode<2;ifmode++) {
              for(amp3=0;amp3<2;amp3++) {
                for(amp2=0;amp2<2;amp2++) {
                  for(amp1=0;amp1<2;amp1++) {
                    for(att4=0;att4<2;att4++) {
                      for(att3=0;att3<2;att3++) {
                        for(att2=0;att2<2;att2++) {
                          for(att1=0;att1<2;att1++) {
	    		    settings.ifmode=ifmode;  // IF Enabled
			    settings.amp1=amp1;    // Stage 1 Amp 20 db
                            settings.amp2=amp2;    // Stage 2 Amp 10 db after IF mixer
                            settings.amp3=amp3;    // Stage 3 Amp 10 db after IF mixer
                            settings.att1=att1;    // 1/2 db Attenuator
                            settings.att2=att2;    //  1  db Attenuator
                            settings.att3=att3;    //  2  db Attenuator 
                            settings.att4=att4;    //  4  db Attenuator
                            addr=build_RXFE_EEPROM_address(settings);
                            code=build_RXFE_EEPROM_code(settings);
                          for(i=0;i<2;i++) {
                            for(j=0;j<2;j++) {
                              card=reverse_addr(i*2+j);
                              temp=read_RXFE_EEPROM_address(IOBASE, card, addr);
                              result=-1;
                              if (temp!=code){
                                result=write_RXFE_EEPROM_address(IOBASE, card,addr,code);
                                temp1=read_RXFE_EEPROM_address(IOBASE, card, addr);
                                printf("Write: Radar: %d Card: %d Addr: %d Old: %d New: %d Write: %d  Check: %d\n"
                                  ,i,card,addr,temp,code,result,temp1);
                              } else {
                                temp1=read_RXFE_EEPROM_address(IOBASE, card, addr);
                                printf("No Write: Radar: %d Card: %d Addr: %d Old: %d New: %d Write: %d  Check: %d\n"
                                  ,i,card,addr,temp,code,result,temp1);
                              }
                            } 
                          }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }  
          if (argc>1) {  
              addr=atoi(argv[1]);
              printf("%s %d\n",argv[1],addr);
              set_RXFE_EEPROM_address(IOBASE,addr); 
              build_settings_from_code(addr,&settings); 
              printf("Amp1: %d Amp2: %d Amp3: %d IF: %d\n",settings.amp1,settings.amp2,settings.amp3,settings.ifmode); 
              printf("Att1: %d Att2: %d Att3: %d Att4: %d\n",settings.att1,settings.att2,settings.att3,settings.att4) ;
          }
  }
#endif
        return 0;
}
