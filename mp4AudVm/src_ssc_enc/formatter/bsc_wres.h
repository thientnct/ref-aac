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

Source file: bsc_wres.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
TM: Tjeu Mans,         Philips CE - PDS-labs Eindhoven <tjeu.mans@philips.com>
AR: Aad Rijnberg,      Philips CE - PDS-labs Eindhoven <aad.rijnberg@philips.com>
NT:    Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:    Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
25 Jul 2002 TM      m8540 improved noise coding + 8 sf/frame
16-Aug-2002 AR      Changes for m8540 core experiment
20 Sep 2002 NT      RM2 change
21-Nov-2003 AG      Changes for RM4
 5-Jan-2004 AG      Changes for RM5
************************************************************************/

/**************************************************************************
 *
 * Bit Stream Conversion tools
 *
 * bsc_wres.h
 *
 * contains all routines for formatting and writing to the elementary stream
 *
 **************************************************************************/

#ifndef __bsc_wres_h
#define __bsc_wres_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/* noise */
#define BSC_N_DEN_MIN          -256
#define BSC_N_DEN_MAX          256
#define BSC_N_DEN_DELTA_MIN    -512
#define BSC_N_DEN_DELTA_MAX    512

#define BSC_N_ENV_GAIN_MIN            0
#define BSC_N_ENV_GAIN_MAX          101
#define BSC_N_ENV_GAIN_DELTA_MIN   -101
#define BSC_N_ENV_GAIN_DELTA_MAX    101

#define BSC_N_LSF_MIN            0
#define BSC_N_LSF_MAX          255 
#define BSC_N_LSF_DELTA_MIN   -255
#define BSC_N_LSF_DELTA_MAX    255

/* sinusoids */
#define BSC_S_FREQ_MIN          0
#define BSC_S_FREQ_MAX          3880
#define BSC_S_FREQ_DELTA_MIN    -40
#define BSC_S_FREQ_DELTA_MAX    40

#define BSC_S_AMP_MIN           0
#define BSC_S_AMP_MAX           241
#define BSC_S_AMP_DELTA_MIN     -32
#define BSC_S_AMP_DELTA_MAX     32

#define BSC_S_FREQPHA_ADPCM_MIN 0
#define BSC_S_FREQPHA_ADPCM_MAX 3

#define BSC_S_PHASE_MIN         -16
#define BSC_S_PHASE_MAX         15

#define BSC_S_ADPCM_GRID_MIN    0
#define BSC_S_ADPCM_GRID_MAX    21

#define BSC_S_CONT_MIN          0
#define BSC_S_CONT_MAX          9

/* transients */
#define BSC_T_B_MIN             0
#define BSC_T_B_MAX             3

#define BSC_T_CHI_MIN           0
#define BSC_T_CHI_MAX           3

#define BSC_T_FREQ_MIN          0
#define BSC_T_FREQ_MAX          485

#define BSC_T_AMP_MIN           0
#define BSC_T_AMP_MAX           30

#define BSC_T_PHASE_MIN         -16
#define BSC_T_PHASE_MAX         15

/* Stereo */

#define OCS_FRAME_FIX_BORDERS           False
#define OCS_FRAME_VAR_BORDERS           True

/* quantisation constants */
#define OCS_NO_IID_STEPS_COARSE         ( 7 )  /* 1 .. +7 */
#define OCS_NO_IID_STEPS_FINE           ( 15 ) /* 1 .. +15 */
#define OCS_NO_ICC_STEPS                ( 8 )  /* 0 .. +7 */
#define OCS_NO_IPD_STEPS                ( 4 )  /* 1 .. +4 */
#define OCS_NO_OPD_STEPS                ( 4 )  /* 1 .. +4 */

#define OCS_NO_IID_LEVELS_COARSE        ( 2 * OCS_NO_IID_STEPS_COARSE + 1 ) /* -7 .. +7 */
#define OCS_NO_IID_LEVELS_FINE          ( 2 * OCS_NO_IID_STEPS_FINE + 1 )   /* -15 .. +15 */
#define OCS_NO_ICC_LEVELS               ( OCS_NO_ICC_STEPS )                /*  0 .. +7 */
#define OCS_NO_IPD_LEVELS               ( 2 * OCS_NO_IPD_STEPS + 1 )        /* -8 .. +8 */

#define OCS_IPD_STEP_SIZE               ( SSC_PI / OCS_NO_IPD_STEPS )
#define OCS_OPD_STEP_SIZE               ( SSC_PI / OCS_NO_OPD_STEPS )

/* bitstream constants */
#define BITS_IID_MODE                   ( 3 )
#define MAX_IID_MODE                    ( 5 )

#define BITS_ICC_MODE                   ( 3 )
#define MAX_ICC_MODE                    ( 5 )

