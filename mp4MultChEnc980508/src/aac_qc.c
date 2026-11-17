/* $Id: aac_qc.c,v 1.3 1998-05-07 21:07:10+03 jojanper Exp $
 */
/***********

This software module was originally developed by Dolby
Laboratories and edited by Sony Corporation 
in the course of development of the MPEG-2 NBC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3. This software module is an 
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools as 
specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2NBC/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 NBC/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 NBC/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. Copyright 1996.  

***********/
/******************************************************************** 
/
/ filename : nbc_qc.c
/ project : MPEG-2 NBC
/ author : SNL, Dolby Laboratories, Inc
/          CL, Chuck Lueck, TI <lueck@ti.com>
/ date : November 15, 1996
/ contents/description : slow rate/distortion quantizer,  
/    huffman coding, bit packing
/
/ Changes:
/ 07-jun-97   CL   Removed final write_to_bitstream call in function
/                  tf_encode_spectrum.  SCE is now written outside
/                  of this function.  Added use of AACQuantInfo
/                  structure.
/ 14-sep-97   CL   Modified scalefactor band energy computation to
/                  work with prediction.  Added store_common_scalefac
/                  variable to correct for occasion global gain error.
/ 24-oct-97   CL   Fixed bug in sort_book_numbers; section bit count
/                  was incorrect.  Modified code to account for tns
/                  bits.  Modified short window energy computation to
/                  compute energy prior to tns.  Corrected SMR 
/                  computation for short windows to use maximum SMR
/                  of each window in window group.
/ 16-dec-97   CL   Added conditional to prevent log(0) when computing
/                  initial common_scalefac value.  Corrected scalefactor
/                  bug which caused common_scalefac to be transmitted 
/                  a step too large and caused the decoded output to
/                  be louder than the original.
/ 22-Jan-98   CL   Added multichannel support.  Added initial joint coding
/                  support.
/
*******************************************************************/
 
#include "common.h" 
#include "bitstream.h"
#include "tf_main.h"
#include "aac_qc.h"
#include "aac_se_enc.h"
#include <stdio.h>
#include <math.h>

static int table_choice = 5;      /* CHOOSE HERE BETWEEN TABLES 4 or 5 */
static  int huff[13][1090][4];
static int qdebug;
static double pow_quant[9000];
/* 
   quantizes each scalefactor band separately, in a very SLOW algorithm.  
   But, it does work, by incrementing (by one) the scalefactor in the corresponding
   scalefactor band with the largest ratio of error energy to allowed distortion.
   There are both rate and distortion loops currently implemented.
   Again, this algorithm has NOT been modified for speed.  With more time, the
   rate/distortion loops can be sped up.  

   --SNL 11/15/96   
   */

/*
  tf_encode_spectrum now correctly outputs the amount of bits written to the bitstream,
  along with accurately keeping a bit count in the quantizer loop.

  --SNL 11/8/96
*/

/* 
   This code sucessfully implements grouping.  To test different versions of
   grouping, change the following variables near the top of tf_encode_spectrum_nbc():
   window_group_length[8] and num_window_groups.  An example of this grouping 
   could be: (currently used)

      int window_group_length[8] = {3,1,2,1,1};
      num_window_groups = 5;

   This translates to there being five groups of short windows.  The first group 
   contains three short windows, the second window contains one, etc. 

November 1, 1996
*/


/*

   This quantizer / noiseless coder is a work in progress.  Currently,
   it supports only long or only short block modes.  The quantizer is
   a uniform quantizer that implements only the rate quantization
   loop, and not yet the distortion quantization loop.  Grouping is
   not yet supported, so during short blocks, the default
   implementation is each of the eight short blocks are individual
   groups.  The noiseless coding section is fully operational.  The
   output of this code is a NBC, CD compliant bitstream.

   This code also allows you to switch between two different huffman
   codebooks,  which I'll call RM4 and CD codebooks, received both
   from AT&T on different dates.  If table_choice == 4, then you
   choose to use the all signed, RM4 codebooks.  If table_choice == 5,
   then the CD codebooks will be loaded instead.  Both of these
   codebooks have been tested sucessfully with their respective
   decoders.

   
     September 24, 1996

*/

/*
   RM4 has tables with the following characteristics:  (they're all signed tables) --->
   book        signed/unsigned   range   values_in_codebook  dimension
    1            signed           +/- 1          81             4
    2            signed           +/- 1          81             4
    3            signed           +/- 2          625            4
    4            signed           +/- 2          625            4
    5            signed           +/- 4          81             2
    6            signed           +/- 4          81             2
    7            signed           +/- 7          225            2
    8            signed           +/- 7          225            2
    9            signed           +/- 12         625            2
    10           signed           +/- 12         625            2
    11           signed           +/- 16         1089           2


   RM5 has tables with the following characteristics:  (they're signed and unsigned) --->
   book        signed/unsigned   range   values_in_codebook  dimension
    1            signed           +/- 1          81             4
    2            signed           +/- 1          81             4
    3          unsigned           0,1,2          81             4
    4          unsigned           0,1,2          81             4
    5            signed           +/- 4          81             2
    6            signed           +/- 4          81             2
    7          unsigned           0...7          64             2
    8          unsigned           0...7          64             2
    9          unsigned           0...12         169            2
    10         unsigned           0...12         169            2
    11         unsigned           0...16         289            2
*/
#ifdef MPEG4V1
int  pns_sfb_start = 1000;         /* lower border for Perceptual Noise Substitution 
                                      (off by default) */
#endif
void tf_init_encode_spectrum_aac( int debugLevel )
{
  FILE *fpout;
  short  tmpc[4];
  unsigned int   j;
  int *huff_ptr= &(huff[0][0][0]);
  qdebug = debugLevel;
  /*
    This function reads either one of the two set of huffman codebook tables into huff[][][].
    */

  if (table_choice == 4) {
    /* read the data in from file (used scan_huff_att2.c to read in rm4huf.txt from ascii)*/
    if ((fpout = fopen("./tables_enc/rm4hufbook","r")) == NULL){
      fprintf(stderr,"\ncannot open %s\n","rm4hufbook");
      exit(-1);
    }
  }

  if (table_choice == 5) {
    /* read the data in from file (used scan_huff_att3.c to read in rm5huf.txt from ascii)*/
    if ((fpout = fopen("./tables_enc/rm5hufbook","r")) == NULL){
      fprintf(stderr,"\ncannot open %s\n","rm5hufbook");
      exit(-1);
    }
  }

  if ((table_choice != 4) && (table_choice != 5)){
    fprintf(stderr,"\nInvalid value of table_choice=%d, please choose either 4 or 5\n",table_choice);
    exit(-1);
  }
 
  for (j=0;j<sizeof(huff)/sizeof(huff[0][0][0]) ;j++){
    tmpc[3] = getc(fpout);
    tmpc[2] = getc(fpout);
    tmpc[1] = getc(fpout);
    tmpc[0] = getc(fpout);
    *huff_ptr++ = (tmpc[0]<<0)+(tmpc[1]<<8)+(tmpc[2]<<16)+(tmpc[3]<<24);
  }
  /*   THIS IS NOT PROTABLE !!: */
  /*   i = fread(huff,sizeof(int),13*1090*4,fpout); */

  fprintf(stderr,"\nread %d huffman codebook values \n",j); 
  for (j=0;j<9000;j++){
    pow_quant[j]=pow(j, ((double)4.0/(double)3.0));
  }
}

void debugPrint(AACQuantInfo* quantInfo,   /* Quantization information */
		/* int nr_of_sfb,*/        /* Now in quantInfo */
		double *ratio,
		double *energy,
		double *allowed_dist,
		double *error_energy)
                /* int *scale_factor, */   /* Now in quantInfo */
                /* int common_scalefac) */ /* Now in quantInfo */
{
  int sb;
    fprintf(stderr,"\nratios    :");
    for (sb=0; sb<quantInfo->nr_of_sfb; sb++){
      fprintf(stderr,"%6.1f|",10*log10(ratio[sb]+1e-15));      
    }    
    fprintf(stderr,"\nenrgy     :");
    for (sb=0; sb<quantInfo->nr_of_sfb; sb++){
      fprintf(stderr,"%6.1f|",10*log10(energy[sb]+1e-15));      
    }    
    fprintf(stderr,"\nmask enrgy:");
    for (sb=0; sb<quantInfo->nr_of_sfb; sb++){
      fprintf(stderr,"%6.1f|",10*log10(allowed_dist[sb]+1e-15));      
    }    
    fprintf(stderr,"\nerro enrgy:");
    for (sb=0; sb<quantInfo->nr_of_sfb; sb++){
      fprintf(stderr,"%6.1f|",10*log10(error_energy[sb]+1e-15));      
    }    
    fprintf(stderr,"\nscalefacto:");
    for (sb=0; sb<quantInfo->nr_of_sfb; sb++){
      fprintf(stderr,"%6i|",(quantInfo -> common_scalefac + SF_OFFSET - (quantInfo -> scale_factor[sb]) ));      
    }    

}

