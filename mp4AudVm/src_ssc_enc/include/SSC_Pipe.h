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

Source file: SSC_Pipe.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
************************************************************************/


/*===========================================================================
 *
 *  Module      : SSC_Pipe
 *
 *  File        : SSC_Pipe.h
 *
 *  Description : This header file contains the prototypes of the pipe module.
 *
 ============================================================================*/

#ifndef _SSC_PIPE_H_
#define _SSC_PIPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SSC_Log.h"

typedef struct sscpipe_tag
{
    SInt       nChannels ;
    SInt       nHistPipeLength ;
    SSCBUNCH **ppHistSignalPipe ;
    SInt       nAheadPipeLength ;
    SSCBUNCH **ppAheadSignalPipe ;
    SSCLOG    *pLog ;

}SSCPIPE;

SSC_INTF_EXPORT SSCPIPE  *SSCPIPE_InitInstance      ( SInt nChannels, SInt nSubframeSize, SInt nDecoder_level,
                                      SInt nHistPipeLength, SInt nAheadPipeLength, SSCLOG *pLog );
SSC_INTF_EXPORT SSCBUNCH *SSCPIPE_GetAt             ( SSCPIPE *pSscPipe,  SInt    nIndex );
SSC_INTF_EXPORT SInt      SSCPIPE_GetHistItemCount  ( SSCPIPE *pSscPipe );
SSC_INTF_EXPORT SInt      SSCPIPE_GetAheadItemCount ( SSCPIPE *pSscPipe );
SSC_INTF_EXPORT SInt      SSCPIPE_GetItemCount      ( SSCPIPE *pSscPipe );

SSC_INTF_EXPORT void      SSCPIPE_Add               ( SSCPIPE *pSscPipe, SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SSCBUNCH *SSCPIPE_Shift             ( SSCPIPE *pSscPipe );
SSC_INTF_EXPORT void      SSCPIPE_DestroyInstance   ( SSCPIPE *pSscPipe );




#ifdef __cplusplus
}
#endif
#endif /* _SSC_PIPE_H_ */

