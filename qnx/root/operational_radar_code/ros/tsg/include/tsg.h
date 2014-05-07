#ifndef _TSG_H
#define _TSG_H

#define CLOCK_PERIOD    10              /* clock period in microseconds */

#define TSG_OK 0
#define TSG_INV_RSEP 1
#define TSG_NO_SMSEP 2
#define TSG_INV_MPPUL_SMSEP 3
#define TSG_INV_PAT 4
#define TSG_INV_MPINC_SMSEP 5
#define TSG_INV_LAGFR_SMSEP 6
#define TSG_INV_DUTY_CYCLE 7
#define TSG_INV_ODD_SMSEP 8
#define TSG_INV_TXPL_BAUD 9
#define TSG_INV_MEMORY 10
#define TSG_INV_PHASE_DELAY 11


struct TSGbuf {
  int32_t index;
  int32_t len;
  int32_t step;  //packed timesequence stepsize in microseconds
  int32_t mppul; 
  int32_t nbaud; 
  int32_t nrang; 
  int32_t mpinc; 
  int32_t smsep; 
  int32_t lagfr; 
  int32_t txpl; 
  unsigned char *code;
  unsigned char *rep;
  int32_t *ppat;
  int32_t *pcode;
};



void TSGFree(struct TSGbuf *ptr);
#endif
