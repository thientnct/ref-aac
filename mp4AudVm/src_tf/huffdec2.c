/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by

Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "huffdec2.h"
#include "huffdec3.h"
#include "port.h"
#include "sony_local.h" /* 971117 YT */
#include "common_m4a.h"
#ifdef VERSION2
#include "resilience.h"
#include "concealment.h"
#include "reorderspec.h"
#endif
#include "bitfct.h"
#include "buffers.h"
#include "bitstream.h"
#include "interface.h"

#ifdef VERSION2		/* HP 990426 */
#include "concealment.h"
#include "rvlcScfResil.h" 
#include "huffScfResil.h" 
#endif

#define MAX_NR_OF_SWB       120
#define VCB11_ARRAY_SIZE    LN4 /* probably too large, but ... better than too small */

  typedef struct {
    unsigned short nrOfSwb;
    unsigned short swbReal;
  } ENTRY_POINT;

#ifdef VERSION2
/* be cool, first declared as global variables, */
/* maybe to be included in error resilience handler HANDLE_RESILIENCE */
static int  vcb11[VCB11_ARRAY_SIZE]; /* virtual codebooks stored in an array */
static int* vcb11Ptr;               /* virtual codebooks pointer */

static void InitVcb11(void) 
{
  unsigned short j;

  for ( j = 0; j < VCB11_ARRAY_SIZE; j++ ) {
    vcb11[j] = 0;
  }
}

#endif /*VERSION2*/

static void PrintSpectralValues ( int *quant )
{
  unsigned short i;
  for ( i = 0; i < LN2; i++ )
    printf ( "bno %13li line %4i value %13i\n", bno, i, quant[i] );
}

#ifdef VERSION2
unsigned char Vcb11Used( unsigned short codebook ) 
{
  if( (codebook == 11) || ((codebook >= 16) && (codebook <= 31)) ) {
    return ( 1 );
  }
  else {
    return ( 0 );
  }
}
#endif /*VERSION2*/

void deinterleave ( void                    *inptr,  /* formerly pointer to type int */
                    void                    *outptr, /* formerly pointer to type int */
                    short                   length,  /* sizeof base type of inptr and outptr in chars */
                    const short             nsubgroups[], 
                    const int               ncells[], 
                    const short             cellsize[],
#ifdef VERSION2
                    int                     islong,
                    const HANDLE_RESILIENCE hResilience,
#endif /*VERSION2*/
                    short                   ngroups )
{
  char *inptr1  = (char*)inptr;
  char *outptr1 = (char*)outptr;

#ifdef VERSION2
  if ( GetReorderSpecFlag( hResilience ) )
    {
      unsigned short    unitNrInSpectralDirection;
      unsigned short    unitsInSpectralDirection;   /* number of units in spectral direction */
      unsigned short    windowNr;
      unsigned short    windows;
      unsigned short    lineNrPerUnit;
      unsigned short    linesPerUnit = 4;
      unsigned short    linesPerWindow;
      unsigned short    inLine = 0;

      if ( islong ) /* long */
        {
          linesPerWindow = LN2;                          /* 1024 */
          unitsInSpectralDirection = LN2 / linesPerUnit; /*  256 */
          windows = 1;                                   /*    1 */
        }
      else /* short */
        {
          linesPerWindow = SN2;                          /*  256 */
          unitsInSpectralDirection = SN2 / linesPerUnit; /*   32 */
          windows = 8;                                   /*    8 */
        }
        for ( unitNrInSpectralDirection = 0; 
              unitNrInSpectralDirection < unitsInSpectralDirection;
              unitNrInSpectralDirection ++ )         /* for each unit of a window within the spectrum */
          for ( windowNr = 0; windowNr < windows; windowNr++ )         /* for each window within a spectrum part (4 lines) */
            for ( lineNrPerUnit = 0; lineNrPerUnit < linesPerUnit; lineNrPerUnit++ ) /* for each line within a unit */
              {
                memcpy( outptr1 + length*(unitNrInSpectralDirection * linesPerUnit + windowNr * linesPerUnit * unitsInSpectralDirection + lineNrPerUnit) ,
                        inptr1  + length*inLine,
                        length);
                inLine++;
              }
    }
  else
#endif /*VERSION2*/
    {
      int i, j, k, l;
      char *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
      short cell_inc, subgroup_inc;

      start_subgroup_ptr = outptr1;

      for (i = 0; i < ngroups; i++)
        {
          cell_inc = 0;
          start_inptr = inptr1;

          /* Compute the increment size for the subgroup pointer */

          subgroup_inc = 0;
          for (j = 0; j < ncells[i]; j++) {
            subgroup_inc += cellsize[j];
          }

          /* Perform the deinterleaving across all subgroups in a group */

          for (j = 0; j < ncells[i]; j++) {
            subgroup_ptr = start_subgroup_ptr;
            for (k = 0; k < nsubgroups[i]; k++) {
              outptr1 = subgroup_ptr + length*cell_inc;
              for (l = 0; l < cellsize[j]; l++) {
                memcpy( outptr1, inptr1, length);
                outptr1 += length;
                inptr1  += length;
              }
              subgroup_ptr += length*subgroup_inc;
            }
            cell_inc += cellsize[j];
          }
          start_subgroup_ptr += (inptr1 - start_inptr);
        }
    }
}

static void calc_gsfb_table ( Info *info, 
                              byte *group )
{
  int group_offset;
  int group_idx;
  int offset;
  short * group_offset_p;
  int sfb,len;
  /* first calc the group length*/
  if (info->islong){
    return;
  } else {
    group_offset = 0;
    group_idx = 0;
    do  {
      info->group_len[group_idx] = group[group_idx] - group_offset;
      group_offset=group[group_idx];
      group_idx++;
    } while (group_offset<8);
    info->num_groups = group_idx;
    group_offset_p = info->bk_sfb_top;
    offset=0;
    for ( group_idx = 0; group_idx < info->num_groups; group_idx++) {
      len = info->group_len[group_idx];
      for (sfb=0;sfb<info->sfb_per_sbk[group_idx];sfb++){
        offset += info->sfb_width_short[sfb] * len;
        *group_offset_p++ = offset;
#if 0
        printf("%d %d: %d\n",  group_idx, sfb, offset);
#endif    
      }
    }
  }
}

void getgroup ( Info*             info, 
                byte*             group, 
#ifdef VERSION2
                HANDLE_RESILIENCE hResilience,
                HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                HANDLE_BUFFER     hVm )
{
  int i, j, first_short;

  if( debug['g'] ) printf("Grouping: 0");
  first_short = 1;
  for (i = 0; i < info->nsbk; i++) {
    if (info->bins_per_sbk[i] > SN2) {
      /* non-short windows are always their own group */
      *group++ = i+1;
    }
    else {
      /* only short-window sequences are grouped! */
      if (first_short) {
        /* first short window is always a new group */
        first_short=0;
      }
      else {
        if((j = GetBits ( 1,
#ifdef VERSION2
                          SCALE_FACTOR_GROUPING, 
                          hResilience, 
                          hEscInstanceData,
#endif /*VERSION2*/
                          hVm )) == 0) {
          *group++ = i;
        }
        if( debug['g'] ) printf("%1d", j);
      }
    }
  }
  *group = i;
  if( debug['g'] ) printf("\n");
}

/*
 * read a synthesis mask
 *  uses EXTENDED_MS_MASK
 *  and grouped mask 
 */
int getmask ( Info*             info, 
              byte*             group, 
              byte              max_sfb, 
              byte*             mask, 
#ifdef VERSION2
              HANDLE_RESILIENCE hResilience, 
              HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
              HANDLE_BUFFER     hVm )
{
  int b, i, mp;

  mp = GetBits ( LEN_MASK_PRES,
#ifdef VERSION2
                 MS_MASK_PRESENT, 
                 hResilience, 
                 hEscInstanceData,
#endif /*VERSION2*/
                 hVm );
  if( debug['m'] )
    printf("\nExt. Mask Present: %d\n",mp);

  /* special EXTENDED_MS_MASK cases */
  if(mp == 0) { /* no ms at all */
    return 0;
  }
  if(mp == 2) {/* MS for whole spectrum on, mask bits set to 1 */
    for(b = 0; b < info->nsbk; b = *group++)
      for(i = 0; i < info->sfb_per_sbk[b]; i ++)
        *mask++ = 1;
    return 2;
  }

  /* otherwise get mask */
  for(b = 0; b < info->nsbk; b = *group++){
    if( debug['m'] ) printf(" gr%1i:",b);
    for(i = 0; i < max_sfb; i ++) {
      *mask = (unsigned char) GetBits ( LEN_MASK,
#ifdef VERSION2
                                        MS_USED, 
                                        hResilience, 
                                        hEscInstanceData,
#endif /*VERSION2*/
                                        hVm );
      if( debug['m'] )printf("%1i",*mask);
      mask++;
    }
    for( ; i < info->sfb_per_sbk[b]; i++){
      *mask = 0;
      if( debug['m'] ) printf("%1i",*mask);
      mask++;
    }
  }
  if( debug['m'] ) printf("\n");
  return 1;
}

void clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int s;

  tns_frame_info->n_subblocks = info->nsbk;
  for (s=0; s<tns_frame_info->n_subblocks; s++)
    tns_frame_info->info[s].n_filt = 0;
}

static void get_tns ( Info*             info, 
                      TNS_frame_info*   tns_frame_info,
#ifdef VERSION2
                      HANDLE_RESILIENCE hResilience, 
                      HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                      HANDLE_BUFFER     hVm )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static short              sgn_mask[] = {     0x2, 0x4, 0x8     };
  static short              neg_mask[] = {     (short) 0xfffc, (short)0xfff8, (short)0xfff0     };

  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];

    if (!(tns_info->n_filt = GetBits ( short_flag ? 1 : 2,
#ifdef VERSION2
                                       N_FILT, 
                                       hResilience, 
                                       hEscInstanceData,
#endif /*VERSION2*/
                                       hVm )))
      continue;
            
    tns_info -> coef_res = res = GetBits ( 1,
#ifdef VERSION2
                                           COEF_RES, 
                                           hResilience, 
                                           hEscInstanceData,
#endif /*VERSION2*/
                                           hVm ) + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      top = tns_filt->start_band = top - GetBits ( short_flag ? 4 : 6,
#ifdef VERSION2
                                                   LENGTH, 
                                                   hResilience, 
                                                   hEscInstanceData,
#endif /*VERSION2*/
                                                   hVm );
      tns_filt->order = GetBits ( short_flag ? 3 : 5,
#ifdef VERSION2
                                  ORDER, 
                                  hResilience, 
                                  hEscInstanceData,
#endif /*VERSION2*/
                                  hVm );

      if (tns_filt->order)  {
        tns_filt->direction = GetBits ( 1,
#ifdef VERSION2
                                        DIRECTION, 
                                        hResilience, 
                                        hEscInstanceData,
#endif /*VERSION2*/
                                        hVm );
        compress = GetBits ( 1,
#ifdef VERSION2
                             COEF_COMPRESS, 
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = (short) GetBits ( res2,
#ifdef VERSION2
                                  COEF, 
                                  hResilience, 
                                  hEscInstanceData,
#endif /*VERSION2*/
                                  hVm );
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
}

/*
 * pulse noiseless coding
 */
struct Pulse_Info
{
  int pulse_data_present;
  int number_pulse;
  int pulse_start_sfb;
  int pulse_position[NUM_PULSE_LINES];
  int pulse_offset[NUM_PULSE_LINES];
  int pulse_amp[NUM_PULSE_LINES];
};
static struct Pulse_Info pulseInfo;

static void GetPulseNc ( struct Pulse_Info* pulseInfo,
#ifdef VERSION2
                         HANDLE_RESILIENCE  hResilience,
                         HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
#endif /*VERSION2*/
                         HANDLE_BUFFER      hVm )
{
  int i;
  pulseInfo->number_pulse = GetBits ( LEN_PULSE_NPULSE,
#ifdef VERSION2
                                      NUMBER_PULSE,
                                      hResilience, 
                                      hEscInstanceData,
#endif /*VERSION2*/
                                      hVm );
  pulseInfo->pulse_start_sfb = GetBits ( LEN_PULSE_ST_SFB,
#ifdef VERSION2
                                         PULSE_START_SFB, 
                                         hResilience, 
                                         hEscInstanceData,
#endif /*VERSION2*/
                                         hVm );
  for(i=0; i<pulseInfo->number_pulse + 1; i++) {
    pulseInfo->pulse_offset[i] = GetBits ( LEN_PULSE_POFF,
#ifdef VERSION2
                                           PULSE_OFFSET, 
                                           hResilience, 
                                           hEscInstanceData,
#endif /*VERSION2*/
                                           hVm );
    pulseInfo->pulse_amp[i] = GetBits ( LEN_PULSE_PAMP,
#ifdef VERSION2
                                        PULSE_AMP, 
                                        hResilience, 
                                        hEscInstanceData,
#endif /*VERSION2*/
                                        hVm );
  }
}

static void pulse_nc ( int*               coef, 
                       struct Pulse_Info* pulse_info
#ifdef VERSION2
                       ,HANDLE_RESILIENCE  hResilience,
                       HANDLE_ESC_INSTANCE_DATA     hEscInstanceData
#endif /*VERSION2*/
 )
{
  int i, k;
  
  if (pulse_info->pulse_start_sfb>0){
    k = only_long_info.sbk_sfb_top[0][pulse_info->pulse_start_sfb-1];
  }
  else{
    k = 0;
  }
  for(i=0; i<=pulse_info->number_pulse; i++) {
    k += pulse_info->pulse_offset[i];
    if ( k < LN2) {
      if (coef[k]>0) {
        coef[k] += pulse_info->pulse_amp[i];
      }
      else {
        coef[k] -= pulse_info->pulse_amp[i];
      }
    }
    else {
#ifdef VERSION2
      if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
        CommonExit( 2, "pulse_nc: pulse data cannot be processed on line %i (huffdec2.c)", k );
#ifdef VERSION2
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("pulse_nc: pulse data cannot be processed on line %i\n", k );
        }
      }
#endif /*VERSION2*/
    } 
  }
}

#ifdef SRS

static long get_gcBuf ( WINDOW_SEQUENCE          window_sequence, 
                 HANDLE_BSBITSTREAM       gc_streamCh,
#ifdef VERSION2
                 HANDLE_RESILIENCE        hResilience,
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
#endif /*VERSION2*/
                 HANDLE_BUFFER            hVm )
{
  unsigned int bd;
  int wd;
  unsigned int ad;
  unsigned long   max_band, natks, ltmp;
  int loc;

  loc = BsCurrentBit(gc_streamCh);
  max_band = GetBits ( NBANDSBITS,
#ifdef VERSION2
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience, 
                       hEscInstanceData,
#endif /*VERSION2*/
                       hVm );
  BsPutBit(gc_streamCh, max_band, NBANDSBITS);/*   0 < max_band <= 3 */

  switch (window_sequence) {
  case ONLY_LONG_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 1; wd++) {
        natks = GetBits ( NATKSBITS,
#ifdef VERSION2
                          MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                          hResilience, 
                          hEscInstanceData,
#endif /*VERSION2*/
                          hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          ltmp = GetBits ( ATKLOCBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
          BsPutBit(gc_streamCh, ltmp, ATKLOCBITS);
        }
      }
    }
    break;
  case LONG_START_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        natks = GetBits ( NATKSBITS,
#ifdef VERSION2
                          MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                          hResilience, 
                          hEscInstanceData,
#endif /*VERSION2*/
                          hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          if (wd == 0) {
            ltmp = GetBits ( ATKLOCBITS_START_A,
#ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_START_A);
          }
          else {
            ltmp = GetBits ( ATKLOCBITS_START_B,
#ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_START_B);
          }
        }
      }
    }
    break;
  case EIGHT_SHORT_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 8; wd++) {
        natks = GetBits (  NATKSBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          ltmp = GetBits (  ATKLOCBITS_SHORT,
#ifdef VERSION2
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience, 
                            hEscInstanceData,
#endif /*VERSION2*/
                            hVm );
          BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_SHORT);
        }
      }
    }
    break;
  case LONG_STOP_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        natks = GetBits (  NATKSBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          if (wd == 0) {
            ltmp = GetBits ( ATKLOCBITS_STOP_A,
#ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_STOP_A);
          }
          else {
            ltmp = GetBits ( ATKLOCBITS_STOP_B,
#ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_STOP_B);
          }
        }
      }
    }
    break;
  default:
    return  -1;
  }
  return BsCurrentBit(gc_streamCh) - loc;
}

#endif /*SRS*/

/*
 * read the codebook and boundaries
 */
