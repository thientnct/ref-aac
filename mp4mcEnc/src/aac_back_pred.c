/**********************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Nokia

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
Copyright (c) ISO/IEC 1997.
**********************************************************************/
/*********************************************************************
 *
 * aac_back_pred.c  -  AAC backward prediction subroutine
 *
 * Authors:
 * LY    Lin Yin, Nokia Research Center
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * ADD   Alberto Duenas, NDS <alberto@ndsuk.com)
 * JB    Jeremy Bennett, NDS <jbennett@ndsuk.com>
 *
 * Changes:
 * 07-jun-97   LY   Initial revision.
 * 14-sep-97   CL   Made predicted samples array, sb_samples_pred, static.
 *                  Modified line 98 to add these predicted values to
 *                  the last spectrum prior to analysis.
 * 03-dec-97   ADD & JB   Added prediction reset.
 * 21-jul-98   CL   Fixed prediction in right channels with common_window.
 *                  In this situation, the predictor data from the left channel
 *                  will be used.  User beware, a precondition of this function 
 *                  is that the prediction data in the left channel be valid.
 *
**********************************************************************/

/**********************************************************************

INPUT:
act_spec[]: the current frame's spectral components to be encoded.
last_spec[]: the previous frame's quantized spectral error components 
btype: the current frame's window type
nsfb: the number of scalefactor bands
isfb_width[]: scalefactor bandwidth

OUTPUT:
act_spec[]: the current frame's spectral error components to be encoded
pred_global_flag: global prediction enable/disable flag
pred_sbf_glag[]: enable/disable flag for each scalefactor band
reset_group: number for the reset group => if -1, then no reset
***********************************************************************/

#include "aac_back_pred.h"
#include <stdio.h>

/* Make these work for mulitchannels */  /* CL, 1/98 */
/*static int psy_init = 0;
static float dr[LPC][FLEN_LONG/2],e[LPC+1+1][FLEN_LONG/2];
static float K[LPC+1][FLEN_LONG/2], R[LPC+1][FLEN_LONG/2];
static float VAR[LPC+1][FLEN_LONG/2], KOR[LPC+1][FLEN_LONG/2];
static float sb_samples_pred[FLEN_LONG/2];
static int thisLineNeedsResetting[FLEN_LONG/2];
static int reset_count = 0;*/


static int psy_init_mc[MAX_TIME_CHANNELS];
static float dr_mc[MAX_TIME_CHANNELS][LPC][FLEN_LONG/2],e_mc[MAX_TIME_CHANNELS][LPC+1+1][FLEN_LONG/2];
static float K_mc[MAX_TIME_CHANNELS][LPC+1][FLEN_LONG/2], R_mc[MAX_TIME_CHANNELS][LPC+1][FLEN_LONG/2];
static float VAR_mc[MAX_TIME_CHANNELS][LPC+1][FLEN_LONG/2], KOR_mc[MAX_TIME_CHANNELS][LPC+1][FLEN_LONG/2];
static float sb_samples_pred_mc[MAX_TIME_CHANNELS][FLEN_LONG/2];
static int thisLineNeedsResetting_mc[MAX_TIME_CHANNELS][FLEN_LONG/2];
static int reset_count_mc[MAX_TIME_CHANNELS];

/* Max prediction band as function of fs index */
int MAX_PRED_SFB[][2] = {{8000,34},
		     {11025,37},
		     {12000,37}, 
		     {16000,37}, 
		     {22050,41}, 
		     {24000,41},
		     {32000,40},
		     {44100,40},
		     {48000,40},
		     {64000,38},
		     {88200,33},
		     {96000,33},
		     {-1,0}};

int GetMaxPredSfb(int samplingRate) {
  int i,max_pred_band;

  i=0;
  while (1) {
    if (MAX_PRED_SFB[i][0]==samplingRate) {
      max_pred_band = MAX_PRED_SFB[i][1];
      return max_pred_band;
      break;
    } else if (MAX_PRED_SFB[i][0] == -1 ) {
      CommonExit(1,"GetMaxPredSfb: unsupported sampling rate");
    }
    i++;
  }
}



