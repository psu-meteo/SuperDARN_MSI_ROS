#ifndef _SITE_H
#define _SITE_H

#define ANTENNA_SEPARATION 15.24 // meters SABRE Array : KOD
/*
#define ANTENNA_SEPARATION 12.8016 // meters MSI Array : McM AD AZ FH CV
*/
#define MAX_ANTENNAS 20
#define MAX_TRANSMITTERS 16
#define MAX_BACK_ARRAY 4
#define BEAM_SPACING  3.24 //degrees
#define MAX_RADARS 1 
#define MAX_CHANNELS 4 
#define GC214_PCI_INDEX -1 
#define TIMING_PCI_INDEX 0 
#define GPS_DEFAULT_REFRESHRATE 1
#define GPS_DEFAULT_TRIGRATE 10
#define IMAGING 1
#define DDS_MAX_CARDS 4
#define DDS_MASTER_INDEX 3
#define DDS_TERM_INDEX 2


#define USE_GPS_USOCK 1 
#define USE_RECV_USOCK 1
#define USE_TIMING_USOCK 0
#define USE_DDS_USOCK 0
#define USE_DIO_USOCK 0

#endif


