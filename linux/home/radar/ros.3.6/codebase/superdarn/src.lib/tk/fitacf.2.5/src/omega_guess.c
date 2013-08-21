/* omega_guess.c
   =============
   Author: R.J.Barnes & K.Baker
*/

/*
 LICENSE AND DISCLAIMER
 
 Copyright (c) 2012 The Johns Hopkins University/Applied Physics Laboratory
 
 This file is part of the Radar Software Toolkit (RST).
 
 RST is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.
 
 RST is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with RST.  If not, see <http://www.gnu.org/licenses/>.
 
 
 
*/




/* Revision 1.4 corrected the method of weighting the
   estimate for omega to be consistent with the way
   it is done in fit_acf and do_phase_fit.

   The error on any given point is estimated to be
   delta_phase[i] = <delta_phase>*<P>/P[i]
*/
#include <stdio.h>
#include <math.h>
#include "rmath.h"

double omega_guess(struct complex *acf,double *tau,
	               int *badlag,double *phi_res,
                   double *omega_err,int mpinc,int mplgs) {

  int i,j,nave=0;
  double delta_tau, delta_phi, omega=0.0,first_omega=0.0,first_omega_err=0.0, 
         omega2=0.0, average=0.0, sigma, tau_lim=1.0;
  double sum_W=0.0, W;
  register double temp;
  double two_sigma;
  int repeat_while=1,first_pass=1;
  double average_delta_tau=0.0;
  for (i=0; i < mplgs-1 ; ++i) {
    average_delta_tau+=fabs(tau[i+1] - tau[i]);
  }
  tau_lim=2.*average_delta_tau/(mplgs-1);
  if (tau_lim < 3) tau_lim=3;
  two_sigma = sigma = 2*PI;
  *omega_err = 9999.;
  while (repeat_while && nave < 3) {
    repeat_while=0;
    for (i=0; i < mplgs-1 ; ++i) {
      for (j=i+1; j< mplgs; ++j) {
        if (badlag[j] || badlag[i]) continue;
        delta_tau = tau[j] - tau[i];
/*
        if (fabs(delta_tau) > tau_lim) continue;
*/
        delta_phi = phi_res[j] - phi_res[i];
/*
        W = (cabs(acf[j]) + cabs(acf[i]))/2.0;
*/
        W = 1.;
        W = W*W;

        if (delta_phi > PI) delta_phi = delta_phi - 2*PI;
        if (delta_phi < -PI) delta_phi = delta_phi + 2*PI;

/* Slope dphi/dtau is an estimate of omega */
        temp = delta_phi/delta_tau;
/* First time through average==0
 * Second time through average!=0 and we strip out any slopes more than 2sigma from the average
*/
        if (first_pass) {
          if (fabs(temp - average) > two_sigma) continue;
        } 
/* Take a weighed sum of the omega estimates */
        omega = omega + temp*W;
        omega2 = omega2 + W*(temp*temp);
        sum_W = sum_W + W;
        nave++;
      }
    }
    if(first_pass) {
/* First pass: lets calculate the average omega and sigma, then run through the while loop again */
      first_pass=0;
      if (nave >= 3) {
	  average = omega/sum_W;
	  sigma = ((omega2/sum_W) - average*average)/(nave-1);
	  sigma = (sigma > 0.0) ? sqrt(sigma) : 0.0;
	  two_sigma = 2.0*sigma;
	  omega = omega/sum_W;
	  omega = omega/(mpinc*1.0e-6);
	  *omega_err = sigma/(mpinc*1.0e-6);
          first_omega=omega;
          first_omega_err=*omega_err;
	  omega = 0.0;
	  omega2 = 0.0; 
	  sum_W = 0;
	  nave = 0;
  	  repeat_while = 1;
      } else {
        /* use worst case omega_err estimate*/
        *omega_err = 9999.;
        return 0.0;
      }
    } else {
/* Second pass: we've only included omega estimates within 2sigma of the first pass average*/
      if (nave >=3) {
	  sigma = ((omega2/sum_W) - average*average)/(nave-1);
	  sigma = (sigma > 0.0) ? sqrt(sigma) : 0.0;
	  omega = omega/sum_W;
	  omega = omega/(mpinc*1.0e-6);
	  *omega_err = sigma/(mpinc*1.0e-6);
	  return omega;
      } else {
        /* use estimate from first pass if second pass fails */
        omega=first_omega;
        *omega_err=first_omega_err;
      }
    }
  }
  return omega;
}