static int huffcb ( byte*                    sect, 
                    Info                     info,
                    int*                     sectbits, 
                    int                      tot_sfb, 
                    int                      sfb_per_sbk, 
                    byte                     max_sfb,
                    unsigned char            numGroups,
#ifdef VERSION2
                    HANDLE_HCR               hHcrInfo,
                    HANDLE_RESILIENCE        hResilience,
                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                    HANDLE_CONCEALMENT       hConcealment,
#endif /*VERSION2*/
                    HANDLE_BUFFER            hVm)
{
  int            nsect;
  int            n;
  int            nrSfb;
  int            bits;
  int            len;
  unsigned char  group = 0;
  unsigned char  currentMaxSfb;
  unsigned short vcb11Flag;
  unsigned short index;
  unsigned short nrOfCodewords = 0;
  unsigned short dim;

  if (debug['s']) {
    printf("total sfb %d\n", tot_sfb);
    printf("   sect    top     cb\n");
  }
  bits = sectbits[0];
  len = (1 << bits) - 1;
  nsect = 0;
  nrSfb = 0;
  index = 0;
#ifdef VERSION2
  InitVcb11();
  vcb11Ptr = vcb11;
#endif /*VERSION2*/
  while ( ( nrSfb < tot_sfb ) && ( nsect < tot_sfb ) ) {
    currentMaxSfb = sfb_per_sbk * group + max_sfb;
    vcb11Flag = 0; /* virtual codebooks flag is false */
#ifdef VERSION2
    if(GetVcb11Flag( hResilience )) { /* virtual codebooks used */
      *sect = (unsigned char) GetBits ( LEN_CB_VCB11, 
                                        SECT_CB, 
                                        hResilience,  
                                        hEscInstanceData, 
                                        hVm ); 
    }
    else { /* section data transcoding off */
#endif /*VERSION2*/
      *sect = (unsigned char) GetBits ( LEN_CB,
#ifdef VERSION2
                                        SECT_CB, 
                                        hResilience, 
                                        hEscInstanceData,
#endif /*VERSION2*/
                                        hVm );
      
#ifdef VERSION2
    }
    ConcealmentDetectErrorCodebook(sect, hConcealment);
    if ( GetVcb11Flag ( hResilience ) ) {         /* virtual codebooks used */
      if( Vcb11Used( *sect ) ) {
        vcb11Flag = 1;                            /* virtual codebooks flag is set */
      }
      if( GetReorderSpecFlag ( hResilience ) ) { /* with hcr store in 'vcb11' all codebooks */
        vcb11[index] = *sect;                     /* store virtual codebook in the heap */
        index++;
      }
      else {                                     /* without hcr store only the virtual codebooks in 'vcb11' */
        if( vcb11Flag ) { 
           vcb11[index] = *sect;                  /* store virtual codebook in the heap */
           index++;
        }
      }
    }
    if( vcb11Flag ) {
      n = 1;                                     /* virtual codebooks are not combined */
    }
    else {                                       /* for non virtual codebooks read the number of occurrences */
#endif /*VERSION2*/
      n = GetBits ( bits,
#ifdef VERSION2
                    SECT_LEN_INCR, 
                    hResilience, 
                    hEscInstanceData,
#endif /*VERSION2*/
                    hVm );
#ifdef VERSION2
    }
    if ( n == 0 ) {
      ConcealmentDetectErrorSectionData(SECTIONLENGTH__EQ__ZERO, hConcealment);
      if ( ! hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf("huffcb: a section length of 0 (huffdec2.c)\n");
      }
    }
#endif /*VERSION2*/
    while ( n == len && nrSfb < currentMaxSfb ) { /* changed from < tot_sfb */
      nrSfb += len;
      n = GetBits ( bits,
#ifdef VERSION2
                    SECT_LEN_INCR, 
                    hResilience, 
                    hEscInstanceData,
#endif /*VERSION2*/
                    hVm );
    }
    nrSfb += n;
    sect++;
    *sect = nrSfb;
    sect++;
    /**/
    if ( sect[-1] > currentMaxSfb ) {
#ifdef VERSION2
      if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
        CommonExit( 2, "huffcb: group %i: nrOfScfBands (%2i) > currentMaxSfb (%2i) (huffdec2.c)",
                    group, sect[-1], currentMaxSfb );
#ifdef VERSION2
      }
      else {
        ConcealmentDetectErrorSectionData(NR_OF_SCF_BANDS__GT__CURRENT_MAX_SFB, hConcealment);
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("huffcb: group %i: nrOfScfBands (%2i) > currentMaxSfb (%2i)\n", group, sect[-1], currentMaxSfb );
          printf("huffcb: --> nrOfScfBands has been clipped at %2i\n", currentMaxSfb );
        }
        sect[-1] = currentMaxSfb;
      }
#endif /*VERSION2*/
    }
    /* derivation of nrOfCodewords for HCR_SQUARE */
    if ( ! ( ( sect[-2] == ZERO_HCB ) ||
#ifdef PNS
             ( sect[-2] == NOISE_HCB ) ||
#endif
             ( sect[-2] == INTENSITY_HCB ) ||
             ( sect[-2] == INTENSITY_HCB2 ) ||
             ( sect[-2] == BOOKSCL ) ) ) {
      if( sect[-2] < BY4BOOKS + 1 ) {
        dim = 4;
      }
      else {
        dim = 2;
      }
      if ( sect[-2] ) {
        if ( ! nsect ) {
          nrOfCodewords = (info.bk_sfb_top[sect[-1]-1] ) / dim;
        }
        else {
          nrOfCodewords += ( info.bk_sfb_top[sect[-1]-1] - info.bk_sfb_top[sect[-3]-1] ) / dim;
        }
      }
    }
    /**/
    nsect++;
    if (debug['s'] ) {
      printf(" %6d %6d %6d %6d %6d\n", 
             nsect, 
             sect[-1], 
             sect[-2], 
             info.bk_sfb_top[sect[-1]-1], 
             nrOfCodewords );
    }
    /* insert a zero section for regions above max_sfb for each group */
    if ( sect[-1] == currentMaxSfb ) {           /* changed from ((sect[-1] % sfb_per_sbk) == max_sfb) */
      nrSfb = ( sfb_per_sbk * ( group + 1 ) );    /* changed from nrSfb += (sfb_per_sbk - max_sfb) */
      group++;
      if ( currentMaxSfb != sfb_per_sbk * group ) { /* this inserts a section with codebook 0 */
        *sect++ = 0;
        *sect++ = nrSfb;
        nsect++;
#ifdef VERSION2
        if ( GetVcb11Flag ( hResilience ) && GetReorderSpecFlag ( hResilience ) ) {
          vcb11[index] = 0;
          index++;
        }
#endif /*VERSION2*/
        if (debug['s'])
          printf("(%6d %6d %6d)\n", nsect, sect[-1], sect[-2]);
      }
    }
  }
  if ( nrSfb != tot_sfb ) {
#ifdef VERSION2
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
      CommonExit ( 2, "huffcb: nrSfb (%3hu) != totSfb (%3hu)", nrSfb, tot_sfb );
#ifdef VERSION2
    }
    else {
      ConcealmentDetectErrorSectionData(NR_SFB__NE__TOT_SFB, hConcealment);
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf ( "huffcb: nrSfb (%3hu) != totSfb (%3hu)\n", nrSfb, tot_sfb );
        printf( "huffcb: --> something should be done here\n");
      }
    }
#endif /*VERSION2*/
    return 0;
  }
  if ( nsect > tot_sfb ) {
#ifdef VERSION2
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
      CommonExit ( 2, "huffcb: nsect (%3hu) > totSfb (%3hu)", nsect, tot_sfb );
#ifdef VERSION2
    }
    else {
      ConcealmentDetectErrorSectionData(NSECT__GT__TOTSFB, hConcealment);
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf("huffcb: nsect (%3hu) > totSfb (%3hu)\n", nsect, tot_sfb );
        printf( "huffcb: --> nsect has been clipped at %3hu\n", tot_sfb );
      }
      nsect = tot_sfb;
    }
#endif /*VERSION2*/
    return 0;
  }
  if ( max_sfb && ( group != numGroups ) ) {
#ifdef VERSION2
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
      CommonExit ( 2, "huffcb: group (%hu) != numGroups (%hu) (huffdec2.c)", group, numGroups );
#ifdef VERSION2
    }
    else {
      ConcealmentDetectErrorSectionData(GROUP__NE__NUMGROUPS, hConcealment);
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf ( "huffcb: group (%hu) != numGroups (%hu)\n", group, numGroups );
        printf ( "huffcb: --> something should be done here\n");
      }
    }
#endif /*VERSION2*/
  }
#ifdef VERSION2
  SetNrOfCodewords ( nrOfCodewords,
                     hHcrInfo );
