/**********************************************************************
MPEG-4 Parametric coder
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

$Id: hvxc_fcomp.c,v 1.3 2003/06/27 10:42:40 sps Exp $

Required libraries:
libtsp.a		TSP library

Required modules:
hvxcCommon.o		common module

Authors:
AI    Akira Inoue, Sony HNC <akira@av.crl.sony.co.jp>
YM    Yuji Maeda, Sony HNC <ymaeda@av.crl.sony.co.jp>
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libtsp.h>
#include <libtsp/AFpar.h>
#include <libtsp/FIpar.h>

#include "hvxcCommon.h"

#define PROGRAM "hvxc_fcomp"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

static void usage(void) 
{
  fprintf(stderr, "usage: %s [options] AFileA AFileB\n", PROGRAM);
  fprintf(stderr, "  AFileA: file name of test signal\n");
  fprintf(stderr, "  AFileB: file name of reference signal(without noise component addition)\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "  -i val\tframe interval value(default: 160)\n");
  fprintf(stderr, "  -w val\twindow length value(default: 256)\n");
  fprintf(stderr, "  -o val\toffset value(default: 112)\n");
  fprintf(stderr, "  -n val\tframe number of snap(default: --)\n");
  fprintf(stderr, "  -r val\tbit rate to be tested(2000 or 4000, default: 2000)\n");
  fprintf(stderr, "  -h\t\tshow this message\n");
  
  return;
}


#define RECTANGLE 0
#define HANNING 1
#define HAMMING 2
#define BLACKMAN 3
#define RAISED_SIN 4
#define TRIANGLE 5

static void make_window(
float *window,
int len, 
int window_type
)
{
  int i;
  float inv_len = 1.0f/((float)len-1.0f);
#ifdef POW_NORM
  float pow;
#endif

  switch(window_type) {
    
  case RECTANGLE:
    for (i = 0; i < len; i++) {	/* Rectangle */
      window[i] = 1.0f;
    }
    break;

  case HANNING:
    for (i = 0; i < len; i++) {	/* Hanning */
      window[i] = 0.5f*(1.0f-cos(2.0f*M_PI*i*inv_len));
    }
    break;
    
  case HAMMING:
    for (i = 0; i < len; i++) {	/* Hamming */
      window[i] = 0.54f-0.46f*cos(2.0f*M_PI*i*inv_len);
    }
    break;
    
  case BLACKMAN:
    for (i = 0; i < len; i++) {	/* Blackman */
      window[i] = 0.42f-0.5f*cos(2*M_PI*i*inv_len)+0.08f*cos(4.0f*M_PI*i*inv_len);
    }
    break;

  case RAISED_SIN:
    for (i = 0; i < len; i++) {	/* Raised Sin */
      window[i] = 0.5f*(1.0f+sin(2.0f*M_PI*(float)i*inv_len - 0.5f*M_PI));
    }
    break;

  case TRIANGLE:
    for (i = 0; i < len/2; i++) {	/* Triangle(Bertlett) */
      window[i] = 2.0f*(float)i/(float)(len-1);
    }
    for (; i < len; i++) {
      window[i] = 2.0f-2.0f*(float)i/(float)(len-1);
    }
    break;
    
  default:
    for (i = 0; i < len; i++) {	/* Hanning */
      window[i] = 0.5f*(1.0f-cos(2.0f*M_PI*i*inv_len));
    }
    break;
  
  }

#ifdef POW_NORM  
  pow = 0.0f;
  for (i = 0; i < len; i++) {
    pow += window[i]*window[i];
  }
  pow = sqrt(pow);
  for (i = 0; i < len; i++) {
    window[i] /= pow;
  }
#endif
}

static void average_filter(
float *in,	/* in: input array */
float *out,	/* out: output array */
int size,	/* in: size of input/output array */
int tap		/* in: tap number of average filter(odd number) */
)
{
  int i, j;
  float *s;
  s = (float *)malloc(sizeof(float)*(size+tap-1));

  for (i = 0; i < tap/2; i++) {
    s[i] = 0.0f;
  }
  for (; i < size+tap/2; i++) {
    s[i] = in[i-tap/2];
  }
  for (; i < size+tap-1; i++) {
    s[i] = 0.0f;
  }

  for (i = 0; i < size; i++) {
    out[i] = 0.0f;
    for (j = 0; j < tap; j++) {
      out[i] += s[i+j-tap/2];
    }
    out[i] /= (float)tap;
  }
  
  free(s);
}


