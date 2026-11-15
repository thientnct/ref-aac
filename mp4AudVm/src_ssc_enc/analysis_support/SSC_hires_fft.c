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

Source file: SSC_hires_fft.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
25-Sep-2001	AP/JD	Initial version based on:
************************************************************************/

/******************************************************************************
 *
 *   Module              : FFT implementation
 *
 *   Description         : Implementation of the (i)fft process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_HIRES_FFT_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "Imp_quantise.h"
#include "SSC_hires_fft.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/
static SInt LOCAL_compare   ( const void *elem1, const void *elem2 );
static Double LOCAL_getamp  ( Double freq, Double *pSigWin, SInt n );
static Double LOCAL_getphase( Double fp, Double dRe, Double dIm, Double bin );

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_HIRES_FFT_Initialise
 *
 * Description :
 *
 * Parameters  :
 *
 * Returns:    :
 *
 *****************************************************************************/

void SSC_HIRES_FFT_Initialise( void )
{
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_HIRES_FFT_Free
 *
 * Description :
 *
 * Parameters  :
 *
 * Returns:    :
 *
 *****************************************************************************/

void SSC_HIRES_FFT_Free( void )
{
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_HIRES_FFT_CreateInstance
 *
 * Description : allocates memory buffers and initializes the required buffers
 *
 * Parameters  :
 *
 * Returns:    :
 *
 *****************************************************************************/

SSC_HIRES_FFT_INFO *SSC_HIRES_FFT_CreateInstance(
                                                  SInt     nLen,
                                                  SInt     nSinusoids,
                                                  Double   Fs,
                                                  Double   FreqLimit
                                                )
{
    SSC_HIRES_FFT_INFO *pHiresFft        = NULL ;
    SSC_EXCEPTION  tec                   = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;
    SInt           Error                 = SSC_HIRES_FFT_ERROR_OK ;

    Try
    {
        SInt    nIdx ;

        pHiresFft  = SSC_CALLOC( sizeof(SSC_HIRES_FFT_INFO), 1, "" );

        pHiresFft->nLen       = nLen;
        pHiresFft->nSinusoids = nSinusoids;
        pHiresFft->Fs         = Fs;
        pHiresFft->FreqLimit  = FreqLimit;

        if( (pHiresFft->nLen <= 0)         ||
            (pHiresFft->nSinusoids <= 0)   ||
            (pHiresFft->Fs <= 0)           ||
            (pHiresFft->FreqLimit <= 0)
          )
        {
            Throw SSC_HIRES_FFT_INVALID_INPUT_PARAMETER ;
        }
        pHiresFft->nAnaLength = (SInt)floor(pHiresFft->FreqLimit * pHiresFft->nLen / pHiresFft->Fs );

        pHiresFft->pWindow    = SSC_CALLOC( nLen                    * sizeof(Double), 1, "" );
        pHiresFft->pSigfft    = SSC_CALLOC( nLen                    * sizeof(Double), 1, "" );
        pHiresFft->pSig_d_fft = SSC_CALLOC( nLen                    * sizeof(Double), 1, "" );
        pHiresFft->pAmpInfo   = SSC_CALLOC( pHiresFft->nAnaLength/2 * sizeof(AMPINFO), 1, "" );
        pHiresFft->pWorkBuf   = SSC_CALLOC( nLen                    * sizeof(Double), 1, "" );
        pHiresFft->pSigWin    = SSC_CALLOC( nLen                    * sizeof(Double), 1, "" );
        pHiresFft->pFftSigWin = SSC_FFT_CreateInstance( nLen );
        pHiresFft->pFft       = SSC_FFT_CreateInstance( nLen );

        if( (pHiresFft->pWindow    == NULL) ||
            (pHiresFft->pSigfft    == NULL) ||
            (pHiresFft->pSig_d_fft == NULL) ||
            (pHiresFft->pAmpInfo   == NULL) ||
            (pHiresFft->pWorkBuf   == NULL) ||
            (pHiresFft->pFftSigWin == NULL) ||
            (pHiresFft->pFft       == NULL) ||
            (pHiresFft->pSigWin    == NULL)
          )
        {
            Throw SSC_HIRES_FFT_MEMORY_ERROR ;
        }
        for( nIdx = 0; nIdx < nLen; nIdx++ )
        {   /* hanning window */
            pHiresFft->pWindow[nIdx]  = .5*( 1 - cos( 2*SSC_PI*(nIdx+1)/(nLen+1) ) );
        }
    }
    Catch(Error)
    {
        SSC_HIRES_FFT_DestroyInstance( &pHiresFft );
    }
    return pHiresFft ;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        :
 *
 * Description :
 *
 * Parameters  :
 *
 * Returns:    :
 *
 *****************************************************************************/

SInt SSC_HIRES_FFT_Process( SSC_HIRES_FFT_INFO * const pHiresFft, Double *pSegment, Double *pFreq, Double *pAmp, Double *pPhase )
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SInt Error = SSC_HIRES_FFT_ERROR_OK ;

    if( pHiresFft == NULL )
    {
        return SSC_HIRES_FFT_INVALID_STATE_POINTER;
    }
    Try
    {
        SInt          nIdx, nAmpIdx ;
        Double       *pdRe, *pdIm ;
        Double       *pWindow    = pHiresFft->pWindow ;
        Double       *pWorkbuf   = pHiresFft->pWorkBuf ;
        Double       *pSigfft    = pHiresFft->pSigfft ;
        Double       *pSig_d_fft = pHiresFft->pSig_d_fft ;
        Double       *pSigWin    = pHiresFft->pSigWin ;
        AMPINFO      *pAmpInfo   = pHiresFft->pAmpInfo ;
        Double        Fs         = pHiresFft->Fs ;
        SInt          nLen       = pHiresFft->nLen ;
        SInt          nSinusoids = pHiresFft->nSinusoids ;
        SInt          nAnaLength = pHiresFft->nAnaLength ;
        SSC_FFT_INFO *pFftSigWin = pHiresFft->pFftSigWin ;
        SSC_FFT_INFO *pFft       = pHiresFft->pFft ;

        memset( pFreq,  0, nSinusoids * sizeof( *pFreq ) );
        memset( pAmp,   0, nSinusoids * sizeof( *pAmp ) );
        memset( pPhase, 0, nSinusoids * sizeof( *pPhase ) );

        for( nIdx = 0; nIdx < nLen; nIdx++ )
        {
            pSigWin[nIdx] = pWindow[nIdx] * pSegment[nIdx] ;
        }
        SSC_FFT_SetRe       ( pFftSigWin, pSigWin, nLen );
        SSC_FFT_SetIm       ( pFftSigWin, NULL, nLen );
        SSC_FFT_Process     ( pFftSigWin, SSC_FFT_GetLength( pFft ) );
        pdRe = SSC_FFT_GetPr( pFftSigWin );
        pdIm = SSC_FFT_GetPi( pFftSigWin );

        pSigfft[0]  = sqrt( pdRe[0] * pdRe[0] + pdIm[0] * pdIm[0] );
        pWorkbuf[0] = 0 ;
        for( nIdx = 1; nIdx < nLen; nIdx++ )
        {
            pSigfft[nIdx]  = sqrt( pdRe[nIdx] * pdRe[nIdx] + pdIm[nIdx] * pdIm[nIdx] );
            pWorkbuf[nIdx] = Fs * (pSegment[nIdx] - pSegment[nIdx-1]) * pWindow[nIdx] ;
        }
        SSC_FFT_SetRe( pFft, pWorkbuf, nLen ); /* sig_d_win */
        SSC_FFT_SetIm( pFft, NULL,     nLen );
        SSC_FFT_Process( pFft, SSC_FFT_GetLength( pFft ) );
        pdRe = SSC_FFT_GetPr( pFft );
        pdIm = SSC_FFT_GetPi( pFft );

        for( nIdx = 0; nIdx < nLen; nIdx++ )
        {
            pSig_d_fft[nIdx]  = sqrt( pdRe[nIdx] * pdRe[nIdx] + pdIm[nIdx] * pdIm[nIdx] );
        }
        for(nIdx = 1, nAmpIdx = 0; nIdx < nAnaLength-1; nIdx++ )
        {
            if( (pSigfft[nIdx] > pSigfft[nIdx-1]) && (pSigfft[nIdx] > pSigfft[nIdx+1]) )
            {
                pAmpInfo[nAmpIdx].dAmplitude = pSigfft[nIdx] ;
                pAmpInfo[nAmpIdx].nOrgIdx    = nIdx ;
                nAmpIdx++ ;
            }
        }
        qsort( (void*)pAmpInfo, nAmpIdx+1, sizeof( AMPINFO ), LOCAL_compare );
        {
            Double we, gain, fp, bin ;
            SInt n = 0;
            SInt k = 0;

            pdRe = SSC_FFT_GetPr( pFftSigWin );
            pdIm = SSC_FFT_GetPi( pFftSigWin );

            while( n < nSinusoids && k<nAmpIdx )
            {
                AMPINFO *pCurrAmpInfo = &pAmpInfo[k] ;

                nIdx = pCurrAmpInfo->nOrgIdx ;

                we = 2 * SSC_PI * nIdx / nLen ;
                gain = we / ( 2 * sin(we / 2) );

                fp = (gain / Fs) * pSig_d_fft[nIdx] / pSigfft[nIdx] ;

                bin = (fp / (2 * SSC_PI)) * nLen ;
                if( fabs( bin - nIdx ) < 0.75 )
                {
                    pFreq[n]  = fp ;
                    pAmp[n]   = LOCAL_getamp  ( fp, pSigWin, nLen );
                    pPhase[n] = LOCAL_getphase( fp, pdRe[nIdx], -pdIm[nIdx], bin );

                    n = n+1 ;
                }
                k = k + 1 ;
            }
        }
    }
    Catch(Error)
    {
        Error = SSC_HIRES_FFT_PROCESS_ERROR ;
    }
    return Error ;
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_HIRES_FFT_DestroyInstance
 *
 * Description : frees all allocated memory
 *
 * Parameters  :
 *
 * Returns:    :
 *
 *****************************************************************************/

void SSC_HIRES_FFT_DestroyInstance( SSC_HIRES_FFT_INFO **ppHiresFft )
{
    SSC_HIRES_FFT_INFO *pHiresFft = *ppHiresFft ;
    if ( pHiresFft != NULL )
    {
        SSC_FREE( pHiresFft->pWindow,    "" );
        SSC_FREE( pHiresFft->pAmpInfo,   "" );
        SSC_FREE( pHiresFft->pSigfft,    "" );
        SSC_FREE( pHiresFft->pSig_d_fft, "" );
        SSC_FREE( pHiresFft->pSigWin,    "" );
        SSC_FREE( pHiresFft->pWorkBuf,   "" );

        SSC_FFT_DestroyInstance( &pHiresFft->pFftSigWin );
        SSC_FFT_DestroyInstance( &pHiresFft->pFft );

        SSC_FREE( pHiresFft, "" );
    }
    *ppHiresFft = NULL;
}

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


static SInt LOCAL_compare( const void *elem1, const void *elem2 )
{
     if( (*(AMPINFO*)elem1).dAmplitude < (*(AMPINFO*)elem2).dAmplitude )
     {
        return 1 ;
     }
     else if ( (*(AMPINFO*)elem1).dAmplitude > (*(AMPINFO*)elem2).dAmplitude )
     {
        return -1 ;
     }
     else
     {
        return 0 ;
     }
}

static Double LOCAL_getamp( Double freq, Double *pSigWin, SInt n )
{
    SInt nIdx ;
    Double dSumRe = 0 ;
    Double dSumIm = 0 ;

    for( nIdx = 0; nIdx < n; nIdx++ )
    {
        dSumRe += cos(freq*nIdx) * pSigWin[nIdx] ;
        dSumIm += sin(freq*nIdx) * pSigWin[nIdx] ;
    }
    dSumRe = 2 * dSumRe / n ;
    dSumIm = 2 * dSumIm / n ;
    return ( 2 * sqrt((dSumRe * dSumRe) + (dSumIm * dSumIm)) );
}

static Double LOCAL_getphase( Double fp, Double dRe, Double dIm, Double bin )
{
    SInt even, noteven ;

    if( ((SInt)(floor(bin+0.5)) % 2) == 0 )
    {
        even    = 1 ;
        noteven = 0 ;
    }
    else
    {
        even    = 0 ;
        noteven = 1 ;
    }
    return fold_phase( (SSC_PI/2) - (atan2(dIm, dRe) + (fp/2) + ((even - noteven) * SSC_PI/2)));
}