int tf_encode_spectrum_aac(
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      *PsySigMaskRatio[MAX_TIME_CHANNELS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         available_bitreservoir_bits,
			   int         padding_limit,
			   BsBitStream *fixed_stream,
			   BsBitStream *var_stream,
			   int         nr_of_chan,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int         useShortWindows,
			   /* Window_shape     window_shape,*/      /* offers the possibility to select different window functions */
			   int aacAllowScalefacs,
			   AACQuantInfo* quantInfo,      /* AAC quantization information */ 
			   Ch_Info* ch_info)
{
  static int quant[CHANNEL][NUM_COEFF];
  int i=0;
  int j=0;
  int k,sfb;
  int ch=0;
  double max_dct_line = 0;
  int global_gain;
  /*  int common_scalefac; */           /* Now in AACQuantInfo */
  /*  int scale_factor[SFB_NUM_MAX]; */ /* Now in AACQuantInfo structure */
  int store_common_scalefac;
  double error_energy[SFB_NUM_MAX];
  double linediff[NUM_COEFF];
  double pow_spectrum[NUM_COEFF];
  int sfb_amplify_check[SFB_NUM_MAX];
  double requant[CHANNEL][NUM_COEFF];
  double ftmp;
  int largest_sb, sb;
  double dtmp;
  double largest_ratio;
  double ratio[SFB_NUM_MAX];
  /* int data[5*NUM_COEFF];  */  /* for each pair of spec values 5 bitstream elemets are requiered: 1*(esc)+2*(sign)+2*(esc value)=5 */  /* Now in AACQuantInfo structure */
  /* int len[5*NUM_COEFF];   */ /* Now in AACQuantInfo structure */
  int max_quant,tmp;
  /* int sfb_offset[250];  */ /* Now in AACQuantInfo structure */
  int sf_bits;
  int extra_bits;
  int book_bits;
  int spectral_bits;
  int max_bits;
  /* int book_vector[SFB_NUM_MAX]; */ /* Now in AACQuantInfo structure */
  int sf_test[SFB_NUM_MAX];
  /* int num_window_groups; */ /* Now in AACQuantInfo structure */
  int bits_written=0;
  int output_book_vector[SFB_NUM_MAX*2];
  int all_amplified;
  int maxRatio;
  /* change these values to test the different GROUPING possibilities */
  /* int window_group_length[8] = {1,0,0,0,0}; */  /* Now in AACQuantInfo structure */
  int start_com_sf = 40 ;
  int rateLoopCnt;
  double SigMaskRatio[SFB_NUM_MAX];
  int  calc_sf[SFB_NUM_MAX];
  /* num_window_groups = 1; */ /* Now in AACQuantInfo structure */
  IS_Info *is_info;
  int *ptr_book_vector;


  /* Set up local pointers to quantInfo elements for convenience */
  int* sfb_offset = quantInfo -> sfb_offset;
  int* scale_factor = quantInfo -> scale_factor;
  int* common_scalefac = &(quantInfo -> common_scalefac);


  /* simple scale factors to make decoding testing easier */
   for (k=0; k<SFB_NUM_MAX; k++){ 
     sf_test[k] = k; 
   } 
 
   /* Set block type in quantization info */
   quantInfo -> block_type = block_type[MONO_CHAN];

  /** create the sfb_offset tables **/
  if (quantInfo -> block_type == ONLY_SHORT_WINDOW) {

    /* Now compute interleaved sf bands and spectrum */
    sort_for_grouping(
                      quantInfo,                       /* ptr to quantization information */
		      /* sfb_offset, */                /* Now in AACQuantInfo structure */
		      sfb_width_table[MONO_CHAN],      /* Widths of single window */
		      p_spectrum,                      /* Spectral values, noninterleaved */
		      /* num_window_groups, */         /* Now in AACQuantInfo structure */
		      /* window_group_length, */       /* Now in AACQuantInfo structure */
		      /* &nr_of_sfb[MONO_CHAN], */     /* Now in AACQuantInfo structure */
		      SigMaskRatio,
		      PsySigMaskRatio[MONO_CHAN]
		      );

    extra_bits = 51;
  } 
  else{

    /* For long windows, band are not actually interleaved */
    if ((quantInfo -> block_type == ONLY_LONG_WINDOW) ||  
	(quantInfo -> block_type == LONG_SHORT_WINDOW) || 
	(quantInfo -> block_type == SHORT_LONG_WINDOW)) {

      quantInfo->nr_of_sfb = quantInfo->max_sfb;

      sfb_offset[0] = 0;
      k=0;
      for( i=0; i< quantInfo -> nr_of_sfb; i++ ){
	sfb_offset[i] = k;
	k +=sfb_width_table[MONO_CHAN][i];
	SigMaskRatio[i]=PsySigMaskRatio[MONO_CHAN][i];
      }
      sfb_offset[i] = k;
      extra_bits = 100; /* header bits and more ... */

    } 
    else {
      CommonExit(-1,"\nERROR : unsupported window type, window type = %d\n",block_type[0]);
    } 
  }

	extra_bits+= quantInfo->gcBitBuf->numBit +1;
  
#if 0
    for( win=0; win<no_sub_win; win++ ) { 
      /* first limit the audio bandwidth to the desired value */ 
      for( i=(bw_lines/8)*window_group_length[win]; i<sub_win_size; i++ ) { 
	spectral_line_vector[MONO_CHAN][(win*sub_win_size)+i] = .0; 
      }
    }
#endif

#ifdef MPEG4V1
    if(quantInfo->block_type!=ONLY_SHORT_WINDOW)
      /* Take into account bits for LTP data */
      extra_bits += WriteLTP_PredictorData(quantInfo, fixed_stream, 0); /* Count but don't write */
#endif

    /* Take into account bits for TNS data */
    extra_bits += WriteTNSData(quantInfo,fixed_stream,0);    /* Count but don't write */

    /* for short windows, compute interleaved energy here */
    if (quantInfo->block_type==ONLY_SHORT_WINDOW) {
      int numWindowGroups = quantInfo->num_window_groups;
      int maxBand = quantInfo->max_sfb;
      int windowOffset=0;
      int sfb_index=0;
      int g;
      for (g=0;g<numWindowGroups;g++) {
	int numWindowsThisGroup = quantInfo->window_group_length[g];
	int b;
	for (b=0;b<maxBand;b++) {
	  double sum=0.0;
	  int w;
	  for (w=0;w<numWindowsThisGroup;w++) {
	    int bandNum = (w+windowOffset)*maxBand + b;
	    sum += energy[MONO_CHAN][bandNum];
	  }
	  energy[MONO_CHAN][sfb_index++]=sum;
	}
	windowOffset += numWindowsThisGroup;
      }
    } 
#ifdef MPEG4V1

    /* PNS prepare */
    for(sfb=0; sfb<quantInfo->nr_of_sfb; sfb++ )
      quantInfo->pns_sfb_flag[sfb] = 0;

    if (block_type[MONO_CHAN] != ONLY_SHORT_WINDOW) {     /* long blocks only */
      for(sfb=pns_sfb_start; sfb<quantInfo->nr_of_sfb; sfb++ ) {
        /* Calc. pseudo scalefactor */
        if (energy[ch][sfb] == 0.0) {
          quantInfo->pns_sfb_flag[sfb] = 0;
          continue;
        }
      
        quantInfo->pns_sfb_flag[sfb] = 1;
        quantInfo->pns_sfb_nrg[sfb] = (int) (2.0 * log(energy[ch][sfb]+1e-60) / log(2.0) + 0.5) + PNS_SF_OFFSET;
    
        /* Erase spectral lines */
        for( i=sfb_offset[sfb]; i<sfb_offset[sfb+1]; i++ ) {
          p_spectrum[0][i] = 0.0;
        }
      }
    }
#endif
    /* Compute allowed distortion */
    for( sfb=0; sfb< quantInfo -> nr_of_sfb; sfb++ ) {
      if ((10*log10(energy[MONO_CHAN][sfb]+1e-15))>70 ) {    /* ? */
	allowed_dist[MONO_CHAN][sfb] = energy[MONO_CHAN][sfb] * SigMaskRatio[sfb];
      } else { 
	allowed_dist[MONO_CHAN][sfb] = energy[MONO_CHAN][sfb] * 1.1;
      }
    }
   
  /** find the maximum spectral coefficient **/
  /* Bug fix, 3/10/98 CL */
 /* for(i=0; i<NUM_COEFF; i++){ */
 for(i=0; i<sfb_offset[quantInfo -> nr_of_sfb]; i++){ 
   pow_spectrum[i]=(pow(fabs(p_spectrum[ch][i]), 0.75));
    if ((p_spectrum[ch][i]) > max_dct_line){
      max_dct_line = p_spectrum[ch][i];
    }
  }

  if (max_dct_line!=0.0) {
    *common_scalefac = start_com_sf + (int)(16/3 * (log(ABS(pow(max_dct_line,0.75)/MAX_QUANT)/log(2.0))));
  } else {
    *common_scalefac = 20;
  }
  if ((*common_scalefac>200) || (*common_scalefac<0) )
    *common_scalefac = 20;
  *common_scalefac -= 10;
  
  /* initialize the scale_factors that aren't intensity stereo bands */
  is_info=&(ch_info->is_info);
  for(k=0; k< quantInfo -> nr_of_sfb ;k++) {
    sfb_amplify_check[k] = 0;
    calc_sf[k]=1;
    ratio[k] = 0.0;
    scale_factor[k]=((is_info->is_present)&&(is_info->is_used[k])) ? scale_factor[k] : 0;
  }

  /* Mark IS bands by setting book_vector to INTENSITY_HCB */
  ptr_book_vector=quantInfo->book_vector;
  for (k=0;k<quantInfo->nr_of_sfb;k++) {
    if ((is_info->is_present)&&(is_info->is_used[k])) {
      ptr_book_vector[k] = (is_info->sign[k]) ? INTENSITY_HCB2 : INTENSITY_HCB;
    } else {
      ptr_book_vector[k] = 0;
    }
  }


  maxRatio=1;
  largest_sb = 0;
  rateLoopCnt=0;
  do {  /* rate loop */

    do {  /* distortion loop */


      max_quant = 0;
      largest_ratio = 0;
      store_common_scalefac=*common_scalefac;
      for (sb=0; sb<quantInfo -> nr_of_sfb;sb++) {
	/*if (calc_sf[sb]){*/ 
	if ( (calc_sf[sb]) && 
	  ((ptr_book_vector[sb]!=INTENSITY_HCB)&&(ptr_book_vector[sb]!=INTENSITY_HCB2)) ) {
	  double quantFac = pow(2.0, 0.1875*(scale_factor[sb] - *common_scalefac ));
	  double invQuantFac = pow(2.0,-0.25 * (scale_factor[sb] - *common_scalefac ));
	  error_energy[sb] = 0.0;

	  ftmp=0;
	  for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){
	  quant[ch][i] = (int)(pow_spectrum[i] * quantFac  + MAGIC_NUMBER);
	  if (quant[ch][i]>MAX_QUANT)
	    CommonExit(-1,"\nMaxquant exceeded");
 	  requant[ch][i] =  pow_quant[quant[ch][i]]  * invQuantFac; 
	  quant[ch][i] = sgn(p_spectrum[ch][i]) * quant[ch][i];  /* restore the original sign */
	  /* 	  requant[][] = pow(quant[ch][i], (1.333333333)) * invQuantFac ; */

	  /* measure the distortion in each scalefactor band */
	  /* 	  error_energy[sb] += pow((p_spectrum[ch][i] - requant), 2.0); */
	  linediff[i] = (fabs(p_spectrum[ch][i]) - requant[ch][i]);
	  error_energy[sb] += linediff[i]*linediff[i];
	  
	  } /* --- for (i=sfb_offset[sb] --- */
	  /*if ((error_energy[sb]>energy[MONO_CHAN][sb]))
	    CommonWarning("\nerror_energy greater than energy, band[%d], e=%5.2f\n",sb,energy[MONO_CHAN][sb]);*/
	  if ( ( allowed_dist[ch][sb] != 0) && (energy[MONO_CHAN][sb] > allowed_dist[ch][sb]) ){
	    ratio[sb] = error_energy[sb] / allowed_dist[ch][sb];
	  }
	  else {
	    ratio[sb] = 1e-15;
	    sfb_amplify_check[sb] = 1;
	  }


	  /* find the scalefactor band with the largest error ratio */
	  if ((ratio[sb] > maxRatio) && (scale_factor[sb]<60) && aacAllowScalefacs){
	    scale_factor[sb]++;
	    sfb_amplify_check[sb] = 1;
	    calc_sf[sb]=1;
	  } else {
	    calc_sf[sb]=0;	  
	  }
	  if ( (ratio[sb] > largest_ratio)&& (scale_factor[sb]<60) ){
	    largest_ratio = ratio[sb];
	  }
	} /* ---  for (sb=0; --- */
      }
      /* amplify the scalefactor of the worst scalefactor band */
      /* check to see if all the sfb's have been amplified.*/
      /* if they have been all amplified, then all_amplified remains at 1 and we're done iterating */
      all_amplified = 1;
      for(j=0; j<quantInfo->nr_of_sfb;j++){
	if (sfb_amplify_check[j] == 0   )
	  all_amplified = 0;
      }

    } while ((largest_ratio > maxRatio) && (all_amplified == 0)  ); 


    /* find a good method to section the scalefactor bands into huffman codebook sections */
    bit_search(quant,              /* Quantized spectral values */
	       /* book_vector, */  /* Now in quantInfo. */
	       huff,               /* Huffman codebooks */
               quantInfo);         /* Quantization information */
	       /* sfb_offset, */   /* Now in quantInfo. */
               /* nr_of_sfb,  */     /* Now in quantInfo. */
               /* block_type);*/   /* Now in quantInfo. */
#ifdef MPEG4V1
    /* Set special codebook for bands coded via PNS  */
    if (block_type[MONO_CHAN] != ONLY_SHORT_WINDOW) {     /* long blocks only */
      for(k=0;k<quantInfo->nr_of_sfb;k++) {
        if (quantInfo->pns_sfb_flag[k]) {
          if (quantInfo->book_vector[k])
            fprintf(stderr,"\n PNS: Subst. over non-zero book (%d), sfb=%d !",quantInfo-> book_vector[k], k);
          quantInfo->book_vector[k] = PNS_HCB;
        }
      }
    }
#endif    
    /* calculate the amount of bits needed for encoding the huffman codebook numbers */
    book_bits = sort_book_numbers(quantInfo,             /* Quantization information */
				  /* book_vector,*/      /* Now in quantInfo. */
				  output_book_vector,    /* Output codebook vector, formatted for bitstream */
				  /* nr_of_sfb[MONO_CHAN],*/   /* Now in quantInfo. */
				  /* block_type[MONO_CHAN],*/  /* Now in quantInfo. */
				  fixed_stream,          /* Bitstream */
				  0);                    /* Write flag: 0 count, 1 write */

    /* calculate the amount of bits needed for the spectral values */
    quantInfo -> spectralCount = 0;
    spectral_bits = 0;
    for(k=0;k< quantInfo -> nr_of_sfb;k++) {  
      calc_sf[k]=1;
      spectral_bits += output_bits(/* block_type,*/     /* Now in quantInfo */    
				   quantInfo,
				   huff,     
				   quantInfo->book_vector[k],
				   quant,
				   quantInfo->sfb_offset[k], 
				   quantInfo->sfb_offset[k+1]-quantInfo->sfb_offset[k], 
				   /*data,*/            /* Now in quantInfo */
				   /*len,*/             /* Now in quantInfo */
				   /*&counter,*/        /* Now in quantInfo */
				   0);
    }

    /* the number of bits for the scalefactors */
    sf_bits = write_scalefactor_bitstream(/* nr_of_sfb[MONO_CHAN],*//* Now in quantInfo */
					  /* scale_factor,*/        /* Now in quantInfo */
					  /*book_vector,*/          /* Now in quantInfo */
					  fixed_stream,             /* Bitstream */  
					  0,                        /* Write flag */
					  /* window_group_length,*/ /* Now in quantInfo */
					  /* num_window_groups, */  /* Now in quantInfo */
					  /* common_scalefac, */    /* Now in quantInfo */
					  quantInfo
                                          );               /* Quantization information */
					  /* block_type[MONO_CHAN]);* Now in quantInfo */


    /* the total amount of bits required */
    max_bits = spectral_bits + sf_bits + book_bits + extra_bits;

    /*     fprintf(stderr,"\n^^^ [bits to transmit = %d] global=%d spectral=%d sf=%d book=%d extra=%d", */
    /* 	   max_bits,common_scalefac,spectral_bits,\ */
    /* 	   sf_bits, book_bits, extra_bits); */
    if (all_amplified){ 
      maxRatio = maxRatio*2;
      *common_scalefac += 1;      
    }

    *common_scalefac += 1;
    if ( *common_scalefac > 200 ) {
      CommonExit(-1,"\nERROR in loops : common_scalefac is %d",*common_scalefac );
    }
    rateLoopCnt++;
    /* --- TMP --- */
    /* byte align ! 
       printf("MAX bits %d to byte align=%d\n", max_bits, (8-(max_bits%8))%8); 
       max_bits += ((8-(max_bits%8))%8); 
       */

  } while( max_bits > average_block_bits );

  *common_scalefac = store_common_scalefac;
  if ( qdebug > 0 ) {
    fprintf(stderr,"\n*** [bits transmitted = %d] spectral=%d sf=%d book=%d extra=%d largestRatio=%4.1f loopCnt=%d comm_sf=%d",
	    max_bits,spectral_bits, sf_bits,book_bits,extra_bits,10*log10(largest_ratio+1e-15),rateLoopCnt,*common_scalefac);
  }
  /* offset the differenec of common_scalefac and scalefactors by SF_OFFSET  */
  for (i=0; i<quantInfo->nr_of_sfb; i++){
    if ((ptr_book_vector[i]!=INTENSITY_HCB)&&(ptr_book_vector[i]!=INTENSITY_HCB2)) {
      scale_factor[i] = *common_scalefac - scale_factor[i] + SF_OFFSET;
    }
  }
  *common_scalefac = global_gain = scale_factor[0];

  /* place the codewords and their respective lengths in arrays data[] and len[] respectively */
  /* there are 'counter' elements in each array, and these are variable length arrays depending on the input */

  quantInfo -> spectralCount = 0;
  for(k=0;k< quantInfo -> nr_of_sfb; k++) {  
    if ( qdebug>3 )
      fprintf(stderr,"\n{band %d}  ",k);
      spectral_bits += output_bits(/* block_type,*/     /* Now in quantInfo */    
				   quantInfo,
				   huff,     
				   quantInfo->book_vector[k],
				   quant,
				   quantInfo->sfb_offset[k],
				   quantInfo->sfb_offset[k+1]-quantInfo->sfb_offset[k], 
				   /*data,*/            /* Now in quantInfo */
				   /*len,*/             /* Now in quantInfo */
				   /*&counter,*/        /* Now in quantInfo */
				   1);
  }

#if 0 /* Move bitstream write to outside of this function */

  /* create the extra header information, and send out the raw data into the bitstream */
  bits_written = write_to_bitstream(quantInfo,                 /* Quantization information */
				    /* nr_of_sfb[MONO_CHAN],*/ /* Now in quantInfo */
				    /* block_type[MONO_CHAN],*//* Now in quantInfo */
				    /* scale_factor,*/         /* Now in quantInfo */
				    fixed_stream);
				    /* book_vector,*/          /* Now in quantInfo */
				    /* data,*/                 /* Now in quantInfo */
				    /* len,*/                  /* Now in quantInfo */
				    /* counter);*/             /* Now in quantInfo */
				    /* num_window_groups,*/    /* Now in quantInfo */
				    /* window_group_length,*/  /* Now in quantInfo */
				    /* global_gain,*/          /* Now in quantInfo */
				    /* window_shape);*/        /* Now in quantInfo */


  if ( qdebug > 1 ) {
    debugPrint(quantInfo,ratio,energy,allowed_dist[MONO_CHAN],error_energy);
  }

  if ( qdebug > 5 ) {
    for (sb=0; sb<nr_of_sfb[MONO_CHAN]; sb++){
      fprintf(stderr,"\nquantval band %d : ",sb);
      for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){
	fprintf(stderr,"%5i",quant[0][i]);
      }
    }    
  }

