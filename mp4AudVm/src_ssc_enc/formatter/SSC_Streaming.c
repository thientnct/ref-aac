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

Source file: SSC_Streaming.h

Required libraries: <none>

Authors:
AP:    Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:    Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:    Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
TM: Tjeu Mans,     Philips CE - PDS-labs Eindhoven <tjeu.mans@philips.com>
AR: Aad Rijnberg,  Philips CE - PDS-labs Eindhoven <aad.rijnberg@philips.com>
NT:    Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:    Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
16-Aug-2002 TM/AR   m8540 improved noise coding + 8 sf/frame
20-Sep-2002 NT      RM2 change
21-Nov-2003 AG      Changes for RM4
************************************************************************/

/******************************************************************************
 *
 *   Module              : Streaming sbunches
 *
 *   Description         :
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_BTSTRM_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Log.h"
#include "SSC_Error.h"
#include "SSC_BitStrm_Types.h"
#include "bsc_buffer.h"

#include "bsc_wres.h"
#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"

#include "SSC_Streaming.h"
#include "SSC_Alloc.h"

#include "SSC_Encoder_Modules.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/* defines the pipe length */
#define PIPE_LOOK_AHEAD   (2)
#define PIPE_LENGTH       (SSC_MAX_SUBFRAMES + PIPE_LOOK_AHEAD)
#define PIPE_LENGTH_HIST  (4)
#define PROCESS_FILLING   (SSC_MAX_SUBFRAMES)


/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

#ifdef SSC_MP4REF_BUILD
static void LOCAL_ProcessPipe(SSC_STRM* pStrm, unsigned char ** ppFrameData, int * pFrameBytes);
#else
static void LOCAL_UpdateFileHeader( SSC_STRM *pStrm );
static void LOCAL_ProcessPipe(SSC_STRM* pStrm);
#endif
static void LOCAL_InitFrame(SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context);

static void LOCAL_GetTransients(SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context);
static void LOCAL_GetSinusoids (SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context);
static void LOCAL_GetNoise     (SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context);
static void LOCAL_GetStereo    (SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context);

static void LOCAL_GetSortedIndexiSinusoids(SSC_STRM* pStrm, UInt sf, UInt ch, UInt maxSinus);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

void SSC_STRM_Initialise(void)
{
    BSC_WRES_Initialise();
    SSCLOG_INITIALISE();
    return;
}

void SSC_STRM_Free(void)
{
    BSC_WRES_Free();
    SSCLOG_FREE();
    return;
}

#ifdef SSC_MP4REF_BUILD
SSC_STRM* SSC_STRM_CreateInstance(SSC_STRM_INFO* pInfo, char* pLogFile, UInt LogLevel, char* pSSCFile, unsigned char ** ppHeaderData, int * pHeaderBytes)
#else
SSC_STRM* SSC_STRM_CreateInstance(SSC_STRM_INFO* pInfo, char* pLogFile, UInt LogLevel, char* pSSCFile)
#endif
{
    SSC_STRM* pStrm = NULL;
    BSC_FILE_INFO  WresInfo;
    SSCLOGINFO     LogInfo;

    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error  = SSC_ERROR_OK;

    Try
    {
        pStrm = SSC_MALLOC(sizeof(SSC_STRM), "SSC Streaming Instance");
        if ( pStrm == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for streaming instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* initialize first all pointers*/
        pStrm->pLog  = NULL;
        pStrm->pWres = NULL;
        pStrm->pPipe = NULL;

        memset(&LogInfo, 0, sizeof(LogInfo));
        LogInfo.bFlushAfterWrite = False;
        LogInfo.bPrintTime       = False;
        LogInfo.bStdOut          = False;
        LogInfo.nTraceLevel      = LogLevel;
        LogInfo.szTitle[0]       = '\0';
        strcpy(LogInfo.szFileName, pLogFile );
        pStrm->pLog = SSCLOG_CREATEINSTANCE(&LogInfo);
#ifdef _SSC_LOG_ENABLE
        if ( pStrm->pLog == NULL )
        {
            fprintf(stdout,"\nWarning: Unable to create requested(%s) log file\n", pLogFile);
        }
#endif

        pStrm->newSubframes = -1; /* -1 : indicates streaming process is not yet started */
        pStrm->framesWritten = 0;
        pStrm->lastSinusRefresh = 0;

        /* reset all index arrays */
        memset(&pStrm->IndexArray, -1 , sizeof(pStrm->IndexArray) );

        /* reset all index arrays */
        memset(&pStrm->IndexArrayLA, -1 , sizeof(pStrm->IndexArrayLA) );

        /* reset all frame output data */
        memset(&(pStrm->FrameParam), 0x00, sizeof(pStrm->FrameParam) );

        /* save the information local*/
        memcpy(&(pStrm->Info), pInfo, sizeof(SSC_STRM_INFO) );

        if ( pStrm->Info.DecoderLevel != SSC_DEC_LEVEL_MEDIUM )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info decoderlevel(%d) out of range\n", pStrm->Info.DecoderLevel);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( pStrm->Info.Channels > SSC_MAX_CHANNELS )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info channels(%d) out of range\n", pStrm->Info.Channels);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( pStrm->Info.SubFrameSize > SSC_MAX_SUBFRAME_SAMPLES )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info SubFramesize(%d) out of range\n", pStrm->Info.SubFrameSize);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( pStrm->Info.Mode > SSC_MODE_LCSTEREO )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info mode(%d) out of range\n", pStrm->Info.Mode);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( (pStrm->Info.Enhancement < SSC_ENHANCEMENT_CONT_PHASE) || (pStrm->Info.Enhancement > SSC_ENHANCEMENT_ORIG_PHASE) )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info enhancement(%d) out of range\n", pStrm->Info.Enhancement);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if (
            (pStrm->Info.Mode == SSC_MODE_LCSTEREO ) &&
            (pStrm->Info.ModeExt > SSC_MODE_EXT_PAR_STEREO)
           )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info ModeExt(%d) out of range\n", pStrm->Info.ModeExt);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if (SSC_StereoNumberFreqBins[pStrm->Info.idxBins] > SSC_ENCODER_MAX_STEREO_BINS)
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info idxBins(%d) give wrong(%d) max number bins\n", pStrm->Info.idxBins, SSC_StereoNumberFreqBins[pStrm->Info.idxBins]);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if (SSC_StereoNumberITDFreqBins[pStrm->Info.idxBins][pStrm->Info.idxBinsItd] > SSC_StereoNumberFreqBins[pStrm->Info.idxBins])
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info idxBinsT(%d) give wrong(%d) max number bins\n", pStrm->Info.idxBinsItd, SSC_StereoNumberITDFreqBins[pStrm->Info.idxBins][pStrm->Info.idxBinsItd]);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( pStrm->Info.stereoUpdateRate != 0 )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Streaming Info stereoUpdateRate(%d) not supported\n", pStrm->Info.stereoUpdateRate);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* make a pipe object */
        pStrm->pPipe = SSCPIPE_InitInstance( pStrm->Info.Channels, pStrm->Info.SubFrameSize, pStrm->Info.DecoderLevel, PIPE_LENGTH_HIST, PIPE_LENGTH, pStrm->pLog);
        if ( pStrm->pPipe == NULL )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_LENGTH);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* make the instance write elementary stream */
        /* translate stream info into wres info */
        WresInfo.DecoderLevel     = pStrm->Info.DecoderLevel;
        WresInfo.EncoderVersion   = pStrm->Info.EncoderVersion;
        WresInfo.Enhancement      = pStrm->Info.Enhancement;
        WresInfo.Mode             = pStrm->Info.Mode;
        WresInfo.ModeExt          = pStrm->Info.ModeExt;
        WresInfo.EnvNoiseOrder    = pStrm->Info.EnvNoiseOrder;
        WresInfo.SampleRateIndex  = pStrm->Info.SampleRateIndex;

        WresInfo.StereoUpdateRate = pStrm->Info.stereoUpdateRate;
        WresInfo.StereoIdxBins    = pStrm->Info.idxBins;
        WresInfo.StereoIdxBinsItd = pStrm->Info.idxBinsItd;

        /* make a pipe object */
        pStrm->pPipe = SSCPIPE_InitInstance( pStrm->Info.Channels, pStrm->Info.SubFrameSize, pStrm->Info.DecoderLevel, PIPE_LENGTH_HIST, PIPE_LENGTH, pStrm->pLog);
        if ( pStrm->pPipe == NULL )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_LENGTH);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* WATCH OUT number channels will be adapted if parametric stereo */
        if ( (WresInfo.Mode == SSC_MODE_LCSTEREO ) && (WresInfo.ModeExt == SSC_MODE_EXT_PAR_STEREO) )
        {
            pStrm->Info.Channels = 1;
        }
        WresInfo.Channels = pStrm->Info.Channels;

        if ( WresInfo.SampleRateIndex > 3 )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Index Value(%d) corresponding to sampling rate out of range\n", WresInfo.SampleRateIndex);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        WresInfo.SynthesisMethod  = pStrm->Info.SynthesisMethod;
        WresInfo.UpdateRate       = pStrm->Info.UpdateRate;

        WresInfo.SubFrameSize     = pStrm->Info.SubFrameSize;

        pStrm->pWres = BSC_WRES_CreateInstance(pSSCFile, pStrm->pLog, &WresInfo );
        if ( pStrm->pWres == NULL )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Unable to create requested(%s) log file\n", pSSCFile);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* write the first part of the file */
