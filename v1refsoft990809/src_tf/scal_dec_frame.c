/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (FhG)

and edited by

Bernhard Grill (University of Erlangen)
Takashi koike (Sony Corporation)

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

Copyright (c) 1997.

*/

#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "monopredHandle.h"      /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "all.h"                 /* structs */
#include "bitstreamStruct.h"     /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "nok_prediction.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "dec_lpc.h"


#include "mod_buf.h"
#include "port.h"
#include "flex_mux.h"
#include "lpc_common.h"
#include "dec_tf.h"
#include "bitstream.h"
#include "common_m4a.h"
/* ---  AAC --- */
#include "aac.h"
#include "huffdec2.h"
#include "nok_lt_prediction.h"
#include "scal_dec.h"
#include "plotmtv.h"
#include "util.h"
#include "flex_mux.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "plotmtv.h"
#include "interface.h"
#include "buffers.h"
#include "mp4_tf.h"
/* ***************************************************************************/
/* scalable decoding : bitstream has 3 subframes (called granules) per frame */

static void vcopy( double src[], double dest[], int inc_src, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src;
    dest += inc_dest;
    src  += inc_src;
  }
  if (vlen) /* just for bounds-checkers sake */
    *dest = *src;

}

/* compute scalar product of two vectors */
static double scalprod( double src1[], double src2[], 
                        int inc_src1, int inc_src2, int vlen)
{
  int i;
  double sum=0.0;

  for (i=0;i<vlen; i++) {
    sum += (*src1)*(*src2);
    src1 += inc_src1;
    src2 += inc_src2;
  }
  return (sum);
}

static void vmult( double src1[], double src2[], double dest[], 
                   int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src1 * *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
  if (i<vlen)
    *dest = *src1 * *src2;

}


static void vadd( double src1[], double src2[], double dest[], 
                  int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src1 + *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
}

static void vsub( double src1[], double src2[], double dest[], 
                  int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src1 - *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
}

static void dec_lowpass(double spectrum[], int lopLong,
                        int lopShort, int maxLine, int iblen, 
                        int windowSequence, int nr_of_sbk)
{
  int bl, block_len, lop, i;
  float slope;
  double zero = 0.0;

  if (windowSequence != EIGHT_SHORT_SEQUENCE) {
    lop = lopLong;
  }
  else {
    lop = lopShort;
  }
    
  block_len = iblen /nr_of_sbk;
  if (lop > maxLine) {
    lop = maxLine;
  }
  slope = (1.0F /(float)lop);
  for (bl=0;bl<nr_of_sbk;bl++){
    vcopy( &zero, &spectrum[bl*block_len+maxLine], 0, 1, block_len - maxLine);
    for (i=1; i<lop; i++) {
      spectrum[bl*block_len+maxLine-i] *= (i*slope);
    }
  }
}

/* get_tns_vm does exactly the same as get_tns in huffdec2.c. Just the
   VM bitstream handling functions replace the AAC-type functions */

static void doInverseMsMatrix(
                       Info       *p_sb_info,
                       double *p_l_spec,
                       double *p_r_spec,
                       int msMask[8][60]
                       )
{
  int sb, win;
  int sfbw; 
  double  tmp_buffer[1024];
  for( win=0; win<p_sb_info->nsbk; win++ ) {
    int sboffs   = win*p_sb_info->bins_per_sbk[win];

    for( sb=0; sb<p_sb_info->sfb_per_sbk[win]; sb++ ) {
      if (p_sb_info->islong==0){
        sfbw = p_sb_info->sfb_width_short[sb];
      }else {
        if (sb==0)
          sfbw = p_sb_info->bk_sfb_top[sb];
        else 
          sfbw = p_sb_info->bk_sfb_top[sb]-p_sb_info->bk_sfb_top[sb-1];        
      }

      if(msMask[win][sb] != 0  ) {
        vadd( &p_l_spec[sboffs], &p_r_spec[sboffs], &tmp_buffer[sboffs],        1, 1, 1, sfbw );  /* -> L */
        vsub( &p_l_spec[sboffs], &p_r_spec[sboffs], &p_r_spec[sboffs], 1, 1, 1, sfbw );  /* -> R */
        vcopy(&tmp_buffer[sboffs], &p_l_spec[sboffs], 1, 1, sfbw );
      }

      sboffs += sfbw;
    }
  }

}

void doMsMatrix(Info *p_sb_info, double *p_l_spec, double *p_r_spec, int msMask[8][60])
{
  int sb, win, i;
  int sfbw;
  double sum[1024];
  double diff[1024];
  for( win=0; win<p_sb_info->nsbk; win++ ) {
    int sboffs   = win*p_sb_info->bins_per_sbk[win];
    
    for( sb=0; sb<p_sb_info->sfb_per_sbk[win]; sb++ ) {
      if (p_sb_info->islong==0){
        sfbw = p_sb_info->sfb_width_short[sb];
      }else {
        if (sb==0)
          sfbw = p_sb_info->bk_sfb_top[sb];
        else 
          sfbw = p_sb_info->bk_sfb_top[sb]-p_sb_info->bk_sfb_top[sb-1];        
      }

      if(msMask[win][sb] != 0  ) {
	vadd(&p_l_spec[sboffs], &p_r_spec[sboffs], &sum[sboffs], 1, 1, 1, sfbw);
	for(i = 0; i < sfbw; i++)
	  p_l_spec[sboffs + i] = 0.5 * sum[sboffs + i];
        
        vsub(&p_l_spec[sboffs], &p_r_spec[sboffs], &diff[sboffs], 1, 1, 1, sfbw);
	for(i = 0; i < sfbw; i++)
	  p_r_spec[sboffs + i] = 0.5 * diff[sboffs + i];
      }
      
      sboffs += sfbw;
    }
  }
  
}

static void  multCoreSpecMsFac(
                        Info       *p_sb_info,
                        int msMask[8][60],
                        double *core_spec,
                        double *out_spec
                        )
{
  int sb, win;
  int sfbw;
  double ms_fac=2;
  for( win=0; win<p_sb_info->nsbk; win++ ) {
    int sboffs   = win*p_sb_info->bins_per_sbk[win];

    for( sb=0; sb<p_sb_info->sfb_per_sbk[win]; sb++ ) {
      if (p_sb_info->islong==0){
        sfbw = p_sb_info->sfb_width_short[sb];
      }else {
        if (sb==0)
          sfbw = p_sb_info->bk_sfb_top[sb];
        else 
          sfbw = p_sb_info->bk_sfb_top[sb]-p_sb_info->bk_sfb_top[sb-1];        
      }
      if( msMask[win][sb] == 0  ) {
        vmult( &core_spec[sboffs], &ms_fac,&out_spec[sboffs] ,1, 0, 1, sfbw );  /* if no ms core = core * fac */
      }else{
        vcopy( &core_spec[sboffs],&out_spec[sboffs] ,1, 1, sfbw );  /* if ms core = core */
      }
      sboffs += sfbw;
    }
  }

}

static int get_tns_vm( BsBitStream *fixed_stream, Info *info, TNS_frame_info *tns_frame_info,int* decoded_bits )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static short              sgn_mask[] = { 
    0x2, 0x4, 0x8     };
  static short              neg_mask[] = { 
    (short) 0xfffc, (short)0xfff8, (short)0xfff0     };
  unsigned long ultmp, numBits;

  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];
    numBits = (short_flag ? 1 : 2);
    BsGetBit( fixed_stream, &ultmp,numBits   );
    *decoded_bits += numBits; 
    if (!(tns_info->n_filt = ultmp) )
      continue;     
    BsGetBit( fixed_stream, &ultmp, 1 );
    *decoded_bits += 1; 
    tns_info -> coef_res = res = ultmp + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      numBits = ( short_flag ? 4 : 6);
      BsGetBit( fixed_stream, &ultmp,numBits );
      *decoded_bits += numBits; 
      top = tns_filt->start_band = top - ultmp;

      numBits = ( short_flag ? 3 : 5);
      BsGetBit( fixed_stream, &ultmp, numBits );
      *decoded_bits += numBits; 

      tns_filt->order = ultmp;

      if (tns_filt->order)  {
        BsGetBit( fixed_stream, &ultmp, 1 );
        *decoded_bits += 1; 

        tns_filt->direction = ultmp;
        BsGetBit( fixed_stream, &ultmp, 1 );
        *decoded_bits += 1; 

        compress = ultmp;

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          BsGetBit( fixed_stream, &ultmp, res2 );
          *decoded_bits += res2; 
          tmp = ultmp;
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
  return 1;
}

static BsBitBuffer *dynBitBuf;
static int         gran;
static BsBitStream *dyn_stream_read;
int testDebug=0;

void aacScaleableDecodeInit( void )
{
 
  dynBitBuf  = BsAllocBuffer( 64000 );     /* just some large number. For AAC: required ca. 6000 */
  gran = 0;

  dyn_stream_read = BsOpenBufferRead( dynBitBuf );


}

  
static void FSSwitch(
              double  p_core[],
              double  p_rest[], 
              double  p_out[], 
              int          *diff_control, 
              Info         *p_sbinfo,
              int           numDiffBands,
              int        shortDiffWidth,
              int           is_decoder
              )
{
  int sbk, diffBand,x;
  int dc_decoded[8][128];
  short diffBandWidth[200];
  double doubleZero=0.0;
  int numDiffBandsPerSbk;
  int g;

  if (p_sbinfo->islong){
    numDiffBandsPerSbk=numDiffBands;
    diffBandWidth[0]= p_sbinfo->bk_sfb_top[0];  
    for (x=1;x< p_sbinfo->sfb_per_bk;x++){
      if  (p_sbinfo->bk_sfb_top[x]==0)
        CommonExit(-1,"\ninternal FSS error");
      diffBandWidth[x]= p_sbinfo->bk_sfb_top[x] - p_sbinfo->bk_sfb_top[x-1];
    }
  } else {
    numDiffBandsPerSbk=1;
    diffBandWidth[0]= shortDiffWidth;
  }

  g=0;
  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    for( diffBand=0; diffBand<numDiffBandsPerSbk; diffBand++ ) {
      dc_decoded[sbk][diffBand] = diff_control[g];
      g++;
    }
  }

  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    int offset = sbk*p_sbinfo->bins_per_sbk[sbk];
    int sum    = 0;
    for( diffBand=0; diffBand<numDiffBandsPerSbk; diffBand++ ) {
      if( dc_decoded[sbk][diffBand] == 0 ) {
        if( is_decoder ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, diffBandWidth[diffBand] ); 
        }
      } else {
        vcopy( &p_rest[offset], &p_out[offset], 1, 1, diffBandWidth[diffBand] ); 
        if( !is_decoder ) {
          vcopy( &doubleZero , &p_core[offset], 0, 1, diffBandWidth[diffBand] ); 
        }
      }
      offset += diffBandWidth[diffBand];
      sum    += diffBandWidth[diffBand];
    }
    vcopy( &p_rest[offset], &p_out[offset], 1, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
    vcopy( &doubleZero ,   &p_core[offset], 0, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
  }
}

