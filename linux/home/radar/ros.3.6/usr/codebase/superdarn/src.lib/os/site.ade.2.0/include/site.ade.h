/* site.ade.h
   ==========
*/


#ifndef _SITEADE_H
#define _SITEADE_H

int SiteAdeStart(char *host);
int SiteAdeKeepAlive();
int SiteAdeSetupRadar();
int SiteAdeStartScan();
int SiteAdeStartIntt(int intsc,int intus);
int SiteAdeFCLR(int stfreq,int edfreq);
int SiteAdeTimeSeq(int *ptab);
int SiteAdeIntegrate(int (*lags)[2]);
int SiteAdeEndScan(int bsc,int bus);
void SiteAdeExit(int signum);

#endif

