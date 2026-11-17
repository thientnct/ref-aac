/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by

Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "monopredHandle.h"      /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "all.h"                 /* structs */
#include "bitstreamStruct.h"     /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "nok_prediction.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "huffdec2.h"
#include "huffdec3.h"
#include "port.h"
#include "sony_local.h" /* 971117 YT */
#include "common_m4a.h"
#include "bitfct.h"
#include "buffers.h"
#include "buffersHandle.h"
#include "bitstream.h"
#include "interface.h"


#define MAX_NR_OF_SWB       120
#define VCB11_ARRAY_SIZE    LN4 /* probably too large, but ... better than too small */

  typedef struct {
    unsigned short nrOfSwb;
    unsigned short swbReal;
  } ENTRY_POINT;


static void PrintSpectralValues ( int *quant )
{
  unsigned short i;
  for ( i = 0; i < LN2; i++ )
    printf ( "bno %13li line %4i value %13i\n", bno, i, quant[i] );
}


void deinterleave ( void              *inptr,  /* formerly pointer to type int */
                    void              *outptr, /* formerly pointer to type int */
                    short             length,  /* sizeof base type of inptr and outptr in chars */
                    short             ngroups,
                    short             nsubgroups[], 
                    int               ncells[], 
                    short             cellsize[],
                    int               islong,
                    HANDLE_RESILIENCE hResilience )
{
  char *inptr1  = (char*)inptr;
  char *outptr1 = (char*)outptr;

    {/* just to avoid compiler messages like "warning: unused parameter" */
      islong = islong; 
      hResilience = hResilience;
    }
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
                HANDLE_RESILIENCE hResilience,
                HANDLE_BUFFER     hVm,
                HANDLE_EP_INFO    hEpInfo )
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
        if((j = GetBits ( SCALE_FACTOR_GROUPING, 
                          1,
                          hResilience,
                          hVm, 
                          hEpInfo )) == 0) {
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
              HANDLE_RESILIENCE hResilience,
              HANDLE_BUFFER     hVm, 
              HANDLE_EP_INFO    hEpInfo )
{
  int b, i, mp;

  mp = GetBits ( MS_MASK_PRESENT, 
                 LEN_MASK_PRES,
                 hResilience,
                 hVm, 
                 hEpInfo );
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
      *mask = GetBits ( MS_USED, 
                        LEN_MASK,
                        hResilience,
                        hVm, 
                        hEpInfo );
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
                      HANDLE_RESILIENCE hResilience,
                      HANDLE_BUFFER     hVm, 
                      HANDLE_EP_INFO    hEpInfo )
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

    if (!(tns_info->n_filt = GetBits ( N_FILT, 
                                       short_flag ? 1 : 2,
                                       hResilience,
                                       hVm, 
                                       hEpInfo )))
      continue;
            
    tns_info -> coef_res = res = GetBits ( COEF_RES, 
                                           1,
                                           hResilience,
                                           hVm, 
                                           hEpInfo ) + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      top = tns_filt->start_band = top - GetBits ( TNS_LENGTH, 
                                                   short_flag ? 4 : 6,
                                                   hResilience,
                                                   hVm, 
                                                   hEpInfo );
      tns_filt->order = GetBits ( ORDER, 
                                  short_flag ? 3 : 5,
                                  hResilience,
                                  hVm, 
                                  hEpInfo );

      if (tns_filt->order)  {
        tns_filt->direction = GetBits ( DIRECTION, 
                                        1,
                                        hResilience,
                                        hVm, 
                                        hEpInfo );
        compress = GetBits ( COEF_COMPRESS, 
                             1,
                             hResilience,
                             hVm, 
                             hEpInfo );

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = GetBits ( TNS_COEF, 
                          res2,
                          hResilience,
                          hVm, 
                          hEpInfo );
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
                         HANDLE_RESILIENCE  hResilience,
                         HANDLE_BUFFER      hVm,
                         HANDLE_EP_INFO     hEpInfo )
{
  int i;
  pulseInfo->number_pulse = GetBits ( NUMBER_PULSE,
                                     LEN_PULSE_NPULSE,
                                     hResilience,
                                     hVm, 
                                     hEpInfo );
  pulseInfo->pulse_start_sfb = GetBits ( PULSE_START_SFB, 
                                        LEN_PULSE_ST_SFB,
                                        hResilience,
                                        hVm, 
                                        hEpInfo );
  for(i=0; i<pulseInfo->number_pulse + 1; i++) {
    pulseInfo->pulse_offset[i] = GetBits ( PULSE_OFFSET, 
                                          LEN_PULSE_POFF,
                                          hResilience,
                                          hVm, 
                                          hEpInfo );
    pulseInfo->pulse_amp[i] = GetBits ( PULSE_AMP, 
                                       LEN_PULSE_PAMP,
                                       hResilience,
                                       hVm, 
                                       hEpInfo );
  }
}

