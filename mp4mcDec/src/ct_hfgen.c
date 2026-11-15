/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

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
Copyright (c) ISO/IEC 2002.

 $Id: ct_hfgen.c,v 1.7 2003/08/27 17:31:36 ct1 Exp $

*******************************************************************************/
/*!
  \file
  \brief  HF Generator  $Revision: 1.7 $
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "adifdec.h"
#include "block.h"
#include "config.h"

#include "ct_hfgen.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

struct PATCH Patch[MAX_NUM_CHANNELS];

static float  BwVector[MAX_NUM_CHANNELS][MAX_NUM_PATCHES];
static float  BwVectorOld[MAX_NUM_CHANNELS][MAX_NUM_PATCHES];


static float InvFiltFactors[7] = {     0,       /* OFF_LEVEL */
                                    0.6f,       /* TRANSITION_LEVEL */
                                   0.75f,       /* LOW_LEVEL */
                                    0.9f,       /* MID_LEVEL */
                                   0.98f };     /* HIGH_LEVEL */







static void
calcAutoCorr ( struct ACORR_COEFS *ac, float realBuf[][64], float imagBuf[][64], int bd, int len)
{
  int   j, jminus1, jminus2;
  const float rel = 1 / (1 + 1e-6f);

  memset(ac, 0, sizeof(struct ACORR_COEFS));

  for ( j = 0; j < len; j++ ) {

    jminus1 = j - 1;
    jminus2 = jminus1 - 1;

#ifdef LOW_POWER_SBR
    ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd];

    ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd];

    ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd];

    ac->r12r += realBuf[jminus1][bd] * realBuf[jminus2][bd];

    ac->r22r += realBuf[jminus2][bd] * realBuf[jminus2][bd];
#else
    ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd] +
                imagBuf[j][bd] * imagBuf[jminus1][bd];

    ac->r01i += imagBuf[j][bd] * realBuf[jminus1][bd] -
                realBuf[j][bd] * imagBuf[jminus1][bd];

    ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd] +
                imagBuf[j][bd] * imagBuf[jminus2][bd];

    ac->r02i += imagBuf[j][bd] * realBuf[jminus2][bd] -
                realBuf[j][bd] * imagBuf[jminus2][bd];

    ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd] +
                imagBuf[jminus1][bd] * imagBuf[jminus1][bd];

    ac->r12r += realBuf[jminus1][bd] * realBuf[jminus2][bd] +
                imagBuf[jminus1][bd] * imagBuf[jminus2][bd];

    ac->r12i += imagBuf[jminus1][bd] * realBuf[jminus2][bd] -
                realBuf[jminus1][bd] * imagBuf[jminus2][bd];

    ac->r22r += realBuf[jminus2][bd] * realBuf[jminus2][bd] +
                imagBuf[jminus2][bd] * imagBuf[jminus2][bd];
#endif

  }



  ac->det = ac->r11r * ac->r22r - rel * (ac->r12r * ac->r12r + ac->r12i * ac->r12i);

}

static float
mapInvfMode (INVF_MODE mode,
             INVF_MODE prevMode)
{
  switch (mode) {

  case INVF_LOW_LEVEL:

    if(prevMode == INVF_OFF)
      return InvFiltFactors[1];
    else
      return InvFiltFactors[2];

  case INVF_MID_LEVEL:
    return InvFiltFactors[3];


  case INVF_HIGH_LEVEL:
    return InvFiltFactors[4];


  default:

    if(prevMode == INVF_LOW_LEVEL)
      return InvFiltFactors[1];
    else
      return InvFiltFactors[0];
  }
}