static void FSSwitchSt( double  p_core[],/* bands of this spec is mult by 2 if ms is off */ 
                        double  p_rest[], 
                        double  p_out[], 
                        int*    diff_control, 
                        int     diffContrBands,
                        Info*   p_sbinfo,
                        int     msMask[8][60],
                        int     channel )
{
  int sbk, sfb;
  int dc_decoded[8][128];
  short sfbWidth[200];
  double doubleZero=0.0;

  sfbWidth[0]= p_sbinfo->sbk_sfb_top[0][0];  
  for (sfb=1;sfb< p_sbinfo->sfb_per_sbk[0];sfb++){
    if  (p_sbinfo->sbk_sfb_top[0][sfb]==0)
      CommonExit(-1,"\ninternal FSS error");
    sfbWidth[sfb]= p_sbinfo->sbk_sfb_top[0][sfb] - p_sbinfo->sbk_sfb_top[0][sfb-1];
  }
  
  if (p_sbinfo->islong){
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
      if (sfb<diffContrBands) {
        dc_decoded[0][sfb] = diff_control[sfb];
      }else {
        dc_decoded[0][sfb] =1;
      }
    }
  } else {
    /* see "correction  for short" */
    for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
      for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
        if (sfb<diffContrBands) {
          if ((msMask[sbk][sfb]==1)&&(channel==1)){
            /* side channel never uses diffencoding */
            dc_decoded[sbk][sfb] = 1;
          } else {                          
            if ((msMask[sbk][sfb]==1)&&(channel==0)) {
              /* M channel : always diff encoding if there is no celp or tvq core */
              dc_decoded[sbk][sfb] = 0;
            } else {
              /* L,R channel : use diff flag from bitstream */
              dc_decoded[sbk][sfb] = diff_control[sbk];
            }
          }
        }else {
          dc_decoded[sbk][sfb] =1;
        }
      }
    }
  }

  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    int offset = sbk*p_sbinfo->bins_per_sbk[sbk];
    int sum    = 0;  
    
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[sbk]; sfb++  ) {
      /* build combined L/R spectrum */
      if( (msMask[sbk][sfb] == 0 ) ) {
        if(  dc_decoded[sbk][sfb] == 0 ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      } else {
      /* build combined M spectrum */
        if( channel == 0 ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      }
      offset += sfbWidth[sfb];
      sum    += sfbWidth[sfb];
    }
    vcopy( &p_rest[offset], &p_out[offset], 1, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
    vcopy( &doubleZero ,   &p_core[offset], 0, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
  }
}

static void FSSwitchStwCore( double  p_core[],/* bands of this spec is mult by 2 if ms is off */ 
                        double  p_rest[], 
                        double  p_out[], 
                        int*    diff_control, 
                        int*    diff_controlLR, 
                        int     diffContrBands,
                        Info*   p_sbinfo,
                        int     msMask[8][60],
                        int     channel )
{
  int sbk, sfb;
  int dc_decoded[8][128];
  short sfbWidth[200];
  double doubleZero=0.0;

  sfbWidth[0]= p_sbinfo->sbk_sfb_top[0][0];  
  for (sfb=1;sfb< p_sbinfo->sfb_per_sbk[0];sfb++){
    if  (p_sbinfo->sbk_sfb_top[0][sfb]==0)
      CommonExit(-1,"\ninternal FSS error");
    sfbWidth[sfb]= p_sbinfo->sbk_sfb_top[0][sfb] - p_sbinfo->sbk_sfb_top[0][sfb-1];
  }
  
  if (p_sbinfo->islong){
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
      if (sfb<diffContrBands) {
        dc_decoded[0][sfb] = diff_controlLR[sfb]; 
      }else {
        dc_decoded[0][sfb] =1;
      }
    }
  } else {
    diffContrBands=120; /* some huge number , actually wrong, but corrected below: */
    /* see "correction  for short" */
    for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
      for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
        if (sfb<diffContrBands) {
          if ((msMask[sbk][sfb]==1)&&(channel==1)){
            /* side channel never uses diffencoding */
            dc_decoded[sbk][sfb] = 1;
          } else {                          
            /* L,R or M channel : use diff flag from bitstream */
            if ((msMask[sbk][sfb]==1)&&(channel==0)) {
              /* M channel: */ 
              dc_decoded[sbk][sfb] = diff_control[sbk];
            } else {
              /* L and R channel */
              dc_decoded[sbk][sfb] = diff_controlLR[sbk];
            }
          }
        }else {
          /* all above lower layer bandwith does not use diff spec */
          dc_decoded[sbk][sfb] =1;
        }
      }
    }
  }
  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    int offset = sbk*p_sbinfo->bins_per_sbk[sbk];
    int sum    = 0;  
    
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[sbk]; sfb++  ) {
      /* build combined L/R spectrum */
      if( (msMask[sbk][sfb] == 0 ) ) {
        if(  dc_decoded[sbk][sfb] == 0 ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      }      else  {
        /* build combined M spectrum */
        if( (channel == 0) && (dc_decoded[sbk][sfb] == 0)) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      }
      offset += sfbWidth[sfb];
      sum    += sfbWidth[sfb];
    }
    vcopy( &p_rest[offset], &p_out[offset], 1, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
    vcopy( &doubleZero ,   &p_core[offset], 0, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
  }
  /*correction  for short: diff encoding only up top diff_Short_lines see Table 6.12 */
  if (p_sbinfo->islong==0){
    for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
      int offset = sbk*p_sbinfo->bins_per_sbk[sbk]+p_sbinfo->shortFssWidth;
      int count = p_sbinfo->bins_per_sbk[sbk]-p_sbinfo->shortFssWidth;
      vcopy( &p_rest[offset], &p_out[offset], 1, 1, count );      
    }
  }
}


static void  decodeDiffCtrl(
                     BsBitStream * fixed_stream, 
                     int diffControl[ ],
                     int *decodedBits, 
                     int fssGroups)
{
  unsigned long int   ultmp, fssIdx, diffControlTmp;
  int fssG,sfb;
  for (sfb=0;sfb<60;sfb++){
    diffControl[sfb]=0;
  }
  sfb=0;
  for (fssG=0;fssG<fssGroups;fssG++){
    diffControlTmp=0;
    BsGetBit( fixed_stream, &ultmp, 2 );
    *decodedBits += 2;
    fssIdx=ultmp;
    switch (ultmp){
    case 0 : diffControlTmp |=0;
      break;
    case 1 : diffControlTmp |=15;
      break;
    default :
      BsGetBit( fixed_stream, &ultmp, 2 );
      *decodedBits += 2;
      fssIdx= (fssIdx<<2)| ultmp;
      switch (fssIdx){
      case  8:  diffControlTmp |=7;
        break;
      case  9:  diffControlTmp |=8;
        break;
      default:
        BsGetBit( fixed_stream, &ultmp, 1 );
        *decodedBits += 1;
        fssIdx= (fssIdx<<1)| ultmp;
        switch (fssIdx) {
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
          diffControlTmp |= (fssIdx-19);
          break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
          diffControlTmp |= (fssIdx-17);
          break;
        default:
          CommonExit(-1,"\nfssdecode error");
        }
        break;
      }
    }
    diffControl[sfb+3]= ((diffControlTmp & 0x1)!=0);
    diffControl[sfb+2]= ((diffControlTmp & 0x2)!=0);
    diffControl[sfb+1]= ((diffControlTmp & 0x4)!=0);
    diffControl[sfb+0]= ((diffControlTmp & 0x8)!=0);
    sfb +=4;      
  }
}

static int decode_grouping( int grouping, short region_len[] )
{
  int i, rlen, mask;

  int no_short_reg = 0;

  mask = 1 << ( 8-2 );
  rlen = 1;  /* min. length of first group is '1' */
  for( i = 1; i<8; i++ ) {
    if( (grouping & mask) == 0) {
      *region_len++ = rlen;
      no_short_reg++;
      rlen = 0;
    }
    rlen++;
    mask >>= 1;
  }
  *region_len = rlen;
  no_short_reg++;

  return( no_short_reg );
}






static void getModeFromOD(OBJECT_DESCRIPTOR *od,int *lowRateChannelPresent,
                   int *layNumChan,
                   int *lastAacLay,
                   int *firstStLay, 
                   enum CORE_CODEC* coreCoderIndex,
                   int *commonWindow)
{
  
  int nEsNumber, i, aac=0, nAacLay = 0;
  int nLowRateChan = 0;
  int audioDecType;
  int nFirstStereo = -1;
  int tfType;
  nEsNumber   = od->streamCount.value;
  *coreCoderIndex=NO_CORE;
  for (i= 0; i< nEsNumber; i++)
    {
      audioDecType = od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
      layNumChan[i] = od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
    switch (audioDecType)
      {
      case 0:
        if (( layNumChan[i] ) == 2 && (nFirstStereo < 0))
          nFirstStereo = i;
        tfType=od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value;
        switch (tfType) {
        case 0:
          nAacLay++;
          aac=1;
          break;
        case 1:
          nAacLay++;
          aac=1;
          break;
        case 2:
          nLowRateChan++;
          *coreCoderIndex=NTT_TVQ;          
          break;
        case 3:
          CommonExit(-1,"tfDecoderType not implemented");
          break;
        default :
          CommonExit(-1,"tfDecoderType not implemented");
          break;
        }          
        break;
      case 1: /* CELP core */
        nLowRateChan++;
        *coreCoderIndex=CC_CELP_MPEG4;          
        break;
      default:
        CommonExit(-1,"audioDecoderType not implemented");
        break;
      }
    switch (od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value ) 
      {
      case 1 :
        commonWindow[i]=0;
        break;
      case 2 :
        commonWindow[i]=1;
        break;
      default :
        CommonExit(-1,"wrong channel config ");
        break;
      }
    }
  *lastAacLay = nLowRateChan ? nAacLay : nAacLay-1;
  if (aac==0) *lastAacLay=-1;
  *lowRateChannelPresent = nLowRateChan;
  *firstStLay = nFirstStereo  ;

}

void
ltp_scal_reconstruct(WINDOW_SEQUENCE blockType,
		     WINDOW_SHAPE windowShape,
		     WINDOW_SHAPE windowShapePrev,
		     int num_channels,
		     double **p_reconstructed_spectrum,
		     int blockSizeSamples,
		     int med_win_in_long,
		     int short_win_in_long,
		     int nr_of_sfb,
		     int msMask[8][60],
		     NOK_LT_PRED_STATUS **nok_lt_status,
		     Info *info, TNS_frame_info **tns_info,
		     QC_MOD_SELECT qc_select)
{
  int i, j,ch;
  Float tmpbuffer[BLOCK_LEN_LONG];
  NOK_LT_PRED_STATUS *nok_ltp;

  /*
   * Following piece of code reconstructs the spectrum of the 
   * first mono (or stereo, if no mono present) layer for LTP.
   */
  
  if(num_channels == 2)
    doInverseMsMatrix(info, p_reconstructed_spectrum[0],
		      p_reconstructed_spectrum[1], msMask);

  for(ch = 0; ch < num_channels; ch++)
  {
    nok_ltp = nok_lt_status[ch];

    for(i = 0; i < blockSizeSamples; i++)
      tmpbuffer[i] = p_reconstructed_spectrum[ch][i];

    /* Add the LTP contribution to the reconstructed spectrum. */
    nok_lt_predict (info, blockType, windowShape, windowShapePrev,
		    nok_ltp->sbk_prediction_used, nok_ltp->sfb_prediction_used, 
                    nok_ltp, nok_ltp->weight, nok_ltp->delay, tmpbuffer, 
                    blockSizeSamples, blockSizeSamples / med_win_in_long, 
		    blockSizeSamples / short_win_in_long, tns_info[ch], qc_select);

    for(i = 0; i < blockSizeSamples; i++)
      p_reconstructed_spectrum[ch][i] = tmpbuffer[i];

    /* TNS synthesis filtering. */
    info->islong = (blockType != EIGHT_SHORT_SEQUENCE);
    for(i = j = 0; i < tns_info[ch]->n_subblocks; i++)
    {
      tns_decode_subblock(tmpbuffer + j, nr_of_sfb, 
			  info->sbk_sfb_top[i], info->islong,
			  &tns_info[ch]->info[i], qc_select);
      
      j += info->bins_per_sbk[i];
    }
    
    /* Update the time domain buffer of LTP. */
    nok_ltp_buf_update(blockType, windowShape, windowShapePrev, 
		       nok_ltp, tmpbuffer, blockSizeSamples, 
		       blockSizeSamples / med_win_in_long, 
		       blockSizeSamples / short_win_in_long,
		       short_win_in_long);
  }

  /*
   * The reconstructed spectrum of the lowest layer is processed as 
   * Mid/Side in the upper layers.
   */
  if(num_channels == 2)
    doMsMatrix(info, p_reconstructed_spectrum[0], 
	       p_reconstructed_spectrum[1], msMask);
}

static int debugDiff=0;

