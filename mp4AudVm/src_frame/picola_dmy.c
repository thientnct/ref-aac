/**********************************************************************
MPEG-4 Audio VM
speed control PICOLA (Panasonic) dummy



This software module was originally developed by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.



Source file: picola_dmy.c

 

Required modules:
common.o		common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
03-sep-96   HP    born, based on pan_picola.h by Panasonic
**********************************************************************/

#include <stdio.h>

#include "common_m4a.h"		/* common module */

#include "pan_picola.h"		/* for speed control (PICOLA) */


/* ---------- functions ---------- */

void PicolaInit(int MaxFrameLength, float SamplingFrequency, 
    float SpeedControlFactor)
{
  CommonExit(1,"PicolaInit: dummy");
}

void PicolaFree()
{
  CommonExit(1,"PicolaFree: dummy");
}

void pit_sel_backward(float *Signal)
{
  CommonExit(1,"pit_sel_backward: dummy");
}

void pit_sel_forward(float *Signal)
{
  CommonExit(1,"pit_sel_forward: dummy");
}

void mod_picola(float InputSignal[], int NumInputSample,
    float OutputSignal[], float SpeedControlFactor,  
    int *DecodeFlag, int *OutputFlag)
{
  CommonExit(1,"mod_picola: dummy");
}


/* end of picola_dmy.c */

