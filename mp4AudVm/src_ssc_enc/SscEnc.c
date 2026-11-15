/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Philips Digital Systems Laboratory

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.

Source file: SscEnc.c


Required modules:


Authors:
RJ    Robert Janssen, Philips <robert.janssen@philips.com>

Changes:
02-may-2003   RJ    initial version
**********************************************************************/


/* =====================================================================*/
/* Standard Includes                                                    */
/* =====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================================*/
/* MPEG-4 VM Includes                                                   */
/* =====================================================================*/
#include "common_m4a.h"     /* common module                            */
#include "cmdline.h"        /* command line module                      */
#include "allHandles.h"
#include "obj_descr.h"
#include "bitstream.h"
#include "audio.h"
                        
/* =====================================================================*/
/* PHILIPS Includes                                                     */
/* =====================================================================*/

#include "SscEnc.h"

/* =====================================================================*/
/* L O C A L     S Y M B O L     D E C L A R A T I O N S                */
/* =====================================================================*/
#define PROGVER "SSC encoder Jan 2004"
#define SEPACHAR " ,="

/* =====================================================================*/
/* L O C A L     D A T A      D E C L A R A T I O N S                   */
/* =====================================================================*/
/* configuration inputs                                                 */
/* =====================================================================*/
static long NumberOfSamples;
static long PhaseJitterBand;
static long PhaseJitterPercentage;
static long FrequencyQuantisation;
static long AmplitudeQuantisation;
static long RelevanceTreshold;
static long RefreshStrategy;
static long PhaseMode;
static long StereoMode;
static long IID_Quantisation;
static long ITD_Quantisation;
static long ICC_Quantisation;

