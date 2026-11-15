/*****************************************************************************
 *                                                                           *
"This software module was originally developed by NTT Mobile
Communications Network, Inc. in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and
3. This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4
Audio standard. ISO/IEC gives users of the MPEG-2 NBC/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 NBC/MPEG-4 Audio standards. Those intending to use this
software module in hardware or software products are advised that this
use may infringe existing patents. The original developer of this
software module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-2 NBC/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 NBC/MPEG-4 Audio
conforming products. This copyright notice must be included in all
copies or derivative works." Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
/* 
 *   Genearate random error sequence
 *   Copyright (C) 1994, 1996 NTT Mobile Communications Network, Inc. (DoCoMo)
 *   All right reserved.
 *                      Toshio  Miki     (miki@mlab.nttdocomo.co.jp)
 *                      Toshiro Kawahara (kawahara@mlab.nttdocomo.co.jp)
 *  Name:  erand
 *  
 *  Usage: erand <iseed> <bitrate(b/s)> <ber> <input_file_name> <output_file_name>
 * 
 *  Example usage:
 *      - Generate random bit error
 *         Bit error rate: 1.0e-4
 *         Bitrate = 24k bits/sec
 *             % erand 10 24000 1.0e-4 input.seq error.seq
 *  Modify (Date: Jul. 25 1995) for  1.5 sec error free duration 
 */
#include <stdio.h>
#include <stdlib.h>

#define EFREESEC 0.0

float frand();

unsigned char display = 0;

int main(argc, argv)
     int argc;
     char **argv;
{
  FILE*         fpin;
  FILE*         fpout;
  long          iseed;
  long          seed;
  float         ber;
  int           bitrate;
  int           len;
  int           nbits;
  unsigned char seq;
  int i,j;
  
  if(argc != 6){
    fprintf(stderr, "Usage: %s <iseed> <Bitrate> <BER> <Inputfile> <Outputfile>\n",argv[0]);
    fprintf(stderr, "\tFilename '-' indicates stdin/stdout.\n");
    fprintf(stderr, "\tWith input filename '+number',this program generate <number> bytes\n\terror sequence.\n");
    exit(-1);
  }
  
  sscanf(argv[1], "%ld", &iseed);
  sscanf(argv[2], "%d", &bitrate);
  sscanf(argv[3], "%f", &ber);
  
  if(argv[4][0] == '-')
    fpin = stdin;
  else if(argv[4][0] == '+'){
    fpin = NULL;
    sscanf(&argv[4][1], "%d", &len);
  }else{
    if(NULL == (fpin = fopen(argv[4],"rb"))){
      perror(argv[4]);
      exit(-1);
    }
  }

  if(argv[5][0] == '-')
    fpout = stdout;
  else{
    if(NULL == (fpout = fopen(argv[5],"wb"))){
      perror(argv[5]);
      exit(-1);
    }
  }

  nbits = bitrate * EFREESEC;	/* Number of error free bits */
  seed = iseed;

  if(fpin != NULL){		/* Additional Error */
    while(fread(&seq, sizeof(seq), 1, fpin) == 1){
      for(i=0; i<8; i++){
	nbits --;
	if(nbits < 0)
	  seq ^= (frand(&seed) < ber ? (0x80>>i) : 0);
      }
      fwrite(&seq, sizeof(seq), 1, fpout);
    }
  }else{
    for(i=0; i<len; i++){	/* Generate Error Pattern */
      seq = 0;
      for(j=0; j<8; j++){
	nbits --;
	if(nbits < 0)
	  seq |= (frand(&seed) < ber ? (0x80>>j) : 0);
      }
      fwrite(&seq, sizeof(seq), 1, fpout);
    }
  }
  if ( display )
    fprintf(stderr, "Iseed = %ld\n", iseed);
#if 0
  {
    char           outputFileName[200];
    char           iseedchar[30];
    unsigned char  lengthOfOutputFileName;
    unsigned short j;
    for ( j = 0; j < 200; j++ )
      outputFileName[j] = 0;
    for ( j = 0; j < 30; j++ )
      iseedchar[j] = 0;
    lengthOfOutputFileName = strlen( argv[5] );
    strncpy ( outputFileName, argv[5], lengthOfOutputFileName - 8 );
    sprintf ( iseedchar,"iseed%06li_main.bit",iseed);
/*     fprintf(stderr, "1 outputFileName: %s\n",outputFileName); */
/*     fprintf(stderr, "1 iseedchar: %s\n",iseedchar); */
    strcat( outputFileName, iseedchar );
/*     fprintf(stderr, "2 outputFileName: %s\n",outputFileName); */
    rename ( argv[5], outputFileName );
    fclose (fpin);
    fclose (fpout);
  }
#endif
  return ( 0 );
}
  
      
