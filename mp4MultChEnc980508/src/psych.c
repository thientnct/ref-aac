/**********************************************************************
MPEG-4 Audio VM



This software module was originally developed by

Fraunhofer Gesellschaft IIS / University of Erlangen (UER

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.

-----
This software module was modified by

Tadashi Araki (Ricoh Company, ltd.)
Tatsuya Okada (Waseda Univ.)
Itaru Kaneko (Graphics Communication Laboratories)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3.

Almost all part of the function EncTf_psycho_acoustic() is made by
T. Araki and T. Okada and its copyright belongs to Ricoh.
The function psy_get_absthr() is made by I. Kaneko
and its copyright belongs to Graphics Communication Laboratories.

Copyright (c) 1997.


Source file: 

$Id: psych.c,v 1.3 1998/01/21 00:28:33 lueck Exp $
$Id: psych.c,v 1.3 1998/01/21 00:28:33 lueck Exp $
$Id: psych.c,v 1.3 1998/01/21 00:28:33 lueck Exp $

**********************************************************************/

/* CREATED BY :  Bernhard Grill -- August-96  */
#include <math.h>

#include "tf_main.h"
#include "psych.h"
#include "common.h"

/* dummy psychoacoustic module */
/*
  static int sfb_width_long[36+1] = { 
        4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6, 
        6,  8,  8,  8, 10, 10, 12, 12, 14, 16, 18, 20, 
       22, 24, 26, 28, 32, 36, 40, 44, 48, 54, 62, 70, 342
    };
    static int sfb_width_short[12+1] = {
        4,  4,  4,  4,  4,  6,  6,  8,  8, 10, 12, 16, 42
    };
    static double fixed_ratio_long[36+1] = { 
        0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.05, 0.05,
        0.05, 0.06, 0.06, 0.06, 0.09, 0.09, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15,
        0.15, 0.15, 0.20, 0.20, 0.20, 0.20, 0.20, 0.25, 0.30, 0.30, 0.30, 0.40, 
        1.00
    };
*/

 

static SR_INFO sr_info_aac[MAX_SAMPLING_RATES+1] =
{
  { 8000  },
  { 11025 },
  { 12000 },
  { 16000 },

/* added by T. Araki (1997.10.16) */
  { 22050, 47, 15,
     { /*  cb_width_long[NSFB_LONG] */
      4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8,
      8,  8,  8,  12, 12, 12, 12, 16, 16, 16, 20, 20, 24, 24, 28, 28, 32,
      36, 36, 40, 44, 48, 52, 52, 64, 64, 64, 64, 64
     },
     { /* cb_width_short[NSFB_SHORT] */
      4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8, 12, 16, 16, 20
     }
  },
/* added by T. Araki (1997.10.16) end */

  { 24000 },
  { 32000 },

/* added by T. Araki (1997.07.09) */
  { 44100, 49, 14,
     { /*  cb_width_long[NSFB_LONG] */
      4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8, 
      12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28, 28, 32, 32, 32, 32, 32, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 96 
     }, 
     { /* cb_width_short[NSFB_SHORT] */
      4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16 
     },
     { /* fixed_ratio_long[NSFB_LONG] */
      0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 
      0.04, 0.04, 0.05, 0.05, 0.05, 0.05, 0.05, 0.06, 0.06, 0.06, 0.06, 0.09, 
      0.09, 0.09, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 
      0.20, 0.20, 0.20, 0.20, 0.20, 0.20, 0.20, 0.25, 0.30, 0.30, 0.30, 0.40,
      1.00
     },
     { /* fixed_ratio_short[NSFB_SHORT] */
      0.04, 0.04, 0.04, 0.05, 0.07, 0.10, 0.12, 0.15, 0.15, 0.20, 0.25, 0.30, 0.40, 1.00
     }
/* added by T. Araki (1997.07.09) end */

  },
  { 48000, 49, 14,
     { /*  cb_width_long[NSFB_LONG] */
      4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8, 
      12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28, 28, 32, 32, 32, 32, 32, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 96 
     }, 
     { /* cb_width_short[NSFB_SHORT] */
      4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16 
     },
     { /* fixed_ratio_long[NSFB_LONG] */
      0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 
      0.04, 0.04, 0.05, 0.05, 0.05, 0.05, 0.05, 0.06, 0.06, 0.06, 0.06, 0.09, 
      0.09, 0.09, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 
      0.20, 0.20, 0.20, 0.20, 0.20, 0.20, 0.20, 0.25, 0.30, 0.30, 0.30, 0.40,
      1.00
     },
     { /* fixed_ratio_short[NSFB_SHORT] */
      0.04, 0.04, 0.04, 0.05, 0.07, 0.10, 0.12, 0.15, 0.15, 0.20, 0.25, 0.30, 0.40, 1.00
     }
  },
  { 96000 },
  { -1 }
};
static SR_INFO sr_info_960[MAX_SAMPLING_RATES+1] =
{
  { 8000  },
  { 11025 },
  { 12000 },
  { 16000 },
  { 22050 },
  { 24000 },
  { 32000 },
  { 44100 },
  { 48000, 49, 14,
     { /*  cb_width_long[NSFB_LONG] */
       4,4,4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28, 28, 32, 32, 32,
     32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
       
     }, 
     { /* cb_width_short[NSFB_SHORT] */
       4,4,4, 4, 4, 8, 8, 8, 12,12,12,16,16, 8
     },
     { /* fixed_ratio_long[NSFB_LONG] */
      0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 
      0.04, 0.04, 0.05, 0.05, 0.05, 0.05, 0.05, 0.06, 0.06, 0.06, 0.06, 0.09, 
      0.09, 0.09, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 
      0.20, 0.20, 0.20, 0.20, 0.20, 0.20, 0.20, 0.25, 0.30, 0.30, 0.30, 0.40,
      1.00
     },
     { /* fixed_ratio_short[NSFB_SHORT] */
      0.04, 0.04, 0.04, 0.05, 0.07, 0.10, 0.12, 0.15, 0.15, 0.20, 0.25, 0.30, 0.40, 1.00
     }
  },
  { 96000 },
  { -1 }
};


static SR_INFO sr_info_NOT_aac[MAX_SAMPLING_RATES+1] =
{
  { 8000  },
  { 11025 },
  { 12000 },
  { 16000 },
  { 22050 },
  { 24000 },
  { 32000 },
  { 44100 },
  { 48000, 37, 13,
    { /*  cb_width_long[NSFB_LONG] */
      4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6, 
      6,  8,  8,  8, 10, 10, 12, 12, 14, 16, 18, 20, 
      22, 24, 26, 28, 32, 36, 40, 44, 48, 54, 62, 70, 342
    },
    { /* cb_width_short[NSFB_SHORT] */
      4,  4,  4,  4,  4,  6,  6,  8,  8, 10, 12, 16, 42
    },
    { /* fixed_ratio_long[NSFB_LONG] */
      0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.04, 0.05, 0.05,
      0.05, 0.06, 0.06, 0.06, 0.09, 0.09, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15,
      0.15, 0.15, 0.20, 0.20, 0.20, 0.20, 0.20, 0.25, 0.30, 0.30, 0.30, 0.40, 
      1.00
    },
    { /* fixed_ratio_short[NSFB_SHORT] */
      0.04, 0.04, 0.04, 0.05, 0.07, 0.10, 0.12, 0.15, 0.15, 0.20, 0.25, 0.30, 
      0.40, 1.00
    }
  },
  { 96000 },
  { -1 }
};