int main(int argc, char *argv[])
{
  int i;
  float *Xa, *Ya, *Ra, *Xb, *Yb, *Rb; 
  
  float *dif;
  float *window;
  
  float *maxR, *minR;
  float *max, *min;
  
  int FrameInterval = 160;
  int WindowLength = 256;
  int WindowType = HANNING;
  int Offset = 112;
  int dim = 8;
  int SnapFrame = 9999;
  int FFTLength = 256;
  int tapNum = 7;	/* tap number of average filter */
  int bitRate = 2000;
  
  int arg_counter;

  AFILE *AFpA = NULL;	/* test signal */
  AFILE *AFpB = NULL;	/* reference signal */
  long int NsampA, NchanA, NsampB, NchanB;
  float SfreqA, SfreqB;
  long int Na, Nb, offs;

  int frm;
  int check_flag = 0;
  
  FILE *fp;

  arg_counter = 1;
  if (argc < 3 || argc > 7) {
    usage();
    exit(1);
  }
  
  while (arg_counter < argc) {
    /* exclude option code */
    if (*(argv[arg_counter]) == '-') {
            
      switch (*(argv[arg_counter]+1)) {
      case 'h':
	usage();
	exit(1);
                
      case 'i': 
	sscanf(argv[arg_counter+1], "%d", &FrameInterval);
	arg_counter++;
	break;

      case 'w': 
	sscanf(argv[arg_counter+1], "%d", &WindowLength);
	arg_counter++;
	break;
                
      case 'o': 
	sscanf(argv[arg_counter+1], "%d", &Offset);
	arg_counter++;
	break;
                
      case 'r': 
	sscanf(argv[arg_counter+1], "%d", &bitRate);
	if (bitRate != 2000 && bitRate != 4000) {
	  fprintf(stderr, "%s: illegal bitrate specified(bitRate=%d)\n", PROGRAM, bitRate);
	  usage();
	  exit(1);
	}
	arg_counter++;
	break;
                
      case 'n': 
	sscanf(argv[arg_counter+1], "%d", &SnapFrame);
	arg_counter++;
	break;
                
      default:  
	fprintf(stderr, "%s: unknown option: %s\n", PROGRAM, argv[arg_counter]);
	usage();
	exit(1);
      }
    }
    
    if (arg_counter == (argc-2)) {
      if ((AFpA = AFopenRead(argv[arg_counter], &NsampA, &NchanA, &SfreqA, stderr)) ==  NULL) {
	fprintf(stderr, "%s: can't open file : %s\n", PROGRAM, argv[arg_counter]);
	exit(1);
      }
      if (NchanA != 1) {
	fprintf(stderr, "%s: only mono channel supported : %s\n", PROGRAM, argv[arg_counter]);
	exit(1);
      }
      if (SfreqA != 8000) {
	fprintf(stderr, "%s: only sampling frequency of 8000[Hz] supported : %s\n", PROGRAM, argv[arg_counter]);
	exit(1);
      }
    }
    
    if (arg_counter == (argc-1)) {
      if ((AFpB = AFopenRead(argv[arg_counter], &NsampB, &NchanB, &SfreqB, stderr)) ==  NULL) {
	fprintf(stderr, "can't open file : %s\n", argv[arg_counter]);
	exit(1);
      }
      if (NchanB != 1) {
	fprintf(stderr, "only mono channel supported : %s\n", argv[arg_counter]);
	exit(1);
      }
      if (SfreqB != 8000) {
	fprintf(stderr, "%s: only sampling frequency of 8000[Hz] supported : %s\n", PROGRAM, argv[arg_counter]);
	exit(1);
      }
    }
    
    arg_counter++;
  }

  if (NsampA != NsampA) {
    fprintf(stderr, "%s: file length differs\n", PROGRAM);
    exit(1);
  }

  dim = (int)(log10(FFTLength)/log10(2.0));
  
  /* allocate buffers */
  if ((Xa = (float *)malloc(sizeof(float)*2*FFTLength))==NULL) {
    fprintf(stderr, "%s: memory alloc error\n", PROGRAM);
  }
  Ya = (float *)malloc(sizeof(float)*2*FFTLength);
  Ra = (float *)malloc(sizeof(float)*(FFTLength/2+1));
  Xb = (float *)malloc(sizeof(float)*2*FFTLength);
  Yb = (float *)malloc(sizeof(float)*2*FFTLength);
  Rb = (float *)malloc(sizeof(float)*(FFTLength/2+1));
  dif = (float *)malloc(sizeof(float)*(FFTLength/2+1));

  maxR = (float *)malloc(sizeof(float)*(FFTLength/2+1));
  minR = (float *)malloc(sizeof(float)*(FFTLength/2+1));
  max = (float *)malloc(sizeof(float)*(FFTLength/2+1));
  min = (float *)malloc(sizeof(float)*(FFTLength/2+1));

  window = (float *)malloc(sizeof(float)*WindowLength);

  make_window(window, WindowLength, WindowType);
  
  offs = Offset;
  
  frm = 0;

  if (bitRate == 2000) {
    
    /* upper bound of acceptable range(2kbps) */
    for (i = 0; i < 70; i++) {
      maxR[i] = -30.0;
    }
    for (; i < 80; i++) {
      maxR[i] = 2.6*(double)(i-70)-30.0;
    }
    for (; i < 122; i++) {
      maxR[i] = -4.0;
    }
    for (; i < 128; i++) {
      maxR[i] = -1.4*(double)(i-122)-4.0;
    }

    /* lower bound of acceptable range(2kbps) */
    for (i = 0; i < 90; i++) {
      minR[i] = -200.0;
    }
    for (; i < 100; i++) {
      minR[i] = 45.0/10.0*(double)(i-100)-35.0;
    }
    for (; i < 118; i++) {
      minR[i] = -35.0;
    }
    for (; i < 128; i++) {
      minR[i] = -38.0/9.0*(double)(i-118)-35.0;
    }
  }
  else if (bitRate == 4000) {
    
    /* upper bound of acceptable range(4kbps) */
    for (i = 0; i < 85; i++) {
      maxR[i] = -30.0;
    }
    for (; i < 95; i++) {
      maxR[i] = 2.6*(double)(i-85)-30.0;
    }
    for (; i < 122; i++) {
      maxR[i] = -4.0;
    }
    for (; i < 128; i++) {
      maxR[i] = -1.4*(double)(i-122)-4.0;
    }

    /* lower bound of acceptable range(4kbps) */
    for (i = 0; i < 100; i++) {
      minR[i] = -200.0;
    }
    for (; i < 110; i++) {
      minR[i] = 45.0/10.0*(double)(i-110)-35.0;
    }
    for (; i < 118; i++) {
      minR[i] = -35.0;
    }
    for (; i < 128; i++) {
      minR[i] = -38.0/9.0*(double)(i-118)-35.0;
    }
  }
  else {
    fprintf(stderr, "%s: illegal bitrate specified(bitRate=%d)\n", PROGRAM, bitRate);
    exit(1);
  }
 
  for (i = 0; i <= FFTLength/2; i++) {
    max[i] = 0.0f;
    min[i] = 100000000.0f;
  }

  while(1) {
    fprintf(stderr, "frame #%d\r", frm);
    fflush(stderr);

    Na = AFreadData(AFpA, offs, Xa, WindowLength);
    Nb = AFreadData(AFpB, offs, Xb, WindowLength);

    offs += FrameInterval;
    if ( ( Na <= 0 && Nb <= 0 ) || Na < WindowLength || Nb < WindowLength)
      break;

    /* normalization (from -1.0 to 1.0) */
    for (i = 0; i < WindowLength; i++) {
      Xa[i] /= 32768.0f;	
      Xb[i] /= 32768.0f;	
    }

    /* windowing */
    IPC_window(window, Xa, Xa, WindowLength);
    IPC_window(window, Xb, Xb, WindowLength);

    for (i = 0; i < WindowLength; i++) {
      Ya[i] = 0.0f;
      Yb[i] = 0.0f;
    }
    for (; i < FFTLength; i++) {
      Xa[i] = 0.0f;
      Ya[i] = 0.0f;
      Xb[i] = 0.0f;
      Yb[i] = 0.0f;
    }
  
    /* fft */
    IPC_fft(Xa, Ya, dim);
    IPC_fft(Xb, Yb, dim);

    /* obtainning differential spectrum */
    for (i = 0; i <= FFTLength/2; i++) {
      Ra[i] = sqrt(Xa[i]*Xa[i]+Ya[i]*Ya[i]);
      Rb[i] = sqrt(Xb[i]*Xb[i]+Yb[i]*Yb[i]);
      Ra[i] = fabs(Ra[i]-Rb[i]);	
    }

    /* average filter */
    average_filter(Ra, dif, FFTLength/2+1, tapNum);
    

    /* test whether all of amplitudes of the differential spectrum are
       within a certain range */
    for (i = 0; i < FFTLength/2; i++) {
      if (dif[i] > max[i]) {
	max[i] = dif[i];
      }
      if (dif[i] < min[i]) {
	min[i] = dif[i];
      }
      if (20.0*log10(dif[i]) > maxR[i] || 20.0*log10(dif[i]) < minR[i])	{
	fprintf(stdout, "%s: conformance condition is NOT satisfied(frame #%3d)\n", PROGRAM, frm);
	check_flag = 1;
      }
    }
    
    if (frm == SnapFrame) {

      fp = fopen("snap.xgh", "w");

      fprintf(fp, "TitleText: frm# %d(bitRate=%d[bps])\n", frm, bitRate);
      
      fprintf(fp, "\n\"rms\n");
      for (i = 0; i < FFTLength/2; i++) {
	fprintf(fp, "%i %f\n", i, 20.0f*log10(Ra[i]));
      }
      fprintf(fp, "\n\"averaged(tap=%d)\n", tapNum);
      for (i = 0; i < FFTLength/2; i++) {
	fprintf(fp, "%i %f\n", i, 20.0f*log10(dif[i]));
      }
      fprintf(fp, "\n\"maxR\n");
      for (i = 0; i < FFTLength/2; i++) {
	fprintf(fp, "%d %f\n", i, maxR[i]);
      }
      fprintf(fp, "\n\"minR\n");
      for (i = 0; i < FFTLength/2; i++) {
	fprintf(fp, "%d %f\n", i, minR[i]);
      }
      
      fclose(fp);
    }

    frm++;
  }

  fp = fopen("result.xgh", "w");

  fprintf(fp, "TitleText: bitRate=%d[bps]\n", bitRate);
    
  fprintf(fp, "\n\"max\n");
  for (i = 0; i < FFTLength/2; i++) {
    fprintf(fp, "%d %f\n", i, 20.0f*log10(max[i]));
  }
  fprintf(fp, "\n\"min\n");
  for (i = 0; i < FFTLength/2; i++) {
    fprintf(fp, "%d %f\n", i, 20.0f*log10(min[i]));
  }
  fprintf(fp, "\n\"maxR\n");
  for (i = 0; i < FFTLength/2; i++) {
    fprintf(fp, "%d %f\n", i, maxR[i]);
  }
  fprintf(fp, "\n\"minR\n");
  for (i = 0; i < FFTLength/2; i++) {
    fprintf(fp, "%d %f\n", i, minR[i]);
  }

  fclose(fp);

  /* the result of conformance testing */
  if (check_flag) {
    fprintf(stdout, "%s: conformance condition is NOT satisfied.\n", PROGRAM);
  }
  else {
    fprintf(stdout, "%s: conformance condition is satisfied.\n", PROGRAM);
  }
  
  /* delete buffers */
  free(Xa);
  free(Ya);
  free(Ra);
  free(Xb);
  free(Yb);
  free(Rb);
  free(dif);

  free(max);
  free(min);
  free(maxR);
  free(minR);

  free(window);

  return 0;
}