static void pulse_nc ( int*               coef, 
                       struct Pulse_Info* pulse_info
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
        CommonExit( 2, "pulse_nc: pulse data cannot be processed on line %i (huffdec2.c)", k );
    } 
  }
}

#ifdef SRS

long get_gcBuf ( byte              window_sequence, 
                 BsBitStream*      gc_streamCh,
                 HANDLE_RESILIENCE hResilience ,                 
                 HANDLE_BUFFER     hVm,
                 HANDLE_EP_INFO    hEpInfo )
{
  unsigned int bd;
  int wd;
  unsigned int ad;
  unsigned long   max_band, natks, ltmp;
  int loc;

  loc = gc_streamCh->currentBit;
  max_band = GetBits ( MAX_BAND, 
                       NBANDSBITS,
                       hResilience,
                       hVm, 
                       hEpInfo );
  BsPutBit(gc_streamCh, max_band, NBANDSBITS);/*   0 < max_band <= 3 */

  switch (window_sequence) {
  case ONLY_LONG_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 1; wd++) {
        natks = GetBits ( ADJUST_NUM, 
                          NATKSBITS,
                          hResilience,
                          hVm, 
                          hEpInfo );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( ALEVCODE, 
                           IDGAINBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          ltmp = GetBits ( ALOCCODE,
                           ATKLOCBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
          BsPutBit(gc_streamCh, ltmp, ATKLOCBITS);
        }
      }
    }
    break;
  case LONG_START_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        natks = GetBits ( ADJUST_NUM, 
                          NATKSBITS,
                          hResilience,
                          hVm, 
                          hEpInfo );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( ALEVCODE,
                           IDGAINBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          if (wd == 0) {
            ltmp = GetBits ( ALOCCODE, 
                             ATKLOCBITS_START_A,
                             hResilience,
                             hVm, 
                             hEpInfo );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_START_A);
          }
          else {
            ltmp = GetBits ( ALOCCODE, 
                             ATKLOCBITS_START_B,
                             hResilience,
                             hVm, 
                             hEpInfo );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_START_B);
          }
        }
      }
    }
    break;
  case EIGHT_SHORT_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 8; wd++) {
        natks = GetBits (  ADJUST_NUM, 
                           NATKSBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( ALEVCODE, 
                           IDGAINBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          ltmp = GetBits (  ALOCCODE, 
                            ATKLOCBITS_SHORT,
                            hResilience,
                            hVm, 
                            hEpInfo );
          BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_SHORT);
        }
      }
    }
    break;
  case LONG_STOP_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        natks = GetBits (  ADJUST_NUM, 
                           NATKSBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( ALEVCODE, 
                           IDGAINBITS,
                           hResilience,
                           hVm, 
                           hEpInfo );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          if (wd == 0) {
            ltmp = GetBits ( ALOCCODE, 
                             ATKLOCBITS_STOP_A,
                             hResilience,
                             hVm, 
                             hEpInfo );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_STOP_A);
          }
          else {
            ltmp = GetBits ( ALOCCODE,
                             ATKLOCBITS_STOP_B,
                             hResilience,
                             hVm, 
                             hEpInfo );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_STOP_B);
          }
        }
      }
    }
    break;
  default:
    return  -1;
  }
  return gc_streamCh->currentBit - loc;
}

