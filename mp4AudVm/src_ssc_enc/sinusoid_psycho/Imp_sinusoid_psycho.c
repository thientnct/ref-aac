/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - FFT and ODFT module by UHD



This software module was originally developed by

Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)

and edited by

Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.



Source file: Imp_sinusoid_psycho.c



Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>
AP    Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD    Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO    Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	  Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
11-sep-96   HP    adapted existing code
11-feb-97   BE    optimised odft/fft
12-feb-97   BE    optimised ifft
06-jun-97   HP    added M_PI
26-jan-01   BE    float -> double
02-nov-01   AP/JD changed interface
20-Sep-2002 NT    RM2 change
**********************************************************************/


/******************************************************************************
 *
 *   Module              : sinusoid_psycho implementation
 *
 *   Description         : Implementation of the sinusoid_psycho process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_PSYCHO_
 *
 *
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

#include "Imp_sinusoid_psycho.h"
#include "SSC_sinusoid_psycho.h"
#include "Imp_Error.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  0
#define PIPE_AHEAD_LENGTH 1

#undef min
#undef max
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

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
static void LOCAL_Psycho_process( PSINUSOID_PSYCHOSTATE pSPS, SSCPIPE *pPipe );
static void LOCAL_UHD_psycho_acoustic ( PSINUSOID_PSYCHOSTATE pSPS, Double *re, Double *im);


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_SINUSOID_PSYCHO_Initialise(void)
{
}


/* Creates an instance of the sinusoid_psycho module */
PSINUSOID_PSYCHOSTATE IMP_SINUSOID_PSYCHO_CreateInstance( PSINUSOID_PSYCHOINFO psinusoid_psychoInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_PSYCHOSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( psinusoid_psychoInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_psycho info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_PSYCHOSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_psycho instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_psycho", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_PSYCHO_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_psychoInfo->Channels,
                                           psinusoid_psychoInfo->SubframeSize,
                                           psinusoid_psychoInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, psinusoid_psychoInfo->sfAnaCount, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSINUSOID_PSYCHO_INIT_PIPE_FAILED ;
        }
        {
            if ( ( psinusoid_psychoInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_psychoInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
            {
                pms->nChannels  = 1 ; /* we then only need to process a single channel */
            }
            else
            {
                pms->nChannels  = psinusoid_psychoInfo->Channels ;
            }
            pms->nSubframeSize = psinusoid_psychoInfo->SubframeSize ;
            pms->sfAnaCount    = psinusoid_psychoInfo->sfAnaCount ;
            pms->fs            = psinusoid_psychoInfo->fs ;
            pms->dMinThreshold = psinusoid_psychoInfo->dMinThreshold ;
            pms->nBits         = (SInt)ceil(log(psinusoid_psychoInfo->sfAnaCount*psinusoid_psychoInfo->SubframeSize)/log(2.0));
            pms->nFFTlength    = (SInt)pow( 2, pms->nBits ) ;
            pms->nPSYlen       = pms->nFFTlength / 2 ;
            pms->dPSYdf        = psinusoid_psychoInfo->fs / pms->nFFTlength ;

            pms->pdPSYpwr      = SSC_CALLOC( sizeof( *pms->pdPSYpwr ),  pms->nPSYlen, "" );
            pms->pdPSYmask     = SSC_CALLOC( sizeof( *pms->pdPSYmask ), pms->nPSYlen, "" );
            pms->pFft          = SSC_FFT_CreateInstance( pms->nFFTlength );

            if(( pms->pdPSYpwr  == NULL ) ||
               ( pms->pdPSYmask == NULL ) ||
               ( pms->pFft      == NULL ) )
            {
                SSCLOG_TRACE5(pms->pLog,"\nError: Unable to allocate memory.\n");
                Throw SSC_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    Catch( Error )
    {
        IMP_SINUSOID_PSYCHO_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_PSYCHO_Process( PSINUSOID_PSYCHOSTATE psinusoid_psychoState, SSCBUNCH* pSscWorkBunch )
{
    if ( psinusoid_psychoState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( psinusoid_psychoState->pPipe, pSscWorkBunch );

        LOCAL_Psycho_process( psinusoid_psychoState, psinusoid_psychoState->pPipe );

        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscWorkBunch = SSCPIPE_Shift( psinusoid_psychoState->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_PSYCHO_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the sinusoid_psycho module instance pointed to by *ppState and
 * frees all the memory allocated on IMP_SINUSOID_PSYCHO_CreateInstance */
void IMP_SINUSOID_PSYCHO_DestroyInstance( PSINUSOID_PSYCHOSTATE* ppsinusoid_psychoState )
{
    PSINUSOID_PSYCHOSTATE psinusoid_psychoState = *ppsinusoid_psychoState ;

    if( psinusoid_psychoState != NULL )
    {
        SSCLOG_TRACE4( psinusoid_psychoState->pLog, "SSCSINUSOID_PSYCHO_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( psinusoid_psychoState->pPipe );
        SSC_FREE( psinusoid_psychoState->pdPSYpwr, "" );
        SSC_FREE( psinusoid_psychoState->pdPSYmask, "" );
        SSC_FFT_DestroyInstance( &psinusoid_psychoState->pFft );

        if( psinusoid_psychoState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( psinusoid_psychoState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( psinusoid_psychoState, "" );
        *ppsinusoid_psychoState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_PSYCHO_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_SINUSOID_PSYCHO_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_PSYCHO_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


/* Local processing function of the sinusoid psycho process */
static void LOCAL_Psycho_process( PSINUSOID_PSYCHOSTATE pSPS, SSCPIPE *pPipe )
{
    SInt nCh, j, nBin ;

    SSCBUNCH *pSscBunch = SSCPIPE_GetAt( pPipe, 0 );

    for( nCh = 0; nCh < pSPS->nChannels; nCh++ )
    {

        if( SSCBUNCH_GetDataFromStream( pSscBunch ) == 1 )
        {
            Double  dFactor ;
            SInt    i, nTracks = 0 ;
            Double *pdFreq       = SSCBUNCH_GetSinusoidFreq          ( pSscBunch, nCh );
            Double *pdAmp        = SSCBUNCH_GetSinusoidAmp           ( pSscBunch, nCh );
            Double *pdPhase_org  = SSCBUNCH_GetSinusoidPhaseOrg      ( pSscBunch, nCh );
            Double *pdPhase_cont = SSCBUNCH_GetSinusoidPhaseCont     ( pSscBunch, nCh );
            Double *pdRel        = SSCBUNCH_GetSinusoidRelevance     ( pSscBunch, nCh );
            Double *pdTrack_idx  = SSCBUNCH_GetSinusoidTrackIdx      ( pSscBunch, nCh );
            Double *pdRe         = SSC_FFT_GetPr( pSPS->pFft );
            Double *pdIm         = SSC_FFT_GetPi( pSPS->pFft );

            SSC_FFT_ClearIm( pSPS->pFft );
            for( j=0; j<pSPS->sfAnaCount; j++ )
            {
                memmove( &pdRe[j*pSPS->nSubframeSize],
                          SSCBUNCH_GetResidual( SSCPIPE_GetAt( pPipe, j), nCh), pSPS->nSubframeSize * sizeof( *pdRe ));
            }
            memset ( &pdRe[j*pSPS->nSubframeSize], 0, (pSPS->nFFTlength - (j*pSPS->nSubframeSize)) * sizeof( *pdRe ));
            SSC_FFT_Process( pSPS->pFft, pSPS->nFFTlength );

            dFactor = 1.0 / pSPS->nFFTlength ;
            for( i=0; i<pSPS->nFFTlength; i++ )
            {
              pdRe[i] *= dFactor ;
              pdIm[i] *= dFactor ;
            }
            LOCAL_UHD_psycho_acoustic ( pSPS, pdRe, pdIm);

            for( j=0; j<SSC_ENCODER_MAX_SINS; j++ )
            {
                if( pdTrack_idx[j] > 0 ) /* check length of track */
                {
                    nBin = (SInt)(( (pdFreq[j] * pSPS->fs / (2*SSC_PI)) / pSPS->dPSYdf)+ 0.5) ;
                    nBin = max(0,nBin);
                    nBin = min(pSPS->nPSYlen-1, nBin);
                    pdRel[j] = pdAmp[j] / pSPS->pdPSYmask[nBin] ;
                    if( pdRel[j] < pSPS->dMinThreshold )
                    {
                        pdFreq[j]       = 0;
                        pdAmp[j]        = 0;
                        pdPhase_org[j]  = 0;
                        pdPhase_cont[j] = 0;
                        pdRel[j]        = 0;
                        pdTrack_idx[j]  = 0;
                    }
                    else
                    {
                        SSCLOG_TRACE3( pSPS->pLog, "freq: %g amp: %g rel: %g\n", pdFreq[j] * pSPS->fs / (2*SSC_PI), pdAmp[j], pdRel[j] );
                        nTracks++ ;
                    }
                }
            }
            SSCLOG_TRACE2( pSPS->pLog, "Sf %d, tracks: %d\n", SSCBUNCH_GetSfCounter( pSscBunch ), nTracks );
        }
    }
}


/* UHD_psycho_acoustic() */
/* UHD psychoacoustic model (BASIC). */
/* Calculate a masking threshold 20 dB below signal spectrum */
/* using a 1 Bark wide rectagular window. */

/*************************LOCAL FUNCTION**************************************
 *
 * Name        : LOCAL_UHD_psycho_acoustic
 *
 * Description : Local psycho mask calculation
 *
 * Parameters  : pSPS : pointer to sinusoid_psychostate structure
 *
 *               re   : real buffer
 *
 *               im   : imaginair buffer
 *
 * Returns:    : pointer to psycho mask buffer
 *
 *****************************************************************************/

static void LOCAL_UHD_psycho_acoustic ( PSINUSOID_PSYCHOSTATE pSPS, Double *re, Double *im)
{
  SInt i;
  SInt high,mid,mid_old,low,low_old;
  SInt bark;
  Double sum;

  /* calc power spectrum */
  for (i=0; i<pSPS->nPSYlen; i++)
  {
    pSPS->pdPSYpwr[i] = sqrt(2*(re[i]*re[i]+im[i]*im[i])); /* base PSYmask on amplitude */
  }
  /* calc masking threshold */
  /* bark(f)=max(100Hz,f*0.2) */
  sum = 0;
  mid = mid_old = low = low_old = -1;
  bark = (SInt)(100/pSPS->dPSYdf+.5);
  for (high=0; high<pSPS->nPSYlen; high++)
  {
    sum += pSPS->pdPSYpwr[high];
    bark = max(bark,(SInt)(high*0.2+.5));
    low = max(low,high-bark);
    if (low > low_old)
    {
      sum -= pSPS->pdPSYpwr[low];
      low_old = low;
    }
    mid = (low+high+1)/2;
    if (mid > mid_old)
    {
      pSPS->pdPSYmask[mid] = sum/(high-low)*0.1 + 1.0;	/* -20 dB + thres. quiet */
      mid_old = mid;
    }
  }
  high--;
  low++;
  for (; low<pSPS->nPSYlen; low++)
  {
    sum -= pSPS->pdPSYpwr[low];
    mid = (low+high+1)/2;
    if (mid > mid_old)
    {
      pSPS->pdPSYmask[mid] = sum/(high-low)*0.1 + 1.0;	/* -20 dB + thres. quiet */
      mid_old = mid;
    }
  }
}
