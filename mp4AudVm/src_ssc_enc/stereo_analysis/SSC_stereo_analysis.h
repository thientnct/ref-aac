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

Source file: SSC_stereo_analysis.h

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
 * Stereo_Analysis module 
 *
 * SSC_Stereo_Analysis.h
 *
 * contains all structures and prototypes of the interface
 *
 **************************************************************************/

#ifndef __SSC_Stereo_Analysis_h
#define __SSC_Stereo_Analysis_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#ifdef _SSC_LIB_BUILD
#include "Imp_Types.h"
#else
#ifndef __SSC_MODULESTATE
#define __SSC_MODULESTATE
    typedef struct SSC_MODULESTATE_tag SSC_MODULESTATE, *PSSC_MODULESTATE ;
#endif
#endif

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_STEREO_ANALYSIS_InitInstance    ( ENCODERINFO* pEncoderInfo );
SSC_INTF_EXPORT SSCBUNCH*        SSC_STEREO_ANALYSIS_Process         ( SSC_MODULESTATE* pms, SSCBUNCH* pSBunch, SSC_ERROR* pnError );
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_STEREO_ANALYSIS_DestroyInstance ( SSC_MODULESTATE* pms );

#ifdef __cplusplus
}
#endif /* __SSC_Stereo_Analysis_h */

#endif



