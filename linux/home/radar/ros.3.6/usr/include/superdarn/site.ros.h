/* site.standard.h
   ==========
*/


#ifndef _SITEROS_H
#define _SITEROS_H

int SiteRosStart(char *host,char *ststr);
int SiteRosKeepAlive();
int SiteRosSetupRadar();
int SiteRosStartScan();
int SiteRosStartIntt(int intsc,int intus);
int SiteRosFCLR(int stfreq,int edfreq);
int SiteRosTimeSeq(int *ptab);
int SiteRosIntegrate(int (*lags)[2]);
int SiteRosEndScan(int bsc,int bus);
void SiteRosExit(int signum);

#endif

