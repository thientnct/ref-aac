/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

#ifndef _coupling_h_
#define _coupling_h_

#include "block.h"
#include "config.h"
#include "adifdec.h"

void		init_cc(void);
void		destroyCC(void);

int		getcc(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef, Float *cc_gain[CChans][Chans][1<<LEN_NCC]);

void		coupling(Info *info, MC_Info *mip, Float **coef, Float **cc_coef, Float *cc_gain[CChans][Chans][1<<LEN_NCC], int ch, int cc_dom, int cc_ind);
void		ind_coupling(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float *cc_coef[CChans], Float *cc_state[CChans]);

#endif