#endif /*SRS*/

/*
 * read the codebook and boundaries
 */
static int huffcb ( byte*              sect, 
                    Info               info,
                    int*               sectbits, 
                    int                tot_sfb, 
                    int                sfb_per_sbk, 
                    byte               max_sfb,
                    unsigned char      numGroups,
                    HANDLE_HCR         hHcrInfo,
                    HANDLE_RESILIENCE  hResilience,
                    HANDLE_BUFFER      hVm,
                    HANDLE_EP_INFO     hEpInfo,
                    HANDLE_CONCEALMENT hConcealment)
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
  while ( ( nrSfb < tot_sfb ) && ( nsect < tot_sfb ) ) {
    currentMaxSfb = sfb_per_sbk * group + max_sfb;
    vcb11Flag = 0; /* virtual codebooks flag is false */
      *sect = GetBits ( SECT_CB, 
                        LEN_CB,
                        hResilience,
                        hVm, 
                        hEpInfo );
      
      hConcealment = hConcealment;
      n = GetBits ( SECT_LEN_INCR, 
                    bits,
                    hResilience,
                    hVm, 
                    hEpInfo );
    while ( n == len && nrSfb < currentMaxSfb ) { /* changed from < tot_sfb */
      nrSfb += len;
      n = GetBits ( SECT_LEN_INCR, 
                    bits,
                    hResilience,
                    hVm, 
                    hEpInfo );
    }
    nrSfb += n;
    sect++;
    *sect = nrSfb;
    sect++;
    /**/
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
    /**/
    nsect++;
    if (debug['s'] ) {
      printf(" %6d %6d %6d %6d", nsect, sect[-1], sect[-2], info.bk_sfb_top[sect[-1]-1] );
      printf("\n");
    }
    if ( sect[-1] > currentMaxSfb ) {
        CommonExit( 2, "huffcb: group %i: nrOfScfBands (%2i) > currentMaxSfb (%2i) (huffdec2.c)",
                    group, sect[-1], currentMaxSfb );
    }
    /* insert a zero section for regions above max_sfb for each group */
    if ( sect[-1] == currentMaxSfb ) {           /* changed from ((sect[-1] % sfb_per_sbk) == max_sfb) */
      nrSfb = ( sfb_per_sbk * ( group + 1 ) );    /* changed from nrSfb += (sfb_per_sbk - max_sfb) */
      group++;
      if ( currentMaxSfb != sfb_per_sbk * group ) { /* this inserts a section with codebook 0 */
        *sect++ = 0;
        *sect++ = nrSfb;
        nsect++;
        if (debug['s'])
          printf("(%6d %6d %6d)\n", nsect, sect[-1], sect[-2]);
      }
    }
  }
  if ( nrSfb != tot_sfb ) {
      CommonExit ( 2, "huffcb: nrSfb (%3hu) != totSfb (%3hu)", nrSfb, tot_sfb );
    return 0;
  }
  if ( nsect > tot_sfb ) {
      CommonExit ( 2, "huffcb: nsect (%3hu) > totSfb (%3hu)", nsect, tot_sfb );
    return 0;
  }
  if ( max_sfb && ( group != numGroups ) ) {
      CommonExit ( 2, "huffcb: group (%hu) != numGroups (%hu) (huffdec2.c)", group, numGroups );
  }
  return ( nsect );
}
static int getescape ( int                q,
                       HANDLE_RESILIENCE  hResilience,
                       HANDLE_HCR         hHcrInfo,
                       HANDLE_BUFFER      hVm, 
                       HANDLE_EP_INFO     hEpInfo )
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
      {
        tmp = GetBits ( ESCAPE_BITS, 1, hResilience, hVm, hEpInfo );
      }
    if ( tmp == 0 ) { /* truncation condition */
      break;
    }
    if ( i >= 12 ) {
      /* if i == 12 and no escape separator has been read -> break,
         because the escape word has a maximum length of Nmax + 4, with Nmax = 8 --> imax = 12 */
      CommonExit(2,"getescape: escape_prefix longer than 8 bit (huffdec2.c)");
      hHcrInfo = hHcrInfo; /* just to avoid compiler warning */
      break; 
    }
  }
    {
      off = GetBits ( ESCAPE_BITS, 
                      i,
                      hResilience,
                      hVm, 
                      hEpInfo );
    }
  i = off + (1<<i);
  if(neg)
    i = -i;
  return i;
}

