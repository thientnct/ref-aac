/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Felip Riera-Palou, Philips Research Eindhoven
* Andy Gerrits, Philips Research Eindhoven
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

Source file: laguerre_analysis.c

Required libraries: <none>

Authors:

Changes:
************************************************************************/

/**************************************************************************
 *
 * Laguerre Analysis module implementation
 *
 * laguerre_analysis.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __lagana_h
#define __lagana_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef struct lagstate_tag
{
    SInt Lp;  		
    Double lambda;
    SInt EZ;
			
    Double *a;
	Double *b;
	
    Double b1;
	
    Double *states;
    Double *out;
    Double delta;
	
	Double *r;
	Double *p;

	Double *y;
	Double *z;
	Double alpha;
	Double beta;
	Double mu;
	Double inprod;

    SSCLOG*        pLog;

}SSC_LAG_STATE, *PSSC_LAG_STATE ;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void            SSC_LAG_Initialise     ( void );
void            SSC_LAG_Free           ( void );

SSC_LAG_STATE* SSC_LAG_CreateInstance ( const SInt denLen,
		const Double lagPole, SSCLOG* pLog );

SSC_LAG_STATE* SSC_LAG_DestroyInstance( SSC_LAG_STATE* const pms );

void SSC_LAG_Analysis( SSC_LAG_STATE* const pms, const Double* const pData,
		const SInt countData, Double* const pLAGData );


#ifdef __cplusplus
}
#endif /* __lagana_h */

#endif


