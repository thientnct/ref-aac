/* $Id: aac_qc.h,v 1.7 1998/11/23 21:10:19 lueck Exp $
 */
/***********

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Dolby Laboratories

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

***********/

#ifndef AAC_QC_H
#define AAC_QC_H

/******************************************************************** 
/ 
/ filename : aac_qc.h
/ project : MPEG-2 aac
/ author : SNL, Dolby Laboratories, Inc
/          CL, TI <lueck@ti.com>
           ADD, NDS <alberto@ndsuk.com>
	   JB, NDS <jbennett@ndsuk.com>
	   
/ date : November 11, 1996
/ contents/description : header file for aac_qc.c
/          
/ Changes:
/ 07-jun-97   CL  Added AACQuantInfo structure.
/ 03-dec-97   ADD & JB  Added reset group number to AACQuantInfo sruct.
/
*******************************************************************/

#include "interface.h"
#include "tf_main.h"
#include "tns.h"
#include "all.h"
#ifdef MPEG4V1
#include "nok_ltp_common.h"
#endif
#ifdef CT_SBR
#include "ct_sbr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* assumptions for the first run of this quantizer */
#define DEBUG 1
#define CHANNEL  1
#define NUM_COEFF  BLOCK_LEN_LONG       /* now using BLOCK_LEN_LONG of block.h */
#define MAGIC_NUMBER  0.4054
#define MAX_QUANT 8192
#define SF_OFFSET 100
#define ABS(A) ((A) < 0 ? (-A) : (A))
#define sgn(A) ((A) > 0 ? (1) : (-1))
#define SFB_NUM_MAX MAX_SCFAC_BANDS     /* now using MAX_SCFAC_BANDS of tf_main.h */

/* typedefs for coupling routines */

typedef double DSPfract;
typedef short DSPshort;
typedef long DSPlong;


#ifdef MPEG4V1

#define PNS_HCB 13                               /* reserved codebook for flagging PNS */
#define PNS_PCM_BITS 9                           /* size of first (PCM) PNS energy */
#define PNS_PCM_OFFSET (1 << (PNS_PCM_BITS-1))   /* corresponding PCM transmission offset */
#define PNS_SF_OFFSET 90                         /* transmission offset for PNS energies */
#define PNS_MAX_START 51                         /* Max start for PNS */ 

extern int pns_sfb_start;                        /* lower border for PNS */
#endif

/*********************************************************/
/* AACQuantInfo, info for AAC quantization and coding.   */
/*********************************************************/
typedef struct {
  int max_sfb;                          /* max_sfb, should = nr_of_sfb/num_window_groups */
  int nr_of_sfb;                        /* Number of scalefactor bands, interleaved */
  int spectralCount;                    /* Number of spectral data coefficients */
  enum WINDOW_TYPE block_type;	        /* Block type */      
  int scale_factor[SFB_NUM_MAX];        /* Scalefactor data array , interleaved */			
  int sfb_offset[250];                  /* Scalefactor spectral offset, interleaved */
  int book_vector[SFB_NUM_MAX];         /* Huffman codebook selected for each sf band */
  int data[5*NUM_COEFF];                /* Data of spectral bitstream elements, for each spectral pair, 
                                           5 elements are required: 1*(esc)+2*(sign)+2*(esc value)=5 */
  int len[5*NUM_COEFF];                 /* Lengths of spectral bitstream elements */
  int num_window_groups;                /* Number of window groups */
  int window_group_length
    [MAX_SHORT_IN_LONG_BLOCK];          /* Length (in windows) of each window group */
  int common_scalefac;                  /* Global gain */
  Window_shape window_shape;            /* Window shape parameter */
  BsBitBuffer *gcBitBuf;                /* bit buffer for packed gain_control_data() */
  int max_pred_sfb;                     /* max prediction band */
  short pred_global_flag;               /* Global prediction enable flag */
  int pred_sfb_flag[SFB_NUM_MAX];       /* Prediction enable flag for each scalefactor band */
  int reset_group_number;               /* Prediction reset group number */
  TNS_INFO* tnsInfo;                    /* Ptr to tns data */
  enum AAC_PROFILE profile;             /* AAC profile in use */
#ifdef MPEG4V1
  NOK_LT_PRED_STATUS *ltpInfo;          /* Prt to LTP data */
  float pns_sfb_nrg[SFB_NUM_MAX];
  int pns_sfb_flag[SFB_NUM_MAX];
#endif
#ifdef PULSE 
  int number_pulse;
  int pulse_start_sfb;
  int pulse_offset[4];
  int pulse_amp[4];
#endif
#ifdef CT_SBR
  SBR_CHAN *sbrChan;
#endif
} AACQuantInfo;


