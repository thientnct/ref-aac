/**********************************************************************
MPEG-4 Parametric coder(HVXC)
Conformance program



This software module was originally developed by

Akira Inoue (Sony Corporation)

and edited by

Yuji Maeda (Sony Corporation)

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

Copyright (c) 1999.



Source file: main.c

$Id: hvxc_ini_phase.c,v 1.4 2003/06/27 16:53:24 sps Exp $

Required libraries:
libtsp.a		TSP library

Required modules:
none

Authors:
AI    Akira Inoue, Sony HNC <akira@av.crl.sony.co.jp>
YM    Yuji Maeda, Sony HNC <ymaeda@av.crl.sony.co.jp>
**********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libtsp.h>
#include <libtsp/AFpar.h>
#include <libtsp/FIpar.h>

#define PROGRAM "hvxc_ini_phase"

#define	AVE_UPPER_LIMIT		5.88
#define AVE_LOWER_LIMIT		5.68
#define	DEV_UPPER_LIMIT		0.50
#define DEV_LOWER_LIMIT		0.32

/* V/UV pattern of tested bitstreams is expected as */
/* | UV | UV | V  | V  | UV | UV | V  | V  | UV | UV | V  | V  | ....  	*/
/*              *****               *****               *****	        */

static void usage() 
{
  fprintf(stderr, "usage: %s [options] AFile\n", PROGRAM);
  fprintf(stderr, "  AFileA: file name of test signal\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "  -i val:  frame interval value(default: 80)\n");
  fprintf(stderr, "  -o val:  offset value(default: 0)\n");
  fprintf(stderr, "  -sl val: segment length[samples](default: 80)\n");
  fprintf(stderr, "  -d:      display data at every frame\n");
  fprintf(stderr, "  -h:      show this message\n");
  
  return;
}

static void error(char *msg)
{
  fprintf(stderr, "error(%s): %s\n", PROGRAM, msg);
  usage();
  exit(1);
}

#define NUM_OF_DSTR	200
#define MAX_VAL 8.0
#define MIN_VAL 0.0

int main(int argc, char **argv)
{
  int i, disp=0;
  float *Xa1;
  
  int FrameInterval = 80;
  int FrameOffset = 0;
  int SegmentLength = 80;
  
  int arg_counter;

  AFILE *AFp = NULL;
  long int Nsamp, Nchan;
  float Sfreq;
  long int Na, offs;
  
  float frameRms1;
  float peak, PeakPerRms, MaxPeakPerRms, MinPeakPerRms;
  
  int frm;

  /* to measure average, deviation */
  float array1[10000];
  float avg1, devi1;
  int count = 0;
  
  arg_counter = 1;
  if (argc < 2 || argc > 12) {
    fprintf(stderr, "argc=%d\n", argc);
    usage();
    exit(1);
  }
  
  while (arg_counter < argc) {
    /* exclude option code */
    if (*(argv[arg_counter]) == '-') {
            
      if (strcmp(argv[arg_counter]+1, "h") == 0) {
	usage();
	exit(1);
      }
      else if (strcmp(argv[arg_counter]+1, "i") == 0) {
	sscanf(argv[arg_counter+1], "%d", &FrameInterval);
	fprintf(stdout, "frame interval = %d\n", FrameInterval);
	arg_counter++;
      }
      else if (strcmp(argv[arg_counter]+1, "o") == 0) {
	sscanf(argv[arg_counter+1], "%d", &FrameOffset);
	fprintf(stdout, "offset = %d\n", FrameOffset);
	arg_counter++;
      }
      else if (strcmp(argv[arg_counter]+1, "sl") == 0) {
	sscanf(argv[arg_counter+1], "%d", &SegmentLength);
	fprintf(stdout, "segment length = %d\n", SegmentLength);
	arg_counter++;
      }
      else if (strcmp(argv[arg_counter]+1, "d") == 0) {
	disp = 1;
      }
    }
    
    if (arg_counter == (argc-1)) {
      if ((AFp = AFopenRead(argv[arg_counter], &Nsamp, &Nchan, &Sfreq, stderr)) ==  NULL) {
	fprintf(stderr, "can't open file : %s\n", argv[arg_counter]);
	exit(1);
      }
      if (Nchan != 1) {
	fprintf(stderr, "only mono channel supported : %s\n", argv[arg_counter]);
	exit(1);
      }
    }
    
    arg_counter++;
  }

  if (Sfreq != 8000) {
    fprintf(stderr, "sampling frequency is NOT 8000Hz\n");
    exit(1);
  }

  /* allocate buffers */
  /* Xa1 = new float[SegmentLength]; */
  Xa1 = (float *)malloc(SegmentLength * sizeof(float));
  
  offs = FrameOffset;
  
  frm = 0;

  MaxPeakPerRms = 0.0f;
  MinPeakPerRms = 32677.0f;
  
  while(1) {
    Na = AFreadData(AFp, offs-SegmentLength/2, Xa1, SegmentLength);
    if (Na <= 0 || Na < SegmentLength)
      break;

    offs += FrameInterval; 

    frameRms1 = 0.0f;
    for (i = 0; i < SegmentLength; i++) {
      frameRms1 += Xa1[i]*Xa1[i];
    }
    frameRms1 = sqrt(frameRms1/(float)SegmentLength);

    peak = 0.0f;
    for (i = 0; i < SegmentLength; i++) {
      if (Xa1[i] > peak) peak = Xa1[i];
    }
    
    PeakPerRms = peak/frameRms1;

    if (((frm % 8) >= 5) && ((frm % 8) <= 7)) {
      if (PeakPerRms > MaxPeakPerRms) {
	MaxPeakPerRms = PeakPerRms;
      }
      if (PeakPerRms < MinPeakPerRms) {
	MinPeakPerRms = PeakPerRms;
      }

      /* to measure average, deviation */
      array1[count] = PeakPerRms;
      count++;
    }

    if ( disp ) {
      if (((frm % 8) >= 5) && ((frm % 8) <= 7)) {
        fprintf(stdout, "[%3dth frame]*peak/rms=%5.2f(rms=%6.1f,peak=%7.1f)\n", frm, PeakPerRms, frameRms1, peak);
      }
      else {
        fprintf(stdout, "[%3dth frame] peak/rms=%5.2f(rms=%6.1f,peak=%7.1f)\n", frm, PeakPerRms, frameRms1, peak);
      }
    }
   
    frm++;
  }

  fprintf(stdout, "peak/rms =[%6.4f,%6.4f]\n", MinPeakPerRms, MaxPeakPerRms);

  /* average, deviation calculation */
  avg1 = 0.0f;
  for (i = 0; i < count; i++) {
    avg1 += array1[i];
  }
  avg1 /= (float)count;
  
  devi1 = 0.0f;

  for (i = 0; i < count; i++) {
    devi1 += (array1[i]-avg1)*(array1[i]-avg1);
  }
  devi1 = sqrt(devi1/(float)count);
  
  fprintf(stdout, "avg(peak/rms)=%6.4f, devi(peak/rms)=%6.4f\n", 
	  avg1, devi1);
  /* fprintf(stdout, "%6.4f %6.4f\n", avg1, devi1); */

  fprintf(stdout, "count=%d\n", count);

  if (AVE_LOWER_LIMIT < avg1 && avg1 < AVE_UPPER_LIMIT
   && DEV_LOWER_LIMIT < devi1 && devi1 < DEV_UPPER_LIMIT)
    fprintf(stdout, "********** Test condition is satisfied. **********\n");
  else
    fprintf(stdout, "********** Test condition is unsatisfied. **********\n");
  
  /* delete buffers */
  free( Xa1 );
  /* delete Xa1; */

  return ( 0 );
}