void aacScaleableFlexMuxDecode ( BsBitStream*       fixed_stream,
                                 BsBitStream*       gc_WRstream[],
                                 WINDOW_SEQUENCE        windowSequence[MAX_TIME_CHANNELS],
                                 int*               flexMuxBits,
                                 double*            spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                                 int                output_select,
                                 Info**             sfbInfo,
                                 int                numChannels,
                                 int                lopLong,
                                 int                lopShort,
                                 float              normBw,
                                 WINDOW_SHAPE*      windowShape,
                                 QC_MOD_SELECT      qc_select,
                                 HANDLE_RESILIENCE  hResilience,
                                 HANDLE_BUFFER      hVm,
                                 HANDLE_BUFFER      hHcrSpecData,
                                 HANDLE_HCR         hHcrInfo,
                                 FRAME_DATA*        fd,
                                 TF_DATA*           tfData,
                                 ntt_DATA*          nttData,
                                 HANDLE_EP_INFO     hEpInfo,
                                 HANDLE_CONCEALMENT hConcealment,
				 NOK_LT_PRED_STATUS **nok_lt_status,
				 int                med_win_in_long,
				 int                short_win_in_long,
				 PRED_TYPE          pred_type)
{
  BsBitStream* layer_stream ;
  unsigned int x, j;
  static int lowRateChannelPresent;
  static enum CORE_CODEC  coreCodecIdx;
  static int     nrGranules = 3;
  static int     lastAacLay = 0;  
  static int     firstAacLay = 0;  
  static int     layNumChan[MAX_TF_LAYER];
  static int     firstStLay;
  static int     lastMonoLay;
  static WINDOW_SHAPE windowShapePrev = WS_FHG;
  unsigned long  ultmp,codedBlockType;
  int            diffControl[MAX_TIME_CHANNELS][ 60 ]={{0x0}};
  int            commonWindow[MAX_TF_LAYER];
  int            aacLayer = 0;
  int            layer;
  int            ch,calcMonoLay;
  double*        low_spec;
  int            diffControlBands;
  int            i;
  int            tmp = 0;
  int            msMask[8][60]={{0x0}};
  int            msMaskPres = 0;
  int            shortFssWidth;
  TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  TNS_frame_info tns_info_ext[MAX_TIME_CHANNELS];
  byte max_sfb[Winds];
  static byte sfbCbMap[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS] = {{{0x0}}};
  int  groupInfo = 0;
  int  tns_data_present[2];
  int  tns_ext_present[2];
  int   used_bits[4][2];
  int decodedBits;
  int            coreDecBits = 0;
  ntt_INDEX           index;
  ntt_INDEX           index_scl;
  int coreChannels;
  unsigned long totalLength;
  unsigned long AUlength[8];
  int tvqTnsPresent  ;
  /* init */
  decodedBits       = 0;
  tvqTnsPresent     = 0;
  coreChannels      = 0;
  tns_ext_present[0]= 0;
  tns_ext_present[1]= 0;
  
  index.nttDataBase=nttData->nttDataBase;
  index_scl.nttDataScl=nttData->nttDataScl;
  index.numChannel =numChannels;
  index_scl.numChannel =numChannels;
  index.block_size_samples=
    ( fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.
      specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;
  index_scl.block_size_samples= index.block_size_samples;

  /*********************/
  for ( x = 0; x < fd->od->streamCount.value; x++ ){
    unsigned int AUindex;/* AUindex = scaleable layer Number , since we have not stream map table*/
    totalLength = 0;
    getAccessUnit ( fixed_stream, fd->layer[x].bitBuf, &AUindex , &totalLength,fd->od->ESDescriptor[x]);
    if (x != AUindex) CommonExit(-1,"\nError in FlexMuxDecode");
    AUlength[x] = totalLength;
  }
  *flexMuxBits = BsCurrentBit(fixed_stream);
  layer=0;

  getModeFromOD(fd->od,&lowRateChannelPresent ,layNumChan ,&lastAacLay ,&firstStLay,&coreCodecIdx,  
                commonWindow);

  
  if (output_select>lastAacLay) output_select=lastAacLay;
  if (output_select<0) output_select=0; /* YB 980508 */
  if (firstStLay>=0)
    lastMonoLay=firstStLay-1;/* debug */
  else 
    lastMonoLay=lastAacLay;/* debug */
  if (output_select > lastMonoLay ){
    calcMonoLay=lastMonoLay ;
  } else {
    calcMonoLay=output_select ;      
  }


  /* open the first layer bitbuffer  to read :
     can  be a AAC or TwinVq bitbuffer as core */
  layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);


  /* 
     According to the new bitstream syntax (98/11/12), 
     there is no more ics_reserved_bit for scalable
  */
  BsGetBit( layer_stream, &ultmp,2); /* window_sequence */
  codedBlockType = (int)ultmp;
  decodedBits += 2;
  switch (   codedBlockType )
    {
    case 0: 
      windowSequence[MONO_CHAN] =  ONLY_LONG_SEQUENCE;
      break;
    case 1: 
      windowSequence[MONO_CHAN] =  LONG_START_SEQUENCE;
      break;
    case 2: 
      windowSequence[MONO_CHAN] =  EIGHT_SHORT_SEQUENCE;
      break;
    case 3: 
      windowSequence[MONO_CHAN] =  LONG_STOP_SEQUENCE;
      break;
    default: 
      CommonExit(-1,"wrong blocktype %d",   codedBlockType);
    }
  BsGetBit( layer_stream, &ultmp,1);
  windowShape[0] = (WINDOW_SHAPE)ultmp; /* window shape */
  decodedBits += 1;
  
  if (numChannels==2){
    windowSequence[1]=windowSequence[MONO_CHAN];
    windowShape[1] =   windowShape[MONO_CHAN];
  }


  /* decode core layer if present */
  if (lowRateChannelPresent)
    {
      coreDecBits = decodedBits;
      firstAacLay = 1; 

      switch (coreCodecIdx)
        {
        case NTT_TVQ:
#if 0
          tvqAUDecode(  numChannels,
                        frameData, /* config data , obj descr. etc. */
                        tfData,
                        hFault,
                        qc_select,
                        &index, &index_scl,
                        fixed_stream);
          mat_usedNumBit = tfData->decoded_bits;
          windowSequence[MONO_CHAN]=tfData->windowSequence[MONO_CHAN];
          windowShape[MONO_CHAN] =   tfData->windowShape[MONO_CHAN];
          if(numChannels == 2){
            windowSequence[1]=windowSequence[MONO_CHAN];
            windowShape[1] =  windowShape[MONO_CHAN];
          }
#else
          index.w_type=(WINDOW_SEQUENCE)codedBlockType; 
          index.ms_mask = 0;
	  index.group_code = 0x7F ; /*default */
          index.last_max_sfb[0] = 0;
	  /* ... HP */
          ntt_headerdec( -1, layer_stream,  &index,  sfbInfo, &decodedBits,
			 tns_info, nok_lt_status, pred_type,
			 hResilience, hVm, hEpInfo );	  
          coreDecBits = decodedBits;

          coreDecBits += ntt_BitUnPack ( layer_stream, 
                                         BsBufferNumBit(fd->layer[0].bitBuf)-
					 decodedBits,
                                         windowSequence[MONO_CHAN], 
                                         &index );
	  ntt_vq_decoder(&index, spectral_line_vector[0], sfbInfo);
          totalLength =   BsBufferNumBit(fd->layer[0].bitBuf);
          BsGetSkip(layer_stream,(totalLength-coreDecBits ));
          BsCloseRemove ( layer_stream,1 );
        
          layer++;
          layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf); 
          coreChannels=1; /* currently only 1 channel is supported */
	  /*-- long term predictor --*/
	  if(pred_type == NOK_LTP)
            {
              Float current_frame[BLOCK_LEN_LONG];
              Info *info = sfbInfo[index.w_type];
	    
              for(ch = 0; ch < numChannels; ch++)
                {
                  for (i = 0; i < info->bins_per_bk; i++)
                    current_frame[i] = spectral_line_vector[0][ch][i];
	     
                  nok_lt_predict (sfbInfo[index.w_type], 
                                  index.w_type, 
                                  *windowShape,
                                  windowShapePrev,
                                  nok_lt_status[ch]->sbk_prediction_used,
                                  nok_lt_status[ch]->sfb_prediction_used,
                                  nok_lt_status[ch], 
                                  nok_lt_status[ch]->weight,
                                  nok_lt_status[ch]->delay,
                                  current_frame,
                                  info->bins_per_bk,
                                  info->bins_per_bk/med_win_in_long,
                                  info->bins_per_bk/short_win_in_long,
                                  (tvqTnsPresent) ? &tns_info[ch] : NULL, 
                                  qc_select);

                  windowShapePrev = *windowShape;

                  /* This is passed to upper layers. */
                  for (i = 0; i < info->bins_per_bk; i++)
                    spectral_line_vector[0][ch][i] = current_frame[i];
	      
                  /* TNS synthesis filtering. */  
                  for(i = j = 0; i < tns_info[ch].n_subblocks; i++)
                    {
                      tns_decode_subblock(current_frame + j,
                                          index.max_sfb[0],
                                          info->sbk_sfb_top[i],
                                          info->islong,
                                          &(tns_info[ch].info[i]),
                                          qc_select);
		  
                      j += info->bins_per_sbk[i];
                    }
	      
                  nok_ltp_buf_update(index.w_type, *windowShape,
                                     *windowShape, nok_lt_status[ch],
                                     current_frame, info->bins_per_bk,
                                     info->bins_per_bk/med_win_in_long,
                                     info->bins_per_bk/short_win_in_long,
                                     short_win_in_long);
                }
            }
#endif
          break;
        default:
          CommonExit(-1,"\n wrong core coder");
          break;
        }
    
      decodedBits = 0;
    }
  if (lastAacLay>=0){
    if (windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE ){
      BsGetBit( layer_stream, &ultmp,4);        /* max_sfb */
      decodedBits += 4;
      max_sfb[firstAacLay]=ultmp;
      BsGetBit( layer_stream, &ultmp,7);        /* scale_factor_grouping */
      groupInfo = ultmp;
      decodedBits += 7;
      sfbInfo[windowSequence[MONO_CHAN]]->num_groups = 
        decode_grouping( ultmp, sfbInfo[windowSequence[MONO_CHAN]]->group_len );
    }else {
      BsGetBit( layer_stream, &ultmp,6);          /* max_sfb */
      decodedBits += 6;
      max_sfb[firstAacLay]=ultmp;
    }
    /* end of ics_info()*/
  
    /* rest of aac_scaleable_main_header */
  
  
    if( layNumChan[firstAacLay]==2 ){
      int g,sfb,win;
      int gwin;
      BsGetBit( layer_stream, &ultmp,2);     /* ms_mask_present ? */
      decodedBits += 2;
      msMaskPres = ultmp;
      if (msMaskPres==1          
          ) {
        win=0;
        for (g=0;g<sfbInfo[windowSequence[MONO_CHAN]]->num_groups;g++){
          for (sfb=0;sfb<max_sfb[firstAacLay];sfb++){
            BsGetBit( layer_stream, &ultmp,1);    /* ms_mask */
            decodedBits += 1;
            msMask[win][sfb]=ultmp;
          }
          for (gwin=1;gwin<sfbInfo[windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
            for (sfb=0;sfb<max_sfb[firstAacLay];sfb++){                
              msMask[win+gwin][sfb]=msMask[win][sfb];/* apply same ms mask to all 
                                                        subwindows of same group */
            }
          }
          win += sfbInfo[windowSequence[MONO_CHAN]]->group_len[g];
        }
      }else if (msMaskPres==2){
        win=0;
        for (g=0;g<sfbInfo[windowSequence[MONO_CHAN]]->num_groups;g++){
          for (gwin=0;gwin<sfbInfo[windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
            for (sfb=0;sfb<max_sfb[firstAacLay];sfb++){
              msMask[win][sfb]=1;
            }
            win++;
          }
        }
      }  
    }    
  
    diffControlBands =0;

    /* tns_channel_mono_layer bit (To DO !!) */
    if((layNumChan[firstAacLay]==2) && (coreChannels == 1) && lowRateChannelPresent && !tvqTnsPresent) {
      BsGetBit( layer_stream, &ultmp, 1);
    }


    for (ch=0;ch<layNumChan[firstAacLay];ch++){

      /* TNS data */
      BsGetBit( layer_stream, &ultmp,1);
      tns_data_present[ch] = ultmp;
      decodedBits += 1;
      if( ultmp ) {
        get_tns_vm( layer_stream, sfbInfo[windowSequence[MONO_CHAN]], &tns_info[ch] ,&decodedBits);
      } else {
        clr_tns( sfbInfo[windowSequence[MONO_CHAN]], &tns_info[ch] );
      }
    
      if (coreCodecIdx!=NO_CORE) {
        if ((ch==0) || ((ch==1) && (coreChannels==2 ) ) ){
          if (windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
            int g;
            for (g=0;g<8;g++){
              BsGetBit( layer_stream,&ultmp, 1 );  /* short FSSwitch  1st aacLayer  */
              decodedBits += 1;
              diffControl[ch][g]= ultmp;
              if (ch==0){
                diffControlBands +=1;
              }
            }
          } else {
            decodeDiffCtrl(layer_stream,diffControl[ch],&decodedBits,
                           sfbInfo[windowSequence[MONO_CHAN]]->longFssGroups);/*  FSSwitch 1st aacLayer */
            if (ch==0){
              diffControlBands +=sfbInfo[windowSequence[MONO_CHAN]]->longFssGroups*4;
            }
          }
        }
        if ((coreChannels == 1) && (layNumChan[firstAacLay]==2)){
          int g,sfb;
          /* diffcontrol lr */
          if (windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
            diffControlBands = 8;
            for (g=0;g<8;g++){
              BsGetBit( layer_stream,&ultmp, 1 );  /* short FSS */
              decodedBits += 1;
              diffControl[ch][g]= ultmp;                
            }
          } else {
            diffControlBands = max_sfb[aacLayer-1];
            for (sfb=0;sfb< max_sfb[aacLayer-1];sfb++){
              if (msMask[0][sfb]==0) {
                BsGetBit( layer_stream,&ultmp, 1 );  /* long FSS */
                decodedBits += 1;
                diffControl[ch][sfb]= ultmp;
              } else {
                diffControl[ch][sfb]= 0;/* allways on if ms on  channel */
              }
            }
          }
        }
      } else {
	if(pred_type == NOK_LTP)
          {
            setHuffdec2BitBuffer(layer_stream);

            /* LTP data. */
            decodedBits += 
              nok_lt_decode(windowSequence[MONO_CHAN], &max_sfb[firstAacLay], 
                            nok_lt_status[ch]->sbk_prediction_used, 
                            nok_lt_status[ch]->sfb_prediction_used, 
                            &nok_lt_status[ch]->weight, nok_lt_status[ch]->delay,
                            hResilience, hVm, hEpInfo, qc_select, NO_CORE, -1);

            ResetReadBitCnt (hVm);
          }
	else
          {
            BsGetBit(layer_stream, &ultmp, 1);  /* no prediction data  */
            decodedBits += 1;
          }
      }
    }
  

    /* decode 1st AAC Layer */
    used_bits[firstAacLay][0] = decodedBits;
    aacLayer=firstAacLay;
    if(qc_select == AAC_SCALABLE) {
      tmp = AacDecodeFrame( layer_stream,
                            gc_WRstream,
                            spectral_line_vector[aacLayer], 
                            &(windowSequence[MONO_CHAN]), 
                            windowShape,
                            SCALABLE,
                            &max_sfb[aacLayer],
                            layNumChan[aacLayer],
                            commonWindow[aacLayer],
                            sfbInfo, 
                            sfbCbMap[aacLayer],
                            hResilience,
                            hVm,
                            hHcrSpecData,
                            hHcrInfo,
                            hEpInfo,
                            hConcealment,
                            qc_select,
			    nok_lt_status);

    }
    used_bits[aacLayer][1] = tmp;
    decodedBits +=tmp;
    totalLength = BsBufferNumBit(fd->layer[aacLayer].bitBuf);
    BsGetSkip(layer_stream,(totalLength-decodedBits)); 
    BsCloseRemove ( layer_stream ,1);
    /* end of 1st aac layer */
    layer++;
  }

  /* read and decode aac_scaleable_extension_stream(s) */
    for ( aacLayer=firstAacLay+1;aacLayer<=lastAacLay;aacLayer++){
      layer_stream= BsOpenBufferRead(fd->layer[aacLayer].bitBuf); 
      decodedBits =0;
      if (windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE){
        BsGetBit( layer_stream, &ultmp,4);          /* max_sfb_med */
        decodedBits += 4;
        max_sfb[aacLayer]=ultmp;
      } else {
        BsGetBit( layer_stream, &ultmp,6);          /*  max_sfb_med */
        decodedBits += 6;
        max_sfb[aacLayer]=ultmp;
      }
    
      if( (layNumChan[aacLayer]==2)){  /* rest of ms mask which was not transmitted in previous layer */
        int g,sfb,win,gwin;
        int startSfb;
        BsGetBit( layer_stream, &ultmp,2);     /* ms_mask_present ? */
        decodedBits += 2;
        msMaskPres = ultmp;          
        if (msMaskPres==1){
          win=0;
          for (g=0;g<sfbInfo[windowSequence[MONO_CHAN]]->num_groups;g++){
            if (layNumChan[aacLayer-1]!=layNumChan[aacLayer]){
              startSfb=0;
            } else {
              startSfb=max_sfb[aacLayer-1];
            }
            for (sfb=startSfb;sfb<max_sfb[aacLayer];sfb++){
              BsGetBit( layer_stream, &ultmp,1);    /* ms_mask */
              decodedBits += 1;
              msMask[win][sfb]=ultmp;
            }
            for (gwin=1;gwin<sfbInfo[windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
              for (sfb=startSfb;sfb<max_sfb[aacLayer];sfb++){                
                msMask[win+gwin][sfb]=msMask[win][sfb];/* apply same ms mask to all 
                                                          subwindows of same group */
              }
            }
            win += sfbInfo[windowSequence[MONO_CHAN]]-> group_len[g] ;
          }          
        }else if (msMaskPres==2){
          win=0;
          for (g=0;g<sfbInfo[windowSequence[MONO_CHAN]]->num_groups;g++){
            if (layNumChan[aacLayer-1]!=layNumChan[aacLayer]){
              startSfb=0;
            } else {
              startSfb=max_sfb[aacLayer-1];
            }
            for (g=0;g<sfbInfo[windowSequence[MONO_CHAN]]->num_groups;g++){
              for (gwin=0;gwin<sfbInfo[windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
                for (sfb=startSfb;sfb<max_sfb[aacLayer];sfb++){
                  msMask[win][sfb]=1;
                }
                win++;
              }
            }
          }  
        }
      }
      diffControlBands =0;            
      if ( (layNumChan[aacLayer-1]!=layNumChan[aacLayer]) ) {
        int sfb,g;
        if ( (layNumChan[aacLayer-1] == 1) && (layNumChan[aacLayer] == 2)  ){
          for (ch=0;ch<layNumChan[aacLayer];ch++){
            BsGetBit( layer_stream, &ultmp,1);           /* tns for left right*/
            tns_ext_present[ch] = ultmp;
            decodedBits += 1;
            if( ultmp ) {
              get_tns_vm( layer_stream, sfbInfo[windowSequence[MONO_CHAN]], &tns_info_ext[ch] ,&decodedBits);
            } else {
              clr_tns( sfbInfo[windowSequence[MONO_CHAN]], &tns_info_ext[ch] );
            }
            
            if (windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
              diffControlBands = 8;
              for (g=0;g<8;g++){
                BsGetBit( layer_stream,&ultmp, 1 );  /* short FSS */
                decodedBits += 1;
                diffControl[ch][g]= ultmp;                
              }
            } else {
              diffControlBands = max_sfb[aacLayer-1];
              for (sfb=0;sfb< max_sfb[aacLayer-1];sfb++){
                if (msMask[0][sfb]==0) {
                  BsGetBit( layer_stream,&ultmp, 1 );  /* long FSS */
                  decodedBits += 1;
                  diffControl[ch][sfb]= ultmp;
                } else {
                  diffControl[ch][sfb]= 0;/* allways on if ms on  channel */
                }
              }
            }
          }        
        }
      }
      used_bits[aacLayer][0] = decodedBits;
    
      ResetReadBitCnt ( hVm );  
      /*
        {
        double zero=0.0;
        vcopy( &zero, spectral_line_vector[aacLayer][0], 0, 1,1024 );
        vcopy( &zero, spectral_line_vector[aacLayer][1], 0, 1,1024 );
        }*/
      
      tmp = AacDecodeFrame( layer_stream,
                            gc_WRstream,
                            spectral_line_vector[aacLayer], 
                            &(windowSequence[MONO_CHAN]), 
                            windowShape,
                            SCALABLE,
                            &max_sfb[aacLayer],
                            layNumChan[aacLayer],
                            commonWindow[aacLayer],
                            sfbInfo, 
                            sfbCbMap[aacLayer],
                            hResilience,
                            hVm,
                            hHcrSpecData,
                            hHcrInfo,
                            hEpInfo,
                            hConcealment,
                            qc_select,
			    nok_lt_status);

      used_bits[aacLayer][1] = tmp;
      decodedBits +=tmp;
      totalLength =   BsBufferNumBit(fd->layer[aacLayer].bitBuf);
      BsGetSkip(layer_stream,(totalLength-decodedBits));
      BsCloseRemove ( layer_stream ,1);

    }



  if ( (coreCodecIdx!=NO_CORE) && (output_select > 0) && (tfData->tnsBevCore==0)  ) {
    /* tns encoder for mono/core  layer */
    int k, j;
    Info *info =sfbInfo[windowSequence[MONO_CHAN]];
    info->islong = (windowSequence[MONO_CHAN] != EIGHT_SHORT_SEQUENCE);
    ch = 0;
    for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
      float tmp_spec[1024];
      for( k=0; k < info->bins_per_sbk[i]; k++ ) {
        tmp_spec[k] = spectral_line_vector[0][ch][j+k];
      }
      tns_encode_subblock( tmp_spec,
                           max_sfb[1],   /* max_sfb[wn] Attention: wn and not ch !! */
                           info->sbk_sfb_top[i],
                           info->islong,
                           &(tns_info[ch].info[i]),
                           qc_select);      
        
      for( k=0; k < info->bins_per_sbk[i]; k++ ) {
        spectral_line_vector[0][ch][j+k] = tmp_spec[k];
      }
        
      j += info->bins_per_sbk[i];
    }
  }
  
  /* compute PNS and restore spectrum */
  {
    int sfb, maxSfb, lineIdx=0;
    int prevSfbCb,sfbCb, sfbWidth;
    double energy, zero = 0.0;
 
    if (sfbInfo[windowSequence[MONO_CHAN]]->islong) {
 
      for (aacLayer=firstAacLay+1; aacLayer <= calcMonoLay; aacLayer++) {       
        /* for the first layer do nothing */
        maxSfb = (max_sfb[aacLayer-1]<max_sfb[aacLayer])?max_sfb[aacLayer-1]:max_sfb[aacLayer];
        
        for (ch=0;ch<layNumChan[aacLayer];ch++) {
        
          for (sfb=0; sfb<maxSfb; sfb++) {
            prevSfbCb = (int)sfbCbMap[aacLayer-1][ch][sfb];/* does not work for mono-stereo combination */
            sfbCb     = (int)sfbCbMap[aacLayer][ch][sfb];
            if (sfb==0) {
              sfbWidth = sfbInfo[windowSequence[MONO_CHAN]]->bk_sfb_top[sfb];
            }
            else {
              sfbWidth = sfbInfo[windowSequence[MONO_CHAN]]->bk_sfb_top[sfb]-
                sfbInfo[windowSequence[MONO_CHAN]]->bk_sfb_top[sfb-1];
            }
            
            if (prevSfbCb == NOISE_HCB) {
              energy = scalprod(&spectral_line_vector[aacLayer][ch][lineIdx], 
                                &spectral_line_vector[aacLayer][ch][lineIdx], 1, 1, sfbWidth);
              /* if sfb of enhancement layer has MDCT lines, set sfb of previous layer to 0 */
              if (energy > 0) {
                vcopy(&zero, &spectral_line_vector[aacLayer-1][ch][lineIdx], 0 ,1 ,sfbWidth);
              }
              else {
                sfbCbMap[aacLayer][ch][sfb] = sfbCbMap[aacLayer-1][ch][sfb];
                vcopy(&spectral_line_vector[aacLayer-1][ch][lineIdx],
                      &spectral_line_vector[aacLayer][ch][lineIdx], 1, 1, sfbWidth);
                vcopy(&zero,&spectral_line_vector[aacLayer-1][ch][lineIdx], 0, 1, sfbWidth);  
              }
              if (sfbCb == NOISE_HCB) {
                CommonWarning("Two consecutive layers have PNS in same SFB !");
              } 
            }
            lineIdx += sfbWidth; 
          }
        }
      }    
    }
  }

  /* combine all mono layers */
  for (aacLayer=firstAacLay+1 ; aacLayer <= calcMonoLay ; aacLayer++) {

    if (!debugDiff){
      vadd(   spectral_line_vector[firstAacLay][MONO_CHAN],
              spectral_line_vector[aacLayer][MONO_CHAN],spectral_line_vector[firstAacLay][MONO_CHAN],  
              1,1,1, sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk );
    } else {
      vcopy (  spectral_line_vector[aacLayer][MONO_CHAN] , 
               spectral_line_vector[firstAacLay][MONO_CHAN],
               1,1 ,sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk);
    }

  }

  /* LTP processing. */
  if(coreCodecIdx == NO_CORE && pred_type == NOK_LTP)
    {
      TNS_frame_info *tns_info_ltp[2];
    

      tns_info_ltp[0] = &tns_info[0];
      tns_info_ltp[1] = &tns_info[1];
    
      ltp_scal_reconstruct(windowSequence[MONO_CHAN],
                           *windowShape, *windowShape,
                           layNumChan[firstAacLay],
                           spectral_line_vector[firstAacLay],
                           sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk,
                           med_win_in_long, short_win_in_long,
                           max_sfb[firstAacLay], msMask, nok_lt_status, 
                           sfbInfo[windowSequence[MONO_CHAN]], 
                           tns_info_ltp, qc_select);
    }

  /* combine all stereo layers */
    
  if (firstStLay!=-1 ) {
    for (aacLayer=firstStLay+1 ; aacLayer <= output_select ; aacLayer++) {
      for (ch=0;ch<layNumChan[aacLayer];ch++){
        vadd(   spectral_line_vector[firstStLay][ch],spectral_line_vector[aacLayer][ch],
                spectral_line_vector[firstStLay][ch],  
                1,1,1, sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk );
      }
    }
  }
  
  /* fss for aac mono layer */
  if ( (output_select > 0) && (lastMonoLay>0) ){
    double tmp[1024];
    low_spec=spectral_line_vector[0][0];
    shortFssWidth = sfbInfo[windowSequence[MONO_CHAN]]->shortFssWidth;
    if (lowRateChannelPresent) {
      FSSwitch( low_spec, spectral_line_vector[firstAacLay][MONO_CHAN],
                tmp,
                diffControl[MONO_CHAN], sfbInfo[windowSequence[MONO_CHAN]] ,
                diffControlBands , shortFssWidth,1 );    
      vcopy (  tmp, spectral_line_vector[firstAacLay][MONO_CHAN],1,1 ,
               sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk);
    }
      
  } 

  if ( ( layNumChan[firstAacLay]==1 )  && ( layNumChan[lastAacLay]==2 ) ) {
    vcopy (  spectral_line_vector[firstAacLay][MONO_CHAN] , 
             spectral_line_vector[firstAacLay][MONO_CHAN+1],
             1,1 ,sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk);        
  }

  low_spec=spectral_line_vector[firstAacLay][MONO_CHAN];
  /* fss for aac stereo layer */
  if ( (output_select >= firstStLay ) && (firstStLay!=-1 ) && (layNumChan[firstAacLay]==1)){
    double tmp_spec[1024];
    /* inverse ms matrix */
    
    multCoreSpecMsFac(sfbInfo[windowSequence[MONO_CHAN]],msMask,low_spec,tmp_spec);
    if (layNumChan[firstAacLay]==2 ){
      shortFssWidth = sfbInfo[windowSequence[MONO_CHAN]]->shortFssWidth;
    }else{
      if  (max_sfb[aacLayer-1]!=0) {
        shortFssWidth = sfbInfo[windowSequence[MONO_CHAN]]->sbk_sfb_top[0][ max_sfb[firstStLay-1]-1] ; 
      } else {
        shortFssWidth=0;
      }
    }
    for (ch=0;ch<layNumChan[firstStLay];ch++){                    
      FSSwitchSt( tmp_spec, 
                  spectral_line_vector[firstStLay][ch],
                  spectral_line_vector[firstAacLay][ch],
                  diffControl[ch],
                  diffControlBands,
                  sfbInfo[windowSequence[MONO_CHAN]], 
                  msMask,
                  ch );    
    }
    doInverseMsMatrix( sfbInfo[windowSequence[MONO_CHAN]],spectral_line_vector[firstAacLay][0], 
                       spectral_line_vector[firstAacLay][1],  msMask );
  } else if ((output_select >= firstStLay ) && (firstStLay!=-1 ) 
             && (coreCodecIdx != NTT_TVQ) ){
    double tmp_spec[1024];
    doInverseMsMatrix( sfbInfo[windowSequence[MONO_CHAN]],spectral_line_vector[firstStLay][0], 
                       spectral_line_vector[firstStLay][1],  msMask );
    if (lowRateChannelPresent){
      multCoreSpecMsFac(sfbInfo[windowSequence[MONO_CHAN]],msMask,low_spec,tmp_spec);
      for (ch=0;ch<layNumChan[firstStLay];ch++){                    
        FSSwitch( tmp_spec, spectral_line_vector[firstStLay][ch],
                  spectral_line_vector[firstAacLay][ch],
                  diffControl[ch], sfbInfo[windowSequence[MONO_CHAN]] ,
                  diffControlBands , sfbInfo[windowSequence[MONO_CHAN]]->shortFssWidth ,1 );    
      }
    }
  }


  if (output_select) {
    if (layNumChan[output_select]==2){
      for (ch=0;ch<layNumChan[lastAacLay];ch++){
        vcopy( spectral_line_vector[firstAacLay][ch] ,   spectral_line_vector[0][ch] , 1, 1,  
               sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk ); 
      }        
    } else {
      for (ch=0;ch<numChannels;ch++){
        vcopy( spectral_line_vector[firstAacLay][MONO_CHAN] ,   spectral_line_vector[0][ch] , 1, 1,  
               sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk ); 
      }        
    }
  } else {
    if ((layNumChan[lastAacLay]==2)&&(layNumChan[output_select]==1)){
      vcopy( spectral_line_vector[0][0] ,   spectral_line_vector[0][1] , 1, 1,  
             sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk ); 
    }
  }

  /* Bandwidth control */
  if ((lopLong != 0) && (max_sfb[lastAacLay] != 0) && (output_select == 0)) {
    int maxline;
      
    if (normBw == 0.0F) {
      maxline = sfbInfo[windowSequence[MONO_CHAN]]->sbk_sfb_top[0][max_sfb[lastAacLay]-1];
    }
    else {
      maxline = (int)((float)(sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk 
                              /sfbInfo[windowSequence[MONO_CHAN]]->nsbk)*normBw + 0.5);
    } 
    for (ch=0; ch<layNumChan[lastAacLay]; ch++) {
      dec_lowpass(spectral_line_vector[0][ch], lopLong, lopShort, maxline,
                  sfbInfo[windowSequence[MONO_CHAN]]->bins_per_bk, windowSequence[MONO_CHAN],
                  sfbInfo[windowSequence[MONO_CHAN]]->nsbk);
    }
  }

  /* tns decoder for tns of main header */
  if  ( ! ( 
           (output_select == 0) && (lowRateChannelPresent) && (tfData->tnsBevCore==0) 
           ) )  {
    TNS_frame_info tns_stereo_info[2];
    if (layNumChan[firstAacLay] ==1 ){
      tns_stereo_info[0] = tns_info[0];
      tns_stereo_info[1] = tns_info[0];      
    } else {
      tns_stereo_info[0] = tns_info[0];
      tns_stereo_info[1] = tns_info[1];      
    }
    for (ch=0;ch<layNumChan[lastAacLay];ch++){      
      int k, j;
      Info *info = sfbInfo[windowSequence[MONO_CHAN]];
      
      for (i=j=0; i<tns_stereo_info[ch].n_subblocks; i++) {
        float tmp_spec[1024];
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tmp_spec[k] = spectral_line_vector[0][ch][j+k];
        }
        tns_decode_subblock( tmp_spec,
                             max_sfb[output_select],  
                             info->sbk_sfb_top[i],
                             info->islong,
                             &(tns_stereo_info[ch].info[i]),
                             qc_select);      
        
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          spectral_line_vector[0][ch][j+k] = tmp_spec[k];
        }
        
        j += info->bins_per_sbk[i];
      }
    }
  }

  /* tns decoder for tns of 1st extension header */
  if ( ( (firstStLay>=1) && (output_select >= firstStLay) && (!lowRateChannelPresent) ) 
       || ( (firstStLay>=2) && (output_select >= firstStLay) && (lowRateChannelPresent))
       ) {
    TNS_frame_info tns_stereo_info[2];
    
    tns_stereo_info[0] = tns_info_ext[0];
    tns_stereo_info[1] = tns_info_ext[1];      
    
    for (ch=0;ch<layNumChan[lastAacLay];ch++){      
      int k, j;
      Info *info = sfbInfo[windowSequence[MONO_CHAN]];
      if (tns_ext_present[ch]==1){
        for (i=j=0; i<tns_stereo_info[ch].n_subblocks; i++) {
          float tmp_spec[1024];
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tmp_spec[k] = spectral_line_vector[0][ch][j+k];
          }
          tns_decode_subblock( tmp_spec,
                               max_sfb[output_select],  
                               info->sbk_sfb_top[i],
                               info->islong,
                               &(tns_stereo_info[ch].info[i]),
                               qc_select);      
          
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            spectral_line_vector[0][ch][j+k] = tmp_spec[k];
          }
          
          j += info->bins_per_sbk[i];
        }
      }
    }
  }
