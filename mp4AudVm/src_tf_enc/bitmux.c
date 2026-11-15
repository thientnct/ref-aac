/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)
and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1998.



 

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Bitmux.c  : Handling of bistream format for AAC Raw and AAC SCALABLE     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include "allHandles.h"
#include "block.h"
#include "interface.h"

#include "nok_ltp_common.h"      /* structs */

#include "bitstream.h"
#include "common_m4a.h"
#include "nok_ltp_enc.h"
#include "tns3.h"

#include "aac_qc.h"
#include "bitmux.h"

int sort_book_numbers(int book_vector[],
                      int output_book_vector[],
		      int nr_of_sfb, int qdebug,
		      WINDOW_SEQUENCE windowSequence,
		      HANDLE_BSBITSTREAM fixed_stream,
		      int write_flag,
                      int num_win_groups
		      )
{

  /*
    This function inputs the vector, 'book_vector[]', which is of length SFB_NUM_MAX,
    and contains the optimal huffman tables of each sfb.  It returns the vector, 'output_book_vector[]', which
    has it's elements formatted for the encoded bit stream.  It's syntax is:
   
    {sect_cb[0], length_segment[0], ... ,sect_cb[num_of_sections], length_segment[num_of_sections]}

    The above syntax is true, unless there is an escape sequence.  An
    escape sequence occurs when a section is longer than 2 ^ (bit_len)
    long in units of scalefactor bands.  Also, the integer returned from
    this function is the number of bits written in the bitstream, 
    'bit_count'.  

    This function supports both long and short blocks.
  */

  int i;
  int repeat_counter = 1;
  int bit_count = 0;
  int previous;
  int max, bit_len;
  int sfb_per_group = nr_of_sfb/num_win_groups;

  if (windowSequence == EIGHT_SHORT_SEQUENCE){
    max = 7;
    bit_len = 3;
  }
  else {  /* the windowSequence is a long,start, or stop window */
    max = 31;
    bit_len = 5;
  }

  previous = book_vector[0];
  if (write_flag) {   
    BsPutBit(fixed_stream,book_vector[0],4);  
    if (qdebug > 1 )
      fprintf(stderr,"\nbookNrs:\n%d = ",book_vector[0]);
  }
  bit_count += 4;		/* bug fix TK */

  for (i=1;i<nr_of_sfb;i++) {
    if( (book_vector[i] != previous) || (i%sfb_per_group==0)) {
      if (write_flag) {
	BsPutBit(fixed_stream,repeat_counter,bit_len);  
	if (qdebug > 1 )
	  fprintf(stderr,"%d\n%d = ",i,book_vector[i]);
      }
      bit_count += bit_len;

      if (repeat_counter == max){  /* in case you need to terminate an escape sequence */
	if (write_flag) BsPutBit(fixed_stream,0,bit_len);  
	bit_count += bit_len;
      }
	
      if (write_flag) BsPutBit(fixed_stream,book_vector[i],4);
      bit_count += 4;		/* bug fix TK */
      previous = book_vector[i];
      repeat_counter=1;

    }
    /* if the length of the section is longer than the amount of bits available in */
    /* the bitsream, "max", then start up an escape sequence */
    else if (((book_vector[i] == previous) && (repeat_counter == max)) ) { 
      if (write_flag) {
	BsPutBit(fixed_stream,repeat_counter,bit_len);  
	if (qdebug > 1 )
	  fprintf(stderr,"(esc)");	
      }
      bit_count += bit_len;
      repeat_counter = 1;
    }
    else {
      repeat_counter++;
    }
  }

  if (write_flag) {
    BsPutBit(fixed_stream,repeat_counter,bit_len);  
    if (qdebug > 1 )
      fprintf(stderr,"%d\n",i);  
  }
  bit_count += bit_len;
  if (repeat_counter == max) {  /* special case if the last section length is an */
    /* escape sequence */
    if (write_flag) BsPutBit(fixed_stream,0,bit_len);  
    bit_count += bit_len;
  }
  return(bit_count);
}