/* added by T. Araki (1997.07.10) */
static PARTITION_TABLE_LONG  part_tbl_long_all[MAX_SAMPLING_RATES+1] =
{
  { 8000  },
  { 11025 },
  { 12000 },
  { 16000 },
 
/* added by T. Araki (1997.10.16) */
  { 22050, 63,
     { /* w_low */
      0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68,
      72, 77, 82, 87, 92, 97, 102, 108, 114, 120, 126, 133, 140, 147, 155,
      163, 172, 181, 191, 201, 212, 224, 237, 251, 266, 282, 299, 318, 338,
      360, 383, 408, 435, 464, 495, 528, 564, 602, 643, 687, 734, 785, 840,
      899, 963
     },
     {/* w_high */
      3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47, 51, 55, 59, 63, 67, 71,
      76, 81, 86, 91, 96, 101, 107, 113, 119, 125, 132, 139, 146, 154, 162,
      171, 180, 190, 200, 211, 223, 236, 250, 265, 281, 298, 317, 337, 359,
      382, 407, 434, 463, 494, 527, 563, 601, 642, 686, 733, 784, 839, 898,
      962, 1023
     },
     { /* width */
      4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
      6, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 13, 14, 15, 16, 17,
      19, 20, 22, 23, 25, 27, 29, 31, 33, 36, 38, 41, 44, 47, 51, 55, 59,
      64, 61
     },
     { /* bval */
      0.22, 0.65, 1.09, 1.52, 1.95, 2.37, 2.79, 3.21, 3.62, 4.02, 4.41, 4.80,
      5.18, 5.55, 5.92, 6.27, 6.62, 6.95, 7.32, 7.71, 8.10, 8.46, 8.82, 9.16,
      9.52, 9.89, 10.25, 10.59, 10.95, 11.31, 11.65, 12.00, 12.35, 12.70,
      13.05, 13.40, 13.74, 14.07, 14.41, 14.76, 15.11, 15.46, 15.80, 16.14,
      16.48, 16.82, 17.16, 17.50, 17.84, 18.17, 18.51, 18.84, 19.17, 19.51,
      19.84, 20.17, 20.50, 20.84, 21.17, 21.50, 21.84, 22.17, 22.48
     },
     { /* qsthr */
      43.30, 43.30, 38.30, 38.30, 38.30, 35.30, 35.30, 30.30, 30.30, 28.30,
      28.30, 28.30, 28.30, 28.30, 28.30, 28.30, 28.30, 28.30, 29.27, 29.27,
      29.27, 29.27, 29.27, 29.27, 30.06, 30.06, 30.06, 30.06, 30.73, 30.73,
      30.73, 31.31, 31.31, 31.82, 31.82, 32.28, 32.28, 32.69, 33.07, 33.42,
      33.74, 34.04, 34.32, 34.58, 35.06, 35.29, 35.70, 35.89, 36.26, 36.59,
      36.90, 37.19, 37.46, 37.84, 38.07, 41.40, 41.71, 42.00, 44.35, 44.68,
      44.98, 50.34, 50.13
     }
  },
/* added by T. Araki (1997.10.16) end */

  { 24000 },
  { 32000 },
  { 44100, 70, 
     { /* w_low */
      0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
      39, 42, 45, 48, 51, 54, 57, 60, 63, 67, 71, 75, 79, 83, 88, 93, 98,
      104, 110, 117, 124, 132, 140, 149, 158, 168, 179, 191, 204, 218, 233,
      249, 266, 284, 304, 325, 348, 372, 398, 426, 456, 489, 525, 564, 607,
      654, 707, 766, 833, 909, 997
     },
     { /* w_high */
      1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 38, 
      41, 44, 47, 50, 53, 56, 59, 62, 66, 70, 74, 78, 82, 87, 92, 97, 103,
      109, 116, 123, 131, 139, 148, 157, 167, 178, 190, 203, 217, 232, 248,
      265, 283, 303, 324, 347, 371, 397, 425, 455, 488, 524, 563, 606, 653,
      706, 765, 832, 908, 996, 1023
     },
     { /* width */
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 
      3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11,
      12, 13, 14, 15, 16, 17, 18, 20, 21, 23, 24, 26, 28, 30, 33, 36, 39,
      43, 47, 53, 59, 67, 76, 88, 27
     },
     { /* bval */
      0.22, 0.65, 1.09, 1.52, 1.95, 2.37, 2.79, 3.21, 3.62, 4.02, 4.41, 4.80,
      5.18, 5.55, 5.92, 6.27, 6.62, 6.95, 7.36, 7.83, 8.28, 8.71, 9.12, 9.52,
      9.89, 10.25, 10.59, 10.97, 11.38, 11.77, 12.13, 12.48, 12.84, 13.22,
      13.57, 13.93, 14.30, 14.67, 15.03, 15.40, 15.76, 16.11, 16.45, 16.79,
      17.13, 17.48, 17.83, 18.18, 18.52, 18.87, 19.21, 19.54, 19.88, 20.22,
      20.56, 20.90, 21.24, 21.57, 21.91, 22.24, 22.58, 22.91, 23.25, 23.58,
      23.91, 24.00, 24.00, 24.00, 24.00, 24.00
     },
     { /* qsthr */
      40.29, 40.29, 35.29, 35.29, 35.29, 32.29, 32.29, 27.29, 27.29, 25.29,
      25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 27.05, 27.05,
      27.05, 27.05, 27.05, 27.05, 27.05, 27.05, 27.05, 28.30, 28.30, 28.30,
      28.30, 28.30, 29.27, 29.27, 29.27, 30.06, 30.06, 30.73, 30.73, 31.31,
      31.31, 31.82, 31.82, 32.28, 32.69, 33.07, 33.42, 33.74, 34.04, 34.32,
      34.58, 34.83, 35.29, 38.50, 38.89, 39.08, 41.43, 41.75, 42.05, 47.46,
      47.84, 48.19, 58.61, 59.00, 59.52, 69.98, 70.54, 71.08, 71.72, 72.09
     }
  },
  { 48000, 69,
     { /* w_low */
      0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
      38, 41, 44, 47, 50, 53, 56, 59, 62, 66, 70, 74, 78, 82, 87, 92, 97,
      103, 109, 116, 123, 131, 139, 148, 158, 168, 179, 191, 204, 218, 233,
      249, 266, 284, 304, 325, 348, 372, 398, 426, 457, 491, 528, 568, 613,
      663, 719, 782, 854, 938
     },
     { /* w_high */
      1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37,
      40, 43, 46, 49, 52, 55, 58, 61, 65, 69, 73, 77, 81, 86, 91, 96, 102,
      108, 115, 122, 130, 138, 147, 157, 167, 178, 190, 203, 217, 232, 248,
      265, 283, 303, 324, 347, 371, 397, 425, 456, 490, 527, 567, 612, 662,
      718, 781, 853, 937, 1023
     }, 
     { /* width */
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
      3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 10, 10, 11, 12,
      13, 14, 15, 16, 17, 18, 20, 21, 23, 24, 26, 28, 31, 34, 37, 40, 45, 50,
      56, 63, 72, 84, 86
     },
     { /* bval */
      0.24, 0.71, 1.18, 1.65, 2.12, 2.58, 3.03, 3.48, 3.92, 4.35, 4.77, 5.19,
      5.59, 5.99, 6.37, 6.74, 7.10, 7.45, 7.80, 8.20, 8.68, 9.13, 9.55, 9.96,
      10.35, 10.71, 11.06, 11.45, 11.86, 12.25, 12.62, 12.96, 13.32, 13.70,
      14.05, 14.41, 14.77, 15.13, 15.49, 15.85, 16.20, 16.55, 16.91, 17.25,
      17.59, 17.93, 18.28, 18.62, 18.96, 19.30, 19.64, 19.97, 20.31, 20.65,
      20.99, 21.33, 21.66, 21.99, 22.32, 22.66, 23.00, 23.33, 23.67, 24.00,
      24.00, 24.00, 24.00, 24.00, 24.00
     },
     { /* qsthr */
      40.29, 40.29, 35.29, 35.29, 32.29, 32.29, 27.29, 27.29, 27.29, 25.29,
      25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 25.29, 27.05,
      27.05, 27.05, 27.05, 27.05, 27.05, 27.05, 27.05, 28.30, 28.30, 28.30,
      28.30, 28.30, 29.27, 29.27, 29.27, 30.06, 30.06, 30.73, 30.73, 31.31,
      31.31, 31.82, 32.28, 32.28, 32.69, 33.07, 33.42, 33.74, 34.04, 34.32,
      34.58, 34.83, 38.29, 38.50, 38.89, 41.08, 41.43, 41.75, 47.19, 47.59,
      47.96, 58.30, 58.81, 69.27, 69.76, 70.27, 70.85, 71.52, 70.20
     }
  },
  { 96000 },
  { -1 }
};

