/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

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

/* this is NOT a config file, but the interface to config.c */

#ifndef _config_h_
#define _config_h_

#include "chandefs.h"
#include "interface.h"

typedef struct
{
    int present;	/* channel present */
    int tag;		/* element tag */
    int cpe;		/* 0 if single channel, 1 if channel pair */
    int	common_window;	/* 1 if common window for cpe */
    int	ch_is_left;	/* 1 if left channel of cpe */
    int	paired_ch;	/* index of paired channel in cpe */
    int widx;		/* window element index for this channel */
    int is_present;	/* intensity stereo is used */
    int ncch;		/* number of coupling channels for this ch */
#if (CChans > 0)
    int cch[CChans];	/* coupling channel idx */
    int cc_dom[CChans];	/* coupling channel domain */
    int cc_ind[CChans];	/* independently switched coupling channel flag */
    int cc_multi[CChans]; /* which gain coefficients if coupling channel has several different */
#endif
    char *fext;		/* filename extension */
} Ch_Info;

typedef struct {
    int nch;		/* total number of audio channels */
    int nfsce;		/* number of front SCE's pror to first front CPE */
    int nfch;		/* number of front channels */
    int nsch;		/* number of side channels */
    int nbch;		/* number of back channels */
    int nlch;		/* number of lfe channels */
    int ncch;		/* number of valid coupling channels */
    int cc_tag[(1<<LEN_TAG)];	/* tags of valid CCE's */
    int cc_ind[(1<<LEN_TAG)];	/* independently switched CCE's */
    int profile;
    int sampling_rate_idx; /* samplingrate-dependent tables to use */
    int sampling_rate;     /* samplingrate for output (before SBR-upsampling) */
#ifdef CT_SBR
    float upsamplingFactor;
    int bDownSampledSbr;
    int HE_AAC_level;
    int sbrPresentFlag;
#ifdef PARAMETRICSTEREO
    int sbrEnablePS;
#endif
#endif
    Ch_Info ch_info[Chans];
} MC_Info;

typedef struct {
    int num_ele;
    int ele_is_cpe[(1<<LEN_TAG)];
    int ele_tag[(1<<LEN_TAG)];
} EleList;

typedef struct {
    int present;
    int ele_tag;
    int pseudo_enab;
} MIXdown;

typedef struct {
    int profile;
    int sampling_rate_idx;
    EleList front;
    EleList side;
    EleList back;
    EleList lfe;
    EleList data;
    EleList coupling;
    MIXdown mono_mix;
    MIXdown stereo_mix;
    MIXdown matrix_mix;
    char comments[(1<<LEN_PC_COMM)+1];
} ProgConfig;

typedef struct {
    char    adif_id[LEN_ADIF_ID+1];
    int	    copy_id_present;
    char    copy_id[LEN_COPYRT_ID+1];
    int	    original_copy;
    int	    home;
    int	    bitstream_type;
    long    bitrate;
    int	    num_pce;
    int	    prog_tags[(1<<LEN_TAG)];
    long    buffer_fullness;	/* put this transport level info here */
} ADIF_Header;

typedef struct {
  int	syncword;
  int	id;
  int	layer;
  int	protection_abs;
  int 	profile;
  int	sampling_freq_idx;
  int	private_bit;
  int	channel_config;
  int	original_copy;
  int	home;
  int	copyright_id_bit;
  int	copyright_id_start;
  int	frame_length;
  int	adts_buffer_fullness;
  int	num_of_rdb;
  int	crc_check;
#if ((defined NEW_ADTS)||(defined WRITE_NEW_ADTS))
  int rdb_position[MAX_RDB_PER_FRAME-1];
  int crc_check_rdb;
#endif
} ADTS_Header;

#define ADTS_HEADER_SIZE(p)	(7+((p).protection_abs?0:(((p).num_of_rdb+1)*2)))


extern	ProgConfig	prog_config;
extern	MC_Info		mc_info;

extern	int		current_program;
extern	int		default_config;
extern	int		channel_num;
extern	int		sampling_rate[];


int		ch_index(MC_Info *mip, int cpe, int tag);
void		check_mc_info(MC_Info *mip, int new_config);
int		chn_config(int id, int tag, int common_window, MC_Info *mip, int* first_cpe);
int		enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip);
int		get_prog_config(ProgConfig *p);
int		getNextSampFreqIndex(int sampRate);
int		tns_max_bands(int win);
int		tns_max_order(int islong);
int		pred_max_bands(void);
void		print_mc_info(MC_Info *mip);
void		print_adif_header(ADIF_Header *p);
void		print_adts_header(ADTS_Header *p);
void		print_PCE(ProgConfig *p);
void		reset_mc_info(MC_Info *mip);

#endif
