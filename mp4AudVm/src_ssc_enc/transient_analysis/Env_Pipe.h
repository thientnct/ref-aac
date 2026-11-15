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

Source file: Env_Pipe.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
11-Oct-2001	AP/JD	Initial version
************************************************************************/

/************************************************************************** 
 *
 * Envelope pipe structure type file 
 *
 * Env_Types.h
 *
 * contains all structures of the implementation of the envelope pipe
 *
 **************************************************************************/

#ifndef __Env_Types_h
#define __Env_Types_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct envbunch_tag
{
    Double *pdEnvelope;   /* zeros(sf_size, nr_of_channels) */
}ENVBUNCH ;

typedef struct envpipe_tag
{
    SInt         pipe_len ;
    SInt         nr_of_channels ;
    SInt         sf_size ;
    Double       dMin_Envelope ;
    Double       Envelope_Decay ; 
    Double      *pdPrevEnvValue ; /* last envelope value of previous sf (1:nr_of_channels) */
    ENVBUNCH   **ppEnvBunch ;     /* (1:pipe_len) */
}ENVPIPE, *PENVPIPE;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

PENVPIPE ENVPIPE_CreateInstance ( SInt nChannels, SInt sf_size, Double dMin_Envelope, Double dDecay, SInt pipe_len );
void     ENVPIPE_DestroyInstance( PENVPIPE pEnvPipe );
void     ENVPIPE_Shift          ( PENVPIPE pEnvPipe );
void     ENVPIPE_CalcEnvelope   ( PENVPIPE pEnvPipe, SInt nCh, Double *pSignal );

#ifdef __cplusplus
}
#endif /* __Env_Types_h */

#endif