#endif 


  /* write the reconstructed spectrum to the output for use with prediction */
  {
    int ch, i;
    for( ch=0; ch<nr_of_chan; ch++ ) {
      for (sb=0; sb<quantInfo -> nr_of_sfb; sb++){
	if ( qdebug > 4 ) {
	  fprintf(stderr,"\nrequantval band %d : ",sb);
	}
	if ((ptr_book_vector[sb]==INTENSITY_HCB)||(ptr_book_vector[sb]==INTENSITY_HCB2)){
	  for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){
	    p_reconstructed_spectrum[ch][i]=673;
	  }
	} else {
	  for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){
	    p_reconstructed_spectrum[ch][i] = sgn(quant[ch][i]) * requant[ch][i]; 
	    if ( qdebug > 4 ) {
	      fprintf(stderr,"%7.0f",(float)p_reconstructed_spectrum[0][i]);
	    }
	  }
	}
      }
    }
  }

  return(bits_written);
}



int sort_for_grouping(AACQuantInfo* quantInfo,        /* ptr to quantization information */
		      int sfb_width_table[],          /* Widths of single window */
		      double *p_spectrum[],           /* Spectral values, noninterleaved */
		      double *SigMaskRatio,
		      double *PsySigMaskRatio)
{
  int i,j,ii;
  int index = 0;
  double tmp[1024];
  int book=1;
  int group_offset=0;
  int k=0;
  int windowOffset = 0;


  /* set up local variables for used quantInfo elements */
  int* sfb_offset = quantInfo -> sfb_offset;
  int* nr_of_sfb = &(quantInfo -> nr_of_sfb);
  int* window_group_length;
  int num_window_groups;
  *nr_of_sfb = quantInfo->max_sfb;              /* Init to max_sfb */
  window_group_length = quantInfo -> window_group_length;
  num_window_groups = quantInfo -> num_window_groups;
 
  /* calc org sfb_offset just for shortblock */
  sfb_offset[k]=0;
  for (k=1 ; k <*nr_of_sfb+1; k++) {
    sfb_offset[k] = sfb_offset[k-1] + sfb_width_table[k-1];
  }

  /* sort the input spectral coefficients */
#ifdef DEBUG_xx
  for (k=0; k<1024; k++){
    p_spectrum[MONO_CHAN][k] = k;
  }
#endif
  index = 0;
  group_offset=0;
  for (i=0; i< num_window_groups; i++) {
    for (k=0; k<*nr_of_sfb; k++) {
      for (j=0; j < window_group_length[i]; j++) {
	for (ii=0;ii< sfb_width_table[k];ii++)
	  tmp[index++] = p_spectrum[MONO_CHAN][ii+ sfb_offset[k] + 128*j +group_offset];
      }
    }
    group_offset +=  128*window_group_length[i];     
  }

#ifdef DEBUG_xx
  ii=0;
  for (i=0;i<num_window_groups;i++){
     fprintf(stderr,"\ngroup%d: " ,i);
    for (k=0; k<128*window_group_length[i]; k++){
      fprintf(stderr," %4.0f" ,tmp[ii++]);
    }
  }
#endif  
  for (k=0; k<1024; k++){
    p_spectrum[MONO_CHAN][k] = tmp[k];
  }

	    

  /* now calc the new sfb_offset table for the whole p_spectrum vector*/
  index = 0;
  sfb_offset[index++] = 0;	  
  windowOffset = 0;
  for (i=0; i < num_window_groups; i++) {
    for (k=0 ; k <*nr_of_sfb; k++) {
      /* for this window group and this band, find worst case inverse sig-mask-ratio */
      int bandNum=windowOffset*NSFB_SHORT + k;
      double worstISMR = PsySigMaskRatio[bandNum];
      int w;
      for (w=1;w<window_group_length[i];w++) {
	bandNum=(w+windowOffset)*NSFB_SHORT + k;
	if (PsySigMaskRatio[bandNum]<worstISMR) {
	  worstISMR = PsySigMaskRatio[bandNum];
	}
     }
      SigMaskRatio[k+ i* *nr_of_sfb]=worstISMR;
      sfb_offset[index] = sfb_offset[index-1] + sfb_width_table[k]*window_group_length[i] ;
      index++;
    }
    windowOffset += window_group_length[i];
  }

  *nr_of_sfb = *nr_of_sfb * num_window_groups;  /* Number interleaved bands. */


  return 0;
}

