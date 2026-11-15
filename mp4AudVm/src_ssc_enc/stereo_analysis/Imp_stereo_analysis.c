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

Source file: IMP_stereo_analysis.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG: Andy Gerrits,      Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
18-Sep-2002	NT	       Initial version
05-Jan-2004 AG         RM5: Low complexity stereo added
************************************************************************/



/******************************************************************************
 *
 *   Module              : Stereo_Analysis implementation
 *
 *   Description         : Implementation of the stereo_analysis process.
 *
 *   Tools               :
 *
 *   Target Platform     : 
 *
 *   Naming Conventions  : All functions begin with IMP_STEREO_ANALYSIS_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "PlatformTypes.h"

#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_stereo_analysis.h"
#include "SSC_stereo_analysis.h"
#include "Imp_Error.h"

#include "cexcept.h"

#include "SSC_Vector.h"
#include "win.h"

#include "ct_polyphase_ssc.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH   0
#define PIPE_AHEAD_LENGTH  3        

#define OCS_UPDATE                      ( 4 )   /* Update rate in sub-frames for OCS */
#define DELAY_QMF_ANA                   ( 4 )
#define OCS_LOOKAHEAD                   ( 6 )

#define NO_SUB_QMF_CHANNELS_HI_RES      ( 32 )
#define NO_QMF_CHANNELS                 ( 64 )

#define FLOAT_EPSILON                   ( 1E-10 )

#define HYBRID_INVERSE_ORDER            ( 0x0F000000 )
#define HYBRID_INVERSE_MASK             ( ~HYBRID_INVERSE_ORDER )

#define SUBQMF_GROUPS                   ( 10 )
#define QMF_GROUPS                      ( 12 )

#define SUBQMF_GROUPS_HI_RES            ( 32 )
#define QMF_GROUPS_HI_RES               ( 18 )

#define NO_IID_GROUPS                   ( SUBQMF_GROUPS + QMF_GROUPS )
#define NO_IID_GROUPS_HI_RES            ( SUBQMF_GROUPS_HI_RES + QMF_GROUPS_HI_RES )

#define NEGATE_IPD_MASK                 ( 0x00001000 )

#define HYBRID_FILTER_LENGTH            ( 13 )
#define HYBRID_FILTER_DELAY             (  6 )
#define HANDLE_ERROR_INFO               int
#define REAL                            ( 0 )
#define CPLX                            ( 1 )

#define MODE_BANDS20                    (0)
#define MODE_BANDS34                    (1)
#define MODE_BANDS10                    (2)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

static const unsigned int noQmfBandsInHybrid10 = 3;
static const unsigned int noQmfBandsInHybrid20 = 3;
static const unsigned int noQmfBandsInHybrid34 = 5;

static const int aHybridResolution10[] = { HYBRID_6_CPLX,
                                           HYBRID_2_REAL | HYBRID_INVERSE_ORDER,
                                           HYBRID_2_REAL };

static const int aHybridResolution20[] = { HYBRID_8_CPLX,
                                           HYBRID_2_REAL,
                                           HYBRID_2_REAL };

static const int aHybridResolution34[] = { HYBRID_12_CPLX,
                                           HYBRID_8_CPLX,
                                           HYBRID_4_CPLX,
                                           HYBRID_4_CPLX,
                                           HYBRID_4_CPLX };

static const double p8_13_20[13] =
{
  0.00746082949812,  0.02270420949825,  0.04546865930473,  0.07266113929591,
  0.09885108575264,  0.11793710567217,  0.125,             0.11793710567217,
  0.09885108575264,  0.07266113929591,  0.04546865930473,  0.02270420949825,
  0.00746082949812
};


static const double p2_13_20[13] =
{
  0.0,               0.01899487526049,  0.0,              -0.07293139167538,
  0.0,               0.30596630545168,  0.5,               0.30596630545168,
  0.0,              -0.07293139167538,  0.0,               0.01899487526049,
  0.0
};

static const double p12_13_34[13] =
{
  0.04081179924692,  0.03812810994926,  0.05144908135699,  0.06399831151592,
  0.07428313801106,  0.08100347892914,  0.08333333333333,  0.08100347892914,
  0.07428313801106,  0.06399831151592,  0.05144908135699,  0.03812810994926,
  0.04081179924692
};

static const double p8_13_34[13] =
{
  0.01565675600122,  0.03752716391991,  0.05417891378782,  0.08417044116767,
  0.10307344158036,  0.12222452249753,  0.12500000000000,  0.12222452249753,
  0.10307344158036,  0.08417044116767,  0.05417891378782,  0.03752716391991,
  0.01565675600122
};

static const double p4_13_34[13] =
{
 -0.05908211155639, -0.04871498374946,  0.0,               0.07778723915851,
  0.16486303567403,  0.23279856662996,  0.25,              0.23279856662996,
  0.16486303567403,  0.07778723915851,  0.0,              -0.04871498374946,
 -0.05908211155639
};


static const int groupBorders10[NO_IID_GROUPS + 1] =
{
     0,  1,  2,  3,  4,  5, /* 6 subqmf subbands - 0th qmf subband */
     6,  7,                 /* 2 subqmf subbands - 1st qmf subband */
     8,  9,                 /* 2 subqmf subbands - 2nd qmf subband */
     3,  4,  5,  6,  7,  8,  9,  11, 14, 18, 23, 35, 64
};


static const int groupBorders20[NO_IID_GROUPS + 1] =
{
     6,  7,  0,  1,  2,  3, /* 6 subqmf subbands - 0th qmf subband */
     9,  8,                 /* 2 subqmf subbands - 1st qmf subband */
    10, 11,                 /* 2 subqmf subbands - 2nd qmf subband */
     3,  4,  5,  6,  7,  8,  9,  11, 14, 18, 23, 35, 64
};

static const int groupBorders34[NO_IID_GROUPS_HI_RES + 1] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, /* 12 subqmf subbands - 0th qmf subband */
     12, 13, 14, 15, 16, 17, 18, 19,                 /*  8 subqmf subbands - 1st qmf subband */
     20, 21, 22, 23,                                 /*  4 subqmf subbands - 2nd qmf subband */
     24, 25, 26, 27,                                 /*  4 subqmf subbands - 3nd qmf subband */
     28, 29, 30, 31,                                 /*  4 subqmf subbands - 4nd qmf subband */
     32-27, 33-27, 34-27, 35-27, 36-27, 37-27, 38-27, 40-27, 42-27, 44-27, 46-27, 48-27, 51-27, 54-27, 57-27, 60-27, 64-27, 68-27, 91-27
};

static const int bins2groupMap20[NO_IID_GROUPS] =
{
  ( NEGATE_IPD_MASK | 1 ), ( NEGATE_IPD_MASK | 0 ),
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
};