#ifdef SSC_MP4REF_BUILD
        Error = BSC_WRES_WriteFileHeader(pStrm->pWres, ppHeaderData, pHeaderBytes);
#else
        Error = BSC_WRES_WriteFileHeader(pStrm->pWres);
#endif
        if ( Error != SSC_ERROR_OK )
        {
            SSCLOG_TRACE5(pStrm->pLog,"\nError: Unable to write file header\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

    }
    Catch( Error )
    {
#ifdef SSC_MP4REF_BUILD
        SInt i0, i1, i2, i3, i4, i5;
        UByte c0, c1, c2;
        /* clean up the whole stuff */
        SSC_STRM_DestroyInstance(pStrm, &c0, &i0, &i1, &c1, &i2, &i3, &c2, &i4, &i5);
#else
        /* clean up the whole stuff */
        SSC_STRM_DestroyInstance(pStrm);
#endif
        pStrm = NULL;
    }

    return pStrm;
}

#ifdef SSC_MP4REF_BUILD
void SSC_STRM_DestroyInstance(SSC_STRM* pStrm,
                              UByte* pFileLengthValue,
                              SInt*  pFileLengthPos,
                              SInt*  pFileLengthBytes,
                              UByte* pAudioLengthValue,
                              SInt*  pAudioLengthPos,
                              SInt*  pAuidoLengthBytes,
                              UByte* pNrOfFramesValue,
                              SInt*  pNrOfFramesPos,
                              SInt*  pNrOfFramesBytes
                              )
#else
void SSC_STRM_DestroyInstance(SSC_STRM* pStrm)
#endif
{
    if ( pStrm != NULL )
    {
        if ( pStrm->pWres != NULL )
        {
            /* ready to write frames: update the length of the form chunk "SSC ' + sound data chunk ' SCHK'*/
#ifdef SSC_MP4REF_BUILD
            /* RJ: only get updated chunk lengths, */
            /* writing is left up to the application */
            if (BSC_WRES_GetFileChunkLengths(pStrm->pWres,
                              pFileLengthValue,
                              pFileLengthPos,
                              pFileLengthBytes,
                              pAudioLengthValue,
                              pAudioLengthPos,
                              pAuidoLengthBytes,
                              pNrOfFramesValue,
                              pNrOfFramesPos,
                              pNrOfFramesBytes)
                != SSC_ERROR_OK)
#else
            if (BSC_WRES_WriteFileChunkLengths(pStrm->pWres) != SSC_ERROR_OK)
#endif
            {
                SSCLOG_TRACE5(pStrm->pLog,"\nError: Writting correct chunk lengths in file failed\n");
            }
        }

        if ( pStrm->pWres != NULL )
        {
            BSC_WRES_DestroyInstance(pStrm->pWres);
        }
        pStrm->pWres = NULL;

        if ( pStrm->pPipe != NULL )
        {
            SSCPIPE_DestroyInstance(pStrm->pPipe);
        }
        pStrm->pPipe = NULL;

        if ( pStrm->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE(pStrm->pLog);
        }
        pStrm->pLog = NULL;

        SSC_FREE(pStrm, "SSC Streaming Instance");
        pStrm = NULL;
    }
}


