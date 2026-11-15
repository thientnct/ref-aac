/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Fraunhofer IIS-A / University of Erlangen (UER)

and edited by
  Sony Corporation

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

/* CREATED BY :  Bernhard Grill -- June-96  */

/*******************************************************************************************
 *
 * Master module for T/F based codecs
 *
 * Authors:
 * MCF    Matt Fellers, Dolby Laboratories
 *
 * Changes:
 * 20-July-98   MCF	Created file.  This file handles the calculation of dynamic range control.
 *
 ******************************************************************************************/

#include "common.h"
#include "bitstream.h"
#include "interface.h"
#include "mp4.h"
#include "tf_main.h"
#include "drc.h"

static DRC_PK drtab[NUM_DRC_MODES];
static short drc_band_top_tab[NUM_DRC_MODES][MAX_DRC_BANDS];
static short dyn_rng_sgn_tab[NUM_DRC_MODES][MAX_DRC_BANDS];
static short dyn_rng_ctl_tab[NUM_DRC_MODES][MAX_DRC_BANDS];
static short exclude_mask_tab[NUM_DRC_MODES][MAX_TIME_CHANNELS];

void initDynRng(void)
{
	DRC_VARS *drlong, *drshort;

	drtab[0].dvars = (DRC_VARS *)calloc (1, sizeof(DRC_VARS));
	drtab[1].dvars = (DRC_VARS *)calloc (1, sizeof(DRC_VARS));

	drlong = drtab[0].dvars;
	drshort = drtab[1].dvars;

	drlong->drc_band_top = drc_band_top_tab[0];
	drlong->dyn_rng_sgn = dyn_rng_sgn_tab[0];
	drlong->dyn_rng_ctl = dyn_rng_ctl_tab[0];
	drlong->exclude_mask = exclude_mask_tab[0];
	drshort->drc_band_top = drc_band_top_tab[1];
	drshort->dyn_rng_sgn = dyn_rng_sgn_tab[1];
	drshort->dyn_rng_ctl = dyn_rng_ctl_tab[1];
	drshort->exclude_mask = exclude_mask_tab[1];
}

/* set dynamic range parameters */

