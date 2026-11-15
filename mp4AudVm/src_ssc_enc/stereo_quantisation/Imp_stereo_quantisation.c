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

Source file: Imp_stereo_quantisation.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG: Andy Gerrits,      Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
18-Sep-2002	NT	Initial version
05-Jan-2004	AG	RM5: Low complexity stereo
************************************************************************/

/******************************************************************************
 *
 *   Module              : stereo_quantisation implementation
 *
 *   Description         : Implementation of the stereo_quantisation process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_STEREO_QUANTISATION_
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

#include "Imp_stereo_quantisation.h"
#include "SSC_stereo_quantisation.h"
#include "Imp_Error.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define EXT_LAYER_PRESENT           1

#define NO_IID_COARSE               ( 15 )
#define MIN_IID_COARSE              ( -7 )
#define MAX_IID_COARSE              (  7 )

#define NO_IID_FINE                 (  31 )
#define MIN_IID_FINE                ( -14 )
#define MAX_IID_FINE                (  14 )

#define NO_ICC                      ( 8 )
#define MIN_ICC                     ( 0 )
#define MAX_ICC                     ( 7 )

#define NO_IPDOPD                   ( 8 )
#define MIN_IPDOPD                  ( 0 )
#define MAX_IPDOPD                  ( 7 )

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

static Double QuantTableIID[NO_IID_COARSE] =  /* in dB */
{
    -25.0,     /* -7 */
    -18.0,     /* -6 */
    -14.0,     /* -5 */
    -10.0,     /* -4 */
     -7.0,     /* -3 */
     -4.0,     /* -2 */
     -2.0,     /* -1 */
      0.0,     /*  0 */
      2.0,     /*  1 */
      4.0,     /*  2 */
      7.0,     /*  3 */
     10.0,     /*  4 */
     14.0,     /*  5 */
     18.0,     /*  6 */
     25.0      /*  7 */
};

static Double QuantTableIIDFine[NO_IID_FINE] =  /* in dB */
{
    -50.0,     /* -15 */
    -45.0,     /* -14 */
    -40.0,     /* -13 */
    -35.0,     /* -12 */
    -30.0,     /* -11 */
    -25.0,     /* -10 */
    -22.0,     /* -9 */
    -19.0,     /* -8 */
    -16.0,     /* -7 */
    -13.0,     /* -6 */
    -10.0,     /* -5 */
     -8.0,     /* -4 */
     -6.0,     /* -3 */
     -4.0,     /* -2 */
     -2.0,     /* -1 */
      0.0,     /*  0 */
      2.0,     /*  1 */
      4.0,     /*  2 */
      6.0,     /*  3 */
      8.0,     /*  4 */
     10.0,     /*  5 */
     13.0,     /*  6 */
     16.0,     /*  7 */
     19.0,     /*  8 */
     22.0,     /*  9 */
     25.0,     /*  10 */
     30.0,     /*  11 */
     35.0,     /*  12 */
     40.0,     /*  13 */
     45.0,     /*  14 */
     50.0,     /*  15 */
};

static Double QuantTableICC[NO_ICC] =  
{
    1.00000,     /* 0 */
    0.93700,     /* 1 */
    0.84118,     /* 2 */
    0.60092,     /* 3 */
    0.36764,     /* 4 */
    0.00000,     /* 5 */
   -0.58900,     /* 6 */
   -1.00000      /* 7 */
};

static Double QuantTableIPDOPD[NO_IPDOPD] =  
{
    0.0,                 /* 0 */
    SSC_PI * (1.0/4.0),  /* 1 */
    SSC_PI * (2.0/4.0),  /* 2 */
    SSC_PI * (3.0/4.0),  /* 3 */
    SSC_PI,              /* 4 */
    SSC_PI * (5.0/4.0),  /* 5 */
    SSC_PI * (6.0/4.0),  /* 6 */
    SSC_PI * (7.0/4.0)   /* 7 */
};



/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/
static void LOCAL_SetError(SSCBUNCH *pSscBunch, SSC_ERROR Error);
static void LOCAL_Quantise(
    UInt   nrOfBins, 
    Double *parIdx, 
    Double *par, 
    Double *quantTable,
    UInt   tableLength,
    SInt   min_idx  
);


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* One time inialisation code to allocate/initialise tables */
void IMP_STEREO_QUANTISATION_Initialise(void)
{
}