static const int bins2groupMap34[NO_IID_GROUPS_HI_RES] =
{
  0,  1,  2,  3,  4,  5,  6,  6,  7, ( NEGATE_IPD_MASK | 2 ), ( NEGATE_IPD_MASK | 1 ), ( NEGATE_IPD_MASK | 0 ),
  10, 10, 4,  5,  6,  7,  8,  9,
  10, 11, 12, 9,
  14, 11, 12, 13,
  14, 15, 16, 13,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};


/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_SetError(SSCBUNCH* pSscBunch, SSC_ERROR Error);
static void ExtractParams
( 
    SSC_STEREO_ANALYSISSTATE* const pms,
    Double **iBufferLeft, 
    Double **rBufferLeft, 
    Double **iBufferRight,
    Double **rBufferRight,
    Double **iBufferMid,
    Double **rBufferMid
);

static void kChannelFiltering
( 
    const double *pQmfReal,
    const double *pQmfImag,
    double **mHybridReal,
    double **mHybridImag,
    int nSamples,
    int k,
    int bCplx,
    const double *p
);

static void
HybridAnalysis 
( 
    const double **mQmfReal, 
    const double **mQmfImag, 
    double **mHybridReal,    
    double **mHybridImag,    
    HANDLE_HYBRID hHybrid,   
    int usedStereoBands
);    


static HANDLE_ERROR_INFO 
CreateHybridFilterBank ( HANDLE_HYBRID *phHybrid,
                         int frameSize,          
                         int noBands,            
                         const int *pResolution );

static void DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid );



/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_STEREO_ANALYSIS_Initialise(void)
{
}

