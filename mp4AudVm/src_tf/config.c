/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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

#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "buffers.h"
#include "bitstream.h"
#include "common_m4a.h"
#include "port.h"
#ifdef VERSION2
#include "resilience.h"
#endif

/* int default_config;  defined in allVariables.h as 'extern int default_config'  */  /* spz 110300 */

/* 
 * profile dependent parameters
 */
int tns_max_bands ( int            islong
#ifdef VERSION2
                    ,QC_MOD_SELECT qc_select
#endif
                    )
{
  int i;

#ifdef VERSION2
  if (qc_select == AAC_LD) {
    if (only_long_info.bins_per_bk == 480)
      return tns_max_bands_tbl_low_delay[mc_info.sampling_rate_idx][0];
    if (only_long_info.bins_per_bk == 512)
      return tns_max_bands_tbl_low_delay[mc_info.sampling_rate_idx][1];
  }
#endif

  /* construct second index */
  i = islong ? 0 : 1;
  i += (mc_info.profile ==  SSR_Profile) ? 2 : 0;
    
  return tns_max_bands_tbl[mc_info.sampling_rate_idx][i];
}

int
tns_max_order(int islong , QC_MOD_SELECT qc_select, int samplRate)
{
  if (qc_select==AAC_SCALABLE) {
    if (islong) {
      if (samplRate<=32000)
        return 20;
      else 
        return 12;
    } else {
      return 7;
    }
  } else {
    if (islong) {
      switch (mc_info.profile) {
      case Main_Profile:
      case LTP_Profile:
        return 20;
      case LC_Profile:
      case SSR_Profile:
        return 12;
      }
    }
    else {
      /* short window */
      return 7;
    }
  }
  return 0;
}

int
pred_max_bands(void)
{
  return pred_max_bands_tbl[mc_info.sampling_rate_idx];
}

/*
 * adif_header
 */