/* Creates an instance of the stereo_quantisation module */
PSSC_STEREO_QUANTISATIONSTATE IMP_STEREO_QUANTISATION_CreateInstance( PSSC_STEREO_QUANTISATIONINFO pstereo_quantisationInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSSC_STEREO_QUANTISATIONSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);

    Try
    {
        if ( pstereo_quantisationInfo == NULL )
        {
            printf( "\nError: Invalid pointer to stereo_quantisation info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_STEREO_QUANTISATIONSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for stereo_quantisation instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        pms->pLog = NULL ;

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
            strncpy( li.szTitle, "SSC_stereo_quantisation", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSTEREO_QUANTISATION_CreateInstance\n" );

        pms->mode          = pstereo_quantisationInfo->Mode;
        pms->mode_ext      = pstereo_quantisationInfo->ModeExt;
        if ( (pms->mode == SSC_MODE_LCSTEREO) && (pms->mode_ext == SSC_MODE_EXT_PAR_STEREO) )
        {
          /* check iid bins index range */
          if ((pstereo_quantisationInfo->iidBinsIndex < 0) || (pstereo_quantisationInfo->iidBinsIndex > 2))
          {
            SSCLOG_TRACE5(pms->pLog,"error: invalid iid bins number index: %d\n",
                          pstereo_quantisationInfo->iidBinsIndex);
            Throw SSC_STEREOQUANTISATION_INVALID_IID_BINS_IDX;
          }
          pms->iidBinsNumber = SSC_StereoNumberFreqBins[pstereo_quantisationInfo->iidBinsIndex];
        }
    }
    Catch( Error )
    {
        IMP_STEREO_QUANTISATION_DestroyInstance( &pms );
    }
    return pms ;
}

  
/* Process current sbunch */
SSCBUNCH* IMP_STEREO_QUANTISATION_Process(PSSC_STEREO_QUANTISATIONSTATE pms, SSCBUNCH* pSscBunch)
{
    SSC_EXCEPTION   tec;
    SSC_EXCEPTION * the_exception_context = &tec;  
    SSC_ERROR       error = SSC_ERROR_OK;
  
    Try
    {
        Bool enableIid;
        Bool enableIcc;
        Bool enableIpd;
        UInt IidFreqRes;
        UInt IidQuant;  
        UInt IccFreqRes;
        UInt NrOfBins;
        Double *icc;
        Double *iid;
        Double *ipd;
        Double *opd;
        Double iid_idx[OCS_MAX_FREQ_RESOLUTION];
        Double icc_idx[OCS_MAX_FREQ_RESOLUTION];
        Double ipd_idx[OCS_MAX_FREQ_RESOLUTION];
        Double opd_idx[OCS_MAX_FREQ_RESOLUTION];

        /* check state pointer */
        if (pms == NULL)
        {
            Throw SSC_STEREOQUANTISATION_INVALID_STATE_PTR;
        }

        /* check if any processing required */
        if ( (pms->mode != SSC_MODE_LCSTEREO) && (pms->mode_ext != SSC_MODE_EXT_PAR_STEREO) )
        {
            Throw SSC_STEREOQUANTISATION_NO_PROCESSING;
        }
    
        SSCLOG_TRACE4(pms->pLog, "IMP_STEREOQUANTISATION_Process\n");

        enableIid = SSCBUNCH_GetStereoIidFlag(pSscBunch);
        enableIcc = SSCBUNCH_GetStereoIccFlag(pSscBunch);
        enableIpd = SSCBUNCH_GetStereoIpdFlag(pSscBunch);

        IidFreqRes = SSCBUNCH_GetStereoIidFreqRes(pSscBunch);
        IidQuant   = SSCBUNCH_GetStereoIidQuant(pSscBunch);
        IccFreqRes = SSCBUNCH_GetStereoIccFreqRes(pSscBunch);

        iid = SSCBUNCH_GetStereoIid(pSscBunch);
        icc = SSCBUNCH_GetStereoIcc(pSscBunch);
        ipd = SSCBUNCH_GetStereoIpd(pSscBunch);
        opd = SSCBUNCH_GetStereoOpd(pSscBunch);

        if (enableIid)
        {
            NrOfBins = SSC_StereoNumberFreqBins[IidFreqRes];
            if (IidQuant == OCS_QUANTISATION_COARSE)
            {
                LOCAL_Quantise(NrOfBins, iid_idx, iid, QuantTableIID,  
                                  NO_IID_COARSE, MIN_IID_COARSE);
            }
            else
            {
                LOCAL_Quantise(NrOfBins, iid_idx, iid, QuantTableIIDFine,  
                                  NO_IID_FINE, MIN_IID_FINE);
            }
            SSCBUNCH_SetQStereoIid(pSscBunch, iid_idx);
        }
        else
        {
            SSCBUNCH_SetQStereoIid(pSscBunch, SSCBUNCH_GetStereoIid(pSscBunch));
        }

        if (enableIcc)
        {
            NrOfBins = SSC_StereoNumberFreqBins[IccFreqRes];
            LOCAL_Quantise(NrOfBins, icc_idx, icc, QuantTableICC,  
                              NO_ICC, MIN_ICC);

            SSCBUNCH_SetQStereoIcc(pSscBunch, icc_idx);
        }
        else
        {
            SSCBUNCH_SetQStereoIcc(pSscBunch, SSCBUNCH_GetStereoIcc(pSscBunch));
        }

        if (enableIpd)
        {
            NrOfBins = SSC_StereoNumberIpdOpdBins[IidFreqRes];
            LOCAL_Quantise(NrOfBins, ipd_idx, ipd, QuantTableIPDOPD,  
                              NO_IPDOPD, MIN_IPDOPD);
            LOCAL_Quantise(NrOfBins, opd_idx, opd, QuantTableIPDOPD,  
                              NO_IPDOPD, MIN_IPDOPD);

            SSCBUNCH_SetQStereoIpd(pSscBunch, ipd_idx);
            SSCBUNCH_SetQStereoOpd(pSscBunch, opd_idx);
        }
        else
        {
            SSCBUNCH_SetQStereoIpd(pSscBunch, SSCBUNCH_GetStereoIpd(pSscBunch));
            SSCBUNCH_SetQStereoOpd(pSscBunch, SSCBUNCH_GetStereoOpd(pSscBunch));
        }

        
        /* copy indices to quantised sbunch */
        SSCBUNCH_SetQStereoEnvBorder(pSscBunch, SSCBUNCH_GetStereoEnvBorder(pSscBunch));

        SSCBUNCH_SetQStereoHeaderFlag(pSscBunch, SSCBUNCH_GetStereoHeaderFlag(pSscBunch));
        
        SSCBUNCH_SetQStereoIidFlag(pSscBunch, enableIid);
        SSCBUNCH_SetQStereoIccFlag(pSscBunch, enableIcc);
        SSCBUNCH_SetQStereoIpdFlag(pSscBunch, enableIpd);
        SSCBUNCH_SetQStereoExtFlag(pSscBunch, SSCBUNCH_GetStereoExtFlag(pSscBunch));
        SSCBUNCH_SetQStereoPcaFlag(pSscBunch, SSCBUNCH_GetStereoPcaFlag(pSscBunch));

        SSCBUNCH_SetQStereoIidFreqRes(pSscBunch, IidFreqRes);
        SSCBUNCH_SetQStereoIidQuant(pSscBunch, IidQuant);
        SSCBUNCH_SetQStereoIccFreqRes(pSscBunch, IccFreqRes);
    }
    Catch(error)
    {
    }

    if (error != SSC_STEREOQUANTISATION_NO_PROCESSING)
    {
        if (error != SSC_STEREOQUANTISATION_INVALID_STATE_PTR)
        {
          /* at the moment all errors are no errors to stop encoding */
          error = SSC_ERROR_OK;
      
          SSCLOG_TRACE4(pms->pLog, "finshed\n");
        }
        else
        {
            printf("IMP_STEREOQUANTISATION_Process error: invalid state pointer\n");
        }
    }
    else
    {
        error = SSC_ERROR_OK;
    }
  
    if (error != SSC_ERROR_OK)
    {
        LOCAL_SetError(pSscBunch, error);
    }
    return pSscBunch;
}

/* Destroys the stereo_quantisation module instance pointed to by *ppState and
 * frees all the memory allocated on IMP_STEREO_QUANTISATION_CreateInstance */

void IMP_STEREO_QUANTISATION_DestroyInstance( PSSC_STEREO_QUANTISATIONSTATE* ppms )
{
    PSSC_STEREO_QUANTISATIONSTATE pms = *ppms;

    if( pms != NULL )
    {
        SSCLOG_TRACE4( pms->pLog, "SSC_STEREO_QUANTISATION_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( pms->pPipe );
        if( pms->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( pms->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( pms, "" );
        *ppms = NULL ;
    }
    else
    {
        printf( "\nError: IMP_STEREO_QUANTISATION_DestroyInstance invalid state pointer\n" );
    }
}


void IMP_STEREO_QUANTISATION_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == 0 )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_STEREO_QUANTISATION_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}

static void LOCAL_Quantise(
    UInt   nrOfBins, 
    Double *parIdx, 
    Double *par, 
    Double *quantTable,
    UInt   tableLength,
    SInt   min_idx  
)
{
    UInt i;

    for (i = 0; i < nrOfBins; i++)
    {
        Double diff;
        Double minDiff;
        UInt   minIdx = 0;
        UInt   j;

        if (tableLength > 0)
        {
            minDiff = fabs(quantTable[0] - par[i]);
            minIdx  = 0;
            for (j = 1; j < tableLength; j++)
            {
                diff = fabs(quantTable[j] - par[i]);
                if (diff < minDiff)
                {
                    minDiff = diff;
                    minIdx  = j;
                }
            }
        }
        parIdx[i] = (Double)minIdx + (Double)min_idx;

    }
}

