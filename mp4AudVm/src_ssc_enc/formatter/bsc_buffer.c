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

Source file: bsc_buffer.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : Writing ssc file streams
 *
 *   Description         : Writing ssc file streams
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with BSC_WRES_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Error.h"
#include "SSC_Log.h"
#include "bsc_buffer.h"
#include "cexcept.h"
#include "SSC_Alloc.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define MASK_BITS(x) ( ((x)==32) ? (0xffffffff) : ((((UInt)1)<<(x))-1) )
static const UByte UCharBitMask[] = {1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0};

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_WriteBitToBuffer(UByte b,BSC_FRAME_BUFFER* fb, SSC_EXCEPTION* the_exception_context);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

void BSC_BUFFER_Initialise(void)
{
    return;
}

void BSC_BUFFER_Free(void)
{
    return;
}

BSC_FRAME_BUFFER* BSC_BUFFER_CreateInstance(SSCLOG* pLog)
{
    BSC_FRAME_BUFFER* pBuf = NULL;

    pBuf = SSC_MALLOC(sizeof(BSC_FRAME_BUFFER), "BSC framebuffer Instance");
    if ( pBuf != NULL )
    {
        pBuf->pLog = pLog;
    }
    else
    {
        SSCLOG_TRACE5(pLog, "\nError: unable to allocate memory for frame buffer\n");
    }

    return pBuf;
}

void BSC_BUFFER_DestroyInstance(BSC_FRAME_BUFFER* pFrameBuffer)
{
    if(pFrameBuffer != NULL)
    {
        SSC_FREE(pFrameBuffer, "BSC framebuffer Instance");
    }
}


/* Writes a bitstream to the buffer using of coarse the
 * WriteBitToBuffer function
 *
 * Parameters  : n  : code to be written to the buffer
 *               b  : Number of bits to be used
 *               fb : Frame buffer
 *               sc : Statistics counter */