sort_book_numbers(AACQuantInfo* quantInfo,     /* Quantization information */
		  int output_book_vector[],    /* Output codebook vector, formatted for bitstream */
		  BsBitStream* fixed_stream,   /* Bitstream */
		  int write_flag)              /* Write flag: 0 count, 1 write */
{

  /*
    This function inputs the vector, 'book_vector[]', which is of length SFB_NUM_MAX,
    and contains the optimal huffman tables of each sfb.  It returns the vector, 'output_book_vector[]', which
    has it's elements formatted for the encoded bit stream.  It's syntax is:
   
    {sect_cb[0], length_segment[0], ... ,sect_cb[num_of_sections], length_segment[num_of_sections]}

    The above syntax is true, unless there is an escape sequence.  An
    escape sequence occurs when a section is longer than 2 ^ (bit_len)
    long in units of scalefactor bands.  Also, the integer returned from
    this function is the number of bits written in the bitstream, 
    'bit_count'.  

    This function supports both long and short blocks.
    */

  int i;
  int repeat_counter = 1;
  int bit_count = 0;
  int previous;
  int max, bit_len,sfbs;
  int max_sfb,g,band;

  /* Set local pointers to quantInfo elements */
  int* book_vector = quantInfo -> book_vector;
  int nr_of_sfb = quantInfo -> nr_of_sfb;

  if (quantInfo->block_type == ONLY_SHORT_WINDOW){
    max = 7;
    bit_len = 3;
  }
  else {  /* the block_type is a long,start, or stop window */
    max = 31;
    bit_len = 5;
  }

  /* Compute number of scalefactor bands */
  max_sfb =   quantInfo->nr_of_sfb/quantInfo->num_window_groups;


  for (g=0;g<quantInfo->num_window_groups;g++) {
    band=g*max_sfb;

    repeat_counter=1;

    previous = book_vector[band];
    if (write_flag) {   
      BsPutBit(fixed_stream,book_vector[band],4);  
      if (qdebug > 1 )
	fprintf(stderr,"\nbookNrs:\n%d = ",book_vector[band]);
    }
    bit_count += 4;

    for (i=band+1;i<band+max_sfb;i++) {
      if( (book_vector[i] != previous)) {
	if (write_flag) {
	  BsPutBit(fixed_stream,repeat_counter,bit_len);  
	  if (qdebug > 1 )
	    fprintf(stderr,"%d\n%d = ",i,book_vector[i]);
	}
	bit_count += bit_len;
	
	if (repeat_counter == max){  /* in case you need to terminate an escape sequence */
	  if (write_flag) BsPutBit(fixed_stream,0,bit_len);  
	  bit_count += bit_len;
	}
	
	if (write_flag) BsPutBit(fixed_stream,book_vector[i],4);  
	bit_count += 4;
	previous = book_vector[i];
	repeat_counter=1;
	
      }
      /* if the length of the section is longer than the amount of bits available in */
      /* the bitsream, "max", then start up an escape sequence */
      else if ((book_vector[i] == previous) && (repeat_counter == max)) { 
	if (write_flag) {
	  BsPutBit(fixed_stream,repeat_counter,bit_len);  
	  if (qdebug > 1 )
	    fprintf(stderr,"(esc)");	
	}
	bit_count += bit_len;
	repeat_counter = 1;
      }
      else {
	repeat_counter++;
      }
    }
    
    if (write_flag) {
      BsPutBit(fixed_stream,repeat_counter,bit_len);  
      if (qdebug > 1 )
	fprintf(stderr,"%d\n",i);  
    }
    bit_count += bit_len;
    if (repeat_counter == max) {  /* special case if the last section length is an */
      /* escape sequence */
      if (write_flag) BsPutBit(fixed_stream,0,bit_len);  
      bit_count += bit_len;
    }
    

  }  /* Bottom of group iteration */

  return(bit_count);
}