int find_grouping_bits(int window_group_length[],
		     int num_window_groups)
{

  /* This function inputs the grouping information and outputs the seven bit 
     'grouping_bits' field that the AAC decoder expects.  */


  int grouping_bits = 0;
  int tmp[8];
  int i,j;
  int index=0;

  for(i=0; i<num_window_groups; i++){
    for (j=0; j<window_group_length[i];j++){
      tmp[index++] = i;
      /*      printf("tmp[%d] = %d\n",index-1,tmp[index-1]);*/
    }
  }

  for(i=1; i<8; i++){
    grouping_bits = grouping_bits << 1;
    if(tmp[i] == tmp[i-1]) {
      grouping_bits++;
    }
  }
  
  /*  printf("grouping_bits = %d  [i=%d]\n",grouping_bits,i);*/
  return(grouping_bits);
}

int write_scalefactor_bitstream(int nr_of_sfb,
                              int scale_factors[],
                              int book_vector[],
                              HANDLE_BSBITSTREAM fixed_stream,
                              int write_flag,
                              int window_group_length[],
                              int num_window_groups,
                              int global_gain,
                              WINDOW_SEQUENCE windowSequence,
                              int noise_nrg[],
                              int qdebug, int huff[13][1090][4])
{

  /* this function takes care of counting the number of bits necessary */
  /* to encode the scalefactors.  In addition, if the write_flag == 1, */
  /* then the scalefactors are written out the fixed_stream output bit */
  /* stream.  it returns k, the number of bits written to the bitstream*/

  int i,j,k=0;
  int diff,length,codeword;
  int previous_scale_factor;
  int index = 0;
  int sf_out = 0;
  int sf_not_out = 0;
  int nr_of_sfb_per_group=0;
  int pns_pcm_flag = 1;
  int previous_noise_nrg = global_gain;

  if (windowSequence == EIGHT_SHORT_SEQUENCE) { /* short windows */
    nr_of_sfb_per_group = nr_of_sfb/num_window_groups;
  }
  else {
    nr_of_sfb_per_group = nr_of_sfb;
    num_window_groups = 1;
    window_group_length[0] = 1;
  }

  previous_scale_factor = scale_factors[0];
  previous_scale_factor = global_gain;
  if ( (qdebug > 2) && write_flag ) {
    fprintf(stderr,"\nscalefacs :");	
  }
    
  for(j=0; j<num_window_groups; j++){
    for(i=0;i<nr_of_sfb_per_group;i++) {  
      /* test to see if any codebooks in a group are zero */
      if ( (qdebug > 1) && write_flag ) {
	fprintf(stderr," %d:%d",index,scale_factors[index]);	
      }
      if (book_vector[index] == PNS_HCB) {
        /* send PNS pseudo scalefactors  */
        diff = noise_nrg[index] - previous_noise_nrg;
        if (pns_pcm_flag) {
          pns_pcm_flag = 0;
          if (diff+PNS_PCM_OFFSET<0  ||  diff+PNS_PCM_OFFSET>=(1<<PNS_PCM_BITS))
            fprintf(stderr,"%3d (PNS PCM Overflow0) *****\n", diff);
          length = PNS_PCM_BITS;
          codeword = diff + PNS_PCM_OFFSET;
        } else {
          if (diff<-60  ||  diff>60)
            printf("%3d (PNS Overflow0) *****\n", diff);
          length = huff[12][diff+60][2];    
	  codeword = huff[12][diff+60][3];
        }
	k += length;
        previous_noise_nrg = noise_nrg[index];
	if (write_flag == 1 ) {   
	  /*	  printf("  scale_factor[%d] = %d\n",index,scale_factors[index]);*/
	  BsPutBit(fixed_stream,codeword,length); 
	  sf_out++;
	}
      } else if (book_vector[index]) {
        /* only send scalefactors if using non-zero codebooks ,*/
	diff = scale_factors[index] - previous_scale_factor;
	length = huff[12][diff+60][2];    
	k+=length;
	previous_scale_factor = scale_factors[index];
	if (write_flag == 1 ) {   
	  /*	  printf("  scale_factor[%d] = %d\n",index,scale_factors[index]);*/
	  codeword = huff[12][diff+60][3];
	  BsPutBit(fixed_stream,codeword,length); 
	  sf_out++;
	}
      } else {
	/*if (write_flag) 
          printf("  zero_flag = 1, [j=%d index=%d i=%d]\n",j,index,i);*/
	sf_not_out++;
      }
      index++;
    }
  }
  /*  if (write_flag) printf("sf_out = %d  sf_not_out = %d\n",sf_out,sf_not_out);*/
  return(k);
}

