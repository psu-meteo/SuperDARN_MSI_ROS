/* sndwrite.h
   ========== 
   Author: E.G.Thomas
*/


#ifndef _SNDWRITE_H
#define _SNDWRITE_H

int SndFwrite(FILE *fp,struct RadarParm *,struct FitData *);
int SndWrite(int fid,struct RadarParm *,struct FitData *);

#endif
