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

Source file: Imp_Transient_Synthesis.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/

/**************************************************************************
 *
 * Transient_Synthesis module implementation
 *
 * Imp_Transient_Synthesis.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_Transient_Synthesis_h
#define __Imp_Transient_Synthesis_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define TRANS_SYNTH_LENGTH  2 /* transient synthesis length in sf */

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct ssc_transient_synthesisinfo_tag
{
    SInt    DecoderLevel;
    SInt    Mode;
    SInt    ModeExt;
    SInt    Channels;
    SInt    SubframeSize;
    Double *pSSC_Chi ;          /* [nCntChi][nCntB] */
    Double *pSSC_a_max ;        /* [nCntChi][nCntB] */
    SInt    nCntChi ;
    SInt    nCntB ;
    SInt    nSynthesizeLength ; /* synthesize length in samples */
}SSC_TRANSIENT_SYNTHESIS_INFO, *PTRANSIENT_SYNTHESISINFO ;

typedef struct ssc_transient_synthesisstate_tag
{
    SSCLOG  *pLog ;
    SSCPIPE *pPipe ;
    SInt     nCntTransBuf;                 /* number of samples in transbuf per channel */
    Double  *pTransBuf[SSC_MAX_CHANNELS] ; /* pointer where the sum of complete transients is stored */
    Double  *pTransWorkBuf ;               /* work buffer where a transient is calculated temporarely */
    Double  *pSSC_Chi ;                    /* [nCntChi][nCntB] */
    Double  *pSSC_a_max ;                  /* [nCntChi][nCntB] */
    SInt     nCntChi ;
    SInt     nCntB ;
    SInt     nSynthesizeLength ; /* nSynthesizeLength in samples */
    SInt     nChannels;
}SSC_TRANSIENT_SYNTHESIS_STATE, *PTRANSIENT_SYNTHESISSTATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void                      IMP_TRANSIENT_SYNTHESIS_Initialise     ( void );
PTRANSIENT_SYNTHESISSTATE IMP_TRANSIENT_SYNTHESIS_CreateInstance ( PTRANSIENT_SYNTHESISINFO pTransient_SynthesisInfo, Char *szLogFile, SInt nTraceLevel );
SSCBUNCH*                 IMP_TRANSIENT_SYNTHESIS_Process        ( PTRANSIENT_SYNTHESISSTATE pTransient_SynthesisState, SSCBUNCH *pSscWorkBunch );
void                      IMP_TRANSIENT_SYNTHESIS_DestroyInstance( PTRANSIENT_SYNTHESISSTATE *ppTransient_SynthesisState );
void                      IMP_TRANSIENT_SYNTHESIS_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_Transient_Synthesis_h */

#endif