#ifdef SSC_MP4REF_BUILD
SSCBUNCH* SSC_STRM_Process(SSC_STRM* pStrm, SSCBUNCH* pSb, unsigned char ** ppFrameData, int * pFrameBytes)
#else
SSCBUNCH* SSC_STRM_Process(SSC_STRM* pStrm, SSCBUNCH* pSb)
#endif
{
    SSCPIPE* pPipe = pStrm->pPipe;
    SSCBUNCH* pSBunch = NULL;

    /* putt the new one in the pipe */
    SSCPIPE_Add(pPipe, pSb);

    /* the first valid subframes is the trigger to count*/
    if  ( pStrm->newSubframes == -1 )
    {
        pSBunch = SSCPIPE_GetAt(pPipe, (PIPE_LENGTH + PIPE_LENGTH_HIST - 1 - PIPE_LOOK_AHEAD));
        if ( SSCBUNCH_GetDataFromStream(pSBunch) == True )
        {
            /* only start if we are not started before */
            if ( pStrm->framesWritten == 0 )
            {
                pStrm->newSubframes = 1;
            }
            else
            {
                /* do nothing --> no restart */
            }
        }
    }
    else
    {
        pStrm->newSubframes++;
    }

    /* check if pipe is filled with SSC_MAX_SUBFRAMES NEW subframes*/
    if ( pStrm->newSubframes == PROCESS_FILLING )
    {
        /* check if first subframe consists of valid data */
        if ( SSCBUNCH_GetDataFromStream( SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST) ) == True )
        {
            /* do the module specific trick*/
#ifdef SSC_MP4REF_BUILD            
            LOCAL_ProcessPipe(pStrm, ppFrameData, pFrameBytes);
#else
            LOCAL_ProcessPipe(pStrm);
#endif            
            /* if last subframe is invalid then force all subframes to be valid*/
            if ( SSCBUNCH_GetDataFromStream( SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + SSC_MAX_SUBFRAMES - 1) ) == False )
            {
                SInt sf = 0;

                for ( sf = 1; sf < SSC_MAX_SUBFRAMES; sf++)
                {
                    SSCBUNCH_SetDataFromStream( SSCPIPE_GetAt(pStrm->pPipe, PIPE_LENGTH_HIST + sf), True);
                }
            }

            /* reset the counter indicating how many new sf are needed*/
            pStrm->newSubframes = 0;
            /* update local variable for the times a frame is written*/
            pStrm->framesWritten++;
#ifndef SSC_MP4REF_BUILD            
            LOCAL_UpdateFileHeader( pStrm );
#endif
        }
        else
        {
            pStrm->newSubframes = -1;
        }
    }

    /* shift the processed subframe out */
    return SSCPIPE_Shift(pPipe);
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/
#ifndef SSC_MP4REF_BUILD            
static void LOCAL_UpdateFileHeader( SSC_STRM *pStrm )
{
    SInt           nFilePos              = -1;
    SInt           nTraceLevel           = -1 ;

    if( pStrm->pWres->pStream != NULL )
    {
        nFilePos    = ftell( pStrm->pWres->pStream );
    }
    if( pStrm->pLog != NULL )
    {
        nTraceLevel = SSCLOG_GETTRACELEVEL( pStrm->pLog );
    }

    SSCLOG_SETTRACELEVEL( pStrm->pLog, 5 );
    if (BSC_WRES_WriteFileChunkLengths(pStrm->pWres) != SSC_ERROR_OK)
    {
        SSCLOG_TRACE5(pStrm->pLog,"\nError: Writing correct chunk lengths in file failed\n");
    }
    SSCLOG_SETTRACELEVEL( pStrm->pLog, nTraceLevel );

    if( nFilePos != -1 )
    {
        fseek( pStrm->pWres->pStream, nFilePos, SEEK_SET );
    }
    if( nTraceLevel != -1 )
    {
        SSCLOG_SETTRACELEVEL( pStrm->pLog, nTraceLevel );
    }
}
#endif

#ifdef SSC_MP4REF_BUILD            
static void LOCAL_ProcessPipe(SSC_STRM* pStrm, unsigned char ** ppFrameData, int * pFrameBytes)
#else
static void LOCAL_ProcessPipe(SSC_STRM* pStrm)
#endif
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error  = SSC_ERROR_OK;

    UInt sf = 0;
    UInt BitsFrame[SSC_MAX_SUBFRAMES][6];

    Try
    {
        LOCAL_InitFrame(pStrm, the_exception_context);

        LOCAL_GetTransients(pStrm, the_exception_context);
        LOCAL_GetSinusoids(pStrm, the_exception_context);
        LOCAL_GetNoise(pStrm, the_exception_context);

        /* if parametric stereo data is present get it */
        if ( 
            (pStrm->Info.Mode    == SSC_MODE_LCSTEREO       ) &&
            (pStrm->Info.ModeExt == SSC_MODE_EXT_PAR_STEREO )
           )
        {
            LOCAL_GetStereo( pStrm, the_exception_context );
        }

#ifdef SSC_MP4REF_BUILD            
        Error = BSC_WRES_WriteFileFrame(pStrm->pWres, &pStrm->FrameParam, BitsFrame, ppFrameData, pFrameBytes);
#else
        Error = BSC_WRES_WriteFileFrame(pStrm->pWres, &pStrm->FrameParam, BitsFrame);
#endif
        if ( Error != SSC_ERROR_OK )
        {
            Throw Error;
        }

        /* set all bit values per subframe in the corresponding sbunch */
        for ( sf = 0; sf < SSC_MAX_SUBFRAMES; sf++)
        {
            Double bits[6];
            UInt n = 0;

            for ( n = 0; n < 6; n++ )
            {
                bits[n] = BitsFrame[sf][n];
            }
            SSCBUNCH_SetActualBitrate( SSCPIPE_GetAt(pStrm->pPipe, PIPE_LENGTH_HIST + sf), &bits[0] );
        }

    }
    Catch( Error )
    {
        /* flush the log file */
        SSCLOG_SETFLUSHAFTERWRITE(pStrm->pLog, True);
        SSCLOG_TRACE5(pStrm->pLog,"\nError: Unable to stream frame[%d]\n", pStrm->framesWritten);
        SSCLOG_SETFLUSHAFTERWRITE(pStrm->pLog, False);

        /* set in the last subframe the error code */
        SSCBUNCH_SetErrorCode( SSCPIPE_GetAt(pStrm->pPipe, PIPE_LENGTH_HIST + PIPE_LENGTH  - 1 ), Error);
        SSCBUNCH_SetErrorOrigin( SSCPIPE_GetAt(pStrm->pPipe, PIPE_LENGTH_HIST + PIPE_LENGTH  - 1 ), SSCENC_BITSTREAM_MODULE);
    }

}