int write_ics_info (int nr_of_sfb, WINDOW_SEQUENCE windowSequence, HANDLE_BSBITSTREAM fixed_stream, 
                    WINDOW_SHAPE window_shape, int num_window_groups, int window_group_length[],
		    int reducedInfo, QC_MOD_SELECT qc_select, PRED_TYPE predictor_type, 
                    NOK_LT_PRED_STATUS *nok_lt_statusLeft,
		    NOK_LT_PRED_STATUS *nok_lt_statusRight, int stereo_flag)
{

  int bit_count = 0;
  int grouping_bits;
  int max_sfb;
  int tmpVar = 0;

  if (!reducedInfo) {
    if((qc_select == AAC_QC) 
#ifdef VERSION2
       || (qc_select == AAC_LD)
#endif
       ) {
      /* ics_reserved_bit only for plain AAC */
      BsPutBit(fixed_stream, 0, 1); 
      bit_count +=1;
    }

    /* write out WINDOW_SEQUENCE */
    switch (windowSequence) 
      {
      case EIGHT_SHORT_SEQUENCE:
        BsPutBit(fixed_stream,2,2);  /* short window */
        bit_count += 2;
        break;
      case ONLY_LONG_SEQUENCE:
        BsPutBit(fixed_stream,0,2);  
        bit_count += 2;
        break;
      case   LONG_START_SEQUENCE:
        BsPutBit(fixed_stream,1,2);  
        bit_count += 2;
        break;
      case  LONG_STOP_SEQUENCE:
        BsPutBit(fixed_stream,3,2);  
        bit_count += 2;
        break;
      default:
        CommonExit(-1,"\n unknown blocktype : %d",windowSequence);
    }

    /* write out WINDOW SHAPE */
    switch( window_shape ) {
    case WS_FHG:
      tmpVar = 0;
      break;
    case WS_DOLBY:
#ifdef VERSION2
    case WS_ZPW:
#endif
      tmpVar = 1;
      break;
    default:
      CommonExit(-1,"\nunknow windowshape : %d", window_shape );
    }

    BsPutBit(fixed_stream, tmpVar, 1);
    bit_count += 1;

  } /* if (!reducedInfo) */

  /* write out SCALE_FACTOR_GROUPING and MAX_SFB */
  max_sfb =   nr_of_sfb/num_window_groups;
  if (max_sfb*num_window_groups != nr_of_sfb)
    CommonExit(-1,"\nwrong num of nr_of_sfb");

  if (windowSequence == EIGHT_SHORT_SEQUENCE){
    BsPutBit(fixed_stream, max_sfb, 4); /* let max_sfb_l = 14, the max number of sfb's */
    bit_count += 4;

    grouping_bits = find_grouping_bits(window_group_length, num_window_groups);
    BsPutBit(fixed_stream, grouping_bits, 7);  /* the grouping bits */
    bit_count += 7;

  } else {    /* block type is either start, stop, or long */
    BsPutBit(fixed_stream, max_sfb, 6); /* let max_sfb_l = 49, the max number of sfb's */
    bit_count += 6;

    if((qc_select == AAC_QC)
#ifdef VERSION2
       || (qc_select == AAC_LD)
#endif
       ){
      switch(predictor_type) {    
      case MONOPRED:
        CommonExit(-1,"\nMonopred not implemented");
        break;
        
      case NOK_LTP:
	{
	  int ltp_present;
	  
	  ltp_present = 0;
	  if(nok_lt_statusLeft->global_pred_flag)
	    ltp_present = 1;
	  if(stereo_flag)
	    if(nok_lt_statusRight->global_pred_flag)
	      ltp_present = 1;

	  BsPutBit(fixed_stream, ltp_present, 1);
	  bit_count += 1;

	  if(ltp_present)
	  {
	    /* predictor data for left channel. */
	    bit_count += 
	      nok_ltp_encode(fixed_stream, windowSequence, nr_of_sfb, 
			     nok_lt_statusLeft, qc_select,
			     NO_CORE, -1);

	    if(stereo_flag)
	      /* predictor data for right channel. */
	      bit_count += 
		nok_ltp_encode(fixed_stream, windowSequence, nr_of_sfb, 
			       nok_lt_statusRight, qc_select,
			       NO_CORE, -1);
	  }
	}
        break;
        
        
      case PRED_NONE:
        BsPutBit(fixed_stream,0,1);  /* no prediction_data_present */
        bit_count += 1; 
        break;
        
      default:
        CommonExit(-1,"\nUnknown preditcor type");
      }
    }
  }  
  return(bit_count);
}