static PARTITION_TABLE_SHORT  part_tbl_short_all[MAX_SAMPLING_RATES+1] =
{
  { 8000  },
  { 11025 },
  { 12000 },
  { 16000 },

/* added by T. Araki (1997.10.16) */
  { 22050, 46,
     { /* w_low */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
      20, 22, 24, 26, 28, 30, 32, 34, 36, 39, 42, 45, 48, 52, 56, 60, 64, 69,
      74, 79, 85, 91, 98, 105, 113, 121
     },
     { /* w_high */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
      21, 23, 25, 27, 29, 31, 33, 35, 38, 41, 44, 47, 51, 55, 59, 63, 68, 73,
      78, 84, 90, 97, 104, 112, 120, 127
     },
     { /* width */
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
      2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 7
     },
     { /* bval */
      0.00, 0.87, 1.73, 2.58, 3.41, 4.22, 4.99, 5.74, 6.45, 7.12, 7.75, 8.36,
      8.92, 9.45, 9.96, 10.43, 10.87, 11.29, 11.68, 12.05, 12.71, 13.32,
      13.86, 14.35, 14.80, 15.21, 15.58, 15.93, 16.32, 16.75, 17.15, 17.51,
      17.89, 18.30, 18.67, 19.02, 19.37, 19.74, 20.09, 20.44, 20.79, 21.15,
      21.50, 21.85, 22.20, 22.49
     },
     { /* qsthr */
      27.28, 27.28, 22.28, 19.28, 14.28, 12.28, 12.28, 12.28, 12.28, 12.28,
      12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28,
      15.29, 15.29, 15.29, 15.29, 15.29, 15.29, 15.29, 15.29, 17.05, 17.05,
      17.05, 17.05, 18.30, 18.30, 18.30, 18.30, 19.27, 19.27, 22.27, 23.06,
      23.06, 25.73, 25.73, 26.31, 31.31, 30.73
     }
  },
/* added by T. Araki (1997.10.16) end */

  { 24000 },
  { 32000 },
  { 44100, 42, 
     { /* w_low */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20,
      22, 24, 26, 28, 30, 32, 35, 38, 41, 44, 48, 52, 56, 60, 65, 70, 76,
      82, 89, 97, 106, 116
     },
     { /* w_high */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 21,
      23, 25, 27, 29, 31, 34, 37, 40, 43, 47, 51, 55, 59, 64, 69, 75, 81,
      88, 96, 105, 115, 127
     },
     { /* width */
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
      2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 12
     },
     { /* bval */
      0.00, 1.73, 3.41, 4.99, 6.45, 7.75, 8.92, 9.96, 10.87, 11.68, 12.39,
      13.03, 13.61, 14.12, 14.59, 15.01, 15.40, 15.76, 16.39, 16.95, 17.45,
      17.89, 18.30, 18.67, 19.02, 19.41, 19.85, 20.25, 20.62, 21.01, 21.43,
      21.81, 22.15, 22.51, 22.87, 23.23, 23.59, 23.93, 24.00, 24.00, 24.00,
      24.00
     },
     { /* qsthr */
      27.28, 22.28, 14.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28,
      12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 15.29, 15.29,
      15.29, 15.29, 15.29, 15.29, 15.29, 17.05, 17.05, 20.05, 20.05, 23.30,
      23.30, 23.30, 28.30, 29.27, 29.27, 40.06, 40.06, 40.73, 51.31, 51.82,
      52.28, 53.07
     }
  },
  { 48000, 42,
     { /* w_low */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 21,
      23, 25, 27, 29, 31, 34, 37, 40, 43, 46, 50, 54, 58, 63, 68, 74, 80,
      87, 95, 104, 114, 126
     },
     { /* w_high */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22,
      24, 26, 28, 30, 33, 36, 39, 42, 45, 49, 53, 57, 62, 67, 73, 79, 86,
      94, 103, 113, 125, 127
     }, 
     { /* width */
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
      2, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 12, 1
     },
     { /* bval */
      0.00, 1.88, 3.70, 5.39, 6.93, 8.29, 9.49, 10.53, 11.45, 12.26, 12.96,
      13.59, 14.15, 14.65, 15.11, 15.52, 15.90, 16.56, 17.15, 17.66, 18.13,
      18.54, 18.93, 19.28, 19.69, 20.14, 20.54, 20.92, 21.27, 21.64, 22.03,
      22.39, 22.76, 23.13, 23.49, 23.85, 24.00, 24.00, 24.00, 24.00, 24.00,
      24.00
     },
     { /* qsthr */
      27.28, 22.28, 14.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28,
      12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 12.28, 15.29, 15.29, 15.29,
      15.29, 15.29, 15.29, 15.29, 17.05, 20.05, 20.05, 20.05, 22.05, 23.30,
      28.30, 28.30, 29.27, 39.27, 40.06, 40.06, 50.37, 51.31, 51.82, 52.28,
      53.07, 53.07
     }
  },
  { 96000 },
  { -1 }
};
/* added by T. Araki (1997.07.10) end */

/* added by T. Araki (1997.10.16) */
static double          sample[MAX_TIME_CHANNELS][BLOCK_LEN_LONG*2];
                               /* sample value */

static FFT_TABLE_LONG    fft_tbl_long;  /* table for long fft */
static FFT_TABLE_SHORT    fft_tbl_short;  /* table for short fft */
static PARTITION_TABLE_LONG    *part_tbl_long;  
                               /* partition table for long block */
static PARTITION_TABLE_SHORT    *part_tbl_short;
                               /* partition table for short block */
/*static PSY_STATVARIABLE_LONG    psy_stvar_long;*/
static PSY_STATVARIABLE_LONG    psy_stvar_long[MAX_TIME_CHANNELS];
                               /* static variables for long block */
/*static PSY_STATVARIABLE_SHORT    psy_stvar_short;*/
static PSY_STATVARIABLE_SHORT    psy_stvar_short[MAX_TIME_CHANNELS];
                               /* static variables for short block */
/* added by T. Araki (1997.10.16) end */

void EncTf_psycho_acoustic_init( void )
{
  int chanNum;
/* added by T. Araki (1997.10.16) */
      psy_fft_table_init(&fft_tbl_long, &fft_tbl_short);
         /* initializing fft table */
      for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
	psy_calc_init(&sample[chanNum], &psy_stvar_long[chanNum], &psy_stvar_short[chanNum]);
	/* initializing static variables */
  }
/* added by T. Araki (1997.10.16) end */

}

/* added by T. Okada (1997.07.10) */
void psy_fft_table_init(FFT_TABLE_LONG *fft_tbl_long, 
			FFT_TABLE_SHORT *fft_tbl_short
			)
{    

    int i,j,k,n,n2,n4,n8;
    double c,s,dc,ds,t;

    /* generating Hann window */
    for(i = 0; i < BLOCK_LEN_LONG*2; ++i)
	fft_tbl_long->hw[i] = (0.5 - 0.5 * cos( M_PI * (double)(i + 0.5)/(double)(BLOCK_LEN_LONG) ) );
    for(i = 0; i < BLOCK_LEN_SHORT*2; ++i)
	fft_tbl_short->hw[i] = (0.5 - 0.5 * cos( M_PI * (double)(i + 0.5)/(double)(BLOCK_LEN_SHORT) ) );

    /* generating sin table (long) */
    n = BLOCK_LEN_LONG * 2;
    n2 = n/2;  
    n4 = n2/2;  
    n8 = n4/2;

    t = sin(M_PI / (double)n);
    dc = 2.0*t*t;
    ds = sqrt(dc * (2.0 - dc));
    t = 2*dc;
    c = fft_tbl_long->st[n4] = 1.0;
    s = fft_tbl_long->st[0] = 0;

    for(i = 1; i < n8; ++i){
	c -= dc;  dc += t * c;
	s += ds;  ds -= t * s;
	fft_tbl_long->st[i] = s;  fft_tbl_long->st[n4 - i] = c;
    }
    if (n8 != 0) fft_tbl_long->st[n8] = sqrt(0.5);
    for (i = 0; i < n4; i++)
	fft_tbl_long->st[n2 - i] = fft_tbl_long->st[i];
    for (i = 0; i < n2 + n4; i++)
	fft_tbl_long->st[i + n2] = -fft_tbl_long->st[i];

    /* generating sin table (short) */
    n = BLOCK_LEN_SHORT * 2;
    n2 = n/2;  
    n4 = n2/2;  
    n8 = n4/2;

    t = sin(M_PI / (double)n);
    dc = 2*t*t;
    ds = sqrt(dc * (2.0 - dc));
    t = 2*dc;
    c = fft_tbl_short->st[n4] = 1.0;
    s = fft_tbl_short->st[0] = 0;

    for(i = 1; i < n8; ++i){
	c -= dc;  dc += t * c;
	s += ds;  ds -= t * s;
	fft_tbl_short->st[i] = s;  fft_tbl_short->st[n4 - i] = c;
    }
    if (n8 != 0) fft_tbl_short->st[n8] = sqrt(0.5);
    for (i = 0; i < n4; i++)
	fft_tbl_short->st[n2 - i] = fft_tbl_short->st[i];
    for (i = 0; i < n2 + n4; i++)
	fft_tbl_short->st[i + n2] = - fft_tbl_short->st[i];

    /* generating bit inverse table (long) */
    n = BLOCK_LEN_LONG * 2;
    n2 = n/2; i = j = 0;

    for(;;){
	fft_tbl_long->brt[i] = j;
	if( ++i >= n ) break;
	k = n2;
	while(k <= j){
	    j -= k;
	    k /= 2;
	}
	j += k;
    }

    /* generating bit inverse table (short) */
    n = BLOCK_LEN_SHORT * 2;
    n2 = n/2; i = j = 0;

    for(;;){
	fft_tbl_short->brt[i] = j;
	if( ++i >= n ) break;
	k = n2;
	while(k <= j){
	    j -= k;
	    k /= 2;
	}
	j += k;
    }
}
/* added by T. Okada (1997.07.10) end */

