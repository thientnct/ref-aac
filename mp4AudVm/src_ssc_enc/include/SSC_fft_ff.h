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

Source file: SSC_fft_ff.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
10-Sep-2002 NT      SSC_FFT_FF_GetPRe function added
************************************************************************/

/**************************************************************************
 *
 * SSC FFT module implementation
 *
 * SSC_fft_ff.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __SSC_fft_ff_h
#define __SSC_fft_ff_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "SSC_Log.h"
#include "SSC_fft.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef enum ssc_fft_fftype_tag
{
    SSC_FFT_FF_FLOAT  = 0,
    SSC_FFT_FF_DOUBLE = 1
}SSC_FFT_FF_TYPE;

typedef struct ssc_fft_ffstate_tag
{

    SSCLOG*  pLog;

    SInt         sizeFFT;
    SSC_FFT_FF_TYPE type;

    Double* pRData;
    Double* pIData;
    SSC_FFT_INFO *pFft ;
}SSC_FFT_FF_STATE, *PSSC_FFT_FF_STATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void            SSC_FFT_FF_Initialise     ( void );
void            SSC_FFT_FF_Free           ( void );

SSC_FFT_FF_STATE* SSC_FFT_FF_CreateInstance ( SSCLOG* const pLog, const SInt sizeFFT, SSC_FFT_FF_TYPE type);
SSC_FFT_FF_STATE* SSC_FFT_FF_DestroyInstance( SSC_FFT_FF_STATE* const pms );

void SSC_FFT_FF_FftCalculate ( SSC_FFT_FF_STATE* const pms, const void* const pData, void* const pResult);
void SSC_FFT_FF_IFftCalculate( SSC_FFT_FF_STATE* const pms, const void* const pData, void* const pResult);

Double *SSC_FFT_FF_GetPRe( const SSC_FFT_FF_STATE * const pFft );


#ifdef __cplusplus
}
#endif /* __SSC_fft_ff_h */

#endif