typedef struct {
	short cplbegf;					/* Coupling begin frequency */
	short cplendf;					/* Coupling end frequency */
	short tag;
	short ind_sw_cce_flag;
	short cplchan_tag;
	short element_instance_tag;
	short num_coupled_elements;
	short *cc_target_is_cpe;
	short *cc_target_tag_select;
	short *cc_r;
	short *cc_l;
	short cc_domain;
	short gain_element_sign;
	short gain_element_scale;
	short *common_gain_element_present;
	short *common_gain_element;
	short **cplexps;
	int nbits_per_coord;
} CPL_VARS;

/*********************************************************/
/* debugPrint                                            */
/*********************************************************/
void debugPrint(AACQuantInfo* quantInfo,              /* Quantization information */
		double *ratio,
		double *energy,
		double *allowed_dist,
		double *error_energy);


/*********************************************************/
/* tf_init_encode_spectrum_aac                           */
/*********************************************************/
void tf_init_encode_spectrum_aac( int debug );


/*********************************************************/
/* tf_encode_spectrum_aac                                */
/*********************************************************/
int tf_encode_spectrum_aac(
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      *SigMaksRatio[MAX_TIME_CHANNELS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         padding_limit,
			   BsBitStream *fixed_stream,
			   BsBitStream *var_stream,
			   int         nr_of_chan,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int         useShortWindows,
			   int         aacAllowScalefacs,
			   AACQuantInfo* quantInfo,      /* AAC quantization information */ 
			   Ch_Info *ch_info
);			   
#if 0
int tf_encode_spectrum_aac( 
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      *PsySigMaskRatio[MAX_TIME_CHANNELS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         padding_limit,
			   BsBitStream *fixed_stream,
			   BsBitStream *var_stream,
			   int         nr_of_ch,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int useShortWindows,
			   Window_shape     wfun_select,      /* offers the possibility to select different window functions */
			   int		   aacAllowScalefacs,
			   );
#endif

/*********************************************************/
/* sort_book_numbers                                     */
/*********************************************************/
int sort_book_numbers(AACQuantInfo* quantInfo,     /* Quantization information */
		  int output_book_vector[],    /* Output codebook vector, formatted for bitstream */
		  BsBitStream* fixed_stream,   /* Bitstream */
		  int write_flag);             /* Write flag: 0 count, 1 write */

#if 0
int sort_book_numbers(int book_vector[],
                      int output_book_vector[],
		      int nr_of_sfb,
		      WINDOW_TYPE block_type,
		      BsBitStream *fixed_stream,
		      int write_flag
		      );
#endif

/*********************************************************/
/* sort_book_numbers                                     */
/*********************************************************/
int sort_for_grouping(AACQuantInfo* quantInfo,        /* ptr to quantization information */
		      int sfb_width_table[],          /* Widths of single window */
		      double *p_spectrum[],           /* Spectral values, noninterleaved */
		      double *SigMaskRatio,
		      double *PsySigMaskRatio);
#if 0
int sort_for_grouping(int sfb_offset[],
		      int sfb_with_table[],
		      double *p_spectrum[],
		      int num_window_groups,
		      int window_group_length[],
		      int *nr_of_sfb,
		      double *SigMaskRatio,
		      double *PsySigMaskRatio
);
#endif

/*********************************************************/
/* bit_search                                            */
/*********************************************************/
int bit_search(int quant[CHANNEL][NUM_COEFF],  /* Quantized spectral values */
	       int huff[13][1090][4],          /* Huffman codebooks */
               AACQuantInfo* quantInfo);       /* Quantization information */
#if 0
int bit_search(int quant[CHANNEL][NUM_COEFF],
               int book_vector[],
               int huff[13][1090][4],
               int sfb_offset[],
	       int nr_of_sfb[],
	       WINDOW_TYPE block_type[]
	       );
#endif

/*********************************************************/
/* noiseless_bit_count                                   */
/*********************************************************/
int noiseless_bit_count(int quant[CHANNEL][NUM_COEFF],
			int huff[13][1090][4],
			int hop,
			int min_book_choice[112][3],
			AACQuantInfo* quantInfo);         /* Quantization information */
#if 0
int noiseless_bit_count(
			int quant[CHANNEL][NUM_COEFF],
			int huff[13][1090][4],
			int hop,
			int min_book_choice[112][3],
                        int sfb_offset[],
			int nr_of_sfb[],
			WINDOW_TYPE block_type[]
);
#endif

/*********************************************************/
/* output_bits                                           */
/*********************************************************/
int output_bits(AACQuantInfo* quantInfo,
		int huff[13][1090][4],
                int book,                /* codebook */
		int quant[CHANNEL][NUM_COEFF],		
		int offset,
		int length,
		int write_flag);
#if 0
int output_bits(
		WINDOW_TYPE block_type[],
		int huff[13][1090][4],
		int book,	   
		int quant[CHANNEL][NUM_COEFF],		
		int offset,
		int length,
                int data[],
                int len[],
                int *counter,
                int write_flag
  );
#endif 


/*********************************************************/
/* tf_init_decode_spectrum_aac                           */
/*********************************************************/
void tf_init_decode_spectrum_aac( long sampling_rate );

/*********************************************************/
/* tf_decode_spectrum_aac                                */
/*********************************************************/
int tf_decode_spectrum_aac(
   double      *p_spectrum[MAX_TIME_CHANNELS],
   int         block_type,
   BsBitStream *fixed_stream,
   BsBitStream *var_stream,
   int         nr_of_chan,
   int         bits_avail
);


/*********************************************************/
/* write_to_bitstream                                    */
/*********************************************************/
int write_to_bitstream(AACQuantInfo* quantInfo,
		       BsBitStream* fixed_stream);
#if 0
int  write_to_bitstream(
  int nr_of_sfb,
  WINDOW_TYPE block_type,			
  int scale_factors[],			
  BsBitStream *fixed_stream,
  int book_vector[],
  int data[],
  int len[],
  int counter,
  int num_window_groups,
  int window_group_length[],
  int global_gain,
  int window_shape
);
#endif


/*********************************************************/
/* find_grouping_bits                                    */
/*********************************************************/
int find_grouping_bits(int window_group_length[],
		       int num_window_groups
		       );

/*********************************************************/
/* write_scalefactor_bitstream                           */
/*********************************************************/
int write_scalefactor_bitstream(BsBitStream* fixed_stream,             /* Bitstream */  
				int write_flag,                        /* Write flag */
				AACQuantInfo* quantInfo);              /* Quantization information */
#if 0
int write_scalefactor_bitstream(int nr_of_sfb,
				int scale_factors[],
				int book_vector[],
				BsBitStream *fixed_stream,
				int write_flag,
				int window_group_length[],
				int num_window_groups,
				int global_gain,
				WINDOW_TYPE block_type		
				);
#endif

#define MAXCPL		60					/* max allowed value for coupling coordinate.*/
#define CPLSCLINDEX 0					/* scaling constant for coupling coords */

int WriteCplData  (BsBitStream *fixedStream,				/* in: bit stream */
				   CPL_VARS *cvars,							
				   AACQuantInfo *quantInfo,
				   int nfchans);
int calc_cplbits(DSPshort nfchans, 
				 double *cplmix, 
				 DSPshort cplenabl, 
				 Ch_Info *channelInfo,
				 CPL_VARS *cvars);
DSPshort DSPnormalize(DSPfract *arg, DSPshort maxnorm, DSPshort scale_index);
int gethuff(int book, int entry, int type);
void couple_init(CPL_VARS *cvars, 
				 int infchans, 
				 AACQuantInfo *iquantInfo,
				 Ch_Info *channelInfo);

#ifdef __cplusplus
}
#endif

#endif
