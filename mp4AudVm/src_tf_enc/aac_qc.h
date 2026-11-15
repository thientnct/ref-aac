/***********

This software module was originally developed by

Dolby Laboratories

and edited by

Takashi Koike (Sony Corporation)

in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3. This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. Copyright 1996.  

***********/

/******************************************************************** 
/ 
/ filename : aac_qc.h
/ project : MPEG-2 aac
/ author : SNL, Dolby Laboratories, Inc
/ date : November 11, 1996
/ contents/description : header file for aac_qc.c
/          
/
*******************************************************************/

#include "block.h"

#ifdef __cplusplus
extern "C" {
#endif

/* assumptions for the first run of this quantizer */
#define DEBUG 1
#define CHANNEL  1
#define NUM_COEFF  1024
#define MAGIC_NUMBER  0.4054
#define MAX_QUANT 8192
#define SF_OFFSET 100
#define abs(A) ((A) < 0 ? (-A) : (A))
#define sgn(A) ((A) > 0 ? (1) : (-1))


#ifndef PNS
#define PNS                    /* Activate code for Perceptual Noise Substitution (PNS) */
#endif



#define PNS_HCB 13                               /* reserved codebook for flagging PNS */
#define PNS_PCM_BITS 9                           /* size of first (PCM) PNS energy */
#define PNS_PCM_OFFSET (1 << (PNS_PCM_BITS-1))   /* corresponding PCM transmission offset */
#define PNS_SF_OFFSET 90                         /* transmission offset for PNS energies */

extern int pns_sfb_start;                        /* lower border for PNS */




void tf_init_encode_spectrum_aac( int qc_select, int debug );

int tf_encode_spectrum_aac( 
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         available_bitreservoir_bits,
			   int         padding_limit,
			   HANDLE_BSBITSTREAM fixed_stream,
			   HANDLE_BSBITSTREAM var_stream,
			   HANDLE_BSBITBUFFER *gcBitBuf,	/* bit buffer for gain_control_data() */
			   int         nr_of_ch,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int useShortWindows,
			   WINDOW_SHAPE     wfun_select[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
			   int aacAllowScalefacs,
                           long        sampling_rate,
                           QC_MOD_SELECT qc_select,
			   PRED_TYPE predictor_type,
			   NOK_LT_PRED_STATUS *nok_lt_status,
                           int blockSizeSamples,
                           int num_window_groups,
                           int window_group_length[],
                           TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
                           int bandwidth
			   );


int sort_for_grouping(int sfb_offset[],
		      int sfb_with_table[],
		      double *p_spectrum,
		      int num_window_groups,
		      int window_group_length[],
		      int nr_of_sfb,
		      double *allowed_dist,
		      double *psy_allowed_dist,
                      int blockSizeSamples
);

int bit_search(int quant[NUM_COEFF],
               int book_vector[],
               int huff[13][1090][4],
               int sfb_offset[],
	       int nr_of_sfb,
	       WINDOW_SEQUENCE windowSequence,
               int nr_sfb_per_win
	       );

int noiseless_bit_count(
			int quant[NUM_COEFF],
			int huff[13][1090][4],
			int hop,
			int min_book_choice[MAX_SCFAC_BANDS][3],
                        int sfb_offset[],
			int nr_of_sfb,
			WINDOW_SEQUENCE windowSequence,
               int nr_sfb_per_win

);

int output_bits(
		WINDOW_SEQUENCE windowSequence,
		int huff[13][1090][4],
		int book,	   
		int quant[NUM_COEFF],		
		int offset,
		int length,
                int data[],
                int len[],
                int *counter,
                int write_flag
  );


void tf_init_decode_spectrum_aac( long sampling_rate );

int tf_decode_spectrum_aac(
   double      *p_spectrum[MAX_TIME_CHANNELS],
   int         windowSequence,
   HANDLE_BSBITSTREAM fixed_stream,
   HANDLE_BSBITSTREAM var_stream,
   int         nr_of_chan,
   int         bits_avail,
   QC_MOD_SELECT qc_select
);



int find_grouping_bits(int window_group_length[],
		       int num_window_groups
		       );

#ifdef __cplusplus
}
#endif





