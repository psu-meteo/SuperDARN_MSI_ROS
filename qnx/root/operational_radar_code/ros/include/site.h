#ifndef _SITE_H
#define _SITE_H

#define ANTENNA_SEPARATION 15.24 // meters
#define MAX_ANTENNAS 20
#define MAX_TRANSMITTERS 16
#define MAX_BACK_ARRAY 4
#define BEAM_SPACING  3.24 //degrees
#define MAX_RADARS 2 
#define MAX_CHANNELS 4 
/* 
   Note that the GC214 card and the Adlink Timing Card use the same 
   PCI vendor and device ID (this is a hardware bug for both of them)
   Systems with gc214 cards install will need to explictly set the pci index
   for both the gc214 and the timing card via a trial and error method
*/
#define GC214_PCI_INDEX -1 
#define TIMING_PCI_INDEX 0 
#define GPS_DEFAULT_REFRESHRATE 1
#define GPS_DEFAULT_TRIGRATE 10
#endif