unsigned char  HuffSpecKernelPure ( int*               qp, 
                                    Hcb*               hcb, 
                                    Huffman*           hcw, 
                                    int                step,
                                    HANDLE_BUFFER      hSpecData,
                                    HANDLE_RESILIENCE  hResilience,
                                    HANDLE_HCR         hHcrInfo,
                                    HANDLE_EP_INFO     hEpInfo,
                                    HANDLE_CONCEALMENT hConcealment )
                   
{
  int            temp;
  unsigned long  codewordBegin;
  unsigned short codewordLength;

  codewordBegin = GetReadBitCnt ( hSpecData );
  temp = decode_huff_cw ( hcw,
                          hResilience,
                          hSpecData, 
                          1, 
                          hEpInfo );
  unpack_idx(qp, temp, hcb);
  
  if (!hcb->signed_cb)
    get_sign_bits ( qp, 
                    step,
                    hResilience,
                    hSpecData, 
                    hEpInfo );
  if ( hcw == &book11[0] ) {
/*   if(codebook == ESCBOOK){ */
    qp[0] = getescape ( qp[0],
                        hResilience,
                        hHcrInfo,
                        hSpecData, 
                        hEpInfo);
    qp[1] = getescape ( qp[1],
                        hResilience,
                        hHcrInfo,
                        hSpecData, 
                        hEpInfo);
  }
  /* section data coding of codebook 11: checks max/min values within a SFB, if codebook 11 is used */

  codewordLength = GetReadBitCnt ( hSpecData ) - codewordBegin;
  return ( codewordLength );
}

static unsigned short HuffSpecKernelNoReordering ( int*               qp, 
                                                   Hcb*               hcb,
                                                   Huffman*           hcw,
                                                   int                step,
                                                   unsigned short     codebook,
                                                   HANDLE_BUFFER      hSpecData,
                                                   HANDLE_RESILIENCE  hResilience,
                                                   HANDLE_HCR         hHcrInfo,
                                                   HANDLE_EP_INFO     hEpInfo,
                                                   HANDLE_CONCEALMENT hConcealment )
{
  unsigned short codewordLength;
  codewordLength = HuffSpecKernelPure ( qp, 
                                        hcb, 
                                        hcw, 
                                        step, 
                                        hSpecData,
                                        hResilience, 
                                        hHcrInfo,
                                        hEpInfo,
                                        hConcealment );
  return ( codewordLength );
}


