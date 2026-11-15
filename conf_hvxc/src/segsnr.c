/**********************************************************************
MPEG-4 Parametric coder
Conformance program(Segmental SNR calculation)



This software module was originally developed by

Akira Inoue (Sony Corporation) 

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

Copyright (c) 1999.

Description: 
This program(segsnr) calculates segmental SNR defined 

                                  SUM xa(i)^2
        SS(k) = log10 (1 + --------------------------) .
                           1.0e-13*L + SUM [xa(i)-xb(i)]^2

        SSNR = 10 * log10 ( 10^[SUM SS(k) / N] - 1 ) dB.


Source file: main.c

$Id: segsnr.c,v 1.3 2003/06/27 16:54:03 sps Exp $

Required libraries:
libtsp.a		TSP library

Authors:
AI    Akira Inoue, Sony HNC <akira@av.crl.sony.co.jp>
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libtsp.h>
#include <libtsp/AFpar.h>
#include <libtsp/FIpar.h>

#define PROGRAM "segsnr"

static void usage(void) 
{
  fprintf(stderr, "usage: %s [options] AFileA AFileB\n", PROGRAM);
  fprintf(stderr, "  AFileA: file name of reference signal\n");
  fprintf(stderr, "  AFileB: file name of test signal\n");

  fprintf(stderr, "options:\n");
  fprintf(stderr, "  -s val\tsegment length(default: 160)\n");
  fprintf(stderr, "  -h\t\tshow this message\n");
  
  return;
}

static void error(char *msg)
{
  fprintf(stderr, "error(%s): %s\n", PROGRAM, msg);
  usage();
  exit(1);
}


int main(int argc, char **argv)
{
  int i;
  float *Xa, *Xb; 
  
  int SegmentLength = 160;
  int arg_counter;

  AFILE *AFpA = NULL;	/* reference signal */
  AFILE *AFpB = NULL;	/* test signal */
  long int NsampA, NchanA, NsampB, NchanB;
  float SfreqA, SfreqB;
  long int Na, Nb, offs;

  int Nseg;
  float Sx2, Sd2, SNRlog, SSNR;

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
                
      case 's': 
	sscanf(argv[arg_counter+1], "%d", &SegmentLength);
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
    }
    
    arg_counter++;
  }

  if (NsampA != NsampA) {
    fprintf(stderr, "%s: file length differs\n", PROGRAM);
    exit(1);
  }

  /* allocate buffers */
  if ((Xa = (float *)malloc(sizeof(float)*SegmentLength))==NULL) {
    fprintf(stderr, "%s: memory alloc error\n", PROGRAM);
  }
  if ((Xb = (float *)malloc(sizeof(float)*SegmentLength))==NULL) {
    fprintf(stderr, "%s: memory alloc error\n", PROGRAM);
  }
  
  Nseg = 0;
  offs = 0;
  SNRlog = 0.0f;
  
  while(1) {
    fprintf(stderr, "segment #%d\r", Nseg);
    fflush(stderr);

    Na = AFreadData(AFpA, offs, Xa, SegmentLength);
    Nb = AFreadData(AFpB, offs, Xb, SegmentLength);

    offs += SegmentLength;
    if ( ( Na <= 0 && Nb <= 0 ) || Na < SegmentLength || Nb < SegmentLength)
      break;

    /* normalization (from -1.0 to 1.0) */
    for (i = 0; i < SegmentLength; i++) {
      Xa[i] /= 32768.0f;	
      Xb[i] /= 32768.0f;	
    }

    /* Segmental SNR update */
    Sx2 = 0.0f;
    Sd2 = 1.0e-13f*(float)SegmentLength;
    for (i = 0; i < SegmentLength; i++) {
      Sx2 += Xa[i]*Xa[i];
      Sd2 += (Xb[i]-Xa[i])*(Xb[i]-Xa[i]);
    }
    SNRlog += log10(1.0+Sx2/Sd2);
    
    Nseg++;
  }

  /* final segmental SNR calculation */
  SSNR = 10.0*log10(pow(10.0, (SNRlog/(float)Nseg))-1.0);

  /*************************************************************

                                  SUM xa(i)^2
        SS(k) = log10 (1 + --------------------------) .
                           1.0e-13*L + SUM [xa(i)-xb(i)]^2

        SSNR = 10 * log10 ( 10^[SUM SS(k) / N] - 1 ) dB.

  *************************************************************/

  /* the result of conformance testing */
  fprintf(stdout, "%s: Segmental SNR=%4.1f[dB](%d sample segment)\n", PROGRAM, SSNR, SegmentLength);
  if (SSNR > 30.0) {
    fprintf(stdout, "    conformance condition is satisfied.\n");
  }
  else {
    fprintf(stdout, "    conformance condition is NOT satisfied.\n");
  }
  
  /* delete buffers */
  free(Xa);
  free(Xb);

  return 0;
}





