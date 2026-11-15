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

Source file: SSC_Streaming.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AR: Aad Rijnberg,  Philips CE - PDS-labs Eindhoven <aad.rijnberg@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:     Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
16-Aug-2002 AR      Changes for m8540 core experiment
20-Sep-2002 NT      RM2 change
21-Nov-2003 AG      Changes for RM4
************************************************************************/

/**************************************************************************
 *
 * Bit Stream Conversion tools
 *
 * bsc_bitstream.h
 *
 * contains all structures of the tools
 *
 **************************************************************************/

#ifndef __ssc_streaming_h
#define __ssc_streaming_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct _SSC_STRM_INFO
{
  UInt EncoderVersion;

  UInt DecoderLevel;
  UInt SampleRateIndex;
  UInt UpdateRate;
  UInt SynthesisMethod;
  Bool Enhancement;
  UInt Mode;
  UInt ModeExt;
  UInt EnvNoiseOrder;
  UInt RefreshStrategySinus;
  UInt RefreshStrategyNoise;
  UInt RefreshStrategyStereo;
  UInt SubFrameSize;
  UInt Channels;

  SInt idxBins;
  SInt idxBinsItd;
  SInt stereoUpdateRate;

} SSC_STRM_INFO;


typedef struct _SSC_STRM
{
    SSC_STRM_INFO Info;
    BSC_FRAME     FrameParam;
    SInt          IndexArray[SSC_MAX_CHANNELS][SSC_ENCODER_MAX_SINS];
    SInt          IndexArrayLA[SSC_MAX_CHANNELS][SSC_ENCODER_MAX_SINS];

    SInt      newSubframes;
    UInt      framesWritten;
    UInt      lastSinusRefresh;

    BSC_WRES* pWres;
    SSCLOG*   pLog;

    SSCPIPE*  pPipe;
} SSC_STRM;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void      SSC_STRM_Initialise(void);
void      SSC_STRM_Free(void);


#ifdef SSC_MP4REF_BUILD
SSC_STRM* SSC_STRM_CreateInstance(SSC_STRM_INFO* pInfo, char* pLogFile, UInt LogLevel, char* pSSCFile, unsigned char ** ppHeaderData, int * pHeaderBytes);
void SSC_STRM_DestroyInstance(SSC_STRM* pStrm,
                              UByte* pFileLengthValue,
                              SInt*  pFileLengthPos,
                              SInt*  pFileLengthBytes,
                              UByte* pAudioLengthValue,
                              SInt*  pAudioLengthPos,
                              SInt*  pAuidoLengthBytes,
                              UByte* pNrOfFramesValue,
                              SInt*  pNrOfFramesPos,
                              SInt*  pNrOfFramesBytes
                              );
SSCBUNCH* SSC_STRM_Process(SSC_STRM* pStrm, SSCBUNCH* pSb, unsigned char ** ppFrameData, int * pFrameBytes);
#else
SSC_STRM* SSC_STRM_CreateInstance(SSC_STRM_INFO* pInfo, char* pLogFile, UInt LogLevel, char* pSSCFile);
void      SSC_STRM_DestroyInstance(SSC_STRM* pStrm);

SSCBUNCH* SSC_STRM_Process(SSC_STRM* pStrm, SSCBUNCH* pSb);
#endif


#ifdef __cplusplus
}
#endif

#endif
