#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#ifdef __QNX__
  #include <hw/pci.h>
  #include <hw/inout.h>
  #include <sys/neutrino.h>
  #include <sys/mman.h>
#endif
#include "include/plx_defines.h"

#define SWITCHES 0
#define ATTEN    1
#define READ     0
#define WRITE    1
#define ON       1
#define OFF      0

/*--- SET SWITCHED/ATTEN BIT ---*/
int32_t set_SA(int32_t base,int32_t sa,int32_t radar,int32_t type){
        int32_t temp;
        int32_t portA,portB,portC;
        if(type==1) {
#ifdef __QNX__
          switch(radar) {
            case 1:
              portC=PC_GRP_0;
              portB=PB_GRP_0;
              portA=PA_GRP_0;
              break;
            case 2:
              portC=PC_GRP_2;
              portB=PB_GRP_2;
              portA=PA_GRP_2;
              break;
          }
          if(sa==SWITCHES){
                temp=in8(base+portC);
                out8(base+portC,temp & 0x7f);
          }
          if(sa==ATTEN){
                temp=in8(base+portC);
                out8(base+portC,temp | 0x80);
          }
#endif
        }
}

/*-SET READ/WRITE BIT-------------------------------------------------------*/
int32_t set_RW(int32_t base,int32_t rw,int32_t radar,int32_t type){
        int32_t temp;
        int32_t portA,portB,portC;
        if(type==1) {
#ifdef __QNX__
          switch(radar) {
            case 1:
              portC=PC_GRP_0;
              portB=PB_GRP_0;
              portA=PA_GRP_0;
              break;
            case 2:
              portC=PC_GRP_2;
              portB=PB_GRP_2;
              portA=PA_GRP_2;
              break;
          }
          if(rw==READ){
                temp=in8(base+portC);
                out8(base+portC,temp & 0xbf);
          }
          if(rw==WRITE){
                temp=in8(base+portC);
                out8(base+portC,temp | 0x40);
          }
#endif
        }  
}

/*--- SET WRITE ENABLE BIT ---*/
int32_t set_WE(int32_t base,int32_t onoff,int32_t radar,int32_t type){
        int32_t temp;
        int32_t portA,portB,portC;
#ifdef __QNX__
        switch(radar) {
          case 1:
            portC=PC_GRP_0;
            portB=PB_GRP_0;
            portA=PA_GRP_0;
            break;
          case 2:
            portC=PC_GRP_2;
            portB=PB_GRP_2;
            portA=PA_GRP_2;
            break;
        }
        if(onoff==OFF){
                temp=in8(base+portC);
                out8(base+portC,temp & 0xfe);
        }
        if(onoff==ON){
                temp=in8(base+portC);
                out8(base+portC,temp | 0x01);
        }
#endif
}

