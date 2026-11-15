/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  -

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

#define d2PI  6.283185307179586

#define DEBUG 0
#define SWAP(a,b) tempr=a;a=b;b=tempr   
/* MDCT.cpp - Implementation file for fast MDCT transform */
/* Note: Defines Transform() and ITransform() for compatibility w/
   header files in previous versions. Replace previous Transfo.c file! */

#include "intrins.h"
#include "transfo.h"

#define PI 3.141592653589795

void CompFFT (Float *data, int nn, int isign); /* Function prototype */
void MDCT (Float *data, int N, int b, int isign); /* Function prototype */

/* In-place forward MDCT transform */

void Transform(Float *data, int N, int b) {
  MDCT (data, N, b, 1);
}

/* In-place inverse MDCT transform */
/* Note: 2/N factor ! */
void ITransform (Float *data, int N, int b) {
  MDCT (data, N, b, -1);

}

static int oldp1 = 0;
static Float **intermed = 0;
static Float *FFTarray = 0;    /* the array for in-place FFT */

void destroyTransfo()
{
  if (FFTarray) free1(FFTarray);
  if (intermed) {
    for (oldp1--;oldp1>=0;oldp1--) free1(intermed[oldp1]);
    free1(intermed);
  }
}

#if 1

/*****************************
  Fast MDCT Code
*****************************/

void MDCT (Float *data, int N, int b, int isign) {

  static Float tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
  Float freq = 2. * PI / N;
  static Float fac;
  static int i, n;
  int a = N - b;

  /* Choosing to allocate 2/N factor to Inverse Xform! */
  if (isign == 1) {
    fac = 2.; /* 2 from MDCT inverse  to forward */
  } 
  else {
    fac = 2. / N; /* remaining 2/N from 4/N IFFT factor */
  }


  {
    static int oldN = 0;
    if (N > oldN) {
      if (FFTarray) {
        free1 (FFTarray);
        FFTarray = 0;
      }
      oldN = N;
    }
        
    if (! FFTarray) {
      Float *temp;
      FFTarray = (Float *) mal1 (N/2 * sizeof (Float));
    }
  }

  /* prepare for recurrence relation in pre-twiddle */
  cfreq = cos (freq);
  sfreq = sin (freq);
  c = cos (freq * 0.125);
  s = sin (freq * 0.125);

  for (i = 0; i < N / 4; i++) {

    /* calculate real and imaginary parts of g(n) or G(p) */

    if (isign == 1) { /* Forward Transform */
      n = N / 2 - 1 - 2 * i;
      if (i < b / 4) {
        tempr = data [a / 2 + n] + data [N + a / 2 - 1 - n]; /* use second form of e(n) for
                                                                n = N / 2 - 1 - 2i */
      } 
      else {
        tempr = data [a / 2 + n] - data [a / 2 - 1 - n]; /* use first form of e(n) for
                                                            n = N / 2 - 1 - 2i */
      }
      n = 2 * i;
      if (i < a / 4) {
        tempi = data [a / 2 + n] - data [a / 2 - 1 - n]; /* use first form of e(n) for n=2i */
      } 
      else {
        tempi = data [a / 2 + n] + data [N + a / 2 - 1 - n]; /* use second form of e(n) for n=2i*/
      }
    } 
    else { /* Inverse Transform */
      tempr = -data [2 * i];
      tempi = data [N / 2 - 1 - 2 * i];
    }

    /* calculate pre-twiddled FFT input */

    FFTarray [2 * i] = tempr * c + isign * tempi * s;
    FFTarray [2 * i + 1] = tempi * c - isign * tempr * s;

    /* use recurrence to prepare cosine and sine for next value of i */
    cold = c;
    c = c * cfreq - s * sfreq;
    s = s * cfreq + cold * sfreq;
  }

  /* Perform in-place complex FFT (or IFFT) of length N/4 */
  /* Note: FFT has physics (opposite) sign convention and doesn't do 1/N factor */
  CompFFT (FFTarray, N / 4, -isign);

  /* prepare for recurrence relations in post-twiddle */
  c = cos (freq * 0.125);
  s = sin (freq * 0.125);

  /* post-twiddle FFT output and then get output data */
  for (i = 0; i < N / 4; i++) {

    /* get post-twiddled FFT output  */
    /* Note: fac allocates 4/N factor from IFFT to forward and inverse */
    tempr = fac * (FFTarray [2 * i] * c + isign * FFTarray [2 * i + 1] * s);
    tempi = fac * (FFTarray [2 * i + 1] * c - isign * FFTarray [2 * i] * s);

    /* fill in output values */
    if (isign == 1) { /* Forward Transform */
      data [2 * i] = -tempr;   /* first half even */
      data [N / 2 - 1 - 2 * i] = tempi;  /* first half odd */
      data [N / 2 + 2 * i] = -tempi;  /* second half even */
      data [N - 1 - 2 * i] = tempr;  /* second half odd */
    } 
    else { /* Inverse Transform */
      data [N / 2 + a / 2 - 1 - 2 * i] = tempr;
      if (i < b / 4) {
        data [N / 2 + a / 2 + 2 * i] = tempr;
      } 
      else {
        data [2 * i - b / 2] = -tempr;
      }
      data [a / 2 + 2 * i] = tempi;
      if (i < a / 4) {
        data [a / 2 - 1 - 2 * i] = -tempi;
      } 
      else {
        data [a / 2 + N - 1 - 2*i] = tempi;
      }
    }

    /* use recurrence to prepare cosine and sine for next value of i */

    cold = c;
    c = c * cfreq - s * sfreq;
    s = s * cfreq + cold * sfreq;
  }
/*   free1 (FFTarray); */
}