static CmdLineSwitch switchList[] = {
    {"h" ,NULL,NULL,NULL,NULL,"print help"},
    {"n" ,&NumberOfSamples,"%d",NULL,NULL,"number of samples (n=0 processes all samples)"},
    {"jb",&PhaseJitterBand,"%d",NULL,NULL,"phase jitter band (allowed values 0, 1, 2, 3)"},
    {"jp",&PhaseJitterPercentage,"%d",NULL,NULL,"phase jitter percentage (allowed values: 0 (no jitter), 1 (25% jitter), 2 (50% jitter), 3 (75% jitter), 4 (100% jitter) )"},
    {"qf",&FrequencyQuantisation,"%d","3",NULL,"frequency quantisation (range from 0 (fine) to 3 (coarse) )"},
    {"qa",&AmplitudeQuantisation,"%d","3",NULL,"amplitude quantisation (range from 0 (fine) to 3 (coarse) )"},
    {"rt",&RelevanceTreshold,"%d",NULL,NULL,"relevance treshold"},
    {"rs",&RefreshStrategy,"%d",NULL,NULL,"refresh strategy (0 = only first frame has refresh, 1 = each frame has refresh"},
    {"sm",&StereoMode,"%d","1",NULL,"stereo mode (0 = dual mono, 1 = joint stereo"},
    {"qi",&IID_Quantisation,"%d",NULL,NULL,"iid quantisation (0 = fine, 1 = coarse)"},
    {"qt",&ITD_Quantisation,"%d",NULL,NULL,"itd quantisation (0 = fine, 1 = coarse)"},
    {"qc",&ICC_Quantisation,"%d",NULL,NULL,"icc quantisation (0 = fine, 1 = coarse)"},
    {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* =====================================================================*/
/* F U N C T I O N      D E F I N I T I O N S                           */
/* =====================================================================*/
/* ---------------------------------------------------------------------*/
/* EncSSCInfo()                                                         */
/* ---------------------------------------------------------------------*/

char *EncSSCInfo (FILE *helpStream)    /* in: print encPara help text to helpStream    */
                                       /*     if helpStream not NULL                   */
                                       /* returns: core version string                 */
{
    if (helpStream != NULL)
    {
    fprintf(helpStream,
        PROGVER "\n"
        "encoder parameter string format:\n"
        "  list of tokens (tokens separated by characters in \"%s\")\n",
        SEPACHAR);
    CmdLineHelp(NULL,NULL,switchList,helpStream);
    }  
    
    return PROGVER;
}

    
void SSC_GetEmptyEncoderInfo(ENCODERINFO *pEI, SInt nDecoder_level)
{
    memset(pEI, 0, sizeof(ENCODERINFO));

    /* fill in some defaults */
    pEI->decoder_level = nDecoder_level ;
    pEI->grid_freq = 3 ;
    pEI->grid_amp = 3 ;
    pEI->grid_iid = 0 ;
    pEI->grid_itd = 0 ;
    pEI->grid_rho = 0 ;
    pEI->phase_jitter_band = 0;
    pEI->phase_jitter_percentage = 0;
    pEI->channel_depend = 1;
    pEI->stereo_upd_rate = 0;
    pEI->stereo_nr_of_bins_idx = 0;
    pEI->stereo_nr_of_itd_bins_idx = 3;
    pEI->max_nr_of_sins = 60;
    pEI->max_nr_of_t_sins = 6;
    pEI->max_den_order = 24;
    pEI->pole = 0.7;
    pEI->enhancement = 1;
    pEI->max_env_order = 15;
    pEI->in_dir_name = SSC_CALLOC(512, sizeof(Char), "") ;
    pEI->in_file_name = SSC_CALLOC(512, sizeof(Char), "") ;
    pEI->out_dir_name = SSC_CALLOC(512, sizeof(Char), "") ;
    pEI->out_file_name = SSC_CALLOC(512, sizeof(Char), "") ;
}



void SSC_ParseCommandLine(char * AudioFileName,
                          char * BitFileName,
                       ENCODERINFO *pEI)
{

    strcpy(pEI->in_file_name, AudioFileName);
    strcpy(pEI->out_file_name, BitFileName);

    pEI->number_samples = floor(NumberOfSamples + 0.5);;
    
    pEI->phase_jitter_band = PhaseJitterBand;

    pEI->phase_jitter_percentage = PhaseJitterPercentage;

    pEI->grid_freq = FrequencyQuantisation;

    pEI->grid_amp = AmplitudeQuantisation;

    pEI->rel_threshold = RelevanceTreshold;

    if (RefreshStrategy != 0)
    {
        pEI->sin_refresh    = 1;
        pEI->noise_refresh  = 1;
        pEI->stereo_refresh = 1;
    }
    else
    {
        pEI->sin_refresh    = 0;
        pEI->noise_refresh  = 0;
        pEI->stereo_refresh = 0;
    }

    pEI->enhancement = 1;
 
    if (StereoMode != 0)
    {
        pEI->channel_depend = 1;
    }
    else
    {
        pEI->channel_depend = 0;
    }

    if (IID_Quantisation != 0)
    {
        pEI->grid_iid = 1;
    }
    else
    {
        pEI->grid_iid = 0;
    }

    if (ITD_Quantisation != 0)
    {
        pEI->grid_itd = 1;
    }
    else
    {
        pEI->grid_itd = 0;
    }

    if (ICC_Quantisation != 0)
    {
        pEI->grid_rho = 1;
    }
    else
    {
        pEI->grid_rho = 0;
    }
}

static Bool SSC_Encoder_Init(PROCESSINFO *pPI, ENCODERINFO *pEI, unsigned char ** ppHeaderData, int * pHeaderBytes, int numChannel, float fSample, long fileNumSample)
{
    SSC_ERROR       Error                 = SSC_ERROR_OK ;
    SSC_EXCEPTION   tec                   = SSC_INIT_EXCEPTION ;
    SSC_EXCEPTION  *the_exception_context = &tec ;

    Try
    {
        memset(pPI, 0, sizeof(PROCESSINFO));

        pPI->pas = SSC_AUDIOREAD_InitInstance    (pEI, numChannel, fSample, fileNumSample);
        if (pPI->pas == NULL)
        {   /* check if input file open succeeded */
            Throw(SSC_ERROR_OUT_OF_MEMORY);
        }
        pPI->pfs   = SSC_FILTER_InitInstance                (pEI);
        pPI->ptds  = SSC_TRANSIENTDETECTION_InitInstance    (pEI);
        pPI->pstas = SSC_STEREO_ANALYSIS_InitInstance       (pEI);
        pPI->ptas  = SSC_TRANSIENT_ANALYSIS_InitInstance    (pEI);
        pPI->ptss  = SSC_TRANSIENT_SYNTHESIS_InitInstance   (pEI);
        pPI->ptqs  = SSC_TRANSIENT_QUANTISATION_InitInstance(pEI);
        pPI->psas  = SSC_SINUSOID_ANALYSIS_InitInstance     (pEI);
        pPI->psss  = SSC_SINUSOID_SYNTHESIS_InitInstance    (pEI);
        pPI->psps  = SSC_SINUSOID_PSYCHO_InitInstance       (pEI);
        pPI->psrs  = SSC_SINUSOID_REMOVE_InitInstance       (pEI);
        pPI->psts  = SSC_SINUSOID_TRACKING_InitInstance     (pEI);
        pPI->pstqs = SSC_STEREO_QUANTISATION_InitInstance   (pEI);
        pPI->psqs  = SSC_SINUSOID_QUANTISATION_InitInstance (pEI);
        pPI->pnas  = SSC_NOISE_ANALYSIS_InitInstance        (pEI);
        pPI->pntes = SSC_NOISE_TEMP_ENV_InitInstance        (pEI);
        pPI->pnqs  = SSC_NOISE_QUANTISATION_InitInstance    (pEI);
        pPI->pbsfs = SSC_BITSTREAM_InitInstance             (pEI, ppHeaderData, pHeaderBytes);
        /* check if all other process are initialized correctly */
        if ((pPI->pfs   == NULL) || (pPI->ptds  == NULL) || (pPI->pstas == NULL) || 
            (pPI->ptas  == NULL) || (pPI->ptss  == NULL) || (pPI->ptqs  == NULL) || 
            (pPI->psas  == NULL) || (pPI->psss  == NULL) || (pPI->psps  == NULL) || 
            (pPI->psrs  == NULL) || (pPI->psts  == NULL) || (pPI->pstqs == NULL) || 
            (pPI->psqs  == NULL) || (pPI->pnas  == NULL) || 
            (pPI->pntes == NULL) || (pPI->pnqs  == NULL) || (pPI->pbsfs == NULL))
        {
           Throw(SSC_ERROR_OUT_OF_MEMORY);
        }
    }
    Catch(Error)
    {
        Error = 1 ;
    }
    return (Error == SSC_ERROR_OK);
}



static Bool SSC_CheckParameters(ENCODERINFO *pEI)
{
    FILE *fp ;
    char szFileName[1024] ;
    char *cp ;

    if (pEI->out_file_name[0] == '\0')
    {
        /* if no output file name is specified use the input filename with a .ssc ext */
        pEI->out_dir_name[0] = '.';
        pEI->out_dir_name[1] = PATH_SEP ;
        pEI->out_dir_name[2] = '\0';

        strcpy(pEI->out_file_name, pEI->in_file_name);
        cp = strrchr(pEI->out_file_name, (SInt)'.');
        if (cp == NULL)
        {
            /* no extension, add .ssc */
            strcat(pEI->out_file_name, ".ssc");
        }
        else
        {
            /* extension found, overwrite with .ssc */
            strcpy(cp, ".ssc") ;
        }
    }

    if (pEI->grid_freq < 0)
    {
        pEI->grid_freq = 0 ;
        printf("Warning: qf parameter out of range, clipped to %d\n", pEI->grid_freq);
    }

    if (pEI->grid_freq > 3)
    {
        pEI->grid_freq = 3 ;
        printf("Warning: qf parameter out of range, clipped to %d\n", pEI->grid_freq);
    }

    if (pEI->grid_amp < 0)
    {
        pEI->grid_amp = 0 ;
        printf("Warning: qa parameter out of range, clipped to %d\n", pEI->grid_amp);
    }

    if (pEI->grid_amp > 3)
    {
        pEI->grid_amp = 3 ;
        printf("Warning: qa parameter out of range, clipped to %d\n", pEI->grid_amp);
    }

    if (pEI->phase_jitter_percentage < 0)
    {
        pEI->phase_jitter_percentage = 0 ;
        printf("Warning: phase jitter percentage parameter out of range, clipped to %d\n", pEI->phase_jitter_percentage);
    }

    if (pEI->phase_jitter_percentage > 4)
    {
        pEI->phase_jitter_percentage = 4 ;
        printf("Warning: phase jitter percentage parameter out of range, clipped to %d\n", pEI->phase_jitter_percentage);
    }

    if (pEI->phase_jitter_band < 0)
    {
        pEI->phase_jitter_band = 0 ;
        printf("Warning: phase jitter band parameter out of range, clipped to %d\n", pEI->phase_jitter_band);
    }

    if (pEI->phase_jitter_band > 3)
    {
        pEI->phase_jitter_band = 3 ;
        printf("Warning: phase jitter band parameter out of range, clipped to %d\n", pEI->phase_jitter_band);
    }

    if (pEI->rel_threshold < 0)
    {
        pEI->rel_threshold = 0 ;
        printf("Warning: rt parameter out of range, clipped to %f\n", pEI->rel_threshold);
    }

    if (pEI->number_samples < 0)
    {
        pEI->number_samples = 0 ;
    }

    strcpy(szFileName, pEI->in_dir_name);
    strcat(szFileName, pEI->in_file_name);
    fp = fopen(szFileName, "r");
    if (fp == NULL)
    {
        printf("Error: unable to open input file '%s'\n", szFileName);
        return False ;
    }
    else
    {
        fclose(fp);
    }
    strcpy(szFileName, pEI->out_dir_name);
    strcat(szFileName, pEI->out_file_name);
    fp = fopen(szFileName, "w");
    if (fp == NULL)
    {
        printf("Error: unable to open output file '%s'\n", szFileName);
        return False ;
    }
    else
    {
        fclose(fp);
    }

    return True ;
}

static void SSC_print_settings(ENCODERINFO *pEI)
{
    char szFileName[1024] ;

    fprintf(stdout, "Settings:\n");
    strcpy(szFileName, pEI->in_dir_name);
    strcat(szFileName, pEI->in_file_name);
    fprintf(stdout, "    inputfile              : %s\n", szFileName);
    strcpy(szFileName, pEI->out_dir_name);
    strcat(szFileName, pEI->out_file_name);
    fprintf(stdout, "    outputfile             : %s\n", szFileName);
    fprintf(stdout, "    number of samples      : %.0f\n", pEI->number_samples);
    fprintf(stdout, "    frequency quantisation : %d\n", pEI->grid_freq);
    fprintf(stdout, "    amplitude quantisation : %d\n", pEI->grid_amp);
    fprintf(stdout, "    phase jitter band      : %d\n", pEI->phase_jitter_band);
    fprintf(stdout, "    phase jitter percentage: %d\n", pEI->phase_jitter_percentage);
    fprintf(stdout, "    relevance threshold    : %0.02f\n", pEI->rel_threshold);
    fprintf(stdout, "    refresh strategy       : %d\n", pEI->sin_refresh);
    fprintf(stdout, "    stereo mode            : %d\n", pEI->channel_depend);
    fprintf(stdout, "\n");
}


/*****************************************************************************************
 ***
 *** Function: EncSSCInit
 ***
 *** Purpose:  Initialize SSC encoder
 ***
 *** Description:
 *** 
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/

Bool EncSSCInit(int numChannel,
                float fSample,
                long fileNumSample,
                char *        AudioFileName,        /* in: audio file name */
                char *        BitFileName,        /* in: output file name */
                char        *encPara,            /* in: encoder parameter string */
                PROCESSINFO**    ppPI,
                ENCODERINFO**    ppEI,
                SSCBUNCH**         ppSSCBunch,
                int *        pFrameNumSample,
                int *       pDelayNumSample,
                HANDLE_BSBITBUFFER bitHeader
                )
{
    int parac;
    char **parav;
    int result;
    SInt            nSfInterval           = 1 ;
    Bool            bSuccess ;
    unsigned char *    pHeaderData;
    int                HeaderBytes;
    int                i;
    
    HANDLE_BSBITSTREAM header_stream;

    header_stream = BsOpenBufferWrite(bitHeader);

    /* Allocate memory for Process info and encoder info */
    *ppPI = (PROCESSINFO *) SSC_MALLOC(sizeof(PROCESSINFO), "");
    *ppEI = (ENCODERINFO *) SSC_MALLOC(sizeof(ENCODERINFO), "");

    SSC_GetEmptyEncoderInfo(*ppEI, SSC_DEC_LEVEL_MEDIUM);

    /* Parsing should involve only parsing of the encPara string */

    parav = CmdLineParseString(encPara,SEPACHAR,&parac);
    result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);

    SSC_ParseCommandLine(
        AudioFileName,
        BitFileName,
        *ppEI);


    bSuccess = SSC_CheckParameters(*ppEI);
    if (bSuccess)
    {
        SSC_print_settings(*ppEI);
        bSuccess = SSC_Encoder_Init(*ppPI, *ppEI, &pHeaderData, &HeaderBytes, numChannel, fSample, fileNumSample);

        if (bSuccess)
        {
            /* Put HeaderBytes in HeaderBuffer */

            for (i = 0; i < HeaderBytes; i++)
            {
                BsPutBit( header_stream, (unsigned long)(pHeaderData[i]), 8 );
            }

            nSfInterval = (SInt)(((Double)((*ppEI)->fs) / ((*ppEI)->sf_size)) + 0.5) ;
            *ppSSCBunch   = SSCBUNCH_InitInstance((*ppEI)->nr_of_channels, (*ppEI)->sf_size, (*ppEI)->decoder_level, NULL);

            (*ppPI)->t_start = clock();

            *pFrameNumSample  = (*ppEI)->sf_size;
            *pDelayNumSample  = 0;

            if (*ppSSCBunch == NULL)
            {
                fprintf(stdout, "\nError %d occurred, exiting.\n", SSC_ERROR_OUT_OF_MEMORY);
            }
        }
    }
    return (bSuccess);
}