/* Creates an instance of the stereo_analysis module */
SSC_STEREO_ANALYSISSTATE* IMP_STEREO_ANALYSIS_CreateInstance( const SSC_STEREO_ANALYSISINFO* const pStereo_AnalysisInfo, Char *szLogFile, SInt nTraceLevel )
{
    SSC_STEREO_ANALYSISSTATE*   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;  

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);



    Try
    {
        SInt    ch = 0;
        SInt    i;

        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_STEREO_ANALYSISSTATE), 1, "Stereo_Analysis Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout, "\nError: Unable to alloc requested memory for stereo_analysis instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        /* all struct members are resetted*/
        pms->pLogState = NULL ;
        pms->pPipeState = NULL ;

        for ( ch = 0; ch < pStereo_AnalysisInfo->Channels; ch++)
        {
            pms->ppData[ch] = NULL;
        }

        pms->channels      = pStereo_AnalysisInfo->Channels;
        pms->mode          = pStereo_AnalysisInfo->Mode;
        pms->mode_ext      = pStereo_AnalysisInfo->ModeExt;
        pms->subframeSize  = pStereo_AnalysisInfo->SubframeSize;
        pms->Fs            = pStereo_AnalysisInfo->Fs;

        /* for debugging messages can be routed to file, here this log
         * objects is contructed*/
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
            if( cp != NULL)
            {
                *cp = '\0' ; /* strip extension, if found */
            }
            strncpy( li.szTitle, "SSC_stereo_analysis", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLogState = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLogState == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLogState, "SSCSTEREO_ANALYSIS_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipeState = SSCPIPE_InitInstance( pStereo_AnalysisInfo->Channels, 
                                                pStereo_AnalysisInfo->SubframeSize,
                                                pStereo_AnalysisInfo->DecoderLevel,
                                                PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipeState == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSTEREO_ANALYSIS_INIT_PIPE_FAILED ;
        }

        /* create a buffer for analysing */
        pms->nBufferLength = OCS_UPDATE + PIPE_AHEAD_LENGTH-1;
        for ( ch = 0; ch < pStereo_AnalysisInfo->Channels; ch++)
        {
            pms->ppData[ch] = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->nBufferLength * pms->subframeSize);
            if ( pms->ppData[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested Data vector for channel[%d]\n", ch);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        pms->mData = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->nBufferLength * pms->subframeSize);
        if ( pms->mData == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested Data vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms->mCount = -PIPE_AHEAD_LENGTH + 1;   /* Read the look ahead frames */

        /* check iid bins index range */
        if ((pStereo_AnalysisInfo->iidBinsIndex < 0) ||
            (pStereo_AnalysisInfo->iidBinsIndex > 2))
        {
            SSCLOG_TRACE5(pms->pLogState, "error: invalid iid bins number index: %d\n", pStereo_AnalysisInfo->iidBinsIndex);
            Throw SSCSTEREO_ANALYSIS_INVALID_IID_BINS_IDX;
        }
        pms->nrOfIidBins  = SSC_StereoNumberFreqBins[pStereo_AnalysisInfo->iidBinsIndex];
        pms->nrOfIccBins  = SSC_StereoNumberFreqBins[pStereo_AnalysisInfo->iidBinsIndex];
        pms->nrOfIpdBins  = SSC_StereoNumberIpdOpdBins[pStereo_AnalysisInfo->iidBinsIndex];
        pms->firstTime = True;

        pms->enableIid = False;
        pms->enableIcc = False;
        pms->enableIpd = False;
        pms->enablePca = False;
        pms->enableExt = False;

        InitAnaFilterbank(&(pms->qmfAnalysisFilterBankLeft));
        InitAnaFilterbank(&(pms->qmfAnalysisFilterBankRight));
        InitAnaFilterbank(&(pms->qmfAnalysisFilterBankMid));

        /* Buffer allocation. */
        pms->mHybridRealLeft  = (Double **) calloc (OCS_SLOTS, sizeof (Double *));
        pms->mHybridImagLeft  = (Double **) calloc (OCS_SLOTS, sizeof (Double *));
        pms->mHybridRealRight = (Double **) calloc (OCS_SLOTS, sizeof (Double *));
        pms->mHybridImagRight = (Double **) calloc (OCS_SLOTS, sizeof (Double *));
        pms->mHybridRealMid   = (Double **) calloc (OCS_SLOTS, sizeof (Double *));
        pms->mHybridImagMid   = (Double **) calloc (OCS_SLOTS, sizeof (Double *));

        if (pms->mHybridRealLeft  == NULL || pms->mHybridImagLeft  == NULL ||
            pms->mHybridRealRight == NULL || pms->mHybridImagRight == NULL ||
            pms->mHybridRealMid   == NULL || pms->mHybridImagMid   == NULL) 
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested Data vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        for (i = 0; i < OCS_SLOTS; i++) 
        {

            pms->mHybridRealLeft[i] =
              (Double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (Double));
            pms->mHybridImagLeft[i] =
              (Double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (Double));
            pms->mHybridRealRight[i] =
              (Double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (Double));
            pms->mHybridImagRight[i] =
              (Double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (Double));
            pms->mHybridRealMid[i] =
              (Double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (Double));
            pms->mHybridImagMid[i] =
              (Double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (Double));

            if (pms->mHybridRealLeft[i]  == NULL || pms->mHybridImagLeft[i]  == NULL ||
                pms->mHybridRealRight[i] == NULL || pms->mHybridImagRight[i] == NULL ||
                pms->mHybridRealMid[i]   == NULL || pms->mHybridImagMid[i]   == NULL) 
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested Data vector\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        
        pms->ppAnalysisLeftReal  = (Double **) calloc (OCS_SLOTS+OCS_LOOKAHEAD, sizeof (Double *));
        pms->ppAnalysisLeftImag  = (Double **) calloc (OCS_SLOTS+OCS_LOOKAHEAD, sizeof (Double *));
        pms->ppAnalysisRightReal = (Double **) calloc (OCS_SLOTS+OCS_LOOKAHEAD, sizeof (Double *));
        pms->ppAnalysisRightImag = (Double **) calloc (OCS_SLOTS+OCS_LOOKAHEAD, sizeof (Double *));
        pms->ppAnalysisMidReal   = (Double **) calloc (OCS_SLOTS+OCS_LOOKAHEAD, sizeof (Double *));
        pms->ppAnalysisMidImag   = (Double **) calloc (OCS_SLOTS+OCS_LOOKAHEAD, sizeof (Double *));

        if (pms->ppAnalysisLeftReal  == NULL || pms->ppAnalysisLeftReal  == NULL ||
            pms->ppAnalysisRightReal == NULL || pms->ppAnalysisRightImag == NULL ||
            pms->ppAnalysisMidReal   == NULL || pms->ppAnalysisMidImag   == NULL) 
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested Data vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        for (i = 0; i < OCS_SLOTS+OCS_LOOKAHEAD; i++) 
        {
            pms->ppAnalysisLeftReal[i] =
              (Double *) calloc (NO_QMF_CHANNELS, sizeof (Double));
            pms->ppAnalysisLeftImag[i] =
              (Double *) calloc (NO_QMF_CHANNELS, sizeof (Double));
            pms->ppAnalysisRightReal[i] =
              (Double *) calloc (NO_QMF_CHANNELS, sizeof (Double));
            pms->ppAnalysisRightImag[i] =
              (Double *) calloc (NO_QMF_CHANNELS, sizeof (Double));
            pms->ppAnalysisMidReal[i] =
              (Double *) calloc (NO_QMF_CHANNELS, sizeof (Double));
            pms->ppAnalysisMidImag[i] =
              (Double *) calloc (NO_QMF_CHANNELS, sizeof (Double));

            if (pms->ppAnalysisLeftReal[i]  == NULL || pms->ppAnalysisLeftImag[i]  == NULL ||
                pms->ppAnalysisRightReal[i] == NULL || pms->ppAnalysisRightImag[i] == NULL || 
                pms->ppAnalysisMidReal[i]   == NULL || pms->ppAnalysisMidImag[i]   == NULL) 
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested Data vector\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }

        /* Only 10 bands processing is supported */
        if (pms->nrOfIidBins == 10)
        {
            CreateHybridFilterBank ( &pms->hHybridLeft,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid10,
                                     aHybridResolution10 );
  
            CreateHybridFilterBank ( &pms->hHybridRight,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid10,
                                     aHybridResolution10 );

            CreateHybridFilterBank ( &pms->hHybridMid,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid10,
                                     aHybridResolution10 );
        }
        else if (pms->nrOfIidBins == 20)
        {
            CreateHybridFilterBank ( &pms->hHybridLeft,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid20,
                                     aHybridResolution20 );
  
            CreateHybridFilterBank ( &pms->hHybridRight,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid20,
                                     aHybridResolution20 );

            CreateHybridFilterBank ( &pms->hHybridMid,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid20,
                                     aHybridResolution20 );
        }
        else if (pms->nrOfIidBins == 34)
        {
            CreateHybridFilterBank ( &pms->hHybridLeft,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid34,
                                     aHybridResolution34 );
  
            CreateHybridFilterBank ( &pms->hHybridRight,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid34,
                                     aHybridResolution34 );
  
            CreateHybridFilterBank ( &pms->hHybridMid,
                                     OCS_SLOTS,
                                     noQmfBandsInHybrid34,
                                     aHybridResolution34 );
        }
        else
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested filter band\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

    }
    Catch( Error )
    {
        IMP_STEREO_ANALYSIS_DestroyInstance( pms );    
    }
    return pms ;
}

/* Destroys the stereo_analysis module instance pointed to by *ppState
 * and frees all the memory allocated on
 * IMP_STEREO_ANALYSIS_CreateInstance */

SSC_STEREO_ANALYSISSTATE* IMP_STEREO_ANALYSIS_DestroyInstance( SSC_STEREO_ANALYSISSTATE* const pms )
{

    if( pms != NULL )
    {
        SInt ch = 0;
        SInt i;

        SSCLOG_TRACE4( pms->pLogState, "SSCSTEREO_ANALYSIS_DestroyInstance\n" );

        /* free memory allocated in pms->pStereo_AnalysisState structure */
        if ( pms->pPipeState != NULL )
        {
            SSCPIPE_DestroyInstance( pms->pPipeState );
        }

        if ( pms->pLogState != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pms->pLogState );
            SSCLOG_FREE();
        }
        
        for ( ch = 0; ch < pms->channels; ch++ )
        {
            if ( pms->ppData[ch] != NULL )
            {
                pms->ppData[ch] = SSC_VECT_DestroyInstance( pms->ppData[ch] );
            }
        }
        if ( pms->mData != NULL )
        {
            pms->mData = SSC_VECT_DestroyInstance( pms->mData );
        }

        for (i = 0; i < OCS_SLOTS; i++) 
        {
              if(pms->mHybridRealLeft[i])
                free(pms->mHybridRealLeft[i]);
              if(pms->mHybridImagLeft[i])
                free(pms->mHybridImagLeft[i]);
              if(pms->mHybridRealRight[i])
                free(pms->mHybridRealRight[i]);
              if(pms->mHybridImagRight[i])
                free(pms->mHybridImagRight[i]);
              if(pms->mHybridRealMid[i])
                free(pms->mHybridRealMid[i]);
              if(pms->mHybridImagMid[i])
                free(pms->mHybridImagMid[i]);
        }
        if(pms->mHybridRealLeft)
          free(pms->mHybridRealLeft);
        if(pms->mHybridImagLeft)
          free(pms->mHybridImagLeft);
        if(pms->mHybridRealRight)
          free(pms->mHybridRealRight);
        if(pms->mHybridImagRight)
          free(pms->mHybridImagRight);
        if(pms->mHybridRealMid)
          free(pms->mHybridRealMid);
        if(pms->mHybridImagRight)
          free(pms->mHybridImagMid);

        for (i = 0; i < OCS_SLOTS+OCS_LOOKAHEAD; i++) 
        {
              if(pms->ppAnalysisLeftReal[i])
                free(pms->ppAnalysisLeftReal[i]);
              if(pms->ppAnalysisLeftImag[i])
                free(pms->ppAnalysisLeftImag[i]);
              if(pms->ppAnalysisRightReal[i])
                free(pms->ppAnalysisRightReal[i]);
              if(pms->ppAnalysisRightImag[i])
                free(pms->ppAnalysisRightImag[i]);
              if(pms->ppAnalysisMidReal[i])
                free(pms->ppAnalysisMidReal[i]);
              if(pms->ppAnalysisMidImag[i])
                free(pms->ppAnalysisMidImag[i]);
        }
        if(pms->ppAnalysisLeftReal)
          free(pms->ppAnalysisLeftReal);
        if(pms->ppAnalysisLeftImag)
          free(pms->ppAnalysisLeftImag);
        if(pms->ppAnalysisRightReal)
          free(pms->ppAnalysisRightReal);
        if(pms->ppAnalysisRightImag)
          free(pms->ppAnalysisRightImag);
        if(pms->ppAnalysisMidReal)
          free(pms->ppAnalysisMidReal);
        if(pms->ppAnalysisMidImag)
          free(pms->ppAnalysisMidImag);

        DeleteHybridFilterBank ( &pms->hHybridLeft  );
        DeleteHybridFilterBank ( &pms->hHybridRight );
        DeleteHybridFilterBank ( &pms->hHybridMid   );

        IMP_STEREO_ANALYSIS_Free();

        SSC_FREE( pms, "Stereo_Analysis Instance" );
    }
    else
    {
        fprintf( stdout, "\nError: IMP_STEREO_ANALYSIS_DestroyInstance invalid state pointer\n" );
    }
    return NULL;
}


/* Process current sbunch */
SSCBUNCH* IMP_STEREO_ANALYSIS_Process( SSC_STEREO_ANALYSISSTATE* const pms, SSCBUNCH* const pSscWorkBunch )
{

    SSCPIPE*   pPipe  = pms->pPipeState;

    if ( pms != NULL )
    {

       SSC_ERROR  Error  = SSC_ERROR_OK ;

       /* put the new received sbunch in the pipe */
       SSCPIPE_Add(pPipe, pSscWorkBunch);
        
        /* check if any processing required*/
        if ( (pms->mode == SSC_MODE_LCSTEREO) && (pms->mode_ext == SSC_MODE_EXT_PAR_STEREO) )
        {
            SSC_EXCEPTION   tec;
            SSC_EXCEPTION * the_exception_context = &tec ;  
            SInt            i;
            SInt            idx1;
            SInt            idx2;
            SInt            ch        = 0;
            SInt            Channels  = pms->channels;
            SInt            SfSize    = pms->subframeSize;

            SSCBUNCH*       outSBunch = SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH);
            Double*         pTemp     = NULL;
            Double*         pTemp1    = NULL;
  

            /* shift the signal 
             * into the 'work' buffer */
            for ( ch = 0; ch < Channels; ch++)
            {
                SSC_VECT_Move(pms->ppData[ch], SfSize,
                              pms->ppData[ch], 0,
                              (pms->nBufferLength-1)*SfSize);

                /* update the buffer with the new data */ 
                for ( idx1 = ((pms->nBufferLength-1)*SfSize), idx2 = 0; idx2 < SfSize; idx1++, idx2++)
                {
                    pms->ppData[ch][idx1] = (Double) SSCBUNCH_GetResidual( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH - 1), ch )[idx2];
                }
            }
            

            /* update the buffer with the new data for the mid signal */ 
            SSC_VECT_Move(pms->mData, SfSize,
                          pms->mData, 0,
                          (pms->nBufferLength-1)*SfSize);
            pTemp  = SSCBUNCH_GetResidual( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH - 1), 0 );
            pTemp1 = SSCBUNCH_GetResidual( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH - 1), 1 );
            for ( idx1 = (pms->nBufferLength-1)*SfSize, idx2 = 0; idx2 < SfSize; idx1++, idx2++, pTemp++, pTemp1++)
            {
                pms->mData[idx1] = 1.41421356237310 * (*pTemp + *pTemp1)/2;
            }

            /* Output mid channel */
            pTemp  = SSCBUNCH_GetResidual( outSBunch, 0 );
            pTemp1 = SSCBUNCH_GetResidual( outSBunch, 1 );
            for ( i = 0; i < SfSize ; i++, pTemp++, pTemp1++ )
            {
                *pTemp  = pms->mData[(pms->nBufferLength-PIPE_AHEAD_LENGTH)*SfSize + i];
                *pTemp1 = 0;
            }

            Try
            {
                UInt iidFreqRes = 0;
                UInt iidQuant = 0;
                UInt iccFreqRes = 0;

                /* Update the counter */
                pms->mCount += 1;

                if ( pms->mCount == OCS_UPDATE )    /* Every four subframes start processing */
                {
                    UInt sf;
                    UInt iidFreqRes;
                    UInt iidQuant;
                    UInt iccFreqRes;
                    pms->mCount = 0;

                    /* Dependent settings */
                    if (pms->nrOfIidBins == 10)
                    {
                       pms->enableIid = True;
                       pms->enableIcc = True;
                       pms->enableIpd = False;
                       pms->enableExt = False;
                       
                       /* Enable IPD/OPD */
                       /* pms->enableIpd = True; */
                       /* pms->enableExt = True; */
                       
                       pms->enablePca = True;
                       iidFreqRes     = 0;
                       iidQuant       = OCS_QUANTISATION_COARSE;
                       iccFreqRes     = 0;
                    }
                    else 
                    {
                       pms->enableIid = True;
                       pms->enableIcc = True;
                       pms->enableIpd = True;
                       pms->enableExt = True;
                       pms->enablePca = True;
                       iidFreqRes     = 0;
                       iidQuant       = OCS_QUANTISATION_COARSE;
                       iccFreqRes     = 0;
                    }

                    /* Process all through analysis filter */
                    if (pms->firstTime)
                    {
                        /* Junk delay of analysis filter */
                        for ( sf = 0; sf < DELAY_QMF_ANA; sf++)
                        {
                            CalculateAnaFilterbank( 
                                  &(pms->qmfAnalysisFilterBankLeft), 
                                  &(pms->ppData[0][sf * NO_QMF_CHANNELS]), 
                                  pms->ppAnalysisLeftReal[sf],
                                  pms->ppAnalysisLeftImag[sf]);  
                                   
                            CalculateAnaFilterbank( 
                                  &(pms->qmfAnalysisFilterBankRight), 
                                  &(pms->ppData[1][sf * NO_QMF_CHANNELS]), 
                                  pms->ppAnalysisRightReal[sf],
                                  pms->ppAnalysisRightImag[sf]);       

                            CalculateAnaFilterbank( 
                                  &(pms->qmfAnalysisFilterBankMid), 
                                  &(pms->mData[sf * NO_QMF_CHANNELS]), 
                                  pms->ppAnalysisMidReal[sf],
                                  pms->ppAnalysisMidImag[sf]);       
                        }

                        /* Process look-ahead frames (for hybrid) */
                        for ( sf = 0; sf < OCS_LOOKAHEAD; sf++)
                        {
                            CalculateAnaFilterbank( 
                                  &(pms->qmfAnalysisFilterBankLeft), 
                                  &(pms->ppData[0][(sf + DELAY_QMF_ANA) * NO_QMF_CHANNELS]), 
                                  pms->ppAnalysisLeftReal[sf],
                                  pms->ppAnalysisLeftImag[sf]);  
                                   
                            CalculateAnaFilterbank( 
                                  &(pms->qmfAnalysisFilterBankRight), 
                                  &(pms->ppData[1][(sf + DELAY_QMF_ANA) * NO_QMF_CHANNELS]), 
                                  pms->ppAnalysisRightReal[sf],
                                  pms->ppAnalysisRightImag[sf]);       

                            CalculateAnaFilterbank( 
                                  &(pms->qmfAnalysisFilterBankMid), 
                                  &(pms->mData[(sf + DELAY_QMF_ANA) * NO_QMF_CHANNELS]), 
                                  pms->ppAnalysisMidReal[sf],
                                  pms->ppAnalysisMidImag[sf]);       
                        }

                        pms->firstTime = False;

                    }

                    for ( sf = 0; sf < OCS_SLOTS; sf++)
                    {
                        CalculateAnaFilterbank( 
                              &(pms->qmfAnalysisFilterBankLeft), 
                              &(pms->ppData[0][(sf + DELAY_QMF_ANA + OCS_LOOKAHEAD)* NO_QMF_CHANNELS]), 
                              pms->ppAnalysisLeftReal[sf + OCS_LOOKAHEAD],
                              pms->ppAnalysisLeftImag[sf + OCS_LOOKAHEAD]);  
                                   
                        CalculateAnaFilterbank( 
                              &(pms->qmfAnalysisFilterBankRight), 
                              &(pms->ppData[1][(sf + DELAY_QMF_ANA + OCS_LOOKAHEAD) * NO_QMF_CHANNELS]), 
                              pms->ppAnalysisRightReal[sf + OCS_LOOKAHEAD],
                              pms->ppAnalysisRightImag[sf + OCS_LOOKAHEAD]);       

                        CalculateAnaFilterbank( 
                              &(pms->qmfAnalysisFilterBankMid), 
                              &(pms->mData[(sf + DELAY_QMF_ANA + OCS_LOOKAHEAD) * NO_QMF_CHANNELS]), 
                              pms->ppAnalysisMidReal[sf + OCS_LOOKAHEAD],
                              pms->ppAnalysisMidImag[sf + OCS_LOOKAHEAD]);       
                    }

                    ExtractParams(pms, 
                                  pms->ppAnalysisLeftReal,  pms->ppAnalysisLeftImag,
                                  pms->ppAnalysisRightReal, pms->ppAnalysisRightImag,
                                  pms->ppAnalysisMidReal,   pms->ppAnalysisMidImag);

                    /* Shift buffers */
                    for ( sf = 0; sf < OCS_LOOKAHEAD; sf++)
                    {
                        for ( i = 0; i < NO_QMF_CHANNELS; i++ )
                        {
                            pms->ppAnalysisLeftReal[sf][i]  = pms->ppAnalysisLeftReal[sf + OCS_SLOTS][i];
                            pms->ppAnalysisLeftImag[sf][i]  = pms->ppAnalysisLeftImag[sf + OCS_SLOTS][i];  
                            pms->ppAnalysisRightReal[sf][i] = pms->ppAnalysisRightReal[sf + OCS_SLOTS][i];
                            pms->ppAnalysisRightImag[sf][i] = pms->ppAnalysisRightImag[sf + OCS_SLOTS][i]; 
                            pms->ppAnalysisMidReal[sf][i]   = pms->ppAnalysisMidReal[sf + OCS_SLOTS][i];
                            pms->ppAnalysisMidImag[sf][i]   = pms->ppAnalysisMidImag[sf + OCS_SLOTS][i]; 
                        }      
                    }
                }
                else
                {
                    pms->enableIid = False;
                    pms->enableIcc = False;
                    pms->enableIpd = False;
                    pms->enableExt = False;
                    pms->enablePca = False;
                }

                /* save stereo parameters */

                SSCBUNCH_SetStereoEnvBorder(outSBunch, OCS_SLOTS);

                SSCBUNCH_SetStereoHeaderFlag(outSBunch, False);

                SSCBUNCH_SetStereoIidFlag(outSBunch, pms->enableIid);
                SSCBUNCH_SetStereoIpdFlag(outSBunch, pms->enableIpd);
                SSCBUNCH_SetStereoExtFlag(outSBunch, pms->enableExt);
                SSCBUNCH_SetStereoIccFlag(outSBunch, pms->enableIcc);
                SSCBUNCH_SetStereoPcaFlag(outSBunch, pms->enablePca);

                SSCBUNCH_SetStereoIidFreqRes(outSBunch, iidFreqRes);
                SSCBUNCH_SetStereoIidQuant(outSBunch, iidQuant);
                SSCBUNCH_SetStereoIccFreqRes(outSBunch, iccFreqRes);

                SSCBUNCH_SetStereoIid(outSBunch, pms->iid);
                SSCBUNCH_SetStereoIpd(outSBunch, pms->ipd);
                SSCBUNCH_SetStereoOpd(outSBunch, pms->opd);
                SSCBUNCH_SetStereoIcc(outSBunch, pms->icc);
            }
            Catch(Error)
            {
                Error = SSC_ERROR_OK;
                SSCLOG_TRACE5( pms->pLogState, "SSCSTEREO_ANALYSIS error %d\n", Error );
            }
        }

        /* Use LOCAL_SetError to set the error in the sbunch if an error occurs*/
        LOCAL_SetError( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), Error );
    }
    else
    {
        printf( "\nError: IMP_STEREO_ANALYSIS_Process invalid state pointer\n" );
    }
    return SSCPIPE_Shift(pPipe);
}