#endif /*VERSION2*/
  return ( nsect );
}
static int getescape ( int                q,
#ifdef VERSION2
                       HANDLE_HCR         hHcrInfo, 
                       HANDLE_RESILIENCE  hResilience,
                       HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
#endif /*VERSION2*/
                       HANDLE_BUFFER      hVm )
{
  int           i;
  int           off;
  int           neg;
  unsigned long tmp; /*error robustness, sfr */

  if ( q < 0 ) {
    if ( q != -16 ) {
      return ( q );
    }
    neg = 1;
  } 
  else {
    if ( q != +16 ) {
      return ( q );
    }
    neg = 0;
  }

  for ( i = 4; ; i++ ) {
#ifdef VERSION2
    if ( GetReorderSpecFlag ( hResilience )) {
      tmp = GetBits ( 1, 
                      MAX_ELEMENTS, 
                      hResilience, 
                      hEscInstanceData, 
                      hVm );
    }
    else
#endif /*VERSION2*/
      {
        tmp = GetBits ( 1, 
#ifdef VERSION2
                        HCOD_ESC, 
                        hResilience, 
                        hEscInstanceData, 
#endif /*VERSION2*/
                        hVm );
      }
    if ( tmp == 0 ) { /* truncation condition */
      break;
    }
    if ( i >= 12 ) {
      /* if i == 12 and no escape separator has been read -> break,
         because the escape word has a maximum length of Nmax + 4, with Nmax = 8 --> imax = 12 */
#ifdef VERSION2
      if ( GetEPFlag ( hResilience ) 
           || ( GetConsecutiveReorderSpecFlag ( hResilience ) && GetReorderStatusFlag (hHcrInfo ) ) ) {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("getescape: escape_prefix longer than 8 bit\n");
          printf("getescape: --> length of escape_prefix is set to 8\n");
        }
      }
      else {
        CommonExit(2, "getescape: escape_prefix longer than 8 bit (huffdec2.c)");
      }
#else /*VERSION2*/
      CommonExit(2,"getescape: escape_prefix longer than 8 bit (huffdec2.c)");
#endif /*VERSION2*/
      break; 
    }
  }
#ifdef VERSION2
  if ( GetReorderSpecFlag ( hResilience )) {
    off = GetBits ( i,
                    MAX_ELEMENTS, 
                    hResilience, 
                    hEscInstanceData,
                    hVm );
  }
  else 
#endif /*VERSION2*/
    {
      off = GetBits ( i,
#ifdef VERSION2
                      HCOD_ESC, 
                      hResilience, 
                      hEscInstanceData,
#endif /*VERSION2*/
                      hVm );
    }
  i = off + (1<<i);
  if(neg)
    i = -i;
  return i;
}

unsigned short  HuffSpecKernelPure ( int*                     qp, 
                                     Hcb*                     hcb, 
                                     Huffman*                 hcw, 
                                     int                      step,
#ifdef VERSION2
                                     HANDLE_HCR               hHcrInfo,
                                     HANDLE_RESILIENCE        hResilience,
                                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                     HANDLE_CONCEALMENT       hConcealment,
#endif /*VERSION2*/ 
                                     HANDLE_BUFFER            hSpecData )
                   
{
  int            temp;
  unsigned long  codewordBegin;
  unsigned short codewordLength;

#ifdef VERSION2
  ConcealmentTouchLine ( step,
                         hConcealment );
#endif /*VERSION2*/
  codewordBegin = GetReadBitCnt ( hSpecData );
  temp = decode_huff_cw ( hcw,
                          1, 
#ifdef VERSION2
                          hResilience, 
                          hEscInstanceData,
#endif /*VERSION2*/
                          hSpecData );
  unpack_idx(qp, temp, hcb);
  
  if (!hcb->signed_cb)
    get_sign_bits ( qp, 
                    step,
#ifdef VERSION2
                    hResilience, 
                    hEscInstanceData,
#endif /*VERSION2*/
                    hSpecData );
  if ( hcw == &book11[0] ) {
/*   if(codebook == ESCBOOK){ */
    qp[0] = getescape ( qp[0],
#ifdef VERSION2
                        hHcrInfo, 
                        hResilience,
                        hEscInstanceData,
#endif /*VERSION2*/
                        hSpecData);
    qp[1] = getescape ( qp[1],
#ifdef VERSION2
                        hHcrInfo, 
                        hResilience,
                        hEscInstanceData,
#endif /*VERSION2*/
                        hSpecData);
  }
  /* section data coding of codebook 11: checks max/min values within a SFB, if codebook 11 is used */

  codewordLength = (unsigned short) (GetReadBitCnt ( hSpecData ) - codewordBegin);
#ifdef VERSION2
  if ( GetLenOfLongestCwFlag( hResilience ) && 
       ( codewordLength > GetLenOfLongestCw ( hHcrInfo ) ) ) {
    if ( ! GetConsecutiveReorderSpecFlag ( hResilience ) ||       /* errors are not allowed, if HCR_SQUARE is disabled */
         ! GetReorderStatusFlag ( hHcrInfo ) ) {                  /* errors are not allowed in PCWs */
      if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
        CommonExit ( 2, "HuffSpecKernelPure: codewordLength (%hu) > lengthOfLongestCodeword (%hu))", codewordLength, GetLenOfLongestCw ( hHcrInfo ) );
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf ("HuffSpecKernelPure: codewordLength (%hu) > lengthOfLongestCodeword (%hu))\n", codewordLength, GetLenOfLongestCw ( hHcrInfo ) );
        }
      }
    }
  }
#endif /*VERSION2*/
  return ( codewordLength );
}

static unsigned short HuffSpecKernelNoReordering ( int*               qp, 
                                                   Hcb*               hcb,
                                                   Huffman*           hcw,
                                                   unsigned short     step,
#ifdef VERSION2
                                                   unsigned short     codebook,
                                                   HANDLE_RESILIENCE  hResilience,
                                                   HANDLE_HCR         hHcrInfo,
                                                   HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                                   HANDLE_CONCEALMENT hConcealment,
#endif /*VERSION2*/
                                                   HANDLE_BUFFER      hSpecData )
{
  unsigned short codewordLength;
  codewordLength = HuffSpecKernelPure ( qp, 
                                        hcb, 
                                        hcw, 
                                        step, 
#ifdef VERSION2
                                        hHcrInfo,
                                        hResilience, 
                                        hEscInstanceData,
                                        hConcealment,
#endif /*VERSION2*/
                                        hSpecData );
#ifdef VERSION2
    ConcealmentDetectErrorCodeword ( step,
                                     codewordLength, 
                                     hcb->maxCWLen,
                                     hcb->lavInclEsc,
                                     codebook,
                                     qp,
                                     CODEWORD_NON_PCW,
                                     hResilience,
                                     hHcrInfo,
                                     hConcealment );
  Vcb11ConcealmentPatch ( "HuffSpecKernelNoReordering",
                          codebook,
                          hcb->lavInclEsc,
                          qp,
                          step,
                          hResilience );
#endif /*VERSION2*/
  return ( codewordLength );
}

#ifdef VERSION2
static unsigned short HuffSpecKernelInclReordering ( int*               qp, 
                                                     Hcb*               hcb, 
                                                     Huffman*           hcw, 
                                                     unsigned short     step, 
                                                     unsigned short     codebook,
                                                     unsigned short*    codewordsInSet,
                                                     HANDLE_BUFFER      hSpecData,
                                                     HANDLE_RESILIENCE  hResilience,
                                                     HANDLE_HCR         hHcrInfo,
                                                     HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                                     HANDLE_CONCEALMENT hConcealment)
{
  unsigned short codewordLength = 0;
  
  StoreBufferPointer ( hSpecData );
  if ( ! GetReorderStatusFlag ( hHcrInfo ) ) {
    ReorderSpecDecPCWFinishedCheck ( hcb->maxCWLen,
                                     hSpecData,
                                     hResilience,
                                     hHcrInfo,
                                     hEscInstanceData );
  }
  if ( ! GetReorderStatusFlag ( hHcrInfo ) ) {
    ReadPcws ( &codewordLength,
               qp,
               hcb,
               hcw,
               codebook,
               hcb->maxCWLen,
               step, 
               hResilience,
               hSpecData,
               hHcrInfo, 
               hEscInstanceData,
               hConcealment );
  }
  else {
    if ( GetConsecutiveReorderSpecFlag ( hResilience ) ) {
      ReadNonPcwsNew ( &codewordLength,
                       qp, 
                       hcb, 
                       hcw, 
                       step, 
                       codebook,
                       codewordsInSet,
                       hResilience, 
                       hHcrInfo,
                       hEscInstanceData,
                       hConcealment );
    }
    else {
      ReadNonPcws ( &codewordLength,
                    qp, 
                    hcb, 
                    hcw, 
                    step, 
                    codebook,
                    hResilience, 
                    hHcrInfo,
                    hEscInstanceData,
                    hConcealment );
    }
  }
  return ( codewordLength );
}
#endif /*VERSION2*/