int bit_search(int quant[CHANNEL][NUM_COEFF],  /* Quantized spectral values */
	       int huff[13][1090][4],          /* Huffman codebooks */
               AACQuantInfo* quantInfo)        /* Quantization information */
  /*
  This function inputs a vector of quantized spectral data, quant[][], and returns a vector,
  'book_vector[]' that describes how to group together the scalefactor bands into a smaller
  number of sections.  There are SFB_NUM_MAX elements in book_vector (equal to 49 in the 
  case of long blocks and 112 for short blocks), and each element has a huffman codebook 
  number assigned to it.

  For a quick and simple algorithm, this function performs a binary
  search across the sfb's (scale factor bands).  On the first approach, it calculates the 
  needed amount of bits if every sfb were its own section and transmitted its own huffman 
  codebook value side information (equal to 9 bits for a long block, 7 for a short).  The 
  next iteration combines adjacent sfb's, and calculates the bit rate for length two sfb 
  sections.  If any wider two-sfb section requires fewer bits than the sum of the two 
  single-sfb sections (below it in the binary tree), then the wider section will be chosen.
  This process occurs until the sections are split into three uniform parts, each with an
  equal amount of sfb's contained.  

  The binary tree is stored as a two-dimensional array.  Since this tree is not full, (there
  are only 49 nodes, not 2^6 = 64), the numbering is a little complicated.  If the tree were
  full, the top node would be 1.  It's children would be 2 and 3.  But, since this tree
  is not full, the top row of three nodes are numbered {4,5,6}.  The row below it is
  {8,9,10,11,12,13}, and so on.  

  The binary tree is called bit_stats[112][3].  There are 112 total nodes (some are not
  used since it's not full).  bit_stats[x][0] holds the bit totals needed for the sfb sectioning
  strategy represented by the node x in the tree.  bit_stats[x][1] holds the optimal huffman
  codebook table that minimizes the bit rate, given the sectioning boundaries dictated by node x.
*/

{
  int i,j,k,m,n;
  int hop;
  int min_book_choice[112][3];
  int bit_stats[240][3];
  int total_bits;
  int total_bit_count;
  int levels;
  double fraction;

  /* Set local pointer to quantInfo book_vector */
  int* book_vector = quantInfo -> book_vector;

#if 1
  levels = (int) ((log((double)quantInfo->nr_of_sfb)/log((double)2.0))+1); 
  fraction = (pow(2,levels)+quantInfo->nr_of_sfb)/(double)(pow(2,levels)); 
#else
  if (nr_of_sfb[MONO_CHAN] == 49){  /* one long window */
    levels = 6;
    fraction = 1.75;
  }
  else if (nr_of_sfb[MONO_CHAN] == 8*14){  /* 8 short windows */
    levels = 7;
    fraction = 1.875;
  }
  else {
    fprintf("       nr_of_sfb[MONO_CHAN] = %d INVALID\n",nr_of_sfb[MONO_CHAN]);
    exit(-1);
  }
#endif

  for(i=0;i<5;i++){
    hop = (int) (pow(2,i));
    total_bits = noiseless_bit_count(quant,
				     huff,
				     hop,
				     min_book_choice,
				     quantInfo);         /* Quantization information */
				     /* sfb_offset,   */ /* Now in quantInfo */
				     /* nr_of_sfb,    */ /* Now in quantInfo */
				     /* block_type);  */ /* Now in quantInfo */

    /* load up the (not-full) binary search tree with the min_book_choice values */
    k=0;
    m=0;
    total_bit_count = 0;

    for (j=(int)(pow(2,levels-i)); j<(int)(fraction*pow(2,levels-i)); j++){
      bit_stats[j][0] = min_book_choice[k][0]; /* the minimum bit cost for this section */
      bit_stats[j][1] = min_book_choice[k][1]; /* used with this huffman book number */

      if (i>0){  /* not on the lowest level, grouping more than one signle scalefactor band per section*/
	if  (bit_stats[j][0] < bit_stats[2*j][0] + bit_stats[2*j+1][0]){

	  /* it is cheaper to combine surrounding sfb secionts into one larger huffman book section */
	  for(n=k;n<k+hop;n++) { /* write the optimal huffman book value for the new larger section */
	    if ( (book_vector[n]!=INTENSITY_HCB)&&(book_vector[n]!=INTENSITY_HCB2) ) { /* Don't merge with IS bands */
	      book_vector[n] = bit_stats[j][1];
	    }
	  }
        }

	else {  /* it was cheaper to transmit the smaller huffman table sections */
          bit_stats[j][0] = bit_stats[2*j][0] + bit_stats[2*j+1][0];
	}

      } else {  /* during the first stage of the iteration, all sfb's are individual sections */
       	if ( (book_vector[k]!=INTENSITY_HCB)&&(book_vector[k]!=INTENSITY_HCB2) ) {
	  book_vector[k] = bit_stats[j][1];  /* initially, set all sfb's to their own optimal section table values */
	}
      }
      total_bit_count = total_bit_count +  bit_stats[j][0];
      k=k+hop;
      m++;
    }
  }
/*   book_vector[k] = book_vector[k-1]; */
  return(total_bit_count);
}




