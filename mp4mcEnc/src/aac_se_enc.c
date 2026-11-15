/**********************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Texas Instruments

and edited by
  NEC

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
 * Module for writing/counting AAC syntactic elements
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * ADD   Alberto Duenas, TI <alberto@ndsuk.com>
 * RG    Ralf Geiger,  FhG/IIS
 * Changes:
 * 07-jun-97   CL   Initial revision.
 * 14-sep-97   CL   Modified WritePredictorData to actually write
 *                  predictor data.  Still need to add resets.
 * 20-oct-97   CL   Updated WriteTNSData to support TNS.
 * 03-Dec-97   ADD  Addding the prediction reset
 * 22-Jan-98   CL   Added support for CPE's and common windows.
 * 07-Apr-98   RG   Added WriteLFE to write LFE channel element.
 * 27-May-98   MCF  Added independent coupling channel write routine. 
 * 24-July-98  MCF  Added bitstream packing routines for dynamic range
 *
**********************************************************************/

#include "aac_se_enc.h"
#include "aac_qc.h"

static int seDebugLevel;
static short diffexps[NSFB_LONG];

/* Set debugLevel */
void InitAACSyntax(int debugLevel) {
  seDebugLevel = debugLevel;
}

/*****************************************************************************/
/* WriteAACFillBits(...)                                                     */
/* Write fill_elements to the bitstream.	                             */
/* Number of bits written is LEN_SE_ID + FIL_COUNT_LEN + a multiple of 8.    */
/* Return number of bits left over (less than 7 ).                           */
/*****************************************************************************/
int WriteAACFillBits(BsBitStream* ptrBs,  /* Pointer to bitstream */
		     int numBits)         /* Number of bits needed to fill */
{
  int numberOfBitsLeft=numBits;
  short indx;
  short fill_byte = 0xA5;
  short fill_nibble = 0x0;

  /* Need at least (LEN_SE_ID + LEN_F_CNT) bits for a fill_element */
  int minNumberOfBits = LEN_SE_ID + LEN_F_CNT;
  while (numberOfBitsLeft>=minNumberOfBits) {
    int numberOfBytes;
    int maxCount;

    BsPutBit(ptrBs,ID_FIL,LEN_SE_ID);	/* Write fill_element ID */
    numberOfBitsLeft-=minNumberOfBits;	/* Subtract for ID,count */

    numberOfBytes=(int)(numberOfBitsLeft/LEN_BYTE);
    maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

    /* if we have less than maxCount bytes, write them now */
    if (numberOfBytes<maxCount) {
      int i;
      BsPutBit(ptrBs,numberOfBytes,LEN_F_CNT);
      for (i=0;i<numberOfBytes;i++) {
	BsPutBit(ptrBs,0,LEN_BYTE);
      }
    /* otherwise, we need to write an escape count */
    } else {
      int maxEscapeCount,maxNumberOfBytes,escCount;
      BsPutBit(ptrBs,maxCount,LEN_F_CNT);
      maxEscapeCount = (1<<LEN_BYTE) - 1;  /* Max escape count */
      maxNumberOfBytes = maxCount + maxEscapeCount;
      numberOfBytes = (numberOfBytes > maxNumberOfBytes ) ?
	(maxNumberOfBytes) : (numberOfBytes);
      escCount = numberOfBytes - maxCount;
      BsPutBit(ptrBs,escCount,LEN_BYTE);
	  BsPutBit(ptrBs, EXT_FILL_DATA, 4);	/* write extension type */
	  BsPutBit(ptrBs, fill_nibble, 4);		/* write extension type fill nibble */
	  for (indx = 0; indx < (numberOfBytes - 2); indx++)
	  {	
		 BsPutBit(ptrBs, fill_byte, 8); /* fill data */
	  }
    }
    numberOfBitsLeft -= LEN_BYTE*numberOfBytes;
  }
  return numberOfBitsLeft;
}

