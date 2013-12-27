/* site.kod.h
   ==========
*/


#ifndef _SITEKOD_H
#define _SITEKOD_H

int SiteKodStart(char *host);
int SiteKodKeepAlive();
int SiteKodSetupRadar();
int SiteKodStartScan();
int SiteKodStartIntt(int intsc,int intus);
int SiteKodFCLR(int stfreq,int edfreq);
int SiteKodTimeSeq(int *ptab);
int SiteKodIntegrate(int (*lags)[2]);
int SiteKodEndScan(int bsc,int bus);
void SiteKodExit(int signum);

#endif

