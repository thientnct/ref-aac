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

Source file: SSC_Bunch.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AR: Aad Rijnberg,      Philips CE - PDS-labs Eindhoven <aad.rijnberg@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
16-Aug-2002 AR      Changes for m8540 core experiment
20-Sep-2002 NT      RM2 change
************************************************************************/

/*===========================================================================
 *
 *  Module      : SSC_Bunch
 *
 *  File        : SSC_Bunch.c
 *
 *  Description : This header file implements the interface for accessing
 *                SBunch objects.
 *
 ============================================================================*/

#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"


static PTRANSIENTPARMS LOCAL_Get_Empty_TransientParms( SInt nChannels )
{
    PTRANSIENTPARMS ptpBase, ptpWalk ;

    ptpBase = SSC_CALLOC( nChannels * sizeof(TRANSIENTPARMS), 1, "" );
    if( ptpBase )
    {
        SInt i;
        SInt nMaxSin = SSC_T_MAX_MEIXNER_SINUSOIDS;

        ptpWalk = ptpBase ;
        for( i=0; i<nChannels; i++ )
        {
            ptpWalk->bTransient = False ;
            ptpWalk->nPosition = 0 ;
            ptpWalk->nTran_type = 0 ;
            ptpWalk->dBpar = 0 ;
            ptpWalk->dChi = 0 ;

            ptpWalk->pTSparms = SSC_CALLOC( sizeof(TSINEPARMS), 1, "");
            if( ptpWalk->pTSparms )
            {
                ptpWalk->pTSparms->nr_of_sins = 0;
                ptpWalk->pTSparms->pdFreq  = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "" ) ;
                ptpWalk->pTSparms->pdAmp   = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "" ) ;
                ptpWalk->pTSparms->pdPhase = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "" ) ;

                ptpWalk->pTSparms->cntFreq  = nMaxSin ;
                ptpWalk->pTSparms->cntAmp   = nMaxSin ;
                ptpWalk->pTSparms->cntPhase = nMaxSin ;

                ptpWalk->cntTSparms = 1 ;
            }
            ptpWalk++ ;
        }
    }
    return ptpBase ;
}

static void SSC_FREE_TransientSineParms( PTSINEPARMS ptsp )
{
    if( ptsp )
    {
        SSC_FREE(ptsp->pdFreq, "");
        SSC_FREE(ptsp->pdAmp, "");
        SSC_FREE(ptsp->pdPhase, "");

        SSC_FREE(ptsp, "");
    }
}

static void SSC_FREE_TransientParms( PTRANSIENTPARMS ptp, SInt nChannels )
{
    if( ptp )
    {
        SInt i;
        PTRANSIENTPARMS ptpWalk = ptp ;

        for( i=0; i<nChannels; i++ )
        {
            SSC_FREE_TransientSineParms( ptpWalk->pTSparms );
            ptpWalk++;
        }
        SSC_FREE( ptp, "" );
    }
}

static PTRANSIENTROOT LOCAL_Get_Empty_TransientRoot( SInt nChannels )
{
    PTRANSIENTROOT ptr = SSC_CALLOC( sizeof( TRANSIENTROOT ), 1, "");

    if( ptr )
    {
        ptr->pTparms = LOCAL_Get_Empty_TransientParms( nChannels );
        if( ptr->pTparms )
            ptr->cntTparms = nChannels  ;
        ptr->pTQparms = LOCAL_Get_Empty_TransientParms( nChannels );
        if( ptr->pTQparms )
            ptr->cntTQparms = nChannels ;
    }
    return ptr ;
}

static void SSC_FREE_TransientRoot( PTRANSIENTROOT ptr )
{
    if( ptr )
    {
        SSC_FREE_TransientParms( ptr->pTparms, ptr->cntTparms );
        SSC_FREE_TransientParms( ptr->pTQparms, ptr->cntTQparms );
        SSC_FREE( ptr, "" );
    }
}

static PSINEPARMS LOCAL_Get_Empty_SineParms( SInt nChannels )
{
    SInt    i ;
    PSINEPARMS pspBase, pspWalk ;

    pspBase = SSC_CALLOC( nChannels*sizeof(SINEPARMS), 1, "");
    if( pspBase )
    {
        SInt nMaxSin = SSC_ENCODER_MAX_SINS ;

        pspWalk = pspBase ;
        for( i=0; i<nChannels; i++ )
        {
            SInt nTrack ;

            /* element pointers */
            pspWalk->pdTrackIdx       = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdTrackPrev      = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdTrackNext      = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            for( nTrack=0; nTrack<nMaxSin; nTrack++ )
            {
                pspWalk->pdTrackPrev[nTrack] = -1.0 ; /* 0 based index, -1 means no track */
                pspWalk->pdTrackNext[nTrack] = -1.0 ; /* 0 based index, -1 means no track */
            }
            pspWalk->pdFreq           = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdAmp            = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdPhaseOrg       = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdPhaseCont      = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdRel            = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->pdQuantGrid      = SSC_CALLOC( nMaxSin * sizeof(Double), 1, "");
            pspWalk->nGridFreq        = 0;
            pspWalk->nGridAmp         = 0;
            pspWalk->dRelThreshold    = 0;

            /* element counters */
            pspWalk->cntTrackIdx       = nMaxSin;
            pspWalk->cntTrackPrev      = nMaxSin;
            pspWalk->cntTrackNext      = nMaxSin;
            pspWalk->cntFreq           = nMaxSin;
            pspWalk->cntAmp            = nMaxSin;
            pspWalk->cntPhaseOrg       = nMaxSin;
            pspWalk->cntPhaseCont      = nMaxSin;
            pspWalk->cntRel            = nMaxSin;
            pspWalk->cntQuantGrid      = nMaxSin;

            pspWalk++ ;
        }
    }
    return pspBase ;
}

static void SSC_FREE_SineParms( PSINEPARMS psp, SInt nChannels )
{
    if( psp )
    {
        SInt i;
        PSINEPARMS pspWalk = psp;

        for( i=0; i<nChannels; i++ )
        {
            SSC_FREE(pspWalk->pdTrackIdx, "" );
            SSC_FREE(pspWalk->pdTrackPrev, "" );
            SSC_FREE(pspWalk->pdTrackNext, "" );
            SSC_FREE(pspWalk->pdFreq, "" );
            SSC_FREE(pspWalk->pdAmp, "" );
            SSC_FREE(pspWalk->pdPhaseOrg, "" );
            SSC_FREE(pspWalk->pdPhaseCont, "" );
            SSC_FREE(pspWalk->pdRel, "" );
            SSC_FREE(pspWalk->pdQuantGrid   , "" );

            pspWalk++ ;
        }
        SSC_FREE( psp, "" );
    }
}

static PSINEROOT LOCAL_Get_Empty_SineRoot( SInt nChannels )
{
    PSINEROOT psr = SSC_CALLOC( sizeof( SINEROOT ), 1, "");
    
    if( psr )
    {
        psr->pSparms = LOCAL_Get_Empty_SineParms( nChannels );
        if( psr->pSparms )
            psr->cntSparms = nChannels ;
        psr->pSQparms = LOCAL_Get_Empty_SineParms( nChannels );
        if( psr->pSQparms )
            psr->cntSQparms = nChannels ;
    }    
    return psr ;
}

static void SSC_FREE_SineRoot( PSINEROOT psr )
{
    if( psr )
    {
        SSC_FREE_SineParms( psr->pSparms,  psr->cntSparms );
        SSC_FREE_SineParms( psr->pSQparms, psr->cntSQparms );
        SSC_FREE( psr, "" );
    }
}

static PNOISEPARMS LOCAL_Get_Empty_NoiseParms( SInt nChannels )
{
    PNOISEPARMS pnpBase, pnpWalk ;

    pnpBase = SSC_CALLOC( nChannels * sizeof(NOISEPARMS), 1, "");
    if( pnpBase )
    {
        SInt i;

        SInt nMaxDen      = SSC_ENCODER_MAX_DEN ;
        SInt nMaxEnvCoefs = SSC_N_MAX_LSF_ORDER ;

        pnpWalk = pnpBase ;
        for( i=0; i<nChannels; i++ )
        {
            /* element pointers */
            pnpWalk->pdDen      = SSC_CALLOC( nMaxDen      * sizeof(Double), 1, "") ;
            pnpWalk->pdEnvCoefs = SSC_CALLOC( nMaxEnvCoefs * sizeof(Double), 1, "") ;
            
            pnpWalk->dEnvGain   = 0.0;
            pnpWalk->envOverlap = 0;

            pnpWalk->nGridLaguerre = 0;
            pnpWalk->dPoleLaguerre = 0.0;

            /* element count */
            pnpWalk->cntDen      = nMaxDen ; 
            pnpWalk->cntEnvCoefs = nMaxEnvCoefs ;

            pnpWalk++ ;
        }
    }
    return pnpBase ;
}

static void SSC_FREE_NoiseParms( PNOISEPARMS pnp, SInt nChannels )
{
    if( pnp )
    {
        SInt i;
        PNOISEPARMS pnpWalk = pnp ;

        for( i=0; i<nChannels; i++ )
        {
            SSC_FREE(pnpWalk->pdNum     , "");
            SSC_FREE(pnpWalk->pdDen     , "");
            SSC_FREE(pnpWalk->pdEnvCoefs, "");

            pnpWalk++;
        }
        SSC_FREE( pnp, "" );
    }
}

