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
 *   Burst error generation program (Gilbert model version)
 *   Copyright (C) 1996 NTT Mobile Communications Network, Inc. (DoCoMo)
 *   All right reserved.
 *                      Toshio  Miki     (miki@mlab.nttdocomo.co.jp)
 *                      Toshiro Kawahara (kawahara@mlab.nttdocomo.co.jp)
 *  Name:  gilbert
 *  
 *  Usage: gilbert <iseed> <bitrate(bps)> <average ber> <burst len(ms)> <bad_ber> <ifile> <ofile>
 * 
 *  Example usage:
 *      - Generate 10000 byte length error pattern sequence
 *         BER in burst duration: 50%
 *         Average bit error rate: 1.0e-3
 *         Bitrate = 24k bits/sec
 *         Burst length = 10 ms
 *             % gilbert 10 24000 1.0e-3 10 0.5 input.seq output.seq
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define EFREESEC 0

#define TOLERANCE_BER  0.05
#define TOLERANCE_BURSTLEN 0.25
#define NTRYMAX 5000

#define GOOD_BER 0.0
/* #define BAD_BER  0.1*/

int putbit();
float frand();
void snap_frand();
void retrieve_frand();
void check_errorcond();


off_t check_filesize();

int len;
static int outbyte = 0;
static int nouterror = 0;

int main(argc, argv)
     int argc;
     char **argv;
{
  long iseed, iseed_bak;
  double bitrate;
  int state;
  int flen, elen;
  int sum_flen, sum_elen, n_e, n_f;
  int nerror;
  double freelen, burstlen, burlenms;
  double badber;
  double aveber;
  int i;

  double p0, p1;
  FILE *fpin, *fpout;
  off_t datasize;

  if(argc != 8){
    fprintf(stderr, "Usage: %s <iseed> <bitrate(bps)> <average ber> <burst len(ms)> <bad_ber> <ifile> <ofile>\n", argv[0]);
    exit(1);
  }

  sscanf(argv[1], "%ld", &iseed);
  sscanf(argv[2], "%lf", &bitrate);
  sscanf(argv[3], "%lf", &aveber);
  sscanf(argv[4], "%lf", &burlenms);
  sscanf(argv[5], "%lf", &badber);

  if(argv[6][0] == '-'){
    fprintf(stderr, "Sorry. This operation is not supported with this version\n");
    exit(1);
  }
  else if(argv[6][0] == '+'){
    fpin = NULL;
    sscanf(&argv[6][1], "%d", &len);
    datasize = len * 8;
  }else{
    if(NULL == (fpin = fopen(argv[6],"rb"))){
      perror(argv[6]);
      exit(-1);
    }
    datasize = check_filesize(argv[6]) * 8;
    fprintf(stderr, "datasize = %ld bit\n", datasize);

  }

  burstlen = burlenms * bitrate / 1000.0;
  p1 = 1.0 / burstlen;
  p0 = aveber * p1 / (badber - aveber);
  freelen = 1.0 / p0;
/*  printf("p0=%e, p1=%e\n", p0,p1);*/
  

#if 0  
  fprintf(stderr, "len = %d\n", len);
  fprintf(stderr, "average BER = %e\n", aveber);
  fprintf(stderr, "freelen = %f\n", freelen);
#endif

  for(i=0; i<bitrate*EFREESEC; i++)
    if(putbit(fpin, fpout, 0)){
      fprintf(stderr, "Finished while beginning error free period!!\n");
      exit(1);
    }
  iseed_bak = iseed;
  for(i=0; i<NTRYMAX; i++) {
    double cal_burlenms, cal_aveber;
    iseed = iseed_bak + i;
    snap_frand();
    check_errorcond(datasize, p0, p1, badber, &iseed, &nerror, &n_e, &sum_elen) ;
    cal_aveber = (double)nerror/datasize;
    cal_burlenms = (double)sum_elen/n_e/bitrate*1000.0;
/*    printf("%d %d %d\n", nerror, sum_elen, n_e);*/
    fprintf(stderr, "%4d: iseed:\t\t     %13ld\n", i, iseed_bak + i );
    fprintf(stderr, "      average BER:\t\t %9.2e (%6.2f %%)\n", cal_aveber, 100 - cal_aveber/aveber*100);
    fprintf(stderr, "      average burst length:\t%7.3f ms (%6.2f %%)\n", cal_burlenms, 100 - cal_burlenms/burlenms*100);
    if(
       (burlenms *(1.0-TOLERANCE_BURSTLEN) < cal_burlenms) &&
       (burlenms *(1.0+TOLERANCE_BURSTLEN) > cal_burlenms) &&
       (aveber * (1.0 - TOLERANCE_BER) < cal_aveber) &&
       (aveber * (1.0 + TOLERANCE_BER) > cal_aveber)
       ){
      fprintf(stderr, "I GOT IT!!!!\n");
      iseed = iseed_bak + i;
      snap_frand();
      /* Open Output File */
      if(argv[7][0] == '-')
	fpout = stdout;
      else{
	if(NULL == (fpout = fopen(argv[7],"wb"))){
	  perror(argv[7]);
	  exit(-1);
	}
      }
      /* Output Bits */
      flen = elen = sum_elen = sum_flen = n_e = n_f = 0;
      state = 0;
      for(;;){
	if(state == 0){		/* GOOD */
	  if(putbit(fpin, fpout, (frand(&iseed) < GOOD_BER)))
	    break;
	  if(frand(&iseed) < p0){	/* TRANSITION */
	    state = 1;
	    /*	printf("flen = %d\n", flen);*/
	    sum_flen += flen;
	    n_f ++;
	    flen = 0;
	  }else{
	    flen ++;
	  }
	}else{			/* BAD */
	  if(putbit(fpin, fpout, (frand(&iseed) < badber)))
	    break;
	  if(frand(&iseed) < p1){	/* TRANSITION */
	    state = 0;
	    /*	printf("elen = %d\n", elen);*/
	    sum_elen += elen;
	    n_e ++;
	    elen = 0;
	  }else{
	    elen ++;
	  }
	}
      }
      fprintf(stderr, "Statistics of generated error pattern:\n");
      fprintf(stderr, "\tfinal iseed:\t\t       %13ld\n", iseed_bak + i);
      fprintf(stderr, "\taverage BER:\t\t\t %11.2e\n", (double)nouterror/(outbyte*8-EFREESEC*bitrate));
      fprintf(stderr, "\taverage burst length:\t\t %8.3f ms\n", (double)sum_elen/n_e/bitrate*1000.0);
      fprintf(stderr, "\taverage error free length:\t %8.3f ms\n", (double)sum_flen/n_f/bitrate*1000.0);
      exit(0);
    }
  }
  fprintf(stderr, "I DIDN'T GET IT (SORRY)!!!!\n");
  return ( 0 );
}