int32_t _verify_data(uint32_t base, int32_t radar, int32_t card, int32_t maddr, int32_t code,int SA,int32_t type,int32_t *mdata){
/* type == 1 for MSI  type == 0 for McM */
/* SA : SWITCHES==0 ATTEN==1 */
/* returns data at memory address,  -1 on error */

        int32_t temp=-1;
        struct  timespec nsleep;
        nsleep.tv_sec=0;
        nsleep.tv_nsec=5000;
        int32_t portA,portB,portC,cntrl1;
        int32_t data,return_val;
        int32_t max_val;
        switch(SA) {
          case SWITCHES:
            max_val=8191;
            data=code ^ 0x1fff;
            break;
          case ATTEN:
            max_val=63;
            data=code ^ 0x3f;
            break;
          default:
            max_val=0;
        }          
    // check that the data to write is valid
        if ( (code>max_val) | (code<0) ){
                fprintf(stderr,"INVALID DATA TO WRITE - must be between 0 and %d\n",max_val);
                return -1;
        }
    // bit reverse the data
        data=reverse_bits(data);
#ifdef __QNX__
        switch(radar) {
          case 1:
            portC=PC_GRP_1;
            portB=PB_GRP_1;
            portA=PA_GRP_1;
            cntrl1=CNTRL_GRP_1;
            break;
          case 2:
            portC=PC_GRP_3;
            portB=PB_GRP_3;
            portA=PA_GRP_3;
            cntrl1=CNTRL_GRP_3;
            break;
          default:
            fprintf(stderr,"_verify_phase_data: Bad Radar value: %d\n",radar);
            return -1; 
        }
        temp=_select_card(base,radar,card);
        temp=_select_beam(base,radar,maddr,0);
        set_SA(base,SA,radar,type);
        set_SA(base,SA,READ,type);
    // reset CH1, PortA and PortB to inputs
        out8(base+cntrl1,0x93);
        out8(base+cntrl1,0x13);
    // verify written data

    // read PortA and PortB to see if EEPROM output is same as progammed
        temp=in8(base+portB);
        temp=temp & 0x1f;
        temp=temp << 8;
        temp=temp + in8(base+portA);
        switch(SA) {
          case SWITCHES:
            temp=temp & 0x1fff;
            break;
          case ATTEN:
            temp=temp & 0x1f80;
            break;
        }
        if(mdata!=NULL){
          if ((temp != data) ){
                fprintf(stderr," WARNING - Unexpected Value: data: %x != readback: %x :: Maddr: %d Card: %d\n", code, reverse_bits(temp),maddr,card);
          }
          switch(SA) {
            case SWITCHES:
              return_val=reverse_bits(temp) ^ 0x1fff;
              *mdata=return_val;
              break;
            case ATTEN:
              return_val=reverse_bits(temp) ^ 0x3f;
              *mdata=return_val;
              break;
            default:
              return_val=-1;
              *mdata=return_val;
          }          
          return 0; 
        } else {
          return 0;
        }
#else
        return -1;
#endif
}

int32_t _write_phase(uint32_t base, int32_t radar, int32_t card, int32_t maddr, int32_t phasecode, int32_t type) {
/* type == 1 for MSI  type == 0 for McM */
        int32_t temp;
        struct  timespec nsleep;
        nsleep.tv_sec=0;
        nsleep.tv_nsec=5000;
        int32_t portA,portB,portC,cntrl1;
        int32_t data;
#ifdef __QNX__
        switch(radar) {
          case 1:
            portC=PC_GRP_1;
            portB=PB_GRP_1;
            portA=PA_GRP_1;
            cntrl1=CNTRL_GRP_1;
            break;
          case 2:
            portC=PC_GRP_3;
            portB=PB_GRP_3;
            portA=PA_GRP_3;
            cntrl1=CNTRL_GRP_3;
            break;
          default:
            fprintf(stderr,"_write_phase: Bad Radar value: %d\n",radar);
            return -1; 
        }
    // check that the data to write is valid
        if ( (phasecode>8192) | (phasecode<0) ){
                fprintf(stderr,"INVALID DATA TO WRITE - must be between 0 and 8192\n");
                return -1;
        }
        data=phasecode ^ 0x1fff;
    // select card to write

        if(type==1) {
          temp=_select_card(base,radar,card);
        } else {
          temp=_select_card(base,radar,31);
        }
    // choose the beam code to write (output appropriate EEPROM address
        temp=_select_beam(base,radar,maddr,0);
    // select the phase switches for MSI cards
        set_SA(base,SWITCHES,radar,type);
    // enable writing
        set_RW(base,WRITE,radar,type);
    // set CH1, PortA and Port B to output for writing
        out8(base+cntrl1,0x81);
    // bit reverse the data
        data=reverse_bits(data);
    // apply the data to be written to PortA and PortB on CH1
    // set CH1, Port A to lowest 8 bits of data and output on PortA
        temp=data & 0xff;
        out8(base+portA,temp);
    // set CH0, Port B to upper 5 bits of data and output on PortB
        temp=data & 0x1f00;
        temp=(temp >> 8);
        out8(base+portB,temp);
        out8(base+cntrl1,0x01);
        
        if(type==1) {
    // toggle write enable bit
          set_WE(base,ON,radar,type);
          usleep(50);
          set_WE(base,OFF,radar,type);
        } else {
          temp=_select_card(base,radar,card);
          temp=_select_card(base,radar,31);
        }
    // reset CH1, PortA and PortB to inputs
        out8(base+cntrl1,0x93);
        out8(base+cntrl1,0x13);
    // disable writing
        set_RW(base,READ,radar,type);
        usleep(10000);
    // verify written data
        if(type==1) {
        } else {
          temp=_select_card(base,radar,card);
        }
    // read PortA and PortB to see if EEPROM output is same as progammed
        temp=in8(base+portB);
        temp=temp & 0x1f;
        temp=temp << 8;
        temp=temp + in8(base+portA);
        temp=temp & 0x1fff;
        if(type==1) {
        } else {
          temp=_select_card(base,radar,31);
        }
        if ((temp == data) ){
                return 0;
        }
        else {
                fprintf(stderr," ERROR - New Card DATA NOT WRITTEN: data: %x != readback: %x :: Maddr: %d Card: %d\n", reverse_bits(data), reverse_bits(temp),maddr,card);
                return -1;
        }
#else
  return 0;
#endif
}

