/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Bernd Edler and Hendrik Fuchs, University of Hannover

and edited by
  Michael Matejko, Fraunhofer IIS

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

/***********************************************************************************
 * MONOPRED									   *
 *										   *
 *	Contains the core functions for an intra channel (or mono) predictor	   *
 *	using a backward adaptive lattice predictor.				   *
 *										   *
 *	init_pred_stat():	initialisation of all predictor parameters	   *
 *	monopred():		calculation of a predicted value from		   *
 *				preceeding (quantised) samples		 	   *
 *	predict():		carry out prediction for all spectral lines	   *
 *	predict_reset():	carry out cyclic predictor reset mechanism	   *
 *				(long blocks) resp. full reset (short blocks)	   *
 *										   *
 *	Internal Functions:							   *
 *	  reset_pred_state():	reset the predictor state variables		   *
 *										   *
 **********************************************************************************/

#include "chandefs.h"
#include "monopred.h"
#include "config.h" /* pred_max_bands() */

/* --- local data --- */
#define GRAD	PRED_ORDER
static	float	ALPHA;
static	float	A;
static	float	B;
static	float	mnt_table[128];
static	float	exp_table[256];

/* --- global data --- */

int *lpflag[Chans];
int *prstflag[Chans];

/* --- private functions --- */

#ifdef	NOT_USED
static void
flt_trunc(float *pf)
{
    unsigned long tmp;
    float *pt = (float *)&tmp;
    *pt = *pf;
    tmp &= (unsigned long)0xffff0000;
    *pf = *pt;
}
#endif

/* this works for all float values, 
 * but does not conform to IEEE conventions of
 * round to nearest, even
 */

static void
flt_round(float *pf)
{
	int exp, sign, rounded;
	float mnt;
	
	/*printf("flt_round(%f)=", *pf);*/
	
	/* decompose mantissa (0.xxxx) and exponent */
	mnt = frexp(*pf, &exp);
		
	/* upscale mantissa to 7 bit accuracy (xxxxxxx.mmm)  */
	mnt = ldexp(mnt, 8);
	
	/* round mantissa (xxxxxxx.0) [towards zero!] */
    rounded = (long) (mnt + ((mnt>0)? 0.5 : -0.5));
    
    /* compose mantissa and exponent to previous scale */
    *pf = ldexp(rounded, exp-8);

    /*printf(" %f\n", *pf);*/

#ifdef NOT_USED
/* old function which didn't work with gcc 4.1.x */

    /* more stable version for clever compilers like gcc 3.x */
    int flg;
    unsigned long tmp, tmp1, tmp2;

    tmp = *(unsigned long*)pf;
    flg = tmp & (unsigned long)0x00008000;
    tmp &= (unsigned long)0xffff0000;
    tmp1 = tmp;
    /* round 1/2 lsb toward infinity */
    if (flg) {
      tmp &= (unsigned long)0xff800000;       /* extract exponent and sign */
      tmp |= (unsigned long)0x00010000;       /* insert 1 lsb */
      tmp2 = tmp;                             /* add 1 lsb and elided one */
      tmp &= (unsigned long)0xff800000;       /* extract exponent and sign */
      
      *pf = *(float*)(void*)&tmp1+*(float*)(void*)&tmp2-*(float*)(void*)&tmp;/* subtract elided one */
    } else {
      *pf = *(float*)(void*)&tmp;
    }
#endif
}


/* This only works for 1.0 < float < 2.0 - 2^-24 !
 * 
 * Comparison of the performance of the two rounding routines:
 *		old (above)	new (below)
 * Max error	0.00385171	0.00179992
 * RMS error	0.00194603	0.00109221
 */

/* New bug fixed version */
static void
inv_table_flt_round(float *ftmp)
     /* new implementation producing the same results for any optimization-level */
{
  int exp,a;
  float tmp;

  frexp((double)*ftmp, &exp);
  tmp = *ftmp * (1<<(8-exp));
  a = (int)tmp;
  if ((tmp-a) >= 0.5) a++;
  if ((tmp-a) == 0.5) a&=-2;
  *ftmp = (float)a/(1<<(8-exp));
}

/* old implementation as described in 13818-7 DCOR1  (25) */
/*{
  int exp;
  double mnt;
  float descale;

  mnt = frexp((double)*ftmp, &exp);
  descale = (float)ldexp(1.0, exp + 15);
  *ftmp += descale;
---> BUG: on x86 with optimization enabled: doesn't round to float!
  *ftmp -= descale;
}*/