int
putbit(fpin, fpout, bpat)
     FILE *fpin, *fpout;
     int bpat;
{
  static int pos = 0;
  static char outdata = 0;
  static char indata;

  if(fpin != NULL && pos == 0){
    if(fread(&indata, sizeof(char), 1, fpin) != 1) /* EOF */
      return(1);		/* END!! */
  }else if(fpin == NULL && len == 0)
    return(1);			/* END!! */
  
  nouterror += bpat;
  outdata <<= 1;
  outdata |= (bpat& 1);
  pos ++;
  if(pos == 8){			/* Write Out!! */
    if(fpin != NULL) outdata ^= indata;
    fwrite(&outdata, sizeof(char), 1, fpout);
    len --;
    outbyte ++;
    pos = 0;
  }
  return(0);
}

off_t
check_filesize(path)
     char *path;     
{
  struct stat buf;
  stat(path, &buf);
  return buf.st_size;
}

void
check_errorcond(datasize, p0, p1, badber, iseed, nerror, n_e, sum_elen)
     off_t datasize;
     double p0, p1, badber;
     long *iseed;
     int *nerror, *n_e, *sum_elen;
{
  long i;
  int state;
  int elen;

  *nerror = *n_e = *sum_elen = 0;
  state = elen = 0;

/*  printf("p0=%e, p1=%e\n", p0,p1);*/
  for(i=0; i<datasize; i++){
    if(state == 0){		/* GOOD */
      (*nerror) += (frand(iseed) < GOOD_BER);
      if(frand(iseed) < p0){	/* TRANSITION */
	state = 1;
      }
    }else{			/* BAD */
      *nerror += (frand(iseed) < badber);
      if(frand(iseed) < p1){	/* TRANSITION */
	state = 0;
	(*sum_elen) += elen;
	(*n_e) ++;
	elen = 0;
      }else{
	elen ++;
      }
    }
  }
/*  printf("%d %d %d\n", *nerror, *sum_elen, *n_e);*/
}