static unsigned short HuffSpecDecDefault ( Info*              info, 
                                           byte*              sect, 
                                           int                nsect, 
                                           int                quant[],
#ifdef VERSION2
                                           HANDLE_RESILIENCE  hResilience,
                                           HANDLE_HCR         hHcrInfo,
                                           HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                           HANDLE_CONCEALMENT hConcealment,
#endif /*VERSION2*/
                                           HANDLE_BUFFER      hSpecData )
{
  short*         bandp;
  short*         bands;
  unsigned short bottom;
  unsigned short lineCnt; 
  unsigned short lineStart;
  unsigned short sectionCnt;
  unsigned short sfbCnt;
  unsigned short step;
  unsigned short stop;
  unsigned short codebook;
  unsigned short top;
  int*           qp;      /* probably could be short */
  Hcb*           hcb;
  Huffman*       hcw;
  unsigned short codewordLength;
  unsigned short lengthOfLongestCodeword = 0;

  bands = info->bk_sfb_top;
  bottom = 0;
  lineCnt = 0;
  bandp = bands;
  for( sectionCnt = nsect; sectionCnt; sectionCnt-- ) {
    codebook = sect[0];  /* codebook for this section */
    top   = sect[1];
    sect += 2;
    if ( codebook == BOOKSCL ) {
#ifdef VERSION2
      if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
        CommonExit ( 2, "HuffspecDecDefault: book %d: invalid book (huffdec2.c, HuffSpecDecDefault())", BOOKSCL );
#ifdef VERSION2
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf ("HuffspecDecDefault: book %d: invalid book (huffdec2.c, HuffSpecDecDefault())\n", BOOKSCL );
          printf( "HuffspecDecDefault: --> book has been set to %d\n", ZERO_HCB );
        }
        codebook = ZERO_HCB;
      }
#endif /*VERSION2*/
    }
    if( ( codebook == ZERO_HCB ) ||
#ifdef PNS
        ( codebook == NOISE_HCB ) ||
#endif
        ( codebook == INTENSITY_HCB ) ||
        ( codebook == INTENSITY_HCB2 ) ) {
      bandp = bands+top;
      lineCnt = bandp[-1];
      bottom = top;
      continue;
    }
    if(codebook < BY4BOOKS+1)
      step = 4;
    else 
      step = 2;
    hcb       = &book[codebook];
    hcw       = hcb->hcw;
    qp        = quant       + lineCnt;
#ifdef VERSION2
    ConcealmentSetLine(lineCnt, hConcealment);
#endif /*VERSION2*/
    lineStart = lineCnt;
    for ( sfbCnt = bottom; sfbCnt < top; sfbCnt++ ) {
      stop = *bandp++;
      while ( lineCnt < stop ) {
        codewordLength = HuffSpecKernelNoReordering ( qp, 
                                                      hcb, 
                                                      hcw, 
                                                      step, 
#ifdef VERSION2
                                                      codebook,
                                                      hResilience, 
                                                      hHcrInfo,
                                                      hEscInstanceData,
                                                      hConcealment,
#endif /*VERSION2*/
                                                      hSpecData );
        lengthOfLongestCodeword = MAX ( codewordLength, lengthOfLongestCodeword );
        qp       += step;
#ifdef VERSION2
#ifdef TESTSECTIONSWITCHING
        if (((bno % 2 == 0) && (sectionCnt % 2 == 0)) ||
            ((bno % 2 == 1) && (sectionCnt % 2 == 1)))
          ConcealmentSetErrorCodeword(step, hConcealment);
#endif
        ConcealmentIncrementLine(step, hConcealment);
#endif /*VERSION2*/
        lineCnt  += step;
      } 
#ifdef VERSION2
      if( (GetVcb11Flag( hResilience )) && hcw == &book11[0]  ) {
        /* section data coding on && codebook 11 section ready, skip to next virtual codebook */
        vcb11Ptr++;
      }
#endif /*VERSION2*/
      if(debug['q']){
        unsigned short idx;
        printf("\nsect %d %d\n", codebook, lineStart);
        for (idx = lineStart; idx < lineCnt; idx += 2 )
          printf("%d %d  ",quant[idx],quant[idx+1]);
        printf("\n");
      }
    }
    bottom = top;
  }
  return ( lengthOfLongestCodeword ); 
}