/* Bug-fixed versions for clever compilers/optimizers */
static void
make_inv_tables(void)
{
  int i;
  float ftmp, var;
  
  for (i=0; i<128; i++) {
    var = 1.0 + ((float)i/(1<<7));	/* float 1.m, 7 msb only */
    ftmp = B / var;
    inv_table_flt_round(&ftmp);		/* round to 16 bits */
    mnt_table[i] = ftmp;
    /*printf("%3d %f\n", i, ftmp);*/
  }
  
  for (i=0; i<256; i++) {
    var = (float) pow(2,i-127);		/* float 1.0 * 2^exp */
    if (var > MINVAR) {
      ftmp = 1.0 / var;
    } else {
      ftmp = 0;
    }
    exp_table[i] = ftmp;
    /*printf("%3d %g\n", i, ftmp);*/
  }
}

/* Bug-fixed version (big-little endian problem) */
static void
inv_quant_pred_state(TMP_PRED_STATUS *tmp_psp, PRED_STATUS *psp)
{
    unsigned int i;
    short *p2;
    unsigned long *p1_tmp;


    p1_tmp = (unsigned long *)tmp_psp;
    p2 = (short *) psp;
    
    for (i=0; i<MAX_PRED_BINS*(sizeof(*psp)/sizeof(short)); i++)
	    p1_tmp[i] = ((unsigned long)p2[i])<<16;

}

#define FAST_QUANT

/* Bug-fixed version (big-little endian problem) */
static void
quant_pred_state(PRED_STATUS *psp, TMP_PRED_STATUS *tmp_psp)
{
    unsigned int i;
    short *p1;
    unsigned long *p2_tmp;

#ifdef	FAST_QUANT
    p1 = (short *) psp;
    p2_tmp = (unsigned long *)tmp_psp;

    for (i=0; i<MAX_PRED_BINS*(sizeof(*psp)/sizeof(short));i++)
      p1[i] = (short) (p2_tmp[i]>>16);
	    
#else
    int j;
    for (i=0; i<MAX_PRED_BINS; i++) {
  	p1 = (short *) &psp[i];
	  p2_tmp = (unsigned long *)tmp_psp;

  for (j=0; j<6; j++)
	    p1[j] = (short) (p2_tmp[i]>>16);
    }
#endif
}

/* --- public functions --- */


/********************************************************************************
 *** FUNCTION: reset_pred_state()						*
 ***										*
 ***    reset predictor state variables						*
 ***										*
 ********************************************************************************/
void
reset_pred_state(PRED_STATUS *psp)
{
    psp->r[0] = Q_ZERO;
    psp->r[1] = Q_ZERO;
    psp->kor[0] = Q_ZERO;
    psp->kor[1] = Q_ZERO;
    psp->var[0] = Q_ONE;
    psp->var[1] = Q_ONE;
#ifdef LAST_RESET_TEST
    psp->last_reset = 0;
#endif
}


/********************************************************************************
 *** FUNCTION: init_pred_stat()							*
 ***										*
 ***    initialisation of all predictor parameter				*
 ***										*
 ********************************************************************************/
void
init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b)
{
    static int first_time = 1;
    /* Test of parameters */

    if(grad<0 || grad>MAX_PGRAD) {
	PRINT(SE,"\n\n ****** error in routine init_pred_stat ******\n");
	PRINT(SE,"\nwrong predictor order: %d\n",grad);
	PRINT(SE,"range of allowed values: 0 ... %d (=MAX_PGRAD)\n\n",MAX_PGRAD);
	exit(1);
    }
    if(alpha<0 || alpha>=1) {
	PRINT(SE,"\n\n ****** error in routine init_pred_stat ******\n");
	PRINT(SE,"\nwrong time constant alpha: %e\n",alpha);
	PRINT(SE,"range of allowed values: 0 ... 1\n\n");
	exit(1);
    }
    if(a<0 || a>1) {
	PRINT(SE,"\n\n ****** error in routine init_pred_stat ******\n");
	PRINT(SE,"\nwrong attenuation factor a: %e\n",a);
	PRINT(SE,"range of allowed values: 0 ... 1\n\n");
	exit(1);
    }
    if(b<0 || b>1) {
	PRINT(SE,"\n\n ****** error in routine init_pred_stat ******\n");
	PRINT(SE,"\nwrong attenuation factor b: %e\n",b);
	PRINT(SE,"range of allowed values: 0 ... 1\n\n");
	exit(1);
    }

    /* Initialisation */
    if (first_time) {
	ALPHA = alpha;
	A = a;
	B = b;
	make_inv_tables();
	first_time = 0;
    }

    reset_pred_state(psp);
}