int noiseless_bit_count(int quant[CHANNEL][NUM_COEFF],
			int huff[13][1090][4],
			int hop,
			int min_book_choice[112][3],
			AACQuantInfo* quantInfo)         /* Quantization information */
{
  int i,j,k;

  /* 
     This function inputs:
     - the quantized spectral data, 'quant[][]';
     - all of the huffman codebooks, 'huff[][]';
     - the size of the sections, in scalefactor bands (SFB's), 'hop';
     - an empty matrix, min_book_choice[][] passed to it; 

     This function outputs:
     - the matrix, min_book_choice.  It is a two dimensional matrix, with its
     rows corresponding to spectral sections.  The 0th column corresponds to 
     the bits needed to code a section with 'hop' scalefactors bands wide, all using 
     the same huffman codebook.  The 1st column contains the huffman codebook number 
     that allows the minimum number of bits to be used.   

     Other notes:
     - Initally, the dynamic range is calculated for each spectral section.  The section
     can only be entropy coded with books that have an equal or greater dynamic range
     than the section's spectral data.  The exception to this is for the 11th ESC codebook.
     If the dynamic range is larger than 16, then an escape code is appended after the
     table 11 codeword which encodes the larger value explicity in a pseudo-non-uniform
     quantization method.
     
     */

  int max_sb_coeff;
  int book_choice[12][2];
  int total_bits_cost = 0;
  int data[NUM_COEFF*5];
  int len[NUM_COEFF*5];
  int counter = 0;
  int offset, length, end;
  int q;
  int write_flag = 0;

  /* set local pointer to sfb_offset */
  int* sfb_offset = quantInfo->sfb_offset;
  int nr_of_sfb = quantInfo->nr_of_sfb;

  /* each section is 'hop' scalefactor bands wide */
  for (i=0;i<nr_of_sfb;i=i+hop){ 
    if ((i+hop) > nr_of_sfb)
      q = nr_of_sfb;
    else
      q = i+hop;

    {
    
      /* find the maximum absolute value in the current spectral section, to see what tables are available to use */
      max_sb_coeff = 0;
      for (j=sfb_offset[i]; j<sfb_offset[q]; j++){  /* snl */
	if (ABS(quant[MONO_CHAN][j]) > max_sb_coeff)
	  max_sb_coeff = ABS(quant[MONO_CHAN][j]);
      }
    
      j = 0;
      offset = sfb_offset[i];
      if ((i+hop) > nr_of_sfb){
	end = sfb_offset[nr_of_sfb];
      }
      else
	end = sfb_offset[i+hop];
      length = end - offset;

      /* all spectral coefficients in this section are zero */
      if (max_sb_coeff == 0) { 
	book_choice[j][0] = output_bits(quantInfo,huff,0,quant,offset,length,write_flag);
	book_choice[j++][1] = 0; 

      }

      else {  /* if the section does have non-zero coefficients */
	if(max_sb_coeff < 2){
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,1,quant,offset,length,write_flag);
	  book_choice[j++][1] = 1;
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,2,quant,offset,length,write_flag);
	  book_choice[j++][1] = 2;
	}
	if (max_sb_coeff < 3){
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,3,quant,offset,length,write_flag);
	  book_choice[j++][1] = 3;
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,4,quant,offset,length,write_flag);
	  book_choice[j++][1] = 4;
	}
	if (max_sb_coeff < 5){
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,5,quant,offset,length,write_flag);
	  book_choice[j++][1] = 5;
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,6,quant,offset,length,write_flag);
	  book_choice[j++][1] = 6;
	}
	if (max_sb_coeff < 8){
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,7,quant,offset,length,write_flag);
	  book_choice[j++][1] = 7;
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,8,quant,offset,length,write_flag);
	  book_choice[j++][1] = 8;
	}
	if (max_sb_coeff < 13){
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,9,quant,offset,length,write_flag);
	  book_choice[j++][1] = 9;
	  quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	  book_choice[j][0] = output_bits(quantInfo,huff,10,quant,offset,length,write_flag);
	  book_choice[j++][1] = 10;
	}
	/* (max_sb_coeff >= 13), choose table 11 */
	quantInfo->spectralCount = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(quantInfo,huff,11,quant,offset,length,write_flag);
	book_choice[j++][1] = 11;
      }

      /* find the minimum bit cost and table number for huffman coding this scalefactor section */
      min_book_choice[i][0] = 100000;  
      for(k=0;k<j;k++){
	if (book_choice[k][0] < min_book_choice[i][0]){
	  min_book_choice[i][1] = book_choice[k][1];
	  min_book_choice[i][0] = book_choice[k][0];
	}
      }
      total_bits_cost += min_book_choice[i][0];
      
    } 
  }
  return(total_bits_cost);
}



int calculate_esc_sequence(
			   int input,
			   int *len_esc_sequence
			   )
  /* 
   This function takes an element that is larger than 16 and generates the base10 value of the 
   equivalent escape sequence.  It returns the escape sequence in the variable, 'output'.  It
   also passed the length of the escape sequence through the parameter, 'len_esc_sequence'.
*/

{
  float x,y;
  int output;
  int N;

  N = -1;
  y = (float)ABS(input);
  x = y / 16;

  while (x >= 1) {
    N++;
    x = x/2;
  }

  *len_esc_sequence = 2*N + 5;  /* the length of the escape sequence in bits */

  output = (int)((pow(2,N) - 1)*pow(2,N+5) + y - pow(2,N+4));
  return(output);
}



