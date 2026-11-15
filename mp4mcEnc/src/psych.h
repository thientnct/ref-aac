/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Fraunhofer IIS-A / University of Erlangen (UER)

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

-----
This software module was modified by

Tadashi Araki (Ricoh Company, ltd.)
Tatsuya Okada (Waseda Univ.)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3.

Copyright (c) 1997.

 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Bernhard Grill -- August-96  */

#include "all.h"
/* #define Chan 2 */

/* Jul 9 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PSY_CHANS MAX_TIME_CHANNELS

typedef struct { 
  long   sampling_rate;                   /* the following entries are for this sampling rate */
  int    num_cb_long;
  int    num_cb_short;
  int    cb_width_long[NSFB_LONG];
  int    cb_width_short[NSFB_SHORT+2];
} SR_INFO;

/* added by T. Araki (1997.10.17) */
#define OFFSET_FOR_SHORT 448
#define NPART_LONG 72
#define NPART_SHORT 48
/* added by T. Araki (1997.10.17) end */

/* added by T. Araki (1997.07.10) */
typedef struct {
  double hw[BLOCK_LEN_LONG*2];     /* Hann window table */
  double st[BLOCK_LEN_LONG*5/2];   /* sin table*/
  int    brt[BLOCK_LEN_LONG*2];    /* bit inverse table */
} FFT_TABLE_LONG;

typedef struct {
  double hw[BLOCK_LEN_SHORT*2];     /* Hann window table */
  double st[BLOCK_LEN_SHORT*5/2];   /* sin table*/
  int    brt[BLOCK_LEN_SHORT*2];    /* bit inverse table */
} FFT_TABLE_SHORT;
 
typedef struct {
  int    sampling_rate;
  int    len;      /* length of the table */
  int    w_low[NPART_LONG];
  int    w_high[NPART_LONG];
  int    width[NPART_LONG];
  double  bval[NPART_LONG];
  double  qsthr[NPART_LONG];
  double  rnorm[NPART_LONG];
} PARTITION_TABLE_LONG;

typedef struct {
  int    sampling_rate;
  int    len;      /* length of the table */ 
  int    w_low[NPART_SHORT];
  int    w_high[NPART_SHORT];
  int    width[NPART_SHORT];
  double  bval[NPART_SHORT];
  double  qsthr[NPART_SHORT];
  double  rnorm[NPART_SHORT];
} PARTITION_TABLE_SHORT;

typedef struct {
  double fft_r[PSY_CHANS][BLOCK_LEN_LONG*3];
  double fft_f[PSY_CHANS][BLOCK_LEN_LONG*3];
  int    p_fft[PSY_CHANS]; /* pointer for fft_r and fft_f */
  double nb[PSY_CHANS][NPART_LONG*2];
  int    p_nb[PSY_CHANS]; /* pointer for nb */
  double ismr[PSY_CHANS][NSFB_LONG]; /* 1/SMR in each swb */
} PSY_STATVARIABLE_LONG;

typedef struct {
  double r_pred[BLOCK_LEN_LONG];
  double f_pred[BLOCK_LEN_LONG];
  double c[BLOCK_LEN_LONG];
  double e[NPART_LONG];
  double cw[NPART_LONG];
  double en[NPART_LONG];
  double cb[NPART_LONG];
  double tb[NPART_LONG];
  double snr[NPART_LONG];
  double bc[NPART_LONG];
  double pe;
  double epart[NSFB_LONG];
  double thr[BLOCK_LEN_LONG];
  double npart[NSFB_LONG];
} PSY_VARIABLE_LONG;

typedef struct {
  double fft_r[MAX_SHORT_WINDOWS][BLOCK_LEN_SHORT];
  double fft_f[MAX_SHORT_WINDOWS][BLOCK_LEN_SHORT];
  double last6_fft_r[PSY_CHANS][BLOCK_LEN_SHORT];
  double last6_fft_f[PSY_CHANS][BLOCK_LEN_SHORT];
  double last7_fft_r[PSY_CHANS][BLOCK_LEN_SHORT];
  double last7_fft_f[PSY_CHANS][BLOCK_LEN_SHORT];
  double nb[MAX_SHORT_WINDOWS][NPART_SHORT];
  double last7_nb[PSY_CHANS][NPART_SHORT];
  double ismr[PSY_CHANS][MAX_SHORT_WINDOWS][NSFB_SHORT]; /* 1/SMR in each swb */
} PSY_STATVARIABLE_SHORT;

typedef struct {
  double r_pred[MAX_SHORT_WINDOWS][BLOCK_LEN_SHORT];
  double f_pred[MAX_SHORT_WINDOWS][BLOCK_LEN_SHORT];
  double c[MAX_SHORT_WINDOWS][BLOCK_LEN_SHORT];
  double e[MAX_SHORT_WINDOWS][NPART_SHORT];
  double cw[MAX_SHORT_WINDOWS][NPART_SHORT];
  double en[MAX_SHORT_WINDOWS][NPART_SHORT];
  double cb[MAX_SHORT_WINDOWS][NPART_SHORT];
  double tb[MAX_SHORT_WINDOWS][NPART_SHORT];
  double snr[MAX_SHORT_WINDOWS][NPART_SHORT];
  double bc[MAX_SHORT_WINDOWS][NPART_SHORT];
  double epart[MAX_SHORT_WINDOWS][NSFB_SHORT];
  double thr[MAX_SHORT_WINDOWS][BLOCK_LEN_SHORT];
  double npart[MAX_SHORT_WINDOWS][NSFB_SHORT];
} PSY_VARIABLE_SHORT;
/* added by T. Araki (1997.07.10) end */