/* One time uninitialise function */
void IMP_STEREO_ANALYSIS_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Sets the error code in the current sbunch */
static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == False )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_STEREO_ANALYSIS_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


/* Extracts the stereo parameters */
static void
ExtractParams
( 
    SSC_STEREO_ANALYSISSTATE* const pms,
    Double **iBufferLeft, 
    Double **rBufferLeft, 
    Double **iBufferRight,
    Double **rBufferRight,
    Double **iBufferMid,
    Double **rBufferMid
)
{
    UInt    i;
    UInt    group;
    SInt    bin;
    UInt    subband, maxSubband, mode;
    Double  temp;
    Double  powerLeft     [SSC_ENCODER_MAX_STEREO_BINS];
    Double  powerRight    [SSC_ENCODER_MAX_STEREO_BINS];
    Double  powerCorrReal [SSC_ENCODER_MAX_STEREO_BINS];
    Double  powerCorrImag [SSC_ENCODER_MAX_STEREO_BINS];
    Double  powerCorr2Real[SSC_ENCODER_MAX_STEREO_BINS];
    Double  powerCorr2Imag[SSC_ENCODER_MAX_STEREO_BINS];

    Double **hybrLeftImag;
    Double **hybrLeftReal;
    Double **hybrRightImag;
    Double **hybrRightReal;
    Double **hybrMidImag;
    Double **hybrMidReal;
    
    SInt   nrOfBins;
    const int    *bins2groupMap;

    /* Determine number of bins */
    nrOfBins = pms->nrOfIidBins;

    /* Determine mode for analysis */
    if (nrOfBins == 10)
    {
        mode = MODE_BANDS10; 
        bins2groupMap = bins2groupMap20; 
    }
    else if (nrOfBins == 20)  
    {
        mode = MODE_BANDS20;  
        bins2groupMap = bins2groupMap20; 
    }
    else
    {
        mode = MODE_BANDS34;  
        bins2groupMap = bins2groupMap34; 
    }

    HybridAnalysis ( (const Double**)rBufferLeft,
                   (const Double**)iBufferLeft,
                   pms->mHybridRealLeft,
                   pms->mHybridImagLeft,
                   pms->hHybridLeft,
                   mode);

    HybridAnalysis ( (const Double**)rBufferRight,
                   (const Double**)iBufferRight,
                   pms->mHybridRealRight,
                   pms->mHybridImagRight,
                   pms->hHybridRight,
                   mode);

    HybridAnalysis ( (const Double**)rBufferMid,
                   (const Double**)iBufferMid,
                   pms->mHybridRealMid,
                   pms->mHybridImagMid,
                   pms->hHybridMid,
                   mode);

    /* calculate energy per bin. high resolution is used all the time */
    memset( powerLeft     , 0, sizeof( powerLeft     ) );
    memset( powerRight    , 0, sizeof( powerRight    ) );
    memset( powerCorrReal , 0, sizeof( powerCorrReal ) );
    memset( powerCorrImag , 0, sizeof( powerCorrImag ) );
    memset( powerCorr2Real, 0, sizeof( powerCorr2Real) );
    memset( powerCorr2Imag, 0, sizeof( powerCorr2Imag) );


    hybrLeftReal  = pms->mHybridRealLeft;
    hybrLeftImag  = pms->mHybridImagLeft;
    hybrRightReal = pms->mHybridRealRight;
    hybrRightImag = pms->mHybridImagRight;
    hybrMidReal   = pms->mHybridRealMid;
    hybrMidImag   = pms->mHybridImagMid;

    for ( group = 0; group < NO_IID_GROUPS; group++ )
    {
        /* Translate group to bin */
        bin = ( ~NEGATE_IPD_MASK ) & bins2groupMap[group];
        
        /* Translate from 20 bins to 10 bins */
        if (mode == MODE_BANDS10)
        {  
            bin = bin/2;
        }

        if ( group == SUBQMF_GROUPS )
        {
            hybrLeftReal  = rBufferLeft;
            hybrLeftImag  = iBufferLeft;
            hybrRightReal = rBufferRight;
            hybrRightImag = iBufferRight;
            hybrMidReal   = rBufferMid;
            hybrMidImag   = iBufferMid;
        }

        maxSubband = ( group < SUBQMF_GROUPS )? groupBorders10[group] + 1: groupBorders10[group + 1];

        for ( i = 0; i < OCS_SLOTS; i++ )
        {
            for ( subband = groupBorders10[group]; subband < maxSubband; subband++ )
            {
                temp                 = hybrLeftReal [i][subband] * hybrLeftReal [i][subband] +
                                       hybrLeftImag [i][subband] * hybrLeftImag [i][subband];
                powerLeft    [bin]  += temp;

                temp                 = hybrRightReal[i][subband] * hybrRightReal[i][subband] +
                                       hybrRightImag[i][subband] * hybrRightImag[i][subband];
                powerRight   [bin]  += temp;

                temp                 = hybrLeftReal [i][subband] * hybrRightReal[i][subband] +
                                       hybrLeftImag [i][subband] * hybrRightImag[i][subband];
                powerCorrReal[bin]  += temp;

                temp                 = hybrLeftImag [i][subband] * hybrRightReal[i][subband] -
                                       hybrLeftReal [i][subband] * hybrRightImag[i][subband];
                powerCorrImag[bin]  += temp;

                temp                 = hybrLeftReal [i][subband] * hybrMidReal[i][subband] +
                                       hybrLeftImag [i][subband] * hybrMidImag[i][subband];
                powerCorr2Real[bin] += temp;

                temp                 = hybrLeftImag [i][subband] * hybrMidReal[i][subband] -
                                       hybrLeftReal [i][subband] * hybrMidImag[i][subband];
                powerCorr2Imag[bin] += temp;

            }
        }
    } 

    for ( bin = 0; bin < nrOfBins; bin++ )
    {
        powerLeft     [bin]    += FLOAT_EPSILON;
        powerRight    [bin]    += FLOAT_EPSILON;
        powerCorrReal [bin]    += FLOAT_EPSILON;
        powerCorr2Real[bin]    += FLOAT_EPSILON;
    }


    if ( pms->enableIid )
    {
        Double tempLeft;
        Double tempRight;

        for ( bin = 0; bin < pms->nrOfIidBins; bin++ )
        {
            tempLeft  = powerLeft [bin];
            tempRight = powerRight[bin];

            pms->iid[bin] = 10.0 * log10( tempLeft / tempRight );
        }
    }
    else
    {
        for ( bin = 0; bin < pms->nrOfIidBins; bin++ )
        {
            pms->iid[bin] = 0.0;
        }
    }

    if ( pms->enableIcc )
    {
        Double tempLeft;
        Double tempRight;
        Double tempCorrR;
        Double tempCorrI;

        for ( bin = 0; bin < pms->nrOfIccBins; bin++ )
        {
            tempLeft  = powerLeft    [bin];
            tempRight = powerRight   [bin];
            tempCorrR = powerCorrReal[bin];
            tempCorrI = powerCorrImag[bin];
 
            tempCorrR = sqrt( tempCorrR * tempCorrR + tempCorrI * tempCorrI );

            pms->icc[bin] = tempCorrR / sqrt( tempLeft * tempRight );
        }
    }
    else
    {
        for ( bin = 0; bin < pms->nrOfIccBins; bin++ )
        {
            pms->icc[bin] = 0.0;
        }
    }

    if ( pms->enableIpd )
    {
        Double tempCorrR;
        Double tempCorrI;

        for ( bin = 0; bin < pms->nrOfIpdBins; bin++ )
        {
            tempCorrR = powerCorrReal[bin];
            tempCorrI = powerCorrImag[bin];
 
            tempCorrR = atan2(tempCorrI, tempCorrR);  /* result between -pi and pi */

            pms->ipd[bin] = tempCorrR + SSC_PI;
        }
        for ( bin = 0; bin < pms->nrOfIpdBins; bin++ )
        {
            tempCorrR = powerCorr2Real[bin];
            tempCorrI = powerCorr2Imag[bin];
 
            tempCorrR = atan2(tempCorrI, tempCorrR);  /* result between -pi and pi */

            pms->opd[bin] = tempCorrR + SSC_PI;
        }
    }
    else
    {
        for ( bin = 0; bin < pms->nrOfIpdBins; bin++ )
        {
            pms->ipd[bin] = 0.0;
            pms->opd[bin] = 0.0;
        }
    }

}



