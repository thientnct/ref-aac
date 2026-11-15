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
 *   Random packet loss generation program
 *   Copyright (C) 1996 NTT Mobile Communications Network, Inc. (DoCoMo)
 *   All right reserved.
 *                      Toshio  Miki     (miki@mlab.nttdocomo.co.jp)
 *                      Toshiro Kawahara (kawahara@mlab.nttdocomo.co.jp)
 *  Name:  packetloss
 *  
 *  Usage: packetloss <iseed> <bitrate(b/s)> <PLR> <in_filename> <out_filename> [<packetlen_file>]
 * 
 *  Default Packet Length : 96 bit / 200 bit / 400 bit (same probability)
 *  Example usage:
 *      - Generate packet-lost sequence with iseed 10
 *         Packet Loss rate = 1.0e-3
 *         Bitrate = 32000 bit/sec
 *
 *             % packetloss 10 32000 1.0e-3 input.dat output.seq
 */

#include <stdio.h>
#include <stdlib.h>
#define EFREESEC 1.5
#define NPSIZEMAX 20

int readbits(fpin, packetlen, buffer)
     FILE *fpin;
     int packetlen;
     int *buffer;
{
  static int bitrem = 0;
  static unsigned char tmp;
  int i;

  for(i=0; i<packetlen; i++){
    if(bitrem == 0){
      if(fread(&tmp, sizeof(char), 1, fpin) == 0)
	return i;		/* EOF */
      bitrem = 8;
    }
    *(buffer++) = ((tmp & 0x80) == 0x80);
    tmp <<= 1;
    bitrem --;
  }
  return i;
}
  
int writebits(fpout, packetlen, buffer)
     FILE *fpout;
     int packetlen;
     int *buffer;
{
  static int bitrem = 0;
  static unsigned char tmp;
  int i;

  for(i=0; i<packetlen; i++){
    tmp <<= 1;
    tmp |= *(buffer++);
    bitrem ++;
    if(bitrem == 8){
      fwrite(&tmp, sizeof(char), 1, fpout);
      bitrem = 0;
    }
  }
  return ( 0 );
}

int main(argc, argv)
     int argc;
     char **argv;
{
  float frand();
  long iseed;
  double bitrate;
  FILE *fpin, *fpout, *fp_pfile;

  char psizetmp[1024];
  int n_psize;
  int psize_max;
  double pratio_max;
  int psize[NPSIZEMAX];
  double prange[NPSIZEMAX];
  int pcount[NPSIZEMAX];
  int packetlen;
  
  int i;
  int npacket;
  int *buffer;
  int nreadbit;
  double plr;

  if((argc != 6)&&(argc != 7)){
    fprintf(stderr, "Usage: %s <iseed> <bitrate(b/s)> <PLR> <in_filename> <out_filename>\n", argv[0]);
    exit(1);
  }

  sscanf(argv[1], "%ld", &iseed);
  sscanf(argv[2], "%lf", &bitrate);
  sscanf(argv[3], "%lf", &plr);

  if(argc == 7){ /* packet size is specified in file */
    if((fp_pfile = fopen(argv[6], "r")) == NULL){
      perror(argv[6]);
      exit(1);
    }
    n_psize = 0;
    psize_max = 0;
    while(fgets(psizetmp, sizeof(psizetmp), fp_pfile) != NULL){
      pcount[n_psize] = 0;
      if(n_psize > NPSIZEMAX-1){
	fprintf(stderr, "The number of packetsize entry should be less than %d\n", NPSIZEMAX);
	exit(2);
      }
      if(psizetmp[0] == '#' || psizetmp[0] == '\n')
	continue;
      sscanf(psizetmp, "%d %lf", &psize[n_psize], &prange[n_psize]);
      pratio_max = (prange[n_psize] += (n_psize > 0 ? prange[n_psize-1] : 0));
      fprintf(stderr, "%d\t %f\n", psize[n_psize],prange[n_psize]);
      if(psize[n_psize] > psize_max)
	psize_max = psize[n_psize];
      n_psize ++;
    }
  }else{ /* use default packet size */
    n_psize = 3;
    psize[0] = 96;  prange[0] = 1.0;
    psize[1] = 200; prange[1] = 2.0;
    psize[2] = 400; prange[2] = pratio_max = 3.0;
    psize_max = 400;
    pcount[0] = pcount[1] = pcount[2] = 0;
  }

  fprintf(stderr, "n_psize = %d, pratio_max = %f, psize_max = %d\n", n_psize, pratio_max, psize_max);

  if((buffer = (int *)malloc(sizeof(int) * psize_max)) == NULL){
    fprintf(stderr, "Memory allocation for %d length packet failed\n", psize_max);
    exit(1);
  }
  

  if(argv[4][0] == '-')
    fpin = stdin;
  else
    if((fpin = fopen(argv[4], "rb")) == NULL){
      perror(argv[4]);
      exit(1);
    }
	
  if(argv[5][0] == '-')
    fpout = stdout;
  else
    if((fpout = fopen(argv[5], "wb")) == NULL){
      perror(argv[5]);
      exit(1);
    }
  
  npacket = 0;
  
  for(i=0; i<bitrate*EFREESEC; i++)
    if(readbits(fpin, 1, buffer) != 1){
      fprintf(stderr, "Finished while beginning error free period!!\n");
      exit(1);
    }else
      writebits(fpout, 1, buffer);
  
  for(;;){
    double toss;
    toss = (double)frand(&iseed) * pratio_max;
    for(i=0; i<n_psize; i++){
      if(toss < prange[i]){
	packetlen = psize[i];
	pcount[i] ++;
	break;
      }
    }
/*     fprintf(stderr, "toss= %f i= %d prange[%d]= %f plen=%d\n", toss, i, i, prange[i], packetlen); */
    
    if((nreadbit = readbits(fpin, packetlen, buffer)) != packetlen)
      break;

    if(frand(&iseed) < plr){	/* Packet Loss !!*/
      ;				/* No writeout */
/*      fprintf(stderr, "%d bit packet lost!!\n", packetlen);*/
    }else{
      writebits(fpout, packetlen, buffer);
    }
  }
  writebits(fpout, nreadbit, buffer);

  for(i=0; i<n_psize; i++){
    fprintf(stderr, "%4d bit packet:\t %d\n", psize[i], pcount[i]);
  }
  return ( 0 );
}

