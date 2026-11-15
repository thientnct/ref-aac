/*
This software module was originally developed by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)

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

Copyright (c) 1997.
*/

/* constants */
#define PICOLA_VFL 1 /* fixed length:0 variable length:1 */

#define PAN_MAX_SPEED 2.
#define PAN_MIN_SPEED .5
#define PAN_PMAX_8K 160
#define PAN_PMAX_16K 320
#define PAN_PMIN_8K 32
#define PAN_PMIN_16K 80

#include <stdio.h>
#include <stdlib.h>
#include "pan_picola.h"

/* -------------- */
/* PICOLA modules */
/* -------------- */

/* static */
static int StartPoint;
static int EndPoint;
static int TargetPoint;
static int OutputPoint;
static int WritePoint;
static int Pitch;

static int MaxPitch;
static int MinPitch;

static float *SignalBuffer;
static int BufferLength;
static int FrameLength;

void PicolaInit(
    int MaxFrameLength,
    float SamplingFrequency,
    float SpeedControlFactor
)
{
    int FrameNumber;

    if(SpeedControlFactor<.5 || SpeedControlFactor>2.) {
        printf("\n PICOLA (speed control): speed control range over  %f\n", SpeedControlFactor);
        printf("speed control factor should be 0.5 - 2.0\n");
        exit(1);
    }

    if(16000.==SamplingFrequency) {
        MaxPitch = PAN_PMAX_16K;
        MinPitch = PAN_PMIN_16K;
    }else if(8000.==SamplingFrequency) {
        MaxPitch = PAN_PMAX_8K;
        MinPitch = PAN_PMIN_8K;
    }else {
        printf("\n PICOLA (speed control): not supported sampling frequency %f\n", SamplingFrequency);
        exit(1);
    }

    if(PICOLA_VFL) {
      BufferLength = MaxFrameLength*2 + MaxPitch*2;
    }else {
      FrameNumber = (MaxPitch*2+MaxFrameLength-1)/MaxFrameLength + 1;
      BufferLength = MaxFrameLength*FrameNumber;
    }

/* for debug 
printf("\n FrameLength = %d", MaxFrameLength);
printf("\n MaxPitch = %d", MaxPitch);
printf("\n MinPitch = %d", MinPitch);
printf("\n BufferLength = %d", BufferLength);
printf("\n");
*/
    if(NULL==(SignalBuffer=(float *)calloc(BufferLength, sizeof(float)))) {
        printf("\n PICOLA (speed control): memory allocation error\n");
        exit(2);
    }

    StartPoint = 0;
    EndPoint = 0;
    TargetPoint = MaxPitch; 
    OutputPoint = 0;
    WritePoint = 0;
    Pitch = 0;
    FrameLength = MaxFrameLength;
}

void PicolaFree()
{
    free(SignalBuffer);
}