#ifdef DEBUGPLOT_FHG 
  plotSend("c", "mdctCore",  MTV_DOUBLE,1024, low_spec , NULL);

  plotSend("l", "mdctSpecL",  MTV_DOUBLE_SQA,1024, spectral_line_vector[0][0] , NULL);
  /*   plotSend("r", "mdctSpecR" , MTV_DOUBLE,1024, spectral_line_vector[firstAacLay][1] , NULL); */
#endif    
 

  /* increment granule counter */
  gran += 1;
  if( gran == nrGranules ) {
    gran = 0;
  }
}

void tfScaleableAUDecode ( int          numChannels, 
                           FRAME_DATA*  fd, /* config data , obj descr. etc. */
                           TF_DATA*     tfData,
                           LPC_DATA*    lpcData,                           
                           ntt_DATA*           nttData,
                           HANDLE_FAULT_TOLERANT hFault,
                           QC_MOD_SELECT      qc_select
                            )
{
#if 0
  BsBitStream*       fixed_stream;
  BsBitStream*       gc_WRstream[];
  WINDOW_SEQUENCE        windowSequence[MAX_TIME_CHANNELS];
  int*               flexMuxBits;
  double*            spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS];
  Info**             sfbInfo;
  int                numChannels;
  int                lopLong;
  int                lopShort;
  float              normBw;
  WINDOW_SHAPE*      windowShape;
  QC_MOD_SELECT      qc_select;
  HANDLE_RESILIENCE  hResilience;
  HANDLE_BUFFER      hVm;
  HANDLE_BUFFER      hHcrSpecData;
  HANDLE_HCR         hHcrInfo;
  FRAME_DATA*        fd;
  HANDLE_EP_INFO     hEpInfo;
  HANDLE_CONCEALMENT hConcealment;
  int                short_win_in_long;
  PRED_TYPE          pred_type;
