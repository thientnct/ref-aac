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

Source file: SSC_Bunch.h

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
 *  File        : SSC_Bunch.h
 *
 *  Description : This header file defines the functions used to access the
 *                SBunch object.
 *
 ============================================================================*/

#ifndef _SSC_BUNCH_H_
#define _SSC_BUNCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SSC_Log.h"

typedef struct sscbunch_tag
{
    SInt    nChannels ;
    SInt    nSubframeSize;
    SInt    nDecoderLevel ;
    SBUNCH *pBunch ;
    SSCLOG *pLog ;
}SSCBUNCH;

SSC_INTF_EXPORT SSCBUNCH *SSCBUNCH_InitInstance(SInt nChannels, SInt nSubframeSize, SInt nDecoder_level, SSCLOG *pLog);
SSC_INTF_EXPORT void SSCBUNCH_DestroyInstance(SSCBUNCH *pSscBunch);
SSC_INTF_EXPORT void SSCBUNCH_Clear(SSCBUNCH *pSscBunch);
SSC_INTF_EXPORT void SSCBUNCH_Copy(SSCBUNCH *pSscBunchDst, SSCBUNCH *pSscBunchSrc);

SSC_INTF_EXPORT Bool SSCBUNCH_GetDataFromStream(SSCBUNCH *pSscBunch);
SSC_INTF_EXPORT SInt SSCBUNCH_GetSfCounter(SSCBUNCH *pSscBunch);
SSC_INTF_EXPORT SInt SSCBUNCH_GetErrorCode(SSCBUNCH *pSscBunch);
SSC_INTF_EXPORT SInt SSCBUNCH_GetErrorOrigin(SSCBUNCH *pSscBunch);
SSC_INTF_EXPORT Double *SSCBUNCH_GetActualBitrate(SSCBUNCH *pSscBunch);

