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



Source file: 

$Id: psych.c,v 1.16 1999/07/22 15:52:03 purnhage Exp $

**********************************************************************/

/* CREATED BY :  Bernhard Grill -- August-96  */

#include <stdio.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "nttNew.h"
#include "tf_main.h"
#include "psych.h"
#include "common_m4a.h"

/* dummy psychoacoustic module */

extern short  sfb_96_1024[];

extern short sfb_96_128[] ;

extern short sfb_64_1024[] ;

extern short sfb_64_128[] ;

extern short sfb_48_1024[] ; 

extern short sfb_48_960[] ; 


extern short sfb_48_480[] ; 

extern short sfb_48_128[] ;

extern short sfb_48_120[] ;

extern short sfb_32_1024[] ;

extern short sfb_24_1024[] ;

extern short sfb_24_960[] ;

extern short sfb_24_128[] ;

extern short sfb_24_120[] ;

extern short sfb_16_1024[] ;

extern short sfb_16_960[] ;

extern short sfb_16_128[] ;

extern short sfb_16_120[] ;

extern short sfb_8_1024[] ;

extern short sfb_8_128[] ;

extern short sfb_8_960[] ;

extern short sfb_8_120[] ;

extern short sfb_32_960[] ;

extern short sfb_32_120[] ;
 

static SR_INFO sr_info_aac[MAX_SAMPLING_RATES+1] =
{
  { 8000  ,40, 15,
    sfb_8_1024,
    sfb_8_128,
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
  { 11025 ,43, 15,
    sfb_16_1024,
    sfb_16_128,
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
  { 12000 ,43, 15,
    sfb_16_1024,
    sfb_16_128,
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
  { 16000 ,43, 15,
    sfb_16_1024,
    sfb_16_128,
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
  { 22050 ,47, 15,
    sfb_24_1024,
    sfb_24_128,
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
  { 24000 ,47, 15,
    sfb_24_1024,
    sfb_24_128,
     { /* fixed_ratio_long[NSFB_LONG] */
      0.04, 0.04, 0.04, 0.04, 0.04,   0.04, 0.04, 0.04, 0.04, 0.04, 
      0.04, 0.04, 0.04, 0.04, 0.04,   0.04, 0.04, 0.04, 0.04, 0.04, 
      0.04, 0.04, 0.04, 0.04, 0.04,   0.04, 0.05, 0.05, 0.05, 0.05, 
      0.15, 0.15, 0.15, 0.15, 0.15,   0.15, 0.20, 0.20, 0.20, 0.20, 
      0.20, 0.20, 0.20, 0.25, 0.30,   0.30, 0.30, 0.40, 1.00
     },
     { /* fixed_ratio_short[NSFB_SHORT] */
      0.04, 0.04, 0.04, 0.05, 0.07,   0.10, 0.12, 0.15, 0.15, 0.20, 
      0.25, 0.30, 0.40, 0.90 , 0.90
     }
  },
  { 32000 ,51, 14,
    sfb_32_1024,
    sfb_48_128,
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
  { 44100 ,49, 14,
    sfb_48_1024,
    sfb_48_128,
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
  { 48000, 49, 14,
    sfb_48_1024,
    sfb_48_128,
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
  { 96000 ,41, 11,
    sfb_96_1024,
    sfb_96_128,
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
  { -1 }
};
static SR_INFO sr_info_960[MAX_SAMPLING_RATES+1] =
{
  { 8000  ,40, 15,
    sfb_8_960,
    sfb_8_120,
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
  { 11025 ,43, 15,
    sfb_16_960,
    sfb_16_120,
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
  { 12000 ,43, 15,
    sfb_16_960,
    sfb_16_120,
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
  { 16000 ,42, 15,
    sfb_16_960,
    sfb_16_120,
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
  { 24000 ,46, 15,
    sfb_24_960,
    sfb_24_120,
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
  { 32000 ,49, 14,
    sfb_32_960,
    sfb_48_120,
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
  { 44100 ,49, 14,
    sfb_48_960,
    sfb_48_120,
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
  { 48000, 49, 14,
    sfb_48_960,
    sfb_48_120,
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
#if 0
    { /*  cb_width_long[NSFB_LONG] */
      4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6, 
      6,  8,  8,  8, 10, 10, 12, 12, 14, 16, 18, 20, 
      22, 24, 26, 28, 32, 36, 40, 44, 48, 54, 62, 70, 342
    },
    { /* cb_width_short[NSFB_SHORT] */
      4,  4,  4,  4,  4,  6,  6,  8,  8, 10, 12, 16, 42
    },
#else
    NULL,
    NULL,
#endif
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



void EncTf_psycho_acoustic_init( void )
{
}

void EncTf_psycho_acoustic ( double          sampling_rate, 
                             int             no_of_chan,         /* no of audio channels */
                             QC_MOD_SELECT   qcSelect,
                             int             frameLength,         
                             CH_PSYCH_OUTPUT p_chpo_long[],
                             CH_PSYCH_OUTPUT p_chpo_short[][MAX_SHORT_WINDOWS] )
{ 
  int             ch,i,j,k;
  SR_INFO*        p_sri = NULL;


  /* YB : 971106 */ /* SH : 971121 */
  if ((qcSelect==AAC_QC) ||
      (qcSelect==AAC_SYS) || 
      (qcSelect==AAC_SCALABLE) || 
      (qcSelect==AAC_BSAC) || 
      (qcSelect==AAC_SCA_BSAC) || 
      (qcSelect== AAC_SYS_BSAC) ||
      (qcSelect==NTT_VQ) ||
      (qcSelect==NTT_VQ_SYS)
      ){ /* AAC */

    switch (frameLength) 
      {
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
    CommonExit(-1,"\nthis qc_mode is not supported");
    p_sri = &sr_info_NOT_aac[0];
  }


  /* find correct sampling rate depending parameters */
  while( p_sri->sampling_rate != (long)sampling_rate ) {
    if( p_sri->sampling_rate == -1 ) {
      CommonExit( 1, "Encode: unknown sampling rate in psychoacoustic module %f", sampling_rate );
    }
    p_sri++;
  }

  j=0;
  for (i=0;i<p_sri->num_cb_long; i++) {
    k = p_sri->cb_offset_long[i];
    p_sri->cb_width_long[i] = k - j;
    j = k;
  }
  j=0;
  for (i=0;i<p_sri->num_cb_short; i++) {
    k = p_sri->cb_offset_short[i];
    p_sri->cb_width_short[i] = k - j;
    j = k;
  }
  

  for( ch=0; ch<no_of_chan; ch++ ) {
    int i;
    p_chpo_long[ch].p_ratio   = p_sri->fixed_ratio_long;
    p_chpo_long[ch].cb_width  = p_sri->cb_width_long;
    p_chpo_long[ch].no_of_cb  = p_sri->num_cb_long;
    for( i=0; i<MAX_SHORT_WINDOWS; i++ ) {
      p_chpo_short[ch][i].p_ratio  = p_sri->fixed_ratio_short;
      p_chpo_short[ch][i].cb_width = p_sri->cb_width_short;
      p_chpo_short[ch][i].no_of_cb = p_sri->num_cb_short;
    }
  }
}


