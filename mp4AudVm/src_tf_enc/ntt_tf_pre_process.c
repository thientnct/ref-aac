/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1887-07-17,                                      */
/*   Akio Jin (NTT) on 1887-10-23,                                           */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14486-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1887.                                                        */
/*****************************************************************************/
#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */


#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "ntt_encode.h"
#include "ntt_tools.h"

#define N_MEAS_MEM 2

#define FINE_CUTOFF 10

#define PST_BASS   10  /* postfilter switch measure base address */
#define PST_CUT_L  30  /* postfilter switch measure lower ntt_cutoff */
#define PST_CUT_U  100 /* postfilter switch measure upper ntt_cutoff */

#define PST_THR_U  0.8 /* postfilter switch upper threshold */
#define PST_THR_L  0.45 /* postfilter switch lower threshold */

void ntt_tf_pre_process(/* Input */
			double sig[],         /* input signal */
			/* Output */
			ntt_INDEX *index,     /* code index for block type */
			ntt_PARAM *param_ntt, /* encoding parameters */
			/* Control */
			int    med_sw,  /* enable/disable flag for medium block length */
			int    InitFlag)/* initialization flag */
{

    /*--- Variables ---*/
    int fine_sw, s_attack, m_attack;

    param_ntt->fine_sw = 0;
    param_ntt->speech_sw = 0;
    fine_sw = param_ntt->fine_sw;
    /*index->pf = param_ntt->speech_sw; */


    /*--- A.Jin 1887.10.18 ---*/
     ntt_mchkat48(sig, index->numChannel, index->block_size_samples,
		  index->isampf, &s_attack, &m_attack, InitFlag);

    m_attack = 0;
    ntt_get_wty( s_attack, m_attack,  &(index->w_type), InitFlag );
}


void ntt_get_wty(int flag,      /* Input : trigger for short block length */
		 int m_flag,    /* Input : trigger for medium block length */
		 WINDOW_SEQUENCE *w_type,   /* Output : code index for block type */
		 int InitFlag ) /* Control : initialization flag */
{
    static int	w_type_pre;
    static int	flag_pre, m_flag_pre;

    if ( InitFlag ){
	flag_pre = 0;
	m_flag_pre = 0;
	w_type_pre = ONLY_LONG_SEQUENCE;
    }

    if (InitFlag){
      if (flag)        *w_type = EIGHT_SHORT_SEQUENCE;
      else             *w_type = ONLY_LONG_SEQUENCE;
    }
    else{
      switch( w_type_pre ){
      case ONLY_LONG_SEQUENCE:
	if ( flag )      *w_type = LONG_START_SEQUENCE;
	else             *w_type = ONLY_LONG_SEQUENCE;
	break;
      case LONG_START_SEQUENCE:
	*w_type = EIGHT_SHORT_SEQUENCE;
	break;
      case LONG_STOP_SEQUENCE:
	if ( flag )      *w_type = LONG_START_SEQUENCE;
	else             *w_type = ONLY_LONG_SEQUENCE;
	break;
      case EIGHT_SHORT_SEQUENCE:
	if (flag || flag_pre) *w_type = EIGHT_SHORT_SEQUENCE;
	else                  *w_type = LONG_STOP_SEQUENCE;
	break;
      default:
	fprintf( stderr,
		"Fatal error! %d: no such window type.\n", w_type_pre );
	exit(1);
      }
    }
    w_type_pre  = *w_type;
    flag_pre = flag;
}



#define N_BLK_MAX	4086
#define	N_SFT_MAX       256
#define	L_POW_MAX	2048
#define	N_LPC_MAX	2