/*******************************************************************************
 Functionname:  kChannelFiltering
 *******************************************************************************
 
 Description:   k-channel complex-valued filtering with 6-tap delay.
 
 Arguments:     

 Return:        none

*******************************************************************************/
static void kChannelFiltering( const double *pQmfReal,
			       const double *pQmfImag,
			       double **mHybridReal,
			       double **mHybridImag,
			       int nSamples,
			       int k,
			       int bCplx,
			       const double *p)
{
  int i, n, q;
  double real, imag;
  double modr, modi;

  for(i = 0; i < nSamples; i++) {
    /* FIR filter. */
    for(q = 0; q < k; q++) {
      real = 0;
      imag = 0;
      for(n = 0; n < 13; n++) {
        if (bCplx)
        {
          modr = (double) cos(SSC_PI*(n-6)/k*(1+2*q));
          modi = (double) -sin(SSC_PI*(n-6)/k*(1+2*q));
        }
        else{
          modr = (double) cos(2*SSC_PI*q*(n-6)/k);
          modi = 0;
        }
        real += p[n] * ( pQmfReal[n+i] * modr - pQmfImag[n+i] * modi );
        imag += p[n] * ( pQmfImag[n+i] * modr + pQmfReal[n+i] * modi );
      }
      mHybridReal[i][q] = real;
      mHybridImag[i][q] = imag;
    }
  }
}


