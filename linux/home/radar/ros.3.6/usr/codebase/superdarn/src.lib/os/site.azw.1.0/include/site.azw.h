/* site.azw.h
   ==========
*/


#ifndef _SITEAZW_H
#define _SITEAZW_H

int SiteAzwStart(char *host);
int SiteAzwKeepAlive();
int SiteAzwSetupRadar();
int SiteAzwStartScan();
int SiteAzwStartIntt(int intsc,int intus);
int SiteAzwFCLR(int stfreq,int edfreq);
int SiteAzwTimeSeq(int *ptab);
int SiteAzwIntegrate(int (*lags)[2]);
int SiteAzwEndScan(int bsc,int bus);
void SiteAzwExit(int signum);

#endif