void mod_picola(
    float InputSignal[],
    int NumInputSample,
    float OutputSignal[],
    float SpeedControlFactor,
    int *DecodeFlag,
    int *OutputFlag
)
{

	int	i;
	float	coef;

    if(1. == SpeedControlFactor){
	    for(i=0;i<FrameLength;i++) OutputSignal[i] = InputSignal[i];
	    *DecodeFlag = 1;
	    *OutputFlag = 1;
	    return;
    }

/* signal input */
/*
printf("start=%d\n", StartPoint);
printf("end=%d\n", EndPoint);
printf("target=%d\n", TargetPoint);
printf("out=%d\n", OutputPoint);
printf("write=%d\n", WritePoint);
printf("pitch=%d\n", Pitch);
printf("in=%d\n", NumInputSample);
printf("\n");
*/
    if(NumInputSample>0) {
        for(i=0;i<NumInputSample;i++) {
            SignalBuffer[EndPoint+i] = InputSignal[i];
        }
        EndPoint += NumInputSample;
    }

    if(EndPoint>BufferLength) {
        printf("\n PICOLA (speed control): buffer overflow\n");
        exit(2);
    }

/* PICOLA processing */
    while(1) {

/* Quick play */
        if(SpeedControlFactor>1. && SpeedControlFactor<=2.){
            if(TargetPoint<=EndPoint) {
                if((FrameLength-WritePoint)<=(TargetPoint-OutputPoint)) {
                    while(WritePoint<FrameLength) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output array is filled */
                    /*    but output point did not reach target point */

                    if(OutputPoint>=FrameLength) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }

                    WritePoint = 0;
                    *OutputFlag = 1;
                    *DecodeFlag = 0;
                    return;

                }else {
                    while(OutputPoint<TargetPoint) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output point reached target point */
                    /*    but output array is not filled */

                    if(OutputPoint>=FrameLength) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }
                    StartPoint = TargetPoint;


                    /* PICOLA processing */

                    if((EndPoint-StartPoint)<(MaxPitch*2)) {
                        *DecodeFlag = 1;
                        *OutputFlag = 0;
                        return;
                    }else {
                        pit_sel_forward(SignalBuffer+StartPoint); 

                        if((StartPoint+Pitch*2)>BufferLength) {
                            printf("\n PICOLA (speed control): buffer overflow\n");
                            printf("    process starting point = %d\n", StartPoint);
                            printf("    waveform interval = %d\n", Pitch);
                            exit(2);
                        }
                        if(StartPoint<0) {
                            printf("\n PICOLA (speed control): buffer underflow\n");
                            printf("    process starting point = %d\n", StartPoint);
                            exit(2);
                        }

                        if(0!=Pitch) {
                            for(i=0;i<Pitch;i++){
                                coef = (i+1)/(float)(Pitch+1);
                                SignalBuffer[StartPoint+Pitch+i] 
                                    = (1.-coef)*SignalBuffer[StartPoint+i]
                                     + coef*SignalBuffer[StartPoint+Pitch+i];
                            }
                            OutputPoint = StartPoint+Pitch;
                            TargetPoint = StartPoint
                                + (int)(SpeedControlFactor/(SpeedControlFactor-1.)*Pitch);
                        }else {
                            OutputPoint = StartPoint;
                            TargetPoint = StartPoint+FrameLength;
                            printf("\n PICOLA processing is skipped");
                        }

                    }
                }

            }else {
                if((FrameLength-WritePoint)<=(EndPoint-OutputPoint)) {
                    while(WritePoint<FrameLength) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output array is filled */
                    /*    remains samples to be outputed */

                    if(OutputPoint>=FrameLength) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }
                    WritePoint = 0;
                    *DecodeFlag = 0;
                    *OutputFlag = 1;
                    return;

                }else {
                    while(OutputPoint<EndPoint) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output array is not filled */
                    /*    requires next frame input */

                    if(OutputPoint>=FrameLength) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }
                    *DecodeFlag = 1;
                    *OutputFlag = 0;
                    return;

                }

            }

/* Slow play */

        }else if(SpeedControlFactor<1. && SpeedControlFactor>=.5) {
            if(TargetPoint<=EndPoint) {
                if((FrameLength-WritePoint)<=(TargetPoint-OutputPoint)) {
                    while(WritePoint<FrameLength) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output array is filled */
                    /*    but output point did not reach target point */

                    if(OutputPoint>=FrameLength+MaxPitch) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }

                    WritePoint = 0;
                    *OutputFlag = 1;
                    *DecodeFlag = 0;
                    return;

                }else {
                    while(OutputPoint<TargetPoint) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output point reached target point */
                    /*    but output array is not filled */

                    if(OutputPoint>=FrameLength+MaxPitch) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }
                    StartPoint = TargetPoint;


                    /* PICOLA processing */

                    if(EndPoint-StartPoint<MaxPitch) {
                        *DecodeFlag = 1;
                        *OutputFlag = 0;
                        return;
                    }else {
                        if(((StartPoint+MaxPitch) > BufferLength) 
                                || StartPoint<MaxPitch) {

                            printf("\n PICOLA (speed control): buffer overflow\n");
                            printf("    process starting point = %d\n", StartPoint);
                            printf("    waveform interval = %d\n", Pitch);
                            exit(2);
                        }

                        pit_sel_backward(SignalBuffer+StartPoint);

                        if(0!=Pitch) {
                            StartPoint -= Pitch;
                            for(i=0;i<Pitch;i++){
                                coef = (i+1)/(float)(Pitch+1);
                                SignalBuffer[StartPoint+i] = 
                                    coef*SignalBuffer[StartPoint+i]
                                        + (1.-coef)*SignalBuffer[StartPoint+Pitch+i];
                            }
                            OutputPoint = StartPoint;
                            TargetPoint = StartPoint
                                + (int)(SpeedControlFactor/(1.-SpeedControlFactor)*Pitch+Pitch);
                        }else {
                            OutputPoint = StartPoint;
                            TargetPoint = StartPoint+FrameLength;
                            printf("\n PICOLA processing is skipped");
                        }
                    }
                }
            }else {
                if((FrameLength-WritePoint)<=(EndPoint-OutputPoint)) {
                    while(WritePoint<FrameLength) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output array is filled */
                    /*    remains samples to be outputed */

                    if(OutputPoint>=FrameLength+MaxPitch) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }
                    WritePoint = 0;
                    *DecodeFlag = 0;
                    *OutputFlag = 1;
                    return;

                }else {
                    while(OutputPoint<EndPoint) {
                        OutputSignal[WritePoint++] = 
                                SignalBuffer[OutputPoint++];
                    }
                    /* <- output array is not filled */
                    /*    requires next frame input */

                    if(OutputPoint>=FrameLength+MaxPitch) {
                        for(i=0;i<BufferLength-FrameLength;i++) {
                            SignalBuffer[i] = SignalBuffer[i+FrameLength];
                        }
                        EndPoint -= FrameLength;
                        TargetPoint -= FrameLength;
                        OutputPoint -= FrameLength;
                    }
                    *DecodeFlag = 1;
                    *OutputFlag = 0;
                    return;

                }

            }

        }else {

            printf("\n Out of control range !!\n");
            exit(3);
        }
