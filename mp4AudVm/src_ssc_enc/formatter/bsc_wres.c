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

Source file: bsc_wres.c

Required libraries: <none>

Authors:
AP:    Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:    Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:    Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
TM:    Tjeu Mans,         Philips CE - PDS-labs Eindhoven <tjeu.mans@philips.com>
AR:    Aad Rijnberg,      Philips CE - PDS-labs Eindhoven <aad.rijnberg@philips.com>
NT:    Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:    Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
05-Dec-2001 JD      m7724 and m7725 implemented
16-Aug-2002 AR/TM   Changes for m8540 core experiment
20-Sep-2002 NT      Changes for RM2
21-Nov-2003 AG      Changes for RM4
05-Jan-2004 AG      Changes for RM5
************************************************************************/

/******************************************************************************
 *
 *   Module              : Bit Stream Tools
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Error.h"
#include "SSC_Log.h"
#include "SSC_BitStrm_Types.h"
#include "bsc_buffer.h"

#include "bsc_wres.h"
#include "SSC_BitStream.h"
#include "SSC_Alloc.h"

#include "HuffmanDecoder.h"
#include "BSC_huftables.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/* 2 : syntax description 0.1 */
/* 3 : syntax description RM0 Release 0.2 */
/* 4 : syntax description RM1.5 */
/* 5 : syntax description RM2 */
/* 6 : syntax description RM3 */
/* 7 : syntax description RM3a */
/* 8 : syntax description RM4 */
#define BSC_FILE_VERSION          8

/* 3 : formatter capable of writing syntax description RM0 Release 0.2 */
/* 4 : formatter capable of writing syntax description RM1.5 */
/* 5 : formatter capable of writing syntax description RM2 */
/* 6 : formatter capable of writing syntax description RM3 */
/* 7 : formatter capable of writing syntax description RM3a */
/* 8 : formatter capable of writing syntax description RM4 */
#define BSC_FORMATTER_VERSION     8

#define SSC_UPDATE_OCS_QMF        2

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/* frequency resolution */
static const UByte OCS_fullFreqRes[OCS_NO_FREQ_RESOLUTIONS] =
{
    10, 20, OCS_MAX_FREQ_RESOLUTION
};

static const UByte OCS_halfFreqRes[OCS_NO_FREQ_RESOLUTIONS] =
{
    10 / 2, 11, OCS_MAX_FREQ_RESOLUTION / 2
};

/* quantisation tables */
static const UByte OCS_fixEnvelopes[OCS_NO_ENVELOPE_INDICES] =
{
    0, 1, 2, 4
};

static const UByte OCS_varEnvelopes[OCS_NO_ENVELOPE_INDICES] =
{
    1, 2, 3, 4
};

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_WriteFrameHeader(BSC_WRES*        pWres,
                                   BSC_FRAME*       fp,
                                   SSC_EXCEPTION*   the_exception_context);
static void LOCAL_WriteFrameData(BSC_WRES*          pWres,
                                 BSC_FRAME*         fp,
                                 SSC_EXCEPTION*     the_exception_context);
static void LOCAL_WriteFrameDataTransient(BSC_WRES*               pWres,
                                          SSC_TRANSIENT_QPARAM*   pTQ,
                                          UInt                    sf,
                                          SSC_EXCEPTION*          the_exception_context);
static void LOCAL_WriteFrameDataSinus(BSC_WRES*            pWres,
                                      BSC_FRAME*           fp,
                                      SSC_SINUSOID_QPARAM* pSQ,
                                      SSC_SINUSOID_QPARAM* pSQPrev,
                                      SSC_SINUSOID_QPARAM* pSQNext,
                                      SSC_SINUSOID_QPARAM* pSQNext2,
                                      UInt                 sf,
                                      SSC_EXCEPTION*       the_exception_context);
static void LOCAL_WriteFrameDataNoise(BSC_WRES*         pWres,
                                      BSC_FRAME*        fp,
                                      SSC_NOISE_QPARAM* pNQ,
                                      SSC_NOISE_QPARAM* pNQPrev2,
                                      SSC_NOISE_QPARAM* pNQPrev4,
                                      UInt              sf,
                                      SSC_EXCEPTION*    the_exception_context);
static void LOCAL_WriteFrameStereoHeader(BSC_WRES*          pWres,
                                         SSC_STEREO_QPARAM* pSTQ,
                                         UInt               sf,
                                         SSC_EXCEPTION*     the_exception_context);
static void LOCAL_WriteFrameStereoEnvelopes(BSC_WRES*          pWres,
                                            SSC_STEREO_QPARAM* pSTQ,
                                            UInt               sf,
                                            SSC_EXCEPTION*     the_exception_context);
static void LOCAL_WriteFrameStereoData(BSC_WRES*          pWres,
                                       SSC_STEREO_QPARAM* pSTQ,
                                       SSC_STEREO_QPARAM* pSTQPrev,
                                       UInt               sf,
                                       SSC_EXCEPTION*     the_exception_context);
static void LOCAL_WriteFrameStereoExtension(BSC_WRES*          pWres,
                                            SSC_STEREO_QPARAM* pSTQ,
                                            SSC_STEREO_QPARAM* pSTQPrev,
                                            UInt               sf,
                                            SSC_EXCEPTION*     the_exception_context);
static void LOCAL_WriteFrameDataStereo(BSC_WRES*          pWres,
                                       BSC_FRAME*         fp,
                                       SSC_STEREO_QPARAM* pSTQ,
                                       SSC_STEREO_QPARAM* pSTQPrev,
                                       UInt               sf,
                                       SSC_EXCEPTION*     the_exception_context);
static void LOCAL_WriteFramePadding(BSC_WRES*      pWres,
                                    SSC_EXCEPTION* the_exception_context);

static void LOCAL_WriteStatistics(BSC_WRES* pWres,
                                  SSC_EXCEPTION* the_exception_context);

static void LOCAL_SearchHuffmanCodeIndex( BSC_WRES* pWres, SInt s, HUFFMAN_CODE_STRUCT* hcs, UInt* idx, SSC_EXCEPTION* the_exception_context);
static void LOCAL_SearchHuffmanCodeIndexCoarse( BSC_WRES* pWres, SInt s, HUFFMAN_CODE_STRUCT* hcs, UInt* idx, SSC_EXCEPTION* the_exception_context);

static void  LOCAL_WriteFineValue(BSC_WRES* pWres, SInt value, UInt qMode, SInt* pStat, BSC_FRAME_BUFFER* pBuf, SSC_EXCEPTION* the_exception_context);

static HUFFMAN_CODE_STRUCT* LOCAL_GetHuffmanTable( BSC_WRES* pWres, const char* s, UInt qMode, SSC_EXCEPTION* the_exception_context);
static SSC_SIN_QSEGMENT*    LOCAL_WRES_GetContinuingSinusSubframe(SSC_SINUSOID_QPARAM* pQS, UInt idx);

static UInt LOCAL_WRES_GetIndexContinuingSinusSubframe(SSC_SINUSOID_QPARAM* pQS, UInt idx);
static void LOCAL_WriteBitsPerSubframe( SSCLOG* pLog, SInt bits[SSC_MAX_SUBFRAMES] );

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

void BSC_WRES_Initialise(void)
{
    BSC_BUFFER_Initialise();
    return;
}


void BSC_WRES_Free(void)
{
    BSC_BUFFER_Free();
    return;
}


BSC_WRES* BSC_WRES_CreateInstance(const char* fileName, SSCLOG* pLog, BSC_FILE_INFO* pInfo)
{
    BSC_WRES* pWres = NULL;

    assert(fileName);

    pWres = SSC_MALLOC(sizeof(BSC_WRES), "BSC Write Elementary Stream Instance");
    if ( pWres != NULL )
    {
        /* create members of this struct*/
        pWres->pLog    = pLog;
        pWres->pStream = fopen(fileName, "wb");
        pWres->pBuf    = BSC_BUFFER_CreateInstance(pLog);

        memcpy(&pWres->fileInfo, pInfo, sizeof(BSC_FILE_INFO));

        pWres->TotalBytesFileHeader = 0;
        pWres->TotalBytesFileAudio  = 0;
        pWres->TotalFramesWritten   = 0;

        /* check if all is ok; if not cleanup */
        if ( (pWres->pStream == NULL) || (pWres->pBuf == NULL) )
        {
            SSCLOG_TRACE5(pLog,"\nError: Unable to create WRES members\n");

            BSC_WRES_DestroyInstance(pWres);
            pWres = NULL;
        }
    }
    else
    {
        SSCLOG_TRACE5(pLog,"\nError: Unable to alocate memory for WRES\n");
    }

    return pWres;
}


void BSC_WRES_DestroyInstance(BSC_WRES* pWres)
{
    if(pWres != NULL)
    {
        if (pWres->pStream != NULL)
        {
            fclose(pWres->pStream);
        }
        if  ( pWres->pBuf != NULL)
        {
            BSC_BUFFER_DestroyInstance(pWres->pBuf);
        }

        SSC_FREE(pWres, "BSC Write Elementary Stream Instance");
    }
}


/* This function writes the SSC file header/ version / configuration /
 * part of sound chunk  */