int32_t _write_atten(uint32_t base, int32_t radar, int32_t card, int32_t maddr, int32_t attencode, int type){

        int32_t temp;
        struct  timespec nsleep;
        nsleep.tv_sec=0;
        nsleep.tv_nsec=5000;
        int32_t portA,portB,portC,cntrl1;
        int32_t data;
        if(type==0) return 0;
#ifdef __QNX__
        switch(radar) {
          case 1:
            portC=PC_GRP_1;
            portB=PB_GRP_1;
            portA=PA_GRP_1;
            cntrl1=CNTRL_GRP_1;
            break;
          case 2:
            portC=PC_GRP_3;
            portB=PB_GRP_3;
            portA=PA_GRP_3;
            cntrl1=CNTRL_GRP_3;
            break;
          default:
            fprintf(stderr,"_write_atten: Bad Radar value: %d\n",radar);
            return -1; 
        }
     // check that the data to write is valid
        if ( (attencode>63) | (attencode<0) ){
                fprintf(stderr,"INVALID ATTENCODE to WRITE - must be between 0 and 63\n");
                return -1;
        }
        data=attencode ^ 0x3f;
     // select card to write
        temp=_select_card(base,radar,card);
     // choose the beam code to write (output appropriate EEPROM address
        temp=_select_beam(base,radar,maddr,0);
        set_SA(base,ATTEN,radar,type);
    // enable writing
        set_RW(base,WRITE,radar,type);
    // set CH1, PortA and Port B to output for writing
        out8(base+cntrl1,0x81);
    // bit reverse the data
        data=reverse_bits(data);
    // apply the data to be written to PortA and PortB on CH1
    // set CH1, Port A to lowest 8 bits of data and output on PortA
        temp=data & 0xff;
        out8(base+portA,temp);
    // set CH0, Port B to upper 5 bits of data and output on PortB
        temp=data & 0x1f00;
        temp=(temp >> 8);
        out8(base+portB,temp);
        out8(base+cntrl1,0x01);
    // toggle write enable bit
        set_WE(base,ON,radar,type);
        set_WE(base,OFF,radar,type);
    // reset CH1, PortA and PortB to inputs
        out8(base+cntrl1,0x93);
        out8(base+cntrl1,0x13);
    // disable writing
        set_RW(base,READ,radar,type);
        usleep(3000);
    // verify written data
    // read PortA and PortB to see if EEPROM output is same as progammed
        temp=in8(base+portB);
        temp=temp & 0x1f;
        temp=temp << 8;
        temp=temp + in8(base+portA);
        temp=temp & 0x1f80;
        if (temp == data){
                return 0;
        }
        else {
                fprintf(stderr," ERROR - ATTEN DATA NOT WRITTEN: data: %x != readback: %x :: Code: %d Card: %d\n", reverse_bits(data), reverse_bits(temp),attencode,card);
                return -1;
        }
#else
  return 0;
#endif
}