/********************************************************************************
 *** FUNCTION: monopred()							*
 ***										*
 ***    calculation of a predicted value from preceeding (quantised) samples	*
 ***	using a second order backward adaptive lattice predictor with full	*
 ***	LMS adaption algorithm for calculation of predictor coefficients	*
 ***										*
 ***    parameters:	pc:	pointer to this quantised sample		*
 ***			psp:	pointer to structure with predictor status	*
 ***			pred_flag:	1 if prediction is used			*
 ***										*
 ********************************************************************************/

static void
monopred(Float *pc, PRED_STATUS *psp, TMP_PRED_STATUS *pst, int pred_flag)
{
    Float qc = *pc;		/* quantized coef */
    float pv;			/* predicted value */
    float dr1;			/* difference in the R-branch */
    float e0,e1;		/* "partial" prediction errors (E-branch) */
    float r0,r1;		/* content of delay elements */
    float k1,k2;		/* predictor coefficients */

    float *R = pst->r;		/* content of delay elements */
    float *KOR = pst->kor;	/* estimates of correlations */
    float *VAR = pst->var;	/* estimates of variances */
    unsigned long tmp;
    int i, j;

/* check that enc and dec track */
#if (0)
{
    static int i=0;
    static int frame=0;
    if (frame != bno) {
        frame=bno;
        i=0;
    }
    printf("P: %6d %6d: %1d %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f\n",
    bno, i, pred_flag, qc, 
    R[0], R[1], KOR[0], KOR[1], VAR[0], VAR[1]);
    fflush(stdout);
    i++;
}    
#endif

    r0=R[0];
    r1=R[1];

    /* Calculation of predictor coefficients to be used for the 
     * calculation of the current predicted value based on previous
     * block's state
     */
     
    /* the test, division and rounding is be pre-computed in the tables 
     * equivalent calculation is:
     * k1 = (VAR[1-1]>MINVAR) ? KOR[1-1]/VAR[1-1]*B : 0.0F;
     * k2 = (VAR[2-1]>MINVAR) ? KOR[2-1]/VAR[2-1]*B : 0.0F;
     */
    tmp = psp->var[1-1];
    j = (tmp >> 7);
    i = tmp & 0x7f;
    k1 = KOR[1-1] * exp_table[j] * mnt_table[i];

    tmp = psp->var[2-1];
    j = (tmp >> 7);
    i = tmp & 0x7f;
    k2 = KOR[2-1] * exp_table[j] * mnt_table[i];
    
    /* Predicted value */
    pv  = k1*r0 + k2*r1;
    flt_round(&pv);
    if (pred_flag)
	*pc = qc + pv;
    /* printf("P1: %8.2f %8.2f\n", pv, *pc);*/

    /* Calculate state for use in next block */
     	
    /* E-Branch:
     *	Calculate the partial prediction errors using the old predictor coefficients
     *	and the old r-values in order to reconstruct the predictor status of the 
     *	previous step
     */

    e0 = *pc;
    e1 = e0-k1*r0;

    /* Difference in the R-Branch:
     *	Calculate the difference in the R-Branch using the old predictor coefficients and
     *	the old partial prediction errors as calculated above in order to reconstruct the
     *	predictor status of the previous step
     */

    dr1 = k1*e0;

    /* Adaption of variances and correlations for predictor coefficients:
     *	These calculations are based on the predictor status of the previous step and give
     *	the new estimates of variances and correlations used for the calculations of the
     *	new predictor coefficients to be used for calculating the current predicted value
     */

    VAR[1-1] = ALPHA*VAR[1-1]+(0.5F)*(r0*r0 + e0*e0);   /* float const */
    KOR[1-1] = ALPHA*KOR[1-1] + r0*e0;
    VAR[2-1] = ALPHA*VAR[2-1]+(0.5F)*(r1*r1 + e1*e1);   /* float const */
    KOR[2-1] = ALPHA*KOR[2-1] + r1*e1;

    /* Summation and delay in the R-Branch => new R-values */

    r1 = A*(r0-dr1);
    r0 = A*e0;

    R[0]=r0;
    R[1]=r1;
}




/********************************************************************************
 *** FUNCTION: predict()							*
 ***										*
 ***    carry out prediction for all allowed spectral lines			*
 ***										*
 ********************************************************************************/


