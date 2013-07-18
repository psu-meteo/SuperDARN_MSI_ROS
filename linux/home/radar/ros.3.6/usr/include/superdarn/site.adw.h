/* site.adw.h
   ==========
*/


#ifndef _SITEADW_H
#define _SITEADW_H

int SiteAdwStart(char *host);
int SiteAdwKeepAlive();
int SiteAdwSetupRadar();
int SiteAdwStartScan();
int SiteAdwStartIntt(int intsc,int intus);
int SiteAdwFCLR(int stfreq,int edfreq);
int SiteAdwTimeSeq(int *ptab);
int SiteAdwIntegrate(int (*lags)[2]);
int SiteAdwEndScan(int bsc,int bus);
void SiteAdwExit(int signum);

#endif

