/*
============================================================

               (C) copyright Fraunhofer - IIS (2001)
                        All Rights Reserved

   filename: PnsConformanceMain.c
             
   author  : Martin Weishart martin.weishart@iis.fhg.de
   date    : 2000-10-26
   contents/description: provides functions to calculate
                         PNS conformance criteria

============================================================
*/

#include "CPnsConformance.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc , char * argv[]){

  char * Pcmfile, * refPcmfile, **s=&argv[3];
  
  unsigned int frameSize = FRAME_SIZE_SHORT;

  int delay=0, to_test=3, result;
    
  if ( argc < 4 ){
    /*
    fprintf(stderr,"\nSupply name of testbitstream, conformancebitstream and  1 or 2 for PNS conformance test P1 or P2\n\n ");
    */

    fprintf(stderr,"usage: pnsConformance <reference waveform> <waveform under test>  <test mode> [-d <delay>] \n");
    fprintf(stderr,"  available test modes:\n");
    fprintf(stderr,"     1: spectral PNS conformance test for long blocks\n");    
    fprintf(stderr,"     2: spectral PNS conformance test for short blocks\n");    
    fprintf(stderr,"     3: temporal PNS conformance test\n"); 
    fprintf(stderr,"     4: spectral PNS conformance test for low delay\n"); 
    fprintf(stderr,"  -d <x>: delay of <waveform under test> in samples\n");    

    exit(1);
  }

  while (*s) {

    if (!strncmp(*s,"-d",2)) {
      if (!*(++s)) {
        fprintf(stderr, "-d needs an argument!\n");
        exit(1);
      }
      delay=atoi(*s);
    } else
    if (!strncmp(*s,"1",1)) {
      frameSize = FRAME_SIZE_LONG;
      to_test=1;
    } else
    if (!strncmp(*s,"2",1)||!strncmp(*s,"3",1)) {
      to_test=atoi(*s);
      frameSize = FRAME_SIZE_SHORT;
    } else
    if (!strncmp(*s,"4",1)) {
      frameSize = FRAME_SIZE_LD;
      to_test=1;
    } 
    s++;
  }

  Pcmfile = argv[1];
  refPcmfile = argv[2]; 
  
  fprintf(stderr,"\n\n*************************************************\n");
  fprintf(stderr,"*                                               *\n");
  fprintf(stderr,"*             PNS conformance check             *\n");
  fprintf(stderr,"*                                               *\n");
  fprintf(stderr,"******* copyright 2002 by Fraunhofer IIS ********\n\n");

  printf("%s  %s and %s:\n", GREP_STRING, Pcmfile, refPcmfile);
  if ( to_test == 1 || to_test == 2){
    result = CalculateSpectralConformance(Pcmfile, refPcmfile,frameSize,delay);
    printf("%s  spectral PNS conformance with analysis frame size %d  : %s\n", GREP_STRING, frameSize, result ? "FAIL" : "OK");
  }
  else {
    result = CalculateTemporalConformance(Pcmfile, refPcmfile,frameSize,delay);
    printf("%s  temporal PNS conformance with analysis frame size %d  : %s\n", GREP_STRING, frameSize, result ? "FAIL" : "OK");
  }
  
  return 0;
}

