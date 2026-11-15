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

Source file: Imp_Audio.c

Required libraries: libsndfile

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
 *   Module              : Audio Read implementation
 *
 *   Description         : Implementation of the audio_read process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with IMP_AUDIO_
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

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "SSC_System.h"
#include "Imp_Audio.h"
#include "Imp_Error.h"

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

static void LOCAL_SetError( SSCBUNCH* pSscBunch, SInt nError );

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_AUDIO_Initialise(void)
{
}

SSC_AUDIO_STATE* IMP_AUDIO_CreateInstance( SSC_AUDIO_INFO* pAudioInfo, Char* szLogFile, SInt nTraceLevel, int numChannel, float fSample, long fileNumSample)
{
    SSC_AUDIO_STATE* pas    = NULL ;
    SSC_EXCEPTION    tec;
    SSC_EXCEPTION*   the_exception_context = &tec;

	FILE  fpinfo;

    SSC_ERROR        Error = SSC_ERROR_OK ;

    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        char   szPath[256] ;

        SInt nr_of_channels = 1;
        SInt mode           = 0 ;
        SInt mode_ext       = 0;

        if ( pAudioInfo == NULL )
        {
            printf( "\nError: Invalid pointer to audio_read info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pas = SSC_CALLOC( sizeof(SSC_AUDIO_STATE), 1, "AudioRead State Instance" );
        if( pas == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for audio_read instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        pas->pLog  = NULL;
        pas->pdBuf = NULL;
		pas->pfBuf = NULL;

        IMP_AUDIO_Initialise();

#ifdef LOG_ENABLE
        {
            SSCLOGINFO li ;
                              Char* cp ;

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
            strncpy( li.szTitle, "SSC_audio", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;

            pas->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pas->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
#endif
        SSCLOG_TRACE4( pas->pLog, "IMPAUDIO_CreateInstance\n" );

        strcpy( szPath, pAudioInfo->dirName );
        strcat( szPath, pAudioInfo->fileName );

		/* RJ: Reading and writing done by application */
		/* Just copy the audio file pointer instead */

		pas->af_info = SSC_CALLOC( sizeof(AFILE), 1, "" );
		pas->af_file = &fpinfo;

		pas->af_info->Nsamp = fileNumSample; /* RJ: Just copy values from application */
		if ( pas->af_info->Nsamp <= 0 )

        {
            printf( "\nWarning: No samples inside file: %s \n", szPath );
            Throw SSC_ERROR_NOT_AVAILABLE ;
        }

		pas->af_info->Nchan = numChannel; /* RJ: Just copy values from application */
		
		nr_of_channels = pas->af_info->Nchan;
 
        pAudioInfo->numberChannels  = nr_of_channels ;
        pAudioInfo->nMode           = mode ;
        pAudioInfo->nModeExt        = mode_ext ;

        if ((pas->af_info->Nsamp < pAudioInfo->numberSamples) |
            (pAudioInfo->numberSamples == 0))

        {
			pAudioInfo->numberSamples = pas->af_info->Nsamp;
        }

        /* at the moment only 44100 is supported but in future other
         * fs will be here therefore switch/case*/

		pas->af_info->Sfreq = fSample; /* RJ: Just copy values from application */
		pAudioInfo->fs = (int)pas->af_info->Sfreq;

		switch ( pAudioInfo->fs )
        {
            case 44100:
                pAudioInfo->samplingRate    = 1;
                pAudioInfo->updateRate      = 4;
                pAudioInfo->sf_size         = 384;
                pAudioInfo->numberSubFrames = (SInt)ceil( pAudioInfo->numberSamples / pAudioInfo->sf_size);
                break;
            default:
                printf( "\nWarning: unsupported sample rate\n" );
                Throw SSCAUDIO_UNSUPPORTED_SAMPLERATE ;
                break;
        }

        /* save some data also in the module state for later use */
        pas->sf_size              = pAudioInfo->sf_size;
        pas->numberChannels       = pAudioInfo->numberChannels;

        pas->nCurPos              = 0;
        pas->nStopPos             = (SInt)(pas->nCurPos + pAudioInfo->numberSamples) ;
        pas->sf_counter = 0 ;

        pas->pdBuf = SSC_CALLOC( sizeof(Double)*pas->numberChannels * pas->sf_size, 1, "Audio Samples Buffer" );
        if ( pas->pdBuf == NULL )
        {
            printf( "\nWarning: unable to create audio read buffer\n" );
                Throw SSCAUDIO_INIT_BUFFER_FAILED ;
        }

		pas->pfBuf = SSC_CALLOC( sizeof(FloatSSC)*pas->numberChannels * pas->sf_size, 1, "Audio Samples Buffer Float" );

        if ( pas->pfBuf == NULL )
        {
            printf( "\nWarning: unable to create audio read buffer\n" );
                Throw SSCAUDIO_INIT_BUFFER_FAILED ;
        }
    }
    Catch( Error )
    {
        pas = IMP_AUDIO_DestroyInstance( pas );
    }
    return pas ;
}


SSCBUNCH* IMP_AUDIO_Process_NoAudioRead( SSC_AUDIO_STATE* pas, SSCBUNCH* pSscWorkBunch, float ** p_time_signal )
{
    /* nStopSample will not be processed ! */
    SInt nCount = pas->nStopPos - pas->nCurPos ;

	SInt ch, sample;
	Double * pdBuffer;

    if ( pas != NULL )
    {
            pas->sf_counter++ ;
            SSCBUNCH_SetSfCounter( pSscWorkBunch,  pas->sf_counter );
            SSCBUNCH_SetDataFromStream( pSscWorkBunch, (pas->nCurPos < pas->nStopPos) );

            if ( nCount > 0 )
            {
				    memset( pas->pdBuf, 0, sizeof(Double)*pas->af_info->Nchan*pas->sf_size );
                    if ( nCount > pas->sf_size )
                    {
                            nCount = pas->sf_size ;
                    }

                    SSCLOG_TRACE0( pas->pLog, "Audio read processing sample %d, to %d\n",
                                               pas->nCurPos, pas->nCurPos + nCount );

					/* Copy time de-multiplex signal instead of reading it from file */
					/* (already done by application) */
					/* pdBuf is interleaved !! */

					for (sample = 0; sample < pas->sf_size; sample++)
					{
						for (ch = 0; ch < pas->numberChannels; ch++)
						{
							pas->pdBuf[ch+pas->numberChannels*sample] = (double)p_time_signal[ch][sample];
						}
					}
					
					pas->nCurPos += nCount ;

					/* Copy de-multiplexed signal to bunch */

					for (ch = 0; ch < pas->numberChannels; ch++)
					{
						pdBuffer = pSscWorkBunch->pBunch->presidual+(ch*pas->sf_size);

						for (sample = 0; sample < pas->sf_size; sample++)
						{
							pdBuffer[sample] = (double)p_time_signal[ch][sample];
						}
					}
            }
    }

    return pSscWorkBunch ;
}

SSC_AUDIO_STATE* IMP_AUDIO_DestroyInstance( SSC_AUDIO_STATE* pas )
{
    if( pas != NULL )
    {
        SSCLOG_TRACE4( pas->pLog, "SSCAUDIO_DestroyInstance\n" );

        if ( pas->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pas->pLog );
            SSCLOG_FREE();
            pas->pLog = NULL;
        }

        if( pas->pdBuf != NULL )
        {
            SSC_FREE( pas->pdBuf, "Audio Samples Buffer" );
            pas->pdBuf = NULL;
        }

        IMP_AUDIO_Free();

        SSC_FREE( pas, "AudioRead State Instance" );

        pas = NULL;

    }
    else
    {
        fprintf( stdout, "\nError: IMP_AUDIO_DestroyInstance invalid state pointer\n" );
    }

    return pas ;
}


void IMP_AUDIO_Free(void)
{
}