/*****************************************************************************/
/* write_tns_data(...), write TNS data.                            */
/*****************************************************************************/
int write_tns_data(TNS_INFO* tnsInfoPtr,       /* TnsInfo structure */
                   WINDOW_SEQUENCE windowSequence,     /* block type */
                   HANDLE_BSBITSTREAM  fixed_stream,  /* Pointer to bitstream */
                   int writeFlag)              /* 1 means write, 0 means count only */ 
{
  int bit_count = 0;
  int numWindows = 1;
  int len_tns_nfilt;
  int len_tns_length;
  int len_tns_order;
  int filtNumber;
  int resInBits;
  unsigned long unsignedIndex;
  int w;

  if (writeFlag) {
    BsPutBit(fixed_stream,tnsInfoPtr->tnsDataPresent,LEN_TNS_PRES);
  }
  bit_count += LEN_TNS_PRES;

  /* If TNS is not present, bail */
  if (!tnsInfoPtr->tnsDataPresent) {
    return bit_count;
  }

  /* Set window-dependent TNS parameters */
  if (windowSequence == EIGHT_SHORT_SEQUENCE) {
    numWindows = MAX_SHORT_IN_LONG_BLOCK;
    len_tns_nfilt = LEN_TNS_NFILTS;
    len_tns_length = LEN_TNS_LENGTHS;
    len_tns_order = LEN_TNS_ORDERS;
  } else {
    numWindows = 1;
    len_tns_nfilt = LEN_TNS_NFILTL;
    len_tns_length = LEN_TNS_LENGTHL;
    len_tns_order = LEN_TNS_ORDERL;
  }

/*  fprintf(stderr,"numWindows %5d len_tns_nfilt %5d\n",numWindows,len_tns_nfilt);*/
  
  /* Write TNS data */
  bit_count += numWindows * len_tns_nfilt;
/*  fprintf(stderr,"TNS count (1) %5d\n",bit_count);*/
  for (w=0;w<numWindows;w++) {
    TNS_WINDOW_DATA* windowDataPtr = &tnsInfoPtr->windowData[w];
    int numFilters = windowDataPtr->numFilters;
/*	fprintf(stderr,"numFilters %5d\n",numFilters);*/
    if (writeFlag) {
      BsPutBit(fixed_stream,numFilters,len_tns_nfilt); /* n_filt[] = 0 */
    }
    if (numFilters) {
      bit_count += LEN_TNS_COEFF_RES;
/*	  fprintf(stderr,"TNS count (2) %5d\n",bit_count);*/
      resInBits = windowDataPtr->coefResolution;
      resInBits = windowDataPtr->coefResolution;
      if (writeFlag) {
	BsPutBit(fixed_stream,resInBits-DEF_TNS_RES_OFFSET,LEN_TNS_COEFF_RES);
      }
      bit_count += numFilters * (len_tns_length+len_tns_order);
/*	  fprintf(stderr,"TNS count (3) %5d\n",bit_count);*/
      for (filtNumber=0;filtNumber<numFilters;filtNumber++) {
	TNS_FILTER_DATA* tnsFilterPtr=&windowDataPtr->tnsFilter[filtNumber];
	int order = tnsFilterPtr->order;
	if (writeFlag) {
/*	  fprintf(stderr,"&& %5d\n",tnsFilterPtr->length);*/
	  BsPutBit(fixed_stream,tnsFilterPtr->length,len_tns_length);
	  BsPutBit(fixed_stream,order,len_tns_order);
	}
	if (order) {
	  bit_count += (LEN_TNS_DIRECTION + LEN_TNS_COMPRESS);
/*	  fprintf(stderr,"TNS count (4) %5d\n",bit_count);*/
	  if (writeFlag) {
	    BsPutBit(fixed_stream,tnsFilterPtr->direction,LEN_TNS_DIRECTION);
	    BsPutBit(fixed_stream,tnsFilterPtr->coefCompress,LEN_TNS_COMPRESS);
	  }
	  bit_count += order * resInBits;
/*	  fprintf(stderr,"TNS count (5) %5d\n",bit_count);*/
	  if (writeFlag) {
	    int i;
	    for (i=1;i<=order;i++) {
	      unsignedIndex = (unsigned long) (tnsFilterPtr->index[i])&(~(~0<<resInBits));
	      BsPutBit(fixed_stream,unsignedIndex,resInBits);
	    }
	  }
	}
      }
    }
  }
  return bit_count;
}