static PNOISEROOT LOCAL_Get_Empty_NoiseRoot( SInt nChannels )
{
    PNOISEROOT pnr = SSC_CALLOC( sizeof( NOISEROOT ), 1, "");

    if( pnr )
    {
        pnr->pNparms = LOCAL_Get_Empty_NoiseParms( nChannels );
        if( pnr->pNparms )
            pnr->cntNparms = nChannels ;
        pnr->pNQparms = LOCAL_Get_Empty_NoiseParms( nChannels );
        if( pnr->pNQparms )
            pnr->cntNQparms = nChannels ;
    }
    return pnr ;
}

static void SSC_FREE_NoiseRoot( PNOISEROOT pnr )
{
    if( pnr )
    {
        SSC_FREE_NoiseParms( pnr->pNparms, pnr->cntNparms );
        SSC_FREE_NoiseParms( pnr->pNQparms, pnr->cntNQparms );
        SSC_FREE( pnr, "" );
    }    
}

static PSTEREOPARMS LOCAL_Get_Empty_StereoParms( SInt nNum )
{
    PSTEREOPARMS pstpBase, pstpWalk;

    pstpBase = SSC_CALLOC( sizeof( STEREOPARMS ), 1, "");

    if ( pstpBase != NULL )
    {
        SInt i;

        pstpWalk = pstpBase ;
        for( i = 0; i < nNum; i++, pstpWalk++ )
        {
            pstpWalk->envBorder   = 0;

            pstpWalk->header       = False;
            pstpWalk->iidEnable    = False;
            pstpWalk->iccEnable    = False;
            pstpWalk->ipdEnable    = False;
            pstpWalk->extEnable    = False;
            pstpWalk->pcaEnable    = False;

            pstpWalk->iidFreqRes   = 0;
            pstpWalk->iidQuant     = 0;
            pstpWalk->iccFreqRes   = 0;
       
            pstpWalk->iid  = SSC_CALLOC( SSC_OCS_MAX_IID * sizeof(Double), 1, "" );
            pstpWalk->ipd  = SSC_CALLOC( SSC_OCS_MAX_IPD * sizeof(Double), 1, "" );
            pstpWalk->opd  = SSC_CALLOC( SSC_OCS_MAX_OPD * sizeof(Double), 1, "" );
            pstpWalk->icc  = SSC_CALLOC( SSC_OCS_MAX_ICC * sizeof(Double), 1, "" );
        }
    }

    return pstpBase;
}

static void SSC_FREE_StereoParms( PSTEREOPARMS pstp, SInt nNum )
{
    if ( pstp != NULL )
    {
        SInt i;
        PSTEREOPARMS pnpWalk = pstp ;

        for( i=0; i<nNum; i++ )
        {
            SSC_FREE(pstp->iid, "" );
            SSC_FREE(pstp->ipd, "" );
            SSC_FREE(pstp->opd, "" );
            SSC_FREE(pstp->icc, "" );

            pnpWalk++;
        }

        SSC_FREE(pstp, "" );
    }
}

static PSTEREOROOT LOCAL_Get_Empty_StereoRoot( void )
{
    PSTEREOROOT pstr = SSC_CALLOC( sizeof( STEREOROOT ), 1, "" );

    if ( pstr != NULL )
    {
        pstr->pSTparms  = LOCAL_Get_Empty_StereoParms(1);
        if( pstr->pSTparms )
            pstr->cntSTparms = 1 ;
        pstr->pSTQparms = LOCAL_Get_Empty_StereoParms(1);
        if( pstr->pSTQparms )
            pstr->cntSTQparms = 1 ;
    }

    return pstr;
}

static void SSC_FREE_StereoRoot( PSTEREOROOT pstr )
{
    if ( pstr != NULL )
    {
        SSC_FREE_StereoParms( pstr->pSTparms, pstr->cntSTparms );
        SSC_FREE_StereoParms( pstr->pSTQparms, pstr->cntSTQparms );
        SSC_FREE( pstr, "");
    }
}

static PSBUNCH LOCAL_Get_Empty_SBunch(  SInt nChannels, SInt nSubframeSize )
{
    SInt    i, chCnt, sfSize;
    PSBUNCH psBase, psWalk ;

    chCnt = nChannels ;
    sfSize = nSubframeSize ;

    psBase = SSC_CALLOC( sizeof(SBUNCH), 1, "");
    if( psBase )
    {
        psWalk = psBase ;

        for( i=0; i<1; i++ )
        {
            psWalk->sf_counter = 0;
            psWalk->data_from_stream = 0;
            psWalk->pactual_bitrate = SSC_CALLOC( 6 * sizeof( *psWalk->pactual_bitrate ), 1, "" ); /* number of bitrate fields */
            psWalk->psignal   = SSC_CALLOC( sfSize * chCnt * sizeof( *psWalk->psignal), 1, "" ); /* [subframesize, channels] */
            psWalk->psynth    = SSC_CALLOC( sfSize * chCnt * sizeof( *psWalk->psynth), 1, "" ); /* [subframesize, channels] */
            psWalk->presidual = SSC_CALLOC( sfSize * chCnt * sizeof( *psWalk->presidual), 1, "" ); /* [subframesize, channels] */

            psWalk->pTroot = LOCAL_Get_Empty_TransientRoot( nChannels ); /* transient [channels] parameters */
            psWalk->pSroot = LOCAL_Get_Empty_SineRoot( nChannels );      /* [channels, 60] sinusoidal parameters */  
            psWalk->pNroot = LOCAL_Get_Empty_NoiseRoot( nChannels );     /* noise [channels] parameters */

            psWalk->pSTroot= LOCAL_Get_Empty_StereoRoot();  /* only one instance */
    
            /* for debugging and range checking */
            if( psWalk->pactual_bitrate )
                psWalk->cntBitrate = 6 ;                /* pactual_bitrate element count */
            if( psWalk->psignal )
                psWalk->cntSignal = sfSize * chCnt ;    /* psignal element count */
            if( psWalk->psynth )
                psWalk->cntSynth = sfSize * chCnt ;     /* psynth element count */
            if( psWalk->presidual )
                psWalk->cntResidual = sfSize * chCnt;   /* presidual element count */
            if( psWalk->pTroot )
                psWalk->cntTroot = 1 ;                  /* transient root element count */
            if( psWalk->pSroot )
                psWalk->cntSroot = 1 ;                  /* sinusoidal info element count */
            if( psWalk->pNroot )
                psWalk->cntNroot = 1;                   /* noise info element count */
            if( psWalk->pSTroot )
                psWalk->cntSTroot= 1;                   /* stereo info element count */
            
            psWalk++ ;
        }
    }
    return psBase ;
}

static void SSC_FREE_SBunch(PSBUNCH psBunch)
{
    if( psBunch )
    {
        SInt i;
        PSBUNCH psWalk = psBunch ;

        for( i=0; i<1; i++ )
        {
            SSC_FREE(psWalk->pactual_bitrate, "" );
            SSC_FREE(psWalk->psignal, "");
            SSC_FREE(psWalk->psynth, "");
            SSC_FREE(psWalk->presidual, "");
            SSC_FREE_TransientRoot( psWalk->pTroot );
            SSC_FREE_SineRoot( psWalk->pSroot );
            SSC_FREE_NoiseRoot( psWalk->pNroot );
            SSC_FREE_StereoRoot ( psWalk->pSTroot );
            
            psWalk++ ;
        }
        SSC_FREE(psBunch, "");
    }
}



/*********************************************************/
/*                                                       */
/*                                                       */
/* global functions                                      */
/*                                                       */
/*                                                       */
/*********************************************************/


SSC_INTF_EXPORT SSCBUNCH *SSCBUNCH_InitInstance( SInt nChannels, SInt nSubframeSize, SInt nDecoder_level, SSCLOG *pLog )
{
    SSCBUNCH *pSscBunch = SSC_CALLOC( sizeof(SSCBUNCH), 1, "" );

    if( pSscBunch )
    {
        pSscBunch->pLog = pLog ; /* copy pointer to log object owned by parent */
        pSscBunch->nChannels = nChannels ;
        pSscBunch->nSubframeSize = nSubframeSize ;
        pSscBunch->nDecoderLevel = nDecoder_level ;
        pSscBunch->pBunch = LOCAL_Get_Empty_SBunch( nChannels, nSubframeSize );
        if( pSscBunch->pBunch )
        {
            SSCLOG_TRACE1( pSscBunch->pLog, "SSCBunch_InitInstance, nChannels %d, nSubframesize %d, decoder level %d\n", 
                                            nChannels, nSubframeSize, nDecoder_level );
        }
        else
        {
            SSC_FREE( pSscBunch, "" );
            pSscBunch = NULL ;
        }
    }
    return pSscBunch ;
}


SSC_INTF_EXPORT void SSCBUNCH_DestroyInstance( SSCBUNCH *pSscBunch )
{
    if( pSscBunch )
    {
        SSCLOG_TRACE1( pSscBunch->pLog, "SSCBunch_DestroyInstance\n" );
        SSC_FREE_SBunch( pSscBunch->pBunch );                     
        pSscBunch->pLog = NULL ; /* do not destroy log object (owned by parent) */
        SSC_FREE( pSscBunch, "" );
        pSscBunch = NULL ;
    }
}