static void LOCAL_InitFrame(SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context)
{

    SSCLOG*           pLog    = NULL;
    BSC_FRAME_HEADER* pHeader = &(pStrm->FrameParam.frameHeader);
    BSC_FRAME_DATA*   pData   = &(pStrm->FrameParam.frameData[0]);
    SSCPIPE*          pPipe   = pStrm->pPipe;
    SSCBUNCH*         pSb     = NULL;

    UInt channels = pStrm->Info.Channels;

    SInt startIdx = 0;
    UInt sf = 0;
    SInt i  = 0;
    UInt ch = 0;

    pLog    = pStrm->pLog;

    /* ************************************************************************************************ */
    /* get the freq/amp Granularity from first channel/ first subframe
     * and check if in other channels/sf they are the same */
    pSb = SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST);
    pHeader->freqGranularity = (UInt)SSCBUNCH_GetQSinusoidFreqGrid( pSb, 0);
    if ( pHeader->freqGranularity > 3 )
    {
        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[0] Input Freq granularity out of range: %d\n", ch, pHeader->freqGranularity);
        Throw SSC_ERROR_S_FREQ;
    }

    pHeader->ampGranularity = (UInt)SSCBUNCH_GetQSinusoidAmpGrid(pSb, 0);
    if ( pHeader->ampGranularity > 3 )
    {
        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[0] Input Amp granularity out of range: %d\n", ch, pHeader->ampGranularity);
        Throw SSC_ERROR_S_AMP;
    }

    pHeader->PhaseJitterPresent = ((UInt)SSCBUNCH_GetQSinusoidPhaseJitterPercentage(pSb, 0) > 0);
    pHeader->PhaseJitterPercentage = (UInt)SSCBUNCH_GetQSinusoidPhaseJitterPercentage(pSb, 0) - 1;
    pHeader->PhaseJitterBand = (UInt)SSCBUNCH_GetQSinusoidPhaseJitterBand(pSb, 0);

    for (sf = 0; sf < SSC_MAX_SUBFRAMES; sf++)
    {
        pSb = SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + sf);
        for ( ch = 0; ch < channels; ch++)
        {
            if ( (UInt)SSCBUNCH_GetQSinusoidFreqGrid(pSb, ch) != pHeader->freqGranularity )
            {
                SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] input Freq granularity: %d different then first channel definition: %d\n", ch, sf, SSCBUNCH_GetQSinusoidFreqGrid(pSb, ch), pHeader->freqGranularity);
                Throw SSC_ERROR_S_FREQ;
            }

            if ( (UInt)SSCBUNCH_GetQSinusoidAmpGrid(pSb, ch) != pHeader->ampGranularity )
            {
                SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] input Amp granularity: %d different then first channel definition: %d\n", ch, sf, SSCBUNCH_GetQSinusoidAmpGrid(pSb, ch), pHeader->ampGranularity);
                Throw SSC_ERROR_S_AMP;
            }
        }
    }


    /* ************************************************************************************************ */
    pHeader->NumberDen = 0;

    /* ************************************************************************************************ */
    /* do simple reshresh strategy */
    if ( pStrm->framesWritten == 0 )
    {
        pHeader->RefreshNoise = 1;
        pHeader->RefreshSinusNextFrame = 0;
        pHeader->RefreshSinus = 1;
        pHeader->RefreshStereo= 1;
    }
    else
    {
        pHeader->RefreshNoise = 0;
        pHeader->RefreshSinusNextFrame = 0;
        pHeader->RefreshSinus = 0;
        pHeader->RefreshStereo= 0;
    }

    /* ************************************************************************************************ */
    /* clear the channel dependant params of the frame + copy the last
     * one to the first */
    for (ch = 0; ch < channels; ch++ )
    {
        /* ************************************************* */
        /* first copy the last subframe the the first position of this channel */
        for ( i = 0; i < PIPE_LENGTH_HIST; i++)
        {
            memcpy(&pData[ch].T [i], &pData[ch].T [PIPE_LENGTH-PIPE_LOOK_AHEAD+i], sizeof(SSC_TRANSIENT_QPARAM) );
            memcpy(&pData[ch].S [i], &pData[ch].S [PIPE_LENGTH-PIPE_LOOK_AHEAD+i], sizeof(SSC_SINUSOID_QPARAM ) );
            memcpy(&pData[ch].N [i], &pData[ch].N [PIPE_LENGTH-PIPE_LOOK_AHEAD+i], sizeof(SSC_NOISE_QPARAM    ) );
            
            memcpy( &pStrm->FrameParam.ST [i], &pStrm->FrameParam.ST [PIPE_LENGTH-PIPE_LOOK_AHEAD+i], sizeof(SSC_STEREO_QPARAM ) );
        }

        if ( pStrm->framesWritten != 0  )
        {
            startIdx = PIPE_LENGTH_HIST;
        }
        else
        {
            startIdx = 0;
        }

        /* ************************************************* */
        /* clear the T/S/N of the rest of the subframes */
        for ( sf = startIdx; sf < (PIPE_LENGTH + PIPE_LENGTH_HIST); sf++)
        {
            pData[ch].T[sf].t_bPresent = False;

            pData[ch].S[sf].s_nrof_births        = 0;
            pData[ch].S[sf].s_nrof_continuations = 0;

            pData[ch].N[sf].n_env_gain           = 0;
            pData[ch].N[sf].n_lsf_overlap        = 0;

            pData[ch].N[sf].n_grid_laguerre      = 0;
            pData[ch].N[sf].n_pole_laguerre      = 0;

        }
    }
}


static void LOCAL_GetTransients(SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context)
{

    SSCLOG*          pLog    = NULL ;
    BSC_FRAME_DATA*  pData   = &(pStrm->FrameParam.frameData[0]);
    SSCPIPE*         pPipe   = pStrm->pPipe;

    SSC_TRANSIENT_QPARAM* pT  = NULL;
    SSC_MEIXNER_QPARAM*   pTM = NULL;
    SSCBUNCH*  pSb = NULL;

    UInt ch = 0;
    UInt sf = 0;
    UInt j = 0;
    UInt k = 0;
    Double value = 0;
    UInt nrOfSin = 0;

    pLog = pStrm->pLog;
    /* do for all subframes */
    for ( sf = 0; sf < PIPE_LENGTH; sf++)
    {
        /* get the sbunch for this subframe*/
        pSb = SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + sf);
        /* check if there is data in it */
        if ( SSCBUNCH_GetDataFromStream(pSb) == True )
        {
            /* do for all channels in this sbunch */
            for (ch = 0; ch < pStrm->Info.Channels; ch++)
            {
                pT = &(pData[ch].T[PIPE_LENGTH_HIST + sf]);
                if ( SSCBUNCH_GetQTransientPresent(pSb, ch) == True )
                {
                    pT->t_bPresent = True;
                    pT->t_loc = SSCBUNCH_GetQTransientPos(pSb, ch) ;
                    if ( pT->t_loc > (pStrm->Info.SubFrameSize - 1) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] subframe[%d] illegal position: %d\n", ch, sf, pT->t_loc);
                        Throw SSC_ERROR_T_LOC;
                    }

                    pT->t_type =  SSCBUNCH_GetQTransientType(pSb, ch);
                    switch (pT->t_type)
                    {
                        case SSC_TRANSIENT_TYPE_MEIXNER:
                            pTM = &(pT->Parameters.Meixner);

                            value = SSCBUNCH_GetQTransientBpar(pSb, ch);
                            pTM->t_b_par = (UInt)floor(value + 0.5 );
                            if ( pTM->t_b_par > BSC_T_B_MAX )
                            {
                                SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] Subframe[%d] input transient Meixner b out of range: %d\n", ch, sf, pTM->t_b_par);
                                Throw SSC_ERROR_T_B_PAR;
                            }

                            value = SSCBUNCH_GetQTransientChi(pSb, ch);
                            pTM->t_chi_par = (UInt)floor(value + 0.5 );
                            if ( pTM->t_chi_par > BSC_T_CHI_MAX )
                            {
                                SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] Subframe[%d] input transinet Meixner chi out of range: %d\n", ch, sf, pTM->t_chi_par);
                                Throw SSC_ERROR_T_CHI_PAR;
                            }

                            nrOfSin =   SSCBUNCH_GetQTransientMeixnerSins(pSb, ch);
                            for (j = 0, k = 0; j < nrOfSin; j++)
                            {
                                value = SSCBUNCH_GetQTransientMeixnerFreq(pSb, ch)[j];
                                /* check if sinus under transient is valid */
                                if ( value != -1 )
                                {
                                    pTM->SineParam[j].Frequency = (UInt)floor(value + 0.5);
                                    if ( pTM->SineParam[j].Frequency > BSC_T_FREQ_MAX )
                                    {
                                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] Subframe[%d] input Freq(transient) out of range: %d\n", ch, sf, pTM->SineParam[j].Frequency);
                                        Throw SSC_ERROR_T_FREQ;
                                    }

                                    value = SSCBUNCH_GetQTransientMeixnerAmp(pSb, ch)[j];
                                    pTM->SineParam[j].Amplitude = (UInt)floor(value + 0.5);
                                    if ( pTM->SineParam[j].Amplitude > BSC_T_AMP_MAX )
                                    {
                                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] Subframe[%d] input Amp(transient) out of range: %d\n", ch, sf, pTM->SineParam[j].Amplitude);
                                        Throw SSC_ERROR_T_FREQ;
                                    }

                                    value = SSCBUNCH_GetQTransientMeixnerPhase(pSb, ch)[j];
                                    pTM->SineParam[j].Phase = (SInt)floor(value + 0.5);
                                    if ( (pTM->SineParam[j].Phase < BSC_T_PHASE_MIN) || (pTM->SineParam[j].Phase > BSC_T_PHASE_MAX) )
                                    {
                                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] Subframe[%d] input Phase(transient) out of range: %d\n", ch, sf, pTM->SineParam[j].Phase);
                                        Throw SSC_ERROR_T_FREQ;
                                    }
                                k++;
                                }
                            }
                            pTM->t_nrof_sin = k;
                            break;
                        case SSC_TRANSIENT_TYPE_STEP:
                            /* do nothing */
                            break;
                        default:
                            pT->t_type = SSC_TRANSIENT_TYPE_RESERVED0;
                            SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] Subframe[%d] illegal transient type(%d)\n", ch, sf, pT->t_type);
                            Throw SSC_ERROR_NOT_SUPPORTED;
                            break;
                    }
                }
            }
        }
    }
}


