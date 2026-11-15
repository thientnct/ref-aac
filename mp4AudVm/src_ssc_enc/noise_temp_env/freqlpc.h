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

Source file: freqlpc.h

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
 * FREQLPC module implementation
 *
 * freqlpc.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __freqlpc_h
#define __freqlpc_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "SSC_Log.h"
#include "SSC_fft_ff.h"
#include "SSC_fft.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct freqlpcstate_tag
{
    SInt     envOrder;

    SInt     dataLength;

    SSCLOG*             pLog;
    SSC_FFT_FF_STATE*   pFft;

    Double*  pSignal;
    Double*  pFftData;    
    Double*  pRSignal;

    Double* S1;
    Double* ac;
    Double* ac1;

}SSC_FREQLPC_STATE, *PSSC_FREQLPC_STATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void            SSC_FREQLPC_Initialise     ( void );
void            SSC_FREQLPC_Free           ( void );

SSC_FREQLPC_STATE* SSC_FREQLPC_CreateInstance ( SSCLOG* pLog, const SInt envOrder, const SInt dataLength );
SSC_FREQLPC_STATE* SSC_FREQLPC_DestroyInstance( SSC_FREQLPC_STATE* const pms );

void SSC_FREQLPC_Calculate( SSC_FREQLPC_STATE* const pms, 
                            const Double* const pData,
                            Double* const pLpcPars );


#ifdef __cplusplus
}
#endif /* __freqlpc_h */

#endif



