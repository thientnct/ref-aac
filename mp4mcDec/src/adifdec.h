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

#ifndef _adifdec_h_
#define _adifdec_h_

#include		<stdio.h>
#include		<stdlib.h>
#include		<sys/types.h>
#include 		<math.h>
#include		<string.h>

#include "interface.h"

#define	PRINT		fprintf
#define	SO		stdout
#define	SE		stderr


typedef struct
{
    int	    islong;			/* true if long block */
    int	    nsbk;			/* sub-blocks (SB) per block */
    int	    bins_per_bk;		/* coef's per block */
    int	    sfb_per_bk;			/* sfb per block */
    int	    bins_per_sbk[MAX_SBK];	/* coef's per SB */
    int	    sfb_per_sbk[MAX_SBK];	/* sfb per SB */
    int	    sectbits[MAX_SBK];
    short   *sbk_sfb_top[MAX_SBK];	/* top coef per sfb per SB */
    short   *sfb_width_128;		/* sfb width for short blocks */
    short   bk_sfb_top[200];		/* cum version of above */
    int	    num_groups;
    short   group_len[8];
    short   group_offs[8];
} Info;


extern Info*	winmap[NUM_WIN_SEQ];
extern long	bno;
extern int	debug[256];
#ifdef CT_SBR
extern int	sbrPresentInFillElem;
#endif

typedef double		Float;
typedef	unsigned char	byte;



/* "create" a decoder, allocate memory & all */
void		decoder_init(void);
void            decoder_free(void);


/* selecting program can only be done before starting to read the file */
void		decoder_selectProgram(int selectedProgram);

/* set various decoder internal parameters */
void		decoder_setupParameters(int sampling_rate_idx);
#ifdef DRC
void		decoder_setDrc(Float drc_high, Float drc_low, int drc_ref_level);
#endif
#ifdef CT_SBR
/* special stuff for sbr-conformance */
void		decoder_setSBR_conformance(int dontApplySBR);
void*		decoder_getSBRbitstream(void); /* returns an SBRBITSTREAM */
#endif


/* parse one frame */
void		dec_parse(void);

/* decode one frame, set the pointer to output data and set the block length */
void		dec_decode(Float ***output_data, int *block_length);

/* parse & decode one frame, set the pointer to output data and set the block length */
void		dec(Float ***output_data, int *block_length);



/* this function has to be implemented in the user program, e.g. decoder.c */
void		myexit(char *s);


#endif
