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

Source file: Imp_noise_analysis.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001     AP/JD   Initial version
20-Sep-2002     NT      RM2 change
************************************************************************/

/**************************************************************************
 *
 * Noise_Analysis module implementation
 *
 * Imp_Noise_Analysis.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_Noise_Analysis_h
#define __Imp_Noise_Analysis_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "SSC_Log.h"

#include "SSC_System.h"
#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"

#include "laguerre_analysis.h"
#include "Filter.h"
#include "LagFilter.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct ssc_noise_analysisinfo_tag
{
    SInt DecoderLevel;
    SInt Mode;
    SInt ModeExt;
    SInt Channels;
    SInt SubframeSize;
    SInt NumOrder;
    SInt DenOrder;
	Double Pole;
}SSC_NOISE_ANALYSIS_INFO, *PSSC_NOISE_ANALYSIS_INFO ;


typedef struct ssc_noise_analysisstate_tag
{
    SSCLOG*       pLogState ;
    SSCPIPE*      pPipeState ;
    SSCFILTER*    pFilterState[SSC_MAX_CHANNELS];
    SSCLAGFILTER* pLagFilter[SSC_MAX_CHANNELS];

    Double*          ppDataVector[SSC_MAX_CHANNELS];

    Double*          ppLagResidual[SSC_MAX_CHANNELS];

    SSC_LAG_STATE*   pLagState;

    SSC_NOISE_ANALYSIS_INFO Info;

    SInt          Channels;

    /* temporary buffer where all sampples are placed for analysing*/
    SInt          anaLength;

    /* the numerator/denominator are saved and used for the next subframe*/
    Double*       prevDenVector[SSC_MAX_CHANNELS];

    /* temp PolyA/Denominator buffer/length */
    Double*       pLarDenVector;
    Double*       pAparDenVector;
    Double*       pDenVector;
    SInt          denLen;

} SSC_NOISE_ANALYSIS_STATE, *PSSC_NOISE_ANALYSIS_STATE ;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void                     IMP_NOISE_ANALYSIS_Initialise     ( void );

SSC_NOISE_ANALYSIS_STATE* IMP_NOISE_ANALYSIS_CreateInstance ( const SSC_NOISE_ANALYSIS_INFO* const pNoise_AnalysisInfo, Char* szLogFile, SInt nTraceLevel );
SSCBUNCH*                 IMP_NOISE_ANALYSIS_Process        ( SSC_NOISE_ANALYSIS_STATE* const pNoise_AnalysisState, SSCBUNCH* const pSscWorkBunch );
SSC_NOISE_ANALYSIS_STATE* IMP_NOISE_ANALYSIS_DestroyInstance( SSC_NOISE_ANALYSIS_STATE* const pNoise_AnalysisState );

void                     IMP_NOISE_ANALYSIS_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_Noise_Analysis_h */

#endif