static void
inverseFilteringLevelEmphasis( INVF_MODE *invFiltMode,
                               INVF_MODE *prevInvFiltMode,
                               int nNfb,
                               int ch)
{
  int i;

  for(i = 0; i < nNfb; i++){

    BwVector[ch][i] = mapInvfMode (invFiltMode[i],
                                   prevInvFiltMode[i]);

    if(BwVector[ch][i] < BwVectorOld[ch][i])
      BwVector[ch][i] = 0.75000f * BwVector[ch][i] + 0.25000f * BwVectorOld[ch][i];
    else
      BwVector[ch][i] = 0.90625f * BwVector[ch][i] + 0.09375f * BwVectorOld[ch][i];

    if(BwVector[ch][i] < 0.015625)
      BwVector[ch][i] = 0;

    if(BwVector[ch][i] >= 0.99609375f)
      BwVector[ch][i] = 0.99609375f;
    
  }
}

static int findClosestEntry(int goalSb, int *v_k_master, int numMaster, int direction)
{
  int index;

  if( goalSb <= v_k_master[0] )
    return v_k_master[0];

  if( goalSb >= v_k_master[numMaster] )
    return v_k_master[numMaster];

  if(direction) {
    index = 0;
    while( v_k_master[index] < goalSb ) {
      index++;
    }
  } else {
    index = numMaster;
    while( v_k_master[index] > goalSb ) {
      index--;
    }
  }

  return v_k_master[index];

}

