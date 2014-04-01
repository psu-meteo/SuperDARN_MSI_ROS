#ifndef _SITE_H
#define _SITE_H

/* All of this should be turned into runtime ini configs */
#define ANTENNA_SEPARATION 15.24 // meters
#define MAX_ANTENNAS 20
#define MAX_TRANSMITTERS 16
#define MAX_BACK_ARRAY 4
#define BEAM_SPACING  3.24 //degrees
#define MAX_RADARS 1 
#define MAX_CHANNELS 2 
#define GC214_PCI_INDEX  0 
#define TIMING_PCI_INDEX 1 
#define STATUS_PORT 3
#define GPS_DEFAULT_REFRESHRATE 1
#define GPS_DEFAULT_TRIGRATE 10
/* This defines the dds output level 
 * set the maximum DDS output for each radar to ensure correct maximum 
 * driving signal into the transmitters with
 * a single active radar channel
 * 
 * dds full range output mapped to 16 bit integer
 * different sites will want slightly different values
 * for tuned transmit performance
 * length should match MAX_RADARS 
*/ 
#define DDS_MAX_RADAR_OUTPUT { 32000 }

#endif


