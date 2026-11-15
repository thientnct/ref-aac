/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
*

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright © 2001.

Source file: SSC_Bunch_Types.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AR: Aad Rijnberg,      Philips CE - PDS-labs Eindhoven <aad.rijnberg@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
16-Aug-2002 AR      Changes for m8540 core experiment
20-Sep-2002 NT      RM2 change
************************************************************************/



/*===========================================================================
 *
 *  Module      : SSC_Bunch_Types
 *
 *  File        : SSC_Bunch_Types.h
 *
 *  Description : This header file defines the SBunch types and subtypes.
 *
 ============================================================================*/


#ifndef _SSC_BUNCH_TYPES_H_
#define _SSC_BUNCH_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct encoderinfo_tag
{
    Char*  in_file_name;
    Char*  out_file_name;
    SInt   decoder_level;
    SInt   enhancement;
    SInt   sin_refresh;
    SInt   noise_refresh ;
    SInt   stereo_refresh;
    Double rel_threshold;
    SInt   grid_freq;
    SInt   grid_amp;
    SInt   grid_iid;
    SInt   grid_itd;
    SInt   grid_rho;
    SInt   phase_jitter_band;
    SInt   phase_jitter_percentage;
    Char*  in_dir_name;
    Char*  out_dir_name;
    SInt   nr_of_channels;
    SInt   channel_depend;
    SInt   fs;
    SInt   sampling_rate;
    SInt   update_rate;
    SInt   sf_size;
    Double number_samples;
    SInt   stereo_upd_rate;
    SInt   stereo_nr_of_bins_idx;
    SInt   stereo_nr_of_itd_bins_idx;
    SInt   nr_of_subframes;
    SInt   synthesis_method;
    SInt   encoder_version;
    SInt   mode;
    SInt   mode_ext;
    SInt   max_nr_of_sins;
    SInt   max_nr_of_t_sins;
    SInt   max_den_order;
    Double pole;
    SInt   max_env_order;
}ENCODERINFO;
typedef ENCODERINFO *PENCODERINFO;


typedef struct sineparms_tag
{
    Double *pdTrackIdx;
    Double *pdTrackPrev;
    Double *pdTrackNext;
    Double *pdFreq;
    Double *pdAmp;
    Double *pdPhaseOrg;
    Double *pdPhaseCont;
    Double *pdRel;              /* relevance criterium */
    Double *pdQuantGrid;
    SInt    nGridFreq;
    SInt    nGridAmp;
    SInt    nPhaseJitterBand;
    SInt    nPhaseJitterPercentage;
    Double  dRelThreshold;      /* relevance criterium threshold */

    /* for debugging and range checking */
    /* number of elements the pointer variables point to */
    SInt cntTrackIdx;
    SInt cntTrackPrev;
    SInt cntTrackNext;
    SInt cntFreq;
    SInt cntAmp;
    SInt cntPhaseOrg;
    SInt cntPhaseCont;
    SInt cntRel;
    SInt cntQuantGrid;
}SINEPARMS;
typedef SINEPARMS *PSINEPARMS;


typedef struct sineroot_tag
{
    PSINEPARMS pSparms;         /* [channels, 60] */
    PSINEPARMS pSQparms;        /* [channels, 60], quantized sine parameters */

    /* for debugging and range checking */
    SInt       cntSparms;       /* sineparms element count */
    SInt       cntSQparms;      /* quantized sineparms element count */
}SINEROOT;
typedef SINEROOT *PSINEROOT;


typedef struct tsineparms_tag
{
    SInt   nr_of_sins;
    Double *pdFreq;
    Double *pdAmp;
    Double *pdPhase;

    /* for debugging and range checking */
    SInt cntFreq;               /* frequency element count */
    SInt cntAmp;                /* amplitude element count */
    SInt cntPhase;              /* phase element count */
}TSINEPARMS;
typedef TSINEPARMS *PTSINEPARMS;