typedef struct {
  double *p_ratio;
  int    *cb_width;
  int    no_of_cb;
} CH_PSYCH_OUTPUT;

#ifdef __cplusplus
extern "C" {
#endif

void EncTf_psycho_acoustic_init( void );
void EncTf_psycho_acoustic( 
  /* input */
  double sampling_rate,
  int    no_of_chan,         /* no of audio channels */
  double *p_time_signal[],
  enum WINDOW_TYPE block_type[],
  int qcSelect,
  int frameLength,
  /* output */
  CH_PSYCH_OUTPUT p_chpo_long[],
  CH_PSYCH_OUTPUT p_chpo_short[][MAX_SHORT_WINDOWS]
);

/* added by T. Okada( 1997.07.10 ) */
/* Jul 10 */
#define psy_max(x,y) ((x) > (y) ? (x) : (y))
#define psy_min(x,y) ((x) < (y) ? (x) : (y))
#define psy_sqr(x) ((x)*(x))


double psy_get_absthr(double f); /* Jul 8 */
double sprdngf(double b1,double b2);

void psy_fft_table_init(FFT_TABLE_LONG *fft_tbl_long, 
			FFT_TABLE_SHORT *fft_tbl_short
			);

void psy_part_table_init(double sampling_rate,
			 PARTITION_TABLE_LONG **part_tbl_long, 
			 PARTITION_TABLE_SHORT **part_tbl_short
			 );

void psy_calc_init(double sample[][BLOCK_LEN_LONG*2],
		   PSY_STATVARIABLE_LONG *psy_stvar_long, 
		   PSY_STATVARIABLE_SHORT *psy_stvar_short
		   ); 

void psy_step1(double* p_time_signal[],
	       double sample[][BLOCK_LEN_LONG*2], 
	       int ch
	       );

void psy_step2(double sample[][BLOCK_LEN_LONG*2], 
               PSY_STATVARIABLE_LONG *psy_stvar_long, 
               PSY_STATVARIABLE_SHORT *psy_stvar_short,
	       FFT_TABLE_LONG *fft_tbl_long,
	       FFT_TABLE_SHORT *fft_tbl_short,
	       int ch
	       );

void psy_step3(PSY_STATVARIABLE_LONG *psy_stvar_long, 
               PSY_STATVARIABLE_SHORT *psy_stvar_short, 
               PSY_VARIABLE_LONG *psy_var_long, 
               PSY_VARIABLE_SHORT *psy_var_short, 
               int ch
	       );

void psy_step4(PSY_STATVARIABLE_LONG *psy_stvar_long,
               PSY_STATVARIABLE_SHORT *psy_stvar_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short,
	       int ch
	       );

void psy_step5(PARTITION_TABLE_LONG *part_tbl_long, 
	       PARTITION_TABLE_SHORT *part_tbl_short, 
	       PSY_STATVARIABLE_LONG *psy_stvar_long,
               PSY_STATVARIABLE_SHORT *psy_stvar_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short,
	       int ch
	       );

void psy_step6(PARTITION_TABLE_LONG *part_tbl_long, 
	       PARTITION_TABLE_SHORT *part_tbl_short, 
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short
	       );

void psy_step7(PARTITION_TABLE_LONG *part_tbl_long, 
	       PARTITION_TABLE_SHORT *part_tbl_short, 
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short
	       );

void psy_step8(PARTITION_TABLE_LONG *part_tbl_long,
	       PARTITION_TABLE_SHORT *part_tbl_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short
	       );

void psy_step9(PARTITION_TABLE_LONG *part_tbl_long,
	       PARTITION_TABLE_SHORT *part_tbl_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short
	       );

void psy_step10(PARTITION_TABLE_LONG *part_tbl_long,
		PARTITION_TABLE_SHORT *part_tbl_short,
		PSY_STATVARIABLE_LONG *psy_stvar_long, 
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
		PSY_VARIABLE_LONG *psy_var_long, 
		PSY_VARIABLE_SHORT *psy_var_short,
		int ch /* ch */
		);

void psy_step11(PARTITION_TABLE_LONG *part_tbl_long, 
		PARTITION_TABLE_SHORT *part_tbl_short, 
		PSY_STATVARIABLE_LONG *psy_stvar_long, 
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
		int ch
		);

void psy_step12(PARTITION_TABLE_LONG *part_tbl_long, 
		PARTITION_TABLE_SHORT *part_tbl_short, 
		PSY_STATVARIABLE_LONG *psy_stvar_long, 
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
		PSY_VARIABLE_LONG *psy_var_long, 
		PSY_VARIABLE_SHORT *psy_var_short,
		int ch
		);

void psy_step13(PSY_VARIABLE_LONG *psy_var_long, 
		enum WINDOW_TYPE *block_type,
		int ch
		);

void psy_step14(SR_INFO *p_sri,
		PARTITION_TABLE_LONG *part_tbl_long, 
		PARTITION_TABLE_SHORT *part_tbl_short, 
		PSY_STATVARIABLE_LONG *psy_stvar_long, 
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
		PSY_VARIABLE_LONG *psy_var_long, 
		PSY_VARIABLE_SHORT *psy_var_short,
		int ch);
 /* added by T. Okada (1997.07.10) end */

#ifdef __cplusplus
}
#endif
