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

Source file: win.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
************************************************************************/

/************************************************************************** 
 *
 * WIN module implementation
 *
 * win.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __win_h
#define __win_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "SSC_Log.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef enum _SSC_WIN_TYPE
{
    SSC_WIN_FLOAT  = 0,
    SSC_WIN_DOUBLE = 1
}SSC_WIN_TYPE;

typedef enum _SSC_WIN_FORM
{
    SSC_WIN_RECTANGUAL     = 0,
    SSC_WIN_HANNING        = 1,
    SSC_WIN_SQRT_HANNING   = 2,

    SSC_WIN_HANNING_BURAK  = 3  /* only for debugging */
}SSC_WIN_FORM;

typedef struct winstate_tag
{

    SSCLOG*      pLog;
    SSC_WIN_TYPE type;

    SInt         winSize;
    
}SSC_WIN_STATE, *PSSC_WIN_STATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void            SSC_WIN_Initialise     ( void );
void            SSC_WIN_Free           ( void );

SSC_WIN_STATE* SSC_WIN_CreateInstance ( SSCLOG* const pLog, const SInt winSize, const SSC_WIN_TYPE type );
SSC_WIN_STATE* SSC_WIN_DestroyInstance( SSC_WIN_STATE* const pms );

SInt           SSC_WIN_Create         ( SSC_WIN_STATE* const pms, const SSC_WIN_FORM winForm, void* const pWindow);


#ifdef __cplusplus
}
#endif /* __win_h */

#endif



