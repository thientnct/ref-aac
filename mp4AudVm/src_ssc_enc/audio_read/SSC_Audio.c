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

Source file: SSC_Audio.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
01-Oct-2002 NT      RM2 change 
************************************************************************/

/******************************************************************************
 *
 *   Module              : AudioRead DLL entry points
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_audio_read_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <stdlib.h>
#include "PlatformTypes.h"

#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_System.h"
#include "cexcept.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_Audio.h"
#include "SSC_Audio.h"

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

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Wrapper function to create the module state structure and then
 * calls the IMP_AUDIO_CreateInstance function to get a module
 * info structure. */
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_AUDIOREAD_InitInstance( ENCODERINFO* pEncoderInfo, int numChannel, float fSample, long fileNumSample)
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    SSC_MODULESTATE* pms = NULL ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "SSC_audio" ) ;
    Char             szLogFile[260] ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        Char* cp = NULL;

        SSC_AUDIO_INFO AudioInfo;

        if( pEncoderInfo == NULL )
        {
            printf( "\nError: Invalid pointer to encoderinfo\n" );
            Throw SSC_ERROR_NOT_SUPPORTED ;
        }
        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "AudioRead Module State" );
        if( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for audio_read instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        /* clear first all pointer values inside module state */
        pms->pAudioState  = NULL;
        pms->pSscWorkPipe = NULL;

        IMP_AUDIO_Initialise();

        /* fill the info struct to initialize the imp_audio object */
        AudioInfo.dirName       = pEncoderInfo->in_dir_name;
        AudioInfo.fileName      = pEncoderInfo->in_file_name;
        AudioInfo.numberSamples = pEncoderInfo->number_samples;
        AudioInfo.channelDepend = pEncoderInfo->channel_depend;


        /* identify/compose the log filename */
        strcpy( szLogFile, pEncoderInfo->out_dir_name );
        strcat( szLogFile, pEncoderInfo->out_file_name);
        cp = strrchr( szLogFile, (int)'.' );
        if ( cp != NULL )
        {
            *cp = '\0' ; /* strip extension, if found */
        }
        strcat( szLogFile, "_audio.log");

        pms->pAudioState = IMP_AUDIO_CreateInstance( &AudioInfo, szLogFile, nTraceLevel, numChannel, fSample, fileNumSample);
        if( pms->pAudioState == NULL )
        {
            printf( "\nError: SSCaudio_read_CreateInstance failed.\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch (AudioInfo.numberChannels)
        {
            case 1:
                AudioInfo.nMode     = 0;
                AudioInfo.nModeExt  = 0;
                break ;
            case 2:
                AudioInfo.nMode     = 2;
                if (AudioInfo.channelDepend == 1)
                {
                    AudioInfo.nModeExt = 1;
                }
                else
                {
                    AudioInfo.nModeExt = 0;
                }
                break;
            default:
                printf( "\nWarning: unsupported number of channels\n" );
                Throw SSCAUDIO_INVALID_NR_CHANNELS ;
                break;
        }
        /* all went well so copy the found information back to encoderinfo */
        pEncoderInfo->nr_of_channels  = AudioInfo.numberChannels;
        pEncoderInfo->mode            = AudioInfo.nMode;
        pEncoderInfo->mode_ext        = AudioInfo.nModeExt;
        pEncoderInfo->number_samples  = AudioInfo.numberSamples;
        pEncoderInfo->sampling_rate   = AudioInfo.samplingRate;
        pEncoderInfo->fs              = AudioInfo.fs;
        pEncoderInfo->update_rate     = AudioInfo.updateRate;
        pEncoderInfo->sf_size         = AudioInfo.sf_size;
        pEncoderInfo->nr_of_subframes = AudioInfo.numberSubFrames;

    }
    Catch(Error)
    {
        pms = SSC_AUDIOREAD_DestroyInstance( pms );
    }
    return pms ;

}

/* Wrapper function which calls the IMP_AUDIO_Process_NoAudioRead function */
/* RJ: adapted for incorporation in the MPEG reference software */
SSC_INTF_EXPORT SSCBUNCH* SSC_AUDIOREAD_Process_NoAudioRead( SSC_MODULESTATE* pms, SSCBUNCH* pSscWorkBunch, float ** p_time_signal )
{
    if ( pms != NULL )
    {
        pSscWorkBunch = IMP_AUDIO_Process_NoAudioRead( pms->pAudioState, pSscWorkBunch, p_time_signal );
    }
    else
    {
        fprintf(stdout,"\nError: Illegal Noise Module state in process\n");
    }

    return pSscWorkBunch ;
}


/* Wrapper function which frees all memory allocated by the
 * SSC_AUDIO_Init call and then calls the IMP_AUDIO_DestroyInstance to
 * free the memory allocated on the IMP_AUDIO_CreateInstance call  */
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_AUDIOREAD_DestroyInstance( SSC_MODULESTATE* pms )
{
    if ( pms != NULL )
    {
        if ( pms->pAudioState != NULL )
        {
            pms->pAudioState = IMP_AUDIO_DestroyInstance( pms->pAudioState );
        }

        SSC_FREE(pms, "AudioRead Module State" );
        pms = NULL ;
    }

    return pms ;
}
