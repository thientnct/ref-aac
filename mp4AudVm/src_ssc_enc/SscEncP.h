/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Philips Digital Systems Laboratory

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.

Source file: SscEncP.h


Required modules:


Authors:
RJ    Robert Janssen, Philips <robert.janssen@philips.com>

Changes:
02-may-2003   RJ    initial version
**********************************************************************/


#ifndef _SSCENCP_H_
#define _SSCENCP_H_

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
#include <time.h>
#include <math.h>

#include "PlatformTypes.h"
#include "cexcept.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"

#include "SSC_Error.h"
#include "SSC_Log.h"
#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"

/* #include "sndfile.h" */
#undef _SSC_LIB_BUILD
#include "audio_read/SSC_Audio.h"
#include "filter/SSC_Filter.h"
#include "transient_detection/SSC_Transient_Detection.h"
#include "stereo_analysis/SSC_stereo_analysis.h"
#include "transient_analysis/SSC_transient_analysis.h"
#include "transient_synthesis/SSC_Transient_Synthesis.h"
#include "transient_quantisation/SSC_transient_quantisation.h"
#include "sinusoid_analysis/SSC_sinusoid_analysis.h"
#include "sinusoid_synthesis/SSC_sinusoid_synthesis.h"
#include "sinusoid_psycho/SSC_sinusoid_psycho.h"
#include "sinusoid_remove/SSC_sinusoid_remove.h"
#include "sinusoid_tracking/SSC_sinusoid_tracking.h"
#include "stereo_quantisation/SSC_stereo_quantisation.h"
#include "sinusoid_quantisation/SSC_sinusoid_quantisation.h"
#include "sinusoid_cont_phase/SSC_sinusoid_cont_phase.h"
#include "noise_analysis/SSC_noise_analysis.h"
#include "noise_temp_env/SSC_noise_temp_env.h"
#include "noise_quantisation/SSC_noise_quantisation.h"
#include "formatter/SSC_Formatter.h"

typedef SSC_MODULESTATE SSC_AUDIO_STATE;
typedef SSC_MODULESTATE SSC_FILTER_STATE;
typedef SSC_MODULESTATE SSC_TRAN_DETECTION_STATE;
typedef SSC_MODULESTATE SSC_STEREO_ANALYSIS_STATE;
typedef SSC_MODULESTATE SSC_TRAN_ANALYSIS_STATE;
typedef SSC_MODULESTATE SSC_TRAN_SYNTHESIS_STATE;
typedef SSC_MODULESTATE SSC_TRAN_QUANTISATION_STATE;
typedef SSC_MODULESTATE SSC_SIN_ANALYSIS_STATE;
typedef SSC_MODULESTATE SSC_SIN_SYNTHESIS_STATE;
typedef SSC_MODULESTATE SSC_SIN_PSYCHO_STATE;
typedef SSC_MODULESTATE SSC_SIN_REMOVE_STATE;
typedef SSC_MODULESTATE SSC_SIN_TRACKING_STATE;
typedef SSC_MODULESTATE SSC_STEREO_QUANTISATION_STATE;
typedef SSC_MODULESTATE SSC_SIN_QUANTISATION_STATE;
typedef SSC_MODULESTATE SSC_SIN_CONTINUOUS_PHASE_STATE;
typedef SSC_MODULESTATE SSC_NOISE_ANALYSIS_STATE;
typedef SSC_MODULESTATE SSC_NOISE_TEMP_ENV_STATE;
typedef SSC_MODULESTATE SSC_NOISE_QUANTISATION_STATE;
typedef SSC_MODULESTATE SSC_BITSTREAM_STATE;

typedef struct tagProcessInfo
{
    SSC_AUDIO_STATE                    *pas;     /* ptr audio state */
    SSC_FILTER_STATE                   *pfs;     /* ptr filter state */
    SSC_TRAN_DETECTION_STATE           *ptds;    /* ptr tran detect state */
    SSC_STEREO_ANALYSIS_STATE          *pstas;   /* ptr tran analysis state */
    SSC_TRAN_ANALYSIS_STATE            *ptas;    /* ptr tran analysis state */
    SSC_TRAN_SYNTHESIS_STATE           *ptss;    /* ptr tran synth state */
    SSC_TRAN_QUANTISATION_STATE        *ptqs;    /* ptr tran quant state */
    SSC_SIN_ANALYSIS_STATE             *psas;    /* ptr sin analysis state */
    SSC_SIN_SYNTHESIS_STATE            *psss;    /* ptr sin synthesis state */
    SSC_SIN_PSYCHO_STATE               *psps;    /* ptr sin psycho state */
    SSC_SIN_REMOVE_STATE               *psrs;    /* ptr sin remove state */
    SSC_SIN_TRACKING_STATE             *psts;    /* ptr sin tracking state */
    SSC_STEREO_QUANTISATION_STATE      *pstqs;   /* ptr sin quantisation state */
    SSC_SIN_QUANTISATION_STATE         *psqs;    /* ptr sin quantisation state */
    SSC_SIN_CONTINUOUS_PHASE_STATE     *pscps;   /* ptr sin cont phase state */
    SSC_NOISE_ANALYSIS_STATE           *pnas;    /* ptr noise analysis state */
    SSC_NOISE_TEMP_ENV_STATE           *pntes;   /* ptr noise temporal envelope state */
    SSC_NOISE_QUANTISATION_STATE       *pnqs;    /* ptr noise quant state */
    SSC_BITSTREAM_STATE                *pbsfs;   /* ptr bitstream state */
    clock_t                            t_start;  /* start time of encoding */
} PROCESSINFO;

void SSC_ParseCommandLine(char * AudioFileName,
                          char * BitFileName,
                          ENCODERINFO *pEI);

void SSC_PrintTiming(PROCESSINFO *pPI, ENCODERINFO *pEI, SInt nSfCount);

void SSC_PrintBitrate(ENCODERINFO *pEI, SInt nSfCount, SInt nBits);

SInt SSC_AddBits(SSCBUNCH *pSscBunch);

void SSC_ClearSscBunch(ENCODERINFO *pEI, SSCBUNCH *pSSCBunch);

#endif