/*****************************************************************************/
/* WriteSCE(...), write a single-channel element to the bitstream.           */
/*****************************************************************************/
int WriteSCE(AACQuantInfo* quantInfo,   /* AACQuantInfo structure */
	     int tag,
	     BsBitStream* fixedStream,  /* Pointer to bitstream */
	     int writeFlag)             /* 1 means write, 0 means count only */
{
  int bit_count=0;

  if (writeFlag) {
    SE_DBUG(fprintf(stdout,"\nWriteSCE, initial sce bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));
    /* write ID_SCE, single_element_channel() identifier */
    BsPutBit(fixedStream,ID_SCE,LEN_SE_ID);  

    /* write the element_identifier_tag */
    BsPutBit(fixedStream,tag,LEN_TAG);  
  }
 
  bit_count += LEN_SE_ID;
  bit_count += LEN_TAG;
  SE_DBUG(if (writeFlag) fprintf(stdout,"\n   Wrote SCE_ID and tag,   sce bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));
    
  /* Write an individual_channel_stream element */
  bit_count += WriteICS(quantInfo,0,fixedStream,writeFlag);
 
  SE_DBUG(if (writeFlag) fprintf(stdout,"\n   Finished SCE,           sce bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));
  return bit_count;
}


/*****************************************************************************/
/* WriteLFE(...), write a lfe-channel element to the bitstream.              */
/*****************************************************************************/
int WriteLFE(AACQuantInfo* quantInfo,   /* AACQuantInfo structure */
	     int tag,
	     BsBitStream* fixedStream,  /* Pointer to bitstream */
	     int writeFlag)             /* 1 means write, 0 means count only */
{
  int bit_count=0;
  
  if (writeFlag) {
    /* write ID_LFE, lfe_element_channel() identifier */
    BsPutBit(fixedStream,ID_LFE,LEN_SE_ID);  

    /* write the element_identifier_tag */
    BsPutBit(fixedStream,tag,LEN_TAG);  
  }
 
  bit_count += LEN_SE_ID;
  bit_count += LEN_TAG;

  /* Write an individual_channel_stream element */
  bit_count += WriteICS(quantInfo,0,fixedStream,writeFlag);
 
  return bit_count;
}


/*****************************************************************************/
/* WriteCPE(...), write a channel_pair_element to the bitstream.           */
/*****************************************************************************/
int WriteCPE(AACQuantInfo* quantInfoL,   /* AACQuantInfo structure, left */
             AACQuantInfo* quantInfoR,   /* AACQuantInfo structure, right */
	     int tag,
	     int commonWindow,          /* common_window flag */
	     MS_Info* ms_info,          /* MS stereo information */
	     BsBitStream* fixedStream,  /* Pointer to bitstream */
	     int writeFlag)             /* 1 means write, 0 means count only */
{
  int bit_count=0;

  if (writeFlag) {
    SE_DBUG(fprintf(stdout,"\nWriteCPE, initial cpe bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));

    /* write ID_CPE, single_element_channel() identifier */
    BsPutBit(fixedStream,ID_CPE,LEN_SE_ID);  
    bit_count += LEN_SE_ID;
    SE_DBUG(fprintf(stdout,"\n   Wrote CPE_ID,           cpe bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));

    /* write the element_identifier_tag */
    BsPutBit(fixedStream,tag,LEN_TAG);  /* Currently, this is zero */
    bit_count += LEN_TAG;
    SE_DBUG(fprintf(stdout,"\n   Wrote tag,              cpe bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));

    /* common_window? */
    BsPutBit(fixedStream,commonWindow,LEN_COM_WIN);
    bit_count += LEN_COM_WIN;
    SE_DBUG(fprintf(stdout,"\n   Wrote common win flag,  cpe bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));
  } else {
    bit_count += LEN_SE_ID;
    bit_count += LEN_TAG;
    bit_count += LEN_COM_WIN;
  }

  /* if common_window, write ics_info */
  if (commonWindow) {
    int numWindows,maxSfb;
    bit_count += WriteICSInfo(quantInfoL,fixedStream,writeFlag, commonWindow, quantInfoR);
    numWindows=quantInfoL->num_window_groups;
    maxSfb = quantInfoL->max_sfb;
    if (writeFlag) {
      SE_DBUG(fprintf(stdout,"\n   Wrote ICS info,         cpe bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));
      BsPutBit(fixedStream,ms_info->is_present,LEN_MASK_PRES);
      if (ms_info->is_present==1) {
	int g;
	int b;
	for (g=0;g<numWindows;g++) {
	  for (b=0;b<maxSfb;b++) {
	    BsPutBit(fixedStream,ms_info->ms_used[g*maxSfb+b],LEN_MASK);
	  }
	}
      }
    }
    bit_count += LEN_MASK_PRES;
    bit_count += (ms_info->is_present==1)*numWindows*maxSfb*LEN_MASK;
  }
  
  /* Write individual_channel_stream elements */
  bit_count += WriteICS(quantInfoL,commonWindow,fixedStream,writeFlag);
  bit_count += WriteICS(quantInfoR,commonWindow,fixedStream,writeFlag);
  SE_DBUG(if (writeFlag) fprintf(stdout,"\n   Finished CPE,           cpe bit_count:%d    bits in buffer:%ld",bit_count,
		  BsBufferNumBit(fixedStream->buffer[0]) ));
 
  return bit_count;
}


/*****************************************************************************/
/* WriteICS(...), write an individual_channel_stream element to the bitstream.*/
/*****************************************************************************/
int WriteICS(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
	     int commonWindow,           /* Common window flag */
	     BsBitStream* fixed_stream,  /* Pointer to bitstream */
	     int writeFlag)              /* 1 means write, 0 means count only */
{
  /* this function writes out an individual_channel_stream to the bitstream and */
  /* returns the number of bits written to the bitstream */
  int bit_count = 0;
  int output_book_vector[SFB_NUM_MAX*2];
  int i;
  int tmp;
  writeFlag = ( writeFlag != 0 );

  SE_DBUG(fprintf(stdout,"\nWriteICS, initial ics bit_count:%d    bits in buffer:%ld\n",bit_count,
		  BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Write the 8-bit global_gain */
  BsPutBit(fixed_stream,quantInfo->common_scalefac,writeFlag*LEN_GLOB_GAIN);  
  bit_count += LEN_GLOB_GAIN;
  SE_DBUG(fprintf(stdout,"   Wrote global_gain,      ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Write ics information */
  if (!commonWindow) {
    bit_count += WriteICSInfo(quantInfo,fixed_stream,writeFlag, commonWindow, NULL);
    SE_DBUG(fprintf(stdout,"   Wrote ics_info,         ics bit_count:%d    bits in buffer:%ld\n",
		    bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));
  }

  /* Write section_data() information to the bitstream */
  bit_count += sort_book_numbers(quantInfo,output_book_vector,fixed_stream,writeFlag);
  SE_DBUG(fprintf(stdout,"   Wrote section_data,     ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Write scale_factor_data() information */
  bit_count += write_scalefactor_bitstream(fixed_stream,writeFlag,quantInfo);
  SE_DBUG(fprintf(stdout,"   Wrote scalefactor_data, ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Write pulse_data() */
  bit_count += WritePulseData(quantInfo,fixed_stream,writeFlag);
  SE_DBUG(fprintf(stdout,"   Wrote pulse_data,       ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Write TNS data */
  bit_count += WriteTNSData(quantInfo,fixed_stream,writeFlag);
  SE_DBUG(fprintf(stdout,"   Wrote tns_data,         ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));
	
  /* Write gain control data */
  bit_count += WriteGainControlData(quantInfo,fixed_stream,writeFlag);
  SE_DBUG(fprintf(stdout,"   Wrote gc_data,          ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Write out spectral_data() */
  bit_count += WriteSpectralData(quantInfo,fixed_stream,writeFlag);
  SE_DBUG(fprintf(stdout,"   Wrote spectral_data,    ics bit_count:%d    bits in buffer:%ld\n",
		  bit_count,BsBufferNumBit(fixed_stream->buffer[0]) ));

  /* Return number of bits */
  return(bit_count);
}


/*****************************************************************************/
/* WriteICSInfo(...), write individual_channel_stream information            */
/*  to the bitstream.                                                        */
/*****************************************************************************/
int WriteICSInfo(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
		 BsBitStream* fixed_stream,  /* Pointer to bitstream */
		 int writeFlag,              /* 1 means write, 0 means count only */
		 int stereo_flag,
		 AACQuantInfo* quantInfoR)
{
  int grouping_bits;
  int max_sfb;
  int bit_count = 0;

  /* Check window sequence */
  if (quantInfo->block_type > (enum WINDOW_TYPE)NUM_WIN_SEQ) {
    CommonExit(-1,"\n unknown blocktype : %d",quantInfo->block_type);
  }

  /* Check window shape */
  if (quantInfo->window_shape != 0)
    CommonExit(-1,"\nunknown windowshape");

  /* Compute number of scalefactor bands */
  if (quantInfo->max_sfb*quantInfo->num_window_groups != quantInfo->nr_of_sfb)
    CommonExit(-1,"\nwrong num of nr_of_sfb");
  max_sfb =   quantInfo->max_sfb;

  if (writeFlag) {
    /* write out ics_info() information */
    BsPutBit(fixed_stream,0,LEN_ICS_RESERV);  /* reserved Bit*/
  
    /* Write out window sequence */
    BsPutBit(fixed_stream,quantInfo->block_type,LEN_WIN_SEQ);  /* short window */

    /* Write out window shape */ 
    BsPutBit(fixed_stream,quantInfo->window_shape,LEN_WIN_SH);  /* window shape */
  }
    
  bit_count += LEN_ICS_RESERV;
  bit_count += LEN_WIN_SEQ;
  bit_count += LEN_WIN_SH;

  /* For short windows, write out max_sfb and scale_factor_grouping */
  if (quantInfo -> block_type == ONLY_SHORT_WINDOW){
    if (writeFlag) {
      BsPutBit(fixed_stream,max_sfb,LEN_MAX_SFBS); 
      grouping_bits = find_grouping_bits(quantInfo->window_group_length,quantInfo->num_window_groups);
      BsPutBit(fixed_stream,grouping_bits,MAX_SHORT_IN_LONG_BLOCK - 1);  /* the grouping bits */
    }
    bit_count += LEN_MAX_SFBS;
    bit_count += MAX_SHORT_IN_LONG_BLOCK - 1;
  }

  /* Otherwise, write out max_sfb and predictor data */
  else { /* block type is either start, stop, or long */
    if (writeFlag) {
      BsPutBit(fixed_stream,max_sfb,LEN_MAX_SFBL);
    }
    bit_count += LEN_MAX_SFBL;

#ifdef MPEG4V1
    if(quantInfo->profile == LTP)
    {
      int ltp_present;
	  
      ltp_present = 0;
      if(quantInfo->ltpInfo->global_pred_flag)
	ltp_present = 1;
      if(stereo_flag)
	if(quantInfoR->ltpInfo->global_pred_flag)
	  ltp_present = 1;
      
      if(writeFlag)
	BsPutBit(fixed_stream, ltp_present, 1);
      bit_count += 1;
      
      if(ltp_present)
      {
	bit_count += WriteLTP_PredictorData(quantInfo,fixed_stream,writeFlag);
	if(stereo_flag)
	  bit_count += WriteLTP_PredictorData(quantInfoR,fixed_stream,writeFlag);
      }
    }
    else
#endif
      bit_count += WritePredictorData(quantInfo,fixed_stream,writeFlag);
  }

  return bit_count;
}

#ifdef MPEG4V1
/*****************************************************************************/
/* WriteLTP_PredictorData(...), write LTP predictor data.                    */
/*****************************************************************************/
int WriteLTP_PredictorData(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
                           BsBitStream* fixed_stream,  /* Pointer to bitstream */
                           int writeFlag)              /* 1 means write, 0 means count only */  
{
  int bit_count = 0;

  bit_count += nok_ltp_encode (fixed_stream, quantInfo->block_type, quantInfo->nr_of_sfb, 
                               quantInfo->ltpInfo, writeFlag);

  return (bit_count);
}
#endif


/*****************************************************************************/
/* WritePredictorData(...), write predictor data.                            */
/*****************************************************************************/
int WritePredictorData(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
		       BsBitStream* fixed_stream,  /* Pointer to bitstream */
		       int writeFlag)              /* 1 means write, 0 means count only */  
{
  int bit_count = 0;

  /* Write global predictor data present */
  short predictorDataPresent = quantInfo->pred_global_flag;
  int numBands = min(max_pred_sfb,quantInfo->nr_of_sfb);

  if (writeFlag) {
    BsPutBit(fixed_stream,predictorDataPresent,LEN_PRED_PRES);  /* predictor_data_present */
    if (predictorDataPresent) {
      int b;
/* Code segment added by JB */
      if (quantInfo->reset_group_number == -1)
        BsPutBit(fixed_stream,0,LEN_PRED_RST); /* No prediction reset */
      else
      {
        BsPutBit(fixed_stream,1,LEN_PRED_RST);
        BsPutBit(fixed_stream,(unsigned long)quantInfo->reset_group_number,
          LEN_PRED_RSTGRP);
      }
/* End of code segment */

      for (b=0;b<numBands;b++) {
	BsPutBit(fixed_stream,quantInfo->pred_sfb_flag[b],LEN_PRED_ENAB);
      }
    }
  }
  bit_count = LEN_PRED_PRES;
  bit_count += (predictorDataPresent) ?
    (LEN_PRED_RST + 
     ((quantInfo->reset_group_number)!=-1)*LEN_PRED_RSTGRP + 
     numBands*LEN_PRED_ENAB) : 0;
  return bit_count;
}

/*****************************************************************************/
/* WritePulseData(...), write pulse data.                            */
/*****************************************************************************/
int WritePulseData(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
		   BsBitStream* fixed_stream,  /* Pointer to bitstream */
		   int writeFlag)              /* 1 means write, 0 means count only */
{
  int bit_count = 0;

#ifndef PULSE
  /* Currently write no pulse data present */
  if (writeFlag) {
    BsPutBit(fixed_stream,0,LEN_PULSE_PRES);  /* no pulse_data_present */
  }
  bit_count = LEN_PULSE_PRES;
#else
  int i;
  /* Pulse data present */
  if (writeFlag) {
    if (quantInfo->number_pulse == 0) {
      BsPutBit(fixed_stream,0,LEN_PULSE_PRES);  /* no pulse_data_present */
      bit_count = LEN_PULSE_PRES;
    } else {
      BsPutBit(fixed_stream,1,LEN_PULSE_PRES);  /* pulse_data_present */
      BsPutBit(fixed_stream, quantInfo->number_pulse-1, 2); 
      BsPutBit(fixed_stream, quantInfo->pulse_start_sfb, 6);
      bit_count = 9;
      for(i=0;i<quantInfo->number_pulse;i++) {
        BsPutBit(fixed_stream, quantInfo->pulse_offset[i], 5);
        BsPutBit(fixed_stream, quantInfo->pulse_amp[i], 4);
        bit_count += 9;
      }
    }
  } else { 
    if (quantInfo->number_pulse == 0) {
      bit_count = LEN_PULSE_PRES;
    } else {
      bit_count = 9;
      for(i=0;i<quantInfo->number_pulse;i++) {
        bit_count += 9;
      }
    }
  }
#endif

  return bit_count;
}

/*****************************************************************************/
/* WriteTNSData(...), write TNS data.                            */
/*****************************************************************************/
int WriteTNSData(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
		 BsBitStream* fixed_stream,  /* Pointer to bitstream */
		 int writeFlag)              /* 1 means write, 0 means count only */ 
{
  int bit_count = 0;
  int numWindows = 1;
  int len_tns_nfilt;
  int len_tns_length;
  int len_tns_order;
  int filtNumber;
  int resInBits;
  int bitsToTransmit;
  unsigned long unsignedIndex;
  int debugVal=3;
  int w;

  TNS_INFO* tnsInfoPtr = quantInfo->tnsInfo;

  if (writeFlag) {
    BsPutBit(fixed_stream,tnsInfoPtr->tnsDataPresent,LEN_TNS_PRES);
    SE_DBUG1(fprintf(stdout,"\nTNS DATA:\n"));
    SE_DBUG1(fprintf(stdout,"  tnsDataPresent: %d\n",tnsInfoPtr->tnsDataPresent));
  }
  bit_count += LEN_TNS_PRES;

  /* If TNS is not present, bail */
  if (!tnsInfoPtr->tnsDataPresent) {
    return bit_count;
  }

  /* Set window-dependent TNS parameters */
  if (quantInfo->block_type == ONLY_SHORT_WINDOW) {
    numWindows = MAX_SHORT_IN_LONG_BLOCK;
    len_tns_nfilt = LEN_TNS_NFILTS;
    len_tns_length = LEN_TNS_LENGTHS;
    len_tns_order = LEN_TNS_ORDERS;
  } else {
    numWindows = 1;
    len_tns_nfilt = LEN_TNS_NFILTL;
    len_tns_length = LEN_TNS_LENGTHL;
    len_tns_order = LEN_TNS_ORDERL;
  }

  /* Write TNS data */
  bit_count += numWindows * len_tns_nfilt;
  for (w=0;w<numWindows;w++) {
    TNS_WINDOW_DATA* windowDataPtr = &tnsInfoPtr->windowData[w];
    int numFilters = windowDataPtr->numFilters;
    if (writeFlag) {
      BsPutBit(fixed_stream,numFilters,len_tns_nfilt); /* n_filt[] = 0 */
      SE_DBUG1(fprintf(stdout,"    Window %d has %d filters",w,numFilters));
      SE_DBUG1(if (!numFilters) fprintf(stdout,".\n"));
    }
    if (numFilters) {
      bit_count += LEN_TNS_COEFF_RES;
      resInBits = windowDataPtr->coefResolution;
      resInBits = windowDataPtr->coefResolution;
      if (writeFlag) {
	BsPutBit(fixed_stream,resInBits-DEF_TNS_RES_OFFSET,LEN_TNS_COEFF_RES);
	SE_DBUG1(fprintf(stdout," of res %d bits:\n",resInBits));
      }
      bit_count += numFilters * (len_tns_length+len_tns_order);
      for (filtNumber=0;filtNumber<numFilters;filtNumber++) {
	TNS_FILTER_DATA* tnsFilterPtr=&windowDataPtr->tnsFilter[filtNumber];
	int order = tnsFilterPtr->order;
	if (writeFlag) {
	  BsPutBit(fixed_stream,tnsFilterPtr->length,len_tns_length);
	  BsPutBit(fixed_stream,order,len_tns_order);
	  SE_DBUG1(fprintf(stdout,"      filter[%d]: ord=%d  len=%d  ",filtNumber,order,tnsFilterPtr->length));
	  SE_DBUG1(if (!order) fprintf(stdout,"{ }\n"));
	}
	if (order) {
	  bit_count += (LEN_TNS_DIRECTION + LEN_TNS_COMPRESS);
	  if (writeFlag) {
	    BsPutBit(fixed_stream,tnsFilterPtr->direction,LEN_TNS_DIRECTION);
	    BsPutBit(fixed_stream,tnsFilterPtr->coefCompress,LEN_TNS_COMPRESS);
	    SE_DBUG1(fprintf(stdout,"dir=%d  com=%d  { ",tnsFilterPtr->direction,tnsFilterPtr->coefCompress));
	  }
	  bitsToTransmit = resInBits - tnsFilterPtr->coefCompress;
	  bit_count += order * bitsToTransmit;
	  if (writeFlag) {
	    int i;
	    for (i=1;i<=order;i++) {
	      unsignedIndex = (unsigned long) (tnsFilterPtr->index[i])&(~(~0<<bitsToTransmit));
	      BsPutBit(fixed_stream,unsignedIndex,bitsToTransmit);
	      SE_DBUG1(fprintf(stdout,"%d ",tnsFilterPtr->index[i]));
	    }
	    SE_DBUG1(fprintf(stdout,"}\n"));
	  }
	}
      }
    }
  }
  return bit_count;
}


/*****************************************************************************/
/* WriteGainControlData(...), write gain control data.                       */
/*****************************************************************************/
int WriteGainControlData(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
			 BsBitStream* fixed_stream,  /* Pointer to bitstream */
			 int writeFlag)              /* 1 means write, 0 means count only */  
{
  /* quantInfo->gcBitBuf contains packed gain_control_data() */
  int bit_count = 0;
	bit_count += quantInfo->gcBitBuf->numBit + LEN_GAIN_PRES;

  if (writeFlag) {
	if(quantInfo->gcBitBuf->numBit >0){
    	BsPutBit(fixed_stream,1,LEN_GAIN_PRES);
   		BsPutBuffer ( fixed_stream, quantInfo -> gcBitBuf );
	}
	else{
    	BsPutBit(fixed_stream,0,LEN_GAIN_PRES);
	}
  }
  	return bit_count;
}


/*****************************************************************************/
/* WriteSpectralData(...), write spectral data.                              */
/*****************************************************************************/
int WriteSpectralData(AACQuantInfo* quantInfo,    /* AACQuantInfo structure */
		      BsBitStream* fixed_stream,  /* Pointer to bitstream */
		      int writeFlag)              /* 1 means write, 0 means count only */
{
  int bit_count = 0;
  int numSpectral = quantInfo->spectralCount;

  /* set up local pointers to data and len */
  /* data array contains data to be written */
  /* len array contains lengths of data words */
  int* data = quantInfo -> data;
  int* len = quantInfo -> len;

  if (writeFlag) {
    int i;
    for(i=0;i<numSpectral;i++) {
      if (len[i] > 0) {  /* only send out non-zero codebook data */
	BsPutBit(fixed_stream,data[i],len[i]); /* write data */
	bit_count += len[i];                   /* update bit_count */
      }
    }
  } else {
    int i;
    for(i=0;i<numSpectral;i++) {
      bit_count += len[i];              /* update bit_count */
    }
  }

  return bit_count;
}


/*****************************************************************************/
/* WriteCplData(...), write coupling channel data.                           */
/*****************************************************************************/

int WriteCplData  (BsBitStream *fixedStream,				/* in: bit stream */
				   CPL_VARS *cvars,							
				   AACQuantInfo *quantInfo,
				   int nfchans)
{
	DSPshort num_gain_element_lists = 0;
	DSPshort cge, length, ele, diff, sfb, gain;
	DSPshort *cplexpsptr;
	int codeword, numbits, *bookvptr;
	DSPshort **cplexps = cvars->cplexps;
	DSPshort lastexp;

#ifdef CPLDEBUG
	DSPfract accum;
#endif

	numbits = 0;		/* Init the bit counter */

	BsPutBit(fixedStream, ID_CCE, LEN_SE_ID);				/* element ID */
	numbits += LEN_SE_ID;
	BsPutBit(fixedStream, cvars->element_instance_tag, LEN_TAG); /* cpl element tag */
	numbits += LEN_TAG;
	BsPutBit(fixedStream, cvars->ind_sw_cce_flag, 1);		/* independent switching flag */
	numbits += 1;
	BsPutBit(fixedStream, cvars->num_coupled_elements, 3);  /* number of coupled elements */
	numbits += 3;

	for (ele = 0; ele < (cvars->num_coupled_elements + 1); ele++)
	{
		num_gain_element_lists++;
		BsPutBit(fixedStream, cvars->cc_target_is_cpe[ele], 1);
		numbits += 1;
		BsPutBit(fixedStream, cvars->cc_target_tag_select[ele], 4);
		numbits += 4;
		if (cvars->cc_target_is_cpe[ele])
		{
			BsPutBit(fixedStream,cvars->cc_l[ele], 1);		/* left channel is coupled */
			numbits += 1;
			BsPutBit(fixedStream,cvars->cc_r[ele], 1);		/* right channel is coupled */
			numbits += 1;
			if (cvars->cc_l[ele] && cvars->cc_r[ele])
			{
				num_gain_element_lists++;
			}
		}
	}

	BsPutBit(fixedStream, cvars->cc_domain, 1);				/* TNS ordering flag */
	numbits += 1;
	BsPutBit(fixedStream, cvars->gain_element_sign, 1);		/* phase flags present */
	numbits += 1;
	BsPutBit(fixedStream, cvars->gain_element_scale, 2);	/* fractional exponent select */
	numbits += 2;

/* Write the coupling single channel element */

	numbits += WriteICS(quantInfo, cvars->cplchan_tag, fixedStream, 1);

	bookvptr = quantInfo->book_vector;
	for (ele = 1; ele < num_gain_element_lists; ele++)
	{
		cplexpsptr = cplexps[ele];
		if (cvars->ind_sw_cce_flag)
		{
			cge = 1;
		}
		else
		{
			BsPutBit(fixedStream, cvars->common_gain_element_present[ele], 1);
			numbits += 1;
			cge = cvars->common_gain_element_present[ele];
		}
		
		if (cge)
		{
			gain = cvars->common_gain_element[ele];
			length = gethuff(12, gain + 60, 2);
			codeword = gethuff(12, gain + 60, 3);
			BsPutBit(fixedStream, codeword, length);
			numbits += length;
		}
		else
		{

/* Compute the differential coordinates for transmission.  There should be no change
   in differential coordinate values if the vector specifies no coordinate to be 
   transmitted (zero huffman table).*/

			cplexpsptr = cplexps[ele];
			diffexps[0] = cplexpsptr[0];
			lastexp = diffexps[0];
			for (sfb = 1; sfb < cvars->cplendf; sfb++)
			{
				if (bookvptr[sfb])
				{
					diffexps[sfb] = cplexpsptr[sfb] - lastexp;
					diffexps[sfb] = min(60, diffexps[sfb]);
					diffexps[sfb] = max(-60, diffexps[sfb]);
					lastexp = cplexpsptr[sfb];
				}
				else
				{
					diffexps[sfb] = diffexps[sfb - 1];
					lastexp = diffexps[sfb];
				}
			}

			for (sfb = 0; sfb < cvars->cplendf; sfb++)
			{
				if (bookvptr[sfb])
				{
					if (sfb < cvars->cplbegf)
					{
						CommonExit(-1,"Packing coupling coordinate less than cplbegf\n");
					}
					diff = diffexps[sfb];
					length = gethuff(12, diff + 60, 2);
					codeword = gethuff(12, diff + 60, 3);
					BsPutBit(fixedStream, codeword, length);
					numbits += length;
				}
			}
		}
	} /* end element loop */

	return (numbits);
} /* WriteCplData */