static void LOCAL_GetSortedIndexiSinusoids(SSC_STRM* pStrm, UInt sf, UInt ch, UInt maxSinus)
{
    UInt i = 0;
    UInt k = 0;
    SInt l = 0;
    UInt birth = 0;

    UInt prev = 0;
    UInt curr = 0;

    SSCBUNCH* pSb = NULL;

    SInt *pIA;

    /* Check which Index array has to be used */
    if (sf < PIPE_LENGTH - PIPE_LOOK_AHEAD)
    {
        pIA = pStrm->IndexArray[ch];
    }
    else
    {
        pIA = pStrm->IndexArrayLA[ch];
    }

    /* first get from the all valid sinus that continue in the
     * previous sf all indexi */
    pSb = SSCPIPE_GetAt(pStrm->pPipe, PIPE_LENGTH_HIST + sf - 1);
    for ( i = 0, k = 0; i < maxSinus; i++)
    {
        l = pIA[i];
        if ( l < 0 )
        {
            /* stop the loop */
            i = maxSinus;
        }
        else
        {
            if ( SSCBUNCH_GetQSinusoidTrackNext(pSb, ch)[l] >= 0 )
            {
                pIA[k] = (SInt)floor(SSCBUNCH_GetQSinusoidTrackNext(pSb, ch)[l] + 0.5) ;
                k++;
            }
        }
    }

    birth = 0;
    /* get from the current sf all birth */
    pSb = SSCPIPE_GetAt(pStrm->pPipe, PIPE_LENGTH_HIST + sf );
    for ( i = 0; i < maxSinus; i++)
    {
        if ( SSCBUNCH_GetQSinusoidTrackIdx(pSb, ch)[i] == 1 )
        {
            pIA[k + birth] = i;
            birth++;
        }
    }

    /* fill the rest with invalid index : -1 */
    for ( i = (k + birth); i < maxSinus; i++)
    {
        pIA[i] = -1;
    }

    /* sort all current births on freq if there are more than 1 birth*/
    if ( birth > 1 )
    {
        Bool swap = True;
        while ( swap == True )
        {
            swap = False;
            prev = (UInt)floor(SSCBUNCH_GetQSinusoidFreq(pSb, ch)[pIA[k]] + 0.5);
            for ( i = (k + 1); i < (k + birth); i++ )
            {
                curr = (UInt)floor(SSCBUNCH_GetQSinusoidFreq(pSb, ch)[pIA[i]] + 0.5);
                if ( curr < prev )
                {
                    SInt tmp = pIA[i];
                    pIA[i]   = pIA[i-1];
                    pIA[i-1] = tmp;
                    swap = True;
                }
                prev = curr;
            }
        }
    }

    /* Copy the array if at the end of the current frame */
    if (sf == PIPE_LENGTH - PIPE_LOOK_AHEAD - 1)
    {
        for ( i = 0; i < maxSinus; i++)
        {
            pStrm->IndexArrayLA[ch][i] = pStrm->IndexArray[ch][i];
        }
    }
}