#endif
    
  int                med_win_in_long=2; /* dummy , not used */
  int                short_win_in_long = 8;
  BsBitStream*       gc_DummyStream[MAX_TIME_CHANNELS]={0x0};
  
  BsBitStream* layer_stream ;
  
  int                lopLong=0;
  int                lopShort=0;
  float              normBw=0.0f;
  int            shortFssWidth;
  double*        low_spec;
  int lowRateChannelPresent;
  enum CORE_CODEC  coreCodecIdx;
  int     lastAacLay = 0;  
  int     firstAacLay = 0;  
  int     layNumChan[MAX_TF_LAYER];
  int     firstStLay;
  int     lastMonoLay;
  unsigned long  ultmp,codedBlockType;
  int            diffControl  [2][ 60 ] = {{0x0}};
  int            diffControlLR[2][ 60 ] = {{0x0}};

  int            commonWindow[MAX_TF_LAYER];
  int            aacLayer = 0;
  int            layer;
  int            ch,calcMonoLay;
  int            diffControlBands;
  int            diffControlBandsExt,diffControlBandsLR ;
  int            i;
  int            tmp = 0;
  int            msMask[8][60]={{0x0}};
  int            msMaskPres = 0;
  TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  TNS_frame_info tns_info_ext[MAX_TIME_CHANNELS];
  byte           max_sfb[Winds];
  byte           sfbCbMap[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS] = {{{0x0}}};
  int            groupInfo = 0;
  int            tns_data_present[2];
  int            tns_ext_present[2];
  int            used_bits[4][2];
  int            decodedBits;
  ntt_INDEX     index, index_scl;
  int           coreChannels;
  unsigned long totalLength;
  int           tvqTnsPresent;
  int           output_select;

  /* init */
  decodedBits       = 0;
  tvqTnsPresent     = 0;
  coreChannels      = 0;
  tns_ext_present[0]= 0;
  tns_ext_present[1]= 0;
  
  output_select = tfData->output_select;
  
  index.nttDataBase=nttData->nttDataBase;
  index_scl.nttDataScl=nttData->nttDataScl;
  index.numChannel =numChannels;
  index_scl.numChannel =numChannels;
  /*********************/

  layer=0;

  getModeFromOD(fd->od,&lowRateChannelPresent ,layNumChan ,&lastAacLay ,&firstStLay,&coreCodecIdx,  
                commonWindow);

  
  if(coreCodecIdx==NTT_TVQ)
     output_select -= index_scl.nttDataScl->ntt_NSclLay;
  if (output_select>lastAacLay) output_select=lastAacLay;
  if (output_select<0) output_select=0; /* YB 980508 */
  if (firstStLay>=0)
    lastMonoLay=firstStLay-1;/* debug */
  else 
    lastMonoLay=lastAacLay;/* debug */
  if (output_select > lastMonoLay ){
    calcMonoLay=lastMonoLay ;
  } else {
    calcMonoLay=output_select ;      
  }


  /* open the first layer bitbuffer  to read :
     can  be a AAC or TwinVq bitbuffer as core */
  if (fd->layer[layer].NoAUInBuffer<=0) { CommonWarning("no Access unit; skiping frame"); return ;}
  layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);


  /* 
     According to the new bitstream syntax (98/11/12), 
     there is no more ics_reserved_bit for scalable
  */
  if ( (coreCodecIdx==CC_CELP_MPEG4) ) {
    float coreSig[1024];
    int i,k;
    int ifactor=tfData->samplFreqFacCore;

    firstAacLay = 1; 
    /* celp decoder */
    coreChannels      = 1;

    while ( GetNumVal ( tfData->coreModuloBuffer ) < tfData->block_size_samples ) {
      if (BsGetBuffer( layer_stream, lpcData->coreBitBuf, lpcData->bitsPerFrame) )
        CommonExit(-11,"Decode core bitstream : error get buffer");

      DecLpcFrameNew(lpcData->coreBitBuf,lpcData->sampleBuf,lpcData,&tmp);      
      if (tmp!=lpcData->bitsPerFrame)
        CommonExit(1,"celp decoder error, wrong num bit read");
      /*    upsampling  */
      for( i=0; i< lpcData->frameNumSample ;i++ ) {
        coreSig[i*ifactor] = lpcData->sampleBuf[0][i] * ifactor;
        for( k=1; k<ifactor; k++ ) {
          coreSig[i*ifactor+k] = 0;
        }
      }            
#ifdef DEBUGPLOT 
    plotSend("c", "timeCore2",  MTV_FLOAT,tfData->block_size_samples/ifactor,lpcData->sampleBuf[0] , NULL);
#endif
    AddFloatModuloBufferValues( tfData->coreModuloBuffer,coreSig , 
                                lpcData->frameNumSample * tfData->samplFreqFacCore);
    }
    /* prepare decoding of next layer */

/*     fd->layer[layer].NoAUInBuffer--; */

    BsCloseRemove(layer_stream,1);
    fd->layer[layer].NoAUInBuffer--;
    layer++;
  }
  if (!(lowRateChannelPresent && (coreCodecIdx==NTT_TVQ))
      && (lastAacLay>=0)
      && (fd->layer[layer].NoAUInBuffer>0)){
    layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);
        
    /* bugfix by tmn@iis.fhg.de ; after official release date  99-08-09 */   
    BsGetBit( layer_stream, &ultmp,1); /* reserved bit */
    decodedBits += 1;

    BsGetBit( layer_stream, &ultmp,2); /* window_sequence */
    codedBlockType = (int)ultmp;
    decodedBits += 2;
    switch (   codedBlockType )
      {
      case 0: 
      tfData->windowSequence[MONO_CHAN] =  ONLY_LONG_SEQUENCE;
      break;
      case 1: 
        tfData->windowSequence[MONO_CHAN] =  LONG_START_SEQUENCE;
        break;
      case 2: 
        tfData->windowSequence[MONO_CHAN] =  EIGHT_SHORT_SEQUENCE;
      break;
      case 3: 
        tfData->windowSequence[MONO_CHAN] =  LONG_STOP_SEQUENCE;
        break;
      default: 
        CommonExit(-1,"wrong blocktype %d",   codedBlockType);
      }
    BsGetBit( layer_stream, &ultmp,1);
    tfData->windowShape[0] = (WINDOW_SHAPE)ultmp; /* window shape */
    decodedBits += 1;
    
    if (numChannels==2){
      tfData->windowSequence[1]=tfData->windowSequence[MONO_CHAN];
      tfData->windowShape[1] =   tfData->windowShape[MONO_CHAN];
    }
  }
  
  if ( (coreCodecIdx==CC_CELP_MPEG4) ) {
    float coreSig[1024];
    double dummy=0.0;
    int coreBWlines;
    /* forward mdct for celp core */
    ReadFloatModuloBufferValues(tfData->coreModuloBuffer, coreSig, tfData->block_size_samples);
    mdct_core ( tfData,
                coreSig,
                tfData->spectral_line_vector[0][0],
                0,
                /* tfData->tfSamplRate, */
                tfData->block_size_samples );
    /* lowpass long: not realy necessary but usefull if upsampled celp layer  should be written directly 
       to file for debugging*/
    if (tfData->windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE ){
      int sbk;      
      for( sbk=0; sbk<tfData->sfbInfo[codedBlockType]->nsbk; sbk++ ) {
        coreBWlines=(int)((float)tfData->sfbInfo[codedBlockType]->bins_per_sbk[sbk]
                          /(float)tfData->samplFreqFacCore+0.5);
        vcopy( &dummy, &tfData->spectral_line_vector[0][0][coreBWlines
                                                      +tfData->sfbInfo[codedBlockType]->bins_per_sbk[sbk]*sbk]
               , 0,1, tfData->sfbInfo[codedBlockType]->bins_per_sbk[sbk]-coreBWlines );
      }      
    } else {
      coreBWlines=(int)((float)tfData->block_size_samples/(float)tfData->samplFreqFacCore+0.5);
      vcopy( &dummy, &tfData->spectral_line_vector[0][0][coreBWlines], 0,1, tfData->block_size_samples-coreBWlines );
    }
#if 0
/*#warning "debug" */
    vcopy( &dummy, &tfData->spectral_line_vector[0][0][0], 0,1, tfData->block_size_samples );
#endif
#ifdef DEBUGPLOT 
/*     vcopy( &dummy, &tfData->spectral_line_vector[0][0][0], 0,1, 960 ); */
    plotSend("c", "mdctCore",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[0][0] , NULL);
    plotSend("c", "timeCore",  MTV_FLOAT,tfData->block_size_samples,coreSig , NULL);
#endif
  }
  
  
  /* decode TVQ core layer if present */
  if (lowRateChannelPresent && (coreCodecIdx==NTT_TVQ)) {
    AUDIO_SPECIFIC_CONFIG *audSpC = &fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
  
    {
    int fsampf[16]={96,88,64,48,44,32,24,22,16,12,11,8,-1,-1,-1,-1};
      index.isampf=
      fsampf[ fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.\
      samplingFreqencyIndex.value];
	   index_scl.isampf = index.isampf;
    }

    index.nttDataBase=nttData->nttDataBase;
    index_scl.nttDataScl=nttData->nttDataScl;
    index.nttDataScl = nttData->nttDataScl;
    index.numChannel =audSpC->channelConfiguration.value;
    index_scl.numChannel =audSpC->channelConfiguration.value;
    index.block_size_samples=
      ( fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.
        specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;
    index_scl.block_size_samples= index.block_size_samples;
    index.w_type=(WINDOW_SEQUENCE)codedBlockType; 
    /* maybe obsolete ???   HP 990201 ... */
    index.ms_mask = 0;
    index.group_code = 0x7F ; /*default */
    index.last_max_sfb[0] = 0;
  /* ... HP */
    tvqAUDecode2(  fd, /* config data , obj descr. etc. */
                   tfData,
                   hFault,
                   qc_select,
                   &index, &index_scl,
                   &layer);
    decodedBits = tfData->decoded_bits;
#if 0
fprintf(stderr, "ttttttttTTT ntt_NSclLay %2d %2d layer%2d fir%2d las%2d out%2d mono%2d st%2d mx%2d\n", 
  index_scl.nttDataScl->ntt_NSclLay, index_scl.nttDataScl->ntt_NSclLayDec,
  layer, firstAacLay, lastAacLay, output_select, lastMonoLay, firstStLay,
  index_scl.max_sfb[layer-1]);
#endif
    /* index_scl.nttDataScl->ntt_NSclLay == num of tvq enhance layers */
    firstAacLay = layer - index_scl.nttDataScl->ntt_NSclLay; 
    lastMonoLay -= index_scl.nttDataScl->ntt_NSclLay;
    firstStLay -= index_scl.nttDataScl->ntt_NSclLay;
    if(firstStLay < -1) firstStLay = -1;
#ifndef _MSC_VER
/*#warning "max_sfb problem unresolved" */
#endif
    /* im not shure which one of of the following 2 lines is correct ? */
    /*max_sfb[layer-1]=index.max_sfb[0]; */
    /*max_sfb[layer-1]=index_scl.max_sfb[0]; */
    if(layer>0) max_sfb[firstAacLay-1]=index_scl.max_sfb[layer-1];

    if(numChannels == 2){
      tfData->windowSequence[1]=    tfData->windowSequence[MONO_CHAN];
      tfData->windowShape[1] =      tfData->windowShape[MONO_CHAN];
    } 
    decodedBits = 0;

  }
  aacLayer=firstAacLay;

  if (lastAacLay>=0){
    if (fd->layer[layer].NoAUInBuffer>0){

      if (!(!(lowRateChannelPresent && (coreCodecIdx==NTT_TVQ))
            && (lastAacLay>=0)
            && (fd->layer[layer].NoAUInBuffer>0))){
        layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);  
      }
      
      if (tfData->windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE ){
        BsGetBit( layer_stream, &ultmp,4);        /* max_sfb */
        decodedBits += 4;
        max_sfb[firstAacLay]=ultmp;
        BsGetBit( layer_stream, &ultmp,7);        /* scale_factor_grouping */
        groupInfo = ultmp;
        decodedBits += 7;
        tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups = 
        decode_grouping( ultmp, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->group_len );
    }else {
      BsGetBit( layer_stream, &ultmp,6);          /* max_sfb */
      decodedBits += 6;
      max_sfb[firstAacLay]=ultmp;
    }
    /* end of ics_info()*/
  
    /* rest of aac_scaleable_main_header */
  
  
    if( layNumChan[firstAacLay]==2 ){
      int g,sfb,win;
      int gwin;
      BsGetBit( layer_stream, &ultmp,2);     /* ms_mask_present ? */
      decodedBits += 2;
      msMaskPres = ultmp;
      if (msMaskPres==1          
      ) {
        win=0;
        for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
          for (sfb=0;sfb<max_sfb[firstAacLay];sfb++){
            BsGetBit( layer_stream, &ultmp,1);    /* ms_mask */
            decodedBits += 1;
            msMask[win][sfb]=ultmp;
          }
          for (gwin=1;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
            for (sfb=0;sfb<max_sfb[firstAacLay];sfb++){                
              msMask[win+gwin][sfb]=msMask[win][sfb];/* apply same ms mask to all 
                                                        subwindows of same group */
            }
          }
          win += tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->group_len[g];
        }
      }else if (msMaskPres==2){
        win=0;
        for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
          for (gwin=0;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
            for (sfb=0;sfb<max_sfb[firstAacLay];sfb++){
              msMask[win][sfb]=1;
            }
            win++;
          }
        }
      }  
    }    
  
    diffControlBands =0;
    /* tns_channel_mono_layer bit (To DO !!) */
    if((layNumChan[firstAacLay]==2) && (coreChannels == 1) && lowRateChannelPresent && !tvqTnsPresent) {
      BsGetBit( layer_stream, &ultmp, 1);
      decodedBits += 1;
    }


    for (ch=0;ch<layNumChan[firstAacLay];ch++){

      /* TNS data */
      BsGetBit( layer_stream, &ultmp,1);
      tns_data_present[ch] = ultmp;
      decodedBits += 1;
      if( ultmp ) {
        get_tns_vm( layer_stream, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info[ch] ,&decodedBits);
      } else {
        clr_tns( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info[ch] );
      }
    
      if (coreCodecIdx!=NO_CORE) {
        if ((ch==0) || ((ch==1) && (coreChannels==2 ) ) ){
          if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
            int g;
            for (g=0;g<8;g++){
              BsGetBit( layer_stream,&ultmp, 1 );  /* short FSSwitch  1st aacLayer  */
              decodedBits += 1;
              diffControl[ch][g]= ultmp;
              if (ch==0){
                diffControlBands +=1;
              }
            }
          } else {
            decodeDiffCtrl(layer_stream,diffControl[ch],&decodedBits,
                           tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups);/*  FSSwitch 1st aacLayer */
            if (ch==0){              
              if ((coreCodecIdx==CC_CELP_MPEG4) && (firstAacLay==aacLayer)) {
                diffControlBands =tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups*4;
              } else {
                diffControlBands = ((max_sfb[aacLayer-1]+3)/4)*4; /* for tvq */                
              }
            }
          }
        }
        if ((coreChannels == 1) && (layNumChan[firstAacLay]==2)){
          int g,sfb;
          /* diffcontrol lr */
          if ((coreCodecIdx==CC_CELP_MPEG4) && (firstAacLay==aacLayer) ) {
            diffControlBands = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups*4;
          } else {
            diffControlBands = ((max_sfb[aacLayer-1]+3)/4)*4; /* for tvq */                
          }
          if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
            for (g=0;g<8;g++){
              BsGetBit( layer_stream,&ultmp, 1 );  /* short FSS */
              decodedBits += 1;
              diffControlLR[ch][g]= ultmp;             
            }
            
          } else {
            for (sfb=0;sfb< diffControlBands  ;sfb++){
              if (msMask[0][sfb]==0) {
                BsGetBit( layer_stream,&ultmp, 1 );  /* long FSS */
                decodedBits += 1;
                diffControlLR[ch][sfb]= ultmp;
              } else {
                /* use the one already decoded above for mid chan 
                   if ms is on  */
                diffControlLR[0][sfb]= diffControl[0][sfb];
                /* no diff for side Chan if ms is on   */
                diffControlLR[1][sfb]= 1;
              }
            }
          }
        }
      } else {
        if(tfData->pred_type == NOK_LTP)
          {
            setHuffdec2BitBuffer(layer_stream);
            
            /* LTP data. */
            decodedBits += 
              nok_lt_decode(tfData->windowSequence[MONO_CHAN], &max_sfb[firstAacLay], 
                            tfData->nok_lt_status[ch]->sbk_prediction_used, 
                            tfData->nok_lt_status[ch]->sfb_prediction_used, 
                            &tfData->nok_lt_status[ch]->weight, tfData->nok_lt_status[ch]->delay,
                            hFault->hResilience, hFault->hVm, hFault->hEpInfo, qc_select, NO_CORE, -1);
            
            ResetReadBitCnt (hFault->hVm);
          }
	else
          {
            BsGetBit(layer_stream, &ultmp, 1);  /* no prediction data  */
            decodedBits += 1;
          }
      }
    }
    
    
    /* decode 1st AAC Layer */
    used_bits[firstAacLay][0] = decodedBits;
      if(qc_select == AAC_SCALABLE) {
      tmp = AacDecodeFrame( layer_stream,
                            gc_DummyStream,
/*                             gc_WRstream, */
                            tfData->spectral_line_vector[aacLayer], 
                            &(tfData->windowSequence[MONO_CHAN]), 
                            tfData->windowShape,
                            SCALABLE,
                            &max_sfb[aacLayer],
                            layNumChan[aacLayer],
                            commonWindow[aacLayer],
                            tfData->sfbInfo, 
                            sfbCbMap[aacLayer],
                            hFault->hResilience,
                            hFault->hVm,
                            hFault->hHcrSpecData,
                            hFault->hHcrInfo,
                            hFault->hEpInfo,
                            hFault->hConcealment,
                            qc_select,
			    tfData->nok_lt_status);

#ifdef DEBUGPLOT 
      plotSend("l", "mdctL1",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[1][0] , NULL);
      if (layNumChan[aacLayer]==2)
        plotSend("r", "mdctL1",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[1][1] , NULL);
#endif    
    }
    used_bits[aacLayer][1] = tmp;
    decodedBits +=tmp;
    totalLength = BsBufferNumBit(fd->layer[layer].bitBuf);
    BsGetSkip(layer_stream,(totalLength-decodedBits)); /* this would not work, if more then one AU is in the buffer */
    fd->layer[layer].NoAUInBuffer--;
    BsCloseRemove ( layer_stream ,1);
    /* end of 1st aac layer */
    layer++;
  } 
  /* read and decode aac_scaleable_extension_stream(s) */
    for ( aacLayer=firstAacLay+1;aacLayer<=lastAacLay;aacLayer++){

      if (fd->layer[layer].NoAUInBuffer<=0) continue ;
      layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf); 

      decodedBits =0;
      if (tfData->windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE){
        BsGetBit( layer_stream, &ultmp,4);          /* max_sfb_med */
        decodedBits += 4;
        max_sfb[aacLayer]=ultmp;
      } else {
        BsGetBit( layer_stream, &ultmp,6);          /*  max_sfb_med */
        decodedBits += 6;
        max_sfb[aacLayer]=ultmp;
      }
    
      if( (layNumChan[aacLayer]==2)){  /* rest of ms mask which was not transmitted in previous layer */
        int g,sfb,win,gwin;
        int startSfb;
        BsGetBit( layer_stream, &ultmp,2);     /* ms_mask_present ? */
        decodedBits += 2;
        msMaskPres = ultmp;          
        if (msMaskPres==1){
          win=0;
          for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
            if (layNumChan[aacLayer-1]!=layNumChan[aacLayer]){
              startSfb=0;
            } else {
              startSfb=max_sfb[aacLayer-1];
            }
            for (sfb=startSfb;sfb<max_sfb[aacLayer];sfb++){
              BsGetBit( layer_stream, &ultmp,1);    /* ms_mask */
              decodedBits += 1;
              msMask[win][sfb]=ultmp;
            }
            for (gwin=1;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
              for (sfb=startSfb;sfb<max_sfb[aacLayer];sfb++){                
                msMask[win+gwin][sfb]=msMask[win][sfb];/* apply same ms mask to all 
                                                          subwindows of same group */
              }
            }
            win += tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g] ;
          }          
        }else if (msMaskPres==2){
          win=0;
          for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
            if (layNumChan[aacLayer-1]!=layNumChan[aacLayer]){
              startSfb=0;
            } else {
              startSfb=max_sfb[aacLayer-1];
            }
            for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
              for (gwin=0;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
                for (sfb=startSfb;sfb<max_sfb[aacLayer];sfb++){
                  msMask[win][sfb]=1;
                }
                win++;
              }
            }
          }  
        }
      }
      diffControlBandsExt =0;            
      diffControlBandsLR =0;            
      if ( (layNumChan[aacLayer-1]!=layNumChan[aacLayer]) ) {
        int sfb,g;
        if ( (layNumChan[aacLayer-1] == 1) && (layNumChan[aacLayer] == 2)  ){
          for (ch=0;ch<layNumChan[aacLayer];ch++){
            BsGetBit( layer_stream, &ultmp,1);           /* tns for left right*/
            tns_ext_present[ch] = ultmp;
            decodedBits += 1;
            if( ultmp ) {
              get_tns_vm( layer_stream, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info_ext[ch] ,&decodedBits);
            } else {
              clr_tns( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info_ext[ch] );
            }
          }
          for (ch=0;ch<layNumChan[aacLayer];ch++){            
            diffControlBandsExt = max_sfb[aacLayer-1];
            diffControlBandsLR = max_sfb[aacLayer-1];            
            if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
              for (g=0;g<8;g++){
                BsGetBit( layer_stream,&ultmp, 1 );  /* short FSS */
                decodedBits += 1;
                diffControlLR[ch][g]= ultmp;                
              }
            } else {
              for (sfb=0;sfb< max_sfb[aacLayer-1];sfb++){
                if (msMask[0][sfb]==0) {
                  BsGetBit( layer_stream,&ultmp, 1 );  /* long FSS */
                  decodedBits += 1;
                  diffControlLR[ch][sfb]= ultmp;
                } else {
                  diffControlLR[ch][sfb]= 0;/* allways on if ms on  channel */
                }
              }
            }
          }        
        }
      }
      used_bits[aacLayer][0] = decodedBits;
    
      ResetReadBitCnt ( hFault->hVm );  
      /*
        {
        double zero=0.0;
        vcopy( &zero, spectral_line_vector[aacLayer][0], 0, 1,1024 );
        vcopy( &zero, spectral_line_vector[aacLayer][1], 0, 1,1024 );
        }*/
      
      tmp = AacDecodeFrame( layer_stream,
                            gc_DummyStream, /* gc_WRstream, */
                            tfData->spectral_line_vector[aacLayer], 
                            &(tfData->windowSequence[MONO_CHAN]), 
                            tfData->windowShape,
                            SCALABLE,
                            &max_sfb[aacLayer],
                            layNumChan[aacLayer],
                            commonWindow[aacLayer],
                            tfData->sfbInfo, 
                            sfbCbMap[aacLayer],
                            hFault->hResilience,
                            hFault->hVm,
                            hFault->hHcrSpecData,
                            hFault->hHcrInfo,
                            hFault->hEpInfo,
                            hFault->hConcealment,
                            qc_select,
			    tfData->nok_lt_status);

      used_bits[aacLayer][1] = tmp;
      decodedBits +=tmp;
      totalLength =   BsBufferNumBit(fd->layer[layer].bitBuf);
      fd->layer[layer].NoAUInBuffer--;
      BsGetSkip(layer_stream,(totalLength-decodedBits));
      BsCloseRemove ( layer_stream,1 );
      layer++;