static int write_spectral_data(int counter, int len[], int data[], 
                               HANDLE_BSBITSTREAM fixed_stream)
{
  int bit_count = 0;
  int i;

  for(i=0; i<counter; i++) {
    if (len[i] > 0) {  /* only send out non-zero codebook data */
      BsPutBit(fixed_stream,data[i],len[i]);
      bit_count += len[i];
    }
  }
  return (bit_count);
}


/* write single channel element header */
int write_aac_sce(HANDLE_BSBITSTREAM fixed_stream)
{
  int bit_count = 0;

  /* write ID_SCE, single_element_channel() identifier */
  BsPutBit(fixed_stream,0,3);  
  bit_count += 3;

  /* write the element_identifier_tag */
  BsPutBit(fixed_stream,0,4);  
  bit_count += 4;

  return (bit_count);
}

int write_aac_cpe(int nr_of_sfb, WINDOW_SEQUENCE windowSequence, HANDLE_BSBITSTREAM fixed_stream, int num_window_groups,
                  int window_group_length[], WINDOW_SHAPE window_shape, int common_window, int ms_mask, int ms_used[10][10],
		  PRED_TYPE predictor_type, QC_MOD_SELECT qc_select,
                  NOK_LT_PRED_STATUS *nok_lt_status)
{
  int bit_count = 0;
  int i,j;

  /* write ID_CPE, channel_pair_element identifier */
  BsPutBit(fixed_stream,1,3);  
  bit_count += 3;

  /* write the element_instance_tag */
  BsPutBit(fixed_stream,0,4);
  bit_count += 4;

  /* write the common_window flag */
  BsPutBit(fixed_stream, common_window, 1);
  bit_count += 1;

  if (common_window) {
    bit_count += write_ics_info(nr_of_sfb, windowSequence, fixed_stream, window_shape,
                                num_window_groups, window_group_length, 0, qc_select,
                                predictor_type, &nok_lt_status[0], 
				&nok_lt_status[1], common_window);

    /* write the ms_mask_present flags */
    BsPutBit(fixed_stream,ms_mask,2);
    bit_count += 2;
    
    if (ms_mask == 1) {
      for (i=0; i<num_window_groups; i++) {
        for (j=0; j<nr_of_sfb; j++) {
          BsPutBit(fixed_stream, ms_used[i][j],1);
          bit_count += 1;
        }
      }
    }
  }
  return (bit_count);
}


