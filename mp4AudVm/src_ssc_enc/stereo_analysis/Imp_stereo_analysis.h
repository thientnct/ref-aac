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

Source file: Imp_stereo_analysis.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
18-Sep-2002	NT	Initial version
************************************************************************/


/************************************************************************** 
 *
 * Stereo_Analysis module implementation
 *
 * Imp_stereo_analysis.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_Stereo_Analysis_h
#define __Imp_Stereo_Analysis_h

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

#include "SSC_fft_ff.h"
#include "SSC_fft.h"

#include "ct_polyphase_ssc.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef enum {

  HYBRID_2_REAL     = 2,
  HYBRID_4_CPLX     = 4,
  HYBRID_6_CPLX     = 6,
  HYBRID_8_CPLX     = 8,
  HYBRID_12_CPLX    = 12

} HYBRID_RES;

typedef struct
{
  int   nQmfBands;
  int   frameSizeInit;
  int   frameSize;
  int   *pResolution;
  int   qmfBufferMove;

  double *pWorkReal;         /**< Working arrays for Qmf samples. */
  double *pWorkImag;

  double **mQmfBufferReal;   /**< Stores old Qmf samples. */
  double **mQmfBufferImag;
  double **mTempReal;        /**< Temporary matrices for filter bank output. */
  double **mTempImag;

} HYBRID;

typedef HYBRID *HANDLE_HYBRID;


typedef struct ssc_stereo_analysisinfo_tag
{
    SInt         DecoderLevel;
    SInt         Channels;    
    SInt         SubframeSize;
    Double       Fs;
    SInt         Mode;
    SInt         ModeExt;
    SInt         iidBinsIndex;
}SSC_STEREO_ANALYSISINFO, *PSSC_STEREO_ANALYSISINFO ;

typedef struct ssc_stereo_analysisstate_tag
{
    SSCLOG*     pLogState;
    SSCPIPE*    pPipeState;

    SInt        channels; 
    SInt        subframeSize;
    SInt        mode;
    SInt        mode_ext;
    Double      Fs;

    Bool        firstTime;
    UInt        nBufferLength;
    Double*     ppData[SSC_MAX_CHANNELS];
    Double*     mData;
    SInt        mCount;

    struct SSC_ANA_FILTERBANK qmfAnalysisFilterBankLeft;
    struct SSC_ANA_FILTERBANK qmfAnalysisFilterBankRight;
    struct SSC_ANA_FILTERBANK qmfAnalysisFilterBankMid;

    Double **ppAnalysisLeftReal;
    Double **ppAnalysisLeftImag;       
    Double **ppAnalysisRightReal;
    Double **ppAnalysisRightImag;       
    Double **ppAnalysisMidReal;
    Double **ppAnalysisMidImag;       

    Double **mHybridRealLeft;
    Double **mHybridImagLeft;
    HANDLE_HYBRID hHybridLeft;

    Double **mHybridRealRight;
    Double **mHybridImagRight;
    HANDLE_HYBRID hHybridRight;

    Double **mHybridRealMid;
    Double **mHybridImagMid;
    HANDLE_HYBRID hHybridMid;

    Bool        enableIid;
    Bool        enableIcc;
    Bool        enableIpd;
    Bool        enableExt;
    Bool        enablePca;

    SInt        nrOfIidBins;
    SInt        nrOfIccBins;
    SInt        nrOfIpdBins;
    Double      iid[SSC_ENCODER_MAX_STEREO_BINS];
    Double      ipd[SSC_ENCODER_MAX_STEREO_BINS];
    Double      opd[SSC_ENCODER_MAX_STEREO_BINS];
    Double      icc[SSC_ENCODER_MAX_STEREO_BINS];

}SSC_STEREO_ANALYSISSTATE, *PSSC_STEREO_ANALYSISSTATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void         IMP_STEREO_ANALYSIS_Initialise     ( void );
SSC_STEREO_ANALYSISSTATE* IMP_STEREO_ANALYSIS_CreateInstance ( const SSC_STEREO_ANALYSISINFO* const ssc_stereo_analysisInfo, Char* szLogFile, SInt nTraceLevel );
SSCBUNCH*    IMP_STEREO_ANALYSIS_Process        ( SSC_STEREO_ANALYSISSTATE* const pstereo_analysisState, SSCBUNCH* const pSscWorkBunch );
SSC_STEREO_ANALYSISSTATE* IMP_STEREO_ANALYSIS_DestroyInstance( SSC_STEREO_ANALYSISSTATE* const ppstereo_analysisState );
void         IMP_STEREO_ANALYSIS_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_Stereo_Analysis_h */

#endif