static unsigned short HuffSpecDecDefault ( Info*              info, 
                                           byte*              sect, 
                                           int                nsect, 
                                           int                quant[],
                                           HANDLE_BUFFER      hSpecData,
                                           HANDLE_RESILIENCE  hResilience,
                                           HANDLE_HCR         hHcrInfo,
                                           HANDLE_EP_INFO     hEpInfo,
                                           HANDLE_CONCEALMENT hConcealment )
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
        CommonExit ( 2, "HuffspecDecDefault: book %d: invalid book (huffdec2.c, HuffSpecDecDefault())", BOOKSCL );
    }
    if( ( codebook == ZERO_HCB ) ||
        ( codebook == NOISE_HCB ) ||
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
    hConcealment = hConcealment; /* dummy to avoid compiler warning */
    lineStart = lineCnt;
    for ( sfbCnt = bottom; sfbCnt < top; sfbCnt++ ) {
      stop = *bandp++;
      while ( lineCnt < stop ) {
        codewordLength = HuffSpecKernelNoReordering ( qp, 
                                                      hcb, 
                                                      hcw, 
                                                      step, 
                                                      codebook,
                                                      hSpecData,
                                                      hResilience, 
                                                      hHcrInfo,
                                                      hEpInfo,
                                                      hConcealment );
        lengthOfLongestCodeword = MAX ( codewordLength, lengthOfLongestCodeword );
        qp       += step;
        lineCnt  += step;
      } 
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
                           HANDLE_RESILIENCE  hResilience,
                           HANDLE_BUFFER      hVm,
                           HANDLE_BUFFER      hHcrSpecData,
                           HANDLE_HCR         hHcrInfo,
                           HANDLE_EP_INFO     hEpInfo,
                           HANDLE_CONCEALMENT hConcealment)
{
  int            quant[LN2]; 
  unsigned short lengthOfLongestCodeword;
  
  intclr  ( quant, LN2);
  
    lengthOfLongestCodeword = HuffSpecDecDefault ( info, 
                                                   sect, 
                                                   nsect, 
                                                   quant, 
                                                   hVm,
                                                   hResilience, 
                                                   hHcrInfo,
                                                   hEpInfo,
                                                   hConcealment );
  hHcrSpecData = hHcrSpecData; /* to aviod compiler warning */

  /* pulse noisless coding reconstruction */
  if ( (info->islong) && (pulseInfo.pulse_data_present) )
    pulse_nc(quant, &pulseInfo
             );

  if (!info->islong) {
    int tmp_spec[LN2];
    deinterleave ( quant,
                   tmp_spec,
                   sizeof(int),
                   info->num_groups,   
                   info->group_len,
                   info->sfb_per_sbk,
                   info->sfb_width_short,
                   info->islong,
                   hResilience );
    memcpy(quant,tmp_spec,sizeof(tmp_spec));
  }

  if ( debug['Q'] ) {
    PrintSpectralValues ( quant );
  }

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
             short*                   global_gain, 
             short*                   factors,
             NOK_LT_PRED_STATUS*      nok_ltp, 
             TNS_frame_info*          tns,
             BsBitStream*             gc_streamCh, 
             enum AAC_BIT_STREAM_TYPE bitStreamType,
             HANDLE_RESILIENCE        hResilience,
             HANDLE_BUFFER            hVm,
             HANDLE_BUFFER            hHcrSpecData,
             HANDLE_HCR               hHcrInfo,
             HANDLE_EP_INFO           hEpInfo,
             HANDLE_CONCEALMENT       hConcealment,
             QC_MOD_SELECT qc_select)

{
  int            nsect;
  int            i;
  int            cb;
  int            top;
  int            bot;
  int            tot_sfb;
  byte           sect[ 2*(MAXBANDS+1) ];
  unsigned long  tmp;
  /*
   * global gain
   */
  *global_gain = 0;
  tmp = GetBits ( GLOBAL_GAIN_MSB_1_5,
                  5,
                  hResilience,
                  hVm, 
                  hEpInfo );
  *global_gain |= (tmp<<3);
  tmp = GetBits ( GLOBAL_GAIN_MSB_6_7,
                  2, 
                  hResilience,
                  hVm, 
                  hEpInfo);
  *global_gain |= (tmp<<1);
  tmp = GetBits ( GLOBAL_GAIN_MSB_8, 
                  1, 
                  hResilience,
                  hVm, 
                  hEpInfo);
  *global_gain |= tmp;
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
                   hResilience,
                   hVm,
                   hEpInfo,
                   nok_ltp,
		   NULL,
		   common_window,
                   qc_select);

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
                   hHcrInfo,
                   hResilience,
                   hVm, 
                   hEpInfo,
                   hConcealment);
  if( ( nsect == 0 ) && ( *max_sfb > 0 ) ) {
      CommonExit ( 2, "getics: nsect = 0 but maxSfb = %hu", *max_sfb);
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
    /* normal AAC Mode */
    if(!hufffac( info, 
                 group, 
                 nsect, 
                 sect, 
                 *global_gain, 
                 factors,
                 hVm,
                 hResilience, 
                 hEpInfo ) ) {
      return 0;
    }

  if ( bitStreamType  !=  SCALABLE ){  
    /*
     * pulse noiseless coding
     */
    if ((pulseInfo.pulse_data_present = GetBits ( PULSE_DATA_PRESENT, 
                                                 LEN_PULSE_PRES,
                                                 hResilience,
                                                 hVm, 
                                                 hEpInfo ))) {
      if (info->islong) {
        GetPulseNc ( &pulseInfo,
                     hResilience,
                     hVm, 
                     hEpInfo );
      }
      else {
          CommonExit ( 2, "getics: pulse data not allowed for short blocks");
      }
    }


    /*
     * tns data
     */
    if (GetBits ( TNS_DATA_PRESENT, 
                  LEN_TNS_PRES,
                  hResilience,
                  hVm, 
                  hEpInfo )) {
      get_tns ( info, 
                tns, 
                hResilience,
                hVm, 
                hEpInfo );
    }
    else {
      clr_tns ( info, tns );
    }
    

    /*
     * Sony gain control
     */
    if (GetBits ( GAIN_CONTROL_DATA_PRESENT, 
                  LEN_GAIN_PRES,
                  hResilience,
                  hVm, 
                  hEpInfo )) {
#ifdef  SRS
      get_gcBuf( *win,
                 gc_streamCh,
                 hResilience,
                 hVm, 
                 hEpInfo);
#else
      CommonExit(1, "Gain control not implemented");
#endif
    }
  }
  return HuffSpecFrame ( info, 
                         nsect, 
                         sect, 
                         factors,
                         coef,
                         hResilience,
                         hVm,
                         hHcrSpecData,
                         hHcrInfo, hEpInfo,
                         hConcealment );
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
              HANDLE_BUFFER      hVm,
              HANDLE_RESILIENCE  hResilience,
              HANDLE_EP_INFO     hEpInfo )
 
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
  int      noise_pcm_flag = 1;
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
                             hResilience,
                             hVm, 
                             0, 
                             hEpInfo );
        fac += t - MIDFAC;    /* 1.5 dB */

        if (debug['f'])
          printf("%3d:%3d", i, fac);
        if( ( fac >= ( 2 * TEXP ) ) ) {
            CommonExit(2, "hufffac: Scale factor too large: %d", fac);
        }
        if( fac < 0 ) {
            CommonExit( 2, "hufffac: scale factor too small: %d", fac);
        }
        factors[i] = fac;
        break;
      case BOOKSCL:         /* invalid books */
          CommonExit ( 2, "hufffac: book %d: invalid book", BOOKSCL );
        break;
      case INTENSITY_HCB:           /* intensity books */
      case INTENSITY_HCB2:
        /* decode intensity position */
        t = decode_huff_cw ( hcw,
                             hResilience,
                             hVm, 
                             0, 
                             hEpInfo );
        is_pos += t - MIDFAC;
        factors[i] = is_pos;
        break;
      case NOISE_HCB:       /* noise books */
        /* decode noise energy */
        if (noise_pcm_flag) {
          noise_pcm_flag = 0;
          t = GetBits ( DPCM_NOISE_NRG, 
                        NOISE_PCM_BITS,
                        hResilience,
                        hVm, 
                        hEpInfo ) - NOISE_PCM_OFFSET;
        }
        else
          t = decode_huff_cw ( hcw,
                               hResilience,
                               hVm, 
                               0, 
                               hEpInfo ) - MIDFAC;
        noise_nrg += t;
        if (debug['f'])
          printf("\n%3d %3d (noise, code %d)", i, noise_nrg, t);
        factors[i] = noise_nrg;
        break;
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