int output_bits(AACQuantInfo* quantInfo,
		int huff[13][1090][4],
                int book,
		int quant[CHANNEL][NUM_COEFF],
                int offset,
    		int length,
		int write_flag)
{
  /* 
     This function inputs 
     - all the huffman codebooks, 'huff[]' 
     - a specific codebook number, 'book'
     - the quantized spectral data, 'quant[][]'
     - the offset into the spectral data to begin scanning, 'offset'
     - the 'length' of the segment to huffman code
     -> therefore, the segment quant[CHANNEL][offset] to quant[CHANNEL][offset+length-1]
     is huffman coded.
     - a flag, 'write_flag' to determine whether the codebooks and lengths need to be written
     to file.  If write_flag=0, then this function is being used only in the quantization
     rate loop, and does not need to spend time writing the codebooks and lengths to file.
     If write_flag=1, then it is being called by the function output_bits(), which is 
     sending the bitsteam out of the encoder.  

     This function outputs 
     - the number of bits required, 'bits'  using the prescribed codebook, book applied to 
     the given segment of spectral data.

     There are three parameters that are passed back and forth into this function.  data[]
     and len[] are one-dimensional arrays that store the codebook values and their respective
     bit lengths.  These are used when packing the data for the bitstream in output_bits().  The
     index into these arrays is 'quantInfo->spectralCount''.  It gets incremented internally in this
     function as counter, then passed to the outside through outside_counter.  The next time
     output_bits() is called, counter starts at the value it left off from the previous call.

   */
     
  int esc_sequence;     
  int len_esc;
  int index;
  int bits=0;
  int tmp = 0;
  int codebook,i,j;
  int counter;

  /* Set up local pointers to quantInfo elements data and len */
  int* data= quantInfo -> data;
  int* len=  quantInfo -> len;

  counter = quantInfo->spectralCount;

  /* This case also applies to intensity stereo encoding */
  /*if (book == 0) { */ /* if using the zero codebook, data of zero length is sent */
  if ((book == 0)||(book==INTENSITY_HCB2)||(book==INTENSITY_HCB)) {  /* if using the zero codebook, 
									data of zero length is sent */
   
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    if (write_flag) {
      quantInfo->data[counter] = 0;
      quantInfo->len[counter++] = 0;
    }
  }

  if ((book == 1) || (book == 2)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+4){
      index = 27*quant[MONO_CHAN][i] + 9*quant[MONO_CHAN][i+1] + 3*quant[MONO_CHAN][i+2] + quant[MONO_CHAN][i+3] + 40;
      codebook = huff[book][index][3];
      tmp = huff[book][index][2];
      bits = bits + tmp;
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d %d %d ",quant[0][i],quant[0][i+1],quant[0][i+2],quant[0][i+3]);
	data[counter] = codebook;
        len[counter++] = tmp;
      }
    }
  }

  if ((book == 3) || (book == 4)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+4){
      index = 27*ABS(quant[MONO_CHAN][i]) + 9*ABS(quant[MONO_CHAN][i+1]) + 3*ABS(quant[MONO_CHAN][i+2]) + ABS(quant[MONO_CHAN][i+3]);
      codebook = huff[book][index][3];
      tmp = huff[book][index][2];
      bits = bits + tmp;
      for(j=0;j<4;j++){
	if(ABS(quant[MONO_CHAN][i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d %d %d ",quant[0][i],quant[0][i+1],quant[0][i+2],quant[0][i+3]);
        data[counter] = codebook;
        len[counter++] = tmp;
	for(j=0;j<4;j++){
	  if(quant[MONO_CHAN][i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant[MONO_CHAN][i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }
    }
  }

  if ((book == 5) || (book == 6)) {
        if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+2){
      index = 9*(quant[MONO_CHAN][i]) + (quant[MONO_CHAN][i+1]) + 40;
      codebook = huff[book][index][3];
      tmp = huff[book][index][2];
      bits = bits + tmp;
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant[0][i],quant[0][i+1]);
        data[counter] = codebook;
        len[counter++] = tmp;
      }
    }
  }

  if ((book == 7) || (book == 8)) {
        if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+2){
      index = 8*ABS(quant[MONO_CHAN][i]) + ABS(quant[MONO_CHAN][i+1]);
      codebook = huff[book][index][3];
      tmp = huff[book][index][2];
      bits = bits + tmp;
      for(j=0;j<2;j++){
	if(ABS(quant[MONO_CHAN][i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant[0][i],quant[0][i+1]);
	data[counter] = codebook;
	len[counter++] = tmp;
	for(j=0;j<2;j++){
	  if(quant[MONO_CHAN][i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant[MONO_CHAN][i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }
    }
  }

  if ((book == 9) || (book == 10)) {
        if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+2){
      index = 13*ABS(quant[MONO_CHAN][i]) + ABS(quant[MONO_CHAN][i+1]);
      codebook = huff[book][index][3];
      tmp = huff[book][index][2];
      bits = bits + tmp;
      for(j=0;j<2;j++){
	if(ABS(quant[MONO_CHAN][i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant[0][i],quant[0][i+1]);
        
	data[counter] = codebook;
        len[counter++] = tmp;

	for(j=0;j<2;j++){
	  if(quant[MONO_CHAN][i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant[MONO_CHAN][i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }
    }
  }

  if ((book == 11)){
        if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    /* First, calculate the indecies into the huffman tables */

    for(i=offset;i<offset+length;i=i+2){
      if ((ABS(quant[MONO_CHAN][i]) >= 16) && (ABS(quant[MONO_CHAN][i+1]) >= 16)) {  /* both codewords were above 16 */
        /* first, code the orignal pair, with the larger value saturated to +/- 16 */
	index = 17*16 + 16;
      }
      else if (ABS(quant[MONO_CHAN][i]) >= 16) {  /* the first codeword was above 16, not the second one */
        /* first, code the orignal pair, with the larger value saturated to +/- 16 */
	index = 17*16 + ABS(quant[MONO_CHAN][i+1]);
      }
      else if (ABS(quant[MONO_CHAN][i+1]) >= 16) { /* the second codeword was above 16, not the first one */
	index = 17*ABS(quant[MONO_CHAN][i]) + 16;
      }
      else {  /* there were no values above 16, so no escape sequences */
	index = 17*ABS(quant[MONO_CHAN][i]) + ABS(quant[MONO_CHAN][i+1]);
      }

      /* write out the codewords */

      tmp = huff[book][index][2];
      codebook = huff[book][index][3];       
      bits += tmp;
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant[0][i],quant[0][i+1]);
	/*	printf("[book %d] {%d %d} \n",book,quant[0][i],quant[0][i+1]);*/
	data[counter] = codebook;
	len[counter++] = tmp;
      }
   
      /* Take care of the sign bits */

      for(j=0;j<2;j++){
	if(ABS(quant[MONO_CHAN][i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	for(j=0;j<2;j++){
	  if(quant[MONO_CHAN][i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant[MONO_CHAN][i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }

      /* write out the escape sequences */

      if ((ABS(quant[MONO_CHAN][i]) >= 16) && (ABS(quant[MONO_CHAN][i+1]) >= 16)) {  /* both codewords were above 16 */
        /* code and transmit the first escape_sequence */
        esc_sequence = calculate_esc_sequence(quant[MONO_CHAN][i],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }

        /* then code and transmit the second escape_sequence */
        esc_sequence = calculate_esc_sequence(quant[MONO_CHAN][i+1],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }
      }

      else if (ABS(quant[MONO_CHAN][i]) >= 16) {  /* the first codeword was above 16, not the second one */
        /* code and transmit the escape_sequence */
        esc_sequence = calculate_esc_sequence(quant[MONO_CHAN][i],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }
      }

      else if (ABS(quant[MONO_CHAN][i+1]) >= 16) { /* the second codeword was above 16, not the first one */
        /* code and transmit the escape_sequence */
        esc_sequence = calculate_esc_sequence(quant[MONO_CHAN][i+1],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }
      } 
    }
  }

  quantInfo -> spectralCount = counter;  /* send the current count back to the outside world */
  return(bits);
}


int write_to_bitstream(AACQuantInfo* quantInfo,
		       BsBitStream* fixed_stream)
{	
	
  /* this function writes out the  bitstream and returns the number of bits written to the bitstream */
  int output_book_vector[SFB_NUM_MAX*2];
  int i;
  int grouping_bits;
  int bit_count=0;
  int tmp;
  int max_sfb;
  int numSpectral;

  /* set up local pointers to data and len */
  int* data = quantInfo -> data;
  int* len = quantInfo -> len;

  /* write ID_SCE, single_element_channel() identifier */
  BsPutBit(fixed_stream,ID_SCE,LEN_SE_ID);  
  bit_count += LEN_SE_ID;

  /* write the element_identifier_tag */
  BsPutBit(fixed_stream,0,LEN_TAG);  
  bit_count += LEN_TAG;

  /* the 8-bit global_gain is the first scalefactor */
  BsPutBit(fixed_stream,quantInfo->common_scalefac,8);  
  bit_count += 8;
  if ( (qdebug > 2)  ) {
    fprintf(stderr,"\nglobal_gain %d ",quantInfo->common_scalefac);
  }
  
  BsPutBit(fixed_stream,0,1);  /* reserved Bit*/
  bit_count +=1;
  /* write out ics_info() information */
  switch (quantInfo->block_type) 
    {
    case ONLY_SHORT_WINDOW:
      BsPutBit(fixed_stream,2,2);  /* short window */
      bit_count += 2;
      break;
    case ONLY_LONG_WINDOW:
      BsPutBit(fixed_stream,0,2);  
      bit_count += 2;
      break;
    case   LONG_SHORT_WINDOW:
      BsPutBit(fixed_stream,1,2);  
      bit_count += 2;
      break;
    case  SHORT_LONG_WINDOW:
      BsPutBit(fixed_stream,3,2);  
      bit_count += 2;
      break;
      
    default:
      CommonExit(-1,"\n unknown blocktype : %d",quantInfo->block_type);
    }

  if (quantInfo->window_shape != 0)
    CommonExit(-1,"\nunknow windowshape");
  BsPutBit(fixed_stream,quantInfo->window_shape,1);  /* window shape */
  bit_count += 1;

  max_sfb =   quantInfo->nr_of_sfb/quantInfo->num_window_groups;
  if (max_sfb* quantInfo->num_window_groups != quantInfo->nr_of_sfb)
    CommonExit(-1,"\nwrong num of nr_of_sfb");

  if (quantInfo -> block_type == ONLY_SHORT_WINDOW){
    BsPutBit(fixed_stream,max_sfb,4); /* let max_sfb_l = 14, the max number of sfb's */
    bit_count += 4;
    grouping_bits = find_grouping_bits(quantInfo->window_group_length,quantInfo->num_window_groups);
    bit_count += 7;
    BsPutBit(fixed_stream,grouping_bits,7);  /* the grouping bits */
    bit_count += 7;
  }
  else { /* block type is either start, stop, or long */
    BsPutBit(fixed_stream,max_sfb,6); /* let max_sfb_l = 49, the max number of sfb's */
    BsPutBit(fixed_stream,0,1);  /* no prediction_data_present */
    bit_count += 7;
  }


  /* writes out section_data() information to the bitstream */
  tmp = sort_book_numbers(quantInfo,output_book_vector,fixed_stream,1);
  bit_count += tmp;

  /* scale_factor_data() information */
  tmp = write_scalefactor_bitstream(fixed_stream,1,quantInfo);

  bit_count += tmp;

  /* no pulse_data_present() */
  BsPutBit(fixed_stream,0,1);
  bit_count += 1;

  /* deal with turning off the TNS data */
  BsPutBit(fixed_stream,1,1); /* switches on TNS for long blocks */
  bit_count += 1;

  if (quantInfo->block_type == ONLY_SHORT_WINDOW) {
    BsPutBit(fixed_stream,0,8); /* n_filt[] = 0 */
    bit_count += 8;
  }
  else  { /* block_type is either long, start, or stop */
    BsPutBit(fixed_stream,0,2); /* n_filt[] = 0 */
    bit_count += 2;
  } 

  /* YT 970615 turn on the gain control data */
  BsPutBit(fixed_stream,0,1); /* switches off the gain control */
  bit_count += 1;


  /* write out the spectral_data() */
  tmp=0;
  numSpectral = quantInfo->spectralCount;
  for(i=0;i<numSpectral;i++) {
    if (quantInfo->len[i] > 0) {  /* only send out non-zero codebook data */
      BsPutBit(fixed_stream,data[i],len[i]);
      tmp += len[i];
    }
  }
  bit_count += tmp;

  return(bit_count);
}


int find_grouping_bits(int window_group_length[],
		       int num_window_groups
		       )
{

  /* This function inputs the grouping information and outputs the seven bit 
     'grouping_bits' field that the NBC decoder expects.  */


  int grouping_bits = 0;
  int tmp[8];
  int i,j;
  int index=0;

  for(i=0; i<num_window_groups; i++){
    for (j=0; j<window_group_length[i];j++){
      tmp[index++] = i;
      /*      printf("tmp[%d] = %d\n",index-1,tmp[index-1]);*/
    }
  }

  for(i=1; i<8; i++){
    grouping_bits = grouping_bits << 1;
    if(tmp[i] == tmp[i-1]) {
      grouping_bits++;
    }
  }
  
  /*  printf("grouping_bits = %d  [i=%d]\n",grouping_bits,i);*/
  return(grouping_bits);
}



int write_scalefactor_bitstream(BsBitStream* fixed_stream,             /* Bitstream */  
				int write_flag,                        /* Write flag */
				AACQuantInfo* quantInfo)               /* Quantization information */
{

  /* this function takes care of counting the number of bits necessary */
  /* to encode the scalefactors.  In addition, if the write_flag == 1, */
  /* then the scalefactors are written out the fixed_stream output bit */
  /* stream.  it returns k, the number of bits written to the bitstream*/

  int i,j,k=0;
  int m;
  int diff,length,codeword;
  int previous_scale_factor;
  int previous_is_factor;       /* Intensity stereo */
  int limit;
  int index = 0;
  int count = 0;
  int zero_flag;
  int group_offset = 0;
  int sf_out = 0;
  int sf_not_out = 0;
  int nr_of_sfb_per_group=0;

#ifdef MPEG4V1
  int pns_pcm_flag = 1;
  int previous_noise_nrg = quantInfo->common_scalefac ;
#endif

  /* set local pointer to quantInfo elements */
  int* scale_factors = quantInfo->scale_factor;
 
  if (quantInfo->block_type == ONLY_SHORT_WINDOW) { /* short windows */
    nr_of_sfb_per_group = quantInfo->nr_of_sfb/quantInfo->num_window_groups;
  }
  else {
    nr_of_sfb_per_group = quantInfo->nr_of_sfb;
    quantInfo->num_window_groups = 1;
    quantInfo->window_group_length[0] = 1;
  }

  previous_scale_factor = scale_factors[0];
  previous_scale_factor = quantInfo->common_scalefac;
  previous_is_factor = 0;
  if ( (qdebug > 2) && write_flag ) {
    fprintf(stderr,"\nscalefacs :");	
  }
    
  for(j=0; j<quantInfo->num_window_groups; j++){
    for(i=0;i<nr_of_sfb_per_group;i++) {  
      /* test to see if any codebooks in a group are zero */
      if ( (qdebug > 1) && write_flag ) {
	fprintf(stderr," %d:%d",index,scale_factors[index]);	
      }
      if ( (quantInfo->book_vector[index]==INTENSITY_HCB) ||
	   (quantInfo->book_vector[index]==INTENSITY_HCB2) ) {
        /* only send scalefactors if using non-zero codebooks */
	diff = scale_factors[index] - previous_is_factor;
	length = huff[12][diff+60][2];
	k+=length;
	previous_is_factor = scale_factors[index];
	if (write_flag == 1 ) {   
	  codeword = huff[12][diff+60][3];
	  BsPutBit(fixed_stream,codeword,length); 
	  sf_out++;
	}
      } else 
#ifdef MPEG4V1
        if (quantInfo-> book_vector[index] == PNS_HCB){
        diff = quantInfo->pns_sfb_nrg[index] - previous_noise_nrg;
        if (pns_pcm_flag) {
          pns_pcm_flag = 0;
          if (diff+PNS_PCM_OFFSET<0  ||  
              diff+PNS_PCM_OFFSET>=(1<<PNS_PCM_BITS)) {
            fprintf(stderr,"%3d (PNS PCM Overflow0) *****\n", diff);
          }
          length = PNS_PCM_BITS;
          codeword = diff + PNS_PCM_OFFSET;
        } else {
          if (diff<-60  ||  diff>60)
            printf("%3d (PNS Overflow0) *****\n", diff);
          length = huff[12][diff+60][2];    
	  codeword = huff[12][diff+60][3];
        }
	k += length;
        previous_noise_nrg = quantInfo->pns_sfb_nrg[index];
	if (write_flag == 1 ) {   
	  /*printf
            ("  scale_factor[%d] = %d\n",index,scale_factors[index]);*/
	  BsPutBit(fixed_stream,codeword,length); 
	  sf_out++;
	}
      } else 
#endif
        if (quantInfo->book_vector[index]) {
        /* only send scalefactors if using non-zero codebooks */
	diff = scale_factors[index] - previous_scale_factor;
	length = huff[12][diff+60][2];    
	k+=length;
	previous_scale_factor = scale_factors[index];
	if (write_flag == 1 ) {   
	  codeword = huff[12][diff+60][3];
	  BsPutBit(fixed_stream,codeword,length); 
	  sf_out++;
	}
      }
      else {
	/*	if (write_flag) printf("  zero_flag = 1, [j=%d index=%d i=%d]\n",j,index,i);*/
	sf_not_out++;
      }
      index++;
    }
  }
  /*  if (write_flag) printf("sf_out = %d  sf_not_out = %d\n",sf_out,sf_not_out);*/
  return(k);
}





