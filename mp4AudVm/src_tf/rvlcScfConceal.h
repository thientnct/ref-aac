/*
  "This software module was originally developed by

  Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

  and edited by

  !!! This software is FhG IIS propriatery !!!

  Copyright(c)1998. 
*/

#ifdef VERSION2

#ifndef _RVLC_SCF_CONCEAL_H_
#define _RVLC_SCF_CONCEAL_H_


int ScfRecovery( HANDLE_RESILIENCE hResilience, 
                 int numOfFacs, 
                 short * forwFacs, 
                 int forwPos, 
                 short * backwFacs, 
                 int backwPos, 
                 short global_gain, 
                 short * outFacs, 
                 short * lastFrame, 
                 char enhc_con_bit, 
                 Info *info );

#endif /* _RVLC_SCF_CONCEAL_H_ */

#endif /* VERSION2 */