static void LOCAL_GetSinusoids(SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context)
{

    SSCLOG*         pLog    = NULL ;
    BSC_FRAME_DATA* pData   = &(pStrm->FrameParam.frameData[0]);
    SSCPIPE*        pPipe   = pStrm->pPipe;

    SSC_SINUSOID_QPARAM* pS  = NULL;
    /* pointer to the array of sinusoids in a subframe of a channel */
    SSC_SIN_QSEGMENT*    pSP = NULL;

    SSCBUNCH*            pSb = NULL;

    UInt ch = 0;
    UInt sf = 0;
    UInt i = 0;

    SInt prevIndex = 0;
    SInt tmpNext = 0;
    SInt tmpIdx = 0;
    UInt n = 0;
    UInt k = 0;
    UInt s_count = 0;
    UInt uvalue = 0;

    Double value = 0;

    SInt indexSinus = 0;

    UInt maxSinus = (UInt)SSC_DecoderLevel[pStrm->Info.DecoderLevel].MaxNrOfSinusoids;

    pLog = pStrm->pLog;
    /* Refresh sinusoids every frame if RefreshStrategySinus == 1,
       and use a smart refresh strategy in case RefreshStrategySinus > 1 */
    if ( pStrm->Info.RefreshStrategySinus == 1 )
    {
        pStrm->FrameParam.frameHeader.RefreshSinus = 1;
    }

    /* do for all subframes */
    for ( sf = 0; sf < PIPE_LENGTH; sf++)
    {
        /* get the sbunch for this subframe*/
        pSb = SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + sf);
        /* check if there is data in this sbunch */
        if ( SSCBUNCH_GetDataFromStream(pSb) == True )
        {
            /* do for all channels */
            for (ch = 0; ch < pStrm->Info.Channels; ch++)
            {
                pS = &(pData[ch].S[PIPE_LENGTH_HIST + sf]);

                pS->s_nrof_continuations = 0;
                pS->s_nrof_births = 0;

                /* Get array of: indexi of continuations sorted on
                 * Tprev(low->high) + appended indexi of births sorted
                 * on freq(low->high)*/
                LOCAL_GetSortedIndexiSinusoids(pStrm, sf, ch, maxSinus);

                /* check for refresh frames on sinusoids in a 'smart' way for every sf + channel*/
                if (pStrm->Info.RefreshStrategySinus > 1)
                {
                    /* Check which Index array has to be used */
                    if (sf < PIPE_LENGTH - PIPE_LOOK_AHEAD)
                    {
                        indexSinus = pStrm->IndexArray[ch][0];
                    }
                    else
                    {
                        indexSinus = pStrm->IndexArrayLA[ch][0];
                    }
                    /* if a valid index */
                    if (indexSinus >= 0 )
                    {
                        /* get the length of this oldest sinus */
                        UInt maxLength = (UInt)floor(SSCBUNCH_GetQSinusoidTrackIdx(pSb, ch)[indexSinus] + 0.5);
                        /* if oldest length bigger than refresh start */
                        if ( maxLength >= (pStrm->Info.RefreshStrategySinus * SSC_MAX_SUBFRAMES) )
                        {
                            /* if current framenumber - last refresh framenumber is bigger then strategy */
                            if ( (pStrm->framesWritten - pStrm->lastSinusRefresh) > pStrm->Info.RefreshStrategySinus) 
                            {
                                pStrm->FrameParam.frameHeader.RefreshSinus = 1;
                                pStrm->lastSinusRefresh = pStrm->framesWritten;
                            }
                        }
                    }
                }

                /* for all sorted sinus positions do */
                for ( i = 0, k = 0; i < maxSinus; i++)
                {
                    /* Check which Index array has to be used */
                    if (sf < PIPE_LENGTH - PIPE_LOOK_AHEAD)
                    {
                        indexSinus = pStrm->IndexArray[ch][i];
                    }
                    else
                    {
                        indexSinus = pStrm->IndexArrayLA[ch][i];
                    }
                    /* if valid index ( -1 is invalid )*/
                    if (  indexSinus >= 0 )
                    {
                        /* get the next sinus position*/
                        pSP = &pS->SineParam[k];

                        /* ******************** */
                        value = SSCBUNCH_GetQSinusoidFreq(pSb, ch)[indexSinus];
                        pSP->Sin.Frequency = (UInt)floor(value + 0.5);
                        if ( pSP->Sin.Frequency > BSC_S_FREQ_MAX )
                        {
                            SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] SinusIndex[%d] input Freq out of range: %d\n", ch, sf, indexSinus, pSP->Sin.Frequency);
                            Throw SSC_ERROR_S_FREQ;
                        }

                        /* ******************** */
                        value = SSCBUNCH_GetQSinusoidAmp(pSb, ch)[indexSinus];
                        pSP->Sin.Amplitude = (UInt)floor(value + 0.5);
                        if ( pSP->Sin.Amplitude > BSC_S_AMP_MAX )
                        {
                            SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] SinusIndex[%d] input Amp out of range: %d\n", ch, sf, indexSinus, pSP->Sin.Amplitude);
                            Throw SSC_ERROR_S_AMP;
                        }

                        /* ******************** */
                        if ( pStrm->Info.Enhancement == 0 )
                        {
                            value = SSCBUNCH_GetQSinusoidPhaseCont(pSb, ch)[indexSinus];
                        }
                        else
                        {
                            value = SSCBUNCH_GetQSinusoidPhaseOrg(pSb, ch)[indexSinus];
                        }
                        pSP->Sin.Phase = (SInt)floor(value + 0.5);
                        if ( pSP->Sin.Phase == (BSC_S_PHASE_MAX+1) )
                        {
                            pSP->Sin.Phase = BSC_S_PHASE_MIN;
                        }
                        if ( pSP->Sin.Phase >  BSC_S_PHASE_MAX )
                        {
                            SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] SinusIndex[%d] input Phase out of range: %d\n", ch, sf, indexSinus, pSP->Sin.Phase);
                            Throw SSC_ERROR_S_PHI;
                        }

                        /* Get ADPCM grid information */
                        /* ******************** */
                        value = SSCBUNCH_GetQSinusoidQuantGrid(pSb, ch)[indexSinus];
                        uvalue = (UInt)floor(value + 0.5);
                        if (uvalue > 0)
                        {
                            pSP->s_adpcm.bGridValid   = True;
                            pSP->s_adpcm.s_adpcm_grid = uvalue - 1;
                        }
                        else
                        {
                            pSP->s_adpcm.bGridValid   = False;
                            pSP->s_adpcm.s_adpcm_grid = 0;
                        }                                       


                        /* ******************** */
                        value = SSCBUNCH_GetQSinusoidTrackPrev(pSb, ch)[indexSinus];
                        prevIndex = (SInt)floor(value + 0.5);
                        if ( prevIndex >= 0 )
                        {
                            pS->s_nrof_continuations++;

                            if ( prevIndex >= (SInt)maxSinus )
                            {
                                SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] Unable to find sinus[%d] in previous subframed\n", ch, sf, indexSinus);
                                Throw SSC_ERROR_NOT_AVAILABLE;
                            }
                            pSP->s_length = (pS-1)->SineParam[ prevIndex ].s_length + 1;

                        }
                        else
                        {
                            pS->s_nrof_births++;
                        }
                        /* the length of the track is equal to Tidx */
                        pSP->s_length = (UInt)floor(SSCBUNCH_GetQSinusoidTrackIdx(pSb, ch)[indexSinus] + 0.5);
                        if ( pSP->s_length < 1 )
                        {
                            SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] Sinus[%d] Length track(%d) is smaller then 1\n", ch, sf, indexSinus, pSP->s_length);
                            Throw SSC_ERROR_NOT_AVAILABLE;
                        }

                        /* determine s_cont */
                        pSP->s_cont = 0;
                        tmpIdx = indexSinus;
                        for ( n = sf; n < PIPE_LENGTH-1; n++ )
                        {
                            value = SSCBUNCH_GetQSinusoidTrackNext(SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + n), ch)[tmpIdx];
                            tmpNext = (SInt)floor(value + 0.5) ;
                            if ( tmpNext >= 0 )
                            {
                                pSP->s_cont++;
                                tmpIdx = tmpNext ;
                            }
                            else
                            {
                                /* stop the loop */
                                n = PIPE_LENGTH-1;
                            }
                        }
                        if ( pSP->s_cont > BSC_S_CONT_MAX )
                        {
                            SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] SinusIndex[%d] input s_cont out of range: %d\n", ch, sf, indexSinus, pSP->s_cont);
                            Throw SSC_ERROR_S_PHI;
                        }

                        k++;
                    }
                    else if ( indexSinus == -1 )
                    {
                        /* first empty index found so stop the loop */
                        i = maxSinus;

                    }
                    else
                    {
                        /* really illegal index found */
                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] SinusIndex[%d] out of range: %d\n", ch, sf, indexSinus);
                        Throw SSC_ERROR_NOT_AVAILABLE;
                    }
                }

                if ( sf < PIPE_LENGTH - 1 ) /* No check for the last look-ahead frame */
                {
                    /* check validity of continuations + births */
                    if (pS->s_nrof_continuations + pS->s_nrof_births > SSC_S_MAX_SIN_TRACKS)
                    {
                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] the number of sinusoids exceeds the decoder level limit\n", ch, sf);
                        Throw SSC_ERROR_TOO_MANY_SIN_TRACKS;
                    }

                    /* count the number of continuations in previous subframe for checking */
                    s_count = 0;
                    for ( i = 0; i < ((pS-1)->s_nrof_births + (pS-1)->s_nrof_continuations); i++)
                    {
                        if ( (pS-1)->SineParam[i].s_cont > 0 )
                        {
                            s_count++;
                        }
                    }
                    /* only check if not the first frame and not first subframe*/
                    if ( 
                        (pStrm->framesWritten != 0) && 
                        (sf != 0 ) &&
                        (pS->s_nrof_continuations != s_count) 
                       )
                    {
                        SSCLOG_TRACE5(pLog, "\nError: In Channel[%d] at Subframe[%d] continuations(%d) does not match with previous subframe continuations(%d)\n", ch, sf, pS->s_nrof_continuations, s_count);
                        Throw SSC_ERROR_WARNING;
                    }
                }
            }
        }
    }
}


