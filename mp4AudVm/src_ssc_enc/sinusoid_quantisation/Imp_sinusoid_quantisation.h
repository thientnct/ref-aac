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

Source file: Imp_sinusoid_quantisation.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:     Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
20-Sep-2002 NT      RM2 change
21-Nov-2003 AG      Changes for RM4
************************************************************************/
/**************************************************************************
 *
 * sinusoid_quantisation module implementation
 *
 * Imp_sinusoid_quantisation.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_sinusoid_quantisation_h
#define __Imp_sinusoid_quantisation_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define INF                 (999999999.0)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct ssc_sinusoid_quantisationinfo_tag
{
    SInt DecoderLevel;
    SInt Channels;
    SInt Mode;
    SInt ModeExt;
    SInt SubframeSize;
    Double dFreq_rl_base;    /* 91.2,  */
    Double dAmp_rl_base;     /* 1.0218 */
    Double dPhase_rl_base;   /* 16 */
    Double dFs ;

    /* ADPCM Specific settings for the encoder */
    UInt   dNumADPCMBits;
    Double dT;

}SSC_SINUSOID_QUANTISATIONINFO, *PSINUSOID_QUANTISATIONINFO ;


typedef struct ssc_adpcm_state_tag
{
    /* ADPCM Specific settings for the encoder */
    Double dT;
    Double dUnwrappedPhase;    /* For monitoring the unwrapped phase */
    Double *pDelta;
    Double *pRepr;
    Double *pH;
    Double *pYz;
    Double dM;

}SSC_ADPCM_STATE, *PSSC_ADPCM_STATE ;


typedef struct ssc_sinusoid_quantisationstate_tag
{
    SSCLOG *pLog ;
    SSCPIPE *pPipe ;
    Double dFreq_rl_base;    /* 91.2,  */
    Double dAmp_rl_base;     /* 1.0218 */
    Double dPhase_rl_base;   /* 16 */
    Double dFs ;
    SInt   nChannels;

    /* ADPCM Specific settings for the encoder */
    UInt dNumLevels;
    UInt dPredMem;
    PSSC_ADPCM_STATE s_adpcm[SSC_MAX_CHANNELS];
    PSSC_ADPCM_STATE s_adpcm_tmp[SSC_MAX_CHANNELS];  /* Temporary array for Phase ADPCM */

}SSC_SINUSOID_QUANTISATIONSTATE, *PSINUSOID_QUANTISATIONSTATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void         IMP_SINUSOID_QUANTISATION_Initialise     ( void );
PSINUSOID_QUANTISATIONSTATE IMP_SINUSOID_QUANTISATION_CreateInstance ( PSINUSOID_QUANTISATIONINFO psinusoid_quantisationInfo, Char* szLogFile, SInt nTraceLevel );
SSCBUNCH*    IMP_SINUSOID_QUANTISATION_Process        ( PSINUSOID_QUANTISATIONSTATE pSQS, SSCBUNCH* pSscBunch );
void         IMP_SINUSOID_QUANTISATION_DestroyInstance( PSINUSOID_QUANTISATIONSTATE* ppsinusoid_quantisationState );
void         IMP_SINUSOID_QUANTISATION_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_sinusoid_quantisation_h */

#endif