Bool EncSSCFrame (
         float **p_time_signal,   /* array of pointers to the modulo time 
                                   signal input buffers for each channel */
         HANDLE_BSBITBUFFER fixed_bitbuffer,             /* bits before bitres size ptr */
         ENCODERINFO*   pEI,
         PROCESSINFO*   pPI,
         SSCBUNCH**        ppSSCBunch)
{
    PSBUNCH         pBunch ;
    SSC_ERROR       Error                 = SSC_ERROR_OK ;
    static SInt     nSfCount              = 0;
    static SInt     nBits                 = 0 ;
    SInt            nSfInterval           = 1 ;

    unsigned char * pFrameData = NULL;
    int                FrameBytes = 0;
    int                i;

    Bool            GoOn;


    HANDLE_BSBITSTREAM fixed_stream;

    nSfInterval = (SInt)(((Double)pEI->fs / pEI->sf_size) + 0.5) ;
    
    pBunch = (*ppSSCBunch)->pBunch ;

       GoOn = (pBunch && !((SSCBUNCH_GetSfCounter(*ppSSCBunch) > 0) && (SSCBUNCH_GetDataFromStream(*ppSSCBunch)== 0)));

    if (GoOn)
    {
        SSC_ClearSscBunch(pEI, *ppSSCBunch);

        *ppSSCBunch = SSC_AUDIOREAD_Process_NoAudioRead(pPI->pas, *ppSSCBunch, p_time_signal);

        if (SSCBUNCH_GetDataFromStream(*ppSSCBunch)!=0)
        {
            nSfCount++ ;
            fprintf(stdout, "Processing sf %d of %d \r", SSCBUNCH_GetSfCounter(*ppSSCBunch), pEI->nr_of_subframes);
            if ((nSfCount % nSfInterval) == 0)
            {
                SSC_PrintTiming(pPI, pEI, nSfCount);
            }
        }
        else
        {
            fprintf(stdout, "Flushing sf %d                                                     \r",
                             SSCBUNCH_GetSfCounter(*ppSSCBunch));
        }

        *ppSSCBunch = SSC_FILTER_Process                 (pPI->pfs,   *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_TRANSIENTDETECTION_Process     (pPI->ptds,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_STEREO_ANALYSIS_Process        (pPI->pstas, *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_TRANSIENT_ANALYSIS_Process     (pPI->ptas,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_TRANSIENT_SYNTHESIS_Process    (pPI->ptss,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_TRANSIENT_QUANTISATION_Process (pPI->ptqs,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_SINUSOID_ANALYSIS_Process      (pPI->psas,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_SINUSOID_SYNTHESIS_Process     (pPI->psss,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_SINUSOID_PSYCHO_Process        (pPI->psps,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_SINUSOID_REMOVE_Process        (pPI->psrs,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_SINUSOID_TRACKING_Process      (pPI->psts,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_STEREO_QUANTISATION_Process    (pPI->pstqs, *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_SINUSOID_QUANTISATION_Process  (pPI->psqs,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_NOISE_ANALYSIS_Process         (pPI->pnas,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_NOISE_TEMP_ENV_Process         (pPI->pntes, *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_NOISE_QUANTISATION_Process     (pPI->pnqs,  *ppSSCBunch, &Error);
        *ppSSCBunch = SSC_BITSTREAM_Process              (pPI->pbsfs, *ppSSCBunch, &Error, &pFrameData, &FrameBytes);

        BsClearBuffer (fixed_bitbuffer);
        
        if (FrameBytes > 0)
        {
            fixed_stream = BsOpenBufferWrite(fixed_bitbuffer);

            for (i = 0; i < FrameBytes; i++)
            {
                BsPutBit( fixed_stream, (unsigned long)(pFrameData[i]), 8 );
            }

            BsClose(fixed_stream);
        }

        nBits += SSC_AddBits(*ppSSCBunch);
    }
    else
    {
        fprintf(stdout, "\n\nResult: ");
        SSC_PrintTiming(pPI, pEI, nSfCount);
        SSC_PrintBitrate(pEI, nSfCount, nBits);
        fprintf(stdout, "Ready.\n");
    }

    return (GoOn);
}


void SSC_PrintTiming(PROCESSINFO *pPI, ENCODERINFO *pEI, SInt nSfCount)
{
    Double duration = ((Double)clock() - pPI->t_start) / CLOCKS_PER_SEC;
    Double filetime = ((Double)(nSfCount*pEI->sf_size)/pEI->fs);
    Double rtValue  = duration / filetime ;

    fprintf(stdout, "%0.02f seconds encoded, %0.02f times real time\n", filetime, rtValue);
}


void SSC_PrintBitrate(ENCODERINFO *pEI, SInt nSfCount, SInt nBits)
{
    SInt nSamples = pEI->sf_size * nSfCount ;

    if ((pEI->fs > 0) && (nSamples > 0))
    {
        Double duration = (Double)nSamples / pEI->fs ;

        fprintf(stdout, "Bitrate %2.2f kBps.\n", (Double)nBits/duration / 1000.0);
    }
}

SInt SSC_AddBits(SSCBUNCH *pSscBunch)
{
    SInt nBits = 0 ;

    if (SSCBUNCH_GetDataFromStream(pSscBunch) != 0)
    {
        SInt   i ;
        Double *pBits = SSCBUNCH_GetActualBitrate(pSscBunch);

        for(i=0; i<6; i++)
        {
            nBits += (SInt)pBits[i] ;
        }
    }
    return nBits ;
}

void SSC_ClearSscBunch(ENCODERINFO *pEI, SSCBUNCH *pSSCBunch)
{
    SInt ch;

    SSCBUNCH_Clear(pSSCBunch);
    for (ch = 0; ch < pEI->nr_of_channels; ch++)
    {
        SSCBUNCH_SetSinusoidFreqGrid(pSSCBunch, ch, pEI->grid_freq);
        SSCBUNCH_SetQSinusoidFreqGrid(pSSCBunch, ch, pEI->grid_freq);
        SSCBUNCH_SetSinusoidAmpGrid(pSSCBunch, ch, pEI->grid_amp);
        SSCBUNCH_SetQSinusoidAmpGrid(pSSCBunch, ch, pEI->grid_amp);
        SSCBUNCH_SetSinusoidPhaseJitterBand(pSSCBunch, ch, pEI->phase_jitter_band);
        SSCBUNCH_SetQSinusoidPhaseJitterBand(pSSCBunch, ch, pEI->phase_jitter_band);
        SSCBUNCH_SetSinusoidPhaseJitterPercentage(pSSCBunch, ch, pEI->phase_jitter_percentage);
        SSCBUNCH_SetQSinusoidPhaseJitterPercentage(pSSCBunch, ch, pEI->phase_jitter_percentage);
        SSCBUNCH_SetSinusoidRelevanceThreshold(pSSCBunch, ch, pEI->rel_threshold);
        SSCBUNCH_SetQSinusoidRelevanceThreshold(pSSCBunch, ch, pEI->rel_threshold);
    }
}
void SSC_Encoder_Close(PROCESSINFO *pPI, CHUNK_LENGTHS * pChunkLengths)
{
    SSC_ERROR       Error                 = SSC_ERROR_OK ;
    SSC_EXCEPTION   tec                   = SSC_INIT_EXCEPTION ;
    SSC_EXCEPTION  *the_exception_context = &tec ;

    Try
    {
        pPI->pas   = SSC_AUDIOREAD_DestroyInstance             (pPI->pas  );
        pPI->pfs   = SSC_FILTER_DestroyInstance                (pPI->pfs  );
        pPI->ptds  = SSC_TRANSIENTDETECTION_DestroyInstance    (pPI->ptds );
        pPI->ptas  = SSC_STEREO_ANALYSIS_DestroyInstance       (pPI->pstas );
        pPI->ptas  = SSC_TRANSIENT_ANALYSIS_DestroyInstance    (pPI->ptas );
        pPI->ptss  = SSC_TRANSIENT_SYNTHESIS_DestroyInstance   (pPI->ptss );
        pPI->ptqs  = SSC_TRANSIENT_QUANTISATION_DestroyInstance(pPI->ptqs );
        pPI->psas  = SSC_SINUSOID_ANALYSIS_DestroyInstance     (pPI->psas );
        pPI->psss  = SSC_SINUSOID_SYNTHESIS_DestroyInstance    (pPI->psss );
        pPI->psps  = SSC_SINUSOID_PSYCHO_DestroyInstance       (pPI->psps );
        pPI->psrs  = SSC_SINUSOID_REMOVE_DestroyInstance       (pPI->psrs );
        pPI->psts  = SSC_SINUSOID_TRACKING_DestroyInstance     (pPI->psts );
        pPI->psqs  = SSC_STEREO_QUANTISATION_DestroyInstance   (pPI->pstqs );
        pPI->psqs  = SSC_SINUSOID_QUANTISATION_DestroyInstance (pPI->psqs );
        pPI->pnas  = SSC_NOISE_ANALYSIS_DestroyInstance        (pPI->pnas );
        pPI->pntes = SSC_NOISE_TEMP_ENV_DestroyInstance        (pPI->pntes );
        pPI->pnqs  = SSC_NOISE_QUANTISATION_DestroyInstance    (pPI->pnqs );
        pPI->pbsfs = SSC_BITSTREAM_DestroyInstance             (pPI->pbsfs,
                                            (pChunkLengths->FileLength.Value),
                                            &(pChunkLengths->FileLength.Position),
                                            &(pChunkLengths->FileLength.Bytes),
                                            (pChunkLengths->AudioLength.Value),
                                            &(pChunkLengths->AudioLength.Position),
                                            &(pChunkLengths->AudioLength.Bytes),
                                            (pChunkLengths->NrOfFrames.Value),
                                            &(pChunkLengths->NrOfFrames.Position),
                                            &(pChunkLengths->NrOfFrames.Bytes));

        SSC_FREE(pPI, "");
    }
    Catch(Error)
    {
        printf("Error closing processes\n");
    }
}

void SSC_WriteChunkLengths(CHUNK_LENGTHS ChunkLengths, HANDLE_BSBITSTREAM stream)
{
    FILE * fp;
    long nFilePos;

    fp = BsFileHandle(stream);

    nFilePos    = ftell( fp);

    /* Write file length */
    fseek(fp, ChunkLengths.FileLength.Position, SEEK_SET);
    fwrite(ChunkLengths.FileLength.Value,1,ChunkLengths.FileLength.Bytes,fp);

    /* Write audio length */
    fseek(fp, ChunkLengths.AudioLength.Position, SEEK_SET);
    fwrite(ChunkLengths.AudioLength.Value,1,ChunkLengths.AudioLength.Bytes,fp);

    /* Write chunk length */
    fseek(fp, ChunkLengths.NrOfFrames.Position, SEEK_SET);
    fwrite(ChunkLengths.NrOfFrames.Value, 1, ChunkLengths.FileLength.Bytes,fp);

    fflush(fp);

    /* Restore file position */
    if( nFilePos != -1 )
    {
        fseek(fp, nFilePos, SEEK_SET );
    }
}
    

void SSC_FreeEncoderInfo(ENCODERINFO *pEI)
{
    SSC_FREE(pEI->in_dir_name, "");
    SSC_FREE(pEI->in_file_name, "");
    SSC_FREE(pEI->out_dir_name, "");
    SSC_FREE(pEI->out_file_name, "");

    SSC_FREE(pEI, "");
}