int setDynRngVars( DRC_INTF *dvars_inpt, 
				   enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS],
				   int nchans)
{
	int i, framedr, chan, is_negative;
	int numbits = 0;			/* set numbits to 4 to account for extension type */
	int sign = 1;
	DRC_VARS *drlong, *drshort;
	int numbits_long = 0;

	drlong = drtab[0].dvars;
	drshort = drtab[1].dvars;

/* Determine if dyn range words will be sent for each short block (if in short 
   block mode), or sent once if in long block or in a transition block */

	drtab[0].exists = 0;
	drtab[1].exists = 0;
	for (chan = 0; chan < nchans; chan++)
	{
		if (block_type[chan] == ONLY_SHORT_WINDOW)
		{
			drtab[1].exists = 1;				/* At least one channel is short block */
			break;
		}
	}
	for (chan = 0; chan < nchans; chan++)
	{
		if (block_type[chan] != ONLY_SHORT_WINDOW)
		{
			drtab[0].exists = 1;				/* At least one channel is not short block */
			break;
		}
	}

	for (chan = 0; chan < nchans; chan++)
	{
		if (block_type[chan] == ONLY_SHORT_WINDOW)
		{
			drlong->exclude_mask[chan] = 1;		/* flag long blk chan to be excluded from short blk drc */
			drshort->exclude_mask[chan] = 0;
		}
		else
		{
			drlong->exclude_mask[chan] = 0;
			drshort->exclude_mask[chan] = 1;	/* flag short blk chan to be excluded from long blk drc */
		}
	}
	for (chan = nchans; chan < ((nchans - 1) / 7 + 1) * 7; chan++) /* exclude channels that do not exist */
	{
		drlong->exclude_mask[chan] = 1;
		drshort->exclude_mask[chan] = 1;	
	}

/* Compute maximum gain reduction or expansion over entire long block frame */

	if (drtab[0].exists)					/* drtab[0] contains long block info */
	{
		numbits += 4;						/* reserve for extension type bits */
		drlong->excluded_chns_present = 0;
		numbits += 1;						/* reserve for excluded_chns_present */

		drlong->drc_bands_present = 0;
		numbits += 1;

		for (chan = 0; chan < nchans; chan++)
		{
			if (drlong->exclude_mask[chan])
			{
				drlong->excluded_chns_present = 1;	/* long block mode excludes short block chans */
				numbits += (((nchans - 1) / 7 + 1) * 8); /* reserve for exclude_mask */
				break;
			}
		}

		sign = 0;
		for (i = 0; i < MAX_SHORT_WINDOWS; i++)	 /* determine if any are negative (gain reduction) */
		{
			if (dvars_inpt->dyn_rng_sgn[i] == 1) /* dynamic range word is negative */
			{
				sign = 1;
				break;
			}
		}

		framedr = 0;
		if (sign == 1)			/* level compression takes precedence over level expansion*/
		{						/* for averaging dyn range words within a frame. */
			for (i = 0; i < MAX_SHORT_WINDOWS; i++)
			{
				is_negative = (dvars_inpt->dyn_rng_sgn[i] == 1) ? (-1) : (1);
				framedr = min(framedr, (dvars_inpt->dyn_rng_ctl[i] * is_negative));
			}
			framedr = abs(framedr);
		}
		else
		{
			for (i = 0; i < MAX_SHORT_WINDOWS; i++)
			{
				framedr = max(framedr, (dvars_inpt->dyn_rng_ctl[i]));
			}
		}

		drlong->dyn_rng_ctl[0] = framedr;
		drlong->dyn_rng_sgn[0] = sign;
		numbits += 8;
		drlong->prog_ref_level_present = 1;
		numbits += 1;
		drlong->prog_ref_level = dvars_inpt->prog_ref_level;
		numbits += 8;			/* includes 1 reserve bit */
		drlong->pce_tag_present = 0;
		numbits += 1;
		drtab[0].nbytes = numbits / 8;
	} /* if drtab[0].exists */
	numbits_long = numbits;
	numbits = 0;

/* Compute gain reduction and expansion over entire short block frame.  Frequency bins in decoder must
   have the short block bins concatenated and non-interleaved such that the lowest 128 bins in frequency 
   correspond to the first short block in the frame. */

	if (drtab[1].exists)					/* drtab[1] contains short block info */
	{
		numbits += 4;						/* reserve for extension type bits */
		drshort->excluded_chns_present = 0;
		numbits += 1;						/* reserve for excluded_chns_present */

		for (chan = 0; chan < nchans; chan++)
		{
			if (drshort->exclude_mask[chan])
			{
				drshort->excluded_chns_present = 1; /* short block mode excludes long block chans */
				numbits += (8 * ((nchans - 1) / 7 + 1)); /* reserve for exclude_mask */
				break;
			}
		}

		for (i = 0; i < MAX_SHORT_WINDOWS; i++)
		{
			drshort->dyn_rng_ctl[i] = dvars_inpt->dyn_rng_ctl[i];
			drshort->dyn_rng_sgn[i] = dvars_inpt->dyn_rng_sgn[i];
			numbits += 8;
			drshort->drc_band_top[i] = 32 * (i + 1) - 1;
			numbits += 8;
		}
		if (!drtab[0].exists)		/* if no long block, send program reference level */
		{
			drshort->prog_ref_level_present = 1;
			numbits += 1;
			drshort->prog_ref_level = dvars_inpt->prog_ref_level;
			numbits += 8;			/* includes 1 reserve bit */
		}
		else
		{
 			drshort->prog_ref_level_present = 0;
			numbits += 1;
		}
		drshort->pce_tag_present = 0;
		numbits += 1;
		drshort->drc_bands_present = 1;
		numbits += 1;
		drshort->drc_band_incr = MAX_SHORT_WINDOWS - 1;
		numbits += 8;				/* includes 4 reserve bits */

		drtab[1].nbytes = numbits / 8;
	} /* if drtab[1].exists */
	return (numbits_long + numbits);
}


/*****************************************************************************/
/* WriteDRCArray(...)														 */
/* Write dynamic range words and program ref level words to the bitstream.	 */
/*****************************************************************************/
int WriteDRCArray(BsBitStream* ptrBs,	  /* Pointer to bitstream */
				  int nchans)
{
	int maxCount, modes;
	int numbits = 0;

	maxCount = (1<<LEN_F_CNT) - 1;		/* Max count without escaping */

	for (modes = 0; modes < NUM_DRC_MODES; modes++)
	{
		if (drtab[modes].exists)
		{
			BsPutBit(ptrBs,ID_FIL,LEN_SE_ID);	/* Write fill_element ID */
			numbits += LEN_SE_ID;				/* Subtract for ID,count */
	
			if (drtab[modes].nbytes > maxCount)
			{
				BsPutBit(ptrBs, maxCount, 4);
				numbits += 4;
				BsPutBit(ptrBs, (drtab[modes].nbytes - maxCount + 1), LEN_BYTE);
				numbits += LEN_BYTE;
			}
			else
			{
				BsPutBit(ptrBs, drtab[modes].nbytes, 4);
				numbits += 4;
			}
			BsPutBit(ptrBs, EXT_DYNAMIC_RANGE, 4);
			numbits += 4;
			numbits += WriteDRCInfo (ptrBs,				/* in: bit stream */
								   drtab[modes].dvars,	/* drc structure */
								   nchans);				/* total num chans */
		}
	}
	return (numbits);
}