/*******************************************************************************
 Functionname:  HybridAnalysis
 *******************************************************************************
 
 Description:   
 
 Arguments:     

 Return:        none

*******************************************************************************/
static void
HybridAnalysis ( const double **mQmfReal,   /*!< The real part of the QMF-matrix.  */
                 const double **mQmfImag,   /*!< The imaginary part of the QMF-matrix. */
                 double **mHybridReal,      /*!< The real part of the hybrid-matrix.  */
                 double **mHybridImag,      /*!< The imaginary part of the hybrid-matrix.  */
                 HANDLE_HYBRID hHybrid,    /*!< Handle to HYBRID struct. */
                 int usedStereoBands)      /*!< indicates which 8 band filter to use */

{
  int    i, j, k, n, band;
  HYBRID_RES hybridRes;
  int    frameSize  = hHybrid->frameSize;
  int    chOffset = 0;
  double shuffleBufReal[NO_QMF_CHANNELS][NO_IID_GROUPS];
  double shuffleBufImag[NO_QMF_CHANNELS][NO_IID_GROUPS];

  for(band = 0; band < hHybrid->nQmfBands; band++) {

    hybridRes = hHybrid->pResolution[band];

    /* Create working buffer. */
    /* Copy stored samples to working buffer. */
    memcpy(hHybrid->pWorkReal, hHybrid->mQmfBufferReal[band],
           hHybrid->qmfBufferMove * sizeof(double));
    memcpy(hHybrid->pWorkImag, hHybrid->mQmfBufferImag[band],
           hHybrid->qmfBufferMove * sizeof(double));

    /* Append new samples to working buffer. */
    for(n = 0; n < frameSize; n++) {
      hHybrid->pWorkReal [hHybrid->qmfBufferMove + n] = mQmfReal [n + HYBRID_FILTER_DELAY] [band];
      hHybrid->pWorkImag [hHybrid->qmfBufferMove + n] = mQmfImag [n + HYBRID_FILTER_DELAY] [band];
    }

    /* Store samples for next frame. */
    memcpy(hHybrid->mQmfBufferReal[band], hHybrid->pWorkReal + frameSize,
           hHybrid->qmfBufferMove * sizeof(double));
    memcpy(hHybrid->mQmfBufferImag[band], hHybrid->pWorkImag + frameSize,
           hHybrid->qmfBufferMove * sizeof(double));


    switch(hybridRes) {
    case HYBRID_2_REAL:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         2,
                         REAL,
                         p2_13_20);
      break;
    case HYBRID_2_REAL | HYBRID_INVERSE_ORDER:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         2,
                         REAL,
                         p2_13_20);

      for ( i = 0; i < frameSize; i++ )
      {
        shuffleBufReal[i][0] = hHybrid->mTempReal[i][0];
        shuffleBufReal[i][1] = hHybrid->mTempReal[i][1];
        shuffleBufImag[i][0] = hHybrid->mTempImag[i][0];
        shuffleBufImag[i][1] = hHybrid->mTempImag[i][1];
        hHybrid->mTempReal[i][0] = shuffleBufReal[i][1];
        hHybrid->mTempReal[i][1] = shuffleBufReal[i][0];
        hHybrid->mTempImag[i][0] = shuffleBufImag[i][1];
        hHybrid->mTempImag[i][1] = shuffleBufImag[i][0];
      }
      break;
    case HYBRID_4_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         2,
                         REAL,
                         p4_13_34);
     break;
    case HYBRID_6_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         8,
                         CPLX,
                         (usedStereoBands==MODE_BANDS34)?p8_13_34:p8_13_20);
      /* do the shuffle */
      for ( i = 0; i < frameSize; i++ )
      {
        hHybrid->mTempReal[i][2] += hHybrid->mTempReal[i][5];
        hHybrid->mTempImag[i][2] += hHybrid->mTempImag[i][5];
        hHybrid->mTempReal[i][3] += hHybrid->mTempReal[i][4];
        hHybrid->mTempImag[i][3] += hHybrid->mTempImag[i][4];
        hHybrid->mTempReal[i][4] = 0.;
        hHybrid->mTempImag[i][4] = 0.;
        hHybrid->mTempReal[i][5] = 0.;
        hHybrid->mTempImag[i][5] = 0.;

        for ( j = 0; j < 8; j++)
        {
            shuffleBufReal[i][j] = hHybrid->mTempReal[i][j]; 
            shuffleBufImag[i][j] = hHybrid->mTempImag[i][j]; 
        }
      }

      for ( i = 0; i < frameSize; i++ )
      {
        hHybrid->mTempReal[i][0] = shuffleBufReal[i][6];
        hHybrid->mTempReal[i][1] = shuffleBufReal[i][7];
        hHybrid->mTempReal[i][2] = shuffleBufReal[i][0];
        hHybrid->mTempReal[i][3] = shuffleBufReal[i][1];
        hHybrid->mTempReal[i][4] = shuffleBufReal[i][2];
        hHybrid->mTempReal[i][5] = shuffleBufReal[i][3];
        hHybrid->mTempImag[i][0] = shuffleBufImag[i][6];
        hHybrid->mTempImag[i][1] = shuffleBufImag[i][7];
        hHybrid->mTempImag[i][2] = shuffleBufImag[i][0];
        hHybrid->mTempImag[i][3] = shuffleBufImag[i][1];
        hHybrid->mTempImag[i][4] = shuffleBufImag[i][2];
        hHybrid->mTempImag[i][5] = shuffleBufImag[i][3];
      }

     break;
    case HYBRID_8_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         8,
                         CPLX,
                         (usedStereoBands==MODE_BANDS34)?p8_13_34:p8_13_20);
      if (usedStereoBands==MODE_BANDS10)
      {
          for ( i = 0; i < frameSize; i++ )
          {
            hHybrid->mTempReal[i][2] += hHybrid->mTempReal[i][5];
            hHybrid->mTempImag[i][2] += hHybrid->mTempImag[i][5];
            hHybrid->mTempReal[i][3] += hHybrid->mTempReal[i][4];
            hHybrid->mTempImag[i][3] += hHybrid->mTempImag[i][4];
            hHybrid->mTempReal[i][4] = 0.;
            hHybrid->mTempImag[i][4] = 0.;
            hHybrid->mTempReal[i][5] = 0.;
            hHybrid->mTempImag[i][5] = 0.;
          }
      }
      break;
    case HYBRID_12_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         2,
                         REAL,
                         p12_13_34);
      break;
    default:
      assert(0);
    }

    hybridRes &= HYBRID_INVERSE_MASK;

    for(n = 0; n < frameSize; n++) {
      for(k = 0; k < (int)hybridRes; k++) {
        mHybridReal [n] [chOffset + k] = hHybrid->mTempReal [n] [k];
        mHybridImag [n] [chOffset + k] = hHybrid->mTempImag [n] [k];
      }
    }
    chOffset += hybridRes;
  }
}