/*******************************************************************************
 Functionname:  generateHF
 *******************************************************************************

 Description:   HF generator with built-in QMF bank inverse filtering function.

 Arguments:

 Return:        none

*******************************************************************************/
void
generateHF (float sourceBufferReal[][64],
            float sourceBufferImag[][64],
            float targetBufferReal[][64],
            float targetBufferImag[][64],
            INVF_MODE *invFiltMode,
            INVF_MODE *prevInvFiltMode,
            int *invFiltBandTable,
            int noInvFiltBands,
            int highBandStartSb,
            int *v_k_master,
            int numMaster,
            int fs,
            int* frameInfo,
#ifdef LOW_POWER_SBR
            float *degreeAlias,
#endif
            int ch,
            float drcFactorVector[64][64])
{
  int    bwIndex, i, loBand, hiBand, patch;
  int    autoCorrStart, autoCorrLength;
  int    startSample, stopSample, goalSb;
  int    targetStopBand, sourceStartBand, patchDistance, numBandsInPatch;

  float  alphar[64][2], alphai[64][2];
  float  a0r, a0i, a1r, a1i;

  struct ACORR_COEFS ac;

  int    lsb = v_k_master[0];                           /* Lowest subband related to the synthesis filterbank */
  int    usb = v_k_master[numMaster];                   /* Stop subband related to the synthesis filterbank */
  int    xoverOffset = highBandStartSb - v_k_master[0]; /* Calculate distance in subbands between k0 and kx */

#ifdef LOW_POWER_SBR
  float k1[64];
#endif

  float  bw = 0.0f;
  float  fac = 0.0f;


  int    slopeLength = 0;
  int    buffOffset  = 0;

  int firstSlotOffs = frameInfo[1];
  int lastSlotOffs  = frameInfo[frameInfo[0]+1] - 16;

  static int masterInit = 0;
  static int init[MAX_NUM_CHANNELS];


   if(masterInit == 0){
    for(i=0;i<MAX_NUM_CHANNELS;i++){
      init[i] = 0;
    }
    masterInit = 1;
  }

  if(init[ch] == 0){
    init[ch] = 1;
    
    memset(BwVectorOld[ch],0,sizeof(int)*MAX_NUM_PATCHES);
    memset(prevInvFiltMode,0,sizeof(INVF_MODE)*MAX_NUM_NOISE_VALUES);
  }




  startSample = buffOffset + firstSlotOffs * 2;
  stopSample  = buffOffset + 32 + lastSlotOffs * 2;
 
    

  inverseFilteringLevelEmphasis( invFiltMode,
                                 prevInvFiltMode,
                                 noInvFiltBands,
                                 ch);

  /* Set subbands to zero */
  for ( i = startSample; i < stopSample; i++ ) {
    memset (targetBufferReal[i], 0, 64 * sizeof (float));
    memset (targetBufferImag[i], 0, 64 * sizeof (float));
  }

  autoCorrStart  = buffOffset;
  autoCorrLength = 38;

  for ( loBand = 1; loBand < v_k_master[0]; loBand++ ) {

    calcAutoCorr (&ac,
                  &sourceBufferReal[autoCorrStart],
                  &sourceBufferImag[autoCorrStart],
                  loBand,
                  autoCorrLength);

    if (ac.det < 1e-38f) {
      alphar[loBand][1] = alphai[loBand][1] = 0;
    } else {
      fac = 1.0f / ac.det;
      alphar[loBand][1] = ( ac.r01r * ac.r12r - ac.r01i * ac.r12i - ac.r02r * ac.r11r ) * fac;
      alphai[loBand][1] = ( ac.r01i * ac.r12r + ac.r01r * ac.r12i - ac.r02i * ac.r11r ) * fac;
    }

    if ( ac.r11r == 0 ) {
      alphar[loBand][0] =  alphai[loBand][0] = 0;
    } else {
      fac = 1.0f / ac.r11r;
      alphar[loBand][0] = - ( ac.r01r + alphar[loBand][1] * ac.r12r + alphai[loBand][1] * ac.r12i ) * fac;
      alphai[loBand][0] = - ( ac.r01i + alphai[loBand][1] * ac.r12r - alphar[loBand][1] * ac.r12i ) * fac;
    }

    /* prevent for shootovers */
    if ( alphar[loBand][0]*alphar[loBand][0] + alphai[loBand][0]*alphai[loBand][0] >= 16.0f ||
         alphar[loBand][1]*alphar[loBand][1] + alphai[loBand][1]*alphai[loBand][1] >= 16.0f )
    {
      alphar[loBand][0] = 0.0f;
      alphai[loBand][0] = 0.0f;
      alphar[loBand][1] = 0.0f;
      alphai[loBand][1] = 0.0f;
    }

#ifdef LOW_POWER_SBR
    if(ac.r11r==0.0) {
      k1[loBand] = 0.0f;
    }else {
      k1[loBand] = -(ac.r01r/ac.r11r);
      if(k1[loBand]>1.0) k1[loBand] = 1.0f;
      if(k1[loBand]<-1.0) k1[loBand] = -1.0f;
    }
    
#endif


  }


#ifdef LOW_POWER_SBR

#define REWRITE_DEG_ALIAS
#undef REWRITE_DEG_ALIAS

#ifdef REWRITE_DEG_ALIAS

  {
    int sign, k;
    int k0 = v_k_master[0];
    float deg[64];
    float ref[64];

    for(k=0;k<64;k++){
      ref[k] = k1[k];
    }

    
    ref[0] = 0.0;
    deg[1] = 0.0;
    
    for(k= 2; k < k0; k++){
      deg[k] = 0;
      if(k%2 == 0 && ref[k] <0){
        sign = 1;
      }
      else{
        if(k%2 == 1 && ref[k] > 0){
          sign = -1;
        }
        else{
          continue;
        }
      }

      if( sign*ref[k-1] < 0){
        deg[k] = 1;
        if(sign*ref[k-2] > 0){
          deg[k-1] = 1- ref[k-1]*ref[k-1];
        }
      }
      else{
        if(sign*ref[k-2] > 0){
          deg[k] = 1- ref[k-1]*ref[k-1];
        }
      }
    }

    k1[0] = 0.0;
    degreeAlias[1] = 0.0;
    for ( loBand = 2; loBand < v_k_master[0]; loBand++ ){
      degreeAlias[loBand] = 0.0;
      if ((loBand % 2 == 0) && (k1[loBand] < 0.0)){
        if (k1[loBand-1] < 0.0) { // 2-CH Aliasing Detection
          degreeAlias[loBand]   = 1.0;
          if ( k1[loBand-2] > 0.0 ) { // 3-CH Aliasing Detection
            degreeAlias[loBand-1] = 1.0f - k1[loBand-1] * k1[loBand-1];
          }
        }
        else if ( k1[loBand-2] > 0.0 ) { // 3-CH Aliasing Detection
          degreeAlias[loBand]   = 1.0f - k1[loBand-1] * k1[loBand-1];
        }
      }
      if ((loBand % 2 == 1) && (k1[loBand] > 0.0)){
        if (k1[loBand-1] > 0.0) { // 2-CH Aliasing Detection
          degreeAlias[loBand]   = 1.0f;
          if ( k1[loBand-2] < 0.0 ) { // 3-CH Aliasing Detection
            degreeAlias[loBand-1] = 1.0f - k1[loBand-1] * k1[loBand-1];
          }
        }
        else if ( k1[loBand-2] < 0.0 ) { // 3-CH Aliasing Detection
          degreeAlias[loBand]   = 1.0f - k1[loBand-1] * k1[loBand-1];
        }
      }
    }


    /* CHECK THE RESULT*/
    for(k=1;k<v_k_master[0];k++){
      if(degreeAlias[k] != deg[k])
        printf("\n APAN SABA ÄTER DEG!!!\n");

      assert(degreeAlias[k] == deg[k]);
    }


  }
#else

  k1[0] = 0.0;
  degreeAlias[1] = 0.0;
  for ( loBand = 2; loBand < v_k_master[0]; loBand++ ){
    degreeAlias[loBand] = 0.0;
    if ((loBand % 2 == 0) && (k1[loBand] < 0.0)){
      if (k1[loBand-1] < 0.0) { /* 2-CH Aliasing Detection */
        degreeAlias[loBand]   = 1.0;
        if ( k1[loBand-2] > 0.0 ) { /* 3-CH Aliasing Detection */
          degreeAlias[loBand-1] = 1.0f - k1[loBand-1] * k1[loBand-1];
        }
      }
      else if ( k1[loBand-2] > 0.0 ) { /* 3-CH Aliasing Detection */
        degreeAlias[loBand]   = 1.0f - k1[loBand-1] * k1[loBand-1];
      }
    }
    if ((loBand % 2 == 1) && (k1[loBand] > 0.0)){
      if (k1[loBand-1] > 0.0) { /* 2-CH Aliasing Detection */
        degreeAlias[loBand]   = 1.0f;
        if ( k1[loBand-2] < 0.0 ) { /* 3-CH Aliasing Detection */
          degreeAlias[loBand-1] = 1.0f - k1[loBand-1] * k1[loBand-1];
        }
      }
      else if ( k1[loBand-2] < 0.0 ) { /* 3-CH Aliasing Detection */
        degreeAlias[loBand]   = 1.0f - k1[loBand-1] * k1[loBand-1];
      }
    }
  }
#endif
/*
  printf("\n");
  for ( loBand = 1; loBand < v_k_master[0]; loBand++ ){
    printf("%f ",degreeAlias[loBand]);
  }
  printf("\n");
  */
#endif 

  /*
   * Initialize the patching parameter
   */
  goalSb = (int)( 2.048e6f / fs + 0.5f );                      /* 16 kHz band */
  goalSb = findClosestEntry(goalSb, v_k_master, numMaster, 1); /* Adapt region to master-table */


  /* First patch */
  sourceStartBand = xoverOffset + 1;
  targetStopBand = lsb + xoverOffset;

  /* even (odd) numbered channel must be patched to even (odd) numbered channel */
  patch = 0;
  while(targetStopBand < usb) {

    Patch[ch].targetStartBand[patch] = targetStopBand;

    numBandsInPatch = goalSb - targetStopBand;                   /* get the desired range of the patch */

    if ( numBandsInPatch >= lsb - sourceStartBand ) {
      /* desired number bands are not available -> patch whole source range */
      patchDistance   = targetStopBand - sourceStartBand;        /* get the targetOffset */
      patchDistance   = patchDistance & ~1;                      /* rounding off odd numbers and make all even */
      numBandsInPatch = lsb - (targetStopBand - patchDistance);
      numBandsInPatch = findClosestEntry(targetStopBand + numBandsInPatch, v_k_master, numMaster, 0) -
                        targetStopBand;  /* Adapt region to master-table */
    }

    /* desired number bands are available -> get the minimal even patching distance */
    patchDistance   = numBandsInPatch + targetStopBand - lsb;  /* get minimal distance */
    patchDistance   = (patchDistance + 1) & ~1;                /* rounding up odd numbers and make all even */

    /* All patches but first */
    sourceStartBand = 1;


    /* Check if we are close to goalSb */
    /* if( abs(targetStopBand + numBandsInPatch - goalSb) < 3) { */
    if( goalSb - (targetStopBand + numBandsInPatch) < 3) { /* MPEG doc */
      goalSb = usb;
    }



    if ( (numBandsInPatch < 3) && (patch > 0) ) {
#ifdef LOW_POWER_SBR
      for (hiBand = targetStopBand; hiBand < targetStopBand + numBandsInPatch; hiBand++ ) {
        degreeAlias[hiBand] = 0;
      }
#endif

      break;
    }
    
    if ( numBandsInPatch <= 0 ) {
      continue;
    }

    for (hiBand = targetStopBand; hiBand < targetStopBand + numBandsInPatch; hiBand++ ) {

      loBand = hiBand - patchDistance;

#ifdef LOW_POWER_SBR
      if(hiBand != targetStopBand)
        degreeAlias[hiBand] = degreeAlias[loBand];
      else
        degreeAlias[hiBand] = 0;
#endif


      bwIndex = 0;
      while (hiBand >= invFiltBandTable[bwIndex])
        bwIndex++;

      bw = BwVector[ch][bwIndex];

      a0r = bw * alphar[loBand][0]; /* Apply current bandwidth expansion factor */
      a0i = bw * alphai[loBand][0];
      bw *= bw;
      a1r = bw * alphar[loBand][1];
      a1i = bw * alphai[loBand][1];


      for(i = startSample + slopeLength; i < stopSample + slopeLength; i++ ) {
        targetBufferReal[i][hiBand] = sourceBufferReal[i][loBand];
      
#ifdef LOW_POWER_SBR
        targetBufferImag[i][hiBand] = 0.0f;
#else
        targetBufferImag[i][hiBand] = sourceBufferImag[i][loBand];
#endif
 
        if ( bw > 0.0f ) {
          targetBufferReal[i][hiBand] += ( a0r * sourceBufferReal[i-1][loBand] -
                                           a0i * sourceBufferImag[i-1][loBand] +
                                           a1r * sourceBufferReal[i-2][loBand] -
                                           a1i * sourceBufferImag[i-2][loBand] );
#ifndef LOW_POWER_SBR
          targetBufferImag[i][hiBand] += ( a0i * sourceBufferReal[i-1][loBand] +
                                           a0r * sourceBufferImag[i-1][loBand] +
                                           a1i * sourceBufferReal[i-2][loBand] +
                                           a1r * sourceBufferImag[i-2][loBand] );
#endif
        }

#ifndef QMF_BASED_DRC

        {
          int fIndex;
          float factor;
/*
          if(i < 26)
            fIndex = 1;
          else
            fIndex = 2;
  */          
          factor = drcFactorVector[32 - 26 + i][loBand];

          targetBufferReal[i][hiBand] /= (factor + EPS);
          targetBufferImag[i][hiBand] /= (factor + EPS);
        }
#endif


      }

    }  /* hiBand */

    targetStopBand += numBandsInPatch;

    patch++;

  }  /* targetStopBand */


  Patch[ch].noOfPatches = patch;

  for (i = 0; i < noInvFiltBands; i++ ) {
    BwVectorOld[ch][i] = BwVector[ch][i];
  }
}