#ifdef VERSION2
static unsigned short HuffSpecDecInterleaved ( Info*              info, 
                                               byte*              sect, 
                                               int                nsect, 
                                               int                quant[], 
                                               HANDLE_BUFFER      hHcrSpecData,
                                               HANDLE_RESILIENCE  hResilience,
                                               HANDLE_HCR         hHcrInfo,
                                               HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                               HANDLE_CONCEALMENT hConcealment)
{
  Hcb*                  hcb = NULL;
  Huffman*              hcw = NULL;
  int*                  qp;
  unsigned short        step;
  volatile int          i; /* volatile is necessary for Optimization only, probably due to compiler bug
                              gcc version 2.95.2 19991024 (release) */
  int                   j;
  unsigned short        codeNr                 = 0;  /* code counter */
  unsigned short        unitsInSpectralDirection;    /* number of units in spectral direction */
  unsigned short        unitNrInSpectralDirection;   /* unit counter in spectral direction */
  unsigned short        sfb            [NSHORT];     /* number of current sfb           for each group */
  unsigned short        spectralLine   [NSHORT];     /* number of current spectral line for each group */
  unsigned short        codebookArray  [NSHORT];     /* number of current codebook      for each group */
  unsigned short        section        [NSHORT];     /* number of current section       for each group */
  unsigned short        groupNr;                     /* number of current group */
  unsigned short        groupNrPast;                 /* number of past groups */
  unsigned short        codewordLength;
  unsigned short        lengthOfLongestCodeword = 0;
  int                   vcb11Copy[VCB11_ARRAY_SIZE]; /* second virtual codebooks pointer */
  unsigned short        index;
  unsigned short        preSortingIndex;
  unsigned short        firstLineForConcealment = 0;
  unsigned short        codewordsInSet          = 0;

  struct {
    unsigned short min;
    unsigned short max;
  } preCodebookSorting[] = {{ 11, 11},
                            { 31, 31},
                            { 30, 30},
                            { 29, 29},
                            { 28, 28},
                            { 27, 27},
                            { 26, 26},
                            { 25, 25},
                            { 24, 24},
                            { 23, 23},
                            { 22, 22},
                            { 21, 21},
                            { 20, 20},
                            { 19, 19},
                            { 18, 18},
                            { 17, 17},
                            { 16, 16},
                            {  9, 10},
                            {  7,  8},
                            {  5,  6},
                            {  3,  4},
                            {  1,  2}};

  for(j = 0; j < VCB11_ARRAY_SIZE; j++) {
    vcb11Copy[j] = vcb11[j]; /* copy the current array */
  }
  InitVcb11(); /* init the current array with zeroes */

  if ( ! nsect )
    return ( 0 );
#if 0
  {
    unsigned short i;
    printf("info->num_groups = %hi\n",info->num_groups);
    for ( i = 0; i < info->num_groups; i++ )
      printf("info->group_len[%i] = %hi\n", i, info->group_len[i]);
    for ( i = 0; i < 200; i++)
      printf("info->bk_sfb_top[%3i] = %4hi\n", i, info->bk_sfb_top[i]);
    for ( i = 0; i < MAXBANDS; i += 2 )
      printf("codebook : %2i, till band : %2i\n", sect[i], sect[i+1]);
    for ( groupNr = 0; groupNr < info->num_groups ; groupNr++ )
      printf("%hu   ", sfb[groupNr]);
    printf("\n");
  }
#endif
  if (debug['i']) {
    if ( info->islong ) {
      printf("long block interleaving:\n\n");
    }
    else {
      printf("short block interleaving:\n\n");
    }
  }
  if ( info->islong ) {
    unitsInSpectralDirection = LN2 / 4;
  }
  else {
    unitsInSpectralDirection = SN2 / 4;
  }
  for ( preSortingIndex = 0; preSortingIndex < sizeof(preCodebookSorting)/ ( sizeof(short) * 2 ); preSortingIndex++) {
    qp       = quant;
    ConcealmentSetLine(0, hConcealment);
    firstLineForConcealment = 0;
    /* initialization */
    shortclr ( (short*) spectralLine, NSHORT );
    shortclr ( (short*) codebookArray,   NSHORT );
    shortclr ( (short*) section,      NSHORT );
    for ( groupNr = 0; groupNr < info->num_groups ; groupNr++ ) {               
      sfb[groupNr] = groupNr * info->sfb_per_sbk[0];                 /* initialization of sfb[]
                                                                        with number of first sfb within each group */
      for ( groupNrPast = 0; groupNrPast < groupNr; groupNrPast++ ) {/* initialization of spectralLine[] */
        spectralLine[groupNr] += SN2 * info->group_len[groupNrPast]; /* with number of first spectral line within each group */
      }
    }                                                               
    /* loops */
    for ( unitNrInSpectralDirection = 0; 
          unitNrInSpectralDirection < unitsInSpectralDirection; 
          unitNrInSpectralDirection ++ ) {  
      for ( groupNr = 0; groupNr < info->num_groups; groupNr++ ) {         /* for each group in temporal direction */
        while ( info->bk_sfb_top[sfb[groupNr]] <= spectralLine[groupNr] ) {/* trace sfb of current group */
          sfb[groupNr]++;
        }
        while ( sect[2 * section[groupNr] + 1] <= sfb[groupNr] ) {       /* trace section/codebook of current group */
          section[groupNr]++;
        }
        codebookArray[groupNr] = sect[2 * section[groupNr]];
        /* correct the virtual codebooks series */
        if( (codebookArray[groupNr] == 11)&&(GetVcb11Flag( hResilience )) ) {
          index = section[groupNr];
          vcb11[groupNr] = vcb11Copy[index];
        }
        if ( codebookArray[groupNr] == BOOKSCL ) {
          if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
            CommonExit ( 2, "HuffSpecDecInterleaved: book %d: invalid book (huffdec2.c, HuffSpecDecInterleaved())", BOOKSCL );
          }
          else {
            if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
              printf( "HuffSpecDecInterleaved: book %d: invalid book (huffdec2.c, HuffSpecDecInterleaved())\n", BOOKSCL );
              printf( "HuffSpecDecInterleaved: --> book has been set to %d\n", ZERO_HCB );
            }
            codebookArray[groupNr] = ZERO_HCB;
          }
        }
        if( ! ( ( codebookArray[groupNr] == ZERO_HCB ) ||
#ifdef PNS
                ( codebookArray[groupNr] == NOISE_HCB ) ||
#endif
                ( codebookArray[groupNr] == INTENSITY_HCB ) ||
                ( codebookArray[groupNr] == INTENSITY_HCB2 ) ) ) {
          hcb = &book[codebookArray[groupNr]];      /* init hcb (pointer) with huffman codebook according codebook */
          hcw = hcb->hcw;                        /* init hcw (pointer) with array of codewords of current codebook */
          if ( codebookArray[groupNr] < BY4BOOKS + 1 )                       /* decide between 2 and 4 dimensional codewords */
            step = 4;
          else 
            step = 2;
        }
        else {
          step = 4;
        }
        for ( j = 0; j < info->group_len[groupNr]; j++ ) {             /* for each unit of the current group in temporal direction */
          for ( i = 0; i < 4; i += step ) {                            /* for each codeword of the current unit (4 spectral lines) */
            if( ! ( ( codebookArray[groupNr] == ZERO_HCB ) ||
#ifdef PNS
                    ( codebookArray[groupNr] == NOISE_HCB ) ||
#endif
                    ( codebookArray[groupNr] == INTENSITY_HCB ) ||
                    ( codebookArray[groupNr] == INTENSITY_HCB2 ) ) ) {
              if ( ( ! GetReorderSpecPreSortFlag ( hResilience ) ) ||
                   ( codebookArray[groupNr] >= preCodebookSorting[preSortingIndex].min &&
                     codebookArray[groupNr] <= preCodebookSorting[preSortingIndex].max ) ) {
                if (debug['i']) {
                  printf("code %3hu, group %1hu, section %2hu, sfb %2hu (%2hu), codebook %2i", 
                        codeNr,                                       /* comparison with wrapper */
                        groupNr,                                      /* comparison with wrapper */
                        section       [groupNr],                      /* comparison with wrapper */
                        sfb           [groupNr],                      /* comparison with wrapper */
                        sfb           [groupNr]%info->sfb_per_sbk[0], /* comparison with wrapper */
                        codebookArray [groupNr]                       /* comparison with wrapper */
                        );
#if 0
                  printf(", first spectralLine %4hu", spectralLine[groupNr]);
#endif
                  printf("\n");
                }
                codewordLength = HuffSpecKernelInclReordering ( qp, 
                                                                hcb, 
                                                                hcw, 
                                                                step, 
                                                                codebookArray[groupNr],
                                                                &codewordsInSet,
                                                                hHcrSpecData,
                                                                hResilience,
                                                                hHcrInfo, 
                                                                hEscInstanceData,
                                                                hConcealment );
                lengthOfLongestCodeword = MAX ( codewordLength, lengthOfLongestCodeword ) ;
              }
              codeNr++;
            }
            qp += step;
            firstLineForConcealment += step;
            ConcealmentIncrementLine(step, hConcealment);  
            /* alternative possibility:
               ConcealmentSetLine ( firstLineForConcealment, hConcealment );
            */
            /* this check works only, if ERRORCONCEALMENT = 1 */
/*             if ( firstLineForConcealment != ConcealmentGetLine (hConcealment)) { */
/*               CommonExit (1, "IMPLEMENTATION ERROR: \n%4d != %4d", firstLineForConcealment, ConcealmentGetLine (hConcealment) ); */
/*             } */
            spectralLine[groupNr] += step;
          }
        }
      }
    }
    if ( ! GetReorderSpecPreSortFlag ( hResilience ) ) {
      break;
    }
  }
  CheckDecodingProgress( hHcrSpecData,
                         hEscInstanceData, 
                         hHcrInfo, 
                         hResilience );

  return ( lengthOfLongestCodeword );
}
#endif /*VERSION2*/

static Float esc_iquant(int q)
{
  if (q > 0) {
    if (q < MAX_IQ_TBL) {
      return((Float)iq_exp_tbl[q]);
    }
    else {
      return(pow(q, 4./3.));
    }
  }
  else {
    q = -q;
    if (q < MAX_IQ_TBL) {
      return((Float)(-iq_exp_tbl[q]));
    }
    else {
      return(-pow(q, 4./3.));
    }
  }
}
 
static int HuffSpecFrame ( Info*              info, 
                           int                nsect, 
                           byte*              sect, 
                           short*             factors, 
                           Float*             coef,
                           int                max_spec_coefficients,
#ifdef VERSION2
                           HANDLE_RESILIENCE  hResilience,
                           HANDLE_BUFFER      hHcrSpecData,
                           HANDLE_CONCEALMENT hConcealment,
                           HANDLE_HCR         hHcrInfo,
                           HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
#endif /*VERSION2*/
                           HANDLE_BUFFER      hVm )
{
  int            quant[LN2]; 
  unsigned short lengthOfLongestCodeword;
  
  intclr  ( quant, LN2);
  
#ifdef VERSION2
  if ( GetReorderSpecFlag ( hResilience ) ) {
    ResetWriteBitCnt ( hHcrSpecData );
    PrepareWriting ( hHcrSpecData );
    TransferBits ( GetLenOfSpecData ( hHcrInfo ), 
                   REORDERED_SPECTRAL_DATA,
                   hVm,
                   hHcrSpecData,
                   hEscInstanceData, 
                   hResilience );
    InitHcr ( hHcrSpecData, hHcrInfo );
    lengthOfLongestCodeword = HuffSpecDecInterleaved ( info, 
                                                       sect, 
                                                       nsect, 
                                                       quant, 
                                                       hHcrSpecData,
                                                       hResilience,
                                                       hHcrInfo, 
                                                       hEscInstanceData,
                                                       hConcealment );
  }
  else
#endif /*VERSION2*/
    lengthOfLongestCodeword = HuffSpecDecDefault ( info, 
                                                   sect, 
                                                   nsect, 
                                                   quant, 
#ifdef VERSION2
                                                   hResilience, 
                                                   hHcrInfo,
                                                   hEscInstanceData,
                                                   hConcealment, 
#endif /*VERSION2*/
                                                   hVm );
#ifdef VERSION2
  if ( hHcrInfo && hEscInstanceData && ! ( BsGetCrcResultInFrameFlagEP ( hEscInstanceData ) || BsGetReadErrorInFrameFlagEP ( hEscInstanceData ) ) &&
       ( hResilience && GetLenOfLongestCwFlag ( hResilience ) ) &&
       ( lengthOfLongestCodeword != GetLenOfLongestCw ( hHcrInfo ) ) ) {
    if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
      printf("HuffSpecFrame: lengthOfLongestCodeword (%hu) != transmittedLengthOfLongestCodeword (%hu) (HuffSpecDecDefault()\n",
             lengthOfLongestCodeword,
             GetLenOfLongestCw ( hHcrInfo ) );
    }
  }

  if ( GetReorderSpecFlag ( hResilience ) ) {
    ConcealmentCheckLenOfHcrSpecData ( hHcrInfo,
                                       hConcealment );
  }
