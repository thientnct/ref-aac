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

Source file: Imp_sinusoid_quantisation.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:     Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
20-Sep-2002 NT      RM2 change
21-Nov-2003 AG      Changes for RM4
************************************************************************/
/******************************************************************************
 *
 *   Module              : sinusoid_quantisation implementation
 *
 *   Description         : Implementation of the sinusoid_quantisation process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_QUANTISATION_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include <assert.h>
#include "PlatformTypes.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"
#include "SSC_System.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_sinusoid_quantisation.h"
#include "SSC_sinusoid_quantisation.h"
#include "Imp_Error.h"

#include "cexcept.h"

#include "Imp_quantise.h"

#include "SSC_BitStream.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  1
#define PIPE_AHEAD_LENGTH 1
#define PIPE_LENGTH       (PIPE_HIST_LENGTH + PIPE_AHEAD_LENGTH)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

extern const Double SSC_ADPCM_TableRepr[];
extern const Double SSC_ADPCM_TableQuant[];

extern const Double SSC_ADPCM_FF[]; 
extern const Double SSC_ADPCM_FS[];
extern const Double SSC_ADPCM_Q[]; 
extern const Double SSC_ADPCM_R[];

extern const Double SSC_ADPCM_delta_min;
extern const Double SSC_ADPCM_delta_max;

extern const Double SSC_ADPCM_P_h[];
extern const Double SSC_ADPCM_P_m[];    


/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void      LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );
static SSC_ERROR LOCAL_Sinusoid_Quantisation_Process
( 
    PSINUSOID_QUANTISATIONSTATE pSQS 
);

static PSSC_ADPCM_STATE LOCAL_CreateADPCMState( Double SfSize, UInt NumSin, UInt NumLevels, UInt PredMem );
static void LOCAL_DestroyADPCMState( PSSC_ADPCM_STATE *s_adpcm, UInt NumSin );
static void LOCAL_CopyADPCMState( PSSC_ADPCM_STATE s_adpcm_out, SSC_ADPCM_STATE s_adpcm_in, UInt NumLevels, UInt PredMem);

static void LOCAL_update_delta(
        Double * delta,
        Double * repr,
        Double C_next,
        const Double delta_max,
        const Double delta_min,
        const Double * m
);
static SInt LOCAL_quantiseADPCM
( 
    PSSC_ADPCM_STATE s_adpcm, 
    SInt             TrackIdx,
    Double           Freq, 
    Double           QuantFreq, 
    Double           Phase, 
    Double           QuantPhase,
    Double           PrevFreq, 
    Double           PrevPhase
);


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_SINUSOID_QUANTISATION_Initialise(void)
{
}


/* Creates an instance of the sinusoid_quantisation module */
PSINUSOID_QUANTISATIONSTATE IMP_SINUSOID_QUANTISATION_CreateInstance( PSINUSOID_QUANTISATIONINFO psinusoid_quantisationInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_QUANTISATIONSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( psinusoid_quantisationInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_quantisation info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_QUANTISATIONSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_quantisation instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_quantisation", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_QUANTISATION_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_quantisationInfo->Channels,
                                           psinusoid_quantisationInfo->SubframeSize,
                                           psinusoid_quantisationInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSINUSOID_QUANTISATION_INIT_PIPE_FAILED ;
        }
        pms->dFreq_rl_base  = psinusoid_quantisationInfo->dFreq_rl_base ;
        pms->dAmp_rl_base   = psinusoid_quantisationInfo->dAmp_rl_base ;
        pms->dPhase_rl_base = psinusoid_quantisationInfo->dPhase_rl_base ;
        pms->dFs            = psinusoid_quantisationInfo->dFs ;
        if ( ( psinusoid_quantisationInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_quantisationInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->nChannels  = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->nChannels  = psinusoid_quantisationInfo->Channels ;
        }

        /* Only 2 bit Phase ADPCM is supported */
        if (2 != psinusoid_quantisationInfo->dNumADPCMBits)
        {
            printf( "\nError: Invalid number of bits for phase ADPCM: %d\n", psinusoid_quantisationInfo->dNumADPCMBits);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        else
        {
            UInt NumLevels = 4;  /* Number of levels in quantiser */
            UInt PredMem   = 2;  /* Size of prediction filter memory */
            SInt nCh;

            pms->dNumLevels  = NumLevels;
            pms->dPredMem    = PredMem;
            for (nCh = 0; nCh < pms->nChannels; nCh++)
            {
                pms->s_adpcm[nCh]     = LOCAL_CreateADPCMState( psinusoid_quantisationInfo->dT, SSC_ENCODER_MAX_SINS, NumLevels, PredMem );
                pms->s_adpcm_tmp[nCh] = LOCAL_CreateADPCMState( psinusoid_quantisationInfo->dT, SSC_ENCODER_MAX_SINS, NumLevels, PredMem );
            }
        }
    }
    Catch( Error )
    {
        IMP_SINUSOID_QUANTISATION_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_QUANTISATION_Process( PSINUSOID_QUANTISATIONSTATE pSQS, SSCBUNCH* pSscBunch )
{
    if ( pSQS != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( pSQS->pPipe, pSscBunch );

        /* Quantise the sinusoids */
        Error = LOCAL_Sinusoid_Quantisation_Process( pSQS );

        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscBunch = SSCPIPE_Shift( pSQS->pPipe );

        /* report error immidiate */
        LOCAL_SetError( pSscBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_QUANTISATION_Process invalid state pointer\n" );
    }
    return pSscBunch ;
}


/* Destroys the sinusoid_quantisation module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_SINUSOID_QUANTISATION_CreateInstance */

void IMP_SINUSOID_QUANTISATION_DestroyInstance( PSINUSOID_QUANTISATIONSTATE* ppsinusoid_quantisationState )
{
    PSINUSOID_QUANTISATIONSTATE psinusoid_quantisationState = *ppsinusoid_quantisationState ;

    if( psinusoid_quantisationState != NULL )
    {
        SInt nCh;
        
        SSCLOG_TRACE4( psinusoid_quantisationState->pLog, "SSCSINUSOID_QUANTISATION_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( psinusoid_quantisationState->pPipe );
        if( psinusoid_quantisationState->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( psinusoid_quantisationState->pLog );
            SSCLOG_FREE();
        }
        for (nCh = 0; nCh < psinusoid_quantisationState->nChannels; nCh++)
        {
            LOCAL_DestroyADPCMState( &(psinusoid_quantisationState->s_adpcm[nCh]), SSC_ENCODER_MAX_SINS );
            LOCAL_DestroyADPCMState( &(psinusoid_quantisationState->s_adpcm_tmp[nCh]), SSC_ENCODER_MAX_SINS );
        }
        SSC_FREE( psinusoid_quantisationState, "" );
        *ppsinusoid_quantisationState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_QUANTISATION_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_SINUSOID_QUANTISATION_Free(void)
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
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_QUANTISATION_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}



static SSC_ERROR LOCAL_Sinusoid_Quantisation_Process
( 
    PSINUSOID_QUANTISATIONSTATE pSQS 
)
{
    SInt      nCh, nSine ;
    SSC_ERROR Error = SSC_ERROR_OK ;
    SSCBUNCH      *pPrevBunch            = SSCPIPE_GetAt( pSQS->pPipe, PIPE_LENGTH-2 );
    SSCBUNCH      *pCurrBunch            = SSCPIPE_GetAt( pSQS->pPipe, PIPE_LENGTH-1 );

    if( SSCBUNCH_GetDataFromStream( pCurrBunch ) == 1 )
    {
        for( nCh = 0; nCh < pSQS->nChannels; nCh++ )
        {
            Double *pTrackIdx  = SSCBUNCH_GetSinusoidTrackIdx ( pCurrBunch, nCh );
            Double *pTrackPrev = SSCBUNCH_GetSinusoidTrackPrev( pCurrBunch, nCh );
            Double *pFrequency = SSCBUNCH_GetSinusoidFreq     ( pCurrBunch, nCh );
            Double *pAmplitude = SSCBUNCH_GetSinusoidAmp      ( pCurrBunch, nCh );
            Double *pPhaseOrg  = SSCBUNCH_GetSinusoidPhaseOrg ( pCurrBunch, nCh );
            SInt   grid_freq   = SSCBUNCH_GetSinusoidFreqGrid ( pCurrBunch, nCh );
            SInt   grid_amp    = SSCBUNCH_GetSinusoidAmpGrid  ( pCurrBunch, nCh );

            Double *pPrevTrackIdx  = SSCBUNCH_GetSinusoidTrackIdx ( pPrevBunch, nCh );
            Double *pPrevFrequency = SSCBUNCH_GetSinusoidFreq     ( pPrevBunch, nCh );
            Double *pPrevPhaseOrg  = SSCBUNCH_GetSinusoidPhaseOrg ( pPrevBunch, nCh );

            Double pQFrequency[SSC_ENCODER_MAX_SINS] ;
            Double pQAmplitude[SSC_ENCODER_MAX_SINS] ;
            Double pQPhaseOrg [SSC_ENCODER_MAX_SINS] ;

            PSSC_ADPCM_STATE s_adpcm_in  = pSQS->s_adpcm[nCh];
            PSSC_ADPCM_STATE s_adpcm_out = pSQS->s_adpcm_tmp[nCh];

            for( nSine = 0; nSine < SSC_ENCODER_MAX_SINS ; nSine++ )
            {
                if( (SInt)pTrackIdx[nSine] != 0 )
                {
                    Double dFreq, dPhase;
                    Double qFreq, qPhase;
                    Double prevFreq, prevPhase;
                    SInt EncoderOut;

                    pQAmplitude[nSine] = quantise_amp  ( pAmplitude[nSine], pSQS->dAmp_rl_base, grid_amp );

                    /* Quantise/dequantise frequency */
                    qFreq = quantise_freq ( pFrequency[nSine], pSQS->dFreq_rl_base, pSQS->dFs, grid_freq );
                    dFreq = dequantise_freq( (SInt)qFreq, pSQS->dFreq_rl_base, pSQS->dFs );

                    /* Quantise/dequantise phase */
                    qPhase = quantise_phase( pPhaseOrg [nSine], pSQS->dPhase_rl_base );
                    dPhase = dequantise_phase( (SInt)qPhase, pSQS->dPhase_rl_base );
                 
                    /* Copy ADPCM state from previous sub-frame */
                    if( (SInt)pTrackIdx[nSine] > 1 )
                    {
                        SInt PrevIdx = (SInt)(pTrackPrev[nSine]);
                        /* For debugging */
                        UNREFERENCED_PARM(pPrevTrackIdx[PrevIdx]);
                        assert (pPrevTrackIdx[PrevIdx] != 0);

                        prevFreq  = pPrevFrequency[PrevIdx];
                        prevPhase = pPrevPhaseOrg[PrevIdx];

                        LOCAL_CopyADPCMState(&(s_adpcm_out[nSine]), s_adpcm_in[PrevIdx], pSQS->dNumLevels, pSQS->dPredMem);
                    }
                    else
                    {
                        /* No previous frequencies for births */
                        prevFreq  = 0.0;
                        prevPhase = 0.0;
                    }
                
                    /* Quantise using ADPCM quantiser */
                    EncoderOut = LOCAL_quantiseADPCM( 
                                    &(s_adpcm_out[nSine]),
                                    (SInt)pTrackIdx[nSine],
                                    pFrequency[nSine], dFreq, 
                                    pPhaseOrg[nSine], dPhase,
                                    prevFreq, prevPhase);

                    /* Store output phase ADPCM for continuations */
                    if( (SInt)pTrackIdx[nSine] > 1 )
                    {
                        pQFrequency[nSine] = (Double)EncoderOut;
                        pQPhaseOrg [nSine] = 0.0;
                    }
                    else
                    {
                        pQFrequency[nSine] = qFreq;
                        pQPhaseOrg [nSine] = qPhase; 
                    }   
                }
                else
                {
                    pQFrequency[nSine] = 0.0 ;
                    pQAmplitude[nSine] = 0.0 ;
                    pQPhaseOrg [nSine] = 0.0 ;
                }
            }

            /* Copy ADPCM output states as input to next states */
            for( nSine = 0; nSine < SSC_ENCODER_MAX_SINS ; nSine++ )
            {
                 LOCAL_CopyADPCMState(&(s_adpcm_in[nSine]), s_adpcm_out[nSine], pSQS->dNumLevels, pSQS->dPredMem);
            }

            SSCBUNCH_SetQSinusoidFreq          ( pCurrBunch, nCh, pQFrequency );
            SSCBUNCH_SetQSinusoidAmp           ( pCurrBunch, nCh, pQAmplitude );
            SSCBUNCH_SetQSinusoidPhaseOrg      ( pCurrBunch, nCh, pQPhaseOrg  );

            SSCBUNCH_SetQSinusoidTrackIdx      ( pCurrBunch, nCh, pTrackIdx );
            SSCBUNCH_SetQSinusoidTrackPrev     ( pCurrBunch, nCh, SSCBUNCH_GetSinusoidTrackPrev     ( pCurrBunch, nCh ));
            SSCBUNCH_SetQSinusoidTrackNext     ( pCurrBunch, nCh, SSCBUNCH_GetSinusoidTrackNext     ( pCurrBunch, nCh ));
            SSCBUNCH_SetQSinusoidPhaseCont     ( pCurrBunch, nCh, SSCBUNCH_GetSinusoidPhaseCont     ( pCurrBunch, nCh ));
            SSCBUNCH_SetQSinusoidRelevance     ( pCurrBunch, nCh, SSCBUNCH_GetSinusoidRelevance     ( pCurrBunch, nCh ));
            SSCBUNCH_SetQSinusoidFreqGrid      ( pCurrBunch, nCh, SSCBUNCH_GetSinusoidFreqGrid      ( pCurrBunch, nCh ));
            SSCBUNCH_SetQSinusoidAmpGrid       ( pCurrBunch, nCh, SSCBUNCH_GetSinusoidAmpGrid       ( pCurrBunch, nCh ));

            SSCBUNCH_SetQSinusoidRelevanceThreshold( pCurrBunch, nCh, SSCBUNCH_GetSinusoidRelevanceThreshold( pCurrBunch, nCh));

        }
    }
    return Error;
}

static PSSC_ADPCM_STATE LOCAL_CreateADPCMState( Double SfSize, UInt NumSin, UInt NumLevels, UInt PredMem )
{
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    PSSC_ADPCM_STATE s_adpcm = NULL;
    UInt k;

    Try
    {
        s_adpcm = SSC_CALLOC( NumSin, sizeof( *s_adpcm ), "" );
        if ( s_adpcm == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_quantisation instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        for (k=0; k < NumSin; k++)
        {
            s_adpcm[k].dT              = SfSize ;
            s_adpcm[k].dM              = 0;
            s_adpcm[k].dUnwrappedPhase = 0.0;

            /* ADPCM memory */
            s_adpcm[k].pDelta = SSC_CALLOC( NumLevels + 1, sizeof( *s_adpcm[k].pDelta ), "" );
            if ( s_adpcm[k].pDelta == NULL )
            {
                printf( "\nError: Unable to alloc requested memory for sinusoid_quantisation instance\n");
                Throw SSC_ERROR_OUT_OF_MEMORY;
            }
            s_adpcm[k].pRepr  = SSC_CALLOC( NumLevels, sizeof( *s_adpcm[k].pRepr ), "" );
            if ( s_adpcm[k].pRepr == NULL )
            {
                printf( "\nError: Unable to alloc requested memory for sinusoid_quantisation instance\n");
                Throw SSC_ERROR_OUT_OF_MEMORY;
            }
            s_adpcm[k].pH     = SSC_CALLOC( PredMem, sizeof( *s_adpcm[k].pH ), "" );
            if ( s_adpcm[k].pH == NULL )
            {
                printf( "\nError: Unable to alloc requested memory for sinusoid_quantisation instance\n");
                Throw SSC_ERROR_OUT_OF_MEMORY;
            }
            s_adpcm[k].pYz    = SSC_CALLOC( PredMem, sizeof( *s_adpcm[k].pYz ), "" );
            if ( s_adpcm[k].pYz == NULL )
            {
                printf( "\nError: Unable to alloc requested memory for sinusoid_quantisation instance\n");
                Throw SSC_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    Catch( Error )
    {
        LOCAL_DestroyADPCMState( &s_adpcm, NumSin );
    }
    return s_adpcm ;

}

static void LOCAL_DestroyADPCMState( PSSC_ADPCM_STATE *s_adpcm, UInt NumSin )
{
    UInt k;
    for (k=0; k < NumSin; k++)
    {
        /* ADPCM memory */
        if ( s_adpcm[k]->pDelta == NULL )
        {
            SSC_FREE(s_adpcm[k]->pDelta  , "" );
        }
        if ( s_adpcm[k]->pRepr == NULL )
        {
            SSC_FREE(s_adpcm[k]->pRepr  , "" );
        }
        if ( s_adpcm[k]->pH == NULL )
        {
            SSC_FREE(s_adpcm[k]->pH  , "" );
        }
        if ( s_adpcm[k]->pYz == NULL )
        {
            SSC_FREE(s_adpcm[k]->pYz  , "" );
        }
    }
    if ( s_adpcm == NULL )
    {
        SSC_FREE(s_adpcm , "" );
    }
}

static void LOCAL_CopyADPCMState( PSSC_ADPCM_STATE s_adpcm_out, SSC_ADPCM_STATE s_adpcm_in, UInt NumLevels, UInt PredMem)
{
    UInt k;

    s_adpcm_out->dT              = s_adpcm_in.dT ;
    s_adpcm_out->dM              = s_adpcm_in.dM;
    s_adpcm_out->dUnwrappedPhase = s_adpcm_in.dUnwrappedPhase;

    for (k = 0; k < NumLevels + 1; k++)
    {
       s_adpcm_out->pDelta[k] = s_adpcm_in.pDelta[k];
    }
    for (k = 0; k < NumLevels; k++)
    {
       s_adpcm_out->pRepr[k] = s_adpcm_in.pRepr[k];
    }
    for (k = 0; k < PredMem; k++)
    {
       s_adpcm_out->pH[k]  = s_adpcm_in.pH[k];
       s_adpcm_out->pYz[k] = s_adpcm_in.pYz[k];
    }
}



static SInt LOCAL_quantiseADPCM
( 
    PSSC_ADPCM_STATE s_adpcm, 
    SInt             TrackIdx,
    Double           Freq, 
    Double           QuantFreq, 
    Double           Phase, 
    Double           QuantPhase,
    Double           PrevFreq, 
    Double           PrevPhase
)
{
    UInt i;
    SInt EncoderOut;

    if (TrackIdx == 1)   /* birth */
    {
        Double scalefac;
    
        /* Determine scale factor */
        i = 0;
        while (QuantFreq > SSC_ADPCM_FF[i])
        { 
            i++;
        }    
        scalefac = SSC_ADPCM_FS[i-1];

        /* Initialise the memory */
        s_adpcm->pYz[0] = QuantPhase;
        s_adpcm->pYz[1] = QuantPhase - QuantFreq * s_adpcm->dT;

        s_adpcm->pH[0] = SSC_ADPCM_P_h[0];
        s_adpcm->pH[1] = SSC_ADPCM_P_h[1];

        s_adpcm->dUnwrappedPhase = Phase;
        s_adpcm->dM = 0;

        /* Use default table (for births) */
        s_adpcm->pDelta[0] = SSC_ADPCM_Q[0];
        s_adpcm->pDelta[4] = SSC_ADPCM_Q[4];
        for (i=1; i < 4; i++)
        {
          s_adpcm->pDelta[i] = SSC_ADPCM_Q[i]/scalefac;
        }
        for (i=0; i < 4; i++)
        {
          s_adpcm->pRepr[i]  = SSC_ADPCM_R[i]/scalefac;
        }       

        EncoderOut = 0;
    }
    else
    {
        Double e, Dbuf, D_q, Y_tmp;
        SInt best_index;

        /* Calculate the unwrapped phase */
        e           = (PrevFreq + Freq)*(s_adpcm->dT)/2 - (Phase - PrevPhase);
        e           = floor((e/(2.0*SSC_PI))+0.5);
        s_adpcm->dM = e + s_adpcm->dM;
        s_adpcm->dUnwrappedPhase = Phase + s_adpcm->dM*2.0*SSC_PI;

        /* Calculate the prediction */
        e = 0.0;
        for (i = 0; i < 2; i++)
        {
          e += s_adpcm->pYz[i] * s_adpcm->pH[i];
        }

        /* Calculate difference */
        Dbuf = s_adpcm->dUnwrappedPhase - e;

        /* Quantise the difference */
        i = 0;
        while (Dbuf > s_adpcm->pDelta[i])
        { 
            i++;
        }    
        best_index = i-1;

        /* Debugging */
        assert( best_index >= 0);
        assert( best_index <= 3);

        /* Generate quantiser output */
        D_q    = s_adpcm->pRepr[best_index];
        Y_tmp  = D_q + e;     /* quantised unwrapped phase */

        /* Update predictor memory */
        s_adpcm->pYz[1] = s_adpcm->pYz[0];
        s_adpcm->pYz[0] = Y_tmp;

        /* Update delta and repr. after quantisation */
        LOCAL_update_delta( s_adpcm->pDelta,s_adpcm->pRepr, D_q,
		        SSC_ADPCM_delta_max, SSC_ADPCM_delta_min, SSC_ADPCM_P_m);

        EncoderOut = best_index;
    }

    return EncoderOut;
}


static void LOCAL_update_delta(
        Double * delta,
        Double * repr,
        Double C_next,
        const Double delta_max,
        const Double delta_min,
        const Double * m
)
{
    Double din, deltaIN, c;
    UInt i;
    
    din      = C_next;
    deltaIN  = delta[3] - delta[2]; /* Inner level */
    
    /* Select multiplier */
    if ( fabs(din) < deltaIN )
    {
        c = m[0];
    }
    else
    {
        c = m[1];
    }    
        
    /* Check if scaling is still needed */
    deltaIN = deltaIN * c;
    if ((deltaIN <= delta_min ) || (deltaIN >= delta_max))
    {
        c = 1.0;
    }
       
    /* Scale table */
    repr[0] *= c;
    for (i=1; i<4; i++)
    {
        delta[i] *= c;
        repr[i]  *= c;
    }     
}