SSC_INTF_EXPORT void SSCBUNCH_Clear( SSCBUNCH *pSscBunch )
{
    SInt    nCh, nSinusoids, nSfSize, nDen, nBin, nTrack ;
    SBUNCH *pBunch ;

    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_Clear\n" );
    pBunch = pSscBunch->pBunch ;
    nSfSize = pSscBunch->nSubframeSize ;
    nSinusoids = SSC_ENCODER_MAX_SINS ;
    nDen = SSC_ENCODER_MAX_DEN ;
    nBin = SSC_ENCODER_MAX_STEREO_BINS;

    for( nCh=0; nCh<pSscBunch->nChannels; nCh++ )
    {
        pBunch->sf_counter = 0;
        pBunch->data_from_stream = 0;
        pBunch->error_code = 0;
        pBunch->error_origin = 0;
        memset( pBunch->pactual_bitrate, 0,
                pBunch->cntBitrate*sizeof(*pBunch->pactual_bitrate) );
        memset( pBunch->psignal, 0,
                pSscBunch->nChannels*pSscBunch->nSubframeSize*sizeof(*pBunch->psignal) );   /* [subframesize, channels] */
        memset( pBunch->psynth,  0,
                pSscBunch->nChannels*pSscBunch->nSubframeSize*sizeof(*pBunch->psynth) );    /* reconstructed signal from module */
        memset( pBunch->presidual,  0,
                pSscBunch->nChannels*pSscBunch->nSubframeSize*sizeof(*pBunch->presidual) ); /* residual signal */
        /* transient [channels] parameters */
        pBunch->pTroot->pTparms[nCh].cntTSparms = 1 ;  
        pBunch->pTroot->pTparms[nCh].dBpar      = 0 ;
        pBunch->pTroot->pTparms[nCh].dChi       = 0 ;  
        pBunch->pTroot->pTparms[nCh].bTransient = False ;
        pBunch->pTroot->pTparms[nCh].nPosition  = 0 ;  
        pBunch->pTroot->pTparms[nCh].nTran_type = 0 ;  
        memset( pBunch->pTroot->pTparms[nCh].pTSparms->pdAmp, 0, 
                SSC_T_MAX_MEIXNER_SINUSOIDS * sizeof( *pBunch->pTroot->pTparms[nCh].pTSparms->pdAmp) );
        memset( pBunch->pTroot->pTparms[nCh].pTSparms->pdFreq, 0,
                SSC_T_MAX_MEIXNER_SINUSOIDS * sizeof( *pBunch->pTroot->pTparms[nCh].pTSparms->pdFreq) );
        memset( pBunch->pTroot->pTparms[nCh].pTSparms->pdPhase, 0, 
                SSC_T_MAX_MEIXNER_SINUSOIDS * sizeof( *pBunch->pTroot->pTparms[nCh].pTSparms->pdPhase) );
        pBunch->pTroot->pTparms[nCh].pTSparms->nr_of_sins = 0 ;  
        pBunch->pTroot->pTparms[nCh].pTSparms->cntAmp     = SSC_T_MAX_MEIXNER_SINUSOIDS ;  
        pBunch->pTroot->pTparms[nCh].pTSparms->cntFreq    = SSC_T_MAX_MEIXNER_SINUSOIDS ;
        pBunch->pTroot->pTparms[nCh].pTSparms->cntPhase   = SSC_T_MAX_MEIXNER_SINUSOIDS ; 

        /* [channels, nSinusoids] sinusoidal parameters */
        pBunch->pSroot->cntSparms  = pSscBunch->nChannels;
        pBunch->pSroot->cntSQparms = pSscBunch->nChannels ;

        pBunch->pSroot->pSparms[nCh].cntTrackIdx = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntTrackPrev = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntTrackNext = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntFreq = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntAmp = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntPhaseOrg = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntPhaseCont = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntRel = nSinusoids ;
        pBunch->pSroot->pSparms[nCh].cntQuantGrid = nSinusoids ;  

        memset( pBunch->pSroot->pSparms[nCh].pdTrackIdx, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdTrackIdx));
        for( nTrack=0; nTrack<nSinusoids; nTrack++ )
        {
            pBunch->pSroot->pSparms[nCh].pdTrackPrev[nTrack] = -1.0 ; /* 0 based index, -1 means no track */
            pBunch->pSroot->pSparms[nCh].pdTrackNext[nTrack] = -1.0 ; /* 0 based index, -1 means no track */
        }
        memset( pBunch->pSroot->pSparms[nCh].pdFreq, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdFreq));
        memset( pBunch->pSroot->pSparms[nCh].pdAmp, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdAmp));
        memset( pBunch->pSroot->pSparms[nCh].pdPhaseOrg, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdPhaseOrg));
        memset( pBunch->pSroot->pSparms[nCh].pdPhaseCont, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdPhaseCont));
        memset( pBunch->pSroot->pSparms[nCh].pdRel, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdRel));
        memset( pBunch->pSroot->pSparms[nCh].pdQuantGrid, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSparms[nCh].pdQuantGrid));
        pBunch->pSroot->pSparms[nCh].nGridFreq = 0 ;
        pBunch->pSroot->pSparms[nCh].nGridAmp = 0 ;
        pBunch->pSroot->pSparms[nCh].dRelThreshold = 0 ;

        pBunch->pSroot->pSQparms[nCh].cntTrackIdx = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntTrackPrev = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntTrackNext = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntFreq = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntAmp = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntPhaseOrg = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntPhaseCont = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntRel = nSinusoids ;
        pBunch->pSroot->pSQparms[nCh].cntQuantGrid = nSinusoids ;

        memset( pBunch->pSroot->pSQparms[nCh].pdTrackIdx, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdTrackIdx));
        for( nTrack=0; nTrack<nSinusoids; nTrack++ )
        {
            pBunch->pSroot->pSQparms[nCh].pdTrackPrev[nTrack] = -1.0 ; /* 0 based index, -1 means no track */
            pBunch->pSroot->pSQparms[nCh].pdTrackNext[nTrack] = -1.0 ; /* 0 based index, -1 means no track */
        }
        memset( pBunch->pSroot->pSQparms[nCh].pdFreq, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdFreq));
        memset( pBunch->pSroot->pSQparms[nCh].pdAmp, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdAmp));
        memset( pBunch->pSroot->pSQparms[nCh].pdPhaseOrg, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdPhaseOrg));
        memset( pBunch->pSroot->pSQparms[nCh].pdPhaseCont, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdPhaseCont));
        memset( pBunch->pSroot->pSQparms[nCh].pdRel, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdRel));
        memset( pBunch->pSroot->pSQparms[nCh].pdQuantGrid, 0,
                nSinusoids*sizeof(*pBunch->pSroot->pSQparms[nCh].pdQuantGrid));
        pBunch->pSroot->pSQparms[nCh].nGridFreq = 0 ;
        pBunch->pSroot->pSQparms[nCh].nGridAmp = 0 ;
        pBunch->pSroot->pSQparms[nCh].dRelThreshold = 0 ;

        /* noise [channels] parameters */
        pBunch->pNroot->cntNparms  = pSscBunch->nChannels ;

        pBunch->pNroot->pNparms[nCh].cntDen      = nDen ;
        pBunch->pNroot->pNparms[nCh].cntEnvCoefs = SSC_N_MAX_LSF_ORDER;
        
        pBunch->pNroot->pNparms[nCh].dEnvGain    = 0 ;
        pBunch->pNroot->pNparms[nCh].envOverlap  = 0 ;

        pBunch->pNroot->pNparms[nCh].nGridLaguerre = 0 ;
        pBunch->pNroot->pNparms[nCh].dPoleLaguerre = 0.0 ;

        memset( pBunch->pNroot->pNparms[nCh].pdDen, 0,
                nDen*sizeof(*pBunch->pNroot->pNparms[nCh].pdDen));
        memset( pBunch->pNroot->pNparms[nCh].pdEnvCoefs, 0,
                SSC_N_MAX_LSF_ORDER*sizeof(*pBunch->pNroot->pNparms[nCh].pdEnvCoefs));


        pBunch->pNroot->cntNQparms = pSscBunch->nChannels ;

        pBunch->pNroot->pNQparms[nCh].cntDen      = nDen ;
        pBunch->pNroot->pNQparms[nCh].cntEnvCoefs = SSC_N_MAX_LSF_ORDER;
        
        pBunch->pNroot->pNQparms[nCh].dEnvGain    = 0 ;
        pBunch->pNroot->pNQparms[nCh].envOverlap  = 0 ;

        pBunch->pNroot->pNQparms[nCh].nGridLaguerre = 0 ;
        pBunch->pNroot->pNQparms[nCh].dPoleLaguerre = 0.0 ;

        memset( pBunch->pNroot->pNQparms[nCh].pdDen, 0,
                nDen*sizeof(*pBunch->pNroot->pNQparms[nCh].pdDen));
        memset( pBunch->pNroot->pNQparms[nCh].pdEnvCoefs, 0,
                SSC_N_MAX_LSF_ORDER*sizeof(*pBunch->pNroot->pNQparms[nCh].pdEnvCoefs));

        /* for debugging and range checking */
        pBunch->cntSignal   = pSscBunch->nChannels*nSfSize ;    /* psignal element count */
        pBunch->cntSynth    = pSscBunch->nChannels*nSfSize ;    /* psynth element count */
        pBunch->cntResidual = pSscBunch->nChannels*nSfSize ;    /* presidual element count */
        pBunch->cntTroot = 1 ;   /* transient info element count */
        pBunch->cntSroot = 1 ;   /* sinusoidal info element count */
        pBunch->cntNroot = 1 ;   /* noise info element count */
        pBunch->cntSTroot= 1 ;   /* stereo info element count */
    }

    /* stereo params */
    pBunch->pSTroot->pSTparms->envBorder   = 0;
    
    pBunch->pSTroot->pSTparms->header    = False;
    pBunch->pSTroot->pSTparms->iidEnable = False;
    pBunch->pSTroot->pSTparms->iccEnable = False;
    pBunch->pSTroot->pSTparms->ipdEnable = False;
    pBunch->pSTroot->pSTparms->extEnable = False;
    pBunch->pSTroot->pSTparms->pcaEnable = False;
    pBunch->pSTroot->pSTparms->iidFreqRes = 0;
    pBunch->pSTroot->pSTparms->iidQuant   = 0;
    pBunch->pSTroot->pSTparms->iccFreqRes = 0;

    memset( pBunch->pSTroot->pSTparms->iid, 0, SSC_OCS_MAX_IID * sizeof(*pBunch->pSTroot->pSTparms->iid) );
    memset( pBunch->pSTroot->pSTparms->ipd, 0, SSC_OCS_MAX_IPD * sizeof(*pBunch->pSTroot->pSTparms->ipd) );
    memset( pBunch->pSTroot->pSTparms->opd, 0, SSC_OCS_MAX_OPD * sizeof(*pBunch->pSTroot->pSTparms->opd) );
    memset( pBunch->pSTroot->pSTparms->icc, 0, SSC_OCS_MAX_ICC * sizeof(*pBunch->pSTroot->pSTparms->icc) );

    pBunch->pSTroot->pSTQparms->envBorder   = 0;
    
    pBunch->pSTroot->pSTQparms->header    = False;
    pBunch->pSTroot->pSTQparms->iidEnable = False;
    pBunch->pSTroot->pSTQparms->iccEnable = False;
    pBunch->pSTroot->pSTQparms->ipdEnable = False;
    pBunch->pSTroot->pSTQparms->extEnable = False;
    pBunch->pSTroot->pSTQparms->pcaEnable = False;
    pBunch->pSTroot->pSTQparms->iidFreqRes = 0;
    pBunch->pSTroot->pSTQparms->iidQuant   = 0;
    pBunch->pSTroot->pSTQparms->iccFreqRes = 0;

    memset( pBunch->pSTroot->pSTQparms->iid, 0, SSC_OCS_MAX_IID * sizeof(*pBunch->pSTroot->pSTQparms->iid) );
    memset( pBunch->pSTroot->pSTQparms->ipd, 0, SSC_OCS_MAX_IPD * sizeof(*pBunch->pSTroot->pSTQparms->ipd) );
    memset( pBunch->pSTroot->pSTQparms->opd, 0, SSC_OCS_MAX_OPD * sizeof(*pBunch->pSTroot->pSTQparms->opd) );
    memset( pBunch->pSTroot->pSTQparms->icc, 0, SSC_OCS_MAX_ICC * sizeof(*pBunch->pSTroot->pSTQparms->icc) );
}


