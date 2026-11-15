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

Copyright  2001.

Source file: bsc_buffer.h

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
 * Bit Stream Conversion tools
 *
 * bsc_buffer.h
 *
 * contains all structures of the tools
 *
 **************************************************************************/

#ifndef __bsc_buffer_h
#define __bsc_buffer_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define BSC_MAX_BUFFER_LENGTH  (2000*SSC_MAX_CHANNELS)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct _BSC_FRAME_BUFFER
{
    SSCLOG*  pLog;

    UByte	 B[BSC_MAX_BUFFER_LENGTH]; /* byte buffer */
    UInt	 BitNr;  /* bit nr where to write next info  */
    UInt	 ByteNr; /* byte nr where to write next info */
} BSC_FRAME_BUFFER;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void              BSC_BUFFER_Initialise(void);
void              BSC_BUFFER_Free(void);

BSC_FRAME_BUFFER* BSC_BUFFER_CreateInstance(SSCLOG* pLog);
void              BSC_BUFFER_DestroyInstance(BSC_FRAME_BUFFER* pFrameBuffer);

void BSC_BUFFER_WriteBsMsbf(BSC_FRAME_BUFFER* fb,
                            UByte* n,
                            SInt b,
                            SInt* sc,
                            SSC_EXCEPTION* the_exception_context);

void BSC_BUFFER_WriteUiMsbf(BSC_FRAME_BUFFER* fb,
                            UInt n,
                            SInt b,
                            SInt* sc,
                            SSC_EXCEPTION* the_exception_context);

void BSC_BUFFER_WriteSiMsbf(BSC_FRAME_BUFFER* fb,
                            SInt n,
                            SInt b,
                            SInt* sc,
                            SSC_EXCEPTION* the_exception_context);

void BSC_BUFFER_WritePadding(BSC_FRAME_BUFFER* fb,
                             SInt* sc,
                             SSC_EXCEPTION* the_exception_context);

void BSC_BUFFER_Clear(BSC_FRAME_BUFFER* fb);

UInt BSC_BUFFER_GetBitLength(BSC_FRAME_BUFFER* fb);

UInt BSC_BUFFER_WriteToFile(BSC_FRAME_BUFFER* fb, FILE* f, SSC_EXCEPTION* the_exception_context);

#ifdef __cplusplus
}
#endif


#endif