int get_adif_header( int               block_size_samples,
#ifdef VERSION2
                     HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                     HANDLE_BUFFER     hVm )
{
  int i, n;
  int tag = 0;
  int  select_status;
  ProgConfig tmp_config;
  ADIF_Header *p = &adif_header;
    
  /* adif header */
  for (i=0; i<LEN_ADIF_ID; i++)
    p->adif_id[i] = (char) GetBits ( LEN_BYTE,
#ifdef VERSION2
                              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                              hResilience,
                              hEscInstanceData,
#endif /* VERSION2*/
                              hVm); 
  p->adif_id[i] = 0;	    /* null terminated string */
  /* test for id */
  if (strncmp(p->adif_id, "ADIF", 4) != 0)
    return -1;	    /* bad id */
  /* copyright string */	
  if ((p->copy_id_present = GetBits ( LEN_COPYRT_PRES,
#ifdef VERSION2
                                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                      hResilience,
                                      hEscInstanceData,
#endif /*VERSION2*/
                                      hVm )) == 1) {
    for (i=0; i<LEN_COPYRT_ID; i++)
      p->copy_id[i] = (char) GetBits ( LEN_BYTE,
#ifdef VERSION2
                                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                hResilience,
                                hEscInstanceData, 
#endif /*VERSION2*/
                                hVm ); 
    p->copy_id[i] = 0;  /* null terminated string */
  }
  p->original_copy = GetBits ( LEN_ORIG,
#ifdef VERSION2
                               MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                               hResilience,
                               hEscInstanceData,
#endif /*VERSION2*/
                               hVm);
  p->home = GetBits ( LEN_HOME,
#ifdef VERSION2
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
#endif /*VERSION2*/
                      hVm );
  p->bitstream_type = GetBits ( LEN_BS_TYPE, 
#ifdef VERSION2
                                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                hResilience, 
                                hEscInstanceData,
#endif /*VERSION2*/
                                hVm );
  p->bitrate = GetBits ( LEN_BIT_RATE,
#ifdef VERSION2
                         MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                         hResilience,
                         hEscInstanceData,
#endif /*VERSION2*/
                         hVm );

  /* program config elements */ 
  select_status = -1;   
  n = GetBits ( LEN_NUM_PCE,
#ifdef VERSION2
                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                hResilience,
                hEscInstanceData,
#endif /*VERSION2*/
                hVm ) + 1;
  for (i=0; i<n; i++) {
    tmp_config.buffer_fullness =
      (p->bitstream_type == 0) ? GetBits ( LEN_ADIF_BF,
#ifdef VERSION2
                                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                           hResilience,
                                           hEscInstanceData,
#endif /*VERSION2*/
                                           hVm ) : 0;
    tag = get_prog_config ( &tmp_config,
                            block_size_samples,
#ifdef VERSION2
                            hResilience, 
                            hEscInstanceData,
#endif /*VERSION2*/
                            hVm );
    if (current_program < 0)
      current_program = tag;	    /* default is first prog */
    if (current_program == tag) {
      memcpy(&prog_config, &tmp_config, sizeof(prog_config));
      select_status = 1;
    }
  }
  if (debug['v']) {
    fprintf(stderr, "\nADIF header:\n");
    fprintf(stderr, "%s %d\n", "p->copy_id_present", p->copy_id_present);
    fprintf(stderr, "%s %d\n", "p->original_copy", p->original_copy);
    fprintf(stderr, "%s %d\n", "p->home", p->home);
    fprintf(stderr, "%s %d\n", "p->bitstream_type", p->bitstream_type);
    fprintf(stderr, "%s %ld\n", "p->bitrate", p->bitrate);
    fprintf(stderr, "%s %d\n", "number of PCE", n);
    fprintf(stderr, "Select status %d for PCE tag %d\n", select_status, tag);
    if (select_status) {
      ProgConfig *p = &prog_config;
      fprintf(stderr, "%s %d\n", "p->profile", p->profile);
      fprintf(stderr, "%s %d\n", "p->sampling_rate_idx", p->sampling_rate_idx);
      fprintf(stderr, "%s %d\n", "p->front.num_ele", p->front.num_ele);
      fprintf(stderr, "%s %d\n", "p->side.num_ele", p->side.num_ele);
      fprintf(stderr, "%s %d\n", "p->back.num_ele", p->back.num_ele);
      fprintf(stderr, "%s %d\n", "p->lfe.num_ele", p->lfe.num_ele);
      fprintf(stderr, "%s %d\n", "p->data.num_ele", p->data.num_ele);
      fprintf(stderr, "%s %d\n", "p->coupling.num_ele", p->coupling.num_ele);
      fprintf(stderr, "%s %d\n", "p->mono_mix.present", p->mono_mix.present);
      fprintf(stderr, "%s %d\n", "p->stereo_mix.present", p->stereo_mix.present);
      fprintf(stderr, "%s %d\n", "p->matrix_mix.present", p->matrix_mix.present);
	    
      fprintf(stderr, "Comment: %s\n", prog_config.comments);
    }
  }

  return select_status;
}


/*
 * program configuration element
 */
static void get_ele_list ( EleList*          p, 
                           int               enable_cpe,
#ifdef VERSION2
                           HANDLE_RESILIENCE hResilience, 
                           HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                           HANDLE_BUFFER     hVm )
{  
  int i, j;
  for (i=0, j=p->num_ele; i<j; i++) {
    if (enable_cpe)
      p->ele_is_cpe[i] = GetBits(LEN_ELE_IS_CPE,
#ifdef VERSION2
                                 MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                 hResilience,
                                 hEscInstanceData,
#endif /*VERSION2*/
                                 hVm );
    p->ele_tag[i] = GetBits(LEN_TAG,
#ifdef VERSION2
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
#endif /*VERSION2*/
                        hVm );
  }
}