SSC_INTF_EXPORT Double *SSCBUNCH_GetSignal(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSynthesized(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetResidual(SSCBUNCH *pSscBunch, SInt nChannel);

SSC_INTF_EXPORT Bool SSCBUNCH_GetTransientPresent(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Bool SSCBUNCH_GetQTransientPresent(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetTransientPos(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQTransientPos(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetTransientType(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQTransientType(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double SSCBUNCH_GetTransientBpar(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double SSCBUNCH_GetQTransientBpar(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double SSCBUNCH_GetTransientChi(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double SSCBUNCH_GetQTransientChi(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetTransientMeixnerSins(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQTransientMeixnerSins(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetTransientMeixnerFreq(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQTransientMeixnerFreq(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetTransientMeixnerAmp(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQTransientMeixnerAmp(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetTransientMeixnerPhase(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQTransientMeixnerPhase(SSCBUNCH *pSscBunch, SInt nChannel);

SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidTrackIdx(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidTrackIdx(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidTrackPrev(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidTrackPrev(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidTrackNext(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidTrackNext(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidFreq(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidFreq(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidAmp(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidAmp(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidPhaseOrg(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidPhaseOrg(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidPhaseCont(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidPhaseCont(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidRelevance(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidRelevance(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetSinusoidQuantGrid(SSCBUNCH *pSscBunch, SInt nChannel );
SSC_INTF_EXPORT Double *SSCBUNCH_GetQSinusoidQuantGrid(SSCBUNCH *pSscBunch, SInt nChannel );

SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidFreqGrid(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidFreqGrid(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidAmpGrid(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidAmpGrid(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidPhaseJitterBand(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidPhaseJitterBand(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetSinusoidPhaseJitterPercentage(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQSinusoidPhaseJitterPercentage(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double SSCBUNCH_GetSinusoidRelevanceThreshold(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double SSCBUNCH_GetQSinusoidRelevanceThreshold(SSCBUNCH *pSscBunch, SInt nChannel);

SSC_INTF_EXPORT Double *SSCBUNCH_GetNoiseDen(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQNoiseDen(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetNoiseEnvCoefs(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double *SSCBUNCH_GetQNoiseEnvCoefs(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double  SSCBUNCH_GetNoiseEnvGain(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double  SSCBUNCH_GetQNoiseEnvGain(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt    SSCBUNCH_GetNoiseEnvOverlap(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt    SSCBUNCH_GetQNoiseEnvOverlap(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double  SSCBUNCH_GetNoiseLaguerrePole(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT Double  SSCBUNCH_GetQNoiseLaguerrePole(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt    SSCBUNCH_GetNoiseLaguerreGrid(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt    SSCBUNCH_GetQNoiseLaguerreGrid(SSCBUNCH *pSscBunch, SInt nChannel);

SSC_INTF_EXPORT SInt SSCBUNCH_GetNoiseNrOfDen(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQNoiseNrOfDen(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetNoiseNrOfEnvCoefs(SSCBUNCH *pSscBunch, SInt nChannel);
SSC_INTF_EXPORT SInt SSCBUNCH_GetQNoiseNrOfEnvCoefs(SSCBUNCH *pSscBunch, SInt nChannel);

SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoEnvBorder            ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoEnvBorder           ( SSCBUNCH *pSscBunch );

SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoIidFreqRes( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoIidFreqRes( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoIidQuant( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoIidQuant( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SInt SSCBUNCH_GetStereoIccFreqRes( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT SInt SSCBUNCH_GetQStereoIccFreqRes( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoHeaderFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoHeaderFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoIidFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoIidFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoIccFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoIccFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoIpdFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoIpdFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoExtFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoExtFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetStereoPcaFlag( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Bool SSCBUNCH_GetQStereoPcaFlag( SSCBUNCH *pSscBunch );

SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoIid                   ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoIid                  ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoIcc                   ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoIcc                  ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoIpd                   ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoIpd                  ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetStereoOpd                   ( SSCBUNCH *pSscBunch );
SSC_INTF_EXPORT Double* SSCBUNCH_GetQStereoOpd                  ( SSCBUNCH *pSscBunch );



SSC_INTF_EXPORT void SSCBUNCH_SetDataFromStream(SSCBUNCH *pSscBunch, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSfCounter(SSCBUNCH *pSscBunch, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetErrorCode(SSCBUNCH *pSscBunch, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetErrorOrigin(SSCBUNCH *pSscBunch, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetActualBitrate(SSCBUNCH *pSscBunch, Double *pNewValue);

SSC_INTF_EXPORT void SSCBUNCH_SetSignal(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSynthesized(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetResidual(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetDemuxResidual(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewData, SInt nChannels);

SSC_INTF_EXPORT void SSCBUNCH_SetTransientPresent(SSCBUNCH *pSscBunch, SInt nChannel, Bool bNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientPresent(SSCBUNCH *pSscBunch, SInt nChannel, Bool bNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientPos(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientPos(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientType(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientType(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientBpar(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientBpar(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientChi(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientChi(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerSins(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerSins(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerFreq(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerFreq(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerAmp(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerAmp(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetTransientMeixnerPhase(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQTransientMeixnerPhase(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);

SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidTrackIdx(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidTrackIdx(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidTrackPrev(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidTrackPrev(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidTrackNext(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidTrackNext(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidFreq(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidFreq(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidAmp(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidAmp(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseOrg(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseOrg(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseCont(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseCont(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidRelevance(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidRelevance(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidFreqGrid(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidFreqGrid(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidAmpGrid(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidAmpGrid(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseJitterBand(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseJitterBand(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidPhaseJitterPercentage(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidPhaseJitterPercentage(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidRelevanceThreshold(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidRelevanceThreshold(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetSinusoidQuantGrid(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQSinusoidQuantGrid(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue );

SSC_INTF_EXPORT void SSCBUNCH_SetNoiseDen(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseDen(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetNoiseEnvGain(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseEnvGain(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetNoiseEnvCoefs(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseEnvCoefs(SSCBUNCH *pSscBunch, SInt nChannel, Double *pNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetNoiseEnvOverlap(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseEnvOverlap(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetNoiseLaguerreGrid(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseLaguerreGrid(SSCBUNCH *pSscBunch, SInt nChannel, SInt nNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetNoiseLaguerrePole(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);
SSC_INTF_EXPORT void SSCBUNCH_SetQNoiseLaguerrePole(SSCBUNCH *pSscBunch, SInt nChannel, Double dNewValue);

SSC_INTF_EXPORT void SSCBUNCH_SetStereoEnvBorder            ( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoEnvBorder           ( SSCBUNCH *pSscBunch, SInt nNewValue );

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIidFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIidFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIidQuant( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIidQuant( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIccFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIccFreqRes( SSCBUNCH *pSscBunch, SInt nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoHeaderFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoHeaderFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIidFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIidFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIccFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIccFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIpdFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIpdFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoExtFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoExtFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoPcaFlag( SSCBUNCH *pSscBunch, Bool nNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoPcaFlag( SSCBUNCH *pSscBunch, Bool nNewValue );

SSC_INTF_EXPORT void SSCBUNCH_SetStereoIid                ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIid               ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIcc                ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIcc               ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoIpd                ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoIpd               ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetStereoOpd                ( SSCBUNCH *pSscBunch, Double* pNewValue );
SSC_INTF_EXPORT void SSCBUNCH_SetQStereoOpd               ( SSCBUNCH *pSscBunch, Double* pNewValue );

#ifdef __cplusplus
}
#endif
#endif /* _SSC_BUNCH_H_ */
