/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)
and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1998.



$Id: bitmux.h,v 1.9 1999/07/22 15:52:19 purnhage Exp $

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Bitmux.h  : Handling of bistream format for AAC Raw and AAC SCALABLE     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#ifndef _bitmux_h_
#define _bitmux_h_

int write_ics_info (int nr_of_sfb, WINDOW_SEQUENCE windowSequence, BsBitStream *fixed_stream, 
                    WINDOW_SHAPE window_shape, int num_window_groups, int window_group_length[], 
                    int reducedInfo, QC_MOD_SELECT qc_select, PRED_TYPE predictor_type, 
                    NOK_BW_PRED_STATUS *nok_bwp_status, NOK_LT_PRED_STATUS *nok_lt_statusLeft,
		    NOK_LT_PRED_STATUS *nok_lt_statusRight, int stereo_flag);
                  

int sort_book_numbers(int book_vector[],
                      int output_book_vector[],
		      int nr_of_sfb,  int qdebug,
		      WINDOW_SEQUENCE windowSequence,
		      BsBitStream *fixed_stream,
		      int write_flag,
                      int num_groups);

/*
int  write_to_bitstream(int nr_of_sfb, WINDOW_SEQUENCE windowSequence, int scale_factors[], BsBitStream *fixed_stream,
			BsBitBuffer *gcBitBufCh, int book_vector[], int data[], int len[], int qdebug, int huff[13][1090][4],
			int counter, int num_window_groups, int window_group_length[], int global_gain, int window_shape,
			QC_MOD_SELECT qc_select, int noise_nrg[], PRED_TYPE predictor_type,
                        NOK_LT_PRED_STATUS *nok_lt_status, NOK_BW_PRED_STATUS *nok_bwp_status);
                        */

int write_tns_data(TNS_INFO* tnsInfoPtr,
                   WINDOW_SEQUENCE windowSequence,
                   BsBitStream* fixed_stream,
                   int writeFlag);

int write_ind_cha_stream(int nr_of_sfb, WINDOW_SEQUENCE windowSequence, int scale_factors[], BsBitStream *fixed_stream,
                         BsBitBuffer *gcBitBufCh, TNS_INFO *tnsInfo, int book_vector[], int data[], int len[], int qdebug, int scale_flag,
                         int huff[13][1090][4], int counter, int num_window_groups, int window_group_length[],
                         int global_gain, WINDOW_SHAPE window_shape, int noise_nrg[],  PRED_TYPE predictor_type,
                         NOK_LT_PRED_STATUS *nok_lt_status, NOK_BW_PRED_STATUS *nok_bwp_status, int common_window,
                         QC_MOD_SELECT qc_select);

int write_scalefactor_bitstream(int nr_of_sfb, int scale_factors[], int book_vector[],
                              BsBitStream *fixed_stream, int write_flag, int window_group_length[],
                              int num_window_groups, int global_gain, WINDOW_SEQUENCE windowSequence,
                              int noise_nrg[], int qdebug, int huff[13][1090][4]);

int write_aac_cpe(int nr_of_sfb, WINDOW_SEQUENCE windowSequence, BsBitStream *fixed_stream, int num_window_groups,
                  int window_group_length[], WINDOW_SHAPE window_shape, int common_window, int ms_mask, int ms_used[10][10],
		  PRED_TYPE predictor_type, NOK_BW_PRED_STATUS *nok_bwp_status,
                  QC_MOD_SELECT qc_select, NOK_LT_PRED_STATUS *nok_lt_status);


int write_aac_sce(BsBitStream *fixed_stream);


int write_fill_elements(BsBitStream *fixed_stream, int numFillBits);


int write_aac_end_id(BsBitStream *fixed_stream);

#endif   /* define _bitmux_ */
