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

Source file: IMP_transient_analysis.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
11-Oct-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/



/******************************************************************************
 *
 *   Module              : transient_analysis implementation
 *
 *   Description         : Implementation of the transient_analysis process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_TRANSIENT_ANALYSIS_
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

#include "Imp_transient_analysis.h"
#include "SSC_transient_analysis.h"
#include "Imp_Error.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  0
#define PIPE_AHEAD_LENGTH 3

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );
static void LOCAL_Transient_Analysis_Process
                                  ( PTRANSIENT_ANALYSISSTATE pTAS );
static void GetEnvelopeWorkBuffer ( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh, SInt nPos );
static void GetSignalWorkBuffer   ( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh, SInt nPos );
static void Fit_Meixner           ( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh );
static void Find_Meixner_Sinusoids( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh, SInt nPos );
static __inline SInt LOCAL_Min( SInt S1, SInt S2 );
static void GenerateMeixner( Double *pMeixner, SInt Length, Double Chi, Double b, Double a_max );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_TRANSIENT_ANALYSIS_Initialise(void)
{
}

/* Creates an instance of the transient_analysis module */
PTRANSIENT_ANALYSISSTATE IMP_TRANSIENT_ANALYSIS_CreateInstance( PTRANSIENT_ANALYSISINFO ptransient_analysisInfo, Char *szLogFile, SInt nTraceLevel )
{
    PTRANSIENT_ANALYSISSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( ptransient_analysisInfo == NULL )
        {
            printf( "\nError: Invalid pointer to transient_analysis info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_TRANSIENT_ANALYSISSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for transient_analysis instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        pms->pLog = NULL ;
        pms->pPipe = NULL ;

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
            strncpy( li.szTitle, "SSC_transient_analysis", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCTRANSIENT_ANALYSIS_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( ptransient_analysisInfo->Channels,
                                           ptransient_analysisInfo->SubframeSize,
                                           ptransient_analysisInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCTRANSIENT_ANALYSIS_INIT_PIPE_FAILED ;
        }

        if ( ( ptransient_analysisInfo->Mode == SSC_MODE_LCSTEREO ) && ( ptransient_analysisInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels       = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels       = ptransient_analysisInfo->Channels ;
        }
        pms->fs                 = ptransient_analysisInfo->fs ;
        pms->SubframeSize       = ptransient_analysisInfo->SubframeSize ;
        pms->Tran_Ana_Length    = ptransient_analysisInfo->Tran_Ana_Length ;
        pms->meixner_sins       = ptransient_analysisInfo->meixner_sins ;
        pms->meixner_ana_offset = ptransient_analysisInfo->meixner_ana_offset ;
        pms->meixner_ana_length = (SInt)pow( 2, (SInt)(log( pms->Tran_Ana_Length-pms->meixner_ana_offset ) / log(2)) );
        pms->pEnvPipe           = ENVPIPE_CreateInstance( pms->Channels, pms->SubframeSize,
                                                          ptransient_analysisInfo->MinEnvelope,
                                                          ptransient_analysisInfo->EnvelopeDecay,
                                                          PIPE_AHEAD_LENGTH );
        if ( pms->pEnvPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested envelope pipe with lenght:%d\n", 3);
            Throw SSCTRANSIENT_ANALYSIS_INIT_PIPE_FAILED ;
        }
        pms->pdWorkBuf = SSC_CALLOC( pms->Tran_Ana_Length * sizeof(Double), 1, "" );
        if ( pms->pdWorkBuf == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to allocate workbuffer\n" );
            Throw SSC_ERROR_OUT_OF_MEMORY; ;
        }
        pms->pdMeixner = SSC_CALLOC( pms->Tran_Ana_Length * sizeof(Double), 1, "" );
        if ( pms->pdMeixner == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to allocate meixner buffer\n" );
            Throw SSC_ERROR_OUT_OF_MEMORY; ;
        }
        pms->pHiresFft = SSC_HIRES_FFT_CreateInstance( pms->meixner_ana_length, pms->meixner_sins,
                                                                pms->fs, pms->fs/2 );
        if ( pms->pHiresFft == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Failed to initialize hires fft instance\n" );
            Throw SSC_ERROR_OUT_OF_MEMORY; ;
        }
    }
    Catch( Error )
    {
        IMP_TRANSIENT_ANALYSIS_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_TRANSIENT_ANALYSIS_Process( PTRANSIENT_ANALYSISSTATE ptransient_analysisState, SSCBUNCH* pSscWorkBunch )
{
    if ( ptransient_analysisState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( ptransient_analysisState->pPipe, pSscWorkBunch );

        LOCAL_Transient_Analysis_Process( ptransient_analysisState );

        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscWorkBunch = SSCPIPE_Shift( ptransient_analysisState->pPipe );
        ENVPIPE_Shift( ptransient_analysisState->pEnvPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_TRANSIENT_ANALYSIS_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the transient_analysis module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_TRANSIENT_ANALYSIS_CreateInstance */
void IMP_TRANSIENT_ANALYSIS_DestroyInstance( PTRANSIENT_ANALYSISSTATE* pptransient_analysisState )
{
    PTRANSIENT_ANALYSISSTATE ptransient_analysisState = *pptransient_analysisState ;

    if( ptransient_analysisState != NULL )
    {
        SSCLOG_TRACE4( ptransient_analysisState->pLog, "SSCTRANSIENT_ANALYSIS_DestroyInstance\n" );

        SSC_FREE( ptransient_analysisState->pdWorkBuf, "" );
        SSC_FREE( ptransient_analysisState->pdMeixner, "" );

        SSC_HIRES_FFT_DestroyInstance( &ptransient_analysisState->pHiresFft );
        SSCPIPE_DestroyInstance  ( ptransient_analysisState->pPipe );
        ENVPIPE_DestroyInstance  ( ptransient_analysisState->pEnvPipe );

        if( ptransient_analysisState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( ptransient_analysisState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( ptransient_analysisState, "" );
        *pptransient_analysisState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_TRANSIENT_ANALYSIS_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_TRANSIENT_ANALYSIS_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Sets the error code in the current sbunch */
static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == 0 )
    {
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_TRAN_ANALYSIS_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}

static void LOCAL_Transient_Analysis_Process( PTRANSIENT_ANALYSISSTATE pTAS )
{
    SInt      nCh ;
    SSCPIPE  *pPipe      = pTAS->pPipe ;
    ENVPIPE  *pEnvPipe   = pTAS->pEnvPipe ;
    SSCBUNCH *pCurrBunch = SSCPIPE_GetAt( pPipe, PIPE_AHEAD_LENGTH-1 );
    SSCBUNCH *pCalcBunch = SSCPIPE_GetAt( pPipe, 0 );

    /* copy data from residual to signal and calculate envelope */
    for( nCh = 0; nCh < pTAS->Channels; nCh++ )
    {
        SSCBUNCH_SetSignal  ( pCurrBunch, nCh, SSCBUNCH_GetResidual( pCurrBunch, nCh )) ;
        ENVPIPE_CalcEnvelope( pEnvPipe,   nCh, SSCBUNCH_GetSignal  ( pCurrBunch, nCh ));
    }
    if( SSCBUNCH_GetDataFromStream( pCalcBunch ) == 1 )
    {
        for( nCh = 0; nCh < pTAS->Channels; nCh++ )
        {
            if( SSCBUNCH_GetTransientPresent( pCalcBunch, nCh ) != 0 )
            {
                SInt nPos = SSCBUNCH_GetTransientPos( pCalcBunch, nCh );

                /* copy envelope around transient to workbuffer */
                GetEnvelopeWorkBuffer( pTAS, nCh, nPos );
                Fit_Meixner( pTAS, nCh );
                if( SSC_TRANSIENT_TYPE_MEIXNER == SSCBUNCH_GetTransientType( pCalcBunch, nCh ) )
                {
                    Find_Meixner_Sinusoids( pTAS, nCh, nPos );
                }
            }
        }
    }
}

static void GetEnvelopeWorkBuffer( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh, SInt nPos )
{
    ENVPIPE *pEnvPipe = pTAS->pEnvPipe ;
    SInt nPipeIdx     = 0 ;
    SInt nLengthToGo  = pTAS->Tran_Ana_Length ;
    SInt nDstIdx      = 0 ;
    SInt nCopyLength  = LOCAL_Min( pTAS->SubframeSize - nPos, nLengthToGo ) ;

    memcpy( &pTAS->pdWorkBuf[nDstIdx],
            &pEnvPipe->ppEnvBunch[nPipeIdx]->pdEnvelope[nCh*pTAS->SubframeSize+nPos],
            nCopyLength * sizeof(Double) );
    nPipeIdx++ ;
    nDstIdx     += nCopyLength ;
    nLengthToGo -= nCopyLength ;
    nCopyLength  = LOCAL_Min( pTAS->SubframeSize, nLengthToGo );

    memcpy( &pTAS->pdWorkBuf[nDstIdx],
            &pEnvPipe->ppEnvBunch[nPipeIdx]->pdEnvelope[nCh*pTAS->SubframeSize+0],
            nCopyLength * sizeof(Double) );
    nPipeIdx++ ;
    nDstIdx     += nCopyLength ;
    nLengthToGo -= nCopyLength ;
    nCopyLength  = LOCAL_Min( pTAS->SubframeSize, nLengthToGo );

    memcpy( &pTAS->pdWorkBuf[nDstIdx],
            &pEnvPipe->ppEnvBunch[nPipeIdx]->pdEnvelope[nCh*pTAS->SubframeSize+0],
            nCopyLength * sizeof(Double) );
}

static void GetSignalWorkBuffer( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh, SInt nPos )
{
    SSCPIPE *pPipe   = pTAS->pPipe ;
    SInt nPipeIdx    = 0 ;
    Double  *pSignal = SSCBUNCH_GetSignal( SSCPIPE_GetAt( pPipe, nPipeIdx ), nCh );
    SInt nLengthToGo = pTAS->Tran_Ana_Length ;
    SInt nDstIdx     = 0 ;
    SInt nCopyLength = LOCAL_Min( pTAS->SubframeSize - nPos, nLengthToGo ) ;

    memcpy( &pTAS->pdWorkBuf[nDstIdx], &pSignal[nPos], nCopyLength * sizeof(Double) );
    nPipeIdx++ ;
    nDstIdx     += nCopyLength ;
    nLengthToGo -= nCopyLength ;
    nCopyLength  = LOCAL_Min( pTAS->SubframeSize, nLengthToGo );
    pSignal      = SSCBUNCH_GetSignal( SSCPIPE_GetAt( pPipe, nPipeIdx ), nCh );

    memcpy( &pTAS->pdWorkBuf[nDstIdx], &pSignal[0], nCopyLength * sizeof(Double) );
    nPipeIdx++ ;
    nDstIdx     += nCopyLength ;
    nLengthToGo -= nCopyLength ;
    nCopyLength  = LOCAL_Min( pTAS->SubframeSize, nLengthToGo );
    pSignal = SSCBUNCH_GetSignal( SSCPIPE_GetAt( pPipe, nPipeIdx ), nCh );

    memcpy( &pTAS->pdWorkBuf[nDstIdx], &pSignal[0], nCopyLength * sizeof(Double) );
}

static void Fit_Meixner( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh )
{
    SInt      i, b, b_rl, chi_rl, idxMin_b, idxMin_chi ;
    Double    dChi, dA_max, dErr, dMinErr ;
    SInt      meixner_len = pTAS->Tran_Ana_Length ;
    Double   *pMeixner    = pTAS->pdMeixner ;
    Double    max_sqr_err = 0.05 * meixner_len ;
    Double   *pSignal     = pTAS->pdWorkBuf ;
    Double    dMaxSignal  = pSignal[0] ;
    SSCBUNCH *pSscBunch   = SSCPIPE_GetAt( pTAS->pPipe, 0 );
    Double    sqr_err[SSC_T_MEIXNER_NROF_CHIS][SSC_T_MEIXNER_NROF_BS];

    for( i=0; i<meixner_len; i++ )
    {
        if( pSignal[i] > dMaxSignal )
        {
            dMaxSignal = pSignal[i] ;
        }
    }
    if( dMaxSignal != 0 )
    {
        for( i=0; i<meixner_len; i++ )
        {
            pSignal[i] /= dMaxSignal ;
        }
    }
    for( b_rl = 0; b_rl < SSC_T_MEIXNER_NROF_BS; b_rl++ )
    {
        b = b_rl + 2 ;
        for( chi_rl = 0; chi_rl < SSC_T_MEIXNER_NROF_CHIS; chi_rl++ )
        {
            dChi   = SSC_Chi  [chi_rl][b_rl];
            dA_max = SSC_a_max[chi_rl][b_rl];

            GenerateMeixner( pMeixner, meixner_len, dChi, b, dA_max );
            dErr = 0 ;
            for( i=0; i<meixner_len; i++ )
            {
                dErr += ((pMeixner[i] - pSignal[i]) * (pMeixner[i] - pSignal[i]) );
            }
            sqr_err[chi_rl][b_rl] = dErr ;
        }
    }
    idxMin_b   = 0 ;
    idxMin_chi = 0 ;
    dMinErr = sqr_err[0][0] ;
    for( b_rl = 0; b_rl < SSC_T_MEIXNER_NROF_BS; b_rl++ )
    {
        for( chi_rl = 0; chi_rl < SSC_T_MEIXNER_NROF_CHIS; chi_rl++ )
        {
            if( sqr_err[chi_rl][b_rl] < dMinErr )
            {
                dMinErr    = sqr_err[chi_rl][b_rl];
                idxMin_b   = b_rl ;
                idxMin_chi = chi_rl ;
            }
        }
    }
    if( dMinErr < max_sqr_err )
    {
        SSCBUNCH_SetTransientType( pSscBunch, nCh, SSC_TRANSIENT_TYPE_MEIXNER );
        SSCBUNCH_SetTransientChi ( pSscBunch, nCh, idxMin_chi );
        SSCBUNCH_SetTransientBpar( pSscBunch, nCh, idxMin_b );
    }
}

static void Find_Meixner_Sinusoids( PTRANSIENT_ANALYSISSTATE pTAS, SInt nCh, SInt nPos )
{
    SInt      i ;
    SInt      nr_of_sins      = 0 ;
    SSCBUNCH *pSscBunch       = SSCPIPE_GetAt( pTAS->pPipe, 0 );
    SInt      analysis_offset = pTAS->meixner_ana_offset ;
    SInt      analysis_length = pTAS->meixner_ana_length ;
    SInt      b_rl            = (SInt)SSCBUNCH_GetTransientBpar( pSscBunch, nCh );
    SInt      chi_rl          = (SInt)SSCBUNCH_GetTransientChi ( pSscBunch, nCh );
    Double    b               = b_rl + 2.0 ;
    Double    chi             = SSC_Chi  [chi_rl][b_rl] ;
    Double    a_max           = SSC_a_max[chi_rl][b_rl] ;
    SInt      meixner_length  = pTAS->Tran_Ana_Length/3 ;
    Double   *pSignal         = pTAS->pdWorkBuf ;
    Double   *pdMeixner       = pTAS->pdMeixner ;
    Double   *pMeixnerFreq    = SSCBUNCH_GetTransientMeixnerFreq ( pSscBunch, nCh );
    Double   *pMeixnerAmp     = SSCBUNCH_GetTransientMeixnerAmp  ( pSscBunch, nCh );
    Double   *pMeixnerPhase   = SSCBUNCH_GetTransientMeixnerPhase( pSscBunch, nCh );

    /* copy signal around transient to pdWorkBuf ( == pSignal ) */
    GetSignalWorkBuffer( pTAS, nCh, nPos );
    /* pdMeixner will be used as window by regression */
    GenerateMeixner( pTAS->pdMeixner, meixner_length, chi, b, a_max );

    for( i=analysis_offset; i<analysis_length; i++ )
    {
        pSignal[i] /= pdMeixner[i] ;
    }
    SSC_HIRES_FFT_Process( pTAS->pHiresFft, &pSignal[analysis_offset],
                           pMeixnerFreq, pMeixnerAmp, pMeixnerPhase ) ;

    for( i=0; i<pTAS->meixner_sins; i++ )
    {
        if( pMeixnerAmp[i] > 0 )
        {
            nr_of_sins++ ;
            if( pMeixnerAmp[i] > 32767 )
            {
                pMeixnerAmp[i] = 32767 ;
            }
        }
    }
    if( nr_of_sins > 0 )
    {
        SSCBUNCH_SetTransientMeixnerSins( pSscBunch, nCh, nr_of_sins );
    }
    else
    {
        SSCBUNCH_SetTransientType( pSscBunch, nCh, SSC_TRANSIENT_TYPE_STEP );
    }
}

static void GenerateMeixner( Double *pMeixner, SInt Length, Double Chi, Double b, Double a_max )
{
	SInt   i;
	double curr_value;

	/* Generate envelope for transient(s) */

	/* Init first value */
	curr_value = pow ((1 - Chi*Chi), 0.5*b) / a_max;

	/* Envelope calculated */
	for (i=0; i<Length; i++)
	{
		pMeixner[i] = curr_value;
		curr_value *= Chi * sqrt((b+i)/(i+1));
	}
}

static __inline SInt LOCAL_Min( SInt S1, SInt S2 )
{
    if( S1 < S2 )
    {
        return S1 ;
    }
    else
    {
        return S2 ;
    }
}