void PredInit(void) {
  int i;
  for (i=0;i<MAX_TIME_CHANNELS;i++) {
    psy_init_mc[i] = 0;
    reset_count_mc[i] = 0;
  }
}
 
void PredCalcPrediction( double *act_spec, double *last_spec, int btype, 
			int nsfb, 
			int *isfb_width, 
			AACQuantInfo *quantInfo,
			Ch_Info *channelInfo,                  /* Pointer to Ch_Info */
			int chanNum) 
{
  int i, k, j, cb_long;
  int leftChanNum = 0;
  int isRightWithCommonWindow;
  float small, small_p, num_bit, snr[SBMAX_L];
  float energy[FLEN_LONG/2], snr_p[FLEN_LONG/2], temp1, temp2;
  Ch_Info *thisChannel;

  /* Set pointers for specified channel number */
  /* int psy_init; */
  int *psy_init;
  float (*dr)[FLEN_LONG/2],(*e)[FLEN_LONG/2];
  float (*K)[FLEN_LONG/2], (*R)[FLEN_LONG/2];
  float (*VAR)[FLEN_LONG/2], (*KOR)[FLEN_LONG/2];
  float *sb_samples_pred;
  int *thisLineNeedsResetting;
  /* int reset_count; */
  int *reset_count;
  short *pred_global_flag;
  int *pred_sfb_flag;
  int *reset_group;

  /* Set pointers for this chanNum */
  pred_global_flag = &(quantInfo[chanNum].pred_global_flag);
  pred_sfb_flag = quantInfo[chanNum].pred_sfb_flag;
  reset_group = &(quantInfo[chanNum].reset_group_number);
  /* psy_init = psy_init_mc[chanNum]; */
  psy_init = &psy_init_mc[chanNum];
  dr = &dr_mc[chanNum][0];
  e = &e_mc[chanNum][0];
  K = &K_mc[chanNum][0]; 
  R = &R_mc[chanNum][0];
  VAR = &VAR_mc[chanNum][0];
  KOR = &KOR_mc[chanNum][0];
  sb_samples_pred = &sb_samples_pred_mc[chanNum][0];
  thisLineNeedsResetting = &thisLineNeedsResetting_mc[chanNum][0];
  reset_count = &reset_count_mc[chanNum];

  thisChannel = &(channelInfo[chanNum]);
  *psy_init = (*psy_init && (btype!=2));

  if((*psy_init) == 0) {
    for (j=0; j<FLEN_LONG/2; j++) {
      thisLineNeedsResetting[j]=1;
    }
    *psy_init = 1;
  }

  if (btype==2) {
    pred_global_flag[0]=0;
    /* SHORT WINDOWS reset all the co-efficients    */
    if (thisChannel->ch_is_left) {
      (*reset_count)++;
      if (*reset_count >= 31 * RESET_FRAME)
	*reset_count = RESET_FRAME;
      /*    *reset_count = ((*reset_count) / RESET_FRAME) * RESET_FRAME;*/
    }
    return;
  }


  /**************************************************/
  /*  Compute state using last_spec                 */
  /**************************************************/
  for (i=0;i<FLEN_LONG/2;i++) 
    {
      /* e[0][i]=last_spec[i]; */ 
      e[0][i]=last_spec[i]+sb_samples_pred[i];
      
      for(j=1;j<=LPC;j++)
	e[j][i] = e[j-1][i]-K[j][i]*R[j-1][i];
      
      for(j=1;j<LPC;j++) 
	dr[j][i] = K[j][i]*e[j-1][i];
      
      for(j=1;j<=LPC;j++) {
	VAR[j][i] = ALPHA*VAR[j][i]+.5*(R[j-1][i]*R[j-1][i]+e[j-1][i]*e[j-1][i]);
	KOR[j][i] = ALPHA*KOR[j][i]+R[j-1][i]*e[j-1][i];
      }

      for(j=LPC-1;j>=1;j--) 
	R[j][i] = A*(R[j-1][i]-dr[j][i]);
      R[0][i] = A*e[0][i];
    }


  /**************************************************/
  /* Reset state here if resets were sent           */
  /**************************************************/
   for (i=0;i<FLEN_LONG/2;i++) {
     if (thisLineNeedsResetting[i]) {
       for (j = 0; j <= LPC; j++)
	 {
	   K[j][i] = 0.0;
	   e[j][i] = 0.0;
	   R[j][i] = 0.0;
	   VAR[j][i] = 1.0;
	   KOR[j][i] = 0.0;
	   dr[j][i] = 0.0;
	 }
     }
   }



  /**************************************************/
  /* Compute predictor coefficients, predicted data */
  /**************************************************/
   for (i=0;i<FLEN_LONG/2;i++) 
    {
      for(j=1;j<=LPC;j++) {
	if(VAR[j][i]>MINVAR)
	  K[j][i] = KOR[j][i]/VAR[j][i]*B;
	else
	  K[j][i] = 0;
      }
    }


   for (k=0; k<FLEN_LONG/2; k++)
    {
      sb_samples_pred[k]=0.0;
      for (i=1; i<=LPC; i++)
	sb_samples_pred[k]+=K[i][k]*R[i-1][k];
    }


  /***********************************************************/
  /* If this is the right channel of a channel_pair_element, */
  /* AND common_window is 1 in this channel_pair_element,    */
  /* THEN copy predictor data to use from the left channel.  */
  /* ELSE determine independent predictor data and resets.   */
  /***********************************************************/
  /* BE CAREFUL HERE, this assumes that predictor data has   */
  /* already been determined for the left channel!!          */
  /***********************************************************/
  isRightWithCommonWindow = 0;     /* Is this a right channel with common_window?*/
  if ((thisChannel->cpe)&&( !(thisChannel->ch_is_left))) {
    leftChanNum = thisChannel->paired_ch;
    if (channelInfo[leftChanNum].common_window) {
      isRightWithCommonWindow = 1;
    }
  }

  if (isRightWithCommonWindow) {
    
    /**************************************************/
    /* Use predictor data from the left channel.      */
    /**************************************************/
    CopyPredInfo(&(quantInfo[chanNum]),&(quantInfo[leftChanNum]));

    /* Make sure to turn off bands with intensity stereo */
    if (thisChannel->is_info.is_present) {
      for (i=0; i<nsfb; i++) {
	if (thisChannel->is_info.is_used[i]) {
	  pred_sfb_flag[i] = 0;
	}
      }
    }
    
    cb_long=0;
    for (i=0; i<nsfb; i++)
      {
	if (!pred_sfb_flag[i]) {
	  for (j=cb_long; j<cb_long+isfb_width[i]; j++)
	    sb_samples_pred[j]=0.0; 
	}
	cb_long+=isfb_width[i];
      }
    
    /* Disable prediction for bands nsfb through SBMAX_L */ 
    for (i=j;i<FLEN_LONG/2;i++) {
      sb_samples_pred[i]=0.0;
    }
    for (i=nsfb;i<SBMAX_L;i++) {
      pred_sfb_flag[i]=0;
    }
    
    /* Is global enable set, if not enabled predicted samples are zeroed */
    if(!pred_global_flag[0]) {
      for (j=0; j<FLEN_LONG/2; j++)
	sb_samples_pred[j]=0.0; 
    }
    for (j=0; j<FLEN_LONG/2; j++)
      act_spec[j]-=sb_samples_pred[j];
    
  } else {

    /**************************************************/
    /* Determine whether to enable/disable prediction */
    /**************************************************/
    
    for (k=0; k<FLEN_LONG/2; k++)
      {energy[k]=act_spec[k]*act_spec[k];
       snr_p[k]=(act_spec[k]-sb_samples_pred[k])*(act_spec[k]-sb_samples_pred[k]);
     }
    
    cb_long=0;
    for (i=0; i<nsfb; i++)
      {
	pred_sfb_flag[i]=1;
	temp1=0.0;
	temp2=0.0;
	for (j=cb_long; j<cb_long+isfb_width[i]; j++)
	  {temp1+=energy[j]; temp2+=snr_p[j];}
	if(temp2<1.e-20) temp2=1.e-20;
	if(temp1!=0.0) {snr[i]=-10.*log10((double ) temp2/temp1);}
	else snr[i]=0.0;
	if(snr[i]<=0.0) {pred_sfb_flag[i]=0; 
			 for (j=cb_long; j<cb_long+isfb_width[i]; j++)
			   sb_samples_pred[j]=0.0; }
	cb_long+=isfb_width[i];
      }
    
    /* Disable prediction for bands nsfb through SBMAX_L */ 
    for (i=j;i<FLEN_LONG/2;i++) {
      sb_samples_pred[i]=0.0;
    }
    for (i=nsfb;i<SBMAX_L;i++) {
      pred_sfb_flag[i]=0;
    }

    num_bit=0.0;
    for (i=0; i<nsfb; i++)
      if(snr[i]>0.0) num_bit+=snr[i]/6.*isfb_width[i];	
    
    /* Determine global enable, if not enabled predicted samples are zeroed */
    pred_global_flag[0]=1;
    if(num_bit<50) {
      pred_global_flag[0]=0; num_bit=0.0; 
      for (j=0; j<FLEN_LONG/2; j++)
	sb_samples_pred[j]=0.0; 
    }
    for (j=0; j<FLEN_LONG/2; j++)
      act_spec[j]-=sb_samples_pred[j];
    
  }
  
  /**********************************************************/
  /* If this is a left channel, determine pred resets.      */
  /* If this is a right channel, using pred reset data from */
  /* left channel.  Keep left and right resets in sync.     */
  /**********************************************************/
  if ((thisChannel->cpe)&&( !(thisChannel->ch_is_left))) {
    /*  if (!thisChannel->ch_is_left) {*/
    /**********************************************************/
    /* Using predictor reset data from the left channel.      */
    /**********************************************************/
    reset_count = &reset_count_mc[leftChanNum];
    /* Reset the frame counter */
    for (i=0;i<FLEN_LONG/2;i++) {
      thisLineNeedsResetting[i]=0;
    }
    reset_group = &(quantInfo[chanNum].reset_group_number);
    if (*reset_count % RESET_FRAME == 0)
      { /* Send a reset in this frame */
	*reset_group = *reset_count / 8;
	for (i = *reset_group - 1; i < FLEN_LONG / 2; i += 30)
	  {
	    thisLineNeedsResetting[i]=1;
	  }
      }
    else
      *reset_group = -1;
  } else {
    /* Code segment added by JB */
    /******************************************************************/
    /* Determine whether a prediction reset is required - if so, then */
    /* set reset flag for the appropriate group.                      */
    /******************************************************************/
    
    /* Increase counter on left channel, keep left and right resets in sync */
    (*reset_count)++;

    /* Reset the frame counter */
    for (i=0;i<FLEN_LONG/2;i++) {
      thisLineNeedsResetting[i]=0;
    }
    if (*reset_count >= 31 * RESET_FRAME)
      *reset_count = RESET_FRAME;
    if (*reset_count % RESET_FRAME == 0)
      { /* Send a reset in this frame */
	*reset_group = *reset_count / 8;
	for (i = *reset_group - 1; i < FLEN_LONG / 2; i += 30)
	  {
	    thisLineNeedsResetting[i]=1;
	  }
      }
    else
      *reset_group = -1;
    /* End of code segment */
 }


  /* Code segment added by JB */

  /* Ensure that prediction data is sent when there is a prediction
   * reset.
   */
  if (*reset_group != -1 && pred_global_flag[0] == 0)
    {
      pred_global_flag[0] = 1;
      for (i = 0; i < nsfb; i++)
	pred_sfb_flag[i] = 0;
    }
  /* End of code segment */
  
}


void CopyPredInfo(AACQuantInfo *right,AACQuantInfo *left) {

  int band;

  right->pred_global_flag = left->pred_global_flag;
  right->reset_group_number = left->reset_group_number;

  for (band = 0; band<SFB_NUM_MAX; band++) {
    right->pred_sfb_flag[band] = left->pred_sfb_flag[band];
  }

}




