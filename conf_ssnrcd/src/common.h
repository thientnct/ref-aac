
#ifndef COMMON_H
#define COMMON_H

/*

====================================================================
                        Copyright Header            
====================================================================

This software module was originally developed by Ralf Funken
(Philips, Eindhoven, The Netherlands) and edited by Takehiro Moriya 
(NTT Labs. Tokyo, Japan) in the course of development of the
MPEG-2/MPEG-4 Conformance ISO/IEC 13818-4, 14496-4. This software
module is an implementation of one or more of the Audio Conformance
tools as specified by MPEG-2/MPEG-4 Conformance. ISO/IEC gives users
of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC 13818-3,7, 14496-3) free
license to this software module or modifications thereof for use in
hardware or software products. Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Philips retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party. This copyright notice must be included in 
all copies or derivative works. Copyright 2000.


Source file: COMMON.H

*/

#include "libtsp.h"
#include "libtsp/AFpar.h"


/*****************************************************************************/

/*     MPEG-4 CELP Conformance criteria   */

#define         ABSDIF_THRESHOLD(K)             ((1.0 / (double) (1 << ((K)-2))))                           /*  2^-(K-2)                            */
#define         RMS_THRESHOLD(K)                (20.0 * log10 (1 / ((double)(1 << ((K)-1)) * sqrt(12))))    /*  20*log10((2^(-(K-1))) / sqrt(12))   */

#define         SEGMENTAL_SNR_THRESHOLD         (30.0)        /*  in dB  */
#define         CEPSTRAL_DISTORTION_THRESHOLD   (1.0)         /*  in dB  */ 

/*****************************************************************************/

/*     Defines for applying conformance criteria    */

#define         RMS_ABSMAXDIFSAMPLE_CRITERION   (0x01)
#define         SEGSNR_CD_CRITERION             (0x02)
#define         NR_BITS_ACCURACY_CRITERION      (0x04)
                                                
/*     general constants    */             

#define         PROGRAM_NAME                    "SSNRCD - Segmental SNR / Cepstral Distortion Analysis Tool"
#define         EXECUTABLE_NAME_1               "ssnrcd"
#define         EXECUTABLE_NAME_2               "mpeg4conf"
                                                 
#define         TRUE                            (1)
#define         FALSE                           (0)
                                                 
#define         SSNRCD_VERSION                  "2.0"
#define         MAX_STRING_LENGTH               (256)
#define         MAX_PARAMETER_SETS              (10)
#define         MAX_RESULT_LINES                (6)
                              
#define         PI                              (3.141592653589793F)
#define         PCM_SCALEFACTOR                 (32768.0)
#define         MAX_PCM_CHANNELS                (64)
#define         DEFAULT_SEGMENT_LENGTH          (320)   /* default segment length in samples                        */
#define         SAMPLES_PER_BLOCK               (1)
#define         DEFAULT_DELAY                   (0)     /* default delay (offset between input files) in samples    */
#define         DEFAULT_CRITERIA                (0)     /* default criteria                                         */
#define         DEFAULT_RESOLUTION              (16)    /* default resolution in bits                               */
#define         MINIMUM_RESOLUTION              (12)
#define         MAXIMUM_RESOLUTION              (24)
                                                 
#define         INITIAL_RMS_dB                  (-999.9)
#define         THRESHOLD_RMS_dB                (-999.0)
#define         INITIAL_SNR_dB                  (-999.9)
#define         THRESHOLD_SNR_dB                (-999.0)
                                                 
#define         GREP_STRING                     "==="
#define         SEGSNR_MIN_NOISE                (1e-13 * segment_length)
                                                
#define         SSNR_CD_LOWERBOUND              (-50.0)  /*  in dB  */
#define         SSNR_CD_UPPERBOUND              (-15.0)  /*  in dB  */

/*  cepstrum analysis related defines           */
                                                
#define         N_PR                            (16)          /*    LPC order                                       */
#define         BW                              (0.0125F)     /*    Bandwidth scalefactor                           */
#define         COR_FACTOR                      (0.99999)     /*    Correction factor to solve numerical            */
                                                              /*    instability for low rank matrices               */


/*  macros for min and max                      */

#ifndef MIN
#define         MIN(a,b)                        (((a) < (b)) ? (a) : (b))
#endif  /* MIN */

#ifndef MAX
#define         MAX(a,b)                        (((a) > (b)) ? (a) : (b))
#endif  /* MAX */

/*  other macros                                */

#ifndef LD /* logarithmus dualis */
#define         LD(a)                           log10(a)/log10(2)
#endif

/*****************************************************************************/

typedef char   string [MAX_STRING_LENGTH];


/*****************************************************************************/
/*****************************************************************************/

/*          PNS Conformance                     */

#define         Float               double
#define         SFB_1               51
#define         SFB_2               15
#define         REP_STRING          "==="

#define         LEN_SAMP_IDX        4

#define         PNS_S_C1            0.4
#define         PNS_S_C2            0.8
#define         PNS_T_C1            91.0
#define         PNS_T_C2            99.0


/*=== spectral PNS analysis ===*/
typedef enum {
  FRAME_SIZE_INVALID     =  0,

  FRAME_SIZE_1024_LONG   =  2048,
  FRAME_SIZE_1024_SHORT  =  256,
    
  /* for AAC, frame size 960 */
  FRAME_SIZE_960_LONG    =  1920,
  FRAME_SIZE_960_SHORT   =  240,
  
  /* for AAC LD, frame size 512 */
  FRAME_SIZE_512_LONG    =  1024,
  
  /* for AAC LD, frame size 480 */
  FRAME_SIZE_480_LONG    =  960  /* for future use (LD) */
} FRAME_SIZE;


/*=== temporal PNS analysis ===*/
#define         BLOCK_LENGTH        64






/***************************************************************************/
/*    Structs   */


typedef struct
{
    long int                channels;
    float                   samplefreq;
    long int                nrofsamples;
    int                     bits_per_sample;
    string                  input_filename;
    AFILE*                  stream;
} audiofile;


/***************************************************************************/

/* critical_ratio for pns-test   */
/* todo: where to include this */
typedef struct 
{
    Float                   meanRatio;
    Float                   standardDeviationRatio;
} critical_ratio;


/***************************************************************************/

#endif    /*    COMMON_H    */