/* added by T. Araki (1997.07.10) */
void psy_part_table_init( 
			   double sampling_rate,
			   PARTITION_TABLE_LONG **part_tbl_long,
			   PARTITION_TABLE_SHORT **part_tbl_short
			   )
{ 
    int b,bb; /* Jul 10 */
    double tmp;

   *part_tbl_long = &part_tbl_long_all[0];

  /* find correct sampling rate depending parameters */
  while( (*part_tbl_long)->sampling_rate != (int)sampling_rate ) {
    if( (*part_tbl_long)->sampling_rate == -1 ) {
      CommonExit( 1, "Encode: unknown sampling rate in psychoacoustic module %f", sampling_rate );
    }
    (*part_tbl_long)++;
  }

   *part_tbl_short = &part_tbl_short_all[0];

  /* find correct sampling rate depending parameters */
  while( (*part_tbl_short)->sampling_rate != (int)sampling_rate ) {
    if( (*part_tbl_short)->sampling_rate == -1 ) {
      CommonExit( 1, "Encode: unknown sampling rate in psychoacoustic module %f", sampling_rate );
    }
    (*part_tbl_short)++;
  }

    /* added by T. Okada (1997.07.10) */
    for( b = 0; b < (*part_tbl_long)->len; ++b){
	tmp = 0.0;
	for( bb = 0; bb < (*part_tbl_long)->len; ++bb)
	    tmp += sprdngf( (*part_tbl_long)->bval[bb],(*part_tbl_long)->bval[b]);
	(*part_tbl_long)->rnorm[b] = 1.0/tmp;
    }
    /* added by T. Okada (1997.07.10) end */

    /* added by T. Araki (1997.10.16) */
    for( b = 0; b < (*part_tbl_short)->len; ++b){
	tmp = 0.0;
	for( bb = 0; bb < (*part_tbl_short)->len; ++bb)
	    tmp += sprdngf( (*part_tbl_short)->bval[bb],(*part_tbl_short)->bval[b]);
	(*part_tbl_short)->rnorm[b] = 1.0/tmp;
    }
    /* added by T. Araki (1997.10.16) end */
}


void psy_calc_init( 
		   double sample[][BLOCK_LEN_LONG*2],
		   PSY_STATVARIABLE_LONG *psy_stvar_long,
		   PSY_STATVARIABLE_SHORT *psy_stvar_short
		   )
{ 
  int ch;
  int chanNum;
  int w;
  int i;

  ch=0;
  /*  for(ch = 0; ch < Chans; ++ch){ */
    for(i = 0; i < BLOCK_LEN_LONG*2; ++i){
      sample[ch][i] = 0.0;
    }
    /* } */

    /*  for(ch = 0; ch < Chans; ++ch){ */
    for(i = 0; i < BLOCK_LEN_LONG*3; ++i){
      psy_stvar_long->fft_r[ch][i] = 0.0;
      psy_stvar_long->fft_f[ch][i] = 0.0;
    }
    /*  }*/

  psy_stvar_long->p_fft = 0;

  /*  for(ch = 0; ch < Chans; ++ch){*/
    for(i = 0; i < NPART_LONG*2; ++i){
      psy_stvar_long->nb[ch][i] = 90.0;
    }
    /*}*/

  psy_stvar_long->p_nb = 0;
/* added by T. Araki (1997.07.10) end */

/* added by T. Araki (1997.10.16) */
  /*  for(ch = 0; ch < Chans; ++ch){*/
    for(i = 0; i < BLOCK_LEN_SHORT; ++i){
      psy_stvar_short->last6_fft_r[ch][i] = 0.0;
      psy_stvar_short->last6_fft_f[ch][i] = 0.0;
      psy_stvar_short->last7_fft_r[ch][i] = 0.0;
      psy_stvar_short->last7_fft_f[ch][i] = 0.0;
    }
    /*  }*/

    /*  for(ch = 0; ch < Chans; ++ch){*/
    for(i = 0; i < NPART_SHORT; ++i){
      psy_stvar_short->last7_nb[ch][i] = 90.0;
    }
    /* }*/
/* added by T. Araki (1997.10.16) end */
}