typedef struct transientparms_tag
{
    Bool   bTransient;          /* True if a transient is present, otherwise False */
    SInt   nPosition;           /* position of transient in sf 0..359 */
    SInt   nTran_type;
    Double dBpar;
    Double dChi;
    PTSINEPARMS pTSparms;

    /* for debugging and range checking */
    SInt    cntTSparms;         /* transient sineparms element count */
}TRANSIENTPARMS;
typedef TRANSIENTPARMS *PTRANSIENTPARMS;


typedef struct transientroot_tag
{
    PTRANSIENTPARMS pTparms ; /* [channels] */
    PTRANSIENTPARMS pTQparms; /* [channels], quantized parms */

    /* for debugging and range checking */
    SInt    cntTparms;  /* transient parms element count */
    SInt    cntTQparms; /* quantized transient parms element count */
}TRANSIENTROOT;
typedef TRANSIENTROOT *PTRANSIENTROOT;


typedef struct noiseparms_tag
{
    Double *pdNum;        /* [SSC_N_MAX_FILTER_COEFFICIENTS]; */
    Double *pdDen;        /* [SSC_N_MAX_FILTER_COEFFICIENTS]; */
    Double *pdEnvCoefs;   /* [SSC_N_MAX_LSF_ORDER]; */
    Double  dEnvGain;
    SInt    envOverlap;
    Double  dPoleLaguerre;
    SInt    nGridLaguerre;

    /* for debugging and range checking */
    SInt    cntNum;       /* numerator element count */
    SInt    cntDen;       /* denumerator element count */
    SInt    cntEnvCoefs ; /* envelope coefficient element count */
}NOISEPARMS;
typedef NOISEPARMS *PNOISEPARMS;


typedef struct noiseroot_tag
{
    PNOISEPARMS pNparms;  /* [nchannels] */
    PNOISEPARMS pNQparms; /* [nchannels], quantized noise parameters */

    /* for debugging and range checking */
    SInt    cntNparms;  /* noise parms element count */
    SInt    cntNQparms; /* quantized noise parms element count */
}NOISEROOT;
typedef NOISEROOT *PNOISEROOT;


typedef struct stereoparms_tag
{
    SInt   envBorder;
    
    Bool   header;    
    Bool   iidEnable; 
    Bool   iccEnable; 
    Bool   ipdEnable; 
    Bool   extEnable; 
    Bool   pcaEnable; 

    SInt   iidFreqRes;
    SInt   iidQuant;  
    SInt   iccFreqRes;
    
    Double *iid;
    Double *ipd;
    Double *opd;
    Double *icc;

}STEREOPARMS;
typedef STEREOPARMS *PSTEREOPARMS;


typedef struct stereoroot_tag
{
    PSTEREOPARMS pSTparms;
    PSTEREOPARMS pSTQparms;
    /* for debugging and range checking */
    SInt    cntSTparms;  /* stereo parms element count */
    SInt    cntSTQparms; /* quantized stereo parms element count */

}STEREOROOT;
typedef STEREOROOT *PSTEREOROOT;


typedef struct sbunch_tag
{
    SInt    sf_counter;
    SInt    data_from_stream;
    SInt    error_code;
    SInt    error_origin;
    Double *pactual_bitrate;
    Double *psignal;       /* [subframesize, channels] */
    Double *psynth;        /* reconstructed signal from module */
    Double *presidual;     /* residual signal */
    PTRANSIENTROOT pTroot; /* transient [channels] parameters */
    PSINEROOT  pSroot;     /* [channels, 60] sinusoidal parameters */
    PNOISEROOT pNroot;     /* noise [channels] parameters */

    PSTEREOROOT pSTroot;
    
    /* for debugging and range checking */
    SInt    cntBitrate;     /* bitrate element count */
    SInt    cntSignal;      /* psignal element count */
    SInt    cntSynth;       /* psynth element count */
    SInt    cntResidual;    /* presidual element count */
    SInt    cntTroot;       /* transient info element count */
    SInt    cntSroot;       /* sinusoidal info element count */
    SInt    cntNroot;       /* noise info element count */
    SInt    cntSTroot;      /* stereo info element count */
}SBUNCH;
typedef SBUNCH *PSBUNCH;


#ifdef __cplusplus
}
#endif
#endif /* _SSC_BUNCH_TYPES_H_ */
