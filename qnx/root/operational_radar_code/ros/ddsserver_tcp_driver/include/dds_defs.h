#ifndef _DDS_H 
#define _DDS_H
#define MAX_TSG 16
#define MAX_TIME_SEQ_LEN 1048576
#define X_bit 0x04
#define P_bit 0x10
#define IMAGING 0 

#define PULSE_LENGTH	   1000 //micro-seconds
#define IPP		   10000 //micro-seconds
#define SAMPLE_FREQ	   100000 //samples-per-second
#define FIFOLVL		   1024
#define STATE_DELAY        130.00 // in Microseconds
#define CH0_SAMPLES_TO_COLLECT 	10000
#define CH1_SAMPLES_TO_COLLECT  10000
#define DDS_MAX_CHANNELS 4
#define DDS_MAX_CARDS 1 
#define DDS_MASTER_INDEX 0 
#define DDS_TERM_INDEX 0 

// Below set the maximum DDS output for each radar to ensure correct maximum driving signal into the transmitters with
//  a single active radar channel

// SPS values, 2014-01-11 
// second output not used for single site set to zero
//#define DDS_MAX_RADAR_OUTPUT { 28355,0 }
#define DDS_MAX_RADAR_OUTPUT { 32000,0 }

#endif