#ifdef SSC_MP4REF_BUILD
SSC_ERROR BSC_WRES_WriteFileHeader(BSC_WRES* pWres, unsigned char ** ppHeaderData, int * pHeaderBytes)
#else
SSC_ERROR BSC_WRES_WriteFileHeader(BSC_WRES* pWres)
#endif
{

    SSC_EXCEPTION   tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION*  the_exception_context = &tec;

    SSC_ERROR Error  = SSC_ERROR_OK;
    SSC_ERROR rvalue = SSC_ERROR_OK;

    SInt count = 0;
    UInt value = 0;

    BSC_FRAME_BUFFER* pBuf  = pWres->pBuf;
    SSCLOG*           pLog  = NULL ;
#ifndef SSC_MP4REF_BUILD
    FILE*             pFile = pWres->pStream;
#endif
    BSC_FILE_INFO*    pInfo = &pWres->fileInfo;

    pLog = pWres->pLog;
    Try
    {
        BSC_BUFFER_Clear(pBuf);

        SSCLOG_TRACE1(pLog, "***************** Fileheader ***************\n");

        value = (UInt)(SSC_formChunk[0] << 24);
        value += (UInt)(SSC_formChunk[1] << 16);
        value += (UInt)(SSC_formChunk[2] <<  8);
        value += (UInt)(SSC_formChunk[3]);
        SSCLOG_TRACE1(pLog, "chunk_id         : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_ID, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = 4;
        SSCLOG_TRACE1(pLog, "chunk_len        : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN  , &count, the_exception_context); /* overwritten with real value at the end */
        SSCLOG_TRACE1(pLog, "\n");

        value  = (UInt)(SSC_typeChunk[0] << 24);
        value += (UInt)(SSC_typeChunk[1] << 16);
        value += (UInt)(SSC_typeChunk[2] <<  8);
        value += (UInt)(SSC_typeChunk[3]);
        SSCLOG_TRACE1(pLog, "form_type        : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_FORM_TYPE, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value  = (UInt)(SSC_verChunk[0] << 24);
        value += (UInt)(SSC_verChunk[1] << 16);
        value += (UInt)(SSC_verChunk[2] <<  8);
        value += (UInt)(SSC_verChunk[3]);
        SSCLOG_TRACE1(pLog, "chunk_id         : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_ID, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = 4;
        SSCLOG_TRACE1(pLog, "chunk_len        : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = BSC_FILE_VERSION;
        SSCLOG_TRACE1(pLog, "file_version     : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_FILE_VERSION, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->EncoderVersion;
        SSCLOG_TRACE1(pLog, "encoder_version  : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_ENCODER_VERSION, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = BSC_FORMATTER_VERSION;
        SSCLOG_TRACE1(pLog, "formatter_version: %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_FORMATTER_VERSION, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = 0;
        SSCLOG_TRACE1(pLog, "reserved         : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_RESERVED_VERSION, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value  = (UInt)(SSC_confChunk[0] << 24);
        value += (UInt)(SSC_confChunk[1] << 16);
        value += (UInt)(SSC_confChunk[2] <<  8);
        value += (UInt)(SSC_confChunk[3]);
        SSCLOG_TRACE1(pLog, "chunk_id         : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_ID, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = 3;
        SSCLOG_TRACE1(pLog, "chunk_len        : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->DecoderLevel;
        SSCLOG_TRACE1(pLog, "decoder_level    : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_DECODER_LEVEL, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->SampleRateIndex;
        SSCLOG_TRACE1(pLog, "sampling_rate    : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_SAMPLING_RATE, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->UpdateRate;
        SSCLOG_TRACE1(pLog, "update_rate      : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_UPDATE_RATE, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->SynthesisMethod;
        SSCLOG_TRACE1(pLog, "synthesis_method : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_SYNTHESIS_METHOD, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->Mode;
        SSCLOG_TRACE1(pLog, "mode             : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_MODE, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = pInfo->ModeExt;
        switch (pInfo->Mode)
        {
            case SSC_MODE_MONO:
                /* in mono no mode_ext defined */
                break;
            case SSC_MODE_STEREO:
                /* in stereo no mode_ext defined */
                break;
            case SSC_MODE_LCSTEREO:
                SSCLOG_TRACE1(pLog, "mode_ext         : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_MODE_EXT_STEREO, &count, the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");
                /* if par_stereo also write reserved bits */
                if ( value == SSC_MODE_EXT_PAR_STEREO )
                {
                    value = 0;
                    SSCLOG_TRACE1(pLog, "ps_reserved: %d", value);
                    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_PS_RESERVED, &count, the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
                break;
            default:
                break;
        }

        value = 0;
        SSCLOG_TRACE1(pLog, "reserved         : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_RESERVED, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        SSCLOG_TRACE1(pLog, "padding_bit      :");
        BSC_BUFFER_WritePadding( pBuf, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        /* safety check: control if 'count' is byte alligned*/
        if  ((count % 8) != 0)
        {
            Throw(SSC_ERROR_PADDING);
        }
        pWres->TotalBytesFileHeader  = count/8;
        pWres->frameStatistics.TotalBytes = 0;

        value = (UInt)(SSC_soundChunk[0] << 24);
        value += (UInt)(SSC_soundChunk[1] << 16);
        value += (UInt)(SSC_soundChunk[2] <<  8);
        value += (UInt)(SSC_soundChunk[3]);
        SSCLOG_TRACE1(pLog, "chunk_id         : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_ID, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = 4;
        SSCLOG_TRACE1(pLog, "chunk_len        : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context); /* overwritten with real value at the end */
        SSCLOG_TRACE1(pLog, "\n");

        value = 0;
        SSCLOG_TRACE1(pLog, "nrof_frames      : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_NROF_FRAMES, &count, the_exception_context); /* overwritten with real value at the end */
        SSCLOG_TRACE1(pLog, "\n");

        /* write complete buffer to file*/
#ifdef SSC_MP4REF_BUILD
        /* RJ: do not write it to file here, but pass the data to the higher level instead */
        *ppHeaderData = pBuf->B;
        *pHeaderBytes = pBuf->ByteNr;

#else
        BSC_BUFFER_WriteToFile(pBuf, pFile, the_exception_context);
#endif

        /* end of writing fixed part of the header from now on only
           audio frames wil be written*/
    }
    Catch(Error)
    {
        rvalue = Error;
    }


    return rvalue;
}


/* Global function which writes a frame( header/data/padding) to the
 * buffer */

#ifdef SSC_MP4REF_BUILD
 SSC_ERROR BSC_WRES_WriteFileFrame(BSC_WRES* pWres, BSC_FRAME* fp,  UInt bitsFrame[SSC_MAX_SUBFRAMES][6], unsigned char ** ppFrameData, int * pFrameBytes)
#else
 SSC_ERROR BSC_WRES_WriteFileFrame(BSC_WRES* pWres, BSC_FRAME* fp,  UInt bitsFrame[SSC_MAX_SUBFRAMES][6])
#endif
{
    SSC_ERROR Error  = SSC_ERROR_OK;
    SSC_ERROR rvalue = SSC_ERROR_OK;

    SSC_EXCEPTION   tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION*  the_exception_context = &tec;

    BSC_FRAME_BUFFER* pBuf  = pWres->pBuf;
    SSCLOG*           pLog  = NULL;
#ifndef SSC_MP4REF_BUILD
    FILE*             pFile = pWres->pStream;
#endif

    UInt sf = 0;
    UInt TotalBits = 0;

    pLog  = pWres->pLog;
    Try
    {
        /* initialisation of frame stuff*/
        BSC_BUFFER_Clear(pWres->pBuf);
        memset(&pWres->frameStatistics, 0x00, sizeof(pWres->frameStatistics) );

        SSCLOG_TRACE1(pLog, "\n***************** Frameheader %4d ***************\n", pWres->TotalFramesWritten);
        LOCAL_WriteFrameHeader(pWres, fp, the_exception_context);

        SSCLOG_TRACE1(pLog, "\n****************** FrameData  %4d ***************\n", pWres->TotalFramesWritten);
        LOCAL_WriteFrameData(pWres, fp, the_exception_context);

        SSCLOG_TRACE1(pLog, "\n****************** FramePadding %4d *************\n", pWres->TotalFramesWritten);
        LOCAL_WriteFramePadding(pWres, the_exception_context);

#ifdef SSC_MP4REF_BUILD
        /* RJ: don't write to file here, but leave it up to the application */
        *ppFrameData = pBuf->B;
        *pFrameBytes = pBuf->ByteNr;
        pWres->frameStatistics.TotalBytes = pBuf->ByteNr;
#else
        /* write complete buffer to file*/
        pWres->frameStatistics.TotalBytes = BSC_BUFFER_WriteToFile(pBuf, pFile, the_exception_context);
#endif

        /* update first + last sf with header + padding bits */
        pWres->frameStatistics.BitsSubframe[                    0] += pWres->frameStatistics.HeaderBits[sf] + pWres->frameStatistics.JitterBits[sf];
        pWres->frameStatistics.BitsSubframe[SSC_MAX_SUBFRAMES - 1] += pWres->frameStatistics.PaddingBits[SSC_MAX_SUBFRAMES - 1];

        SSCLOG_TRACE3(pLog, "\n****************** Statistics  %4d **************\n", pWres->TotalFramesWritten);
        LOCAL_WriteStatistics(pWres, the_exception_context);

        pWres->TotalFramesWritten  += 1;
        pWres->TotalBytesFileAudio += pWres->frameStatistics.TotalBytes;

        /* save the found bits/sf to the callee*/
        TotalBits = 0;
        for ( sf = 0; sf < SSC_MAX_SUBFRAMES; sf++)
        {
            /* Header bits */
            bitsFrame[sf][0] =
                (pWres->frameStatistics.HeaderBits[sf] +
                 pWres->frameStatistics.JitterBits[sf]);

            /* Transient bits */
            bitsFrame[sf][1] =
                (pWres->frameStatistics.TraPresentBits[sf] +
                 pWres->frameStatistics.TraLocBits    [sf] +
                 pWres->frameStatistics.TraTypeBits   [sf] +
                 pWres->frameStatistics.TraParBits    [sf] +
                 pWres->frameStatistics.TraChiBits    [sf] +
                 pWres->frameStatistics.TraNrOfSinBits[sf] +
                 pWres->frameStatistics.TraSinFreqBits[sf] +
                 pWres->frameStatistics.TraSinAmpBits [sf] +
                 pWres->frameStatistics.TraSinPhiBits [sf]);

            /* Sinusoids bits */
            bitsFrame[sf][2] =
                (pWres->frameStatistics.SinConSContBits      [sf] +
                 pWres->frameStatistics.SinConFreqBits       [sf] +
                 pWres->frameStatistics.SinConAmpBits        [sf] +
                 pWres->frameStatistics.SinConPhiBits        [sf] +
                 pWres->frameStatistics.SinConDeltaFreqBits  [sf] +
                 pWres->frameStatistics.SinConDeltaAmpBits   [sf] +
                 pWres->frameStatistics.SinConEnhPhiBits     [sf] +
                 pWres->frameStatistics.SinNrOfBirthBits     [sf] +
                 pWres->frameStatistics.SinBirthSContBits    [sf] +
                 pWres->frameStatistics.SinBirthFreqBits     [sf] +
                 pWres->frameStatistics.SinBirthAmpBits      [sf] +
                 pWres->frameStatistics.SinBirthPhiBits      [sf] +
                 pWres->frameStatistics.SinBirthDeltaFreqBits[sf] +
                 pWres->frameStatistics.SinBirthDeltaAmpBits [sf]);

            /* Noise bits */
            bitsFrame[sf][3] =
                (pWres->frameStatistics.NoiDenLARBits        [sf] +
                 pWres->frameStatistics.NoiLsfBits           [sf] +
                 pWres->frameStatistics.NoiOverlapLsfBits    [sf] +
                 pWres->frameStatistics.NoiPoleLaguerreBits  [sf] +
                 pWres->frameStatistics.NoiGridLaguerreBits  [sf] + 
                 pWres->frameStatistics.NoiEnvGainBits       [sf] +
                 pWres->frameStatistics.NoiDeltaDenLARBits   [sf] +
                 pWres->frameStatistics.NoiDeltaLsfBits      [sf] +
                 pWres->frameStatistics.NoiDeltaEnvGainBits  [sf]);

            /* Stereo Bits */
            bitsFrame[sf][4] =
                (pWres->frameStatistics.StereoHeader         [sf] +
                 pWres->frameStatistics.StereoEnvelopes      [sf] +
                 pWres->frameStatistics.StereoData           [sf] +
                 pWres->frameStatistics.StereoExtension      [sf]);

            /* Padding bits */
            bitsFrame[sf][5] = pWres->frameStatistics.PaddingBits[sf];

            TotalBits += (bitsFrame[sf][0] +
                          bitsFrame[sf][1] +
                          bitsFrame[sf][2] +
                          bitsFrame[sf][3] +
                          bitsFrame[sf][4] +
                          bitsFrame[sf][5]);
        }
        /* safety check*/
        if ( TotalBits != (pWres->frameStatistics.TotalBytes * 8) )
        {
            SSCLOG_TRACE5(pLog, "\nError: total bits per subframe added not the same as totalBytes\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        rvalue = Error;
    }

    return Error;
}


/* This function writes the correct FORM + SCHK chunks lenghts in the
 * SSC file + the correct number of frames written in the SCHK chunk */
#ifdef SSC_MP4REF_BUILD
SSC_ERROR BSC_WRES_GetFileChunkLengths(BSC_WRES* pWres,
                                       UByte* pFileLengthValue,
                                       SInt*  pFileLengthPos,
                                       SInt*  pFileLengthBytes,
                                       UByte* pAudioLengthValue,
                                       SInt*  pAudioLengthPos,
                                       SInt*  pAudioLengthBytes,
                                       UByte* pNrOfFramesValue,
                                       SInt*  pNrOfFramesPos,
                                       SInt*  pNrOfFramesBytes)
{
    SSC_ERROR Error  = SSC_ERROR_OK;
    SSC_ERROR rvalue = SSC_ERROR_OK;
    UInt value = 0;
    SInt count = 0;

    SSC_EXCEPTION   tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION*  the_exception_context = &tec;

    SSCLOG*  pLog          = NULL ;

    BSC_FRAME_BUFFER* pBuf = pWres->pBuf;
    pLog = pWres->pLog;

    Try
    {
        SSCLOG_TRACE1(pLog, "\n*************** Adjust FileHeader ****************\n");

        /* ******************************************************************* */
        BSC_BUFFER_Clear(pBuf);
        
        value = (pWres->TotalBytesFileAudio + 12 + pWres->TotalBytesFileHeader - 8);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);

        *pFileLengthPos = 4;
        *pFileLengthBytes = (int)(SSC_BITS_CHUNK_LEN/8);
        memcpy(pFileLengthValue,pBuf->B, *pFileLengthBytes);
        SSCLOG_TRACE1(pLog, "SSC FORM chunk_len           : 0x%0.4X", *pFileLengthValue);
        SSCLOG_TRACE1(pLog, "\n");

        /* ******************************************************************* */
        BSC_BUFFER_Clear(pBuf);
        
        value = (pWres->TotalBytesFileAudio + 4);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);

        *pAudioLengthPos   = pWres->TotalBytesFileHeader + 4;
        *pAudioLengthBytes = (int)(SSC_BITS_CHUNK_LEN/8);
        memcpy(pAudioLengthValue,pBuf->B, *pAudioLengthBytes);
        SSCLOG_TRACE1(pLog, "SCHK chunk_len               : 0x%0.4X", *pAudioLengthValue);
        SSCLOG_TRACE1(pLog, "\n");

        /* ******************************************************************* */
        BSC_BUFFER_Clear(pBuf);
        
        value = pWres->TotalFramesWritten;
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);

        *pNrOfFramesPos   = (*pAudioLengthPos) + (*pAudioLengthBytes);
        *pNrOfFramesBytes = (int)(SSC_BITS_NROF_FRAMES/8);
        memcpy(pNrOfFramesValue,pBuf->B, *pNrOfFramesBytes);
        SSCLOG_TRACE1(pLog, "Total Frames Written in file : %d", *pNrOfFramesValue);
        SSCLOG_TRACE1(pLog, "\n");

        /* ******************************************************************* */
        SSCLOG_TRACE3(pLog, "\n****************** Summary ***********************\n");
        SSCLOG_TRACE3(pLog, "\nNumber Frames : %d", pWres->TotalFramesWritten);
        SSCLOG_TRACE3(pLog, "\nBitrate       : %f bits/second\n", ((float)pWres->TotalBytesFileAudio*8*SSC_sample_freq[pWres->fileInfo.SampleRateIndex])/((float)(pWres->TotalFramesWritten*SSC_MAX_SUBFRAMES*pWres->fileInfo.SubFrameSize)) );
    }
    Catch (Error)
    {
        rvalue = Error;
    }

    return rvalue;
}
#else 
SSC_ERROR BSC_WRES_WriteFileChunkLengths(BSC_WRES* pWres)
{
    SSC_ERROR Error  = SSC_ERROR_OK;
    SSC_ERROR rvalue = SSC_ERROR_OK;
    UInt value = 0;
    SInt count = 0;

    SSC_EXCEPTION   tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION*  the_exception_context = &tec;

    FILE*    pSSCFile      = pWres->pStream;
    SSCLOG*  pLog          = NULL ;
    BSC_FRAME_BUFFER* pBuf = pWres->pBuf;

    pLog = pWres->pLog;
    Try
    {
        SSCLOG_TRACE1(pLog, "\n*************** Adjust FileHeader ****************\n");

        /* ******************************************************************* */
        BSC_BUFFER_Clear(pBuf);
        /* seek to correct position in file */
        if ( fseek(pSSCFile, 4L, SEEK_SET) != 0 )
        {
            SSCLOG_TRACE5(pLog, "\nError: Unable to seek to form chunk len\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        };

        value = pWres->TotalBytesFileAudio + 12 + pWres->TotalBytesFileHeader - 8;
        SSCLOG_TRACE1(pLog, "SSC FORM chunk_len           : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");
        BSC_BUFFER_WriteToFile(pBuf, pSSCFile, the_exception_context);

        /* ******************************************************************* */
        BSC_BUFFER_Clear(pBuf);
        /* seek to the correct position in file*/
        if ( fseek(pSSCFile, pWres->TotalBytesFileHeader + 4 , SEEK_SET) != 0 )
        {
            SSCLOG_TRACE5(pLog, "\nError: Unable to seek to audio data chunk len\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        value = pWres->TotalBytesFileAudio + 4;
        SSCLOG_TRACE1(pLog, "SCHK chunk_len               : 0x%0.4X", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_CHUNK_LEN, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");
        BSC_BUFFER_WriteToFile(pBuf, pSSCFile, the_exception_context);

        /* ******************************************************************* */
        BSC_BUFFER_Clear(pBuf);
        /* no seek set because nrofframes comes directly after len of audio chunk*/
        value = pWres->TotalFramesWritten;
        SSCLOG_TRACE1(pLog, "Total Frames Written in file : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_NROF_FRAMES, &count, the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");
        BSC_BUFFER_WriteToFile(pBuf, pSSCFile, the_exception_context);

        /* ******************************************************************* */
        SSCLOG_TRACE3(pLog, "\n****************** Summary ***********************\n");
        SSCLOG_TRACE3(pLog, "\nNumber Frames : %d", pWres->TotalFramesWritten);
        SSCLOG_TRACE3(pLog, "\nBitrate       : %f bits/second\n", ((float)pWres->TotalBytesFileAudio*8*SSC_sample_freq[pWres->fileInfo.SampleRateIndex])/((float)(pWres->TotalFramesWritten*SSC_MAX_SUBFRAMES*pWres->fileInfo.SubFrameSize)) );
    }
    Catch( Error )
    {
        rvalue = Error;
    }

    return rvalue;
}
#endif

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* This function gets the idx 'th sinus, in struct pQS, of which
 * s_cont is NOT equal to zero
 *
 * Parameters  :
 *    pQS : valid structure to look for a sinus parameter struct
 *    idx : the index to look for [1 .. (pQS->s_nrof_continuations + pQS->s_nrof_births)
 *
 * Returns:    : pointer to the asked sinus parameter structure  */
static SSC_SIN_QSEGMENT* LOCAL_WRES_GetContinuingSinusSubframe( SSC_SINUSOID_QPARAM* pQS, UInt idx)
{
    SSC_SIN_QSEGMENT* prvalue = NULL;

    UInt n;
    UInt m;
    UInt nSinus;
    Bool stop;

    m = 0;
    stop = False;
    nSinus = pQS->s_nrof_continuations + pQS->s_nrof_births;
    for ( n = 0; (n < nSinus) && (stop == False); n++ )
    {
        if ( pQS->SineParam[n].s_cont > 0 )
        {
            m++;
        }
        if ( m == idx )
        {
            stop = True;
            prvalue = &pQS->SineParam[n];
        }
    }

    return prvalue;
}

static UInt LOCAL_WRES_GetIndexContinuingSinusSubframe( SSC_SINUSOID_QPARAM* pQS, UInt idx)
{
    UInt n;
    UInt m;
    UInt nSinus;
 
    m = 0;
    nSinus = pQS->s_nrof_continuations + pQS->s_nrof_births;

    if ((idx > nSinus) || (idx < 1) || (pQS->SineParam[idx-1].s_cont == 0))
    {
        return m;
    }
    else
    {
        for ( n = 0; n < idx; n++ )
        {
            if ( pQS->SineParam[n].s_cont > 0 )
            {
                m++;
            }
        }
        return m;
    }

}

static HUFFMAN_CODE_STRUCT* LOCAL_GetHuffmanTable( BSC_WRES* pWres, const char* s, UInt qMode, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;
    HUFFMAN_CODE_STRUCT* prvalue = NULL;

    pLog  = pWres->pLog;

    if ( strcmp(s,"AmpContRel"   ) == 0 )
    {
        switch ( qMode )
        {
            case 0:
                prvalue = &hufAmpContRel_0;
                break;
            case 1:
                prvalue = &hufAmpContRel_1;
                break;
            case 2:
                prvalue = &hufAmpContRel_2;
                break;
            case 3:
                prvalue = &hufAmpContRel_3;
                break;
            default:
                SSCLOG_TRACE5(pLog, "\nError: Huffman table: %s; Qmode: %d not found\n", s, qMode);
                Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
                break;
        }
    }
    else if ( strcmp(s,"FreqContRel"  ) == 0 )
    {
        switch ( qMode )
        {
            case 0:
                prvalue = &hufFreqContRel_0;
                break;
            case 1:
                prvalue = &hufFreqContRel_1;
                break;
            case 2:
                prvalue = &hufFreqContRel_2;
                break;
            case 3:
                prvalue = &hufFreqContRel_3;
                break;
            default:
                SSCLOG_TRACE5(pLog, "\nError: Huffman table: %s; Qmode: %d not found\n", s, qMode);
                Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
                break;
        }
    }
    else
    {
        SSCLOG_TRACE5(pLog, "\nError: Huffman table: %s not found\n", s);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }

    return prvalue;
}

static void LOCAL_SearchHuffmanCodeIndex( BSC_WRES* pWres, SInt s, HUFFMAN_CODE_STRUCT* hcs, UInt* idx, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL;

    SYMBOL_TYPE beginValue;
    SYMBOL_TYPE endValue;
    SYMBOL_TYPE stepValue;

    pLog       = pWres->pLog;
    beginValue = hcs->table[0].Value;
    endValue   = hcs->table[hcs->length - 1].Value;
    stepValue  = (endValue - beginValue)/(hcs->length-1);

    if (s < beginValue)
    {
        SSCLOG_TRACE5(pLog, "\nError: value: %d smaller than Huffman table start:%s\n", s, hcs->name);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }
    else if ( s > endValue )
    {
        SSCLOG_TRACE5(pLog, "\nError: value: %d greater than Huffman table end: %s\n", s, hcs->name);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }
    else if ( ((s - beginValue) % stepValue)!= 0 )     /* check if the value can be found in the table */
    {
       SSCLOG_TRACE5(pLog, "\nError: step: %d Huffman table: %s not constant\n", stepValue, hcs->name);
       Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }
    else if ( hcs->table[(s - beginValue)/stepValue].Value != s )
    {
        SSCLOG_TRACE5(pLog, "\nError: value: %d not found in Huffman table: %s\n", s, hcs->name);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }
    else
    {
        *idx = (s - beginValue)/stepValue;
    }
}
static void LOCAL_SearchHuffmanCodeIndexCoarse( BSC_WRES* pWres, SInt s, HUFFMAN_CODE_STRUCT* hcs, UInt* idx, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;
    
    SYMBOL_TYPE beginValue;
    SYMBOL_TYPE endValue;
    SYMBOL_TYPE stepValue;
  
    pLog       = pWres->pLog;
    beginValue = hcs->table[0].Value;
    endValue   = hcs->table[hcs->length - 1].Value;
    stepValue  = (endValue - beginValue)/(hcs->length-1);

    if      ( s  < (beginValue - (stepValue/2)) )
    {
        SSCLOG_TRACE5(pLog, "\nError: value: %d smaller than Huffman table start:%s\n", s, hcs->name);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }
    else if ( s >= (endValue + (stepValue/2)) )
    {
        SSCLOG_TRACE5(pLog, "\nError: value: %d greater than Huffman table end: %s\n", s, hcs->name);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }
    else
    {
        *idx = (s - beginValue + (stepValue/2))/stepValue;
    }
    
}

static void  LOCAL_WriteFineValue(BSC_WRES* pWres, SInt value, UInt qMode, SInt* pStat, BSC_FRAME_BUFFER* pBuf, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;
    
    pLog  = pWres->pLog;

    if ( (value % (1 << qMode)) != 0 )
    {
        SSCLOG_TRACE5(pLog, "\nError: value: %d not fitting on Qgrid: %d\n", value, qMode);
        Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
    }

    value = value / (1 << qMode);
    
    switch ( qMode )
    {
        case 0:
            BSC_BUFFER_WriteSiMsbf( pBuf, value, 3, pStat, the_exception_context);
            break;
        case 1:
            BSC_BUFFER_WriteSiMsbf( pBuf, value, 2, pStat, the_exception_context);
            break;
        case 2:
            BSC_BUFFER_WriteSiMsbf( pBuf, value, 1, pStat, the_exception_context);
            break;
        case 3:
            break;
        default:
            SSCLOG_TRACE5(pLog, "\nError: Illegal Huffman table Qmode: %d\n", qMode);
            Throw SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
            break;
    }
    
}


/*************************LOCAL FUNCTION**************************************
 * 
 * Name        : LOCAL_WriteFineValueNoise
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void  LOCAL_WriteFineValueNoise(BSC_WRES* pWres, SInt value, SInt* pStat, BSC_FRAME_BUFFER* pBuf, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG* pLog = NULL ;
    
    pLog  = pWres->pLog;

    BSC_BUFFER_WriteSiMsbf( pBuf, value, 2, pStat, the_exception_context);
}



static void LOCAL_WriteFrameHeader(BSC_WRES* pWres, BSC_FRAME* fp, SSC_EXCEPTION* the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;
    BSC_FILE_INFO*        pInfo = &pWres->fileInfo;

    UInt value = 0;
    UInt ch = 0;
    pLog  = pWres->pLog;

    value = fp->frameHeader.RefreshSinus;
    SSCLOG_TRACE1(pLog, "refresh_sinusoids       : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_REFRESH_SINUSOIDS, &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    value = fp->frameHeader.RefreshSinusNextFrame;
    SSCLOG_TRACE1(pLog, "refresh_sinusoids_next_frame : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_REFRESH_SINUSOIDS, &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    value = fp->frameHeader.RefreshNoise;
    SSCLOG_TRACE1(pLog, "refresh_noise           : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_REFRESH_NOISE    , &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");
    
    for (ch = 0; ch < pWres->fileInfo.Channels; ch++)
    {
        value = fp->frameData[ch].S[BSC_HISTORY].s_nrof_continuations;
        SSCLOG_TRACE1(pLog, "s_nrof_continuations[%d] : %d", ch, value);
        BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_DecoderLevel[pInfo->DecoderLevel].NrOfBits, &pStat->HeaderBits[0], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");
    }

    value = fp->frameHeader.NumberDen;
    SSCLOG_TRACE1(pLog, "n_nrof_den              : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_NROF_DEN, &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    value = pInfo->EnvNoiseOrder;
    SSCLOG_TRACE1(pLog, "n_nrof_lsf              : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_NROF_LSF, &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    value = fp->frameHeader.freqGranularity;
    SSCLOG_TRACE1(pLog, "freqGranularity         : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_FREQ_GRANULARITY, &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    value = fp->frameHeader.ampGranularity;
    SSCLOG_TRACE1(pLog, "ampGranularity          : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_AMP_GRANULARITY, &pStat->HeaderBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    value = fp->frameHeader.PhaseJitterPresent;
    SSCLOG_TRACE1(pLog, "phase_jitter_present    : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_PHASE_JITTER_PRESENT, &pStat->JitterBits[0], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    if (fp->frameHeader.PhaseJitterPresent == 1)
    {
        value = fp->frameHeader.PhaseJitterPercentage;
        SSCLOG_TRACE1(pLog, "phase_jitter_percentage : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_PHASE_JITTER_PERCENTAGE,
                                &pStat->JitterBits[0], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        value = fp->frameHeader.PhaseJitterBand;
        SSCLOG_TRACE1(pLog, "phase_jitter_band       : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_PHASE_JITTER_BAND,
                                &pStat->JitterBits[0], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");
    }
}


static void LOCAL_WriteFrameData(BSC_WRES* pWres, BSC_FRAME* fp, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG*               pLog  = NULL;

    UInt                  sf;
    UInt                  ch;

    SSC_TRANSIENT_QPARAM* pTQ;
    SSC_TRANSIENT_QPARAM* pTQPrev;

    SSC_SINUSOID_QPARAM*  pSQ;
    SSC_SINUSOID_QPARAM*  pSQPrev;

    SSC_NOISE_QPARAM*     pNQ;
    SSC_NOISE_QPARAM*     pNQPrev2;
    SSC_NOISE_QPARAM*     pNQPrev4;

    SSC_SINUSOID_QPARAM*  pSQNext;
    SSC_SINUSOID_QPARAM*  pSQNext2;

    SSC_STEREO_QPARAM*    pSTQ;
    SSC_STEREO_QPARAM*    pSTQPrev;

    UInt bits = BSC_BUFFER_GetBitLength(pWres->pBuf);
    pLog = pWres->pLog;

    for ( sf = 0; sf < SSC_MAX_SUBFRAMES; sf++ )
    {
        for ( ch = 0; ch < pWres->fileInfo.Channels; ch++)
        {
            SSCLOG_TRACE1(pLog, "****************** fr%d*sf%d*ch%d *************\n", pWres->TotalFramesWritten, sf, ch);

            /* ******************************************************************************** */
            /* get first all the pointers to T/S/N structures + their previous subframe pointers*/
            pTQ      = &fp->frameData[ch].T[BSC_HISTORY + sf];
            pSQ      = &fp->frameData[ch].S[BSC_HISTORY + sf];
            pNQ      = &fp->frameData[ch].N[BSC_HISTORY + sf];
            pSTQ     = &fp->ST             [BSC_HISTORY + sf];
            pSTQPrev = &fp->ST             [BSC_HISTORY + sf - SSC_StereoUpdateRateFactor[SSC_UPDATE_OCS_QMF]];

            pTQPrev  = &fp->frameData[ch].T[BSC_HISTORY + sf - 1];
            pSQPrev  = &fp->frameData[ch].S[BSC_HISTORY + sf - 1];
            pNQPrev2 = &fp->frameData[ch].N[BSC_HISTORY + sf - 2];
            pNQPrev4 = &fp->frameData[ch].N[BSC_HISTORY + sf - 4];

            pSQNext  = &fp->frameData[ch].S[BSC_HISTORY + sf + 1];
            pSQNext2 = &fp->frameData[ch].S[BSC_HISTORY + sf + 2];

            /* ******************************************************************************** */
            /* Transient  components */
            LOCAL_WriteFrameDataTransient(pWres, pTQ, sf, the_exception_context);

            /* ******************************************************************************** */
            /* Sinusoids  components */
            LOCAL_WriteFrameDataSinus(pWres, fp, pSQ, pSQPrev, pSQNext, pSQNext2, sf, the_exception_context);

            /* ******************************************************************************** */
            /* Noise      components */
            LOCAL_WriteFrameDataNoise(pWres, fp, pNQ, pNQPrev2, pNQPrev4, sf, the_exception_context);

            /* ******************************************************************************** */
            /* Stereo     components */
            if ( 
                (pWres->fileInfo.Mode    == SSC_MODE_LCSTEREO      ) && 
                (pWres->fileInfo.ModeExt == SSC_MODE_EXT_PAR_STEREO) &&
                ( (((sf+1)%SSC_StereoUpdateRateFactor[SSC_UPDATE_OCS_QMF])==0)    )
               )
            {
                /* Check if header has to be written */
                if ((fp->frameHeader.RefreshStereo == 1) && 
                          ((sf+1) == (UInt)SSC_StereoUpdateRateFactor[SSC_UPDATE_OCS_QMF])) 
                {
                    pSTQ->header = True;
                }

                /* STEREO BASE LAYER */
                LOCAL_WriteFrameDataStereo(pWres, fp, pSTQ, pSTQPrev, sf, the_exception_context);
            }

        }
        pWres->frameStatistics.BitsSubframe[sf] = BSC_BUFFER_GetBitLength(pWres->pBuf) - bits;
        bits = BSC_BUFFER_GetBitLength(pWres->pBuf);
    }
}


static void LOCAL_WriteFrameDataTransient(BSC_WRES*             pWres,
                                          SSC_TRANSIENT_QPARAM* pTQ,
                                          UInt                  sf,
                                          SSC_EXCEPTION*        the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;

    UInt      i;
    SInt      value;

    pLog = pWres->pLog;
    /* Transients components */
    SSCLOG_TRACE1(pLog, "*TRANSIENTS\n");

    /* t_bPresent **************************/
    if ( pTQ->t_bPresent == True)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }
    SSCLOG_TRACE1(pLog, "t_bPresent: %d", value);
    if ( (value < 0) || (value > 1) )
    {
        SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] t_transient_present: %d out of range\n", sf, value);
        Throw SSC_ERROR_NOT_AVAILABLE;
    }
    else
    {
        BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_TRANSIENT_PRESENT, &pStat->TraPresentBits[sf], the_exception_context);
    }
    SSCLOG_TRACE1(pLog, "\n");

    if ( pTQ->t_bPresent == True )
    {
        /* t_loc **************************/
        value = pTQ->t_loc;
        SSCLOG_TRACE1(pLog, "t_loc     : %d", value);
        if ( (value < 0) || (value > (SInt)(pWres->fileInfo.SubFrameSize - 1)) )
        {
            SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] t_loc: %d out of range\n", sf, value);
            Throw SSC_ERROR_T_LOC;
        }
        else
        {
            BSC_BUFFER_WriteUiMsbf( pBuf,value, (SInt)ceil(log(pWres->fileInfo.SubFrameSize)/log(2)), &pStat->TraLocBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        /* t_type **************************/
        value = pTQ->t_type;
        SSCLOG_TRACE1(pLog, "t_type    : %d", value);
        if ( (value < SSC_TRANSIENT_TYPE_STEP) || ( value > SSC_TRANSIENT_TYPE_RESERVED1) )
        {
            SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] t_type: %d out of range\n", sf, value);
            Throw SSC_ERROR_T_TYPE;
        }
        else
        {
            BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_TYPE, &pStat->TraTypeBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        if (pTQ->t_type == SSC_TRANSIENT_TYPE_MEIXNER)
        {
            SSC_MEIXNER_QPARAM* pTQM;

            pTQM = &pTQ->Parameters.Meixner;

            /* t_b_par **************************/
            value = pTQM->t_b_par;
            SSCLOG_TRACE1(pLog, "t_b_par   : %d", value);
            if ( (value < BSC_T_B_MIN) || (value > BSC_T_B_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] Meixner t_b_par: %d out of range\n", sf, value);
                Throw SSC_ERROR_T_B_PAR;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_B_PAR, &pStat->TraParBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* t_chi_par **************************/
            value = pTQM->t_chi_par;
            SSCLOG_TRACE1(pLog, "t_chi_par : %d", value);
            if ( (value < BSC_T_CHI_MIN) || (value > BSC_T_CHI_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] Meixner t_chi_par: %d out of range\n", sf, value);
                Throw SSC_ERROR_T_CHI_PAR;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_CHI_PAR , &pStat->TraChiBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* t_nrof_sin**************************/
            value = pTQM->t_nrof_sin - 1;
            SSCLOG_TRACE1(pLog, "t_nrof_sin: %d", value);
            if ( (value < 0) || (value > 7) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] Meixner t_nrof_sin: %d out of range\n", sf, value);
                Throw SSC_ERROR_T_NROF_SIN;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_NROF_SIN, &pStat->TraNrOfSinBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            for (i = 0; i < pTQM->t_nrof_sin; i++)
            {
                /* t_freq **************************/
                value = pTQM->SineParam[i].Frequency;
                SSCLOG_TRACE1(pLog, "t_freq [%d]: %d", i, value);
                if ( (value < BSC_T_FREQ_MIN) || (value > BSC_T_FREQ_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] Meixner t_freq: %d out of range\n", sf, value);
                    Throw SSC_ERROR_T_FREQ;
                }
                else
                {
                    BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_FREQ, &pStat->TraSinFreqBits[sf], the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");

                /* t_amp **************************/
                value = pTQM->SineParam[i].Amplitude;
                SSCLOG_TRACE1(pLog, "t_amp  [%d]: %d", i, value);
                if ( (value < BSC_T_AMP_MIN) || (value > BSC_T_AMP_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] Meixner t_amp: %d out of range\n", sf, value);
                    Throw SSC_ERROR_T_AMP;
                }
                else
                {
                    BSC_BUFFER_WriteUiMsbf( pBuf,value, SSC_BITS_T_AMP, &pStat->TraSinAmpBits[sf], the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");

                /* t_phi **************************/
                value = pTQM->SineParam[i].Phase;
                SSCLOG_TRACE1(pLog, "t_phi  [%d]: %d", i, value);
                if ( (value < BSC_T_PHASE_MIN) || (value > BSC_T_PHASE_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Transient Subframe[%d] Meixner t_phi: %d out of range\n", sf, value);
                    Throw SSC_ERROR_T_PHI;
                }
                else
                {
                    BSC_BUFFER_WriteSiMsbf( pBuf,value, SSC_BITS_T_PHI, &pStat->TraSinPhiBits[sf], the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");
            }
        }
    }
}


static void LOCAL_WriteFrameDataSinus(BSC_WRES*            pWres,
                                      BSC_FRAME*           fp,
                                      SSC_SINUSOID_QPARAM* pSQ,
                                      SSC_SINUSOID_QPARAM* pSQPrev,
                                      SSC_SINUSOID_QPARAM* pSQNext,
                                      SSC_SINUSOID_QPARAM* pSQNext2,
                                      UInt                 sf,
                                      SSC_EXCEPTION*       the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL ;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;

    UInt                  i;

    SSC_SIN_QSEGMENT*     pSQS;

    HUFFMAN_CODE_STRUCT*  phuf;
    UInt                  idx;
    SInt                  value;
    UInt                  uvalue;
    Bool                  bvalue;

    pLog  = pWres->pLog;
    SSCLOG_TRACE1(pLog, "*SINUSOIDS\n");
    SSCLOG_TRACE1(pLog, "*s_nrof_continuations : %d\n", pSQ->s_nrof_continuations);

    /* ******************************************************************** */
    /* *                                                                    */
    /* *              CONTINUATIONS                                         */
    /* *                                                                    */
    /* ******************************************************************** */

    for ( i = 0; i < pSQ->s_nrof_continuations; i++)
    {
        pSQS = &pSQ->SineParam[i];

        if ( sf == 0 )
        {
            /* s_cont *******************************/
            value = (SInt)pSQS->s_cont;
            SSCLOG_TRACE1(pLog, "s_cont            [%2d]: %d", i, value);
            phuf = &huf_s_cont;
            if ( (value < BSC_S_CONT_MIN) || ( value > BSC_S_CONT_MAX ) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_cont: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_S_NROF_CONTINUATIONS;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinConSContBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* only for logging/debugging*/
            value = pSQS->s_length;
            SSCLOG_TRACE1(pLog, "*s_length         [%2d]: %d\n", i, value);

            if ( fp->frameHeader.RefreshSinus == 1 )
            {
                UInt idx_next = 0;

                /* abs freq continuations *****************************/
                value = (SInt) pSQS->Sin.Frequency;
                SSCLOG_TRACE1(pLog, "s_freq            [%2d]: %d", i, value);
                phuf = &hufFreqContAbs;
                if (  (value < BSC_S_FREQ_MIN) || (value > BSC_S_FREQ_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] subframe[%d] refresh s_freq: %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_S_FREQ;
                }
                else
                {
                    LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinConFreqBits[sf], the_exception_context);
                    LOCAL_WriteFineValue(pWres, value-phuf->table[idx].Value, fp->frameHeader.freqGranularity, &pStat->SinConFreqBits[sf], pBuf, the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");

                /* abs amp continuations ******************************/
                value = (SInt) pSQS->Sin.Amplitude;
                SSCLOG_TRACE1(pLog, "s_amp             [%2d]: %d", i, value);
                phuf = &hufAmpContAbs;
                if (  (value < BSC_S_AMP_MIN) || (value > BSC_S_AMP_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] refresh s_amp: %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_S_AMP;
                }
                else
                {
                    LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinConAmpBits[sf], the_exception_context);
                    LOCAL_WriteFineValue(pWres, value-phuf->table[idx].Value, fp->frameHeader.ampGranularity, &pStat->SinConAmpBits[sf], pBuf, the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");

                /* abs phi continuations ******************************/
                value = pSQS->Sin.Phase;
                SSCLOG_TRACE1(pLog, "s_phi             [%2d]: %d", i, value);
                phuf = NULL;
                if ( (value < BSC_S_PHASE_MIN) || ( value > BSC_S_PHASE_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] refresh s_phi: %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_S_PHI;
                }
                else
                {
                    BSC_BUFFER_WriteSiMsbf( pBuf, value, SSC_BITS_S_PHI, &pStat->SinConPhiBits[sf], the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");
                
                /* Write APDCM grid (for continuations only!) */
                bvalue = pSQS->s_adpcm.bGridValid ;
                if (bvalue == False)
                {
                    SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] refresh adpcm grid: no ADPCM refresh\n", i, sf);
                    Throw SSC_ERROR_S_ADPCM_GRID;
                }
                else
                {
                    value = (SInt)(pSQS->s_adpcm.s_adpcm_grid) ;
                    SSCLOG_TRACE1(pLog, "s_adpcm_grid      [%2d]: %d", i, value);
                    phuf = &hufAdpcmGrid;
                    if ( (value < BSC_S_ADPCM_GRID_MIN) || ( value > BSC_S_ADPCM_GRID_MAX) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] refresh s_adpcm_grid: %d out of range\n", i, sf, value);
                        Throw SSC_ERROR_S_ADPCM_GRID;
                    }
                    else
                    {
                        LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                        /* Add the data to Phase Statistics */
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinConPhiBits[sf], the_exception_context);
                    }
                }    
                SSCLOG_TRACE1(pLog, "\n");

                if (pSQS->s_cont > 0)
                {
                    /* Write ADPCM data from next sub-frame */
                    /* pSQS->s_adpcm.bGridValid should be True */
                    SSC_SIN_QSEGMENT* pSQSNext;

                    /* Get the sinusoidal data from the next sub-frame */
                    idx_next = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQ, i+1);
                    if (idx_next == 0)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", i, sf);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }
                    pSQSNext = &(pSQNext->SineParam[idx_next-1]);
                   
                    /* delta freq continuations ****************************/
                    uvalue =  pSQSNext->Sin.Frequency;
                    SSCLOG_TRACE1(pLog, "s_adpcm_cont_freqpha sf+1 [%2d]: %d", idx_next-1, uvalue);
                    if (uvalue > BSC_S_FREQPHA_ADPCM_MAX)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: %d out of range\n", idx_next-1, sf+1, uvalue);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }
                    else
                    {
                        BSC_BUFFER_WriteUiMsbf( pBuf, uvalue, SSC_BITS_S_FREQ_PHASE_ADPCM, &pStat->SinConDeltaFreqBits[sf], the_exception_context );
                    }
                    SSCLOG_TRACE1(pLog, "\n");

                }

                if (pSQS->s_cont > 1)
                {
                    /* Write ADPCM data from the (next+1) sub-frame */
                    SSC_SIN_QSEGMENT* pSQSNext2;
                    UInt idx_next2;

                    idx_next2 = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQNext, idx_next);
                    if (idx_next2 == 0)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", idx_next-1, sf+1);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }
                    pSQSNext2 = &(pSQNext2->SineParam[idx_next2-1]);

                    /* delta freq continuations ****************************/
                    uvalue =  pSQSNext2->Sin.Frequency;
                    SSCLOG_TRACE1(pLog, "s_adpcm_cont_freqpha sf+2 [%2d]: %d", idx_next2-1, uvalue);
                    if (uvalue > BSC_S_FREQPHA_ADPCM_MAX)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: %d out of range\n", idx_next2-1, sf+2, uvalue);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }
                    else
                    {
                        BSC_BUFFER_WriteUiMsbf( pBuf, uvalue, SSC_BITS_S_FREQ_PHASE_ADPCM, &pStat->SinConDeltaFreqBits[sf], the_exception_context );
                    }
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
            else
            {
                SSC_SIN_QSEGMENT* pSQSPrev;

                /* get the sinus params from the (i+1)th continuation from the previous subframe */
                pSQSPrev = LOCAL_WRES_GetContinuingSinusSubframe(pSQPrev, i+1);
                if ( pSQSPrev == NULL)
                {
                    SSCLOG_TRACE5(pLog, "\nError: Unable to find sinus[%d] in previous subframe\n", i);
                    Throw SSC_ERROR_NOT_AVAILABLE;
                }

                /* Check if track is continued for two more sub-frames */
                /* If not, do not transmit any ADPCM data */
                if (pSQS->s_cont > 1)
                {
                    SSC_SIN_QSEGMENT* pSQSNext2;
                    UInt idx_next, idx_next2;

                    /* Get the sinusoidal data from the 2 sub-frames ahead */
                    idx_next  = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQ, i+1);
                    if (idx_next == 0)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", i, sf);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }

                    idx_next2 = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQNext, idx_next);
                    if (idx_next2 == 0)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", idx_next-1, sf+1);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }
                    pSQSNext2 = &(pSQNext2->SineParam[idx_next2-1]);

                    
                    /* delta freq continuations ****************************/
                    uvalue =  pSQSNext2->Sin.Frequency;
                    SSCLOG_TRACE1(pLog, "s_adpcm_cont_freqpha sf+2 [%2d]: %d", idx_next2-1, uvalue);
                    if (uvalue > BSC_S_FREQPHA_ADPCM_MAX)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: %d out of range\n", idx_next2-1, sf+2, uvalue);
                        Throw SSC_ERROR_S_DELTA_FREQ;
                    }
                    else
                    {
                        BSC_BUFFER_WriteUiMsbf( pBuf, uvalue, SSC_BITS_S_FREQ_PHASE_ADPCM, &pStat->SinConDeltaFreqBits[sf], the_exception_context );
                    }
                    SSCLOG_TRACE1(pLog, "\n");
                }

                /* delta amp continuations ****************************/
                value = (SInt)pSQS->Sin.Amplitude - (SInt)pSQSPrev->Sin.Amplitude;
                SSCLOG_TRACE1(pLog, "s_delta_cont_amp  [%2d]:(%d) %d", i, (SInt)pSQS->Sin.Amplitude, value);
                phuf = LOCAL_GetHuffmanTable( pWres, "AmpContRel",fp->frameHeader.ampGranularity, the_exception_context);
                if ( (value < BSC_S_AMP_DELTA_MIN) || (value > BSC_S_AMP_DELTA_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_delta_cont_amp: %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_S_DELTA_AMP;
                }
                else
                {
                    LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinConDeltaAmpBits[sf], the_exception_context);
                }
                SSCLOG_TRACE1(pLog, "\n");
            }
        }
        else
        {
            SSC_SIN_QSEGMENT* pSQSPrev;

            /* get the sinus params from the (i+1)th continuation from the previous subframe */
            pSQSPrev = LOCAL_WRES_GetContinuingSinusSubframe(pSQPrev, i+1);
            if ( pSQSPrev == NULL )
            {                                
                SSCLOG_TRACE5(pLog, "\nError: Unable to find sinus[%d] in previous subframe\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* for logging only */
            SSCLOG_TRACE1(pLog, "*s_cont           [%2d]: %d\n", i, (SInt)pSQS->s_cont);

            /* only for logging/debugging*/
            value = pSQS->s_length;
            SSCLOG_TRACE1(pLog, "*s_length         [%2d]: %d\n", i, value);

            /* delta freq continuations ****************************/
            /* Check if track is continued for two more sub-frames */
            /* If not, do not transmit any ADPCM data */
            if (pSQS->s_cont > 1)
            {
                SSC_SIN_QSEGMENT* pSQSNext2;
                 UInt idx_next, idx_next2;

                /* Get the sinusoidal data from the 2 sub-frames ahead */
                idx_next  = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQ, i+1);
                if (idx_next == 0)
                {
                  SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", i, sf);
                  Throw SSC_ERROR_S_DELTA_FREQ;
                }
                idx_next2 = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQNext, idx_next);
                if (idx_next2 == 0)
                {
                  SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", idx_next-1, sf+1);
                  Throw SSC_ERROR_S_DELTA_FREQ;
                }
                pSQSNext2 = &(pSQNext2->SineParam[idx_next2-1]);

                
                /* delta freq continuations ****************************/
                uvalue =  pSQSNext2->Sin.Frequency;
                SSCLOG_TRACE1(pLog, "s_adpcm_cont_freqpha sf+2 [%2d]: %d", idx_next2-1, uvalue);
                if (uvalue > BSC_S_FREQPHA_ADPCM_MAX)
                {
                  SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: %d out of range\n", idx_next2-1, sf+2, uvalue);
                  Throw SSC_ERROR_S_DELTA_FREQ;
                }
                else
                {
                  BSC_BUFFER_WriteUiMsbf( pBuf, uvalue, SSC_BITS_S_FREQ_PHASE_ADPCM, &pStat->SinConDeltaFreqBits[sf], the_exception_context );
                }
                SSCLOG_TRACE1(pLog, "\n");
            }
            /* delta amp continuations ****************************/
            value = (SInt)pSQS->Sin.Amplitude - (SInt)pSQSPrev->Sin.Amplitude;
            SSCLOG_TRACE1(pLog, "s_delta_cont_amp  [%2d]:(%d) %d", i, (SInt)pSQS->Sin.Amplitude, value);
            phuf = LOCAL_GetHuffmanTable( pWres, "AmpContRel",fp->frameHeader.ampGranularity, the_exception_context);
            if ( (value < BSC_S_AMP_DELTA_MIN) || (value > BSC_S_AMP_DELTA_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_delta_cont_amp: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_S_DELTA_AMP;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinConDeltaAmpBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");
        }
    }

    /* s_nrof_births *******************************/
    value = (SInt)pSQ->s_nrof_births;
    SSCLOG_TRACE1(pLog, "s_nrof_births         : %d", value);
    phuf = &huf_s_nrof_births;
    if ( (value < 0 )  || (value > SSC_S_MAX_SIN_TRACKS ) )
    {
        SSCLOG_TRACE5(pLog, "\nError: Birth Sinus Subframe[%d] s_nrof_births: %d out of range\n", sf, value);
        Throw SSC_ERROR_S_NROF_BIRTHS;
    }
    else
    {
        LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinNrOfBirthBits[sf], the_exception_context);
    }
    SSCLOG_TRACE1(pLog, "\n");

    /* ******************************************************************** */
    /* *                                                                    */
    /* *              BIRTHS                                                */
    /* *                                                                    */
    /* ******************************************************************** */

    for ( i = 0; i < pSQ->s_nrof_births; i++)
    {
        UInt idx_next = 0;

        pSQS = &pSQ->SineParam[i + pSQ->s_nrof_continuations];

        /* s_cont **************************/
        value = (SInt)pSQS->s_cont;
        SSCLOG_TRACE1(pLog, "s_cont            [%2d]: %d", i, value);
        phuf = &huf_s_cont;
        if ( (value < BSC_S_CONT_MIN) || (value > BSC_S_CONT_MAX) )
        {
            SSCLOG_TRACE5(pLog, "\nError: Birth Sinus[%d] Subframe[%d] s_cont: %d out of range\n", i, sf, value);
            Throw SSC_ERROR_S_NROF_CONTINUATIONS;
        }
        else
        {
            LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
            BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinBirthSContBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        /* only for logging/debugging*/
        value = pSQS->s_length;
        SSCLOG_TRACE1(pLog, "*s_length         [%2d]: %d\n", i, value);

        if ( i == 0)
        {
            /* abs freq births  *******************************************/
            value = (SInt)pSQS->Sin.Frequency;
            SSCLOG_TRACE1(pLog, "s_freq            [%2d]: %d", i, value);
            phuf = &hufFreqBirthAbs;
            if ( (value < BSC_S_FREQ_MIN) || (value > BSC_S_FREQ_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Birth Sinus[%d] Subframe[%d] s_freq: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_S_FREQ;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinBirthFreqBits[sf], the_exception_context);
                LOCAL_WriteFineValue(pWres, value-phuf->table[idx].Value, fp->frameHeader.freqGranularity, &pStat->SinBirthFreqBits[sf], pBuf, the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* abs amp births ********************************************/
            value = (SInt)pSQS->Sin.Amplitude;
            SSCLOG_TRACE1(pLog, "s_amp             [%2d]: %d", i, value);
            phuf = &hufAmpBirthAbs;
            if ( (value < BSC_S_AMP_MIN) || (value > BSC_S_AMP_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Birth Sinus[%d] Subframe[%d] s_amp: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_S_AMP;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinBirthAmpBits[sf], the_exception_context);
                LOCAL_WriteFineValue(pWres, value-phuf->table[idx].Value, fp->frameHeader.ampGranularity, &pStat->SinBirthAmpBits[sf], pBuf, the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");
        }
        else
        {

            /* delta freq births *******************************************************/
            value = (SInt)pSQS->Sin.Frequency - (SInt)(pSQS-1)->Sin.Frequency;
            SSCLOG_TRACE1(pLog, "s_delta_birth_freq[%2d]:(%d) %d", i, (SInt)pSQS->Sin.Frequency, value);
            phuf = &hufFreqBirthRel;
            if ( (value < BSC_S_FREQ_MIN) || (value > BSC_S_FREQ_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Birth Sinus[%d] Subframe[%d] s_delta_birth_freq: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_S_DELTA_FREQ;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinBirthDeltaFreqBits[sf], the_exception_context);
                LOCAL_WriteFineValue(pWres, value-phuf->table[idx].Value, fp->frameHeader.freqGranularity, &pStat->SinBirthDeltaFreqBits[sf], pBuf, the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* delta amp births *******************************************************/
            value = (SInt)pSQS->Sin.Amplitude - (SInt)(pSQS-1)->Sin.Amplitude;
            SSCLOG_TRACE1(pLog, "s_delta_birth_amp [%2d]:(%d) %d", i, (SInt)pSQS->Sin.Amplitude, value);
            phuf = &hufAmpBirthRel;
            if ( (value < -BSC_S_AMP_MAX) || (value > BSC_S_AMP_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Birth Sinus[%d] Subframe[%d] s_delta_birth_amp: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_S_DELTA_AMP;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndexCoarse( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->SinBirthDeltaAmpBits[sf], the_exception_context);
                LOCAL_WriteFineValue(pWres, value-phuf->table[idx].Value, fp->frameHeader.ampGranularity, &pStat->SinBirthDeltaAmpBits[sf], pBuf, the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");
        }

        /* abs phi births ******************************************************/
        value = pSQS->Sin.Phase;
        SSCLOG_TRACE1(pLog, "s_phi             [%2d]: %d", i, value);
        phuf = NULL;
        if ( (value < BSC_S_PHASE_MIN) || ( value > BSC_S_PHASE_MAX) )
        {
            SSCLOG_TRACE5(pLog, "\nError: Birth Sinus[%d] Subframe[%d] s_phi: %d out of range\n", i, sf, value);
            Throw SSC_ERROR_S_PHI;
        }
        else
        {
            BSC_BUFFER_WriteSiMsbf( pBuf, value, SSC_BITS_S_PHI, &pStat->SinBirthPhiBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        if (pSQS->s_cont > 0)
        {
            /* Write ADPCM data from next sub-frame */
            /* pSQS->s_adpcm.bGridValid should be True */
            SSC_SIN_QSEGMENT* pSQSNext;

            /* Get the sinusoidal data from the 2 sub-frames ahead */
            idx_next = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQ, pSQ->s_nrof_continuations+i+1);
            if (idx_next == 0)
            {
              SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", i, sf);
              Throw SSC_ERROR_S_DELTA_FREQ;
            }
            pSQSNext = &(pSQNext->SineParam[idx_next-1]);
           
            /* delta freq continuations ****************************/
            uvalue =  pSQSNext->Sin.Frequency;
            SSCLOG_TRACE1(pLog, "s_adpcm_cont_freqpha sf+1 [%2d]: %d", idx_next-1, uvalue);
            if (uvalue > BSC_S_FREQPHA_ADPCM_MAX)
            {
              SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: %d out of range\n", idx_next-1, sf+1, uvalue);
              Throw SSC_ERROR_S_DELTA_FREQ;
            }
            else
            {
              BSC_BUFFER_WriteUiMsbf( pBuf, uvalue, SSC_BITS_S_FREQ_PHASE_ADPCM, &pStat->SinConDeltaFreqBits[sf], the_exception_context );
            }
            SSCLOG_TRACE1(pLog, "\n");
        }

        if (pSQS->s_cont > 1)
        {
            /* Write ADPCM data from the (next+1) sub-frame */
            SSC_SIN_QSEGMENT* pSQSNext2;
             UInt idx_next2;

            idx_next2 = LOCAL_WRES_GetIndexContinuingSinusSubframe(pSQNext, idx_next);
            if (idx_next2 == 0)
            {
              SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: Continuing sinusoid cannot be found\n", idx_next-1, sf+1);
              Throw SSC_ERROR_S_DELTA_FREQ;
            }
            pSQSNext2 = &(pSQNext2->SineParam[idx_next2-1]);

            /* delta freq continuations ****************************/
            uvalue =  pSQSNext2->Sin.Frequency;
            SSCLOG_TRACE1(pLog, "s_adpcm_cont_freqpha sf+2 [%2d]: %d", idx_next2-1, uvalue);
            if (uvalue > BSC_S_FREQPHA_ADPCM_MAX)
            {
              SSCLOG_TRACE5(pLog, "\nError: Cont Sinus[%d] Subframe[%d] s_adpcm_cont_freqpha: %d out of range\n", i, sf, uvalue);
              Throw SSC_ERROR_S_DELTA_FREQ;
            }
            else
            {
              BSC_BUFFER_WriteUiMsbf( pBuf, uvalue, SSC_BITS_S_FREQ_PHASE_ADPCM, &pStat->SinConDeltaFreqBits[sf], the_exception_context );
            }
            SSCLOG_TRACE1(pLog, "\n");
        }
    }
}




static void LOCAL_WriteFrameDataNoise(BSC_WRES*         pWres,
                                      BSC_FRAME*        fp,
                                      SSC_NOISE_QPARAM* pNQ,
                                      SSC_NOISE_QPARAM* pNQPrev2,
                                      SSC_NOISE_QPARAM* pNQPrev4,
                                      UInt              sf,
                                      SSC_EXCEPTION*    the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;

    UInt                  i;

    HUFFMAN_CODE_STRUCT*  phuf;
    UInt                  idx;
    SInt                  value;

    pLog = pWres->pLog;
    SSCLOG_TRACE1(pLog, "*NOISE\n");
    if ( (sf == 0) && (fp->frameHeader.RefreshNoise == 1) )
    {
        /* Laguerre pole */
        value = pNQ->n_pole_laguerre;
        SSCLOG_TRACE1(pLog, "n_pole_laguerre: %d", value);

        if ( (value < 0) || (value > 2) )
        {
            SSCLOG_TRACE5(pLog, "\nError: Laguerre pole Subframe[%d] n_pole_laguerre: %d out of range\n", sf, value);
            Throw SSC_ERROR_N_LAGUERRE;
        }
        else
        {
            BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_POLE_LAGUERRE, &pStat->NoiPoleLaguerreBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        /* Laguerre quant grid */
        value = pNQ->n_grid_laguerre;
        SSCLOG_TRACE1(pLog, "n_grid_laguerre: %d", value);

        if ( (value < 0) || (value > 1) )
        {
            SSCLOG_TRACE5(pLog, "\nError: Laguerre grid Subframe[%d] n_grid_laguerre: %d out of range\n", sf, value);
            Throw SSC_ERROR_N_LAGUERRE;
        }
        else
        {
            BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_GRID_LAGUERRE, &pStat->NoiGridLaguerreBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        /* Lar den, huffman encoded using rel table */
        for (i = 0; i < fp->frameHeader.NumberDen; i++)
        {
            value = pNQ->n_lar_den[i];
            SSCLOG_TRACE1(pLog, "n_lar_den [%2d]:(%d) %d", i, pNQ->n_lar_den[i], value);
            phuf = &hufNoiRelLag;
            if ( (value < BSC_N_DEN_MIN) ||(value > BSC_N_DEN_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Denominator Noise[%d] Subframe[%d] n_lar_den: %d out of range\n", i, sf, value);
                Throw SSC_ERROR_N_LAR_DEN;
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndexCoarse( pWres, value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDenLARBits[sf], the_exception_context);

                if (pNQ->n_grid_laguerre == 1)
                {
                    LOCAL_WriteFineValue(pWres, value - phuf->table[idx].Value, 1, &pStat->NoiDenLARBits[sf], pBuf, the_exception_context);
                }
            } 

            SSCLOG_TRACE1(pLog, "\n");
        }

        /* env gain */
        value = pNQ->n_env_gain;
        SSCLOG_TRACE1(pLog, "n_env_gain            : %d", 0, value);
        phuf = NULL;
        if ( (value < BSC_N_ENV_GAIN_MIN) || (value > BSC_N_ENV_GAIN_MAX) )
        {
            SSCLOG_TRACE5(pLog, "\nError: ENV Gain Noise Subframe[%d] n_env_gain: %d out of range\n", sf, value);
            Throw SSC_ERROR_N_ENV_GAIN;
        }
        else
        {
            BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_ENV_GAIN, &pStat->NoiEnvGainBits[sf], the_exception_context);
        }
        SSCLOG_TRACE1(pLog, "\n");

        /* first abs lsf */
        i = 0;
        value = pNQ->n_lsf[i];
        SSCLOG_TRACE1(pLog, "n_lsf             [%2d]: %d", i, value);
        phuf = &hufNoiRelLsfE;
        if ( (value < BSC_N_LSF_MIN) || (value > BSC_N_LSF_MAX) )
        {
            SSCLOG_TRACE5(pLog, "\nError: LSF Noise[%d] Subframe[%d] n_lsf: %d out of range\n", i, sf, value);
            Throw SSC_ERROR_N_LSF;
        }
        else
        {
            if      ( value <= 7 )
            {
                /* write escape code */
                LOCAL_SearchHuffmanCodeIndex( pWres,  7, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiLsfBits[sf], the_exception_context);
                /* write also value */
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 3, &pStat->NoiLsfBits[sf], the_exception_context);
            }
            else if ( value >= 25 )
            {
                /* write escape code */
                LOCAL_SearchHuffmanCodeIndex( pWres,  25, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiLsfBits[sf], the_exception_context);
                /* write also value */
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 8, &pStat->NoiLsfBits[sf], the_exception_context);
            }
            else
            {
                LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiLsfBits[sf], the_exception_context);
            }
        }
        SSCLOG_TRACE1(pLog, "\n");

        /* delta lsf */
        for ( i = 1; i < SSC_N_MAX_LSF_ORDER; i++)
        {
            value = pNQ->n_lsf[i] - pNQ->n_lsf[i-1];
            SSCLOG_TRACE1(pLog, "n_delta_lsf       [%2d]:(%d) %d", i, pNQ->n_lsf[i], value);
            phuf = &hufNoiRelLsfE;
            if ( (value < BSC_N_LSF_DELTA_MIN) || (value > BSC_N_LSF_DELTA_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: LSF  Noise[%d] Subframe[%d] n_delta_lsf : %d out of range\n", i, sf, value);
                Throw SSC_ERROR_N_LSF;
            }
            else
            {
                if      ( value <= 7 )
                {
                    /* write escape code */
                    LOCAL_SearchHuffmanCodeIndex( pWres,  7, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                    /* write also value */
                    BSC_BUFFER_WriteUiMsbf( pBuf, value, 3, &pStat->NoiLsfBits[sf], the_exception_context);
                }
                else if ( value >= 25 )
                {
                    /* write escape code */
                    LOCAL_SearchHuffmanCodeIndex( pWres,  25, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                    /* write also value */
                    BSC_BUFFER_WriteUiMsbf( pBuf, value, 8, &pStat->NoiLsfBits[sf], the_exception_context);
                }
                else
                {
                    LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                }
            }
            SSCLOG_TRACE1(pLog, "\n");
        }
    }
    else
    {
        if ( (sf % 2) == 0 )
        {
            /* Laguerre quant grid */
            value = pNQ->n_grid_laguerre;
            SSCLOG_TRACE1(pLog, "n_grid_laguerre: %d", value);

            if ( (value < 0) || (value > 1) )
            {
                SSCLOG_TRACE5(pLog, "\nError: Laguerre grid Subframe[%d] n_grid_laguerre: %d out of range\n", sf, value);
                Throw SSC_ERROR_N_LAGUERRE;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_GRID_LAGUERRE, &pStat->NoiGridLaguerreBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* delta lar den with escape code*/
            for (i = 0; i < fp->frameHeader.NumberDen; i++)
            {
                if ( (pNQ->n_grid_laguerre == 0) && 
                     (pNQPrev2->n_grid_laguerre == 1) )
                {
                    value = pNQ->n_lar_den[i] - 4 * (SInt)floor( (Double)pNQPrev2->n_lar_den[i] / 4.0 + 0.5);
                }
                else
                {
                    value = pNQ->n_lar_den[i] - pNQPrev2->n_lar_den[i];
                }

                SSCLOG_TRACE1(pLog, "n_delta_lar_den   [%2d]:(%d) %d", i, pNQ->n_lar_den[i], value);
                phuf = &hufNoiRelLag;
                if ( (value < BSC_N_DEN_DELTA_MIN) ||(value > BSC_N_DEN_DELTA_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: Denominator Noise[%d] Subframe[%d] n_delta_lar_den: %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_N_LAR_DEN;
                }
                else
                {       
                    LOCAL_SearchHuffmanCodeIndexCoarse( pWres, value, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaDenLARBits[sf], the_exception_context);
                    
                    if (pNQ->n_grid_laguerre == 1)
                    {                        
                        /* Write extra two bits */
                        LOCAL_WriteFineValueNoise(pWres, value - phuf->table[idx].Value, &pStat->NoiDeltaDenLARBits[sf], pBuf, the_exception_context);
                    }
                    
                }
                SSCLOG_TRACE1(pLog, "\n");
            }
        }

        if ( (sf % 4 ) == 0 )
        {
            /* delta env gain with escape code*/
            value = pNQ->n_env_gain - pNQPrev4->n_env_gain;
            SSCLOG_TRACE1(pLog, "n_delta_env_gain      :(%d) %d", pNQ->n_env_gain, value);
            phuf = &hufNoiRelNdgE;
            if ( (value < BSC_N_ENV_GAIN_DELTA_MIN) || (value > BSC_N_ENV_GAIN_DELTA_MAX) )
            {
                SSCLOG_TRACE5(pLog, "\nError: ENV Gain Noise Subframe[%d] n_delta_env_gain: %d out of range\n", sf, value);
                Throw SSC_ERROR_N_ENV_GAIN;
            }
            else
            {
                if ( (value < -12) || (value > 12) )
                {
                    LOCAL_SearchHuffmanCodeIndex( pWres,  -13, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaEnvGainBits[sf], the_exception_context);
                    BSC_BUFFER_WriteSiMsbf( pBuf, value, 8, &pStat->NoiDeltaEnvGainBits[sf], the_exception_context);
                }
                else
                {
                    LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaEnvGainBits[sf], the_exception_context);
                }
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* lsf overlap */
            if ( pNQ->n_lsf_overlap > 0 )
                value = 1;
            else
                value = 0;
            SSCLOG_TRACE1(pLog, "n_overlap_lsf         : %d", value);
            phuf = NULL;
            if ( (value < 0) || (value > 1) )
            {
                SSCLOG_TRACE5(pLog, "\nError: LSF overlap Subframe[%d] n_lsf_overlap: %d out of range\n", sf, value);
                Throw SSC_ERROR_N_LSF;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf, value, SSC_BITS_N_OVERLAP_LSF, &pStat->NoiOverlapLsfBits[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");

            /* first lsf depends on lsf_overlap */
            i = pNQ->n_lsf_overlap;
            if ( i == 0 )
            {
                value = pNQ->n_lsf[i];
                SSCLOG_TRACE1(pLog, "n_lsf             [%2d]: %d", i, pNQ->n_lsf[i], value);
                phuf = &hufNoiRelLsfE;
                if ( (value < BSC_N_LSF_MIN) || (value > BSC_N_LSF_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: LSF  Noise[%d] Subframe[%d] n_lsf : %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_N_LSF;
                }
                else
                {
                    if      ( value <= 7 )
                    {
                        /* write escape code */
                        LOCAL_SearchHuffmanCodeIndex( pWres,  7, phuf, &idx, the_exception_context);
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiLsfBits[sf], the_exception_context);
                        /* write also value */
                        BSC_BUFFER_WriteUiMsbf( pBuf, value, 3, &pStat->NoiLsfBits[sf], the_exception_context);
                    }
                    else if ( value >= 25 )
                    {
                        /* write escape code */
                        LOCAL_SearchHuffmanCodeIndex( pWres,  25, phuf, &idx, the_exception_context);
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiLsfBits[sf], the_exception_context);
                        /* write also value */
                        BSC_BUFFER_WriteUiMsbf( pBuf, value, 8, &pStat->NoiLsfBits[sf], the_exception_context);
                    }
                    else
                    {
                        LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiLsfBits[sf], the_exception_context);
                    }
                }
                i += 1;
                SSCLOG_TRACE1(pLog, "\n");
            }

            /* rest are lsf delta's */
            for ( ; i < SSC_N_MAX_LSF_ORDER; i++ )
            {
                value = pNQ->n_lsf[i] - pNQ->n_lsf[i-1];
                SSCLOG_TRACE1(pLog, "n_delta_lsf       [%2d]:(%d) %d", i, pNQ->n_lsf[i], value);
                phuf = &hufNoiRelLsfE;
                if ( (value < BSC_N_LSF_DELTA_MIN) || (value > BSC_N_LSF_DELTA_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: LSF  Noise[%d] Subframe[%d] n_delta_lsf : %d out of range\n", i, sf, value);
                    Throw SSC_ERROR_N_LSF;
                }
                else
                {
                    if      ( value <= 7 )
                    {
                        /* write escape code */
                        LOCAL_SearchHuffmanCodeIndex( pWres,  7, phuf, &idx, the_exception_context);
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                        /* write also value */
                        BSC_BUFFER_WriteUiMsbf( pBuf, value, 3, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                    }
                    else if ( value >= 25 )
                    {
                        /* write escape code */
                        LOCAL_SearchHuffmanCodeIndex( pWres,  25, phuf, &idx, the_exception_context);
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                        /* write also value */
                        BSC_BUFFER_WriteUiMsbf( pBuf, value, 8, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                    }
                    else
                    {
                        LOCAL_SearchHuffmanCodeIndex( pWres,  value, phuf, &idx, the_exception_context);
                        BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->NoiDeltaLsfBits[sf], the_exception_context);
                    }
                }
                SSCLOG_TRACE1(pLog, "\n");
            }
        }
    }
}

static void LOCAL_WriteFrameStereoHeader(BSC_WRES*          pWres,
                                         SSC_STEREO_QPARAM* pSTQ,
                                         UInt               sf,
                                         SSC_EXCEPTION*     the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;
    SInt                  value;

    pLog = pWres->pLog;

    /* Generate header info if needed */
    value = pSTQ->header;
    SSCLOG_TRACE1(pLog, "enable_ps_header      : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoHeader[sf], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");

    if ( pSTQ->header )
    {
        /* enable_iid */
        value = pSTQ->iidEnable;
        SSCLOG_TRACE1(pLog, "enable_iid            : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoHeader[sf], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        if ( pSTQ->iidEnable )
        {
            /* iid_mode */
            switch ( pSTQ->iidQuant )
            {
                case OCS_QUANTISATION_COARSE:
                    value = 0;
                    break;
                case OCS_QUANTISATION_FINE:
                    value = OCS_NO_FREQ_RESOLUTIONS;
                    break;
                default:
                    SSCLOG_TRACE5(pLog, "\nError: iid_quant: %d out of range\n", pSTQ->iidQuant);
                    Throw SSC_ERROR_NOT_AVAILABLE;
                    break;
            }

            if (!( pSTQ->iidFreqRes < OCS_NO_FREQ_RESOLUTIONS ) )
            {
                SSCLOG_TRACE5(pLog, "\nError: iid_freq_res: %d out of range\n", pSTQ->iidFreqRes);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            
            value += pSTQ->iidFreqRes;
           
            SSCLOG_TRACE1(pLog, "iid_mode            : %d", value);
            if ( ( value <  0) || ( value > MAX_IID_MODE) )
            {
                SSCLOG_TRACE5(pLog, "\nError: iid_mode: %d out of range\n", value);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf, value, BITS_IID_MODE, &pStat->StereoHeader[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");
        }

        /* enable_icc */
        value = pSTQ->iccEnable;
        SSCLOG_TRACE1(pLog, "enable_icc            : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoHeader[sf], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        if ( pSTQ->iccEnable )
        {
            /* icc_mode */
            value = ( pSTQ->pcaEnable )? OCS_NO_FREQ_RESOLUTIONS: 0;

            if (!( pSTQ->iccFreqRes < OCS_NO_FREQ_RESOLUTIONS ))
            {
                SSCLOG_TRACE5(pLog, "\nError: icc_freq_res: %d out of range\n", pSTQ->iccFreqRes);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            value += pSTQ->iccFreqRes;

            SSCLOG_TRACE1(pLog, "icc_mode            : %d", value);
            if ( ( value <  0) || ( value > MAX_ICC_MODE) )
            {
                SSCLOG_TRACE5(pLog, "\nError: icc_mode: %d out of range\n", value);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf, value, BITS_ICC_MODE, &pStat->StereoHeader[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");
        }

        /* enable_ext */
        value = pSTQ->extEnable;
        SSCLOG_TRACE1(pLog, "enable_ext            : %d", value);
        BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoHeader[sf], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");
    }
}

static void LOCAL_WriteFrameStereoEnvelopes(BSC_WRES*          pWres,
                                            SSC_STEREO_QPARAM* pSTQ,
                                            UInt               sf,
                                            SSC_EXCEPTION*     the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;
    SInt                  value;
    UInt                  i;
    const UByte*          envelopes;

    pLog = pWres->pLog;

    /* frame_class */
    value = pSTQ->frameClass;
    SSCLOG_TRACE1(pLog, "frame_class      : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoEnvelopes[sf], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");


    /* num_env_idx */
    envelopes = ( pSTQ->frameClass == OCS_FRAME_FIX_BORDERS )? OCS_fixEnvelopes: OCS_varEnvelopes;
    for ( i = 0; i < OCS_NO_ENVELOPE_INDICES; i++ )
    {
        if ( envelopes[i] == pSTQ->envelopes )
        {
            break;
        }
    }
    value = i;

    SSCLOG_TRACE1(pLog, "num_env_idx            : %d", value);
    if ( ( value <  0) || ( value > OCS_NO_ENVELOPE_INDICES) )
    {
        SSCLOG_TRACE5(pLog, "\nError: num_env_idx: %d out of range\n", value);
        Throw SSC_ERROR_NOT_AVAILABLE;
    }
    else
    {
        BSC_BUFFER_WriteUiMsbf( pBuf, value, BITS_ENVELOPES, &pStat->StereoEnvelopes[sf], the_exception_context);
    }
    SSCLOG_TRACE1(pLog, "\n");

    if ( pSTQ->frameClass == OCS_FRAME_VAR_BORDERS )
    {
        /* border_position */
        value = pSTQ->frameSlotLength - 1;

        for ( i = 1; i <= pSTQ->envelopes; i++ )
        {
            SSCLOG_TRACE1(pLog, "border_position            : %d", pSTQ->envBorder[i] - 1);
            if ( ( (SInt)pSTQ->envBorder[i] - 1 <  0) || ((SInt)pSTQ->envBorder[i] - 1 > value) )
            {
                SSCLOG_TRACE5(pLog, "\nError: border_position: %d out of range\n", pSTQ->envBorder[i] - 1);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            else
            {
                BSC_BUFFER_WriteUiMsbf( pBuf, pSTQ->envBorder[i] - 1, BITS_BORDER, &pStat->StereoEnvelopes[sf], the_exception_context);
            }
            SSCLOG_TRACE1(pLog, "\n");
        }
    }
}


static void LOCAL_WriteFrameStereoData(BSC_WRES*          pWres,
                                       SSC_STEREO_QPARAM* pSTQ,
                                       SSC_STEREO_QPARAM* pSTQPrev,
                                       UInt               sf,
                                       SSC_EXCEPTION*     the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;
    SInt                  value, prevValue;
    HUFFMAN_CODE_STRUCT*  phuf;
    UInt                  idx;
    UInt                  env;
    UInt                  bin;
    UInt                  lengthDF;
    UInt                  lengthDT = 0;
    SInt                  diffFreq[OCS_MAX_FREQ_RESOLUTION];
    SInt                  diffTime[OCS_MAX_FREQ_RESOLUTION];

    pLog  = pWres->pLog;

    if ( pSTQ->iidEnable )
    {
        SInt                 iidMin;
        SInt                 iidMax;
        SInt                 iidDiffMin;
        SInt                 iidDiffMax;
        HUFFMAN_CODE_STRUCT* iidDiffFreqHET;
        HUFFMAN_CODE_STRUCT* iidDiffTimeHET;
        UInt                 iidBins;

        iidBins = OCS_fullFreqRes[pSTQ->iidFreqRes];

        if ( pSTQ->iidQuant == OCS_QUANTISATION_FINE )
        {
            iidDiffFreqHET = &OCS_HUF_iidDeltaFreqFineHET;
            iidDiffTimeHET = &OCS_HUF_iidDeltaTimeFineHET;
            iidMin = -OCS_NO_IID_STEPS_FINE;
            iidMax =  OCS_NO_IID_STEPS_FINE;
        }
        else
        {
            iidDiffFreqHET = &OCS_HUF_iidDeltaFreqCoarseHET;
            iidDiffTimeHET = &OCS_HUF_iidDeltaTimeCoarseHET;
            iidMin = -OCS_NO_IID_STEPS_COARSE;
            iidMax =  OCS_NO_IID_STEPS_COARSE;
        }
        iidDiffMin = iidMin - iidMax;
        iidDiffMax = iidMax - iidMin;

        for ( env = 0; env < pSTQ->envelopes; env++ )
        {
            /* delta coding in frequency direction */
            for ( bin = 0, prevValue = 0, lengthDF = 0; bin < iidBins; bin++ )
            {
                value = pSTQ->iid[env][bin];

                SSCLOG_TRACE1(pLog, "iid[%d][%d]            : %d", env, bin, value);
                if (!( ( value >= iidMin ) || ( value <= iidMax ) ))
                {
                    SSCLOG_TRACE5(pLog, "\nError: iid: %d out of range\n", value);
                    Throw SSC_ERROR_NOT_AVAILABLE;

                }
                SSCLOG_TRACE1(pLog, "\n");

                diffFreq[bin] = value - prevValue; 
                if ( (diffFreq[bin] < iidDiffMin) || (diffFreq[bin] > iidDiffMax) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: iid diffFreq: %d out of range\n", diffFreq[bin]);
                    Throw SSC_ERROR_NOT_AVAILABLE;
                }
                LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], iidDiffFreqHET, &idx, the_exception_context);                
                lengthDF     += iidDiffFreqHET->table[idx].Length;

                prevValue     = value;
            }

            if ( !pSTQ->header )
            {
                /* delta coding in time direction */
                for ( bin = 0, lengthDT = 0; bin < iidBins; bin++ )
                {
                    diffTime[bin] = pSTQ->iid[env][bin] - pSTQPrev->iid[env][bin];
                    if ( (diffTime[bin] < iidDiffMin) || (diffTime[bin] > iidDiffMax) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: iid diffFreq: %d out of range\n", diffFreq[bin]);
                        Throw SSC_ERROR_NOT_AVAILABLE;
                    }
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], iidDiffTimeHET, &idx, the_exception_context);                
                    lengthDT     += iidDiffTimeHET->table[idx].Length;
                }
            }

            if ( pSTQ->header || ( lengthDF < lengthDT ) )
            {
                /* iid_dt */
                value = DIFF_FREQ_CODING;
                SSCLOG_TRACE1(pLog, "iid_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoData[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");

                /* iid_par_df */
                for ( bin = 0; bin < iidBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "iid_par_df[%d][%d]            : %d", env, bin, diffFreq[bin]);
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], iidDiffFreqHET, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, iidDiffFreqHET->table[idx].Code, iidDiffFreqHET->table[idx].Length, &pStat->StereoData[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
            else
            {
                /* iid_dt */
                value = DIFF_TIME_CODING;
                SSCLOG_TRACE1(pLog, "iid_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoData[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");
 
                /* iid_par_dt */
                for ( bin = 0; bin < iidBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "iid_par_dt[%d][%d]            : %d", env, bin, diffTime[bin]);
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], iidDiffTimeHET, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, iidDiffTimeHET->table[idx].Code, iidDiffTimeHET->table[idx].Length, &pStat->StereoData[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
        }
    }

    /* icc */
    if ( pSTQ->iccEnable )
    {
        UInt                 iccBins;

        iccBins = OCS_fullFreqRes[pSTQ->iccFreqRes];

        for ( env = 0; env < pSTQ->envelopes; env++ )
        {
            /* delta coding in frequency direction */
            for ( bin = 0, prevValue = 0, lengthDF = 0; bin < iccBins; bin++ )
            {
                value = pSTQ->icc[env][bin];

                SSCLOG_TRACE1(pLog, "icc[%d][%d]            : %d", env, bin, value);
                if (!( ( value >= ICC_RANGE_MIN ) || ( value <= ICC_RANGE_MAX ) ))
                {
                    SSCLOG_TRACE5(pLog, "\nError: icc: %d out of range\n", value);
                    Throw SSC_ERROR_NOT_AVAILABLE;

                }
                SSCLOG_TRACE1(pLog, "\n");

                diffFreq[bin] = value - prevValue;
                if ( (diffFreq[bin] < ICC_D_RANGE_MIN) || (diffFreq[bin] > ICC_D_RANGE_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: icc diffFreq: %d out of range\n", diffFreq[bin]);
                    Throw SSC_ERROR_NOT_AVAILABLE;
                }
                phuf = &OCS_HUF_iccDeltaFreqHET;
                LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], phuf, &idx, the_exception_context);                
                lengthDF     += phuf->table[idx].Length;
                                
                prevValue     = value;
            }

            if ( !pSTQ->header )
            {
                /* delta coding in time direction */
                for ( bin = 0, lengthDT = 0; bin < iccBins; bin++ )
                {
                    diffTime[bin] = pSTQ->icc[env][bin] - pSTQPrev->icc[env][bin];
                    if ( (diffTime[bin] < ICC_D_RANGE_MIN) || (diffTime[bin] > ICC_D_RANGE_MAX) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: icc timeFreq: %d out of range\n", diffFreq[bin]);
                        Throw SSC_ERROR_NOT_AVAILABLE;
                    }
                    phuf = &OCS_HUF_iccDeltaTimeHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], phuf, &idx, the_exception_context);                
                    lengthDT     += phuf->table[idx].Length;
                }
            }

            if ( pSTQ->header || ( lengthDF < lengthDT ) )
            {
                /* icc_dt */
                value = DIFF_FREQ_CODING;
                SSCLOG_TRACE1(pLog, "icc_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoData[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");

                /* icc_par_df */
                for ( bin = 0; bin < iccBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "icc_par_df[%d][%d]            : %d", env, bin, diffFreq[bin]);
                    phuf = &OCS_HUF_iccDeltaFreqHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], phuf, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->StereoData[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
            else
            {
                /* icc_dt */
                value = DIFF_TIME_CODING;
                SSCLOG_TRACE1(pLog, "icc_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoData[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");
 
                /* icc_par_dt */
                for ( bin = 0; bin < iccBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "icc_par_dt[%d][%d]            : %d", env, bin, diffTime[bin]);
                    phuf = &OCS_HUF_iccDeltaTimeHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], phuf, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->StereoData[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
        }
    }
}


static void LOCAL_WriteFrameStereoExtension(BSC_WRES*          pWres,
                                            SSC_STEREO_QPARAM* pSTQ,
                                            SSC_STEREO_QPARAM* pSTQPrev,
                                            UInt               sf,
                                            SSC_EXCEPTION*     the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;
    SInt                  value, prevValue;
    HUFFMAN_CODE_STRUCT*  phuf;
    UInt                  idx;
    UInt                  env;
    UInt                  bin;
    UInt                  ipdBins, opdBins;
    UInt                  lengthDF;
    UInt                  lengthDT = 0;
    SInt                  diffFreq[OCS_MAX_FREQ_RESOLUTION];
    SInt                  diffTime[OCS_MAX_FREQ_RESOLUTION];

    pLog = pWres->pLog;

    ipdBins = OCS_halfFreqRes[pSTQ->iidFreqRes];
    opdBins = OCS_halfFreqRes[pSTQ->iidFreqRes];

    /* enable_ipdopd */
    value = pSTQ->ipdEnable;
    SSCLOG_TRACE1(pLog, "enable_ipdopd      : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoExtension[sf], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");


    /* ipd/opd */
    if ( pSTQ->ipdEnable )
    {
        for ( env = 0; env < pSTQ->envelopes; env++ )
        {
            /* delta coding in frequency direction */
            for ( bin = 0, prevValue = 0, lengthDF = 0; bin < ipdBins; bin++ )
            {
                value = pSTQ->ipd[env][bin];

                SSCLOG_TRACE1(pLog, "ipd[%d][%d]            : %d", env, bin, value);
                if (!( ( value >= IPD_RANGE_MIN ) || ( value <= IPD_RANGE_MAX ) ))
                {
                    SSCLOG_TRACE5(pLog, "\nError: ipd: %d out of range\n", value);
                    Throw SSC_ERROR_NOT_AVAILABLE;

                }
                SSCLOG_TRACE1(pLog, "\n");

                diffFreq[bin] = value - prevValue; 
                if ( diffFreq[bin] < 0 )
                {
                    diffFreq[bin] += IPD_D_RANGE_MAX + 1;
                }
                if ( (diffFreq[bin] < IPD_D_RANGE_MIN) || (diffFreq[bin] > IPD_D_RANGE_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: ipd diffFreq: %d out of range\n", diffFreq[bin]);
                    Throw SSC_ERROR_NOT_AVAILABLE;
                }

                phuf = &OCS_HUF_ipdDeltaFreqHET;
                LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], phuf, &idx, the_exception_context);                
                lengthDF     += phuf->table[idx].Length;

                prevValue     = value;
            }

            if ( !pSTQ->header )
            {
                /* delta coding in time direction */
                for ( bin = 0, lengthDT = 0; bin < ipdBins; bin++ )
                {
                    diffTime[bin] = pSTQ->ipd[env][bin] - pSTQPrev->ipd[env][bin];
                    if ( diffTime[bin] < 0 )
                    {
                        diffTime[bin] += IPD_D_RANGE_MAX + 1;
                    }

                    if ( (diffTime[bin] < IPD_D_RANGE_MIN) || (diffTime[bin] > IPD_D_RANGE_MAX) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: ipd diffTime: %d out of range\n", diffTime[bin]);
                        Throw SSC_ERROR_NOT_AVAILABLE;
                    }

                    phuf = &OCS_HUF_ipdDeltaTimeHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], phuf, &idx, the_exception_context);                
                    lengthDT     += phuf->table[idx].Length;
                }
            }

            if ( pSTQ->header || ( lengthDF < lengthDT ) )
            {
                /* ipd_dt */
                value = DIFF_FREQ_CODING;
                SSCLOG_TRACE1(pLog, "ipd_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoExtension[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");

                /* ipd_par_df */
                for ( bin = 0; bin < ipdBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "ipd_par_df[%d][%d]            : %d", env, bin, diffFreq[bin]);
                    phuf = &OCS_HUF_ipdDeltaFreqHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], phuf, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->StereoExtension[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
            else
            {
                /* ipd_dt */
                value = DIFF_TIME_CODING;
                SSCLOG_TRACE1(pLog, "ipd_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoExtension[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");
 
                /* ipd_par_dt */
                for ( bin = 0; bin < ipdBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "ipd_par_dt[%d][%d]            : %d", env, bin, diffTime[bin]);
                    phuf = &OCS_HUF_ipdDeltaTimeHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], phuf, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->StereoExtension[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }


            /* delta coding in frequency direction */
            for ( bin = 0, prevValue = 0, lengthDF = 0; bin < opdBins; bin++ )
            {
                value = pSTQ->opd[env][bin];

                SSCLOG_TRACE1(pLog, "opd[%d][%d]            : %d", env, bin, value);
                if (!( ( value >= OPD_RANGE_MIN ) || ( value <= OPD_RANGE_MAX ) ))
                {
                    SSCLOG_TRACE5(pLog, "\nError: opd: %d out of range\n", value);
                    Throw SSC_ERROR_NOT_AVAILABLE;

                }
                SSCLOG_TRACE1(pLog, "\n");

                diffFreq[bin] = value - prevValue;
                if ( diffFreq[bin] < 0 )
                {
                    diffFreq[bin] += OPD_D_RANGE_MAX + 1;
                }
                if ( (diffFreq[bin] < OPD_D_RANGE_MIN) || (diffFreq[bin] > OPD_D_RANGE_MAX) )
                {
                    SSCLOG_TRACE5(pLog, "\nError: opd diffFreq: %d out of range\n", diffFreq[bin]);
                    Throw SSC_ERROR_NOT_AVAILABLE;
                }

                phuf = &OCS_HUF_opdDeltaFreqHET;
                LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], phuf, &idx, the_exception_context);                
                lengthDF     += phuf->table[idx].Length;

                prevValue = value;
            }

            if ( !pSTQ->header )
            {
                /* delta coding in time direction */
                for ( bin = 0, lengthDT = 0; bin < opdBins; bin++ )
                {
                    diffTime[bin] = pSTQ->opd[env][bin] - pSTQPrev->opd[env][bin];
                    if ( diffTime[bin] < 0 )
                    {
                        diffTime[bin] += OPD_D_RANGE_MAX + 1;
                    }
                    if ( (diffTime[bin] < OPD_D_RANGE_MIN) || (diffTime[bin] > OPD_D_RANGE_MAX) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: opd diffTime: %d out of range\n", diffTime[bin]);
                        Throw SSC_ERROR_NOT_AVAILABLE;
                    }

                    phuf = &OCS_HUF_opdDeltaTimeHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], phuf, &idx, the_exception_context);                
                    lengthDT     += phuf->table[idx].Length;
                }
            }

            if ( pSTQ->header || ( lengthDF < lengthDT ) )
            {
                /* opd_dt */
                value = DIFF_FREQ_CODING;
                SSCLOG_TRACE1(pLog, "ipd_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoExtension[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");

                /* opd_par_df */
                for ( bin = 0; bin < opdBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "opd_par_df[%d][%d]            : %d", env, bin, diffFreq[bin]);
                    phuf = &OCS_HUF_opdDeltaFreqHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffFreq[bin], phuf, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->StereoExtension[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
            else
            {
                /* ipd_dt */
                value = DIFF_TIME_CODING;
                SSCLOG_TRACE1(pLog, "ipd_dt      : %d", value);
                BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoExtension[sf], the_exception_context);
                SSCLOG_TRACE1(pLog, "\n");
 
                /* ipd_par_dt */
                for ( bin = 0; bin < opdBins; bin++ )
                {
                    SSCLOG_TRACE1(pLog, "opd_par_dt[%d][%d]            : %d", env, bin, diffTime[bin]);
                    phuf = &OCS_HUF_opdDeltaTimeHET;
                    LOCAL_SearchHuffmanCodeIndex( pWres,  diffTime[bin], phuf, &idx, the_exception_context);                
                    BSC_BUFFER_WriteBsMsbf( pBuf, phuf->table[idx].Code, phuf->table[idx].Length, &pStat->StereoExtension[sf], the_exception_context);
                    SSCLOG_TRACE1(pLog, "\n");
                }
            }
        }
    }

    /* reserved_ps */
    value = False;
    SSCLOG_TRACE1(pLog, "reserved_ps      : %d", value);
    BSC_BUFFER_WriteUiMsbf( pBuf, value, 1, &pStat->StereoExtension[sf], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");
}

static void LOCAL_WriteFrameDataStereo(BSC_WRES*          pWres,
                                       BSC_FRAME*         fp,
                                       SSC_STEREO_QPARAM* pSTQ,
                                       SSC_STEREO_QPARAM* pSTQPrev,
                                       UInt               sf,
                                       SSC_EXCEPTION*     the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;
    SInt                  value, num_bits_left;

    UNREFERENCED_PARM(fp);

    pLog = pWres->pLog;
    SSCLOG_TRACE1(pLog, "*STEREO\n");

    LOCAL_WriteFrameStereoHeader(pWres, pSTQ, sf, the_exception_context); 
    LOCAL_WriteFrameStereoEnvelopes(pWres, pSTQ, sf, the_exception_context); 
    LOCAL_WriteFrameStereoData(pWres, pSTQ, pSTQPrev, sf, the_exception_context); 

    if (pSTQ->extEnable)
    {
        UInt extLength, Length;
        SInt dummy;

        /* Use temporary buffer */
        BSC_FRAME_BUFFER* pBufTemp;
        BSC_FRAME_BUFFER* pBufSaved;
        
        pBufTemp = BSC_BUFFER_CreateInstance(pLog);
        BSC_BUFFER_Clear(pBufTemp);

        /* Save current buffer */
        pBufSaved   = pWres->pBuf;
        pWres->pBuf = pBufTemp;
        LOCAL_WriteFrameStereoExtension(pWres, pSTQ, pSTQPrev, sf, the_exception_context); 
    
        /* Restore buffer */
        pWres->pBuf = pBufSaved;

        /* Write extension size to buffer */
        Length = BSC_BUFFER_GetBitLength(pBufTemp);
        extLength        = ( Length + BITS_EXTENSION_ID + 7 ) / 8; /* two bits for id */
        if ( extLength >= 15 )
        {
            SSCLOG_TRACE1(pLog, "extension_size      : 15");
            BSC_BUFFER_WriteUiMsbf( pBuf, 15, BITS_EXTENSION_SIZE, &pStat->StereoExtension[sf], the_exception_context);
            SSCLOG_TRACE1(pLog, "\n");

            value = extLength - 15;
            SSCLOG_TRACE1(pLog, "extension_esc_count      : %d", value);
            if ( (value < 0) || (value > 255) )
            {
                SSCLOG_TRACE5(pLog, "\nError: extension_esc_count: %d out of range\n", value);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            BSC_BUFFER_WriteUiMsbf( pBuf, value, BITS_EXTENSION_ESC_COUNT, &pStat->StereoExtension[sf], the_exception_context);
            SSCLOG_TRACE1(pLog, "\n");
        }
        else
        {
            value = extLength;
            SSCLOG_TRACE1(pLog, "extension_size      : %d", value);
            if ( (value < 0) || (value > 15) )
            {
                SSCLOG_TRACE5(pLog, "\nError: extension_size: %d out of range\n", value);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            BSC_BUFFER_WriteUiMsbf( pBuf, value, BITS_EXTENSION_SIZE, &pStat->StereoExtension[sf], the_exception_context);
            SSCLOG_TRACE1(pLog, "\n");
        }

        /* extension_id */
        SSCLOG_TRACE1(pLog, "extension_id      : %d", PS_EXTENSION_ID);
        BSC_BUFFER_WriteUiMsbf( pBuf, PS_EXTENSION_ID, BITS_EXTENSION_ID, &pStat->StereoExtension[sf], the_exception_context);
        SSCLOG_TRACE1(pLog, "\n");

        /* Write temporary buffer to current buffer */
        BSC_BUFFER_WriteBsMsbf( pBuf, pBufTemp->B, Length, &dummy, the_exception_context);

        /* Write padding bits to buffer */
        num_bits_left = extLength * 8 - ( Length + BITS_EXTENSION_ID );
        if (num_bits_left > 0)
        {
            SSCLOG_TRACE1(pLog, "num_bits_left      : %d", num_bits_left);
            BSC_BUFFER_WriteUiMsbf( pBuf, 0, num_bits_left, &pStat->StereoExtension[sf], the_exception_context);
            SSCLOG_TRACE1(pLog, "\n");
        }

        /* Free temporary buffer */
        BSC_BUFFER_DestroyInstance(pBufTemp);
    }
}

static void LOCAL_WriteFramePadding(BSC_WRES* pWres, SSC_EXCEPTION* the_exception_context)
{
    BSC_FRAME_BUFFER*     pBuf  = pWres->pBuf;
    SSCLOG*               pLog  = NULL ;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;

    pLog = pWres->pLog;
    SSCLOG_TRACE1(pLog, "padding_bit:");
    BSC_BUFFER_WritePadding(pBuf, &pStat->PaddingBits[SSC_MAX_SUBFRAMES -1], the_exception_context);
    SSCLOG_TRACE1(pLog, "\n");
}


static void LOCAL_WriteBitsPerSubframe( SSCLOG* pLog, SInt bits[SSC_MAX_SUBFRAMES] )
{
    SInt n = 0;

    UNREFERENCED_PARM(pLog);
    UNREFERENCED_PARM(bits);

    for ( n = 0; n < SSC_MAX_SUBFRAMES; n++)
    {
        SSCLOG_TRACE3(pLog, "%3d ", bits[n] );
    }
    SSCLOG_TRACE3(pLog, "\n");
}

static void LOCAL_WriteStatistics(BSC_WRES* pWres,     SSC_EXCEPTION* the_exception_context)
{
    SSCLOG*               pLog  = pWres->pLog;
    BSC_FRAME_STATISTICS* pStat = &pWres->frameStatistics;

    UNREFERENCED_PARM(the_exception_context);

    SSCLOG_TRACE3(pLog, "Bits for frame header            : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->HeaderBits);
    SSCLOG_TRACE3(pLog, "Bits for phase jitter            : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->JitterBits);

    /* transient bits */
    SSCLOG_TRACE3(pLog, "Bits for t_nroftrans             : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraPresentBits );
    SSCLOG_TRACE3(pLog, "Bits for t_loc                   : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraLocBits);
    SSCLOG_TRACE3(pLog, "Bits for t_type                  : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraTypeBits);
    SSCLOG_TRACE3(pLog, "Bits for t_b_par                 : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraParBits);
    SSCLOG_TRACE3(pLog, "Bits for t_chi_par               : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraChiBits);
    SSCLOG_TRACE3(pLog, "Bits for t_nrof_sin              : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraNrOfSinBits);
    SSCLOG_TRACE3(pLog, "Bits for t_freq                  : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraSinFreqBits);
    SSCLOG_TRACE3(pLog, "Bits for t_amp                   : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraSinAmpBits);
    SSCLOG_TRACE3(pLog, "Bits for t_phi                   : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->TraSinPhiBits);

    /* sinus bits */
    SSCLOG_TRACE3(pLog, "Bits for s_cont                  : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinConSContBits);
    SSCLOG_TRACE3(pLog, "Bits for s_freq continuing       : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinConFreqBits);
    SSCLOG_TRACE3(pLog, "Bits for s_amp  continuing       : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinConAmpBits);
    SSCLOG_TRACE3(pLog, "Bits for s_freq continuing delta : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinConDeltaFreqBits);
    SSCLOG_TRACE3(pLog, "Bits for s_amp  continuing delta : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinConDeltaAmpBits);
    SSCLOG_TRACE3(pLog, "Bits for s_phi  continuing       : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinConPhiBits);

    SSCLOG_TRACE3(pLog, "Bits for s_cont births           : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinBirthSContBits);
    SSCLOG_TRACE3(pLog, "Bits for s_nrof_births           : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinNrOfBirthBits);
    SSCLOG_TRACE3(pLog, "Bits for s_freq birth            : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinBirthFreqBits);
    SSCLOG_TRACE3(pLog, "Bits for s_amp  birth            : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinBirthAmpBits);
    SSCLOG_TRACE3(pLog, "Bits for s_freq birth delta      : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinBirthDeltaFreqBits);
    SSCLOG_TRACE3(pLog, "Bits for s_amp  birth delta      : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinBirthDeltaAmpBits);
    SSCLOG_TRACE3(pLog, "Bits for s_phi  birth            : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->SinBirthPhiBits);

    /* noise bits */
    SSCLOG_TRACE3(pLog, "Bits for n_lar_den               : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiDenLARBits);
    SSCLOG_TRACE3(pLog, "Bits for n_lsf                   : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiLsfBits);
    SSCLOG_TRACE3(pLog, "Bits for n_env_gain              : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiEnvGainBits);
    SSCLOG_TRACE3(pLog, "Bits for n_lsf_overlap           : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiOverlapLsfBits);

    SSCLOG_TRACE3(pLog, "Bits for n_pole_laguerre         : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiPoleLaguerreBits);
    SSCLOG_TRACE3(pLog, "Bits for n_grid_laguerre         : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiGridLaguerreBits);

    SSCLOG_TRACE3(pLog, "Bits for n_lar_den delta         : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiDeltaDenLARBits);
    SSCLOG_TRACE3(pLog, "Bits for n_lsf delta             : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiDeltaLsfBits);
    SSCLOG_TRACE3(pLog, "Bits for n_env_gain delta        : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->NoiDeltaEnvGainBits);

    /* stereo bits */
    SSCLOG_TRACE3(pLog, "Bits for stereo header           : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->StereoHeader);
    SSCLOG_TRACE3(pLog, "Bits for stereo envelopes        : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->StereoEnvelopes);
    SSCLOG_TRACE3(pLog, "Bits for stereo data             : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->StereoData);
    SSCLOG_TRACE3(pLog, "Bits for stereo extension        : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->StereoExtension);
    
    /* padding bits */
    SSCLOG_TRACE3(pLog, "Bits for padding                 : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->PaddingBits);

    SSCLOG_TRACE3(pLog, "Bits for Subframe                : "); LOCAL_WriteBitsPerSubframe( pLog, pStat->BitsSubframe);

    SSCLOG_TRACE3(pLog, "Bits for Frame                   : %d (%d)\n", pStat->TotalBytes*8, pStat->TotalBytes);
    SSCLOG_TRACE3(pLog, "\n");

}