static void LOCAL_GetNoise(SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context)
{

    SSCLOG*           pLog    = NULL ;
    BSC_FRAME_HEADER* pHeader = &(pStrm->FrameParam.frameHeader);
    BSC_FRAME_DATA*   pData   = &(pStrm->FrameParam.frameData[0]);
    SSCPIPE*          pPipe   = pStrm->pPipe;

    SSC_NOISE_QPARAM*   pN      = NULL;
    SSC_NOISE_QPARAM*   pNprev2 = NULL;
    SSCBUNCH*           pSb     = NULL;

    UInt ch = 0;
    UInt sf = 0;
    UInt i = 0;

    Double value = 0;
    UInt nrValues = 0;

    pLog = pStrm->pLog;
    /* adapt noise strategy */
    if ( (pStrm->Info.RefreshStrategyNoise != 0) && ((pStrm->framesWritten % pStrm->Info.RefreshStrategyNoise) == 0) )
    {
        pStrm->FrameParam.frameHeader.RefreshNoise = 1;
    }

    /* do for all subframes */
    for ( sf = 0; sf < PIPE_LENGTH; sf++)
    {
        /*get the sbunch for this subframe */
        pSb = SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + sf);
        
        /* check if there is data for this sbunch */
        if ( SSCBUNCH_GetDataFromStream(pSb) == True )
        {
            for (ch = 0; ch < pStrm->Info.Channels; ch++)
            {
                pN = &pData[ch].N[PIPE_LENGTH_HIST + sf];

                pN->n_grid_laguerre =       SSCBUNCH_GetQNoiseLaguerreGrid(pSb, ch);
                pN->n_pole_laguerre = (UInt)SSCBUNCH_GetQNoiseLaguerrePole(pSb, ch);

                /* Denominator LAR coefficients */
                nrValues = SSCBUNCH_GetQNoiseNrOfDen(pSb, ch);
                for (i = 0; i < nrValues; i++)
                {
                    value = SSCBUNCH_GetQNoiseDen(pSb, ch)[i];
                    pN->n_lar_den[i] = (SInt)floor(value + 0.5);
                    if ( (pN->n_lar_den[i] < BSC_N_DEN_MIN) || (pN->n_lar_den[i] > BSC_N_DEN_MAX) )
                    {
                        SSCLOG_TRACE5(pLog, "\nError:  In Channel[%d] at Subframe[%d] input LAR[%d] denominator out of range: %d\n", ch, sf, i, pN->n_lar_den[i]);
                        Throw SSC_ERROR_N_NROF_DEN;
                    }
                    if ( pN->n_lar_den[i] != 0 )
                    {
                        /* adjust max number of denominators for this frame if neccessary*/
                        if ( (i + 1) > pHeader->NumberDen )
                        {
                            pHeader->NumberDen = i + 1;
                        }
                    }
                }

                /* ENV Gain*/
                value = SSCBUNCH_GetQNoiseEnvGain(pSb, ch);
                pN->n_env_gain = (UInt)floor(value + 0.5) ;
                if ( pN->n_env_gain > BSC_N_ENV_GAIN_MAX )
                {
                    SSCLOG_TRACE5(pLog, "\nError:  In Channel[%d] at Subframe[%d] input ENV Gain[%] out of range: %d\n", ch, sf, i, pN->n_env_gain);
                    Throw SSC_ERROR_N_ENV_GAIN;
                }

                /* lsf overlap*/
                /* find from NOT quantised array the numbers that ovelap in current + 'previous'(-4)*/
                /* count from current sf overlap */
                if ( (sf % 4) == 0 )
                {
                    pN->n_lsf_overlap = SSCBUNCH_GetQNoiseEnvOverlap(pSb, ch);

                    nrValues = SSCBUNCH_GetQNoiseNrOfEnvCoefs(pSb, ch);
                    /* check if this values is correct by looking at previous (last) lsf coefs */
                    for ( i = 0; ((SInt)i) < pN->n_lsf_overlap; i++ )
                    {
                        SInt value1 = (SInt)SSCBUNCH_GetQNoiseEnvCoefs( SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + sf - 4), ch)[nrValues - pN->n_lsf_overlap + i]; 
                        SInt value2 = (SInt)SSCBUNCH_GetQNoiseEnvCoefs( pSb, ch)[i];
                        if ( (value1 - (3*256)/4) != value2 )
                        {
                            SSCLOG_TRACE5(pLog, "\nError:  In Channel[%d] at Subframe[%d] Lsf overlap[%]  not consistent with prev value : %d\n", ch, sf, value2, value1);
                            Throw SSC_ERROR_N_LSF;
                        }
                        else
                        {
                            pN->n_lsf[i] = value2;
                        }
                    }
                }
                else
                {
                    pN->n_lsf_overlap = 0;
                }

                /* Lsf */
                nrValues = SSCBUNCH_GetQNoiseNrOfEnvCoefs(pSb, ch);
                for ( i = pN->n_lsf_overlap; i < nrValues ; i++)
                {
                    value = SSCBUNCH_GetQNoiseEnvCoefs(pSb, ch)[i];
                    pN->n_lsf[i] = (UInt)floor(value + 0.5) ;
                    if ( pN->n_lsf[i] > BSC_N_LSF_MAX )
                    {
                        SSCLOG_TRACE5(pLog, "\nError:  In Channel[%d] at Subframe[%d] input Lsf[%]  out of range: %d\n", ch, sf, i, pN->n_lsf[i]);
                        Throw SSC_ERROR_N_LSF;
                    }
                }
            }
        }
        else
        {
            /* Clear all parameters (necessary for the last invalid subframes of a file) */
            for (ch = 0; ch < pStrm->Info.Channels; ch++)
            {
                pN      = &pData[ch].N[PIPE_LENGTH_HIST + sf];
                pNprev2 = &pData[ch].N[PIPE_LENGTH_HIST + sf - 2];


                /* Denominator LAR coefficients */
                nrValues = SSCBUNCH_GetQNoiseNrOfDen(pSb, ch);
                for (i = 0; i < nrValues; i++)
                {
                    pN->n_lar_den[i] = pNprev2->n_lar_den[i];
                }
        
                /* ENV Gain*/
                pN->n_env_gain = 0;

                /* lsf overlap*/
                pN->n_lsf_overlap = 0;

                pN->n_grid_laguerre = 0;
                pN->n_pole_laguerre = 0;

                /* Lsf */
                nrValues = SSCBUNCH_GetQNoiseNrOfEnvCoefs(pSb, ch);
                for ( i = pN->n_lsf_overlap; i < nrValues ; i++)
                {
                    pN->n_lsf[i] = (i + 1) * SSC_N_LSF_LEVELS / (nrValues + 1);
                }
            }
        }
    }
}
static void LOCAL_GetStereo( SSC_STRM* pStrm, SSC_EXCEPTION* the_exception_context)
{
    SSCLOG*               pLog    = NULL; 
    SSCPIPE*              pPipe   = pStrm->pPipe;

    SSC_STEREO_QPARAM*    pOCS = NULL;
    SSCBUNCH*             pSb  = NULL;
    
    UInt sf = 0;
    SInt i = 0;
    UInt env;

    Double value = 0;

    pLog    = pStrm->pLog;

    /* adapt stereo strategy */
    if ( (pStrm->Info.RefreshStrategyStereo != 0) && ((pStrm->framesWritten % pStrm->Info.RefreshStrategyStereo) == 0) )
    {
        pStrm->FrameParam.frameHeader.RefreshStereo = 1;
    }
    
    /* do for all subframes */
    for ( sf = 0; sf < PIPE_LENGTH; sf++)
    {
        /*get the sbunch for this subframe */
        pSb = SSCPIPE_GetAt(pPipe, PIPE_LENGTH_HIST + sf);
        
        /* check if there is data for this sbunch */
        if ( SSCBUNCH_GetDataFromStream(pSb) == True )
        {
            pOCS   = &(pStrm->FrameParam.ST[PIPE_LENGTH_HIST + sf]);

            /* Get header */
            value = SSCBUNCH_GetQStereoHeaderFlag(pSb);
            pOCS->header = (Bool)floor(value + 0.5);
            if ( (pOCS->header < False) || (pOCS->header > True) )
            {
                SSCLOG_TRACE5(pLog, "\nError:  In Subframe[%d] input header out of range: %d\n", sf, pOCS->header);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* Read IID flag from s-bunch */
            value = SSCBUNCH_GetQStereoIidFlag(pSb);
            pOCS->iidEnable = (Bool)floor(value + 0.5);
            if ( (pOCS->iidEnable < False) || (pOCS->iidEnable > True) )
            {
                SSCLOG_TRACE5(pLog, "\nError:  In Subframe[%d] input IidFlag out of range: %d\n", sf, pOCS->iidEnable);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* Read ICC flag from s-bunch */
            value = SSCBUNCH_GetQStereoIccFlag(pSb);
            pOCS->iccEnable = (Bool)floor(value + 0.5);
            if ( (pOCS->iccEnable < False) || (pOCS->iccEnable > True) )
            {
                SSCLOG_TRACE5(pLog, "\nError:  In Subframe[%d] input IccFlag out of range: %d\n", sf, pOCS->iccEnable);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* Read IPD flag from s-bunch */
            value = SSCBUNCH_GetQStereoIpdFlag(pSb);
            pOCS->ipdEnable = (Bool)floor(value + 0.5);
            if ( (pOCS->ipdEnable < False) || (pOCS->ipdEnable > True) )
            {
                SSCLOG_TRACE5(pLog, "\nError:  In Subframe[%d] input IpdFlag out of range: %d\n", sf, pOCS->ipdEnable);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* Read PCA flag from s-bunch */
            value = SSCBUNCH_GetQStereoPcaFlag(pSb);
            pOCS->pcaEnable = (Bool)floor(value + 0.5);
            if ( (pOCS->pcaEnable < False) || (pOCS->pcaEnable > True) )
            {
                SSCLOG_TRACE5(pLog, "\nError:  In Subframe[%d] input PcaFlag out of range: %d\n", sf, pOCS->pcaEnable);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* Read Ext. flag from s-bunch */
            value = SSCBUNCH_GetQStereoExtFlag(pSb);
            pOCS->extEnable = (Bool)floor(value + 0.5);
            if ( (pOCS->extEnable < False) || (pOCS->extEnable > True) )
            {
                SSCLOG_TRACE5(pLog, "\nError:  In Subframe[%d] input ExtFlag out of range: %d\n", sf, pOCS->extEnable);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* Read IID Frequency resolution from s-bunch */
            value = SSCBUNCH_GetQStereoIidFreqRes(pSb);
            pOCS->iidFreqRes = (UInt)floor(value + 0.5);

            /* Read IID Frequency resolution from s-bunch */
            value = SSCBUNCH_GetQStereoIidQuant(pSb);
            pOCS->iidQuant = (UInt)floor(value + 0.5);

            /* Read ICC Frequency resolution from s-bunch */
            value = SSCBUNCH_GetQStereoIccFreqRes(pSb);
            pOCS->iccFreqRes = (UInt)floor(value + 0.5);


            /* envelopes                */
            /* Only one envelope supported now  */
            pOCS->envelopes      = 1;

            /* envBorders               */
            pOCS->envBorder[0]    = 0;
            value                 = SSCBUNCH_GetQStereoEnvBorder(pSb);
            pOCS->envBorder[1]    = (value > 0 && value <= OCS_SLOTS)? (UInt)value : OCS_SLOTS;

            /* frameClass               */
            pOCS->frameClass      = (value > 0 && value <= OCS_SLOTS)? OCS_FRAME_VAR_BORDERS : OCS_FRAME_FIX_BORDERS;
            pOCS->frameSlotLength = OCS_SLOTS;

            /* iid                      */
            for( env = 0; env < pOCS->envelopes; env++)
            {
                for( i = 0; i < SSC_OCS_MAX_IID; i++ )
                {
                    pOCS->iid[env][i] = (SInt)SSCBUNCH_GetQStereoIid(pSb)[i];
                }
            }

            /* ipd                      */
            for( env = 0;  env < pOCS->envelopes; env++)
            {
                for( i = 0; i < SSC_OCS_MAX_IPD; i++ )
                {
                    pOCS->ipd[env][i] = (SInt)SSCBUNCH_GetQStereoIpd(pSb)[i];
                }
            }

            /* opd                      */
            for( env = 0;  env < pOCS->envelopes; env++)
            {
                for( i = 0; i < SSC_OCS_MAX_OPD; i++ )
                {
                    pOCS->opd[env][i] = (SInt)SSCBUNCH_GetQStereoOpd(pSb)[i];
                }
            }

            /* icc (rho)                */
            for( env = 0;  env < pOCS->envelopes; env++)
            {
                for( i = 0; i < SSC_OCS_MAX_ICC; i++ )
                {
                    pOCS->icc[env][i] = (SInt)SSCBUNCH_GetQStereoIcc(pSb)[i];
                }
            }
        }
    }
}