#define BITS_ENVELOPES                  ( 2 )
#define BITS_BORDER                     ( 5 )

#define DIFF_FREQ_CODING                False
#define DIFF_TIME_CODING                True

#define ICC_RANGE_MIN                   (  0 )
#define ICC_RANGE_MAX                   (  OCS_NO_ICC_STEPS - 1 )

#define ICC_D_RANGE_MIN                 ( ICC_RANGE_MIN - ICC_RANGE_MAX )
#define ICC_D_RANGE_MAX                 ( ICC_RANGE_MAX - ICC_RANGE_MIN )

#define BITS_EXTENSION_SIZE             ( 4 )
#define BITS_EXTENSION_ESC_COUNT        ( 8 )

#define BITS_EXTENSION_ID               ( 2 )
#define PS_EXTENSION_ID                 ( 0 )

#define IPD_RANGE_MIN                   ( 0 )
#define IPD_RANGE_MAX                   ( 2 * OCS_NO_IPD_STEPS - 1 )
#define OPD_RANGE_MIN                   ( 0 )
#define OPD_RANGE_MAX                   ( 2 * OCS_NO_OPD_STEPS - 1 )

#define IPD_D_RANGE_MIN                 ( 0 )
#define IPD_D_RANGE_MAX                 ( IPD_RANGE_MAX - IPD_RANGE_MIN )
#define OPD_D_RANGE_MIN                 ( 0 )
#define OPD_D_RANGE_MAX                 ( OPD_RANGE_MAX - OPD_RANGE_MIN )

#define BSC_HISTORY             4
#define BSC_FUTURE              2
    
/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct _BSC_FILE_HEADER
{
    UInt EncoderVersion;

    UInt DecoderLevel;
    UInt SampleRateIndex;
    UInt UpdateRate;
    UInt SynthesisMethod;
    Bool Enhancement;
    SSC_MODE        Mode;
    SSC_MODE_EXT    ModeExt;
    UInt EnvNoiseOrder;

    UInt            Channels; 
    UInt            SubFrameSize;
    
    /* fixed for the time being should be in future in frame_header*/
    SInt             StereoUpdateRate;
    SInt             StereoIdxBins;
    SInt             StereoIdxBinsItd;
    
} BSC_FILE_INFO;

typedef struct _BSC_FRAME_HEADER
{
    UInt RefreshSinus;
    UInt RefreshSinusNextFrame;
    UInt RefreshNoise;
    UInt RefreshStereo;

    UInt NumberDen;

    UInt freqGranularity;
    UInt ampGranularity;

    UInt PhaseJitterPresent;
    UInt PhaseJitterBand;
    UInt PhaseJitterPercentage;

} BSC_FRAME_HEADER;

typedef struct _BSC_FRAME_DATA
{
    /* Transients */
    SSC_TRANSIENT_QPARAM    T [SSC_MAX_SUBFRAMES+BSC_HISTORY+BSC_FUTURE];
    /* Sinusoids */
    SSC_SINUSOID_QPARAM     S [SSC_MAX_SUBFRAMES+BSC_HISTORY+BSC_FUTURE];
    /* Noise */
    SSC_NOISE_QPARAM        N [SSC_MAX_SUBFRAMES+BSC_HISTORY+BSC_FUTURE];

} BSC_FRAME_DATA;

typedef struct _BSC_FRAME
{
    BSC_FRAME_HEADER  frameHeader;
    BSC_FRAME_DATA    frameData[SSC_MAX_CHANNELS];

    /* Stereo */
    SSC_STEREO_QPARAM     ST [SSC_MAX_SUBFRAMES+BSC_HISTORY+BSC_FUTURE];

} BSC_FRAME;