#endif /*VERSION2*/

  /* check number of coefficients (only 12 allowed for LFE) */
  if (max_spec_coefficients>0) {
    int x;
    for (x=max_spec_coefficients; x<LN2; x++) {
      if (quant[x] != 0) max_spec_coefficients=-1;
    }
    if (max_spec_coefficients==-1) printf("WARNING: more non-zero spectral lines than expected!\n");
  }

  /* pulse noisless coding reconstruction */
  if ( (info->islong) && (pulseInfo.pulse_data_present) )
    pulse_nc(quant, &pulseInfo
#ifdef VERSION2
             ,hResilience, hEscInstanceData
#endif /*VERSION"*/
             );

  if (!info->islong) {
    int tmp_spec[LN2];
    deinterleave ( quant,
                   tmp_spec,
                   sizeof(int),
                   info->group_len,
                   info->sfb_per_sbk,
                   info->sfb_width_short,
#ifdef VERSION2
                   info->islong,
                   hResilience,
#endif /*VERSION2*/
                   info->num_groups );
    memcpy(quant,tmp_spec,sizeof(tmp_spec));
  }
#ifdef VERSION2
  ConcealmentDeinterleave (info, hResilience, hConcealment);
#endif /*VERSION2*/

  if ( debug['Q'] ) {
    PrintSpectralValues ( quant );
  }
#ifdef VERSION2
#ifdef SENSITIVITY
  if ( hEscInstanceData && hConcealment ) {
    ResilienceDealWithSpectralValues ( quant,
                                       hConcealment,
                                       bno );
  }
#endif /*SENSITIVITY*/
#endif /*VERSION2*/

  /* inverse quantization */
  {
    unsigned short i;
    for (i=0; i<info->bins_per_bk; i++) {
      coef[i] = esc_iquant(quant[i]);
    }
  }
  /* rescaling */
  {
    unsigned short i;
    unsigned short k;
    int sbk, nsbk, sfb, nsfb, fac, top;
    Float *fp, scale;

    i = 0;
    fp = coef;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) {
      nsfb = info->sfb_per_sbk[sbk];
      k=0;
      for (sfb=0; sfb<nsfb; sfb++) {
        top = info->sbk_sfb_top[sbk][sfb];
        fac = factors[i++]-SF_OFFSET;

        if (fac >= 0 && fac < TEXP) {
          scale = exptable[fac];
        }
        else {
          if (fac == -SF_OFFSET) {
            scale = 0;
          }
          else {
            scale = pow( 2.0,  0.25*fac );
          }
        }
        for ( ; k<top; k++) {
          *fp++ *= scale;
        }
      }
    }
  }
  return 1;
}

int getics ( Info*                    info, 
             int                      common_window, 
             WINDOW_SEQUENCE*         win, 
             WINDOW_SHAPE*            wshape, 
             byte*                    group, 
             byte*                    max_sfb, 
             PRED_TYPE                pred_type, 
             int*                     lpflag, 
             int*                     prstflag, 
             byte*                    cb_map, 
             Float*                   coef, 
             int                      max_spec_coefficients,
             short*                   global_gain, 
             short*                   factors,
             NOK_LT_PRED_STATUS*      nok_ltp, 
             TNS_frame_info*          tns,
             HANDLE_BSBITSTREAM              gc_streamCh, 
             enum AAC_BIT_STREAM_TYPE bitStreamType,
#ifdef VERSION2
             HANDLE_RESILIENCE        hResilience,
             HANDLE_BUFFER            hHcrSpecData,
             HANDLE_HCR               hHcrInfo,
             HANDLE_ESC_INSTANCE_DATA           hEscInstanceData,
             HANDLE_CONCEALMENT       hConcealment,
             QC_MOD_SELECT            qc_select,
#endif /*VERSION2*/
             HANDLE_BUFFER            hVm )

{
  int            nsect;
  int            i;
  int            cb;
  int            top;
  int            bot;
  int            tot_sfb;
  byte           sect[ 2*(MAXBANDS+1) ];
  /*
   * global gain
   */
  *global_gain = (short) GetBits ( 8,
#ifdef VERSION2
                                   GLOBAL_GAIN,
                                   hResilience, 
                                   hEscInstanceData,
#endif /*VERSION2*/
                                   hVm );

  if (debug['f'])
    printf("global gain: %3d\n", *global_gain);
  if ( bitStreamType != SCALABLE ){
    memcpy(info, winmap[*win], sizeof(Info));
  }
  if (( !common_window ) && ( bitStreamType != SCALABLE )) {
    get_ics_info ( win, 
                   wshape, 
                   group, 
                   max_sfb, 
                   pred_type, 
                   lpflag, 
                   prstflag, 
                   bitStreamType,
#ifdef VERSION2
                   qc_select,
                   hResilience,
                   hEscInstanceData,
#endif /*VERSION2*/
                   nok_ltp,
		   NULL,
		   common_window,
                   hVm );

    memcpy(info, winmap[*win], sizeof(Info));
  } else {
    if( info->nsbk == 1 ) {
      group[0] = 1;
    } else {
      int sum = 0;
      for( i=0; i<info->num_groups; i++ ) {
        sum  += info->group_len[i];
        group[i]  = sum;
      }
    }
  }

  /* calculate total number of sfb for this grouping */
  if (*max_sfb == 0) {
    tot_sfb = 0;
  }
  else {
    i=0;
    tot_sfb = info->sfb_per_sbk[0];
    if (debug['f'])printf("tot sfb %d %d\n", i, tot_sfb);
    while (group[i++] < info->nsbk) {
      tot_sfb += info->sfb_per_sbk[0];
      if (debug['f'])printf("tot sfb %d %d\n", i, tot_sfb);
    }
  }

  /* calculate band offsets
   * (because of grouping and interleaving this cannot be
   * a constant: store it in info.bk_sfb_top)
   */
  calc_gsfb_table(info, group);

  /* 
   * section data
   */
  nsect = huffcb ( sect, 
                   *info,
                   info->sectbits, 
                   tot_sfb, 
                   info->sfb_per_sbk[0], 
                   *max_sfb,
                   info->num_groups,
#ifdef VERSION2
                   hHcrInfo,
                   hResilience, 
                   hEscInstanceData,
                   hConcealment,
#endif /*VERSION2*/
                   hVm);
  if( ( nsect == 0 ) && ( *max_sfb > 0 ) ) {
#ifdef VERSION2
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
      CommonExit ( 2, "getics: nsect = 0 but maxSfb = %hu", *max_sfb);
#ifdef VERSION2
    }
    else {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf ( "getics: nsect = 0 but maxSfb = %hu\n", *max_sfb);
        printf ( "getics: --> maxSfb has been set to 0\n" );
      }
      *max_sfb = 0;
    }
#endif /*VERSION2*/
  }

  /* generate "linear" description from section info
   * stored as codebook for each scalefactor band and group
   */
  if (nsect) {
    bot = 0;
    for (i=0; i<nsect; i++) {
      cb = sect[2*i];
      top = sect[2*i + 1];
      for (; bot<top; bot++)
        *cb_map++ = cb;
      bot = top;
    }
  }  
  else {
    for (i=0; i<MAXBANDS; i++)
      cb_map[i] = 0;
  }
  
  /*
   * scale factor data
   */
#ifdef VERSION2
  if ( GetLenOfScfDataFlag(hResilience) && GetRvlcScfFlag(hResilience) ) {
    CommonExit (1,"getics: -lsf *and* -rvlc makes no sense!" ); 
  }

  if ( !GetLenOfScfDataFlag(hResilience) && !GetRvlcScfFlag(hResilience) ) {
#endif /* VERSION2 */
    /* normal AAC Mode */
    if(!hufffac( info, 
                 group, 
                 nsect, 
                 sect, 
                 *global_gain, 
                 factors,
#ifdef VERSION2
                 hResilience, 
                 hEscInstanceData,
#endif /* VERSION2 */
                 hVm ) ) {
#ifdef VERSION2
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf("getics: hufffac returns zero (huffdec2.c, getics())\n");
      }
#endif /*VERSION2*/
      return 0;
    }