/*******************************************************************************
 Functionname:  HybridSynthesis
 *******************************************************************************
 
 Description:   
 
 Arguments:     

 Return:        none

*******************************************************************************/
static HANDLE_ERROR_INFO
CreateHybridFilterBank ( HANDLE_HYBRID *phHybrid,  /*!< Pointer to handle to HYBRID struct. */
                         int frameSize,            /*!< Framesize (in Qmf sbband samples). */
                         int noBands,              /*!< Number of Qmf bands for hybrid filtering. */
                         const int *pResolution )  /*!< Resolution in Qmf bands (length noBands). */
{
  int i;
  int maxNoChannels = HYBRID_12_CPLX;
  HANDLE_HYBRID hs;

  *phHybrid = NULL;

  hs = (HANDLE_HYBRID) calloc (1, sizeof (HYBRID));

  if (hs == NULL)
    return 1;

  hs->pResolution = (int *) malloc (noBands * sizeof (int));

  for (i = 0; i < noBands; i++) {
    hs->pResolution[i] = pResolution[i];
    if( pResolution[i] != HYBRID_12_CPLX &&
        pResolution[i] != HYBRID_8_CPLX &&
        pResolution[i] != HYBRID_6_CPLX &&
        pResolution[i] != HYBRID_2_REAL &&
        pResolution[i] != (HYBRID_2_REAL | HYBRID_INVERSE_ORDER) &&
        pResolution[i] != HYBRID_4_CPLX )
      return 1;
    if((pResolution[i] & HYBRID_INVERSE_MASK) > maxNoChannels)
      maxNoChannels = pResolution[i] & HYBRID_INVERSE_MASK;
  }

  hs->nQmfBands     = noBands;
  hs->frameSize     = frameSize;
  hs->frameSizeInit = frameSize;
  hs->qmfBufferMove = HYBRID_FILTER_LENGTH - 1;

  hs->pWorkReal = (double *)
                  malloc ((frameSize + hs->qmfBufferMove) * sizeof (double));
  hs->pWorkImag = (double *)
                  malloc ((frameSize + hs->qmfBufferMove) * sizeof (double));
  if (hs->pWorkReal == NULL || hs->pWorkImag == NULL)
    return 1;

  /* Allocate buffers */
  hs->mQmfBufferReal = (double **) malloc (noBands * sizeof (double *));
  hs->mQmfBufferImag = (double **) malloc (noBands * sizeof (double *));
  if (hs->mQmfBufferReal == NULL || hs->mQmfBufferImag == NULL)
    return 1;
  for (i = 0; i < noBands; i++) {
    hs->mQmfBufferReal[i] = (double *) calloc (hs->qmfBufferMove, sizeof (double));
    hs->mQmfBufferImag[i] = (double *) calloc (hs->qmfBufferMove, sizeof (double));
    if (hs->mQmfBufferReal[i] == NULL || hs->mQmfBufferImag[i] == NULL)
      return 1;
  }

  hs->mTempReal = (double **) malloc (frameSize * sizeof (double *));
  hs->mTempImag = (double **) malloc (frameSize * sizeof (double *));
  if (hs->mTempReal == NULL || hs->mTempImag == NULL)
    return 1;
  for (i = 0; i < frameSize; i++) {
    hs->mTempReal[i] = (double *) calloc (maxNoChannels, sizeof (double));
    hs->mTempImag[i] = (double *) calloc (maxNoChannels, sizeof (double));
    if (hs->mTempReal[i] == NULL || hs->mTempImag[i] == NULL)
      return 1;
  }

  *phHybrid = hs;
  return 0;
}