#ifdef DEBUGPLOT 
      plotSend("l", "mdctL2",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[aacLayer][0] , NULL);
      if (layNumChan[aacLayer]==2)
        plotSend("r", "mdctL2",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[aacLayer][1] , NULL);
#endif    

    }


#if 0
if(tns_info[0] != NULL){
  if ( (coreCodecIdx!=NO_CORE) && (output_select > 0) && (tfData->tnsBevCore==0)  ) {
    /* tns encoder for mono/core  layer */
    int k, j;
    Info *info =tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
    info->islong = (tfData->windowSequence[MONO_CHAN] != EIGHT_SHORT_SEQUENCE);
    ch = 0;
    for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
      float tmp_spec[1024];
      for( k=0; k < info->bins_per_sbk[i]; k++ ) {
        tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
      }
      tns_encode_subblock( tmp_spec,
                           max_sfb[1],   /* max_sfb[wn] Attention: wn and not ch !! */
                           info->sbk_sfb_top[i],
                           info->islong,
                           &(tns_info[ch].info[i]),
                           qc_select);      
        
      for( k=0; k < info->bins_per_sbk[i]; k++ ) {
        tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
      }
        
      j += info->bins_per_sbk[i];
    }
  }
 } 
#endif
  
  /* compute PNS and restore spectrum */
  {
    int sfb, maxSfb, lineIdx=0;
    int prevSfbCb,sfbCb, sfbWidth;
    double energy, zero = 0.0;
 
    if (tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->islong) {
 
      for (aacLayer=firstAacLay+1; aacLayer <= calcMonoLay; aacLayer++) {       
        /* for the first layer do nothing */
        maxSfb = (max_sfb[aacLayer-1]<max_sfb[aacLayer])?max_sfb[aacLayer-1]:max_sfb[aacLayer];
        
        for (ch=0;ch<layNumChan[aacLayer];ch++) {
        
          for (sfb=0; sfb<maxSfb; sfb++) {
            prevSfbCb = (int)sfbCbMap[aacLayer-1][ch][sfb];/* does not work for mono-stereo combination */
            sfbCb     = (int)sfbCbMap[aacLayer][ch][sfb];
            if (sfb==0) {
              sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bk_sfb_top[sfb];
            }
            else {
              sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bk_sfb_top[sfb]-
                tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bk_sfb_top[sfb-1];
            }
            
            if (prevSfbCb == NOISE_HCB) {
              energy = scalprod(&tfData->spectral_line_vector[aacLayer][ch][lineIdx], 
                                &tfData->spectral_line_vector[aacLayer][ch][lineIdx], 1, 1, sfbWidth);
              /* if sfb of enhancement layer has MDCT lines, set sfb of previous layer to 0 */
              if (energy > 0) {
                vcopy(&zero, &tfData->spectral_line_vector[aacLayer-1][ch][lineIdx], 0 ,1 ,sfbWidth);
              }
              else {
                sfbCbMap[aacLayer][ch][sfb] = sfbCbMap[aacLayer-1][ch][sfb];
                vcopy(&tfData->spectral_line_vector[aacLayer-1][ch][lineIdx],
                      &tfData->spectral_line_vector[aacLayer][ch][lineIdx], 1, 1, sfbWidth);
                vcopy(&zero,&tfData->spectral_line_vector[aacLayer-1][ch][lineIdx], 0, 1, sfbWidth);  
              }
              if (sfbCb == NOISE_HCB) {
                CommonWarning("Two consecutive layers have PNS in same SFB !");
              } 
            }
            lineIdx += sfbWidth; 
          }
        }
      }    
    }
  }
  
  }

  /* combine all mono layers */
  for (aacLayer=firstAacLay+1 ; aacLayer <= calcMonoLay ; aacLayer++) {

    if (!debugDiff){
      vadd(   tfData->spectral_line_vector[firstAacLay][MONO_CHAN],
              tfData->spectral_line_vector[aacLayer][MONO_CHAN],tfData->spectral_line_vector[firstAacLay][MONO_CHAN],  
              1,1,1, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk );
    } else {
      vcopy (  tfData->spectral_line_vector[aacLayer][MONO_CHAN] , 
               tfData->spectral_line_vector[firstAacLay][MONO_CHAN],
               1,1 ,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);
    }

  }

  /* LTP processing. */
  if(coreCodecIdx == NO_CORE && tfData->pred_type == NOK_LTP)
  {
    TNS_frame_info *tns_info_ltp[2];
    

    tns_info_ltp[0] = &tns_info[0];
    tns_info_ltp[1] = &tns_info[1];
    
    ltp_scal_reconstruct(tfData->windowSequence[MONO_CHAN],
			 *tfData->windowShape, *tfData->windowShape,
			 layNumChan[firstAacLay],
			 tfData->spectral_line_vector[firstAacLay],
			 tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk,
			 med_win_in_long, short_win_in_long,
			 max_sfb[firstAacLay], msMask, tfData->nok_lt_status, 
			 tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
			 tns_info_ltp, qc_select);
  }

  /* combine all stereo layers */
    
  if (firstStLay!=-1 ) {
    for (aacLayer=firstStLay+1 ; aacLayer <= output_select ; aacLayer++) {
      for (ch=0;ch<layNumChan[aacLayer];ch++){
        vadd(   tfData->spectral_line_vector[firstStLay][ch],tfData->spectral_line_vector[aacLayer][ch],
                tfData->spectral_line_vector[firstStLay][ch],  
                1,1,1, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk );
      }
    }
  }
  
  /* fss for aac mono layer */
  low_spec=tfData->spectral_line_vector[0][0];
  if ( (output_select > 0) && (lastMonoLay>0) ){
    double tmp[1024];
    shortFssWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth;
    if (lowRateChannelPresent) {
#if 1
      FSSwitch( low_spec, tfData->spectral_line_vector[firstAacLay][MONO_CHAN],
                tmp,
                diffControl[MONO_CHAN], tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]] ,
                diffControlBands , shortFssWidth,1 );    
      vcopy (  tmp, tfData->spectral_line_vector[firstAacLay][MONO_CHAN],1,1 ,
               tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);
