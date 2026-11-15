/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Texas Instruments

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
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

/*******************************************************************************************
 *
 * MS stereo coding module
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 *
 * Changes:
 * 22-jan-98   CL   Initial revision.  Lacks psychoacoustics for MS stereo.
 * 06-sep-98   CL   Disable MS stereo for bands which use PNS (MPEG4 only).
 ***************************************************************************************/

#include "ms.h"
#include "aac_qc.h"

/* Perform MS encoding.  Spectrum is non-interleaved.  */
/* This would be a lot simpler on interleaved spectral data */
void MSEncode(double *spectrum[MAX_TIME_CHANNELS],   /* array of pointers to spectral data */
	      Ch_Info *channelInfo,                  /* Pointer to Ch_Info */
	      int sfb_offset_table[][MAX_SCFAC_BANDS+1],
	      enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS], /* Block type */
	      AACQuantInfo* quantInfo,               /* Quant info */
	      int numberOfChannels,               /* Number of channels */
	      short msenable) {              

  int chanNum;
  int sfbNum;
  int lineNum;
  double sum,diff;

  /* Look for channel_pair_elements */
  for (chanNum=0;chanNum<numberOfChannels;chanNum++) {
    if (channelInfo[chanNum].present) {
      if ((channelInfo[chanNum].cpe)&&(channelInfo[chanNum].ch_is_left)) {
	int leftChan=chanNum;
        int rightChan=channelInfo[chanNum].paired_ch;
	channelInfo[leftChan].ms_info.is_present=0;

	/* Perform MS if block_types are the same */
	if ((block_type[leftChan]==block_type[rightChan])&&(msenable)) { 
	  int numGroups;
          int maxSfb;
	  int maxMSBand;
	  int g,b,w,line_offset;
	  int startWindow,stopWindow;
	  IS_Info *isInfo;
	  MS_Info *msInfo;

	  channelInfo[leftChan].common_window = 1;  /* Use common window */
	  channelInfo[leftChan].ms_info.is_present=1;

	  numGroups = quantInfo[leftChan].num_window_groups;
          maxSfb = quantInfo[leftChan].max_sfb;
	  w=0;

	  /* Determine which bands should be enabled */
	  /* Right now, simply enable bands which are below the pns start band */
	  /* and which do not use intensity stereo */
	  isInfo = &(channelInfo[rightChan].is_info);
	  msInfo = &(channelInfo[leftChan].ms_info);
#ifdef MPEG4V1
	  if (block_type[leftChan]!=ONLY_SHORT_WINDOW) {
	    maxMSBand = (pns_sfb_start>maxSfb) ? maxSfb : pns_sfb_start;
	    for (sfbNum=0;sfbNum<maxMSBand;sfbNum++) {
	      msInfo->ms_used[sfbNum] = ( (!isInfo->is_used[sfbNum])||(!isInfo->is_present) );
	    }
	    for (sfbNum=maxMSBand;sfbNum<maxSfb;sfbNum++) {
	      msInfo->ms_used[sfbNum] = 0;
	    }
	  } else {
#endif
	    for (g=0;g<numGroups;g++) {
	      for (sfbNum=0;sfbNum<maxSfb;sfbNum++) {
		b = g*maxSfb+sfbNum;
		msInfo->ms_used[b] = ( (!isInfo->is_used[b])||(!isInfo->is_present) );
	      }
	    }
#ifdef MPEG4V1
	  }
#endif
 
	  /* Perform sum and differencing on bands in which ms_used flag */
          /* has been set. */
	  line_offset=0;
	  startWindow = 0;
	  for (g=0;g<numGroups;g++) {
	    int numWindows = quantInfo[leftChan].window_group_length[g];
	    stopWindow = startWindow + numWindows;
	    for (sfbNum=0;sfbNum<maxSfb;sfbNum++) {
	      /* Enable MS mask */
	      if (msInfo->ms_used[g*maxSfb+sfbNum]) {
		for (w=startWindow;w<stopWindow;w++) {
		  for (lineNum=sfb_offset_table[leftChan][sfbNum];
		       lineNum<sfb_offset_table[leftChan][sfbNum+1];
		       lineNum++) {
		    line_offset = w*BLOCK_LEN_SHORT;
		    sum=spectrum[leftChan][line_offset+lineNum]+spectrum[rightChan][line_offset+lineNum];
		    diff=spectrum[leftChan][line_offset+lineNum]-spectrum[rightChan][line_offset+lineNum];
		    spectrum[leftChan][line_offset+lineNum] = 0.5 * sum;
		    spectrum[rightChan][line_offset+lineNum] = 0.5 * diff;
		  }  /* for (lineNum... */
		}  /* for (w=... */
	      }
	    }  /* for (sfbNum... */
	    startWindow = stopWindow;
	  } /* for (g... */
	}  /* if (block_type... */
      }
    }  /* if (channelInfo[chanNum].present */
  }  /* for (chanNum... */
}



void MSReconstruct(double *spectrum[MAX_TIME_CHANNELS],   /* array of pointers to spectral data */
		   Ch_Info *channelInfo,                  /* Pointer to Ch_Info */
		   int sfb_offset_table[][MAX_SCFAC_BANDS+1],
		   enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS], /* Block type */
		   AACQuantInfo* quantInfo,               /* Quant info */
		   int numberOfChannels) {                /* Number of channels */

  int chanNum;
  int sfbNum;
  int lineNum;
  double sum,diff;

  /* Look for channel_pair_elements */
  for (chanNum=0;chanNum<numberOfChannels;chanNum++) {
    if (channelInfo[chanNum].present) {
      if ((channelInfo[chanNum].cpe)&&(channelInfo[chanNum].ch_is_left)) {
	int leftChan=chanNum;
        int rightChan=channelInfo[chanNum].paired_ch;

	MS_Info *msInfo;
	msInfo = &(channelInfo[leftChan].ms_info);
	if (msInfo->is_present) {
	  int numGroups = quantInfo[leftChan].num_window_groups;
          int maxSfb = quantInfo[leftChan].max_sfb;
	  int g,b,w,line_offset;
	  int startWindow,stopWindow;
	  w=0;

	  /* Perform sum and differencing on bands in which ms_used flag */
          /* has been set. */
	  line_offset=0;
	  startWindow = 0;
	  for (g=0;g<numGroups;g++) {
	    int numWindows = quantInfo[leftChan].window_group_length[g];
	    stopWindow = startWindow + numWindows;
	    for (sfbNum=0;sfbNum<maxSfb;sfbNum++) {
	      /* Enable MS mask */
	      if ((msInfo->ms_used[g*maxSfb+sfbNum]) || (msInfo->is_present==2)) {
		for (w=startWindow;w<stopWindow;w++) {
		  line_offset = w*BLOCK_LEN_SHORT;
		  for (lineNum=sfb_offset_table[leftChan][sfbNum];
		       lineNum<sfb_offset_table[leftChan][sfbNum+1];
		       lineNum++) {
		    sum=spectrum[leftChan][line_offset+lineNum];
		    diff=spectrum[rightChan][line_offset+lineNum];
		    spectrum[leftChan][line_offset+lineNum] = (sum+diff);
		    spectrum[rightChan][line_offset+lineNum] = (sum-diff);
		  }  /* for (lineNum... */
		}  /* for (w=start... */
	      }  /* if (msInfo... */
	    }  /* for (sfbNum... */
	    startWindow = stopWindow;
	  } /* for (g... */
	}  /* if (ms_info.is_present... */
      }  /* if (channelInfo... */
    }  /* if (channelInfo[chanNum].present */
  }  /* for (chanNum... */
}