#else
/*****************************
 (Very) Slow MDCT Code
*****************************/

void MDCT(double* data, int N, int b, int isign){
  /* Very slow implementation for brute force testing */
  /* Note: 2/N factor allocated to forward Xform */

  double phi = 2.*PI/N;
  double no = 0.5*(b+1);
  double fac, temp;
  double * outData;
  int i,j;
  int a = N-b;

  outData = NewDouble(N);

  fac = 2./N;

  if (isign==1) { /* Forward */
    for (i=0;i<N;i++) {
      temp=0;
      for (j=0;j<N;j++) {
        temp += data[j]*cos(phi*(i+0.5)*(j+no));
      }
      outData[i] = fac*temp;
    }
  } else { /* Inverse */
    for (j=0;j<N;j++) {
      temp=0;
      for (i=0;i<N;i++) {
        temp += data[i]*cos(phi*(i+0.5)*(j+no));
      }
      outData[j] = temp;
    }
  }
  for (i=0;i<N;i++) data[i] = outData[i];
  DeleteDouble(outData);
}

#endif


void CompFFT (Float *data, int nn, int isign) {

  static int i, j, k, kk;
  static int p1, q1;
  static int m, n, logq1;
  static Float ar, ai;
  static Float d2pn;
  static Float ca, sa, curcos, cursin, oldcos, oldsin;
  static Float ca1, sa1, curcos1, cursin1, oldcos1, oldsin1;


  /* Factorize n;  n = p1*q1 where q1 is a power of 2.
     For n = 1152, p1 = 9, q1 = 128.             */

  n = nn;
  logq1 = 0;

  for (;;) {
    m = n >> 1;  /* shift right by one*/
    if ((m << 1) == n) {
      logq1++;
      n = m;
    } 
    else {
      break;
    }
  }

  p1 = n;
  q1 = 1;
  q1 <<= logq1;

  d2pn = d2PI / nn;

  {
    static int oldq1 = 0;
    
    if ((oldp1 < p1) || (oldq1 < q1)) {
      if (intermed) {
        free1 (intermed);
        intermed = 0;
      }
      if (oldp1 < p1) oldp1 = p1;
      if (oldq1 < q1) oldq1 = q1;;
    }
                
    if (!intermed) {
      typedef Float *pFloat;
      int N = oldp1;
      int M = 2 * oldq1;
      /* allocate N pointers to Float arrays */
      intermed = (pFloat *) mal1 (N * sizeof (pFloat));

      /* Allocate a Float array M long for each of the N array pointers */
      for (i = 0; i < N; i++) {
        intermed [i] = (Float *) mal1 (M * sizeof (Float));
      }
    }
  }

  /* Sort the p1 sequences */

  for (i = 0; i < p1; i++) {
    for (j = 0; j < q1; j++){
      intermed [i] [2 * j] = data [2 * (p1 * j + i)];
      intermed [i] [2 * j + 1] = data [2 * (p1 * j + i) + 1];
    }
  }

  /* compute the power of two fft of the p1 sequences of length q1 */

  for (i = 0; i < p1; i++) {
    /* Forward FFT in place for n complex items */
    FFT (intermed [i], q1, isign);
  }

  /* combine the FFT results into one sequence of length N */

  ca1 = cos (d2pn);
  sa1 = sin (d2pn);
  curcos1 = 1.;
  cursin1 = 0.;

  for (k = 0; k < nn; k++) {
    data [2 * k] = 0.;
    data [2 * k + 1] = 0.;
    kk = k % q1;

    ca = curcos1;
    sa = cursin1;
    curcos = 1.;
    cursin = 0.;

    for (j = 0; j < p1; j++) {
      ar = curcos;
      ai = isign * cursin;
      data [2 * k] += intermed [j] [2 * kk] * ar - intermed [j] [2 * kk + 1] * ai;
      data [2 * k + 1] += intermed [j] [2 * kk] * ai + intermed [j] [2 * kk + 1] * ar;

      oldcos = curcos;
      oldsin = cursin;
      curcos = oldcos * ca - oldsin * sa;
      cursin = oldcos * sa + oldsin * ca;
    }
    oldcos1 = curcos1;
    oldsin1 = cursin1;
    curcos1 = oldcos1 * ca1 - oldsin1 * sa1;
    cursin1 = oldcos1 * sa1 + oldsin1 * ca1;
  }
/*   free1 (intermed); */
}


