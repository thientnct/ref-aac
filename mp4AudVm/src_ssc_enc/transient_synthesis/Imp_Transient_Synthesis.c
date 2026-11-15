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

Source file: Imp_Transient_Synthesis.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/

/******************************************************************************
 *
 *   Module              : Transient_Synthesis implementation
 *
 *   Description         : Implementation of the transient_synthesis process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_TRANSIENT_SYNTHESIS_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include "PlatformTypes.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"
#include "SSC_System.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_Transient_Synthesis.h"
#include "SSC_Transient_Synthesis.h"
#include "Imp_Error.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  0
#define PIPE_AHEAD_LENGTH 1

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_SetError
                         (
                             SSCBUNCH *pSscBunch,
                             SSC_ERROR Error
                         );

static SSC_ERROR LOCAL_TRANSIENT_SYNTHESIS_Process
                         (
                             PTRANSIENT_SYNTHESISSTATE pTSS,
                             SSCBUNCH *pSscBunch
                         );

static void LOCAL_GenerateMeixner
                         (
                             PTRANSIENT_SYNTHESISSTATE pTSS,
                             SSCBUNCH *pSscBunch,
                             SInt nCh,
                             SSC_EXCEPTION *the_exception_context
                         );

static Double LOCAL_Get_Chi
                         (
                             PTRANSIENT_SYNTHESISSTATE pTSS,
                             SInt nChiIdx,
                             SInt nBIdx,
                             SSC_EXCEPTION *the_exception_context
                         );

static Double LOCAL_Get_a_max
                         (
                             PTRANSIENT_SYNTHESISSTATE pTSS,
                             SInt nChiIdx,
                             SInt nBIdx,
                             SSC_EXCEPTION *the_exception_context
                         );

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_TRANSIENT_SYNTHESIS_Initialise(void)
{
}

/* Creates an instance of the transient_synthesis module */
PTRANSIENT_SYNTHESISSTATE IMP_TRANSIENT_SYNTHESIS_CreateInstance( PTRANSIENT_SYNTHESISINFO pTransient_SynthesisInfo, Char *szLogFile, SInt nTraceLevel )
{
    PTRANSIENT_SYNTHESISSTATE   pms = NULL ;
    SInt           nCh ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if( pTransient_SynthesisInfo == NULL )
        {
            printf( "\nError: Invalid pointer to transient_synthesis info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_TRANSIENT_SYNTHESIS_STATE), 1, "" );
        if( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for transient_synthesis instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        pms->pLog = NULL ;
        pms->pPipe = NULL ;
        for( nCh=0; nCh<SSC_MAX_CHANNELS; nCh++ )
        {
            pms->pTransBuf[nCh] = NULL ;
        }
        pms->pTransWorkBuf = NULL ;

        #ifdef LOG_ENABLE
        {
            SSCLOGINFO li ;
            Char *cp ;

            SSCLOG_INITIALISE();
            li.bFlushAfterWrite = False ;
            li.bPrintTime = False ;
            li.bPrintTitle = False ;
            li.bStdOut = True ;
            li.nTraceLevel = nTraceLevel ;
            strncpy( li.szFileName , szLogFile, sizeof(li.szFileName)/sizeof(Char) - 1 );
            li.szFileName[sizeof(li.szFileName)/sizeof(Char) - 1] = '\0' ;
            cp = strrchr( szLogFile, (int)'.' );
            if( cp )
            {
                *cp = '\0' ; /* strip extension, if found */
            }
            strncpy( li.szTitle, "SSC_transient_synthesis", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;

            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCTRANSIENT_SYNTHESIS_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( pTransient_SynthesisInfo->Channels,
                                           pTransient_SynthesisInfo->SubframeSize,
                                           pTransient_SynthesisInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCTRANSIENT_SYNTHESIS_INIT_PIPE_FAILED ;
        }

        /* module specific initialisation starts here */
        if ( ( pTransient_SynthesisInfo->Mode == SSC_MODE_LCSTEREO ) && ( pTransient_SynthesisInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->nChannels     = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->nChannels     = pTransient_SynthesisInfo->Channels ;
        }
        pms->pSSC_Chi          = pTransient_SynthesisInfo->pSSC_Chi ;
        pms->pSSC_a_max        = pTransient_SynthesisInfo->pSSC_a_max ;
        pms->nCntChi           = pTransient_SynthesisInfo->nCntChi ;
        pms->nCntB             = pTransient_SynthesisInfo->nCntB ;
        pms->nSynthesizeLength = pTransient_SynthesisInfo->nSynthesizeLength ;
        pms->nCntTransBuf      = pTransient_SynthesisInfo->nSynthesizeLength + pTransient_SynthesisInfo->SubframeSize ;

        for( nCh=0; nCh < pTransient_SynthesisInfo->Channels; nCh++ )
        {
            pms->pTransBuf[nCh] = SSC_CALLOC( pms->nCntTransBuf * sizeof(Double), 1, "" );
        }
        pms->pTransWorkBuf = SSC_CALLOC( pms->nSynthesizeLength * sizeof(Double), 1, "" );
    }
    Catch( Error )
    {
        IMP_TRANSIENT_SYNTHESIS_DestroyInstance( &pms );
    }
    return pms ;
}

/* Process current sbunch */
SSCBUNCH *IMP_TRANSIENT_SYNTHESIS_Process( PTRANSIENT_SYNTHESISSTATE pTransient_SynthesisState, SSCBUNCH *pSscWorkBunch )
{
    if( pTransient_SynthesisState )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;

        SSCPIPE_Add( pTransient_SynthesisState->pPipe, pSscWorkBunch );

        Error = LOCAL_TRANSIENT_SYNTHESIS_Process( pTransient_SynthesisState, pSscWorkBunch );

        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscWorkBunch = SSCPIPE_Shift( pTransient_SynthesisState->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_TRANSIENT_SYNTHESIS_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}

/* Destroys the transient_synthesis module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_TRANSIENT_SYNTHESIS_CreateInstance */
void IMP_TRANSIENT_SYNTHESIS_DestroyInstance( PTRANSIENT_SYNTHESISSTATE *ppTransient_SynthesisState )
{
    PTRANSIENT_SYNTHESISSTATE pTransient_SynthesisState = *ppTransient_SynthesisState ;

    if( pTransient_SynthesisState != NULL )
    {
        SInt nCh ;

        SSCLOG_TRACE4( pTransient_SynthesisState->pLog, "SSCTRANSIENT_SYNTHESIS_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( pTransient_SynthesisState->pPipe );
        if( pTransient_SynthesisState->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pTransient_SynthesisState->pLog );
            SSCLOG_FREE();
        }
        for( nCh=0; nCh < SSC_MAX_CHANNELS; nCh++ )
        {
            if( pTransient_SynthesisState->pTransBuf[nCh] != NULL )
            {
                SSC_FREE( pTransient_SynthesisState->pTransBuf[nCh], "" );
            }
        }
        if( pTransient_SynthesisState->pTransWorkBuf != NULL )
        {
            SSC_FREE( pTransient_SynthesisState->pTransWorkBuf, "" );
        }
        SSC_FREE( pTransient_SynthesisState, "" );
        *ppTransient_SynthesisState = 0 ;
    }
    else
    {
        printf( "\nError: IMP_TRANSIENT_SYNTHESIS_DestroyInstance invalid state pointer\n" );
    }
}

/* One time uninitialise function */
void IMP_TRANSIENT_SYNTHESIS_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Sets the error code in the current sbunch */
static void LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == 0 )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_TRAN_SYNTHESIS_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


static SSC_ERROR LOCAL_TRANSIENT_SYNTHESIS_Process( PTRANSIENT_SYNTHESISSTATE pTSS, SSCBUNCH *pSscBunch )
{
    SSC_ERROR Error    = SSC_ERROR_OK ;
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        SInt nCh ;
        SInt nSfSize       = pSscBunch->nSubframeSize ;
        SInt nChannels     = pTSS->nChannels ;

        /* check if data valid and start processing */
        if( SSCBUNCH_GetDataFromStream( pSscBunch ) )
        {
            /* preprocess: copy residual to signal buffer and clear the synthesize buffer */
            for( nCh = 0; nCh<nChannels; nCh++ )
            {
                SSCBUNCH_SetSignal( pSscBunch, nCh, SSCBUNCH_GetResidual( pSscBunch, nCh) );
                memset( SSCBUNCH_GetSynthesized( pSscBunch, nCh), 0, nSfSize );
            }

            for( nCh=0; nCh<nChannels; nCh++ )
            {
                if( SSCBUNCH_GetTransientPresent( pSscBunch, nCh ) )
                {
                    SInt nTranType = SSCBUNCH_GetTransientType( pSscBunch, nCh );
                    switch( nTranType )
                    {
                        case 0:     /* step transient, no synthesis required */
                            break ;
                        case 1:     /* meixner */
                            {
                                LOCAL_GenerateMeixner( pTSS, pSscBunch, nCh, the_exception_context );
                            }
                            break ;
                        default:
                            SSCLOG_TRACE5( pTSS->pLog, "Invalid transient type: %d\n", nTranType );
                            Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_T_TYPE ;
                            break;
                    }
                }
                /* copy first sf samples from pTransBuf to sbunch synthesize buffer */
                SSCBUNCH_SetSynthesized( pSscBunch, nCh, pTSS->pTransBuf[nCh] );
                {
                    /* generate the new residual */
                    SInt   i ;
                    Double *pResidual = SSCBUNCH_GetResidual( pSscBunch, nCh );
                    Double *pSignal   = SSCBUNCH_GetSignal  ( pSscBunch, nCh );
                    Double *pSynth    = pTSS->pTransBuf[nCh] ;

                    for( i=0; i<pSscBunch->nSubframeSize; i++ )
                    {
                        *pResidual = *pSignal - *pSynth ;
                        pResidual++ ;
                        pSignal++ ;
                        pSynth++ ;
                    }
                }
                /* shift the transient buffer sfsize samples */
                /* to the left and initialize the last sf samples to 0 */
                memmove( pTSS->pTransBuf[nCh], &pTSS->pTransBuf[nCh][nSfSize], pTSS->nSynthesizeLength * sizeof(Double) );
                memset (&pTSS->pTransBuf[nCh][pTSS->nCntTransBuf-nSfSize], 0, nSfSize * sizeof(Double) );
            }
        }

    }
    Catch(Error)
    {
        LOCAL_SetError( pSscBunch, Error );
    }
    return Error ;
}


static void  LOCAL_GenerateMeixner( PTRANSIENT_SYNTHESISSTATE pTSS,
                                    SSCBUNCH *pSscBunch,
                                    SInt nCh,
                                    SSC_EXCEPTION *the_exception_context )
{
	SInt i,j;
    double curr_value;
    SInt nChiIdx     = (SInt)SSCBUNCH_GetTransientChi   ( pSscBunch, nCh );
    SInt nBIdx       = (SInt)SSCBUNCH_GetTransientBpar  ( pSscBunch, nCh );
    SInt nTsinusoids = SSCBUNCH_GetTransientMeixnerSins ( pSscBunch, nCh );
    Double *pFreq    = SSCBUNCH_GetTransientMeixnerFreq ( pSscBunch, nCh );
    Double *pAmp     = SSCBUNCH_GetTransientMeixnerAmp  ( pSscBunch, nCh );
    Double *pPhase   = SSCBUNCH_GetTransientMeixnerPhase( pSscBunch, nCh );
    Double chi       = LOCAL_Get_Chi  ( pTSS, nChiIdx, nBIdx, the_exception_context );
    Double a_max     = LOCAL_Get_a_max( pTSS, nChiIdx, nBIdx, the_exception_context );
    Double b         = (Double)nBIdx + 2 ;

	/* Clear workbuffer */
	for (i=0; i<pTSS->nSynthesizeLength; i++)
	{
		pTSS->pTransWorkBuf[i]=0;
	}

	/* Generate/Sum sinusoids */

	for (j = 0; j < nTsinusoids; j++)
	{
		/* Init cos() parameters */
		double amp   = pAmp[j] ;
		double freq  = pFreq[j];
		double phase = pPhase[j];

		for (i=0; i<pTSS->nSynthesizeLength; i++)
		{
            pTSS->pTransWorkBuf[i] += amp * cos( freq * i + phase );
		}
	}

	/* Generate envelope for transient(s) */

	/* Init first value */
	curr_value = pow ((1 - chi*chi), 0.5*b) / a_max;

	/* Envelope calculated */
	for (i=0; i<pTSS->nSynthesizeLength; i++)
	{
		pTSS->pTransWorkBuf[i] *= curr_value;
		curr_value             *= chi * sqrt((b+i)/(i+1));
	}
    /* Transient has been calculated in the workbuffer. */
    /* Now copy the data to the output buffer at the correct (transient) position */
    {
        Double *pDst = &pTSS->pTransBuf[nCh][SSCBUNCH_GetTransientPos( pSscBunch, nCh ) ] ; /* transientpos now zero based (handled by interface) */
        for( i=0; i<pTSS->nSynthesizeLength; i++, pDst++ )
        {
            *pDst += pTSS->pTransWorkBuf[i] ;
        }
    }
}


static Double LOCAL_Get_Chi(  PTRANSIENT_SYNTHESISSTATE pTSS,
                        SInt nChiIdx,
                        SInt nBIdx,
                        SSC_EXCEPTION *the_exception_context )
{
    if( (nChiIdx < 0) || (nChiIdx >= pTSS->nCntChi) )
    {
        SSCLOG_TRACE5( pTSS->pLog, "Get_Chi: Invalid Chi index value: %d\n", nChiIdx );
        Throw SSC_ERROR_NOT_SUPPORTED ;
    }
    if( (nBIdx < 0) || (nBIdx >= pTSS->nCntB) )
    {
        SSCLOG_TRACE5( pTSS->pLog, "Get_Chi: Invalid B index value: %d\n", nBIdx );
        Throw SSC_ERROR_NOT_SUPPORTED ;
    }
    return pTSS->pSSC_Chi[ (nChiIdx * pTSS->nCntChi) + nBIdx ] ;
}


static Double LOCAL_Get_a_max(  PTRANSIENT_SYNTHESISSTATE pTSS,
                          SInt nChiIdx,
                          SInt nBIdx,
                          SSC_EXCEPTION *the_exception_context )
{
    if( (nChiIdx < 0) || (nChiIdx >= pTSS->nCntChi) )
    {
        SSCLOG_TRACE5( pTSS->pLog, "Get_a_max: Invalid Chi index value: %d\n", nChiIdx );
        Throw SSC_ERROR_NOT_SUPPORTED ;
    }
    if( (nBIdx < 0) || (nBIdx >= pTSS->nCntB) )
    {
        SSCLOG_TRACE5( pTSS->pLog, "Get_a_max: Invalid B index value: %d\n", nBIdx );
        Throw SSC_ERROR_NOT_SUPPORTED ;
    }
    return pTSS->pSSC_a_max[ (nChiIdx * pTSS->nCntChi) + nBIdx ] ;
}