void ntt_mchkat48(double in[],  /* Input signal */
                  int    numChannel,
		  int    block_size_samples,
		  int    isampf,
		  int    *flag, /* flag for attack */
                  int    *mflag,
                  int    InitFlag) /* Initialization Flag */
{
  /*--- Variables ---*/
  int                 n_div, iblk, ismp, ich;
  double              bufin[N_SFT_MAX];
  static double sig[MAX_TIME_CHANNELS][N_BLK_MAX+N_LPC_MAX];
  double        wsig[N_BLK_MAX];
  static double wdw[N_BLK_MAX];
  static double wlag[N_LPC_MAX+1];
  double        cor[N_LPC_MAX+1],ref[N_LPC_MAX+1],alf[N_LPC_MAX+1];
  static double prev_alf[MAX_TIME_CHANNELS][N_LPC_MAX+1];
  double        resid, wpowfr;
  double        long_power;
  double        synth, resid2;
  static int    botflag, botflagm;
  static int    N_BLK, N_SFT, S_POW, L_POW, N_LPC;
  double recip, ratior, ratio, sum, product;
  double sum_i, sum_d;
  double mod, band;
 static int frame =0;
 static double product_p ;
 static double product_pp;
 static double ref_p;
  /*--- Initialization ---*/
  if ( InitFlag ){
    /* Set parameters */
    N_BLK      = block_size_samples;
    N_SFT      = block_size_samples/8;
    S_POW      = block_size_samples/8;
    L_POW      = block_size_samples;
    N_LPC      = 1;
    /* clear buffers */
    for ( ich=0; ich<numChannel; ich++ ){
      ntt_zerod( N_LPC+N_BLK, sig[ich] );
      ntt_zerod( N_LPC+1, prev_alf[ich] );
  }
    product_p = 0.0;
    product_pp =0.0;
    ref_p =0.0;
    /* set windows */
    ntt_lagwdw( wlag, N_LPC, 0.02 );
    ntt_hamwdw( wdw, N_BLK );
    botflag=botflagm=0;
}


  n_div = block_size_samples/N_SFT;
  *flag = *mflag = 0;
  if (botflag) *flag=1;
  botflag=0;
  botflagm=0;
  sum=0.0; product=1.0, recip =0.0;
  sum_i=sum_d=0.1;
  for ( ich=0; ich<numChannel; ich++ ){
    for ( iblk=0; iblk<n_div; iblk++ ){
      ntt_cutfr( (int)floor(block_size_samples*(1/2.+1/8.)+iblk*N_SFT), 
                N_SFT, ich, in, 
		numChannel, block_size_samples, bufin );
      ntt_movdd(N_SFT,bufin,&sig[ich][N_LPC+N_BLK-N_SFT]);
      /*--- Calculate long power ---*/
      long_power =
        (ntt_dotdd( L_POW, &sig[ich][N_BLK-L_POW], &sig[ich][N_BLK-L_POW] )+0.1)
          / L_POW;
      long_power = sqrt(long_power);

      /*--- Calculate alpha parameter ---*/
      ntt_mulddd( N_BLK, &sig[ich][N_LPC], wdw, wsig );
      ntt_sigcor( wsig, N_BLK, &wpowfr, cor, N_LPC ); cor[0] = 1.0;
      ntt_mulddd( N_LPC+1, cor, wlag, cor );
      ntt_corref( N_LPC, cor, alf, ref, &resid );
      /*--- Get residual signal and its power ---*/
      resid2 = 0.;
      for ( ismp=N_BLK-S_POW; ismp<N_BLK; ismp++ ){
        synth = sig[ich][ismp]+
          prev_alf[ich][1]*sig[ich][ismp-1];
        resid2 += synth*synth;
    }
      resid2 /= long_power;
      resid2 = sqrt(resid2);
      recip += 1./(resid2 +0.0001);
      sum += resid2+0.0001;
      product *= (resid2+0.0001);
      sum_i += resid2*(iblk+1);
      sum_d += resid2*(n_div-iblk);
      ntt_movdd( N_LPC, &alf[1], &prev_alf[ich][1] );

      ntt_movdd( N_BLK-N_SFT, &sig[ich][N_LPC+N_SFT], &sig[ich][N_LPC] );
  }
}
  product = pow(product, 1./(double)(n_div*numChannel)); 
  recip   =1./(recip/(n_div*numChannel));
  sum = sum/(n_div*numChannel);
  ratior = product/recip;
  ratio = sum/product;
#if 1
  if((ratior> 1.15) && (product > 2.* product_p)
  && (product > 2.* product_pp) && (ref_p*fabs(ref[1])<0.4)) ratio *= ratior*ratior;
#endif
  product_pp = product_p;
  product_p = product;
  ref_p = fabs(ref[1]);
  mod =1.0;
  band =1.0;
  if(isampf <44 ) band = 0.75;
  if(isampf <16 ) band = 0.5;
  if(L_POW/S_POW ==8) mod = 0.85;
  if(L_POW/S_POW ==4) mod = 0.8;
  
  if((ratio > 1.8*mod)
     ||((ratio>1.4*mod) && (ratio > 0.5/band/(1.-ref[1]*ref[1])))){
    *mflag = 0;
    *flag = 1;

   }
#if 0
else if((ratio > 1.25*mod)
          ||((ratio>1.2) && (ratio > 0.5/band/(1.-ref[1]*ref[1])))){
    *mflag =0 /*1*/;
    *flag = 1 /*0*/;
}
#endif
else {
    *mflag = 0;
    *flag = 0;
}
/*
   fprintf(stderr, "UUUUU%12.3f %12.3f %12.3f %12.3f %12.3f %12.3f \n" , 
  product, recip, sum, ratior, ratio, sum_i/sum_d);
   fprintf(stderr, "%12.3f %12.3f %12.3f %5d \n"
   ,ratio, 1.8*mod, 0.5/band/(1.-ref[1]*ref[1]), *flag);
*/
if( ref[1] > 0.95 ) *flag = 0;
if( sum_i/sum_d < 0.25 ) *flag = 0;
/* if(*flag == 1)fprintf(stderr, "SHORT \n"); */
}

#if 0

void ntt_win_sw_init(/* Input */
		     int max_ch,
		     int block_size_samples,
		     int sampling_rate,
		     int bit_rate,
		     int short_win_in_long)
{
    numChannel = max_ch;
    block_size_samples  = block_size_samples;
    isampf = (int)(sampling_rate/1000.+0.5);
    block_size_samples_S = block_size_samples / short_win_in_long;
    fprintf(stderr, "UUUUiniwinPPP %5d %5d %5d %5d numChannel block_size_samples isampf block_size_samples_S\n", numChannel, block_size_samples, isampf, block_size_samples_S);
}
#endif
