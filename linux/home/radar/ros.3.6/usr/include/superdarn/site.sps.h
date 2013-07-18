/* site.sps.h
   ==========
*/


#ifndef _SITESPS_H
#define _SITESPS_H

int SiteSpsStart(char *host);
int SiteSpsKeepAlive();
int SiteSpsSetupRadar();
int SiteSpsStartScan();
int SiteSpsStartIntt(int intsc,int intus);
int SiteSpsFCLR(int stfreq,int edfreq);
int SiteSpsTimeSeq(int *ptab);
int SiteSpsIntegrate(int (*lags)[2]);
int SiteSpsEndScan(int bsc,int bus);
void SiteSpsExit(int signum);

#endif