#ifdef VERSION2
  } else {
    /* length_of_sf_data within bitstream */
    if ( GetLenOfScfDataFlag(hResilience) ) {
      if(!HuffScfResil( info, 
                        group, 
                        nsect, 
                        sect, 
                        *global_gain, 
                        factors,
                        hVm,
                        hResilience, 
                        hEscInstanceData ) ) {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("getics: HuffScfResil returns zero (huffdec2.c, getics())\n");
        }
        return 0;
      }
    }
    /* RVL coded scalefactors */
    if ( GetRvlcScfFlag(hResilience) ) {
      RVLCScfDecoding( info, 
                       group, 
                       nsect, 
                       sect, 
                       *global_gain, 
                       factors,
                       hVm,
                       hResilience, 
                       hEscInstanceData );
    }
  }
#endif /* VERSION2 */

  if ( bitStreamType  !=  SCALABLE ){  
    /*
     * pulse noiseless coding
     */
    if ((pulseInfo.pulse_data_present = GetBits (LEN_PULSE_PRES,
#ifdef VERSION2
                                                 PULSE_DATA_PRESENT, 
                                                 hResilience, 
                                                 hEscInstanceData,
#endif /* VERSION2 */
                                                 hVm ))) {
      if (info->islong) {
        GetPulseNc ( &pulseInfo,
#ifdef VERSION2
                     hResilience, 
                     hEscInstanceData,
#endif /* VERSION2 */
                     hVm );
      }
      else {
#ifdef VERSION2
        if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
          CommonExit ( 2, "getics: pulse data not allowed for short blocks");
#ifdef VERSION2
        }
        else {
          if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
            printf ( "getics: pulse data not allowed for short blocks\n");    
            printf ( "getics: -->something should be done here\n");
          }
        }
#endif /*VERSION2*/
      }
    }


    /*
     * tns data
     */
    if (GetBits ( LEN_TNS_PRES,
#ifdef VERSION2
                  TNS_DATA_PRESENT, 
                  hResilience, 
                  hEscInstanceData,
#endif /*VERSION2*/
                  hVm )) {
      get_tns ( info, 
                tns, 
#ifdef VERSION2
                hResilience, 
                hEscInstanceData,
#endif /*VERSION2*/
                hVm );
    }
    else {
      clr_tns ( info, tns );
    }
    

    /*
     * Sony gain control
     */
    if (GetBits ( LEN_GAIN_PRES,
#ifdef VERSION2
                  GAIN_CONTROL_DATA_PRESENT, 
                  hResilience, 
                  hEscInstanceData,
#endif /*VERSION2*/
                  hVm )) {
#ifdef  SRS
      get_gcBuf( *win,
                 gc_streamCh,
#ifdef VERSION2
                 hResilience, 
                 hEscInstanceData,
#endif /*VERSION2*/
                 hVm);
#else
      CommonExit(1, "Gain control not implemented");
#endif
    }
  }
#ifdef VERSION2
  if ( GetReorderSpecFlag ( hResilience ) ) {
    ReadLenOfSpecData ( hHcrInfo, 
                        hResilience,
                        hVm, 
                        hEscInstanceData );
  }
  if ( GetLenOfLongestCwFlag ( hResilience ) ) {
    ReadLenOfLongestCw ( hHcrInfo, 
                         hResilience, 
                         hVm, 
                         hEscInstanceData);
  }
#endif /*VERSION2*/
  return HuffSpecFrame ( info, 
                         nsect, 
                         sect, 
                         factors,
                         coef,
                         max_spec_coefficients,
#ifdef VERSION2
                         hResilience,
                         hHcrSpecData,
                         hConcealment, 
                         hHcrInfo, 
                         hEscInstanceData,
#endif /*VERSION2*/
                         hVm );
}

/* 
 * get scale factors
 */
int hufffac ( Info*              info, 
              byte*              group, 
              int                nsect, 
              byte*              sect,
              short              global_gain, 
              short*             factors,
#ifdef VERSION2
              HANDLE_RESILIENCE  hResilience,
              HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
#endif /*VERSION2*/
              HANDLE_BUFFER      hVm )
 
{
  Hcb*     hcb;
  Huffman* hcw;
  int      i;
  int      b;
  int      bb;
  int      t;
  int      n;
  int      sfb;
  int      top;
  int      fac;
  int      is_pos;
  int      factor_transmitted[MAXBANDS];
  int*     fac_trans;
#ifdef PNS
  int      noise_pcm_flag = 1;
#endif /*PNS*/
  int      noise_nrg;

  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);
  shortclr(factors, MAXBANDS);

  sfb = 0;
  fac_trans = factor_transmitted;
  for ( i = 0; i < nsect; i++ ) {
    t   = sect[0];              /* codebook for this section */
    top = sect[1];              /* top of section in sfb */
    sect += 2;
    for ( ; sfb < top; sfb++ ) {
      fac_trans[sfb] = t;
    }
  }

  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
  noise_nrg = global_gain - NOISE_OFFSET;

  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    printf("scale factors\n");
  for(b = 0; b < info->nsbk; ){
    n = info->sfb_per_sbk[b];
    b = *group++;
    for(i = 0; i < n; i++){
      switch (fac_trans[i]) {
      case ZERO_HCB:        /* zero book */
        break;
      default:              /* spectral books */
        /* decode scale factor */
        t = decode_huff_cw ( hcw,
                             0, 
#ifdef VERSION2
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
        fac += t - MIDFAC;    /* 1.5 dB */

        if (debug['f'])
          printf("%3d:%3d", i, fac);
        if( ( fac >= ( 2 * TEXP ) ) ) {
#ifdef VERSION2
          if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
            CommonExit(2, "hufffac: Scale factor too large: %d", fac);
#ifdef VERSION2
          }
          else {
            if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
              printf( "hufffac: scale factor too large: %d\n", fac);
              printf( "hufffac: --> scale factor has been set to 255\n");
            }
            fac = 255;
          }
#endif /*VERSION2*/
        }
        if( fac < 0 ) {
#ifdef VERSION2
          if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
            CommonExit( 2, "hufffac: scale factor too small: %d", fac);
#ifdef VERSION2
          }
          else {
            if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
              printf( "hufffac: scale factor too small: %d\n", fac);
              printf( "hufffac: --> scale factor has been set to 0\n");
            }
            fac = 0;
          }
#endif /*VERSION2*/
        }
        factors[i] = fac;
        break;
      case BOOKSCL:         /* invalid books */
#ifdef VERSION2
        if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
          CommonExit ( 2, "hufffac: book %d: invalid book", BOOKSCL );
#ifdef VERSION2
        }
        else {
          if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
            printf( "hufffac: book %d: invalid book\n", BOOKSCL );
          }
        }
#endif /*VERSION2*/
        break;
      case INTENSITY_HCB:           /* intensity books */
      case INTENSITY_HCB2:
        /* decode intensity position */
        t = decode_huff_cw ( hcw,
                             0, 
#ifdef VERSION2
                             hResilience, 
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
        is_pos += t - MIDFAC;
        factors[i] = is_pos;
        break;
#ifdef PNS
      case NOISE_HCB:       /* noise books */
        /* decode noise energy */
        if (noise_pcm_flag) {
          noise_pcm_flag = 0;
          t = GetBits ( NOISE_PCM_BITS,
#ifdef VERSION2
                        DPCM_NOISE_NRG, 
                        hResilience, 
                        hEscInstanceData,
#endif /*VERSION2*/
                        hVm ) - NOISE_PCM_OFFSET;
        }
        else
          t = decode_huff_cw ( hcw,
                               0, 
#ifdef VERSION2
                               hResilience, 
                               hEscInstanceData,
#endif /*VERSION2*/
                               hVm ) - MIDFAC;
        noise_nrg += t;
        if (debug['f'])
          printf("\n%3d %3d (noise, code %d)", i, noise_nrg, t);
        factors[i] = noise_nrg;
        break;
#endif /*PNS*/
      }
      if (debug['f'])
        printf("%3d: %3d %3d\n", i, fac_trans[i], factors[i]);
    }
    if (debug['f'])
      printf("\n");

    /* expand short block grouping */
    if (!(info->islong)) {
      for(bb++; bb < b; bb++) {
        for (i=0; i<n; i++) {
          factors[i+n] = factors[i];
        }
        factors += n;
      }
    }
    fac_trans += n;
    factors += n;
  }
  return 1;
}

/* rm2 inverse quantization
 * escape books need ftn call
 * other books done via macro
 */
#define iquant( q ) ( q >= 0 ) ? \
(Float)iq_exp_tbl[ q ] : (Float)(-iq_exp_tbl[ - q ])