SSC_INTF_EXPORT void SSCBUNCH_Copy( SSCBUNCH *pSscBunchDst, SSCBUNCH *pSscBunchSrc )
{
    SInt nCh, nTraceLevel ;

    #ifdef _DEBUG
        assert( pSscBunchDst );
        assert( pSscBunchSrc );
    #endif

    SSCLOG_TRACE0( pSscBunchDst->pLog, "SSCBunch_Copy: sf_counter( from %d to %d)\n", 
                  pSscBunchSrc->pBunch->sf_counter, pSscBunchDst->pBunch->sf_counter );
    nTraceLevel = SSCLOG_GETTRACELEVEL( pSscBunchDst->pLog );
    SSCLOG_SETTRACELEVEL( pSscBunchDst->pLog, 10 ); /* disable logging */

    pSscBunchDst->nChannels = pSscBunchSrc->nChannels ;
    pSscBunchDst->nSubframeSize = pSscBunchSrc->nSubframeSize ;
    for( nCh=0; nCh<pSscBunchSrc->nChannels; nCh++ )
    {
        SSCBUNCH_SetDataFromStream(pSscBunchDst, SSCBUNCH_GetDataFromStream(pSscBunchSrc));
        SSCBUNCH_SetSfCounter(pSscBunchDst, SSCBUNCH_GetSfCounter(pSscBunchSrc));
        SSCBUNCH_SetErrorCode(pSscBunchDst, SSCBUNCH_GetErrorCode(pSscBunchSrc));
        SSCBUNCH_SetErrorOrigin(pSscBunchDst, SSCBUNCH_GetErrorOrigin(pSscBunchSrc));
        SSCBUNCH_SetActualBitrate(pSscBunchDst, SSCBUNCH_GetActualBitrate(pSscBunchSrc));
        
        SSCBUNCH_SetSignal(pSscBunchDst, nCh, SSCBUNCH_GetSignal(pSscBunchSrc, nCh));
        SSCBUNCH_SetSynthesized(pSscBunchDst, nCh, SSCBUNCH_GetSynthesized(pSscBunchSrc, nCh));
        SSCBUNCH_SetResidual(pSscBunchDst, nCh, SSCBUNCH_GetResidual(pSscBunchSrc, nCh));

        SSCBUNCH_SetTransientPresent(pSscBunchDst, nCh, SSCBUNCH_GetTransientPresent(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientPresent(pSscBunchDst, nCh, SSCBUNCH_GetQTransientPresent(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientPos(pSscBunchDst, nCh, SSCBUNCH_GetTransientPos(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientPos(pSscBunchDst, nCh, SSCBUNCH_GetQTransientPos(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientType(pSscBunchDst, nCh, SSCBUNCH_GetTransientType(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientType(pSscBunchDst, nCh, SSCBUNCH_GetQTransientType(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientBpar(pSscBunchDst, nCh, SSCBUNCH_GetTransientBpar(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientBpar(pSscBunchDst, nCh, SSCBUNCH_GetQTransientBpar(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientChi(pSscBunchDst, nCh, SSCBUNCH_GetTransientChi(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientChi(pSscBunchDst, nCh, SSCBUNCH_GetQTransientChi(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientMeixnerSins(pSscBunchDst, nCh, SSCBUNCH_GetTransientMeixnerSins(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientMeixnerSins(pSscBunchDst, nCh, SSCBUNCH_GetQTransientMeixnerSins(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientMeixnerFreq(pSscBunchDst, nCh, SSCBUNCH_GetTransientMeixnerFreq(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientMeixnerFreq(pSscBunchDst, nCh, SSCBUNCH_GetQTransientMeixnerFreq(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientMeixnerAmp(pSscBunchDst, nCh, SSCBUNCH_GetTransientMeixnerAmp(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientMeixnerAmp(pSscBunchDst, nCh, SSCBUNCH_GetQTransientMeixnerAmp(pSscBunchSrc, nCh));
        SSCBUNCH_SetTransientMeixnerPhase(pSscBunchDst, nCh, SSCBUNCH_GetTransientMeixnerPhase(pSscBunchSrc, nCh));
        SSCBUNCH_SetQTransientMeixnerPhase(pSscBunchDst, nCh, SSCBUNCH_GetQTransientMeixnerPhase(pSscBunchSrc, nCh));

        SSCBUNCH_SetSinusoidTrackIdx(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidTrackIdx(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidTrackIdx(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidTrackIdx(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidTrackPrev(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidTrackPrev(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidTrackPrev(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidTrackPrev(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidTrackNext(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidTrackNext(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidTrackNext(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidTrackNext(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidFreq(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidFreq(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidFreq(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidFreq(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidAmp(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidAmp(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidAmp(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidAmp(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidPhaseOrg(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidPhaseOrg(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidPhaseOrg(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidPhaseOrg(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidPhaseCont(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidPhaseCont(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidPhaseCont(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidPhaseCont(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidRelevance(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidRelevance(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidRelevance(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidRelevance(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidQuantGrid(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidQuantGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidQuantGrid(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidQuantGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidFreqGrid(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidFreqGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidFreqGrid(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidFreqGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidAmpGrid(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidAmpGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidAmpGrid(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidAmpGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidPhaseJitterBand(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidPhaseJitterBand(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidPhaseJitterBand(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidPhaseJitterBand(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidPhaseJitterPercentage(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidPhaseJitterPercentage(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidPhaseJitterPercentage(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidPhaseJitterPercentage(pSscBunchSrc, nCh));
        SSCBUNCH_SetSinusoidRelevanceThreshold(pSscBunchDst, nCh, SSCBUNCH_GetSinusoidRelevanceThreshold(pSscBunchSrc, nCh));
        SSCBUNCH_SetQSinusoidRelevanceThreshold(pSscBunchDst, nCh, SSCBUNCH_GetQSinusoidRelevanceThreshold(pSscBunchSrc, nCh));

        SSCBUNCH_SetNoiseDen(pSscBunchDst, nCh, SSCBUNCH_GetNoiseDen(pSscBunchSrc, nCh));
        SSCBUNCH_SetQNoiseDen(pSscBunchDst, nCh, SSCBUNCH_GetQNoiseDen(pSscBunchSrc, nCh));
        SSCBUNCH_SetNoiseEnvCoefs(pSscBunchDst, nCh, SSCBUNCH_GetNoiseEnvCoefs(pSscBunchSrc, nCh));
        SSCBUNCH_SetQNoiseEnvCoefs(pSscBunchDst, nCh, SSCBUNCH_GetQNoiseEnvCoefs(pSscBunchSrc, nCh));
        SSCBUNCH_SetNoiseEnvGain(pSscBunchDst, nCh, SSCBUNCH_GetNoiseEnvGain(pSscBunchSrc, nCh));
        SSCBUNCH_SetQNoiseEnvGain(pSscBunchDst, nCh, SSCBUNCH_GetQNoiseEnvGain(pSscBunchSrc, nCh));
        SSCBUNCH_SetNoiseEnvOverlap(pSscBunchDst, nCh, SSCBUNCH_GetNoiseEnvOverlap(pSscBunchSrc, nCh));
        SSCBUNCH_SetQNoiseEnvOverlap(pSscBunchDst, nCh, SSCBUNCH_GetQNoiseEnvOverlap(pSscBunchSrc, nCh));
        SSCBUNCH_SetNoiseLaguerrePole(pSscBunchDst, nCh, SSCBUNCH_GetNoiseLaguerrePole(pSscBunchSrc, nCh));
        SSCBUNCH_SetQNoiseLaguerrePole(pSscBunchDst, nCh, SSCBUNCH_GetQNoiseLaguerrePole(pSscBunchSrc, nCh));
        SSCBUNCH_SetNoiseLaguerreGrid(pSscBunchDst, nCh, SSCBUNCH_GetNoiseLaguerreGrid(pSscBunchSrc, nCh));
        SSCBUNCH_SetQNoiseLaguerreGrid(pSscBunchDst, nCh, SSCBUNCH_GetQNoiseLaguerreGrid(pSscBunchSrc, nCh));
    }
    /* stereo */

    SSCBUNCH_SetStereoEnvBorder        (pSscBunchDst, SSCBUNCH_GetStereoEnvBorder(pSscBunchSrc));    
    SSCBUNCH_SetQStereoEnvBorder       (pSscBunchDst, SSCBUNCH_GetQStereoEnvBorder(pSscBunchSrc));    
    
    SSCBUNCH_SetStereoHeaderFlag       (pSscBunchDst, SSCBUNCH_GetStereoHeaderFlag(pSscBunchSrc));      
    SSCBUNCH_SetQStereoHeaderFlag      (pSscBunchDst, SSCBUNCH_GetQStereoHeaderFlag(pSscBunchSrc));      
    SSCBUNCH_SetStereoIidFlag          (pSscBunchDst, SSCBUNCH_GetStereoIidFlag(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIidFlag         (pSscBunchDst, SSCBUNCH_GetQStereoIidFlag(pSscBunchSrc));      
    SSCBUNCH_SetStereoIccFlag          (pSscBunchDst, SSCBUNCH_GetStereoIccFlag(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIccFlag         (pSscBunchDst, SSCBUNCH_GetQStereoIccFlag(pSscBunchSrc));      
    SSCBUNCH_SetStereoIpdFlag          (pSscBunchDst, SSCBUNCH_GetStereoIpdFlag(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIpdFlag         (pSscBunchDst, SSCBUNCH_GetQStereoIpdFlag(pSscBunchSrc));      
    SSCBUNCH_SetStereoExtFlag          (pSscBunchDst, SSCBUNCH_GetStereoExtFlag(pSscBunchSrc));      
    SSCBUNCH_SetQStereoExtFlag         (pSscBunchDst, SSCBUNCH_GetQStereoExtFlag(pSscBunchSrc));      
    SSCBUNCH_SetStereoPcaFlag          (pSscBunchDst, SSCBUNCH_GetStereoPcaFlag(pSscBunchSrc));      
    SSCBUNCH_SetQStereoPcaFlag         (pSscBunchDst, SSCBUNCH_GetQStereoPcaFlag(pSscBunchSrc));      

    SSCBUNCH_SetStereoIidFreqRes       (pSscBunchDst, SSCBUNCH_GetStereoIidFreqRes(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIidFreqRes      (pSscBunchDst, SSCBUNCH_GetQStereoIidFreqRes(pSscBunchSrc));      
    SSCBUNCH_SetStereoIidQuant         (pSscBunchDst, SSCBUNCH_GetStereoIidQuant(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIidQuant        (pSscBunchDst, SSCBUNCH_GetQStereoIidQuant(pSscBunchSrc));      
    SSCBUNCH_SetStereoIccFreqRes       (pSscBunchDst, SSCBUNCH_GetStereoIccFreqRes(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIccFreqRes      (pSscBunchDst, SSCBUNCH_GetQStereoIccFreqRes(pSscBunchSrc));      

    SSCBUNCH_SetStereoIid              (pSscBunchDst, SSCBUNCH_GetStereoIid(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIid             (pSscBunchDst, SSCBUNCH_GetQStereoIid(pSscBunchSrc));      
    SSCBUNCH_SetStereoIcc              (pSscBunchDst, SSCBUNCH_GetStereoIcc(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIcc             (pSscBunchDst, SSCBUNCH_GetQStereoIcc(pSscBunchSrc));      
    SSCBUNCH_SetStereoIpd              (pSscBunchDst, SSCBUNCH_GetStereoIpd(pSscBunchSrc));      
    SSCBUNCH_SetQStereoIpd             (pSscBunchDst, SSCBUNCH_GetQStereoIpd(pSscBunchSrc));      
    SSCBUNCH_SetStereoOpd              (pSscBunchDst, SSCBUNCH_GetStereoOpd(pSscBunchSrc));      
    SSCBUNCH_SetQStereoOpd             (pSscBunchDst, SSCBUNCH_GetQStereoOpd(pSscBunchSrc));      

    SSCLOG_SETTRACELEVEL( pSscBunchDst->pLog, nTraceLevel ); /* restore setting */
}

/******************************************************/
/* member access functions                            */
/******************************************************/

SSC_INTF_EXPORT SInt SSCBUNCH_GetSfCounter( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSfCounter(%d) returned %d\n", pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->sf_counter );
    return pSscBunch->pBunch->sf_counter ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSfCounter( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSfCounter(%d) to %d\n", pSscBunch->pBunch->sf_counter, nNewValue );
    pSscBunch->pBunch->sf_counter = nNewValue;
}


SSC_INTF_EXPORT Bool SSCBUNCH_GetDataFromStream( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetDataFromStream(%d) returned %d\n", pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->data_from_stream );
    return pSscBunch->pBunch->data_from_stream ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetDataFromStream( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetDataFromStream(%d) to %d\n", pSscBunch->pBunch->sf_counter, nNewValue );
    pSscBunch->pBunch->data_from_stream = nNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetErrorCode( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetErrorCode(%d) returned %d\n", pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->error_code );
    return pSscBunch->pBunch->error_code ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetErrorCode( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetErrorCode(%d) to %d\n", pSscBunch->pBunch->sf_counter, nNewValue );
    pSscBunch->pBunch->error_code = nNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetErrorOrigin( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetErrorOrigin(%d) returned %d\n", pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->error_origin );
    return pSscBunch->pBunch->error_origin ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetErrorOrigin( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetErrorOrigin(%d) to %d\n", pSscBunch->pBunch->sf_counter, nNewValue );
    pSscBunch->pBunch->error_origin = nNewValue;
}

Double *SSCBUNCH_GetActualBitrate( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetActualBitrate(%d)\n", pSscBunch->pBunch->sf_counter );
    return pSscBunch->pBunch->pactual_bitrate ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetActualBitrate( SSCBUNCH *pSscBunch, Double *pNewData )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetActualBitrate(%d)\n", pSscBunch->pBunch->sf_counter);
    memmove( pSscBunch->pBunch->pactual_bitrate, pNewData,
             pSscBunch->pBunch->cntBitrate*sizeof(*pSscBunch->pBunch->pactual_bitrate)) ;
}

Double *SSCBUNCH_GetSignal( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntSignal/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSignal(%d) channel %d\n", pSscBunch->pBunch->sf_counter, nChannel );
    return &pSscBunch->pBunch->psignal[nChannel*pSscBunch->nSubframeSize];
}


SSC_INTF_EXPORT void SSCBUNCH_SetSignal( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewData )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntSignal/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSignal(%d) channel %d\n", pSscBunch->pBunch->sf_counter, nChannel );
    memmove( &pSscBunch->pBunch->psignal[nChannel*pSscBunch->nSubframeSize], pNewData,
              pSscBunch->nSubframeSize*sizeof(*pSscBunch->pBunch->psignal)) ;
}

Double *SSCBUNCH_GetSynthesized( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntSynth/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSynthesized(%d) channel %d\n", pSscBunch->pBunch->sf_counter, nChannel );
    return &pSscBunch->pBunch->psynth[nChannel*pSscBunch->nSubframeSize];
}


SSC_INTF_EXPORT void SSCBUNCH_SetSynthesized( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewData )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntSynth/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSynthesized(%d) channel %d\n", pSscBunch->pBunch->sf_counter, nChannel );
    memmove( &pSscBunch->pBunch->psynth[nChannel*pSscBunch->nSubframeSize], pNewData,
              pSscBunch->nSubframeSize*sizeof(*pSscBunch->pBunch->psynth)) ;
}

Double *SSCBUNCH_GetResidual( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntResidual/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetResidual(%d) channel %d\n", pSscBunch->pBunch->sf_counter, nChannel );
    return &pSscBunch->pBunch->presidual[nChannel*pSscBunch->nSubframeSize];
}


SSC_INTF_EXPORT void SSCBUNCH_SetResidual( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewData )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntResidual/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetResidual(%d) channel %d\n", pSscBunch->pBunch->sf_counter, nChannel );
    memmove( &pSscBunch->pBunch->presidual[nChannel*pSscBunch->nSubframeSize], pNewData,
              pSscBunch->nSubframeSize*sizeof(*pSscBunch->pBunch->presidual)) ;
}

/* demultiplex input buffer and write it to the residual buffer */
/* pNewData points to the first sample of the correct channel, so only do the correct increment */
SSC_INTF_EXPORT void SSCBUNCH_SetDemuxResidual( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewData, SInt nChannels )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->cntResidual/pSscBunch->nSubframeSize > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SeDemuxResidual(%d) channel %d, channels %d\n", pSscBunch->pBunch->sf_counter, nChannel, nChannels );
    {
        SInt i;
        Double *pSrc = pNewData ;
        Double *pDst = &pSscBunch->pBunch->presidual[nChannel*pSscBunch->nSubframeSize] ;

        for( i=0; i<pSscBunch->nSubframeSize; i++ )
        {
            *pDst = *pSrc ;
            pSrc += nChannels ;
            pDst += 1 ;
        }
    }
}


SSC_INTF_EXPORT Bool SSCBUNCH_GetTransientPresent( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientPresent(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTparms[nChannel].bTransient );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].bTransient ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientPresent( SSCBUNCH *pSscBunch, SInt nChannel, Bool bNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientPresent(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, bNewValue );
    pSscBunch->pBunch->pTroot->pTparms[nChannel].bTransient = bNewValue ;
}


SSC_INTF_EXPORT Bool SSCBUNCH_GetQTransientPresent( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientPresent(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].bTransient );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].bTransient ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientPresent( SSCBUNCH *pSscBunch, SInt nChannel, Bool bNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientPresent(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, bNewValue );
    pSscBunch->pBunch->pTroot->pTQparms[nChannel].bTransient = bNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetTransientPos( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientPos(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTparms[nChannel].nPosition );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].nPosition ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientPos( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientPos(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pTroot->pTparms[nChannel].nPosition = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQTransientPos( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientPos(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].nPosition );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].nPosition ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientPos( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientPos(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pTroot->pTQparms[nChannel].nPosition = nNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetTransientType( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientType(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTparms[nChannel].nTran_type );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].nTran_type ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientType( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientType(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pTroot->pTparms[nChannel].nTran_type = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQTransientType( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientType(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].nTran_type );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].nTran_type ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientType( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientType(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pTroot->pTQparms[nChannel].nTran_type = nNewValue;
}


SSC_INTF_EXPORT Double SSCBUNCH_GetTransientBpar( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientBpar(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTparms[nChannel].dBpar );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].dBpar ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientBpar( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientBpar(%d) channel %d to %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue);
    pSscBunch->pBunch->pTroot->pTparms[nChannel].dBpar = dNewValue;
}


SSC_INTF_EXPORT Double SSCBUNCH_GetQTransientBpar( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientBpar(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].dBpar );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].dBpar ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientBpar( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientBpar(%d) channel %d to %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pTroot->pTQparms[nChannel].dBpar = dNewValue;
}


SSC_INTF_EXPORT Double SSCBUNCH_GetTransientChi( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientChi(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTparms[nChannel].dChi );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].dChi ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientChi( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientChi(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pTroot->pTparms[nChannel].dChi = dNewValue ;
}


SSC_INTF_EXPORT Double SSCBUNCH_GetQTransientChi( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientChi(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].dChi );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].dChi ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientChi( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientChi(%d) channel %d to %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pTroot->pTQparms[nChannel].dChi = dNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetTransientMeixnerSins( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientMeixnerSins(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->nr_of_sins );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->nr_of_sins ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerSins( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientMeixnerSins(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->nr_of_sins = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQTransientMeixnerSins( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientMeixnerSins(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerSins( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientMeixnerSins(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins );
    pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins = nNewValue ;
}

Double *SSCBUNCH_GetTransientMeixnerFreq( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientMeixnerFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdFreq ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerFreq( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientMeixnerFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdFreq, pNewValue,
             pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->nr_of_sins *
             sizeof(*pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdFreq) );
}

Double *SSCBUNCH_GetQTransientMeixnerFreq( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientMeixnerFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdFreq ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerFreq( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientMeixnerFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdFreq, pNewValue,
             pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins *
             sizeof(*pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdFreq) );
}

Double *SSCBUNCH_GetTransientMeixnerAmp( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientMeixnerAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdAmp ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerAmp( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientMeixnerAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdAmp, pNewValue,
             pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->nr_of_sins *
             sizeof(*pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdAmp) );
}

Double *SSCBUNCH_GetQTransientMeixnerAmp( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientMeixnerAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdAmp ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerAmp( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQTransientMeixnerAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdAmp, pNewValue,
             pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins *
             sizeof(*pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdAmp) );
}

Double *SSCBUNCH_GetTransientMeixnerPhase( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetTransientMeixnerPhase(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdPhase ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerPhase( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTparms );
        assert( pSscBunch->pBunch->pTroot->cntTparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetTransientMeixnerPhase(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdPhase, pNewValue,
             pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->nr_of_sins *
             sizeof(*pSscBunch->pBunch->pTroot->pTparms[nChannel].pTSparms->pdPhase) );
}

Double *SSCBUNCH_GetQTransientMeixnerPhase( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientMeixnerPhase(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdPhase ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerPhase( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pTroot );
        assert( pSscBunch->pBunch->pTroot->pTQparms );
        assert( pSscBunch->pBunch->pTroot->cntTQparms > nChannel );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms );
        assert( pSscBunch->pBunch->pTroot->pTQparms[nChannel].cntTSparms == 1 );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQTransientMeixnerPhase(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdPhase, pNewValue,
             pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->nr_of_sins *
             sizeof(*pSscBunch->pBunch->pTroot->pTQparms[nChannel].pTSparms->pdPhase) );
}

Double *SSCBUNCH_GetSinusoidTrackIdx( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidTrackIdx(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackIdx ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidTrackIdx( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidTrackIdx(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackIdx, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntTrackIdx *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackIdx) );
}

Double *SSCBUNCH_GetQSinusoidTrackIdx( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidTrackIdx(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackIdx ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidTrackIdx( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidTrackIdx(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackIdx, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntTrackIdx *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackIdx) );
}

Double *SSCBUNCH_GetSinusoidTrackPrev( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidTrackPrev(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackPrev ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidTrackPrev( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidTrackPrev(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackPrev, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntTrackPrev *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackPrev) );
}

Double *SSCBUNCH_GetQSinusoidTrackPrev( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidTrackPrev(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackPrev ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidTrackPrev( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidTrackPrev(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackPrev, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntTrackPrev *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackPrev) );
}

Double *SSCBUNCH_GetSinusoidTrackNext( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidTrackNext(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackNext ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidTrackNext( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidTrackNext(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackNext, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntTrackNext *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdTrackNext) );
}

Double *SSCBUNCH_GetQSinusoidTrackNext( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidTrackNext(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackNext ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidTrackNext( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidTrackNext(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackNext, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntTrackNext *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdTrackNext) );
}

Double *SSCBUNCH_GetSinusoidFreq( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdFreq ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidFreq( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdFreq, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntFreq *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdFreq) );
}

Double *SSCBUNCH_GetQSinusoidFreq( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdFreq ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidFreq( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidFreq(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdFreq, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntFreq *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdFreq) );
}

Double *SSCBUNCH_GetSinusoidAmp( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdAmp ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidAmp( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdAmp, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntAmp *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdAmp) );
}

Double *SSCBUNCH_GetQSinusoidAmp( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdAmp ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidAmp( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidAmp(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdAmp, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntAmp *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdAmp) );
}

Double *SSCBUNCH_GetSinusoidPhaseOrg( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidPhaseOrg(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdPhaseOrg ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseOrg( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidPhaseOrg(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdPhaseOrg, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntPhaseOrg *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdPhaseOrg) );
}

Double *SSCBUNCH_GetQSinusoidPhaseOrg( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidPhaseOrg(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdPhaseOrg ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseOrg( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidPhaseOrg(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdPhaseOrg, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntPhaseOrg *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdPhaseOrg) );
}

Double *SSCBUNCH_GetSinusoidPhaseCont( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidPhaseCont(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdPhaseCont ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseCont( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidPhaseCont(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdPhaseCont, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntPhaseCont *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdPhaseCont) );
}

Double *SSCBUNCH_GetQSinusoidPhaseCont( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidPhaseCont(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdPhaseCont ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseCont( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidPhaseCont(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdPhaseCont, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntPhaseCont *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdPhaseCont) );
}

Double *SSCBUNCH_GetSinusoidRelevance( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidRelevance(%d) channel\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdRel ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidRelevance( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidRelevance(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdRel, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntRel *
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdRel) );
}

Double *SSCBUNCH_GetQSinusoidRelevance( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidRelevance(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdRel ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidRelevance( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidRelevance(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdRel, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntRel *
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdRel) );
}

Double *SSCBUNCH_GetSinusoidQuantGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidQuantGrid(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].pdQuantGrid ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidQuantGrid( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidQuantGrid(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSparms[nChannel].pdQuantGrid, pNewValue,
             pSscBunch->pBunch->pSroot->pSparms[nChannel].cntQuantGrid * 
             sizeof(*pSscBunch->pBunch->pSroot->pSparms[nChannel].pdQuantGrid) );
}

Double *SSCBUNCH_GetQSinusoidQuantGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidQuantGrid(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdQuantGrid ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidQuantGrid( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidQuantGrid(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdQuantGrid, pNewValue,
             pSscBunch->pBunch->pSroot->pSQparms[nChannel].cntQuantGrid * 
             sizeof(*pSscBunch->pBunch->pSroot->pSQparms[nChannel].pdQuantGrid) );
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidFreqGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidFreqGrid(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSparms[nChannel].nGridFreq );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].nGridFreq ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidFreqGrid( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidFreqGrid(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSparms[nChannel].nGridFreq = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidFreqGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidFreqGrid(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSQparms[nChannel].nGridFreq );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].nGridFreq ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidFreqGrid( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidFreqGrid(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSQparms[nChannel].nGridFreq = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidAmpGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidAmpGrid(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSparms[nChannel].nGridAmp );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].nGridAmp ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidAmpGrid( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidAmpGrid(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSparms[nChannel].nGridAmp = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidAmpGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidAmpGrid(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSQparms[nChannel].nGridAmp );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].nGridAmp ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidAmpGrid( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidAmpGrid(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSQparms[nChannel].nGridAmp = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidPhaseJitterBand( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidPhaseJitterBand(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSparms[nChannel].nPhaseJitterBand );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].nPhaseJitterBand ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseJitterBand( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidPhaseJitterBand(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSparms[nChannel].nPhaseJitterBand = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidPhaseJitterBand( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidPhaseJitterBand(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSQparms[nChannel].nPhaseJitterBand );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].nPhaseJitterBand ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseJitterBand( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidPhaseJitterBand(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSQparms[nChannel].nPhaseJitterBand = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidPhaseJitterPercentage( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidPhaseJitterPercentage(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSparms[nChannel].nPhaseJitterPercentage );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].nPhaseJitterPercentage ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseJitterPercentage( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidPhaseJitterPercentage(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSparms[nChannel].nPhaseJitterPercentage = nNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidPhaseJitterPercentage( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidPhaseJitterPercentage(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSQparms[nChannel].nPhaseJitterPercentage );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].nPhaseJitterPercentage ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseJitterPercentage( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidPhaseJitterPercentage(%d) channel %d to %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pSroot->pSQparms[nChannel].nPhaseJitterPercentage = nNewValue ;
}


SSC_INTF_EXPORT Double SSCBUNCH_GetSinusoidRelevanceThreshold( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetSinusoidRelevanceThreshold(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSparms[nChannel].dRelThreshold );
    return pSscBunch->pBunch->pSroot->pSparms[nChannel].dRelThreshold ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidRelevanceThreshold( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSparms );
        assert( pSscBunch->pBunch->pSroot->cntSparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetSinusoidRelevanceThreshold(%d) channel %d to %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pSroot->pSparms[nChannel].dRelThreshold = dNewValue ;
}


SSC_INTF_EXPORT Double SSCBUNCH_GetQSinusoidRelevanceThreshold( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQSinusoidRelevanceThreshold(%d) channel %d returned %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pSroot->pSQparms[nChannel].dRelThreshold );
    return pSscBunch->pBunch->pSroot->pSQparms[nChannel].dRelThreshold ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidRelevanceThreshold( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSroot );
        assert( pSscBunch->pBunch->pSroot->pSQparms );
        assert( pSscBunch->pBunch->pSroot->cntSQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQSinusoidRelevanceThreshold(%d) channel %d to %f\n",
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pSroot->pSQparms[nChannel].dRelThreshold = dNewValue ;
}


Double *SSCBUNCH_GetNoiseDen( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNparms );
        assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseDen(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].pdDen ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetNoiseDen( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNparms );
        assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseDen(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pNroot->pNparms[nChannel].pdDen, pNewValue,
             pSscBunch->pBunch->pNroot->pNparms[nChannel].cntDen *
             sizeof(*pSscBunch->pBunch->pNroot->pNparms[nChannel].pdDen));
}

Double *SSCBUNCH_GetQNoiseDen( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNQparms );
        assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseDen(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].pdDen ;
}


SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseDen( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNQparms );
        assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQNoiseDen(%d) channel %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pNroot->pNQparms[nChannel].pdDen, pNewValue,
             pSscBunch->pBunch->pNroot->pNQparms[nChannel].cntDen *
             sizeof(*pSscBunch->pBunch->pNroot->pNQparms[nChannel].pdDen));
}

Double *SSCBUNCH_GetNoiseEnvCoefs( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseEnvCoefs(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].pdEnvCoefs ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetNoiseEnvCoefs( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseEnvCoefs(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pNroot->pNparms[nChannel].pdEnvCoefs, pNewValue,
             pSscBunch->pBunch->pNroot->pNparms[nChannel].cntEnvCoefs * 
             sizeof(*pSscBunch->pBunch->pNroot->pNparms[nChannel].pdEnvCoefs));
}

Double *SSCBUNCH_GetQNoiseEnvCoefs( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseEnvCoefs(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].pdEnvCoefs ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseEnvCoefs( SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQNoiseEnvCoefs(%d) channel %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel );
    memmove( pSscBunch->pBunch->pNroot->pNQparms[nChannel].pdEnvCoefs, pNewValue,
             pSscBunch->pBunch->pNroot->pNQparms[nChannel].cntEnvCoefs * 
             sizeof(*pSscBunch->pBunch->pNroot->pNQparms[nChannel].pdEnvCoefs));
}

SSC_INTF_EXPORT Double SSCBUNCH_GetNoiseEnvGain( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseEnvGain(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNparms[nChannel].dEnvGain );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].dEnvGain ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetNoiseEnvGain( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseEnvGain(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pNroot->pNparms[nChannel].dEnvGain = dNewValue ;
}

SSC_INTF_EXPORT Double SSCBUNCH_GetQNoiseEnvGain( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseEnvGain(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNQparms[nChannel].dEnvGain );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].dEnvGain ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseEnvGain( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseEnvGain(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pNroot->pNQparms[nChannel].dEnvGain = dNewValue ;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetNoiseEnvOverlap( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseEnvOverlap(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNparms[nChannel].envOverlap );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].envOverlap ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetNoiseEnvOverlap( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseEnvOverlap(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pNroot->pNparms[nChannel].envOverlap = nNewValue ;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQNoiseEnvOverlap( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseEnvOverlap(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNQparms[nChannel].envOverlap );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].envOverlap ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseEnvOverlap( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQNoiseEnvOverlap(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pNroot->pNQparms[nChannel].envOverlap = nNewValue ;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetNoiseLaguerreGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseLaguerreGrid(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNparms[nChannel].nGridLaguerre );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].nGridLaguerre ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetNoiseLaguerreGrid( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseLaguerreGrid(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pNroot->pNparms[nChannel].nGridLaguerre = nNewValue ;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQNoiseLaguerreGrid( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseLaguerreGrid(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNQparms[nChannel].nGridLaguerre );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].nGridLaguerre ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseLaguerreGrid( SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQNoiseLaguerreGrid(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, nNewValue );
    pSscBunch->pBunch->pNroot->pNQparms[nChannel].nGridLaguerre = nNewValue ;
}




SSC_INTF_EXPORT Double SSCBUNCH_GetNoiseLaguerrePole( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseLaguerrePole(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNparms[nChannel].dPoleLaguerre );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].dPoleLaguerre ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetNoiseLaguerrePole( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNparms );
    assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetNoiseLaguerrePole(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pNroot->pNparms[nChannel].dPoleLaguerre = dNewValue ;
}

SSC_INTF_EXPORT Double SSCBUNCH_GetQNoiseLaguerrePole( SSCBUNCH *pSscBunch, SInt nChannel )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseLaguerrePole(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNQparms[nChannel].dPoleLaguerre );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].dPoleLaguerre ;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseLaguerrePole( SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue )
{
    assert( pSscBunch );
    assert( pSscBunch->pBunch );
    assert( pSscBunch->pBunch->pNroot );
    assert( pSscBunch->pBunch->pNroot->pNQparms );
    assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_SetQNoiseLaguerrePole(%d) channel %d returned %f\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, dNewValue );
    pSscBunch->pBunch->pNroot->pNQparms[nChannel].dPoleLaguerre = dNewValue ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetNoiseNrOfDen( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNparms );
        assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseNrOfDen(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNparms[nChannel].cntDen );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].cntDen ;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetQNoiseNrOfDen( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNQparms );
        assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );
    #endif
    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseNrOfDen(%d) channel %d returned %d\n",
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNQparms[nChannel].cntDen );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].cntDen ;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetNoiseNrOfEnvCoefs( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNparms );
        assert( pSscBunch->pBunch->pNroot->cntNparms > nChannel );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetNoiseNrOfEnvCoefs(%d) channel %d returned %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNparms[nChannel].cntEnvCoefs );
    return pSscBunch->pBunch->pNroot->pNparms[nChannel].cntEnvCoefs ;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQNoiseNrOfEnvCoefs( SSCBUNCH *pSscBunch, SInt nChannel )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pNroot );
        assert( pSscBunch->pBunch->pNroot->pNQparms );
        assert( pSscBunch->pBunch->pNroot->cntNQparms > nChannel );
    #endif

    SSCLOG_TRACE0( pSscBunch->pLog, "SSCBunch_GetQNoiseNrOfEnvCoefs(%d) channel %d returned %d\n", 
                  pSscBunch->pBunch->sf_counter, nChannel, pSscBunch->pBunch->pNroot->pNQparms[nChannel].cntEnvCoefs );
    return pSscBunch->pBunch->pNroot->pNQparms[nChannel].cntEnvCoefs ;
}

/* stereo *********************************************** */

SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoEnvBorder( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoEnvBorder(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->envBorder );

    return pSscBunch->pBunch->pSTroot->pSTparms->envBorder;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoEnvBorder( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoEnvBorder(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->envBorder = nNewValue;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoEnvBorder( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQStereoEnvBorder(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->envBorder );

    return pSscBunch->pBunch->pSTroot->pSTQparms->envBorder;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoEnvBorder( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQStereoEnvBorder(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->envBorder = nNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoIidFreqRes( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoIidFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->iidFreqRes );

    return pSscBunch->pBunch->pSTroot->pSTparms->iidFreqRes;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIidFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoIidFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->iidFreqRes = nNewValue;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoIidFreqRes( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQIidFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->iidFreqRes );

    return pSscBunch->pBunch->pSTroot->pSTQparms->iidFreqRes;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIidFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQIidFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->iidFreqRes = nNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoIidQuant( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoIidQuant(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->iidQuant );

    return pSscBunch->pBunch->pSTroot->pSTparms->iidQuant;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIidQuant( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoIidQuant(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->iidQuant = nNewValue;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoIidQuant( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQIidQuant(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->iidQuant );

    return pSscBunch->pBunch->pSTroot->pSTQparms->iidQuant;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIidQuant( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQIidQuant(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->iidQuant = nNewValue;
}


SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoIccFreqRes( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoIccFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->iccFreqRes );

    return pSscBunch->pBunch->pSTroot->pSTparms->iccFreqRes;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIccFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoIccFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->iccFreqRes = nNewValue;
}

SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoIccFreqRes( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQIccFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->iccFreqRes );

    return pSscBunch->pBunch->pSTroot->pSTQparms->iccFreqRes;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIccFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQIccFreqRes(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->iccFreqRes = nNewValue;
}



SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoHeaderFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoHeaderFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->header );

    return pSscBunch->pBunch->pSTroot->pSTparms->header;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoHeaderFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoHeaderFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->header = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoHeaderFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQHeaderFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->header );

    return pSscBunch->pBunch->pSTroot->pSTQparms->header;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoHeaderFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQHeaderFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->header = nNewValue;
}


SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoIidFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoIidFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->iidEnable );

    return pSscBunch->pBunch->pSTroot->pSTparms->iidEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIidFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoIidFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->iidEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoIidFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQIidFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->iidEnable );

    return pSscBunch->pBunch->pSTroot->pSTQparms->iidEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIidFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQIidFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->iidEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoIccFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoIccFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->iccEnable );

    return pSscBunch->pBunch->pSTroot->pSTparms->iccEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIccFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoIccFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->iccEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoIccFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQIccFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->iccEnable );

    return pSscBunch->pBunch->pSTroot->pSTQparms->iccEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIccFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQIccFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->iccEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoIpdFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoIpdFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->ipdEnable );

    return pSscBunch->pBunch->pSTroot->pSTparms->ipdEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIpdFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoIpdFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->ipdEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoIpdFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQIpdFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->ipdEnable );

    return pSscBunch->pBunch->pSTroot->pSTQparms->ipdEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIpdFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQIpdFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->ipdEnable = nNewValue;
}


SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoExtFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoExtFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->extEnable );

    return pSscBunch->pBunch->pSTroot->pSTparms->extEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoExtFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoExtFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->extEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoExtFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQExtFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->extEnable );

    return pSscBunch->pBunch->pSTroot->pSTQparms->extEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoExtFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQExtFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->extEnable = nNewValue;
}


SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoPcaFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetStereoPcaFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTparms->pcaEnable );

    return pSscBunch->pBunch->pSTroot->pSTparms->pcaEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoPcaFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetStereoPcaFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTparms->pcaEnable = nNewValue;
}

SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoPcaFlag( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_GetQPcaFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, pSscBunch->pBunch->pSTroot->pSTQparms->pcaEnable );

    return pSscBunch->pBunch->pSTroot->pSTQparms->pcaEnable;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoPcaFlag( SSCBUNCH *pSscBunch, Bool nNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBunch_SetQPcaFlag(%d) returned %d\n",
                   pSscBunch->pBunch->sf_counter, nNewValue );
    
    pSscBunch->pBunch->pSTroot->pSTQparms->pcaEnable = nNewValue;
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoIid( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetStereoIid(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTparms->iid;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIid( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetStereoIid(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTparms->iid, pNewValue,  
             SSC_OCS_MAX_IID * sizeof(*pSscBunch->pBunch->pSTroot->pSTparms->iid) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoIid( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetQStereoIid(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTQparms->iid;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIid( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetQStereoIid(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTQparms->iid, pNewValue,  
             SSC_OCS_MAX_IID * sizeof(*pSscBunch->pBunch->pSTroot->pSTQparms->iid) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoIcc( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetStereoIcc(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTparms->icc;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIcc( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetStereoIcc(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTparms->icc, pNewValue,  
             SSC_OCS_MAX_ICC * sizeof(*pSscBunch->pBunch->pSTroot->pSTparms->icc) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoIcc( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetQStereoIcc(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTQparms->icc;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIcc( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetQStereoIcc(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTQparms->icc, pNewValue,  
             SSC_OCS_MAX_ICC * sizeof(*pSscBunch->pBunch->pSTroot->pSTQparms->icc) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoIpd( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetStereoIpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTparms->ipd;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIpd( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetStereoIpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTparms->ipd, pNewValue,  
             SSC_OCS_MAX_IPD * sizeof(*pSscBunch->pBunch->pSTroot->pSTparms->ipd) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoIpd( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetQStereoIpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTQparms->ipd;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIpd( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetQStereoIpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTQparms->ipd, pNewValue,  
             SSC_OCS_MAX_IPD * sizeof(*pSscBunch->pBunch->pSTroot->pSTQparms->ipd) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoOpd( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetStereoOpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTparms->opd;
}

SSC_INTF_EXPORT void SSCBUNCH_SetStereoOpd( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetStereoOpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTparms->opd, pNewValue,  
             SSC_OCS_MAX_OPD * sizeof(*pSscBunch->pBunch->pSTroot->pSTparms->opd) ); 
}

SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoOpd( SSCBUNCH *pSscBunch )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_GetQStereoOpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );

    return pSscBunch->pBunch->pSTroot->pSTQparms->opd;
}

SSC_INTF_EXPORT void SSCBUNCH_SetQStereoOpd( SSCBUNCH *pSscBunch, Double* pNewValue )
{
    #ifdef _DEBUG
        assert( pSscBunch );
        assert( pSscBunch->pBunch );
        assert( pSscBunch->pBunch->pSTroot );
        assert( pSscBunch->pBunch->pSTroot->pSTQparms );
    #endif

    SSCLOG_TRACE0 ( pSscBunch->pLog, "SSCBUNCH_SetQStereoOpd(%d)\n",
                   pSscBunch->pBunch->sf_counter );
    
    memmove( pSscBunch->pBunch->pSTroot->pSTQparms->opd, pNewValue,  
             SSC_OCS_MAX_OPD * sizeof(*pSscBunch->pBunch->pSTroot->pSTQparms->opd) ); 
}