typedef struct _BSC_FRAME_STATISTICS
{
    SInt HeaderBits[SSC_MAX_SUBFRAMES];
    SInt JitterBits[SSC_MAX_SUBFRAMES];

    SInt TraPresentBits[SSC_MAX_SUBFRAMES];
    SInt TraLocBits[SSC_MAX_SUBFRAMES];
    SInt TraTypeBits[SSC_MAX_SUBFRAMES];
    SInt TraParBits[SSC_MAX_SUBFRAMES];
    SInt TraChiBits[SSC_MAX_SUBFRAMES];
    SInt TraNrOfSinBits[SSC_MAX_SUBFRAMES];
    SInt TraSinFreqBits[SSC_MAX_SUBFRAMES];
    SInt TraSinAmpBits[SSC_MAX_SUBFRAMES];
    SInt TraSinPhiBits[SSC_MAX_SUBFRAMES];

    SInt SinConSContBits[SSC_MAX_SUBFRAMES];
    SInt SinConFreqBits[SSC_MAX_SUBFRAMES];
    SInt SinConAmpBits[SSC_MAX_SUBFRAMES];
    SInt SinConPhiBits[SSC_MAX_SUBFRAMES];
    SInt SinConDeltaFreqBits[SSC_MAX_SUBFRAMES];
    SInt SinConDeltaAmpBits[SSC_MAX_SUBFRAMES];
    SInt SinConEnhPhiBits[SSC_MAX_SUBFRAMES];

    SInt SinNrOfBirthBits[SSC_MAX_SUBFRAMES];
    SInt SinBirthSContBits[SSC_MAX_SUBFRAMES];
    SInt SinBirthFreqBits[SSC_MAX_SUBFRAMES];
    SInt SinBirthAmpBits[SSC_MAX_SUBFRAMES];
    SInt SinBirthPhiBits[SSC_MAX_SUBFRAMES];
    SInt SinBirthDeltaFreqBits[SSC_MAX_SUBFRAMES];
    SInt SinBirthDeltaAmpBits[SSC_MAX_SUBFRAMES];

    SInt NoiDenLARBits[SSC_MAX_SUBFRAMES];
    SInt NoiDeltaDenLARBits[SSC_MAX_SUBFRAMES];
    SInt NoiEnvGainBits[SSC_MAX_SUBFRAMES];
    SInt NoiDeltaEnvGainBits[SSC_MAX_SUBFRAMES];
    SInt NoiPoleLaguerreBits[SSC_MAX_SUBFRAMES];
    SInt NoiGridLaguerreBits[SSC_MAX_SUBFRAMES];
    SInt NoiLsfBits[SSC_MAX_SUBFRAMES];
    SInt NoiDeltaLsfBits[SSC_MAX_SUBFRAMES];
    SInt NoiOverlapLsfBits[SSC_MAX_SUBFRAMES];
    SInt StereoWindowType[SSC_MAX_SUBFRAMES];
    SInt StereoTranPosIdx[SSC_MAX_SUBFRAMES];
    
    SInt StereoHeader[SSC_MAX_SUBFRAMES];
    SInt StereoEnvelopes[SSC_MAX_SUBFRAMES];
    SInt StereoData[SSC_MAX_SUBFRAMES];
    SInt StereoExtension[SSC_MAX_SUBFRAMES];
 
    SInt PaddingBits[SSC_MAX_SUBFRAMES];

    SInt BitsSubframe[SSC_MAX_SUBFRAMES];
    UInt TotalBytes;
} BSC_FRAME_STATISTICS;

typedef struct _BSC_WRES
{
    BSC_FRAME_STATISTICS frameStatistics;

    UInt TotalFramesWritten;
    UInt TotalBytesFileHeader;
    UInt TotalBytesFileAudio;  /* includes only bytes written for an audio frame */

    SSCLOG*           pLog;
    BSC_FRAME_BUFFER* pBuf;

    BSC_FILE_INFO     fileInfo;
    FILE*             pStream;

} BSC_WRES;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void      BSC_WRES_Initialise(void);
void      BSC_WRES_Free(void);

BSC_WRES* BSC_WRES_CreateInstance(const char* fileName, SSCLOG* pLog, BSC_FILE_INFO* pInfo);
void      BSC_WRES_DestroyInstance(BSC_WRES* pWres);

#ifdef SSC_MP4REF_BUILD
SSC_ERROR BSC_WRES_WriteFileHeader(BSC_WRES* pWres, unsigned char ** ppHeaderData, int * pHeaderBytes);
SSC_ERROR BSC_WRES_WriteFileFrame(BSC_WRES* pWres, BSC_FRAME* fp,  UInt bitsFrame[SSC_MAX_SUBFRAMES][6], unsigned char ** ppFrameData, int * pFrameBytes);
SSC_ERROR BSC_WRES_GetFileChunkLengths(BSC_WRES* pWres,
                                       UByte* pFileLengthValue,
                                       SInt*  pFileLengthPos,
                                       SInt*  pFileLengthBytes,
                                       UByte* pAudioLengthValue,
                                       SInt*  pAudioLengthPos,
                                       SInt*  pAudioLengthBytes,
                                       UByte* pNrOfFramesValue,
                                       SInt*  pNrOfFramesPos,
                                       SInt*  pNrOfFramesBytes
                                       );

#else
SSC_ERROR   BSC_WRES_WriteFileHeader(BSC_WRES* pWres );
SSC_ERROR   BSC_WRES_WriteFileFrame(BSC_WRES* pWres, BSC_FRAME* fp, UInt bitsFrame[SSC_MAX_SUBFRAMES][6]);
SSC_ERROR   BSC_WRES_WriteFileChunkLengths(BSC_WRES* pWres);
#endif



#ifdef __cplusplus
}
#endif

#endif