int get_prog_config ( ProgConfig*       p,
                      int               block_size_samples,
#ifdef VERSION2
                      HANDLE_RESILIENCE hResilience, 
                      HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                      HANDLE_BUFFER     hVm )
{
  int i, j, tag;

  tag = GetBits(LEN_TAG,
#ifdef VERSION2
                ELEMENT_INSTANCE_TAG, 
                hResilience,
                hEscInstanceData,
#endif /*VERSION2*/
                hVm );

  p->profile = GetBits(LEN_PROFILE,
#ifdef VERSION2
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience,
                       hEscInstanceData,
#endif /*VERSION2*/
                       hVm );

  p->sampling_rate_idx = GetBits(LEN_SAMP_IDX,
#ifdef VERSION2
                                 MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                 hResilience,
                                 hEscInstanceData,
#endif /*VERSION2*/
                                 hVm );

  p->front.num_ele = GetBits(LEN_NUM_ELE,
#ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience,
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );

  p->side.num_ele = GetBits(LEN_NUM_ELE,
#ifdef VERSION2
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
#endif /*VERSION2*/
                            hVm );

  p->back.num_ele = GetBits(LEN_NUM_ELE,
#ifdef VERSION2
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
#endif /*VERSION2*/
                            hVm );

  p->lfe.num_ele = GetBits(LEN_NUM_LFE,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience,
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm);

  p->data.num_ele = GetBits(LEN_NUM_DAT,
#ifdef VERSION2
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
#endif /*VERSION2*/
                            hVm );

  p->coupling.num_ele = GetBits(LEN_NUM_CCE,
#ifdef VERSION2
                                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                hResilience,
                                hEscInstanceData,
#endif /*VERSION2*/
                                hVm );

  if ((p->mono_mix.present = GetBits(LEN_MIX_PRES,
#ifdef VERSION2
                                     MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                     hResilience,
                                     hEscInstanceData,
#endif /*VERSION2*/
                                     hVm )) == 1)

    p->mono_mix.ele_tag = GetBits(LEN_TAG,
#ifdef VERSION2
                                  MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                  hResilience,
                                  hEscInstanceData,
#endif /*VERSION2*/
                                  hVm );

  if ((p->stereo_mix.present = GetBits(LEN_MIX_PRES,
#ifdef VERSION2
                                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                       hResilience,
                                       hEscInstanceData,
#endif /*VERSION2*/
                                       hVm )) == 1)

    p->stereo_mix.ele_tag = GetBits(LEN_TAG,
#ifdef VERSION2
                                    MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                    hResilience,
                                    hEscInstanceData,
#endif /*VERSION2*/
                                    hVm );

  if ((p->matrix_mix.present = GetBits(LEN_MIX_PRES,
#ifdef VERSION2
                                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                       hResilience,
                                       hEscInstanceData,
#endif /*VERSION2*/
                                       hVm )) == 1) {
    p->matrix_mix.ele_tag = GetBits(LEN_MMIX_IDX,
#ifdef VERSION2
                                    MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                    hResilience,
                                    hEscInstanceData,
#endif /*VERSION2*/
                                    hVm );
    p->matrix_mix.pseudo_enab = GetBits(LEN_PSUR_ENAB,
#ifdef VERSION2
                                        MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                        hResilience,
                                        hEscInstanceData,
#endif /*VERSION2*/
                                        hVm );
  }
  /* front_channel_elements */
  get_ele_list(&p->front, 
               1, 
#ifdef VERSION2
               hResilience, 
               hEscInstanceData,
#endif /*VERSION2*/
               hVm );
  /* side_channel_elements */
  get_ele_list(&p->side, 
               1,
#ifdef VERSION2
               hResilience,  
               hEscInstanceData,
#endif /*VERSION2*/
               hVm );
  /* back_channel_elements */
  get_ele_list(&p->back, 
               1,
#ifdef VERSION2
               hResilience,  
               hEscInstanceData,
#endif /*VERSION2*/
               hVm );
  /* lfe_channel_elements */
  get_ele_list(&p->lfe, 
               0,
#ifdef VERSION2
               hResilience,  
               hEscInstanceData,
#endif /*VERSION2*/
               hVm );
  /* assoc_data_elements */
  get_ele_list(&p->data, 
               0,
#ifdef VERSION2
               hResilience,  
               hEscInstanceData,
#endif /*VERSION2*/
               hVm );
  /* valid_cc_elements */
  get_ele_list(&p->coupling, 
               1,
#ifdef VERSION2
               hResilience,  
               hEscInstanceData,
#endif /*VERSION2*/
               hVm );
    
  byte_align();
  j = GetBits(LEN_COMMENT_BYTES,
#ifdef VERSION2
              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
              hResilience,
              hEscInstanceData,
#endif /*VERSION2*/
              hVm );
  for (i=0; i<j; i++)
    p->comments[i] = (char) GetBits(LEN_BYTE,
 #ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience,
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
  p->comments[i] = 0;	/* null terminator for string */

  /* activate new program configuration if appropriate */
  if (current_program < 0)
    current_program = tag;	    /* always select new program */
  if (tag == current_program) {
    /* enter configuration into MC_Info structure */
    if (enter_mc_info(&mc_info, 
                      p,
                      block_size_samples
#ifdef VERSION2
                      ,
                      hEscInstanceData, 
                      hResilience 
#endif /*VERSION2*/
                      ) < 0)
      return -1;
    /* inhibit default configuration */
    default_config = 0;
  }
   
  return tag;
}

unsigned long get_channel_config_from_progConfig (ProgConfig* p)
{
  int i = 0, c = 0;
  unsigned long ch = 0;
  i = p->front.num_ele;
  for (c = 0; c < i ; c++)
    {
      if (p->front.ele_is_cpe[c])
        ch +=2;
      else
        ch +=1;
    }

  i = p->side.num_ele;
  for (c = 0; c < i ; c++)
    {
      if (p->side.ele_is_cpe[c])
        ch +=2;
      else
        ch +=1;
    }
  i = p->back.num_ele;
  for (c = 0; c < i ; c++)
    {
      if (p->back.ele_is_cpe[c])
        ch +=2;
      else
        ch +=1;
    }
  i = p->lfe.num_ele;
  for (c = 0; c < i ; c++)
    {
      if (p->lfe.ele_is_cpe[c])
        ch +=2;
      else
        ch +=1;
    }
/*i = p->data.num_ele;
  for (c = 0; c < i ; c++)
    {
      if (p->data.ele_is_cpe[c])
        ch +=2;
      else
        ch +=1;
    }
  i = p->coupling.num_ele;
  for (c = 0; c < i ; c++)
    {
      if (p->coupling.ele_is_cpe[c])
        ch +=2;
      else
        ch +=1;
    }*/
  return ch;
}


static void get_ele_list_from_bs ( EleList*          p, 
                                   int               enable_cpe,
                                   HANDLE_BSBITSTREAM bs )
{  
  int i, j;
  unsigned long ultmp;

  for (i=0, j=p->num_ele; i<j; i++) {
    if (enable_cpe) {
      BsGetBit (bs, &ultmp, LEN_ELE_IS_CPE);
      p->ele_is_cpe[i] = ultmp;
    }
    BsGetBit (bs, &ultmp, LEN_TAG);
    p->ele_tag[i] =  ultmp;
  }
}

int get_prog_config_from_bs ( ProgConfig*              p,
                              int                      block_size_samples,
#ifdef VERSION2
                              HANDLE_RESILIENCE        hResilience, 
                              HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
#endif /*VERSION2*/
                              HANDLE_BSBITSTREAM       bs )
{
  int i, j, tag;
  unsigned long ultmp;

  BsGetBit (bs, &ultmp, LEN_TAG);      tag = ultmp;
  BsGetBit (bs, &ultmp, LEN_PROFILE);  p->profile = ultmp;
  BsGetBit (bs, &ultmp, LEN_SAMP_IDX); p->sampling_rate_idx = ultmp;
  BsGetBit (bs, &ultmp, LEN_NUM_ELE);  p->front.num_ele = ultmp;
  BsGetBit (bs, &ultmp, LEN_NUM_ELE);  p->side.num_ele = ultmp;
  BsGetBit (bs, &ultmp, LEN_NUM_ELE);  p->back.num_ele = ultmp;
  BsGetBit (bs, &ultmp, LEN_NUM_LFE);  p->lfe.num_ele = ultmp;
  BsGetBit (bs, &ultmp, LEN_NUM_DAT);  p->data.num_ele = ultmp;
  BsGetBit (bs, &ultmp, LEN_NUM_CCE);  p->coupling.num_ele = ultmp;
  BsGetBit (bs, &ultmp, LEN_MIX_PRES); p->mono_mix.present = ultmp;
  if (p->mono_mix.present == 1) {
    BsGetBit (bs, &ultmp, LEN_TAG); p->mono_mix.ele_tag = ultmp;
  }
  BsGetBit (bs, &ultmp, LEN_MIX_PRES); p->stereo_mix.present = ultmp;
  if (p->stereo_mix.present == 1) {
    BsGetBit (bs, &ultmp, LEN_TAG); p->stereo_mix.ele_tag = ultmp;
  }
  BsGetBit (bs, &ultmp, LEN_MIX_PRES); p->matrix_mix.present = ultmp;
  if (p->matrix_mix.present == 1) {
    BsGetBit (bs, &ultmp, LEN_MMIX_IDX);  p->matrix_mix.ele_tag = ultmp;
    BsGetBit (bs, &ultmp, LEN_PSUR_ENAB); p->matrix_mix.pseudo_enab = ultmp;
  }
  get_ele_list_from_bs(&p->front, 1, bs);
  get_ele_list_from_bs(&p->side, 1, bs);
  get_ele_list_from_bs(&p->back, 1, bs);
  get_ele_list_from_bs(&p->lfe, 0, bs);
  get_ele_list_from_bs(&p->data, 0, bs);
  get_ele_list_from_bs(&p->coupling, 1, bs);
  byte_align_on_bs(bs);
  BsGetBit (bs, &ultmp, LEN_COMMENT_BYTES); j = ultmp;
  for (i=0; i<j; i++) {
    BsGetBit (bs, &ultmp, LEN_BYTE);
    p->comments[i] = (char) ultmp;
  }
  p->comments[i] = 0;	/* null terminator for string */

  /* activate new program configuration if appropriate */
  if (current_program < 0)
    current_program = tag;	    /* always select new program */
  if (tag == current_program) {
    /* enter configuration into MC_Info structure */
    if (enter_mc_info(&mc_info, 
                      p,
                      block_size_samples
#ifdef VERSION2
                      ,
                      hEscInstanceData, 
                      hResilience 
#endif /*VERSION2*/
                      ) < 0)
      return -1;
    /* inhibit default configuration */
    default_config = 0;
  }
  return tag;
}

/* enter program configuration into MC_Info structure
 *  only configures for channels specified in all.h
 */
int enter_mc_info ( MC_Info*          mip, 
                    ProgConfig*       pcp,
                    int               block_size_samples
#ifdef VERSION2
                    ,
                    HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                    HANDLE_RESILIENCE hResilience
#endif /*VERSION2*/
                    )
{  
  int i, j, cpe, tag, cw;
  EleList *elp;
  MIXdown *mxp;

  /* reset channel counts */
  mip->nch = 0;
  mip->nfch = 0;
  mip->nfsce = 0;
  mip->nsch = 0;
  mip->nbch = 0;
  mip->nlch = 0;
  mip->ncch = 0;

  /* profile and sampling rate
   *	re-configure if new sampling rate
   */    
  mip->profile = pcp->profile;
  if (mip->sampling_rate_idx != pcp->sampling_rate_idx) {
    mip->sampling_rate_idx = pcp->sampling_rate_idx;
    infoinit(&samp_rate_info[mip->sampling_rate_idx],block_size_samples);
  }
    
  cw = 0;			    /* changed later */
    
  /* front elements, center out */
  elp = &pcp->front;
  /* count number of leading SCE's */
  for (i=0, j=elp->num_ele; i<j; i++) {
    if (elp->ele_is_cpe[i])
      break;
    mip->nfsce++;
  }
  for (i=0, j=elp->num_ele; i<j; i++) {
    cpe = elp->ele_is_cpe[i];
    tag = elp->ele_tag[i];
    if (enter_chn(cpe, tag, 'f', cw, mip) < 0)
      return(-1);
  }
    
  /* side elements, left to right then front to back */
  elp = &pcp->side;
  for (i=0, j=elp->num_ele; i<j; i++) {
    cpe = elp->ele_is_cpe[i];
    tag = elp->ele_tag[i];
    if (enter_chn(cpe, tag, 's', cw, mip) < 0)
      return(-1);
  }
	  
  /* back elements, outside to center */
  elp = &pcp->back;
  for (i=0, j=elp->num_ele; i<j; i++) {
    cpe = elp->ele_is_cpe[i];
    tag = elp->ele_tag[i];
    if (enter_chn(cpe, tag, 'b', cw, mip) < 0)
      return(-1);
  }
    
  /* lfe elements */
  elp = &pcp->lfe;
  for (i=0, j=elp->num_ele; i<j; i++) {
    cpe = elp->ele_is_cpe[i];
    tag = elp->ele_tag[i];
    if (enter_chn(cpe, tag, 'l', cw, mip) < 0)
      return(-1);
  }
        
  /* coupling channel elements */
  elp = &pcp->coupling;
  for (i=0, j=elp->num_ele; i<j; i++)
    mip->cch_tag[i] = elp->ele_tag[i];
  mip->ncch = j;
    
  /* mono mixdown elements */
  mxp = &pcp->mono_mix;
  if (mxp->present) {
    fprintf(stderr,"Unanticipated mono mixdown channel\n");
    return(-1);
  }
    
  /* stereo mixdown elements */
  mxp = &pcp->stereo_mix;
  if (mxp->present) {
    fprintf(stderr,"Unanticipated stereo mixdown channel\n");
    return(-1);
  }
    
  /* matrix mixdown elements */
  mxp = &pcp->matrix_mix;
  if (mxp->present) {
    fprintf(stderr,"Unanticipated matrix mixdown channel\n");
    return(-1);
  }
    
  /* save to check future consistency */
  check_mc_info ( &mc_info, 
                  1
#ifdef VERSION2
                  , 
                  hEscInstanceData, 
                  hResilience
#endif /*VERSION2*/
                  );
  return 1;
}

/* translate prog config or default config 
 * into to multi-channel info structure
 *  returns index of channel in MC_Info
 */
int enter_chn ( int      cpe, 
                int      tag, 
                char     position, 
                int      common_window, 
                MC_Info* mip)
{
  int nch, cidx=0;
  Ch_Info *cip;

  nch = (cpe == 1) ? 2 : 1;
    
  switch (position) {
    /* use configuration already in MC_Info, but now common_window is valid */
  case 0:
    cidx = ch_index(mip, cpe, tag);
    if (common_window) {
      mip->ch_info[cidx].widx = cidx;	/* window info is left */
      mip->ch_info[cidx+1].widx = cidx;
    }
    else {
      mip->ch_info[cidx].widx = cidx;	/* each has window info */
      mip->ch_info[cidx+1].widx = cidx+1;
    }
    return cidx;
	
    /* build configuration */
  case 'f':
    if (mip->nfch + nch > FChans) {
      fprintf(stderr,"Unanticipated front channel\n");
      return -1;
    }
    if (mip->nfch == 0) {
      /* consider case of center channel */
      if (FCenter) {
        if (cpe) {
          /* has center speaker but center channel missing */
          cidx = 0 + 1;
          mip->nfch = 1 + nch;
        }
        else {
          if (mip->nfsce & 1) {
            /* has center speaker and this is center channel */
            /* odd number of leading SCE's */
            cidx = 0;
            mip->nfch = nch;
          }
          else {
            /* has center speaker but center channel missing */
            /* even number of leading SCE's */
            /* (Note that in implicit congiguration
             * channel to speaker mapping may be wrong 
             * for first block while count of SCE's prior
             * to first CPE is being make. However first block
             * is not written so it doesn't matter.
             * Second block will be correct.
             */
            cidx = 0 + 1;
            mip->nfch = 1 + nch;
          }
        }
      }
      else {
        if (cpe) {
          /* no center speaker and center channel missing */
          cidx = 0;
          mip->nfch = nch;
        }
        else {
          /* no center speaker so this is left channel */
          cidx = 0;
          mip->nfch = nch;
        }
      }
    }
    else {
      cidx = mip->nfch;
      mip->nfch += nch;
    }
    break;
  case 's':
    if (mip->nsch + nch > SChans)  {
      fprintf(stderr,"Unanticipated side channel\n");
      return -1;
    }
    cidx = FChans + mip->nsch;
    mip->nsch += nch;
    break;
  case 'b':
    if (mip->nbch + nch > BChans) {
      fprintf(stderr,"Unanticipated back channel\n");
      return -1;
    }
    cidx = FChans + SChans + mip->nbch;
    mip->nbch += nch;
    break;
  case 'l':
    if (mip->nlch + nch > LChans) {
      fprintf(stderr,"Unanticipated LFE channel\n");
      return -1;
    }
    cidx = FChans + SChans + BChans + mip->nfch;
    mip->nlch += nch;
    break;
  default:
    CommonExit(1,"enter_chn");
  }
  mip->nch += nch;

  if (cpe == 0) {
    /* SCE */
    cip = &mip->ch_info[cidx];
    cip->present = 1;
    cip->tag = tag;
    cip->cpe = 0;
    cip->common_window = common_window;
    cip->widx = cidx;
    mip->nch = cidx + 1;
  }
  else {
    /* CPE */
    /* left */
    cip = &mip->ch_info[cidx];
    cip->present = 1;
    cip->tag = tag;
    cip->cpe = 1;
    cip->common_window = common_window;
    cip->ch_is_left = 1;
    cip->paired_ch = cidx+1;
    /* right */
    cip = &mip->ch_info[cidx+1];
    cip->present = 1;
    cip->tag = tag;
    cip->cpe = 1;
    cip->common_window = common_window;
    cip->ch_is_left = 0;
    cip->paired_ch = cidx;
    if (common_window) {
      mip->ch_info[cidx].widx = cidx;	/* window info is left */
      mip->ch_info[cidx+1].widx = cidx;
    }
    else {
      mip->ch_info[cidx].widx = cidx;	/* each has window info */
      mip->ch_info[cidx+1].widx = cidx+1;
    }
    mip->nch = cidx + 2;
  }
    
  return cidx;
}

static char
default_position(MC_Info *mip, int id)
{
  static int first_cpe = 0;

  if (mip->nch < FChans) {
    if (id == ID_CPE)
      /* here is the first CPE */
      first_cpe = 1;
    else if ( (bno==0) && !first_cpe )
      /* count number of SCE prior to first CPE in first block */
      mip->nfsce++;
    return('f');
  }
  else if  (mip->nch < FChans+SChans) {
    return('s');
  }
  else if  (mip->nch < FChans+SChans+BChans) {
    return('b');
  }
    
  return 0;
}

/* retrieve appropriate channel index for the program
 * and decoder configuration
 */
int
chn_config(int id, int tag, int common_window, MC_Info *mip)
{
  int cpe, cidx=0;
  char position;
    
  /* channel index to position mapping for 5.1 configuration is
   *  0	center
   *  1	left  front
   *  2	right front
   *  3	left surround
   *  4	right surround
   *  5	lfe
   */

  cpe = (id == ID_CPE) ? 1 : 0;
    
  if (default_config) {
    switch (id) {
    case ID_SCE:
    case ID_CPE:
      if ((position = default_position(mip, id)) == 0) {
        CommonExit(1,"Unanticipated channel\n");
      }
      cidx = enter_chn(cpe, tag, position, common_window, mip);
      break;
    case ID_LFE:
      cidx = enter_chn(cpe, tag, 'l', common_window, mip);
      break;
    }
  }
  else {	
    cidx = enter_chn(cpe, tag, 0, common_window, mip);
  }	

  return cidx;	    /* index of chn in mc_info */
}

/* 
 * check continuity of configuration from one
 * block to next
 */
void
reset_mc_info(MC_Info *mip)
{
  int i;
  Ch_Info *p;

  /* only reset for implicit configuration */
  if (default_config) {
    /* reset channel counts */
    mip->nch = 0;
    mip->nfch = 0;
    mip->nsch = 0;
    mip->nbch = 0;
    mip->nlch = 0;
    mip->ncch = 0;
    if (bno==0)
      /* reset prior to first block scan only! */
      mip->nfsce = 0;
    for (i=0; i<Chans; i++) {
      p = &mip->ch_info[i];
      p->present = 0;
      p->cpe = 0;
      p->ch_is_left = 0;
      p->paired_ch = 0;
      p->is_present = 0;
      p->widx = 0;
      p->ncch = 0;
    }
  }
}


void check_mc_info ( MC_Info*          mip, 
                     int               new_config
#ifdef VERSION2
                     ,
                     HANDLE_ESC_INSTANCE_DATA   hEscInstanceData,
                     HANDLE_RESILIENCE hResilience
#endif /*VERSION2*/
 )
{
  int            i;
  int            nch = 0;
  unsigned char  err = 0;
  Ch_Info*       s;
  Ch_Info*       p;
  static MC_Info save_mc_info;

#ifdef VERSION2
  if ( ! hEscInstanceData || mc_info.mcInfoCorrectFlag || ! BsGetErrorForDataElementFlagEP ( COMMON_WINDOW, hEscInstanceData ) ) {
    mc_info.mcInfoCorrectFlag = 1;
  }
#else
  mc_info.mcInfoCorrectFlag = 1;
#endif /*VERSION2*/  
  if (new_config) {
    memcpy ( &save_mc_info, mip, sizeof(MC_Info) );
  }
  else {
    nch = save_mc_info.nch;
    /* check this block's configuration */
    for ( i = 0; i < nch; i++) {
      s = &save_mc_info.ch_info[i];
      p = &mip->ch_info[i];
      if ( (s->present       != p->present)
           || (s->cpe        != p->cpe)        
           || (s->ch_is_left != p->ch_is_left) 
           || (s->paired_ch  != p->paired_ch) ) {
        err = 1;
      }
    }
  }
  if (err) {
#ifdef VERSION2
    if ( hEscInstanceData && (BsGetEpDebugLevel ( hEscInstanceData ) >= 3) ) {
#endif /*VERSION2*/
      fprintf(stderr, "Channel configuration inconsistency\n");
      for (i=0; i<nch; i++) {
        s = &save_mc_info.ch_info[i];
        p = &mip->ch_info[i];
        fprintf(stderr, "Channel %d\n", i);
        fprintf(stderr, "present    %d %d\n", s->present,  p->present);
        fprintf(stderr, "cpe        %d %d\n", s->cpe,  p->cpe);
        fprintf(stderr, "ch_is_left %d %d\n", s->ch_is_left,  p->ch_is_left);
        fprintf(stderr, "paired_ch  %d %d\n", s->paired_ch,  p->paired_ch);
        fprintf(stderr, "\n");
      }
#ifdef VERSION2
    }
    if ( ! ( hResilience && GetEPFlag ( hResilience ) ) )
#endif /*VERSION2*/
      CommonExit(1,"check_mc_info");
#ifdef VERSION2
    else {
      memcpy ( mip, &save_mc_info, sizeof ( MC_Info ) );
    }
#endif /*VERSION2*/
  }
}


/* given cpe and tag,
 *  returns channel index of SCE or left chn in CPE
 */
int
ch_index(MC_Info *mip, int cpe, int tag)
{
  int ch;
    
  for (ch=0; ch<mip->nch; ch++) {

    if (!(mip->ch_info[ch].present))
      continue;

    if ( (mip->ch_info[ch].cpe == cpe) && 
         (mip->ch_info[ch].tag == tag) )
      return ch;
  }
    
  /* no match, so channel is not in this program
   *	dummy up the ch_info structure so rest of chn will parse
   */
  if (XChans > 0) {
    ch = Chans - XChans;	/* left scratch channel */
    mip->ch_info[ch].cpe = cpe;
    mip->ch_info[ch].ch_is_left = 1;
    mip->ch_info[ch].widx = ch;
    if (cpe) {
      mip->ch_info[ch].paired_ch = ch+1;
      mip->ch_info[ch+1].ch_is_left = 0;
      mip->ch_info[ch+1].paired_ch = ch;
    }
  }
  else {
    ch = -1;		/* error, no scratch space */
  }
    
  return ch;
}