void
predict(Info* info, int profile, int *lpflag, PRED_STATUS *psp, Float *coef)
{
    int j, k, b, to, flag0;
    short *top;

    if (profile != Main_Profile) {
	if (*lpflag == 0) {
	    /* prediction calculations not required */
	    return;
	}
	else {
	    PRINT(SE, "Prediction not allowed in this profile!\n");
	    myexit("profile");
	}
    }
    
    if (info->islong) {
	TMP_PRED_STATUS tmp_ps[MAX_PRED_BINS];
	inv_quant_pred_state(tmp_ps, psp);
	b = 0;
	k = 0;
	top = info->sbk_sfb_top[b];
	flag0 = *lpflag++;
	for (j = 0; j < pred_max_bands(); j++) {
	    to = *top++;
	    if (flag0 && *lpflag++) {
		for ( ; k < to; k++) {
		    monopred(&coef[k], &psp[k], &tmp_ps[k], 1);
		}
	    }
	    else {
		for ( ; k < to; k++) {
                  monopred(&coef[k], &psp[k], &tmp_ps[k], 0);
		}
	    }
	}
	quant_pred_state(psp, tmp_ps);
    }
}





/********************************************************************************
 *** FUNCTION: predict_reset()							*
 ***										*
 ***    carry out cyclic predictor reset mechanism (long blocks)		*
 ***    resp. full reset (short blocks)						*
 ***										*
 ********************************************************************************/
void
predict_reset(Info* info, int *prstflag, PRED_STATUS **psp, 
              int firstCh, int lastCh, int *last_rstgrp_num , int *lpflag)
{
  int j, prstflag0, prstgrp, ch;
  static int warn_flag = 0;
  prstgrp = 0;


  if (info->islong) {

    if (*lpflag == 0)
       return;

    prstflag0 = *prstflag++;
    if (prstflag0) {
      
      /* for loop modified because of bit-reversed group number */
      for (j=0; j<LEN_PRED_RSTGRP-1; j++) {
        prstgrp |= prstflag[j];
        prstgrp <<= 1;
      }
      prstgrp |= prstflag[LEN_PRED_RSTGRP-1];
      
      if (debug['r']) {
        PRINT(SE,"bno: %3ld ",bno);
        PRINT(SE,"PRST: prstgrp: %2d (last_rstgrp: %2d) ", prstgrp, last_rstgrp_num[firstCh]);
        PRINT(SE,"  prstbits:"); 
        for (j=0; j<LEN_PRED_RSTGRP; j++)  
          PRINT(SE,"%d ", prstflag[j]);  
        PRINT(SE,"FIRST: %d LAST %d\n", firstCh, lastCh);
      }
      
      if ( (prstgrp<1) || (prstgrp>30) ) {
        PRINT(SE, "ERROR in prediction reset pattern\n");
        return;
      }
      
      for (ch=firstCh; ch<=lastCh; ch++) {
        /* check if two consecutive reset group numbers are incremented by one 
           (this is a poor check, but we don't have much alternatives) */
        if ((warn_flag) && (last_rstgrp_num[ch] < 31) && (last_rstgrp_num[ch] > 0)) {
          if ((last_rstgrp_num[ch] + 1) != prstgrp) {
            if (last_rstgrp_num[ch] == 30 && prstgrp ==1) {
              /* everything is ok */
            }
            else {
              PRINT(SE,"WARNING: suspicious Predictor Reset sequence (ch %d,last_rstgrp %2d, prstgrp %2d)!\n",ch,last_rstgrp_num[ch],prstgrp);
              /* warn_flag = 0; */
            }
          }
        }

        last_rstgrp_num[ch] = prstgrp;
        for (j=prstgrp-1; j<LN2; j+=30) {
          reset_pred_state(&psp[ch][j]);
        }
      }
    } /* end predictor reset */
  } /* end islong */
  else { /* short blocks */
    /* complete prediction reset in all bins */
    for (ch=firstCh; ch<=lastCh; ch++) {
      last_rstgrp_num[ch] = 0;
      for (j=0; j<LN2; j++) {
        reset_pred_state(&psp[ch][j]);
      }
      if (debug['r']) {
        PRINT(SE,"bno: %3ld ",bno);
        PRINT(SE,"PRST: complete reset due to short blocks  ");
        for (j=0; j<LEN_PRED_RSTGRP; j++)  
          PRINT(SE,"%d ", prstflag[j]);  
        PRINT(SE,"FIRST: %d LAST %d\n", firstCh, lastCh);
      }
    }
  }
#ifdef LAST_RESET_TEST
  for (ch=firstCh; ch<=lastCh; ch++) {
    for (j=0; j<LN2; j++) {
      /*         if (ch == 2 && j == 0) { */
      if (psp[ch][j].last_reset>=240) { 
        PRINT(SE,"WARNING: predictor reset error! bno: %3ld, nch: %d, j: %4d, lastReset: %3d\n",bno,ch,j,psp[ch][j].last_reset );
      }
      (psp[ch][j].last_reset)++;
      if (debug['r'] && j<30)
          PRINT (SE,"lastReset[%i][%i]: %i   -  bno: %3ld\n",ch,j,psp[ch][j].last_reset,bno );
    } 
  } 
#endif
}