/* main */
void EncTf_psycho_acoustic( 
			   /* input */
			   double sampling_rate,
			   int    no_of_chan,         /* no of audio channels */
			   double *p_time_signal[],
			   enum WINDOW_TYPE block_type[],
			   int qcSelect,
			   int frameLength,
			   /* output */
			   CH_PSYCH_OUTPUT p_chpo_long[],
			   CH_PSYCH_OUTPUT p_chpo_short[][MAX_SHORT_WINDOWS]
			   )
{ 

    int             ch;
    SR_INFO         *p_sri;

/* added by T. Araki (1997.07.10) */

    static int   flag = 0;

    PSY_VARIABLE_LONG    psy_var_long;  /* variables for long block */
    PSY_VARIABLE_SHORT    psy_var_short;  /* variables for short block */
/* added by T. Araki (1997.07.10) end */

    if ((qcSelect==AAC_QC)||(qcSelect==AAC_PRED)) { /* AAC */
	switch (frameLength) {
	case 960:
	    p_sri = &sr_info_960[0];
	    break;
	case 1024:
	    p_sri = &sr_info_aac[0];
	    break;
	default:
	    CommonExit(-1,"\n illegal frameLength in EncTf_psycho_acoustic %d,frameLength");
	}
    }   
   
    else{
	p_sri = &sr_info_NOT_aac[0];
    }

  /* find correct sampling rate depending parameters */
  while( p_sri->sampling_rate != (long)sampling_rate ) {
    if( p_sri->sampling_rate == -1 ) {
      CommonExit( 1, "Encode: unknown sampling rate in psychoacoustic module %f", sampling_rate );
    }
    p_sri++;
  }

/* added by T. Araki (1997.07.10) */
  if( flag==0 ) {
      psy_part_table_init(sampling_rate, &part_tbl_long, &part_tbl_short);
         /* initializing Table B 2.1.*.a, B 2.1.*.b in N1650 */
      flag = 1;
  }


  /* This was originally written to work for N channels using one call to */
  /* EncTf_psycho_acoustic.  This has been modified to process only one channel */
  /* at a time.  For N channels, the user should call EncTf_psycho_acoustic */
  /* once for each channel, and pass the channel number as the no_of_chan */
  /* parameter.  (CL, 97.01.10) */
#if 0
 for( ch=0; ch<no_of_chan; ch++ ) {
      psy_step1(p_time_signal, sample, ch);
         /* input sample value */
      psy_step2(sample, &psy_stvar_long, &psy_stvar_short, &fft_tbl_long, &fft_tbl_short, ch);
         /* FFT */
      psy_step3(&psy_stvar_long, &psy_stvar_short, &psy_var_long, &psy_var_short, ch);
         /* caluculation of predicted value */
      psy_step4(&psy_stvar_long, &psy_stvar_short, &psy_var_long, &psy_var_short, ch);
         /* caluculation of unpredictability mesure */
      psy_step5(part_tbl_long, part_tbl_short, &psy_stvar_long, &psy_stvar_short, &psy_var_long, &psy_var_short, ch);
         /* caluculation of energy and unpredictability in each partition */
      psy_step6(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
         /* convolution of energy and unpredictability */ 
      psy_step7(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
         /* caluculation of tonality in each partition */
      psy_step8(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
         /* caluculation of SNR in each partition */
      psy_step9(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
         /* caluculation of power ratio in each partition */
      psy_step10(part_tbl_long, part_tbl_short, &psy_stvar_long, &psy_stvar_short, &psy_var_long, &psy_var_short, ch);
         /* caluculation of actual energy threshold in each partition */
      psy_step11(part_tbl_long, part_tbl_short, &psy_stvar_long, &psy_stvar_short, ch);
         /* pre-echo control and threshold in quiet */
      psy_step12(part_tbl_long, part_tbl_short, &psy_stvar_long, &psy_stvar_short, &psy_var_long, &psy_var_short, ch);
         /* caluculation of PE */
      psy_step13(&psy_var_long, block_type, ch);
         /* decision of block type */
      psy_step14(p_sri, part_tbl_long, part_tbl_short, &psy_stvar_long, &psy_stvar_short, &psy_var_long, &psy_var_short, ch);
         /* caluculation of 1/SMR in each swb */
  }	
/* added by T. Araki (1997.07.10) end */
#else
 {
   ch =0;
      psy_step1(p_time_signal,&sample[no_of_chan], ch);
      psy_step2(&sample[no_of_chan], &psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], &fft_tbl_long, 
         &fft_tbl_short, ch);
      psy_step3(&psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], &psy_var_long, &psy_var_short, ch);
      psy_step4(&psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], &psy_var_long, &psy_var_short, ch);
      psy_step5(part_tbl_long, part_tbl_short, &psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], 
         &psy_var_long, &psy_var_short, ch);
      psy_step6(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
      psy_step7(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
      psy_step8(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
      psy_step9(part_tbl_long, part_tbl_short, &psy_var_long, &psy_var_short);
      psy_step10(part_tbl_long, part_tbl_short, &psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], 
         &psy_var_long, &psy_var_short, ch);
      psy_step11(part_tbl_long, part_tbl_short, &psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], ch);
      psy_step12(part_tbl_long, part_tbl_short, &psy_stvar_long[no_of_chan], &psy_stvar_short[no_of_chan], 
         &psy_var_long, &psy_var_short, ch);
      psy_step13(&psy_var_long, block_type, ch);
      psy_step14(p_sri, part_tbl_long, part_tbl_short, &psy_stvar_long[no_of_chan], 
         &psy_stvar_short[no_of_chan], &psy_var_long, &psy_var_short, ch);
  }	
#endif




 /*  for( ch=0; ch<no_of_chan; ch++ ) { */
 {   /* Now performed for only one channel at a time, CL 97.01.10 */
   int i;

    p_chpo_long[ch].p_ratio   = psy_stvar_long[no_of_chan].ismr[ch];
                           /* changed by T. Araki (1997.7.10) */
    p_chpo_long[ch].cb_width  = p_sri->cb_width_long;
    p_chpo_long[ch].no_of_cb  = p_sri->num_cb_long;

    for( i=0; i<MAX_SHORT_WINDOWS; i++ ) {
      p_chpo_short[ch][i].p_ratio  = psy_stvar_short[no_of_chan].ismr[ch][i];
                           /* changed by T. Araki (1997.10.16) */
      p_chpo_short[ch][i].cb_width = p_sri->cb_width_short;
      p_chpo_short[ch][i].no_of_cb = p_sri->num_cb_short;
    }

  }

}

/* added by I. Kaneko (1997.7.10) */
/* Itaru Kaneko Absolute threshold generator module */
double psy_get_absthr(double f)
{
	double db;	/* absolute threshold in [db]	*/
	double f0; 	/* freq for min thr		*/
	double db10k;	/* thr @ 10k Hz			*/
	double db100;	/* thr @ 100 Hz			*/
	double dbmax;	/* max absthr			*/
	double pow_ord;	/* power order			*/
	double dbdep;	/* DB depth			*/


	f0 = 2000.0;	/* freq for min thr		*/
	db10k = 20.0;	/* thr @ 10k Hz			*/
	db100 = 30.0;	/* thr @ 100 Hz			*/
	dbmax = 50.0;	/* max absthr			*/
	pow_ord= 3.0;	/* power order			*/
	dbdep = -5.0;	/* DB depth			*/

	if ( f <= 10.0 ) {	/* DC cutoff at 10Hz	*/
		db = 200.0;	/* virtual infinity	*/
	}
	else {
		db = psy_sqr(log(f0/f+1.0)) / psy_sqr(log(f0/100.0+1.0)) * 50.0
		   + pow((f - f0)/(10000.0-f0),pow_ord) * db10k - dbdep;
	}

	if (db <= dbmax)
		return(db);
	else		/* anything is audible at 50dB	*/
		return(dbmax);

}
/* added by I. Kaneko (1997.7.10) end */

/* added by T. Okada (1997.7.10) */
double sprdngf(double b1, double b2)
{
    double tmpx,tmpy,tmpz;

    tmpx = (b2 >= b1) ? 3.0*(b2-b1) : 1.5*(b2-b1);
    
    tmpz = 8.0 * psy_min( (tmpx-0.5)*(tmpx-0.5) - 2.0*(tmpx-0.5),0.0 );

    tmpy = 15.811389 + 7.5*(tmpx + 0.474)-17.5 *sqrt(1.0 + (tmpx+0.474)*(tmpx+0.474));

    return( tmpy < -100.0 ? 0.0 : pow(10.0, (tmpz + tmpy)/10.0) );
}

void psy_step1(double* p_time_signal[], 
	       double sample[][BLOCK_LEN_LONG*2], 
	       int ch) 
{
    int i;
    for(i = 0; i < BLOCK_LEN_LONG; ++i){
	sample[ch][i] = sample[ch][i+BLOCK_LEN_LONG];
	sample[ch][i+BLOCK_LEN_LONG] = p_time_signal[ch][i]/32767.0;
    }

}

void psy_step2(double sample[][BLOCK_LEN_LONG*2], 
               PSY_STATVARIABLE_LONG *psy_stvar_long, 
               PSY_STATVARIABLE_SHORT *psy_stvar_short,
	       FFT_TABLE_LONG *fft_tbl_long,
	       FFT_TABLE_SHORT *fft_tbl_short,
	       int ch
	       )
{
    int w,i,j,k,l,h,n,d,ik,k2,n4;
    double t,s,c,dx,dy;
    double *xl,*yl;

    /* FFT for long */
    xl = (double *)malloc( sizeof(double) * BLOCK_LEN_LONG * 2 );
    yl = (double *)malloc( sizeof(double) * BLOCK_LEN_LONG * 2 );

    psy_stvar_long->p_fft += BLOCK_LEN_LONG;

    if(psy_stvar_long->p_fft == BLOCK_LEN_LONG * 3)
	psy_stvar_long->p_fft = 0;

    /* window *//* static int it = 0; */
    for(i = 0; i < BLOCK_LEN_LONG*2; ++i){
	xl[i] = fft_tbl_long->hw[i] * sample[ch][i];
	yl[i] = 0.0;
    }

    n = BLOCK_LEN_LONG*2;
    n4 = n/4;

    for (i = 0; i < n; ++i) {    /* bit inverse */
	j = fft_tbl_long->brt[i];
	if (i < j) {
	    t = xl[i];  xl[i] = xl[j];  xl[j] = t;
	    t = yl[i];  yl[i] = yl[j];  yl[j] = t;
	}
    }
    for (k = 1; k < n; k = k2) {    /* translation */
	h = 0;  k2 = k + k;  d = n / k2;
	for (j = 0; j < k; j++) {
	    c = fft_tbl_long->st[h + n4];
	    s = fft_tbl_long->st[h];
	    for (i = j; i < n; i += k2) {
		ik = i + k;
		dx = s * yl[ik] + c * xl[ik];
		dy = c * yl[ik] - s * xl[ik];
		xl[ik] = xl[i] - dx;  xl[i] += dx;
		yl[ik] = yl[i] - dy;  yl[i] += dy;
	    }
	    h += d;
	}
    }

    for(w = 0; w < BLOCK_LEN_LONG; ++w){
	psy_stvar_long->fft_r[ch][w+psy_stvar_long->p_fft] 
	    = sqrt(xl[w]*xl[w] + yl[w]*yl[w]);

	if( xl[w] > 0.0 ){
	    if( yl[w] >= 0.0 )
		psy_stvar_long->fft_f[ch][w+psy_stvar_long->p_fft] = atan( yl[w] / xl[w] );
	    else
		psy_stvar_long->fft_f[ch][w+psy_stvar_long->p_fft] = atan( yl[w] / xl[w] )+ M_PI * 2.0;
	} else if( xl[w] < 0.0 ) {
	    psy_stvar_long->fft_f[ch][w+psy_stvar_long->p_fft] = atan( yl[w] / xl[w] ) + M_PI;
	} else {
	    if( yl[w] > 0.0 )
		psy_stvar_long->fft_f[ch][w+psy_stvar_long->p_fft] = M_PI * 0.5;
	    else if( yl[w] < 0.0 )
		psy_stvar_long->fft_f[ch][w+psy_stvar_long->p_fft] = M_PI * 1.5;
	    else
		psy_stvar_long->fft_f[ch][w+psy_stvar_long->p_fft] = 0.0; /* tmp */
	}
    }

    free(xl);
    free(yl);


/* added by T. Araki (1997.10.16) */
    /* FFT for short */
    xl = (double *)malloc( sizeof(double) * BLOCK_LEN_SHORT * 2 );
    yl = (double *)malloc( sizeof(double) * BLOCK_LEN_SHORT * 2 );

    for(l = 0; l < MAX_SHORT_WINDOWS; ++l){

        /* window */        
        for(i = 0; i < BLOCK_LEN_SHORT*2; ++i){
	    xl[i] = fft_tbl_short->hw[i] * sample[ch][OFFSET_FOR_SHORT + BLOCK_LEN_SHORT * l + i];
	    yl[i] = 0.0;
	}

	n = BLOCK_LEN_SHORT*2;
	n4 = n/4;

	for (i = 0; i < n; ++i) {    /* bit inverse */
	    j = fft_tbl_short->brt[i];
	    if (i < j) {
	        t = xl[i];  xl[i] = xl[j];  xl[j] = t;
		t = yl[i];  yl[i] = yl[j];  yl[j] = t;
	    }
	}
	for (k = 1; k < n; k = k2) {    /* translation */
	    h = 0;  k2 = k + k;  d = n / k2;
	    for (j = 0; j < k; j++) {
	        c = fft_tbl_short->st[h + n4];
		s = fft_tbl_short->st[h];
		for (i = j; i < n; i += k2) {
		    ik = i + k;
		    dx = s * yl[ik] + c * xl[ik];
		    dy = c * yl[ik] - s * xl[ik];
		    xl[ik] = xl[i] - dx;  xl[i] += dx;
		    yl[ik] = yl[i] - dy;  yl[i] += dy;
		}
		h += d;
	    }
	}

	for(w = 0; w < BLOCK_LEN_SHORT; w++){
	    psy_stvar_short->fft_r[l][w] 
	        = sqrt(xl[w]*xl[w] + yl[w]*yl[w]);

	    if( xl[w] > 0.0 ){
	        if( yl[w] >= 0.0 )
		    psy_stvar_short->fft_f[l][w] = atan( yl[w] / xl[w] );
		else
		    psy_stvar_short->fft_f[l][w] = atan( yl[w] / xl[w] )+ M_PI * 2.0;
	    } else if( xl[w] < 0.0 ) {
	        psy_stvar_short->fft_f[l][w] = atan( yl[w] / xl[w] ) + M_PI;
	    } else {
	        if( yl[w] > 0.0 )
		    psy_stvar_short->fft_f[l][w] = M_PI * 0.5;
		else if( yl[w] < 0.0 )
		    psy_stvar_short->fft_f[l][w] = M_PI * 1.5;
		else
		    psy_stvar_short->fft_f[l][w] = 0.0; /* tmp */
	    }
	}
    }

    free(xl);
    free(yl);
/* added by T. Araki (1997.10.16) end */
}

void psy_step3(PSY_STATVARIABLE_LONG *psy_stvar_long, 
               PSY_STATVARIABLE_SHORT *psy_stvar_short, 
               PSY_VARIABLE_LONG *psy_var_long, 
               PSY_VARIABLE_SHORT *psy_var_short, 
               int ch
	       )
{
    int w,i;
    int p1_l,p2_l;

    p1_l = psy_stvar_long->p_fft - BLOCK_LEN_LONG;
    if( p1_l < 0 ) p1_l = BLOCK_LEN_LONG * 2;
    p2_l = p1_l - BLOCK_LEN_LONG;
    if( p2_l < 0 ) p2_l = BLOCK_LEN_LONG * 2;

    for(w = 0; w < BLOCK_LEN_LONG; ++w){
	psy_var_long->r_pred[w] = 2.0 * psy_stvar_long->fft_r[ch][p1_l + w] - psy_stvar_long->fft_r[ch][p2_l + w];
	psy_var_long->f_pred[w] = 2.0 * psy_stvar_long->fft_f[ch][p1_l + w] - psy_stvar_long->fft_f[ch][p2_l + w];
    }

/* added by T. Araki (1997.10.16) */
    for(w = 0; w < BLOCK_LEN_SHORT; ++w){
        psy_var_short->r_pred[0][w] = 2.0 * psy_stvar_short->last7_fft_r[ch][w] - psy_stvar_short->last6_fft_r[ch][w];
        psy_var_short->f_pred[0][w] = 2.0 * psy_stvar_short->last7_fft_f[ch][w] - psy_stvar_short->last6_fft_f[ch][w];
        psy_var_short->r_pred[1][w] = 2.0 * psy_stvar_short->fft_r[0][w] - psy_stvar_short->last7_fft_r[ch][w];
        psy_var_short->f_pred[1][w] = 2.0 * psy_stvar_short->fft_f[0][w] - psy_stvar_short->last7_fft_f[ch][w];
    }

    for(i = 2; i < MAX_SHORT_WINDOWS; ++i){
        for(w = 0; w < BLOCK_LEN_SHORT; ++w){
	    psy_var_short->r_pred[i][w] = 2.0 * psy_stvar_short->fft_r[i - 1][w] - psy_stvar_short->fft_r[i - 2][w];
	    psy_var_short->f_pred[i][w] = 2.0 * psy_stvar_short->fft_f[i - 1][w] - psy_stvar_short->fft_f[i - 2][w];
	}
    }

    for(w = 0; w < BLOCK_LEN_SHORT; ++w){
        psy_stvar_short->last6_fft_r[ch][w] = psy_stvar_short->fft_r[6][w];
	psy_stvar_short->last6_fft_f[ch][w] = psy_stvar_short->fft_f[6][w];
        psy_stvar_short->last7_fft_r[ch][w] = psy_stvar_short->fft_r[7][w];
	psy_stvar_short->last7_fft_f[ch][w] = psy_stvar_short->fft_f[7][w];
    }
/* added by T. Araki (1997.10.16) end */
}

void psy_step4(PSY_STATVARIABLE_LONG *psy_stvar_long,
               PSY_STATVARIABLE_SHORT *psy_stvar_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short,
	       int ch
	       )
{
    int w,i;
    double r,f,rp,fp;

    for(w = 0; w < BLOCK_LEN_LONG; ++w){
	r = psy_stvar_long->fft_r[ch][psy_stvar_long->p_fft+w];
	f = psy_stvar_long->fft_f[ch][psy_stvar_long->p_fft+w];
	rp = psy_var_long->r_pred[w];
	fp = psy_var_long->f_pred[w];
	
	if( r + fabs(rp) != 0.0 )
	    psy_var_long->c[w] = sqrt( psy_sqr(r*cos(f) - rp*cos(fp))
				      +psy_sqr(r*sin(f) - rp*sin(fp)) )/ ( r + fabs(rp) ) ;
	else
	    psy_var_long->c[w] = 0.0; /* tmp */
    }

/* added by T. Araki (1997.10.16) */
    for(i = 0; i < MAX_SHORT_WINDOWS; ++i){
        for(w = 0; w < BLOCK_LEN_SHORT; ++w){
	    r = psy_stvar_short->fft_r[i][w];
	    f = psy_stvar_short->fft_f[i][w];
	    rp = psy_var_short->r_pred[i][w];
	    fp = psy_var_short->f_pred[i][w];
	
	    if( r + fabs(rp) != 0.0 )
	        psy_var_short->c[i][w] = sqrt( psy_sqr(r*cos(f) - rp*cos(fp))
				      +psy_sqr(r*sin(f) - rp*sin(fp)) )/ ( r + fabs(rp) ) ;
	    else
	        psy_var_short->c[i][w] = 0.0; /* tmp */
	}
    }
/* added by T. Araki (1997.10.16) end */
}

void psy_step5(PARTITION_TABLE_LONG *part_tbl_long, 
	       PARTITION_TABLE_SHORT *part_tbl_short, 
	       PSY_STATVARIABLE_LONG *psy_stvar_long,
               PSY_STATVARIABLE_SHORT *psy_stvar_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short,
	       int ch
	       )
{
    int b,w,i;
    double tmp_cb;

    for(b = 0; b < part_tbl_long->len; ++b){
	psy_var_long->e[b] = 0.0;
	tmp_cb = 0.0;

/* added by T. Araki (1997.10.16) */
	for(w = part_tbl_long->w_low[b]; w <= part_tbl_long->w_high[b]; ++w){
	    psy_var_long->e[b] += psy_sqr(psy_stvar_long->fft_r[ch][psy_stvar_long->p_fft+w]);
	    tmp_cb += psy_sqr(psy_stvar_long->fft_r[ch][psy_stvar_long->p_fft+w]) * psy_var_long->c[w]; 
	}
/* added by T. Araki (1997.10.16) end */

	psy_var_long->c[b] = tmp_cb;
    }

/* added by T. Araki (1997.10.16) */
    for(i = 0; i < MAX_SHORT_WINDOWS; ++i){
        for(b = 0; b < part_tbl_short->len; ++b){
	    psy_var_short->e[i][b] = 0.0;
	    tmp_cb = 0.0;

	    for(w = part_tbl_short->w_low[b]; w <= part_tbl_short->w_high[b]; ++w){
	        psy_var_short->e[i][b] += psy_sqr(psy_stvar_short->fft_r[i][w]);
		tmp_cb += psy_sqr(psy_stvar_short->fft_r[i][w]) * psy_var_short->c[i][w]; 
	    }

	    psy_var_short->c[i][b] = tmp_cb;
	}
    }
/* added by T. Araki (1997.10.16) end */

}

void psy_step6(PARTITION_TABLE_LONG *part_tbl_long, 
	       PARTITION_TABLE_SHORT *part_tbl_short, 
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short
	       )
{
    int b,bb,i;
    double ecb,ct;
    double sprd;

    for(b = 0; b < part_tbl_long->len; ++b){
	ecb = 0.0;
	ct = 0.0;
	for(bb = 0; bb < part_tbl_long->len; ++bb){
	    sprd = sprdngf(part_tbl_long->bval[bb],part_tbl_long->bval[b]);
	    ecb += psy_var_long->e[bb] * sprd;
	    ct += psy_var_long->c[bb] * sprd;
	}
	if (ecb!=0.0) {
	  psy_var_long->cb[b] = ct / ecb;
	} else {
	  psy_var_long->cb[b] = 0.0;
	}
	psy_var_long->en[b] = ecb * part_tbl_long->rnorm[b];
    }

/* added by T. Araki (1997.10.16) */
    for(i = 0; i < MAX_SHORT_WINDOWS; ++i){ 
        for(b = 0; b < part_tbl_short->len; ++b){
	    ecb = 0.0;
	    ct = 0.0;
	    for(bb = 0; bb < part_tbl_short->len; ++bb){
	        sprd = sprdngf(part_tbl_short->bval[bb],part_tbl_short->bval[b]);
		ecb += psy_var_short->e[i][bb] * sprd;
		ct += psy_var_short->c[i][bb] * sprd;
	    }
	    if (ecb!=0.0) {	
	      psy_var_short->cb[i][b] = ct / ecb;
	    } else {
	      psy_var_short->cb[i][b] = 0.0;
	    }
	    psy_var_short->en[i][b] = ecb * part_tbl_short->rnorm[b];
	}
    }
/* added by T. Araki (1997.10.16) end */
}

void psy_step7(PARTITION_TABLE_LONG *part_tbl_long,
	       PARTITION_TABLE_SHORT *part_tbl_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short)
{
    int b,i;

    for(b = 0; b < part_tbl_long->len; ++b){
      if (psy_var_long->cb[b]>0.0) {
	psy_var_long->tb[b] = -0.299 - 0.43 * log( psy_var_long->cb[b] );
      } else {
	psy_var_long->tb[b] = 1.0;
      }
      if( psy_var_long->tb[b] > 1.0 ) psy_var_long->tb[b] = 1.0;
      else if( psy_var_long->tb[b] < 0.0 ) psy_var_long->tb[b] = 0.0;
    }


/* added by T. Araki (1997.10.16) */
    for(i = 0;  i < MAX_SHORT_WINDOWS; ++i){
        for(b = 0; b < part_tbl_short->len; ++b){
	  if (psy_var_short->cb[i][b]>0.0) {
	    psy_var_short->tb[i][b] = -0.299 - 0.43 * log( psy_var_short->cb[i][b] );
	  } else {
	    psy_var_short->tb[i][b] = 1.0;
	  }
	  if( psy_var_short->tb[i][b] > 1.0 ) psy_var_short->tb[i][b] = 1.0;
	  else if( psy_var_short->tb[i][b] < 0.0 ) psy_var_short->tb[i][b] = 0.0;
	}
    }
/* added by T. Araki (1997.10.16) end */
}

void psy_step8(PARTITION_TABLE_LONG *part_tbl_long,
	       PARTITION_TABLE_SHORT *part_tbl_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short)
{
    int b,i;
    double tmn = 18.0,nmt = 6.0;
    
    for(b = 0; b < part_tbl_long->len; ++b)
	psy_var_long->snr[b] = psy_var_long->tb[b] * tmn + (1.0 - psy_var_long->tb[b] ) * nmt ;

/* added by T. Araki (1997.10.16) */
    for(i = 0;  i < MAX_SHORT_WINDOWS; ++i){
        for(b = 0; b < part_tbl_short->len; ++b)
	    psy_var_short->snr[i][b] = psy_var_short->tb[i][b] * tmn + (1.0 - psy_var_short->tb[i][b] ) * nmt ;
    }
/* added by T. Araki (1997.10.16) end */

}    

void psy_step9(PARTITION_TABLE_LONG *part_tbl_long,
	       PARTITION_TABLE_SHORT *part_tbl_short,
	       PSY_VARIABLE_LONG *psy_var_long, 
	       PSY_VARIABLE_SHORT *psy_var_short
	       )
{
    int b,i;
    
    for(b = 0; b < part_tbl_long->len; ++b)
	psy_var_long->bc[b] = pow(10.0, -psy_var_long->snr[b]/10.0);

/* added by T. Araki (1997.10.16) */
    for(i = 0;  i < MAX_SHORT_WINDOWS; ++i){
        for(b = 0; b < part_tbl_short->len; ++b)
	    psy_var_short->bc[i][b] = pow(10.0, -psy_var_short->snr[i][b]/10.0);
    }
/* added by T. Araki (1997.10.16) end */
}

void psy_step10(PARTITION_TABLE_LONG *part_tbl_long,
		PARTITION_TABLE_SHORT *part_tbl_short,
		PSY_STATVARIABLE_LONG *psy_stvar_long, 
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
		PSY_VARIABLE_LONG *psy_var_long, 
		PSY_VARIABLE_SHORT *psy_var_short,
		int ch
		)
{
    int b,i;
    
    psy_stvar_long->p_nb += NPART_LONG;

    if( psy_stvar_long->p_nb == NPART_LONG*2 ) psy_stvar_long->p_nb = 0;

    for(b = 0; b < part_tbl_long->len; ++b){
	psy_stvar_long->nb[ch][psy_stvar_long->p_nb + b]
	    = psy_var_long->en[b] * psy_var_long->bc[b];
    }

/* added by T. Araki (1997.10.16) */
    for(i = 0;  i < MAX_SHORT_WINDOWS; ++i){
        for(b = 0; b < part_tbl_short->len; ++b){
	    psy_stvar_short->nb[i][b]
	        = psy_var_short->en[i][b] * psy_var_short->bc[i][b];
	}
    }
/* added by T. Araki (1997.10.16) end */
}

void psy_step11(PARTITION_TABLE_LONG *part_tbl_long, 
		PARTITION_TABLE_SHORT *part_tbl_short, 
		PSY_STATVARIABLE_LONG *psy_stvar_long, 
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
		int ch
		)
{
    int b,i;
    int p1,p2;
    double f;

    p1 = psy_stvar_long->p_nb;
    if( p1 == 0 ) p2 = NPART_LONG;
    else if( p1 == NPART_LONG ) p2 = 0;

    for(b = 0; b < part_tbl_long->len; ++b){
	f = part_tbl_long->sampling_rate 
	    * (double)(part_tbl_long->w_low[b] + part_tbl_long->w_high[b])
		/ 4.0 / (double)(part_tbl_long->w_high[part_tbl_long->len - 1] + 1);

	psy_stvar_long->nb[ch][p1+b]
#if 0 /*  to use Itaru Kaneko's absolute threshold function */
	    = psy_max( pow(10.0,psy_get_absthr(f)/10.0)/32767.0 * part_tbl_long->width[b],
		       psy_min( psy_stvar_long->nb[ch][p1+b],2.0*psy_stvar_long->nb[ch][p2+b])
		      ); 
#endif
#if 1 /* to use DIS 13818-7 model*/
	    = psy_max( pow(10.0,part_tbl_long->qsthr[b]/10.0)/32767.0 ,
		       psy_min( psy_stvar_long->nb[ch][p1+b],2.0*psy_stvar_long->nb[ch][p2+b])
		      );
#endif
    }

/* added by T. Araki (1997.10.16) */
    for(b = 0; b < part_tbl_short->len; ++b){
	f = part_tbl_short->sampling_rate 
	    * (double)(part_tbl_short->w_low[b] + part_tbl_short->w_high[b])
		/ 4.0 / (double)(part_tbl_short->w_high[part_tbl_short->len - 1] + 1);

	psy_stvar_short->nb[0][b]
#if 0 /*  to use Itaru Kaneko's absolute threshold function */
	    = psy_max( pow(10.0,psy_get_absthr(f)/10.0)/32767.0 * part_tbl_short->width[b],
		       psy_min( psy_stvar_short->nb[0][b],psy_stvar_short->last7_nb[ch][b])
		      ); 
#endif
#if 1 /* to use DIS 13818-7 model*/
	    = psy_max( pow(10.0,part_tbl_short->qsthr[b]/10.0)/32767.0 ,
		       psy_min( psy_stvar_short->nb[0][b],psy_stvar_short->last7_nb[ch][b])
		      );
#endif
    }

    for(i = 1;  i < MAX_SHORT_WINDOWS; ++i){
        for(b = 0; b < part_tbl_short->len; ++b){
	    f = part_tbl_short->sampling_rate 
	        * (double)(part_tbl_short->w_low[b] + part_tbl_short->w_high[b])
	            / 4.0 / (double)(part_tbl_short->w_high[part_tbl_short->len - 1] + 1);

	    psy_stvar_short->nb[i][b]
#if 0 /*  to use Itaru Kaneko's absolute threshold function */
	        = psy_max( pow(10.0,psy_get_absthr(f)/10.0)/32767.0 * part_tbl_short->width[b],
			   psy_min( psy_stvar_short->nb[i][b],psy_stvar_short->nb[i - 1][b])
			   ); 
#endif
#if 1 /* to use DIS 13818-7 model*/
	        = psy_max( pow(10.0,part_tbl_short->qsthr[b]/10.0)/32767.0 ,
			   psy_min( psy_stvar_short->nb[i][b],psy_stvar_short->nb[i - 1][b])
			   );
#endif
        }
    }

    for(b = 0; b < part_tbl_short->len; ++b){
        psy_stvar_short->last7_nb[ch][b] = psy_stvar_short->nb[7][b];
    }
/* added by T. Araki (1997.10.16) end */

}
/* added by T. Okada (1997.7.10) end */

/* added by T. Araki (1997.7.10) */
void psy_step12(PARTITION_TABLE_LONG *part_tbl_long, 
	        PARTITION_TABLE_SHORT *part_tbl_short,
		PSY_STATVARIABLE_LONG *psy_stvar_long,
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
	        PSY_VARIABLE_LONG *psy_var_long, 
	        PSY_VARIABLE_SHORT *psy_var_short,
	        int ch
		)
{
    int b;

    psy_var_long->pe = 0.0;
    for(b = 0; b < part_tbl_long->len; ++b){
	psy_var_long->pe -= part_tbl_long->width[b]
	    * log10(psy_stvar_long->nb[ch][psy_stvar_long->p_nb + b]
	            / (psy_var_long->e[b] + 0.0000000001)); 

    }

}

void psy_step13(PSY_VARIABLE_LONG *psy_var_long, 
	        enum WINDOW_TYPE *block_type,
		int ch
		)
{
  if(psy_var_long->pe > -1000) {
        *block_type = ONLY_SHORT_WINDOW;
  } else {
        *block_type = ONLY_LONG_WINDOW;
  }
}

void psy_step14(SR_INFO *p_sri,
		PARTITION_TABLE_LONG *part_tbl_long, 
	        PARTITION_TABLE_SHORT *part_tbl_short,
		PSY_STATVARIABLE_LONG *psy_stvar_long,
		PSY_STATVARIABLE_SHORT *psy_stvar_short, 
	        PSY_VARIABLE_LONG *psy_var_long, 
	        PSY_VARIABLE_SHORT *psy_var_short,
	        int ch
		)
{
    int b, n, w, i;
    int w_low, w_high;
    double thr, minthr;
    
    w_high = 0;
    for(n = 0; n < p_sri->num_cb_long; ++n){
	w_low = w_high;
	w_high += p_sri->cb_width_long[n];

        psy_var_long->epart[n] = 0.0;
	for(w = w_low; w < w_high; ++w){
	    psy_var_long->epart[n]
	      += psy_stvar_long->fft_r[ch][psy_stvar_long->p_fft + w]
	      * psy_stvar_long->fft_r[ch][psy_stvar_long->p_fft + w];
	}
    }

    for(b = 0; b < part_tbl_long->len; ++b){
        thr = psy_stvar_long->nb[ch][psy_stvar_long->p_nb + b]
	        / part_tbl_long->width[b];
        for(w = part_tbl_long->w_low[b]; w <= part_tbl_long->w_high[b]; ++w){
	    psy_var_long->thr[w] = thr;
	}
    }
    w_high = 0;
    for(n = 0; n < p_sri->num_cb_long; ++n){
        w_low = w_high;
	w_high += p_sri->cb_width_long[n];

        minthr = psy_var_long->thr[w_low];
	for(w = w_low; w < w_high; ++w){
	    if(psy_var_long->thr[w] < minthr){
	        minthr = psy_var_long->thr[w];
	    }
	}

	psy_var_long->npart[n] = minthr * (w_high - w_low);
    }

    for(n = 0; n < p_sri->num_cb_long; ++n){
      if (psy_var_long->epart[n]!=0.0) {
	psy_stvar_long->ismr[ch][n] = psy_var_long->npart[n] / psy_var_long->epart[n];
      } else {
	psy_stvar_long->ismr[ch][n] = 0.0;
      }
    }

/* added by T. Araki (1997.10.16) */
    for(i = 0; i < MAX_SHORT_WINDOWS; ++i){
        w_high = 0;
	for(n = 0; n < p_sri->num_cb_short; ++n){
	    w_low = w_high;
	    w_high += p_sri->cb_width_short[n];

	    psy_var_short->epart[i][n] = 0.0;
	    for(w = w_low; w < w_high; ++w){
	        psy_var_short->epart[i][n]
		    += psy_stvar_short->fft_r[i][w]
		    * psy_stvar_short->fft_r[i][w];
	    }
	}

	for(b = 0; b < part_tbl_short->len; ++b){
            thr = psy_stvar_short->nb[i][b] / part_tbl_short->width[b];
	    for(w = part_tbl_short->w_low[b]; w <= part_tbl_short->w_high[b]; ++w){
	        psy_var_short->thr[i][w] = thr;
	    }
	}

	w_high = 0;
	for(n = 0; n < p_sri->num_cb_short; ++n){
            w_low = w_high;
	    w_high += p_sri->cb_width_short[n];

	    minthr = psy_var_short->thr[i][w_low];
	    for(w = w_low + 1; w < w_high; ++w){
	        if(psy_var_short->thr[i][w] < minthr){
	            minthr = psy_var_short->thr[i][w];
	        }
	    }

	    psy_var_short->npart[i][n] = minthr * (w_high - w_low);
        }

	for(n = 0; n < p_sri->num_cb_short; ++n){
	  if (psy_var_short->epart[i][n]!=0.0) {
	    psy_stvar_short->ismr[ch][i][n] = psy_var_short->npart[i][n] / psy_var_short->epart[i][n];
	  } else {
	    psy_stvar_short->ismr[ch][i][n] = 0.0;
	  }
	}

    }
/* added by T. Araki (1997.10.16) end */

}
/* added by T. Araki (1997.7.10) end */
