/* site.aze.h
   ==========
*/


#ifndef _SITEAZE_H
#define _SITEAZE_H

int SiteAzeStart(char *host);
int SiteAzeKeepAlive();
int SiteAzeSetupRadar();
int SiteAzeStartScan();
int SiteAzeStartIntt(int intsc,int intus);
int SiteAzeFCLR(int stfreq,int edfreq);
int SiteAzeTimeSeq(int *ptab);
int SiteAzeIntegrate(int (*lags)[2]);
int SiteAzeEndScan(int bsc,int bus);
void SiteAzeExit(int signum);

#endif