/*
printf("  start=%d\n", StartPoint);
printf("  end=%d\n", EndPoint);
printf("  target=%d\n", TargetPoint);
printf("  out=%d\n", OutputPoint);
printf("  write=%d\n", WritePoint);
printf("  pitch=%d\n", Pitch);
printf("  in=%d\n", NumInputSample);
printf("\n");
*/
    } 
}


void pit_sel_backward(
    float *Signal
)
{
    int i, j;
    float MinError;
    float Error;
    float CrossCorrelation;
    float PastSignalEnergy;
    float CurrentSignalEnergy;

    Pitch = 0;
    MinError = 1.0e20;

    PastSignalEnergy = 0.;
    CurrentSignalEnergy = 0.;
    for(i=1;i<MinPitch;i++) {
        PastSignalEnergy += (*(Signal-i))*(*(Signal-i));
        CurrentSignalEnergy += (*(Signal+i-1))*(*(Signal+i-1));
    }

    for(j=MinPitch;j<=MaxPitch;j++) {
        PastSignalEnergy += (*(Signal-j))*(*(Signal-j));
        CurrentSignalEnergy += (*(Signal+j-1))*(*(Signal+j-1));
	    CrossCorrelation = 0.;
	    for(i=0;i<j;i++) 
                CrossCorrelation += (*(Signal-j+i))*(*(Signal+i));

        Error = (PastSignalEnergy + CurrentSignalEnergy - 2*CrossCorrelation)
                    /(float)j;
        if((MinError>Error) && (CrossCorrelation>0.)) {
            MinError = Error;
            Pitch = j;
        }
    }

    if(0==Pitch) {
        if((PastSignalEnergy<1.) || (CurrentSignalEnergy<1.)) {
            Pitch = MinPitch;
        }
    }

}

void pit_sel_forward(
    float  *Signal
)
{
    int i, j;
    float MinError;
    float Error;
    float CrossCorrelation;
    float FirstHalfEnergy;
    float SecondHalfEnergy;

    FirstHalfEnergy = 0.;
    SecondHalfEnergy = 0.;
    for(i=0;i<MinPitch-1;i++) {
        FirstHalfEnergy += (*(Signal+i))*(*(Signal+i));
        SecondHalfEnergy 
            += (*(Signal+i+MinPitch-1))*(*(Signal+i+MinPitch-1));
    }

    Pitch = 0;
    MinError = 1.0e20;

    for(j=MinPitch;j<=MaxPitch;j++) {
        FirstHalfEnergy += (*(Signal+j-1))*(*(Signal+j-1));
        SecondHalfEnergy += ((*(Signal+j*2-1))-(*(Signal+j-1)))
                                *((*(Signal+j*2-1))+(*(Signal+j-1)));
        SecondHalfEnergy += (*(Signal+j*2-2))*(*(Signal+j*2-2));

        CrossCorrelation = 0.;
        for(i=0;i<j;i++) 
            CrossCorrelation += (*(Signal+j+i))*(*(Signal+i));

        Error = (FirstHalfEnergy + SecondHalfEnergy - 2*CrossCorrelation)
                    /(float)j;
        if((MinError>Error) && (CrossCorrelation>0.)) {
            MinError = Error;
            Pitch =j;
        }

    }

    if(0==Pitch) {
        if((FirstHalfEnergy<1.) || (SecondHalfEnergy<1.)) {
            Pitch = MinPitch;
        }
    }

}