/*****************************************************************************/
/* WriteDRCInfo(...), write dynamic range control data.                           */
/*****************************************************************************/

int WriteDRCInfo  (BsBitStream *fixedStream,  /* in: bit stream */
				   DRC_VARS *dvars,
				   int nchans)
{
	short numbits = 0, indx;
	short drc_num_bands = 1;
	short drc_tag_reserved_bits = 0xA;
	short drc_bands_reserved_bits = 0xA;
	short prog_ref_level_reserved_bits = 0x1;

	BsPutBit(fixedStream, dvars->pce_tag_present, 1);		 /* PCE element tag */
	numbits += 1;
	if (dvars->pce_tag_present)
	{
		BsPutBit(fixedStream, dvars->pce_instance_tag, LEN_TAG); /* PCE instance bits */
		BsPutBit(fixedStream, drc_tag_reserved_bits, 4);	     /* drc reserved bits */
		numbits += 8;
	}

	BsPutBit(fixedStream, dvars->excluded_chns_present, 1);  /* exclude channels from dyn range */
	numbits += 1;
	if (dvars->excluded_chns_present)
	{
		numbits += WriteExclDRCInfo(fixedStream, dvars, nchans);
	}

	BsPutBit(fixedStream, dvars->drc_bands_present, 1);		 /* multiple dyn range bands */
	numbits += 1;
	if (dvars->drc_bands_present)
	{
		BsPutBit(fixedStream, dvars->drc_band_incr, LEN_TAG); /* number of dyn range words */
		BsPutBit(fixedStream, drc_bands_reserved_bits, 4);	  /* drc reserved bits */
		drc_num_bands += dvars->drc_band_incr;
		numbits += 8;
		for (indx = 0; indx < drc_num_bands; indx++)
		{
			BsPutBit(fixedStream, dvars->drc_band_top[indx], 8); /* last bin for each DRC word */
			numbits += 8;
		}	
	}

	BsPutBit(fixedStream, dvars->prog_ref_level_present, 1); /* element ID */
	numbits += 1;
	if (dvars->prog_ref_level_present)
	{
		BsPutBit(fixedStream, dvars->prog_ref_level, 7);	    /* program reference level */
		BsPutBit(fixedStream, prog_ref_level_reserved_bits, 1); /* drc reserved bits */
		numbits += 8;
	}
	
	for (indx = 0; indx < drc_num_bands; indx++)
	{
		BsPutBit(fixedStream, dvars->dyn_rng_sgn[indx], 1); /* dyn range sign bit */
		BsPutBit(fixedStream, dvars->dyn_rng_ctl[indx], 7); /* dyn range value */
		numbits += 8;
	}

	return (numbits);
} /* WriteDRCData */


int WriteExclDRCInfo(BsBitStream *fixedStream,  /* in: bit stream */
						DRC_VARS *dvars,
						int nchans)
{
	short numbits = 0, indx;
	short num_excl_chans = 7;
	short exclude_cnt = 0;

	for (indx = 0; indx < num_excl_chans; indx++)
	{
		BsPutBit(fixedStream, dvars->exclude_mask[indx], 1); /* excl chn bit */
	}
	numbits += 8;

	if (nchans > num_excl_chans)
	{
		exclude_cnt = nchans - num_excl_chans;
		while (exclude_cnt > 0)
		{
			BsPutBit(fixedStream, 1, 1); /* enable additional excluded channels */
			for (indx = num_excl_chans; indx < (num_excl_chans + 7); indx++)
			{
				BsPutBit(fixedStream, dvars->exclude_mask[indx], 1); /* excl chn bit */
			}
			numbits += 8;
			num_excl_chans += 7;
			exclude_cnt -= 7;
		}
	}
	else
	{
		BsPutBit(fixedStream, 0, 1);	/* indicate no additional excluded channels */
	}
	return (numbits);
}