void FFT (Float *data, int nn, int isign)  {
  /* Varient of Numerical Recipes code from off the internet.  It takes nn
     interleaved complex input data samples in the array data and returns nn interleaved
     complex data samples in place where the output is the FFT of input if isign==1 and it
     is nn times the IFFT of the input if isign==-1. 
(Note: it doesn't renormalize by 1/N when doing the inverse transform!!!)
(Note: this follows physicists convention of +i, not EE of -j in forward
transform!!!!)
 */

/* Press, Flannery, Teukolsky, Vettering "Numerical
 * Recipes in C" tuned up ; Code works only when nn is
 * a power of 2  */

    static int n, mmax, m, j, i;
    static Float wtemp, wr, wpr, wpi, wi, theta, wpin;
    static Float tempr, tempi, datar, datai;
    static Float data1r, data1i;

    n = nn * 2;

/* bit reversal */

    j = 0;
    for (i = 0; i < n; i += 2) {
                if (j > i) {  /* could use j>i+1 to help compiler analysis */
                        SWAP (data [j], data [i]);
                        SWAP (data [j + 1], data [i + 1]);
                        }
                m = nn;
                while (m >= 2 && j >= m) {
                        j -= m;
                        m >>= 1;
                        }
                j += m;
                }

    theta = 3.141592653589795 * .5;
    if (isign < 0)
                theta = -theta;
    wpin = 0;   /* sin(+-PI) */
    for (mmax = 2; n > mmax; mmax *= 2) {
                wpi = wpin;
                wpin = sin (theta);
                wpr = 1 - wpin * wpin - wpin * wpin; /* cos(theta*2) */
                theta *= .5;
                wr = 1;
                wi = 0;
                for (m = 0; m < mmax; m += 2) {
                        j = m + mmax;
                        tempr = (Float) wr * (data1r = data [j]);
                        tempi = (Float) wi * (data1i = data [j + 1]);
                        for (i = m; i < n - mmax * 2; i += mmax * 2) {
                          /* mixed precision not significantly more
                           * accurate here; if removing float casts,
                           * tempr and tempi should be double */
                          tempr -= tempi;
                          tempi = (Float) wr *data1i + (Float) wi *data1r;
                          /* don't expect compiler to analyze j > i + 1 */
                          data1r = data [j + mmax * 2];
                          data1i = data [j + mmax * 2 + 1];
                          data [i] = (datar = data [i]) + tempr;
                          data [i + 1] = (datai = data [i + 1]) + tempi;
                          data [j] = datar - tempr;
                          data [j + 1] = datai - tempi;
                          tempr = (Float) wr *data1r;
                          tempi = (Float) wi *data1i;
                          j += mmax * 2;
                        }
                        tempr -= tempi;
                        tempi = (Float) wr * data1i + (Float) wi *data1r;
                        data [i] = (datar = data [i]) + tempr;
                        data [i + 1] = (datai = data [i + 1]) + tempi;
                        data [j] = datar - tempr;
                        data [j + 1] = datai - tempi;
                        wr = (wtemp = wr) * wpr - wi * wpi;
                        wi = wtemp * wpi + wi * wpr;
                }
    }
}