void BSC_BUFFER_WriteBsMsbf(BSC_FRAME_BUFFER* fb,
                            UByte* n,
                            SInt b,
                            SInt* sc,
                            SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;
    UByte     bitField;

    pLog = fb->pLog;

    if (b < 1 )
    {
        fprintf(stdout, "\nERROR: #bits huffman to be written must be >= 1\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }

    SSCLOG_TRACE0(pLog, " H%d ", b);

    bitField = 0x80;
    while ( b > 0 )
    {
        UByte outputbit;
        if ( (*n & bitField) == 0 )
        {
            outputbit = (UByte)(0);
        }
        else
        {
            outputbit = (UByte)(1);
        }

        LOCAL_WriteBitToBuffer(outputbit, fb, the_exception_context);

        bitField >>=  1 ;
        if ( bitField == 0 )
        {
            bitField = 0x80;
            n++;
        }
        b--;
        *sc += 1;
    }
}

/* Writes an Unsigned Integer to the buffer using WriteBitToBuffer
 * function
 *
 * Parameters  : n  : Number to be written to the buffer
 *               b  : Number of bits to be used
 *               fb : Frame buffer
 *               sc : Statistics counter */
void BSC_BUFFER_WriteUiMsbf(BSC_FRAME_BUFFER* fb,
                            UInt n,
                            SInt b,
                            SInt* sc,
                            SSC_EXCEPTION* the_exception_context)
{
    SInt	  i;
    UInt      bitField;
    SSCLOG* pLog = NULL ;

    pLog = fb->pLog;

    if (b < 1 )
    {
        SSCLOG_TRACE5(pLog, "\nError: #bits to be written must be >= 1\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }

    if (n > MASK_BITS(b) )
    {
        SSCLOG_TRACE5(pLog, "\nError: number does not fit in #bits\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }

    SSCLOG_TRACE0(pLog, " U%d ",b);

    bitField = 1 << (b-1);
    for (i = 0; i < b; i++, bitField = bitField >> 1 )
    {
        UByte outputbit;

        if ( (n & bitField) == 0 )
        {
            outputbit = (UByte)(0);
        }
        else
        {
            outputbit = (UByte)(1);
        }
        LOCAL_WriteBitToBuffer(outputbit, fb,the_exception_context);
    }
    *sc += b;
}


/* Write signed most significant bit first number to buffer.
 *
 * Parameters  : n  : Number to be written to the buffer
 *               b  : Number of bits to be used
 *               fb : Frame buffer
 *               sc : Statistics counter */
void BSC_BUFFER_WriteSiMsbf(BSC_FRAME_BUFFER* fb,
                            SInt n,
                            SInt b,
                            SInt* sc,
                            SSC_EXCEPTION* the_exception_context)
{
    SInt	  i;
    UInt	  un;
    UInt      bitField;
    SSCLOG* pLog = NULL ;

    pLog = fb->pLog;

    if (b < 1)
    {
        SSCLOG_TRACE5(pLog, "\nError: #bits to be written must be >= 1\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }
    if ((n >= (SInt)(1 << (b-1))) || (n < -(SInt)(1 << (b-1))))
    {
        SSCLOG_TRACE5(pLog, "\nError: number does not fit in #bits\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }

    SSCLOG_TRACE0(pLog, " S%d ",b);

    n &= MASK_BITS(b);

    un = (UInt)n;

    bitField = 1 << (b-1);
    for (i = 0; i < b; i++, bitField = bitField >> 1 )
    {
        UByte outputbit;

        if ( (un & bitField) == 0 )
        {
            outputbit = (UByte)(0);
        }
        else
        {
            outputbit = (UByte)(1);
        }
        LOCAL_WriteBitToBuffer(outputbit, fb, the_exception_context);
    }

    *sc += b;
}


void BSC_BUFFER_WritePadding(BSC_FRAME_BUFFER* fb,
                             SInt* sc,
                             SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;
    UInt    value = 0;
    UInt    nrBits = (8 - fb->BitNr) % 8;

    pLog = fb->pLog;
    if ( nrBits != 0 )
    {
        value = 0;
        SSCLOG_TRACE0(pLog, " %d", value);
        BSC_BUFFER_WriteUiMsbf(fb, value, nrBits, sc, the_exception_context);
    }
    else
    {
        SSCLOG_TRACE0(pLog, " -");
    }
}


/* Gets the total number of bits written to the buffer so far */
UInt BSC_BUFFER_GetBitLength(BSC_FRAME_BUFFER* fb)
{
    return ((fb->ByteNr * 8) + fb->BitNr );
}


/* This function really writes the written buffer to the file specified */
UInt BSC_BUFFER_WriteToFile(BSC_FRAME_BUFFER* fb, FILE* f, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;

    pLog = fb->pLog;

    if ( fb->BitNr != 0 )
    {
        SSCLOG_TRACE5(pLog, "\nError: Write file buffer is not alligned to bytes for file writting\n");
        Throw(SSC_ERROR_PADDING);
    }

    if ( (fwrite(fb->B, 1, fb->ByteNr, f)) != fb->ByteNr)
    {
        SSCLOG_TRACE5(pLog, "\nError: Not all bytes of the frame could be written\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }
    if ( fflush(f) != 0 )
    {
        SSCLOG_TRACE5(pLog, "\nError: Flush of dat to file not succeeded\n");
        Throw SSC_ERROR_NOT_AVAILABLE;
    }

	return fb->ByteNr;

}


/* Clear the internal buffer */
void BSC_BUFFER_Clear(BSC_FRAME_BUFFER* fb)
{
    fb->ByteNr = 0;
    fb->BitNr  = 0;
}

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Writes a bit to the buffer. This is the only function which fills the buffer
 *
 * Parameters  : b : value of bit to be written
 *               fb: frame buffer */
static void LOCAL_WriteBitToBuffer(UByte b, BSC_FRAME_BUFFER* fb, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;

    pLog = fb->pLog;

    SSCLOG_TRACE0(pLog, "%d",b);

    if (b != 0)
    {
        fb->B[fb->ByteNr] |= UCharBitMask[fb->BitNr];
    }
    else
    {
        fb->B[fb->ByteNr] &= ~UCharBitMask[fb->BitNr];
    }
    fb->BitNr++;
    if (fb->BitNr >= 8)
    {
        fb->BitNr = 0;
        fb->ByteNr++;
        if (fb->ByteNr > BSC_MAX_BUFFER_LENGTH)
        {
            SSCLOG_TRACE5(pLog, "\nError: Frame does not fit in the buffer\n");
            Throw SSC_ERROR_FRAME_TOO_LONG;
        }
    }
}