#endif
    }
    diffControlBands=diffControlBandsExt;
    low_spec=tfData->spectral_line_vector[firstAacLay][MONO_CHAN];
  } 
if(lastAacLay>=0){
/*
  fprintf(stderr, "OOOOOO first%5d %5d  \n",
  firstAacLay,  layNumChan[firstAacLay]);
  
  fprintf(stderr, "OOOOOO last%5d %5d \n",
  lastAacLay,  layNumChan[lastAacLay]);
*/
  if ( ( layNumChan[firstAacLay]==1 )  && ( layNumChan[lastAacLay]==2 ) ) {
    vcopy (  tfData->spectral_line_vector[firstAacLay][MONO_CHAN] , 
             tfData->spectral_line_vector[firstAacLay][MONO_CHAN+1],
             1,1 ,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);        
  }
}
  /* fss for aac stereo layer */
  if ( (output_select >= firstStLay ) && (firstStLay!=-1 ) && (layNumChan[firstAacLay]==1)){
    /* aac mono + aac stereo */
    double tmp_spec[1024];
    /* inverse ms matrix */
    
    multCoreSpecMsFac(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],msMask,low_spec,tmp_spec);
    if (layNumChan[firstAacLay]==2 ){
      shortFssWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth;
    }else{
      if  (max_sfb[aacLayer-1]!=0) {
        shortFssWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[0][ max_sfb[firstStLay-1]-1] ; 
      } else {
        shortFssWidth=0;
      }
    }
    for (ch=0;ch<layNumChan[firstStLay];ch++){                    
#if 1
      FSSwitchSt( tmp_spec, 
                  tfData->spectral_line_vector[firstStLay][ch],
                  tfData->spectral_line_vector[firstAacLay][ch],
                  diffControlLR[ch], 
                  diffControlBandsLR,
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
                  msMask,
                  ch );    
#endif
    }
