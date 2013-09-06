/* site.ksr.h
   ==========
*/


#ifndef _SITEKSR_H
#define _SITEKSR_H

int SiteKsrStart(char *host);
int SiteKsrKeepAlive();
int SiteKsrSetupRadar();
int SiteKsrStartScan();
int SiteKsrStartIntt(int intsc,int intus);
int SiteKsrFCLR(int stfreq,int edfreq);
int SiteKsrTimeSeq(int *ptab);
int SiteKsrIntegrate(int (*lags)[2]);
int SiteKsrEndScan(int bsc,int bus);
void SiteKsrExit(int signum);

#endif