static void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid ) /*!< Pointer to handle to HYBRID struct. */
{
  int i;

  if (*phHybrid) {

    HANDLE_HYBRID hs = *phHybrid;
    int noBands    = hs->nQmfBands;
    int frameSize  = hs->frameSizeInit;

    if(hs->pResolution)
      free(hs->pResolution);

    for (i = 0; i < frameSize; i++) {
      if(hs->mTempReal[i])
        free(hs->mTempReal[i]);
      if(hs->mTempImag[i])
        free(hs->mTempImag[i]);
    }

    if(hs->mTempReal)
     free(hs->mTempReal);
    if(hs->mTempImag)
     free(hs->mTempImag);

    for (i = 0; i < noBands; i++) {
      if(hs->mQmfBufferReal[i])
        free(hs->mQmfBufferReal[i]);
      if(hs->mQmfBufferImag[i])
        free(hs->mQmfBufferImag[i]);
    }

    if(hs->mQmfBufferReal)
      free(hs->mQmfBufferReal);
    if(hs->mQmfBufferImag)
      free(hs->mQmfBufferImag);

    if(hs->pWorkReal)
      free(hs->pWorkReal);
    if(hs->pWorkImag)
      free(hs->pWorkImag);

    free (hs);
    *phHybrid = NULL;
  }
}