#ifdef DEBUGPLOT 
      plotSend("l", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[firstStLay][0] , NULL);
      if (layNumChan[aacLayer]==2)
        plotSend("r", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
                 tfData->spectral_line_vector[firstStLay][1] , NULL);

#endif    
    doInverseMsMatrix( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],tfData->spectral_line_vector[firstAacLay][0], 
                       tfData->spectral_line_vector[firstAacLay][1],  msMask );
  } else if ((output_select >= firstStLay ) && (firstStLay!=-1 ) ){
    /* core + aac stereo , no aac mono layer */
    double tmp_spec[1024];
#ifdef DEBUGPLOT 
    plotSend("l", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
             tfData->spectral_line_vector[firstStLay][0] , NULL);
    if (layNumChan[aacLayer]==2)
      plotSend("r", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[firstStLay][1] , NULL);
    
#endif    
    
    if (lowRateChannelPresent){
      multCoreSpecMsFac(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],msMask,low_spec,tmp_spec);
      for (ch=0;ch<layNumChan[firstStLay];ch++){                 
#if 1
        FSSwitchStwCore( tmp_spec, /* core spec */
                         tfData->spectral_line_vector[firstStLay][ch],
                         tfData->spectral_line_vector[firstAacLay][ch],
                         diffControl[ch], 
                         diffControlLR[ch], 
                         diffControlBands,
                         tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]] ,
                         msMask,
                         ch );    
#endif
      }
    }
    doInverseMsMatrix( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],
                       tfData->spectral_line_vector[firstAacLay][0], 
                       tfData->spectral_line_vector[firstAacLay][1],  msMask );

  }


  if (output_select) {
    if (layNumChan[output_select]==2){
      for (ch=0;ch<layNumChan[lastAacLay];ch++){
        vcopy( tfData->spectral_line_vector[firstAacLay][ch] ,   tfData->spectral_line_vector[0][ch] , 1, 1,  
               tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk ); 
      }        
    } else {
      for (ch=0;ch<numChannels;ch++){
        vcopy( tfData->spectral_line_vector[firstAacLay][MONO_CHAN] ,   tfData->spectral_line_vector[0][ch] , 1, 1,  
               tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk ); 
      }        
    }
  } else {
    if ((layNumChan[lastAacLay]==2)&&(layNumChan[output_select]==1)){
      vcopy( tfData->spectral_line_vector[0][0] ,   tfData->spectral_line_vector[0][1] , 1, 1,  
             tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk ); 
    }
  }

  /* Bandwidth control */
  if ((lopLong != 0) && (max_sfb[lastAacLay] != 0) && (output_select == 0)) {
    int maxline;
      
    if (normBw == 0.0F) {
      maxline = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[0][max_sfb[lastAacLay]-1];
    }
    else {
      maxline = (int)((float)(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk 
                              /tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk)*normBw + 0.5);
    } 
    for (ch=0; ch<layNumChan[lastAacLay]; ch++) {
      dec_lowpass(tfData->spectral_line_vector[0][ch], lopLong, lopShort, maxline,
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk, tfData->windowSequence[MONO_CHAN],
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk);
    }
  }

  /* tns decoder for tns of main header */
  if  ( ! ( 
           (output_select == 0) && (lowRateChannelPresent) && (tfData->tnsBevCore==0) 
           ) )  {
    TNS_frame_info tns_stereo_info[2];
    if (layNumChan[firstAacLay] ==1 ){
      tns_stereo_info[0] = tns_info[0];
      tns_stereo_info[1] = tns_info[0];      
    } else {
      tns_stereo_info[0] = tns_info[0];
      tns_stereo_info[1] = tns_info[1];      
    }
    for (ch=0;ch<layNumChan[lastAacLay];ch++){      
      int k, j;
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      
      for (i=j=0; i<tns_stereo_info[ch].n_subblocks; i++) {
        float tmp_spec[1024];
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
        }
        tns_decode_subblock( tmp_spec,
                             max_sfb[output_select],  
                             info->sbk_sfb_top[i],
                             info->islong,
                             &(tns_stereo_info[ch].info[i]),
                             qc_select);      
        
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
        }
        
        j += info->bins_per_sbk[i];
      }
    }
  }

  /* tns decoder for tns of 1st extension header */
  if ( ( (firstStLay>=1) && (output_select >= firstStLay) && (!lowRateChannelPresent) ) 
       || ( (firstStLay>=2) && (output_select >= firstStLay) && (lowRateChannelPresent))
       ) {
    TNS_frame_info tns_stereo_info[2];
    
    tns_stereo_info[0] = tns_info_ext[0];
    tns_stereo_info[1] = tns_info_ext[1];      
    
    for (ch=0;ch<layNumChan[lastAacLay];ch++){      
      int k, j;
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      if (tns_ext_present[ch]==1){
        for (i=j=0; i<tns_stereo_info[ch].n_subblocks; i++) {
          float tmp_spec[1024];
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
          }
          tns_decode_subblock( tmp_spec,
                               max_sfb[output_select],  
                               info->sbk_sfb_top[i],
                               info->islong,
                               &(tns_stereo_info[ch].info[i]),
                               qc_select);      
          
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
          }
          
          j += info->bins_per_sbk[i];
        }
      }
    }
  }


#ifdef DEBUGPLOT 
      plotSend("l", "mdctOut",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[0][0] , NULL);
      if (layNumChan[aacLayer]==2)
        plotSend("r", "mdctOut",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
                 tfData->spectral_line_vector[0][1] , NULL);

#endif    

#ifdef DEBUGPLOT
  plotSend("c", "mdctCorex",  MTV_DOUBLE,tfData->block_size_samples, tfData->spectral_line_vector[0][0] , NULL);

  plotSend("l", "mdctSpecL",  MTV_DOUBLE_SQA,tfData->block_size_samples, tfData->spectral_line_vector[0][0] , NULL);
  /*   plotSend("r", "mdctSpecR" , MTV_DOUBLE,1024, spectral_line_vector[firstAacLay][1] , NULL); */
#endif    
 

}