int write_ind_cha_stream(int nr_of_sfb, WINDOW_SEQUENCE windowSequence, int scale_factors[], 
                         HANDLE_BSBITSTREAM fixed_stream, HANDLE_BSBITBUFFER gcBitBufCh, TNS_INFO *tnsInfo,
                         int book_vector[], int data[], int len[], int qdebug, int scale_flag,
                         int huff[13][1090][4], int counter, int num_window_groups, int window_group_length[],
                         int global_gain, WINDOW_SHAPE window_shape, int noise_nrg[],  PRED_TYPE predictor_type,
                         NOK_LT_PRED_STATUS *nok_lt_status, 
                         int common_window, QC_MOD_SELECT qc_select)
{
  int output_book_vector[SFB_NUM_MAX*2];
  int bit_count = 0;
  int reducedInfo=0;
  /* the 8-bit global_gain is the first scalefactor */
  /*  BsPutBit(fixed_stream,scale_factors[0],8);  */
  BsPutBit(fixed_stream,global_gain,8);  
  bit_count += 8;
  if ( (qdebug > 2)  ) {
    fprintf(stderr,"\nglobal_gain %d ",global_gain);
  }

  if ((!common_window) && (!scale_flag))
    /* write out ICS Info block */  
    bit_count += write_ics_info(nr_of_sfb, 
                                windowSequence, 
                                fixed_stream, 
                                window_shape, 
                                num_window_groups, 
                                window_group_length,
                                reducedInfo,
                                qc_select,
                                predictor_type, 
                                nok_lt_status,
				NULL,
				common_window);


  /* write out section_data() information to the bitstream */
  bit_count += sort_book_numbers(book_vector, output_book_vector, nr_of_sfb, qdebug, windowSequence, fixed_stream, 1,num_window_groups);

  /* scale_factor_data() information */
  bit_count += write_scalefactor_bitstream(nr_of_sfb, scale_factors, book_vector, fixed_stream, 1,
                                             window_group_length, num_window_groups, global_gain,
                                             windowSequence, noise_nrg, qdebug, huff);

    
  if (!scale_flag) {
  
    /* no pulse_data_present() */
    BsPutBit(fixed_stream,0,1);
    bit_count += 1;

    /* TNS data */
    if (tnsInfo) {              /* TNS active */
      bit_count += write_tns_data( tnsInfo, windowSequence, fixed_stream, 1 );
    } else {                    /* switch it off */
      BsPutBit(fixed_stream,0,1);
      bit_count += 1;
    }

    /* write gain_control_data()  TK */
    if (gcBitBufCh && ( BsBufferNumBit(gcBitBufCh) > 0)) {		/* gain_control_data() present */
      BsPutBit(fixed_stream,1,1);		/* switches on the gain control */
      bit_count += 1;
      BsPutBuffer(fixed_stream, gcBitBufCh); /* gain_control_data() */
      bit_count +=  BsBufferNumBit(gcBitBufCh) ;
    }
    else {
      BsPutBit(fixed_stream,0,1);		/* switches off the gain control */
      bit_count += 1;
    }
  }

  /* write out the spectral_data() */
  bit_count += write_spectral_data(counter, len, data, fixed_stream);

  return(bit_count);
}


int write_fill_elements(HANDLE_BSBITSTREAM fixed_stream, int numFillBits)
{

  int i;
  int cnt;
  int bitCount = numFillBits;

  /* write fill elements */    
  while(bitCount>7) {
    /* Fill elements ID */
    BsPutBit(fixed_stream, 6, 3);
    bitCount -= 3;
    
    /* check for escape values */
    cnt = (bitCount-4)>>3;
    if(cnt>=15) {

      BsPutBit(fixed_stream, 15, 4);
      bitCount -= 4;

      BsPutBit(fixed_stream, ((cnt-15)>255)?255:cnt-15, 8);  
      bitCount -= 8;
      cnt = ((cnt>270)?270:cnt)-1;
 
    } else {
      BsPutBit(fixed_stream, cnt, 4);
      bitCount -= 4;
    }

    for(i=0; i<cnt; i++) {
      BsPutBit(fixed_stream, 0, 8);
      bitCount -= 8;
    }

  } /* while(bitCount>7) */

  return (numFillBits-bitCount);
}


int write_aac_end_id(HANDLE_BSBITSTREAM fixed_stream)
{
  BsPutBit(fixed_stream,7,3);

  return (3);
}


