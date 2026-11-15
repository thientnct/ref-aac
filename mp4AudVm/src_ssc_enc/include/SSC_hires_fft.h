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

Source file: SSC_hires_fft.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
25-Sep-2001	AP/JD	Initial version
************************************************************************/

/**************************************************************************
 *
 * HIRES FFT module implementation
 *
 * SSC_hires_fft.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __SSC_HIRES_FFT_h
#define __SSC_HIRES_FFT_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "cexcept.h"
#include "SSC_fft.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define SSC_HIRES_FFT_ERROR_OK                0
#define SSC_HIRES_FFT_MEMORY_ERROR            1
#define SSC_HIRES_FFT_INVALID_STATE_POINTER   2
#define SSC_HIRES_FFT_INVALID_INPUT_PARAMETER 3
#define SSC_HIRES_FFT_INVALID_RETURN_BUFFER   4
#define SSC_HIRES_FFT_PROCESS_ERROR           5
/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef struct ampinfo_tag
{
    Double dAmplitude ;
    SInt   nOrgIdx ;
}AMPINFO; AMPINFO *PAMPINFO ;

typedef struct hiresfftinfo_tag
{
    /* copy of input data */
    SInt    nSinusoids;        /* max number of frequencies to extract */
    Double  Fs ;               /* sample frequency */
    Double  FreqLimit ;        /* extract frequencies below this limit */
    SInt    nLen ;             /* length of data */

    /* derived input paraters */
    SInt    nAnaLength ;

    /* pointer to allocated memory */
    Double       *pWindow ;
    AMPINFO      *pAmpInfo ;
    Double       *pSigfft ;
    Double       *pSig_d_fft ;
    Double       *pWorkBuf ;
    Double       *pSigWin ;
    SSC_FFT_INFO *pFftSigWin ;
    SSC_FFT_INFO *pFft ;

}SSC_HIRES_FFT_INFO, *PSSC_HIRES_FFT_INFO;



/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void                SSC_HIRES_FFT_Initialise     ( void );
void                SSC_HIRES_FFT_Free           ( void );

SSC_HIRES_FFT_INFO *SSC_HIRES_FFT_CreateInstance (
                                                   SInt    nLen,
                                                   SInt    nSinusoids,
                                                   Double  Fs,
                                                   Double  FreqLimit
                                                 );
SInt                SSC_HIRES_FFT_Process        ( SSC_HIRES_FFT_INFO * const pHiresFft, Double *pSegment,
                                                   Double *pFreq, Double *pAmp, Double *pPhase );
void                SSC_HIRES_FFT_DestroyInstance( SSC_HIRES_FFT_INFO **ppHiresFft );


#ifdef __cplusplus
}
#endif /* __SSC_HIRES_FFT_h */

#endif
