/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Olaf Kähler (Fraunhofer IIS-A)

and edited by


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

/* AAC file routines */

#include "bitinput.h"

#include "adifdec.h"
#include "config.h"
#include "diagnose.h"
#include "huffinit.h"

#ifdef MPEG4V1
#include "ISOMovies.h"
/* defines missing in older ISOMovies.h */
#ifndef ISOOpenMovieNormal
#define ISOOpenMovieNormal MP4OpenMovieNormal
#define ISOGetMovieDuration MP4GetMovieDuration
#define ISOGetTrackDuration MP4GetTrackDuration
#endif
#else
typedef unsigned int u32;
typedef unsigned char u8;
#endif

/* --- local data --- */

struct tagBitInput {
  FILE *fp;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  u8 *MP4AacBp; 
  u64  mp4_expected_duration;
  u64  mp4_total_read_duration;
  ISOMovie	mp4_moov;
  ISOTrack	mp4_trak;
  ISOTrackReader	mp4_reader;
  ISOHandle	mp4_sampleH;
  u32		mp4_frameLen;
#endif
#endif
  HANDLE_CRC_CALCULATOR crc;

  long	byte_count;
  int	nbits;
  unsigned long	cword;

  /* for adts */
  ADTS_Header adts_head;
  int rdb_in_frame;

  int adif_header_present;
  int adts_header_present;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  int mpeg4_header_present;
#endif
#endif

  int (*getbyte)(HANDLE_BITINPUT input);

  void (*getbits_callback)(int numBits, long bitValue);

  void (*set_log_level_callback)(int id, int add_bits_num, int add_bits);
  void (*eof_callback)();

};


HANDLE_BITINPUT static_input = NULL;


#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
static int getbyte_fromMP4(HANDLE_BITINPUT input);
#endif
#endif
static int getbyte_fromFile(HANDLE_BITINPUT input);




/* ------------------------------------- 
 *  -  constructors and destructors   -
 * ------------------------------------- */


static HANDLE_BITINPUT bitInput_alloc()
{
  HANDLE_BITINPUT ret = (HANDLE_BITINPUT)malloc(sizeof(struct tagBitInput));
  if (ret==NULL) return ret;
  ret->fp = NULL;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ret->MP4AacBp = NULL;
  ret->mp4_frameLen = -1;
  ret->mp4_expected_duration = -1;
  ret->mp4_total_read_duration = 0;
  ret->mp4_moov = NULL;
  ret->mp4_trak = NULL;
  ret->mp4_reader = NULL;
  ret->mp4_sampleH = NULL;
#endif
#endif

  ret->rdb_in_frame = -1;

  ret->byte_count = 0;
  ret->nbits = 0;
  ret->cword = 0;

  ret->adif_header_present = 0;
  ret->adts_header_present = 0;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ret->mpeg4_header_present = 0;
#endif
#endif

  ret->crc = CRC_calculator_create();

  ret->getbyte = &getbyte_fromFile;
  ret->getbits_callback = NULL;
  ret->set_log_level_callback = NULL;
  ret->eof_callback = NULL;

  if (static_input==NULL) static_input = ret;
  return ret;
}

static void bitInput_free(HANDLE_BITINPUT input)
{
  CRC_calculator_free(input->crc);
  free(input);
}


/* ------------------------------------- 
 *  -  bits and bytes                 -
 * ------------------------------------- */


int bitinput_resetCounter(HANDLE_BITINPUT input)
     /* reset bytePerBlock couter, but be aware of nbits/cword! */
{
  int read_bytes = input->byte_count-(input->nbits>>3);
  input->byte_count = (input->nbits>>3);
  return read_bytes;
}


static void reset_input_file(HANDLE_BITINPUT input)
{
  input->nbits = input->cword = 0;
  input->byte_count = 0;
  fseek(input->fp, 0, SEEK_SET);
  input->rdb_in_frame=-1;
  CRC_reset(input->crc);
}

#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
static void reset_input_mp4(HANDLE_BITINPUT input, char *Mp4Pos, int length)
{
  input->nbits = input->cword = 0;
  input->byte_count = 0;
  input->MP4AacBp = (u8*)Mp4Pos;
  input->mp4_frameLen = length;
  CRC_reset(input->crc);
}



/* get byte from input */
static int getbyte_fromMP4(HANDLE_BITINPUT input)
{
  if ((u32)input->byte_count++<input->mp4_frameLen) {
    return *(input->MP4AacBp)++;
  }
  return -1;
}
#endif
#endif


/* get byte from input */
static int getbyte_fromFile(HANDLE_BITINPUT input)
{
  int c;
  c = getc(input->fp);
  if (c < 0) return -1;
  
  input->byte_count++;
  return c;
}

#ifdef FRAMELENGTH_FLAG
extern int copy_to_output;
#endif

void writebits(int n, long value) {
#ifdef FRAMELENGTH_FLAG
  copy_to_output = 1;
#endif
  if (static_input->getbits_callback) {
    (*static_input->getbits_callback)(n, value);
  }
}

/* whole input goes through this function */
long getbits(int n)
{
  return GetBits(static_input, n);
}

long GetBits(HANDLE_BITINPUT input, long n)
{
  int retval;
  if (!n) return 0;

  while (input->nbits < n) {
    int c1;
    if ((c1 = (*input->getbyte)(input)) < 0) {
      if (input->eof_callback!=NULL) (*input->eof_callback)();
      else myexit("GetBits: End of Bitstream");
    }
    input->cword = (input->cword<<8) | c1;
    input->nbits += 8;
  }

  input->nbits -= n;
  retval = (input->cword>>input->nbits) & ((1<<n)-1);
  CRC_feed_calculator(input->crc, n, retval);
  if (input->getbits_callback) {
    (*input->getbits_callback)(n, retval);
  }
  return retval;
}

int byte_align(void)
{
  return byteAlign(static_input);
}

int byteAlign(HANDLE_BITINPUT input)
{
  int i=0;

  if (debug['a']) PRINT(SE,"align: %lx %d\n", input->cword, input->nbits);

  i=(input->nbits & ((1<<3)-1));
  getbits(i);

  return(i);
}




/* ------------------------------------- 
 *  -     A D T S                     -
 * ------------------------------------- */

static int get_adts_header(HANDLE_BITINPUT input, int extended_show)
{
#ifdef NEW_ADTS
   int i;
#endif
   ADTS_Header *p = &input->adts_head;

   p->syncword = GetBits(input, LEN_SYNCWORD);
   p->id = GetBits(input, LEN_ID);
   p->layer = GetBits(input, LEN_LAYER);
   p->protection_abs = GetBits(input, LEN_PROTECT_ABS);
   p->profile = GetBits(input, LEN_PROFILE);
   p->sampling_freq_idx = GetBits(input, LEN_SAMP_IDX);
   p->private_bit = GetBits(input, LEN_PRIVTE_BIT);
   p->channel_config = GetBits(input, LEN_CHANNEL_CONF);
   p->original_copy = GetBits(input, LEN_ORIG);
   p->home = GetBits(input, LEN_HOME);
   p->copyright_id_bit = GetBits(input, LEN_COPYRT_ID_ADTS);
   p->copyright_id_start = GetBits(input, LEN_COPYRT_START);
   p->frame_length = GetBits(input, LEN_FRAME_LEN);
   p->adts_buffer_fullness = GetBits(input, LEN_ADTS_BUF_FULLNESS);
   p->num_of_rdb = GetBits(input, LEN_NUM_OF_RDB);

   if (p->syncword != 0xfff) {
     PRINT(SE, "ADTS syncword error\n");
     return -1;
   }

#ifndef MPEG4V1
   if (p->id != 1) {
     PRINT(SE, "ADTS-header id has to be 1 for MPEG2-AAC\n");
   }
#endif

#ifdef NEW_ADTS
   if (p->num_of_rdb == 0) {
#endif
     if(p->protection_abs == 0) {
       p->crc_check = GetBits(input, LEN_CRC);
     }
#ifdef NEW_ADTS
   } /* if (p->num_of_rdb == 0) */
   else
   {
     /* adts_header_error_check */
     if(p->protection_abs == 0) {
       CRC_enable(input->crc, (p->num_of_rdb*16), 0);
       for (i = 1; i <= p->num_of_rdb; i++) {
         p->rdb_position[i-1] = GetBits(input, LEN_RDB_POSITION);
       }
       p->crc_check = GetBits(input, LEN_CRC);
     }
   }
#endif

   if(p->layer != 0){
     PRINT(SE, "ADTS layer data error!   layer: %d \n",p->layer);
     return -1;
   }

#ifdef MPEG4V1
   if (p->profile > LTP_Profile)
#else
   if (p->profile > SSR_Profile)
#endif
     PRINT(SE, "Warning!!! Invalid Profile ID in ADTS-Header\n");

   mc_info.profile = p->profile;
   mc_info.sampling_rate_idx = p->sampling_freq_idx;
   mc_info.sampling_rate = sampling_rate[mc_info.sampling_rate_idx];

   if ((bno==0)&&(debug['v']||debug['h']||extended_show)) {
     print_adts_header(p);
   }
   return 0;
}

static int check_adts_crc(HANDLE_BITINPUT input, unsigned long calculated_crc, int is_header)
{
  ADTS_Header *p = &input->adts_head;
  unsigned long announced_crc;
  if ((!input->adts_header_present)||(p->protection_abs)) return 0;

#ifdef NEW_ADTS  
  if ((p->num_of_rdb == 0)||(is_header)) {
#endif
    announced_crc = p->crc_check;
#ifdef NEW_ADTS
  } else {
    setLogLevel(input, ID_NUL, 0, 0);    /* cut away the CRC-values */
    p->crc_check_rdb = GetBits(input, LEN_CRC);
    setLogLevel(input, LOG_FIN, 0, 0);
    announced_crc = p->crc_check_rdb;
  }
#endif

  if (calculated_crc!=announced_crc) {
    PRINT(SE,"CRC: 0x%04lx, but header said : 0x%04lx => FAIL\n",calculated_crc,announced_crc);
    return -1;
  } else {
    if (debug ['C']) PRINT(SE,"CRC: 0x%04lx => OK!\n",calculated_crc);
    return 0;
  }
}

const ADTS_Header* bitinput_getCurrentADTSHeader(HANDLE_BITINPUT input)
{
  if (input == NULL) return NULL;
  if (input->adts_header_present == 0) return NULL;
  if (input->rdb_in_frame<=0) return &input->adts_head;
  return NULL;
}






static int getADTSinfo(HANDLE_BITINPUT input,
                ProgConfig *prog_cfg,
                MC_Info *mip,
                int *channelConfig,
                long *maxBitrate,
                long *avgBitrate,
                int *bufferFullness,
                int verbose )
{
  ADTS_Header *p;
  int tmp;
  
  setLogLevel(input, ID_HDR, 0, 0);
  tmp = get_adts_header(input, verbose);

  p = &input->adts_head;
  if (tmp) {
    PRINT(SE,"Warning: missing ADTS-syncword... trying RAW\n");
    *maxBitrate = *avgBitrate = *bufferFullness = 0;
    if (mip->profile==-1) mip->profile=Main_Profile;
    reset_input_file(input);
    decoder_setupParameters(mc_info.sampling_rate_idx);
    dec_parse();
  } else {
    input->adts_header_present = 1;
    if (p->adts_buffer_fullness==0x7ff) { /* variable bitrate */
      *bufferFullness = 0x7fffffff;
      *maxBitrate = *avgBitrate = -1;
    } else { /* constant bitrate */
      int old_fullness = p->adts_buffer_fullness;

      setLogLevel(input, LOG_FIN, 0, 0);
#ifdef NEW_ADTS
      if (p->num_of_rdb > 0) {
        setLogLevel(input, END_HDR, 0, 0);
      }
#endif

      /* parse one block to estimate bitrate */
      decoder_setupParameters(mc_info.sampling_rate_idx);
      dec_parse();

      *bufferFullness = (old_fullness<<5)*channel_num;

      input->byte_count = (input->nbits>>3);
      if (get_adts_header(input, 0)) {
        *avgBitrate = 0; PRINT(SE, "multiple raw-data-blocks per frame => expect bitrate problems\n");
      } else {
        *avgBitrate = (unsigned int)FRAMELEN2BITRATE((((p->adts_buffer_fullness-old_fullness)<<2)*channel_num +p->frame_length)<<3);
      }
      *maxBitrate = -1;
    }
  }

  decoder_setupParameters(mc_info.sampling_rate_idx);
  reset_input_file(input);

  if (input->adts_header_present) {
    if ((!p->channel_config)||(prog_config.profile)||(prog_config.sampling_rate_idx)) {
      /* there was a PCE */
      *channelConfig = 0;
      memcpy(prog_cfg, &prog_config, sizeof(ProgConfig));
    } else *channelConfig = p->channel_config;
  } else {
    if (mc_info.nch==8) *channelConfig = 7; /* see 13818-7, Table 8.1 */
    else *channelConfig = mc_info.nch;
  }

  memcpy(prog_cfg,&prog_config,sizeof(ProgConfig));

  return 0;
}



/* ------------------------------------- 
 *  -     A D I F                     -
 * ------------------------------------- */


/* read adif_header */
static int get_adif_header(HANDLE_BITINPUT input, ProgConfig *pc, ADIF_Header *head, int extended_show)
{
  int i, n, tag, select_status;
  ProgConfig tmp_config;
  
  select_status = 0;
  
  /* adif header */
  for (i=0; i<LEN_ADIF_ID; i++)
    head->adif_id[i] = GetBits(input, LEN_BYTE); 
  head->adif_id[i] = 0;	    /* null terminated string */
  /* test for id */
  if (strncmp(head->adif_id, "ADIF", 4) != 0)
    return -1;	    /* bad id */

  /* copyright string */	
  if ((head->copy_id_present = GetBits(input, LEN_COPYRT_PRES)) == 1) {
    for (i=0; i<LEN_COPYRT_ID; i++)
      head->copy_id[i] = GetBits(input, LEN_BYTE); 
    head->copy_id[i] = 0;  /* null terminated string */
  }
  head->original_copy = GetBits(input, LEN_ORIG);
  head->home = GetBits(input, LEN_HOME);
  head->bitstream_type = GetBits(input, LEN_BS_TYPE);
  head->bitrate = GetBits(input, LEN_BIT_RATE);

  /* program config elements */ 
  n = GetBits(input, LEN_NUM_PCE) + 1;

  head->buffer_fullness = (head->bitstream_type == 0) ? GetBits(input, LEN_ADIF_BF) : 0;

  if (debug['v']||debug['h']||extended_show) {
    print_adif_header(head);
    PRINT(SE, "number of PCE: %i\n", n);
  }
    
  for (i=0; i<n; i++) {
    tag = get_prog_config(&tmp_config);
    /***** this is redundant
           if (current_program < 0)
           current_program = tag;
    *****/
    if (tag == current_program) {
      memcpy(pc, &tmp_config, sizeof(tmp_config));
      select_status = 1;
#ifdef MPEG4V1
      if (tmp_config.profile > LTP_Profile)
#else
      if (tmp_config.profile > SSR_Profile)
#endif
        PRINT(SE, "Warning!!! Invalid Profile ID in PCE\n");
    }
    if (debug['v'])
      PRINT(SE, "tag %d %s\n", tag, (tag == current_program) ? "match" : "");
    if (extended_show||debug['h']) {
      PRINT(SE, "\nPCE number %i:\n", i);
      print_PCE(&tmp_config);
    }
  }

  if ((debug['v'] && select_status)&&(!(extended_show||debug['h']))) {
    PRINT(SE, "\n" );
    print_PCE(pc);
  }
  
  return select_status;
}




static int getADIFinfo(HANDLE_BITINPUT input,
                ProgConfig *pc,
                MC_Info *mip,
                int *channelConfig,
                long *maxBitrate,
                long *avgBitrate,
                int *bufferFullness,
                int verbose )
{
  ADIF_Header adif_header;
  int tmp;

  tmp = get_adif_header(input, pc, &adif_header, verbose);
  if (tmp<0) { return -2; } /* not ADIF */
  if (tmp==0) { /* selected program not found */
    PRINT(SE, "adif_header\n");
    return -1;
  }

  input->byte_count = (input->nbits>>3); /* reset bytecounter */
  if ((input->nbits & 7) != 0) {
    PRINT(SE, "ADIF header byte alignment error\n");
    return -1;
  }

  if ( adif_header.bitrate <= 0 ) {
    PRINT(SE, "negative or zero ADIF-bitrate\n");
    return -1;
  }

  /* move pc.sampling_rate_idx -> mc_info? */
  *channelConfig = 0;

  if (adif_header.bitstream_type == 0) {
    *bufferFullness = adif_header.buffer_fullness;
    *maxBitrate = -1;
    *avgBitrate = adif_header.bitrate;
  } else {
    *bufferFullness = 0x7fffffff;
    *maxBitrate = adif_header.bitrate;
    *avgBitrate = -2;
  }


#ifdef CT_SBR
  /* Check for SBR data in the fill element, 
     by parsing the first block. 
     Reset the file-ptr and the CRC and 
     read the adif header oncemore.*/
  
  decoder_setupParameters(mip->sampling_rate_idx);
  dec_parse();
  reset_input_file(input);
  get_adif_header(input, pc, &adif_header, 0);
#endif

  input->adif_header_present=1;

  return 0;
}



#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
/* ------------------------------------- 
 *  -     M P 4 f f                   -
 * ------------------------------------- */




/* parse a audio-specific-config; verbose if extended_show is set */
static void get_decoderSpecificConfig(HANDLE_BITINPUT input,
                                      MC_Info *mip,
                                      ProgConfig *pc,
                                      int extended_show,
#ifdef CT_SBR
                                      int bytesToDecode,
#endif
                                      int *channelConfig)
{
  int objtype, samplingInfo, tmp;
  int sampRate;
  int nChannelConf;

#ifdef CT_SBR
  int extensionSampRate = 0;
  int extensionSamplingInfo;
  int extensionObjtype;
  int bits_to_decode;
#endif

  /* Audio Specific Configuration */
  objtype = GetBits(input, lenAudioObjectType); /* from AudioSpecificConfig */

  if (extended_show||debug['h']) PRINT(SE, "audioObjectType           %d\n",objtype);
#ifdef CT_SBR
  if ((objtype > LTP_Profile + 1 || objtype < Main_Profile + 1) && objtype != 5){
#ifdef PARAMETRICSTEREO
    if (objtype != 29)
#endif
      PRINT(SE, "Warning!!! Invalid Object Type in AudioSpecificConfig\n");
  }
#else
  if (objtype > LTP_Profile + 1 || objtype < Main_Profile + 1 ){
    PRINT(SE, "Warning!!! Invalid Object Type in AudioSpecificConfig\n");
  }
#endif  
  

  samplingInfo = GetBits(input, lenSamplingFrequencyIndex);

  if (extended_show||debug['h']) PRINT(SE, "samplingFrequencyIndex    %d\n",samplingInfo);
  if (samplingInfo == samplingFrequencyEscape) {
    sampRate = GetBits(input, lenSamplingFrequency);

    samplingInfo = getNextSampFreqIndex(sampRate);
    if (extended_show||debug['h']) PRINT(SE, "samplingFrequency         %d (tables %d)\n",sampRate,samplingInfo);
  } else {
    sampRate = sampling_rate[samplingInfo];
  }

  nChannelConf = GetBits(input, lenChannelConfiguration);
  if (channelConfig) *channelConfig = nChannelConf;
  if (extended_show||debug['h']) PRINT(SE, "channelConfiguration      %d\n",nChannelConf);
	

#ifdef CT_SBR
  /* Non-backwards compatible explicit signalling of SBR.*/
  mip->sbrPresentFlag = 2;
#ifdef PARAMETRICSTEREO
  if (mip->sbrEnablePS == 1) /* the user wants to decode PS */
    mip->sbrEnablePS = 2;
#endif
  if (objtype == 5
#ifdef PARAMETRICSTEREO
      || objtype == 29
#endif
      ) {
    if (extended_show||debug['n']) {
#ifdef PARAMETRICSTEREO
      if (objtype == 29) {
        PRINT(SE, "Non-backwards compatible explicit signalling of PS (and thereby SBR).\n");
      }
      else
#endif
        PRINT(SE, "Non-backwards compatible explicit signalling of SBR.\n");
    }
    extensionObjtype = 5;
    mip->sbrPresentFlag = 1;
#ifdef PARAMETRICSTEREO
    if (objtype == 29) {
      if (mip->sbrEnablePS != 0) {
        mip->sbrEnablePS = 1;
      }
      else {
        PRINT(SE, "Warning: explicit non-backward compatible signaling of PS found, but PS decoding not enabled on the commandline! PS decoding disabled!\n");
      }
    }
#endif

    extensionSamplingInfo = GetBits(input, lenSamplingFrequencyIndex);

    if (extended_show||debug['h']) PRINT(SE, "extensionSamplingFrequencyIndex    %d\n",extensionSamplingInfo);
    if (extensionSamplingInfo == samplingFrequencyEscape) {
      extensionSampRate     = GetBits(input, lenSamplingFrequency);
      extensionSamplingInfo = getNextSampFreqIndex(extensionSampRate);
      if (extended_show||debug['h']) 
        PRINT(SE, "extensionSamplingFrequency         %d (tables %d)\n",extensionSampRate,extensionSamplingInfo);
    } 
    else {
      extensionSampRate = sampling_rate[extensionSamplingInfo];
    }
    
    /* Audio Specific Configuration */
    objtype = GetBits(input, lenAudioObjectType); /* from AudioSpecificConfig */
    if (extended_show||debug['h']) PRINT(SE, "audioObjectType           %d\n",objtype);
    if(objtype > LTP_Profile + 1 || objtype < Main_Profile + 1 ){
      PRINT(SE, "Warning!!! Invalid Object Type in AudioSpecificConfig\n");
    }
  }
  else{
    extensionObjtype = 0;
  }
#endif

  /* GA Specific Configurtaion */
  tmp = GetBits(input, lenFrameLengthFlag);
  if (extended_show||debug['h']) PRINT(SE, "frameLengthFlag           %d\n",tmp);
  tmp = GetBits(input, lenDependsOnCoreCoder);
  if (extended_show||debug['h']) PRINT(SE, "dependsOnCoreCoder        %d\n",tmp);
  tmp = GetBits(input, lenExtensionFlag);
  if (extended_show||debug['h']) PRINT(SE, "extensionFlag             %d\n",tmp);


  /* pce */

  if(nChannelConf==0) {
    get_prog_config(pc);
    if (extended_show||debug['h'])  {
      PRINT(SE, "\n");
      print_PCE(pc);
    }
    mip->profile = pc->profile; 
    if (extended_show||debug['h']) PRINT(SE, "ProfileID in PCE          %d\n",mip->profile);
    if(mip->profile > LTP_Profile || mip->profile < Main_Profile){
      PRINT(SE, "Warning!!! Invalid Profile ID in PCE\n");
    }
  }
  /*else*/ {
    mip->profile = objtype -1 ; /* Use object type for AAC Profile */
    mip->sampling_rate_idx = samplingInfo;
    mip->sampling_rate = sampRate;
  }

#ifdef CT_SBR
  /* Backwards compatible explicit signalling of SBR.*/
 
  bits_to_decode = bytesToDecode*8 - input->byte_count*8 + input->nbits;
  if(extensionObjtype != 5 && bits_to_decode >= 16){
    
    int syncExtensionType = GetBits(input, 11);
    if (extended_show||debug['h']) PRINT(SE, "syncExtensionType (0x2b7) 0x%x\n",syncExtensionType);
    if(syncExtensionType == 0x2b7){
      extensionObjtype = GetBits(input, lenAudioObjectType); /* from AudioSpecificConfig */
      if (extended_show||debug['h']) PRINT(SE, "extensionAudioObjectType  %d\n",extensionObjtype);
      if(extensionObjtype == 5){
        if (extended_show||debug['n']) 
          PRINT(SE, "Backwards compatible explicit signalling of SBR.\n");

        mip->sbrPresentFlag = GetBits(input, 1);
        if (extended_show||debug['h']) PRINT(SE, "sbrPresentFlag            %d\n",mip->sbrPresentFlag);
        if (mip->sbrPresentFlag) {
          extensionSamplingInfo = GetBits(input, lenSamplingFrequencyIndex);
          if (extended_show||debug['h']) PRINT(SE, "extensionSamplingFrequencyIndex    %d\n",extensionSamplingInfo);
          if (extensionSamplingInfo == samplingFrequencyEscape) {
            extensionSampRate     = GetBits(input, lenSamplingFrequency);
            extensionSamplingInfo = getNextSampFreqIndex(extensionSampRate);
            if (extended_show||debug['h']) 
              PRINT(SE, "extensionSamplingFrequency         %d (tables %d)\n",extensionSampRate,extensionSamplingInfo);
          }
          else {
            extensionSampRate = sampling_rate[extensionSamplingInfo];
          }
#ifdef PARAMETRICSTEREO
          bits_to_decode = bytesToDecode*8 - input->byte_count*8 + input->nbits;
          if (bits_to_decode >= 12) {
            syncExtensionType = GetBits(input, 11);
            if (extended_show||debug['h']) PRINT(SE, "syncExtensionType (0x548) 0x%x\n",syncExtensionType);
            if (syncExtensionType == 0x548) {
              int psEnabled;
              if (extended_show||debug['n']) {
                PRINT(SE, "Backwards compatible explicit signalling of PS.\n");
              }
              psEnabled = GetBits(input, 1);
              if (extended_show||debug['h']) PRINT(SE, "psPresentFlag             %d\n",psEnabled);
              if (mip->sbrEnablePS != 0) {
                mip->sbrEnablePS = psEnabled;
              }
              else {
                if (psEnabled) {
                  PRINT(SE, "Warning: explicit backward compatible signaling of PS found, but PS decoding not enabled on the commandline! PS decoding disabled!\n");
                }
              }
            }
          }
#endif
        }
#ifdef PARAMETRICSTEREO
        else {
          mip->sbrEnablePS = 0;
        }
#endif
      }
    }
  }
#endif



#ifdef CT_SBR
  if(mip->sbrPresentFlag == 1){
    if(extensionSampRate == sampRate)
      mip->bDownSampledSbr = 1;
  }
#endif

}


/* init all variables specific for decoding a mp4file */
static ISOErr getMP4info(HANDLE_BITINPUT input,
                         ProgConfig *prog_cfg,
                         MC_Info *mip,
                         int *channelConfig,
                         long *maxBitrate,
                         long *avgBitrate,
                         int *bufferFullness,
                         int verbose )
{
  MP4Err err;
  ISOMedia media;
  ISOHandle initialObjectDescriptorH;
  ISOHandle decoderSpecificInfoH;

  u32 handlerType;
  u32 objectTypeIndication;
  u32 streamType;
  u32 decoderBufferSize;
  u32 tmp;
  u32 media_timeScale;
  u64 media_duration;

  if ((err = ISONewHandle( 0, &initialObjectDescriptorH ))) {
    PRINT(SE, "mp4lib: isoNewHandle failed\n");
    return err;
  }
  if ((err = ISONewHandle( 0, &decoderSpecificInfoH ))) {
    PRINT(SE, "mp4lib: isoNewHandle failed\n");
    return err;
  }
  if ((err = MP4GetMovieInitialObjectDescriptor( input->mp4_moov, initialObjectDescriptorH ))) {
    PRINT(SE, "mp4lib: get initialOD failed\n");
    return err;
  }
  if ((err = ISOGetMovieIndTrack( input->mp4_moov, /*trackNumber*/ 1, &input->mp4_trak ))) {
    PRINT(SE, "mp4lib: GetTrack failed\n");
    return err;
  }
  if ((err = ISOGetTrackMedia( input->mp4_trak, &media ))) {
    PRINT(SE, "mp4lib: getTrackMedia failed\n");
    return err;
  }
  if ((err = ISOGetMediaHandlerDescription( media, &handlerType, NULL ))) {
    PRINT(SE, "mp4lib: getMediaDescription failed\n");
    return err;
  }
  if ((err = MP4GetMediaDecoderInformation( media,
                                            /*sampleDescIndex*/1,
                                            &objectTypeIndication,
                                            &streamType,
                                            &decoderBufferSize,
                                            /*upstream*/&tmp,
                                            (u32*)maxBitrate,
                                            (u32*)avgBitrate,
                                            decoderSpecificInfoH ))) {
    PRINT(SE, "mp4lib: getMediaDecoderInformation failed\n");
    return err;
  }

  {
    /* check durations and timeScales */
    u64 duration1, duration2, *durationptr = &duration2;
    u32 timeScale1, timeScale2, swaptmp32;
    if ((err = ISOGetMovieTimeScale(input->mp4_moov, &timeScale1))) {
      PRINT(SE,"mp4lib: getMovieTimeScale failed\n");
      return err;
    }
    if (timeScale1==0) {
      PRINT(SE, "WARNING: Movie timeScale equals zero!\n");
      timeScale1=1;
    }

    if ((err = ISOGetMovieDuration(input->mp4_moov, &duration2))) {
      PRINT(SE, "mp4lib: getMovieDuration failed\n");
      return err;
    }
    if ((err = ISOGetTrackDuration(input->mp4_trak, &duration1))) {
      PRINT(SE, "mp4lib: getTrackDuration failed\n");
      return err;
    }

    if (duration1 != duration2) {
      PRINT(SE, "WARNING: Movie duration (%i) != Track duration (%i)\n",
            (int)duration2, (int)duration1);
    }

    if ((err = ISOGetMediaTimeScale(media, &timeScale2))) {
      PRINT(SE, "mp4lib: getMediaTimeScale failed\n");
      return err;
    }
    if (timeScale2==0) {
      PRINT(SE, "WARNING: Media timeScale equals zero!\n");
      timeScale2=1;
    }
    media_timeScale = timeScale2;
    if (timeScale2 < timeScale1) {
      durationptr = &duration1;
      swaptmp32 = timeScale1;
      timeScale1 = timeScale2;
      timeScale2 = swaptmp32;
    }
    if ((err = ISOGetMediaDuration(media, durationptr))) {
      PRINT(SE, "mp4lib: getMediaDuration failed\n");
      return err;
    }
    media_duration = *durationptr;

    if (((double) duration1   /timeScale1 > (double)duration2/timeScale2) || 
        ((double)(duration1+1)/timeScale1 <=(double)duration2/timeScale2)) {
      PRINT(SE,"WARNING: Media play-time != Track play-time (%f!=%f)\n",
            (double)duration1/timeScale1, (double)duration2/timeScale2);
    }
  }

  if ( handlerType != ISOAudioHandlerType ) {
    PRINT(SE, "mp4ff: not an audio movie");
    return ISOBadDataErr;
  }
  if ( objectTypeIndication != 0x40 ) {
    PRINT(SE, "mp4ff: not an MPEG-4 movie");
    return ISOBadDataErr;
  }
  if ( streamType != 0x05 ) {
    PRINT(SE, "mp4ff: not an audio stream");
    return ISOBadDataErr;
  }
  if ( decoderBufferSize > MAX_FRAME_LEN ) {
    PRINT(SE, "mp4ff: required buffer size too large");
    return ISOBadDataErr;
  }

  /* read (and possibly print) decoderSpecificInfo */
  if ((err = ISOGetHandleSize( decoderSpecificInfoH, &tmp/*unitSize*/ ))) {
    PRINT(SE, "mp4lib: getHandleSize failed\n");
    return err;
  }
  reset_input_mp4(input, *decoderSpecificInfoH, tmp/*unitSize*/);
  get_decoderSpecificConfig(input, mip, prog_cfg,
                            (verbose == 1),
#ifdef CT_SBR
                            tmp,
#endif
                            channelConfig);

  if ((unsigned)mip->sampling_rate != media_timeScale) {
    PRINT(SE, "WARNING: Media timeScale (%i) != Sampling rate (%i)\n",
          media_timeScale, mip->sampling_rate);
  }

  ISODisposeHandle(initialObjectDescriptorH);
  ISODisposeHandle(decoderSpecificInfoH);

  if ((err = ISOCreateTrackReader( input->mp4_trak, &input->mp4_reader ))) {
    PRINT(SE, "mp4lib: createTrackReader failed\n");
    return err;
  }
  if ((err = ISONewHandle( 0, &input->mp4_sampleH ))) {
    PRINT(SE, "mp4lib: isoNewHandle failed\n");
    return err;
  }

  input->mp4_expected_duration = media_duration;
  
  input->mpeg4_header_present = 1;

  if (*avgBitrate==0) {
    *bufferFullness = 0x7fffffff;
    *avgBitrate = -1;
  } else {
    *bufferFullness = -1;
  }

  return err;
}
#endif
#endif



/* ------------------------------------- 
 *  -  opening files                  -
 * ------------------------------------- */


static int openInputFile( HANDLE_BITINPUT input, char *filename )
{
  if ((input->fp = fopen( filename, "rb" )) == NULL) {
    PRINT(SE, "open failed: %s\n",filename);
    return -1;
  }
  return 0;
}

#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
static ISOErr openMP4InputFile( HANDLE_BITINPUT input, char *filename )
{
  ISOErr err = ISOOpenMovieFile( &input->mp4_moov, filename, ISOOpenMovieNormal );
  if (err!=ISONoErr) input->mp4_moov=NULL;
  return err;
}
#endif
#endif



HANDLE_BITINPUT	bitinput_openFile(char *filename,
                                  ProgConfig *pc,
                                  MC_Info *mip,
                                  int *channelConfig,
                                  long *maxBitrate,
                                  long *avgBitrate,
                                  int *bufferFullness,
                                  int verbose )
{
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISOErr err = ISONoErr;
#endif
#endif
  HANDLE_BITINPUT ret = NULL;
  int retVal = 0;
  ret = bitInput_alloc();
  if (ret==NULL) return NULL;

  ret->adif_header_present = 0;
  ret->adts_header_present = 0;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ret->mpeg4_header_present = 0;
  err = openMP4InputFile(ret, filename);
  if (err == ISONoErr) {
    ret->getbyte = &getbyte_fromMP4;
    err = getMP4info(ret,
                     pc,
                     mip,
                     channelConfig,
                     maxBitrate,
                     avgBitrate,
                     bufferFullness,
                     verbose );
    if (err) {
      bitInput_free(ret);
      return NULL;
    }

    if (debug['n']) PRINT(SE, "MPEG4 header present\n");
  } else
#endif
#endif
  {
    retVal = openInputFile(ret, filename);
    if (retVal) {
      PRINT(SE, "Error opening %s\n",filename);
      bitInput_free(ret);
      return NULL;
    }

    retVal = getADIFinfo(ret,
                         pc,
                         mip,
                         channelConfig,
                         maxBitrate,
                         avgBitrate,
                         bufferFullness,
                         verbose );
    if (retVal!=-2) {
      if (retVal!=0) { /* error opening ADIF */
        bitInput_free(ret);
        return NULL;
      }

      if (debug['n']) PRINT(SE, "ADIF header present\n");
    } else {
      reset_input_file(ret);
      retVal = getADTSinfo(ret,
                           pc,
                           mip,
                           channelConfig,
                           maxBitrate,
                           avgBitrate,
                           bufferFullness,
                           verbose );
      if (retVal!=0) { /* error opening raw or adts */
        bitInput_free(ret);
        return NULL;
      }

      if ((debug['n'])&&(ret->adts_header_present)) PRINT(SE, "ADTS header found\n");
    }

    /* if (debug['n'] && sbrPresentInFillElem) PRINT(SE, "\rSBR present in Fill element\n"); */
  }

  return ret;
}


void bitinput_setCallbacks(HANDLE_BITINPUT input,
                           void (*getbits_callback)(int numBits, long bitValue),
                           void (*set_log_level_callback)(int id, int add_bits_num, int add_bits))
{
  if (input == NULL) return;
  input->getbits_callback        = getbits_callback;
  input->set_log_level_callback  = set_log_level_callback;
}



/* ------------------------------------- 
 *  -  get next block/access unit     -
 * ------------------------------------- */


int bitinput_getNextBlock(HANDLE_BITINPUT input)
{
  int c;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  u32 i=0, duration, unitSize;
  ISOErr err = ISONoErr;
#endif
#endif
  if (input==NULL) return -1;


#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  /* read next AccexxUnit from MP4 file */
  if (input->mpeg4_header_present) {
    err = MP4TrackReaderGetNextAccessUnitWithDuration( input->mp4_reader, input->mp4_sampleH, &unitSize, &i, &i, &i, &duration ); if (err) {
      if (err==ISOEOF) {
        PRINT(SE, "End of bitstream\n");

        /* test the announced movie duration */
        if (input->mp4_expected_duration != -1) {
          if (input->mp4_total_read_duration != input->mp4_expected_duration) {
            PRINT(SE,"WARNING: Header announced duration of %li samples, got %li samples\n",
                  (long)input->mp4_expected_duration, (long)input->mp4_total_read_duration);
          }
        }

        return -2;
      }
      return -1;
    }
    if (duration != aacFrameLength) {
      PRINT(SE, "WARNING: sample duration is %i and not %i\n", duration, aacFrameLength);
    }
    input->mp4_total_read_duration += duration;
    reset_input_mp4(input, (char*)*input->mp4_sampleH, unitSize);
  } else
#endif
#endif
  {
    if ((c = getc(input->fp)) == EOF) {
      PRINT(SE, "End of bitstream\n");
      return -2;
    }
    ungetc(c, input->fp);

    /* read ADTS header */
    if (input->adts_header_present == 1) {
      ADTS_Header *p = &input->adts_head;

      input->rdb_in_frame++;
      if (input->rdb_in_frame>p->num_of_rdb) input->rdb_in_frame = 0;
      if (input->rdb_in_frame<=0) { /* only if ADTS frame contains no more raw data blocks */

        setLogLevel(input, ID_HDR, 0, 0);  /* required for CRC and ADIF2MP4 */
        if (get_adts_header(input, 0)) myexit("ADTS header");
        setLogLevel(input, LOG_FIN, 0, 0);  /* required for CRC and ADIF2MP4 */

#ifdef NEW_ADTS
        if (p->num_of_rdb > 0) {
          setLogLevel(input, END_HDR, 0, 0);
        }
#endif

        if(debug['v'])
          PRINT(SE, "raw data blocks this ADTS frame:%i\n", p->num_of_rdb +1);

        byteAlign(input);
      }
    }
  }
  return 0;
}





/* ------------------------------------- 
 *  -  closing files                  -
 * ------------------------------------- */


#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
static void close_input_mp4(HANDLE_BITINPUT input)
{
  if (input->mp4_sampleH!=NULL) ISODisposeHandle(input->mp4_sampleH);
  if (input->mp4_reader!=NULL) ISODisposeTrackReader(input->mp4_reader);
  if (input->mp4_moov!=NULL) ISODisposeMovie(input->mp4_moov);
}
#endif
#endif

static void close_input_file(HANDLE_BITINPUT input)
{
  fclose(input->fp);
}


void bitinput_close(HANDLE_BITINPUT input)
{
  if (input==NULL) return;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  if (input->mpeg4_header_present) close_input_mp4(input);else
#endif
#endif
    close_input_file(input);
}




/* -------------------------------------
 *  - bit level logging               -
 * ------------------------------------- */

void set_log_level(int id, int add_bits_num, int add_bits)
{
  setLogLevel(static_input, id, add_bits_num, add_bits);
}

void setLogLevel(HANDLE_BITINPUT input, int id, int add_bits_num, int add_bits)
     /*
      * to be called once for each element in the stream
      * unfortunally this additional-bits construction is necessary, because
      * these bits indicate the elements tag
      */
{
  switch(id) {
  case ID_SCE:
  case ID_CPE:
  case ID_CCE:
  case ID_LFE: /*CRC-log 192 bit*/
    CRC_enable(input->crc, CRC_LEN_ELE,0);
    CRC_feed_calculator(input->crc, add_bits_num, add_bits);
    break;
  case ID_2ND: /*CRC-log additional 128 bit*/
    CRC_enable(input->crc, CRC_LEN_2ND,1);
    break;
  case ID_DSE:
  case ID_PCE: /*CRC-log until LOG_FIN*/
    CRC_enable(input->crc, CRC_LEN_ALL,0);
    CRC_feed_calculator(input->crc, add_bits_num, add_bits);
    break;
#ifdef NEW_ADTS
  case END_HDR: {/* CRC of ADTS header only */
    unsigned long crcValue = CRC_reset(input->crc);
    check_adts_crc(input, crcValue, 1);
  }
  break;
#endif
  case ID_END: {
    unsigned long crcValue = CRC_reset(input->crc);
    check_adts_crc(input, crcValue, 0);
  }
  case LOG_FIN: /*CRC finish (zero-pad)*/
    CRC_fillup(input->crc);
    break;
  case ID_HDR: /*CRC-log 56bit*/
    CRC_enable(input->crc, CRC_LEN_HDR,0);
    break;
  case ID_FIL:
  case ID_NUL: /*no CRC*/
    break;
  break;
  default:
    myexit("logger: HELP!!!");
  }

  if (input->set_log_level_callback!=NULL) (*input->set_log_level_callback)(id, add_bits_num, add_bits);
}


/* ------------------------------------- 
 *  -  other functions                -
 * ------------------------------------- */



static int num_audio_chn(const EleList *p, int cce )
{
  int n, i, j;

  n = 0;
  for (i=0; i<p->num_ele; i++) {
    j = p->ele_is_cpe[i];
    if (!cce)
      n += (j) ? 2 : 1;
    else 
      n += (j) ? 1 : 0;	/* ind sw cce */
  }
  return n;
}

int getDecoderBufferSize(const ProgConfig *p, int channelConfig, long *decoderBufferSize )
{
  int nch;
  if (channelConfig>0) {
    if (channelConfig==7) channelConfig++; /* FIXME: no LFE's??? */
    nch = channelConfig;
  } else {
    nch  = num_audio_chn(&p->front, 0);
    nch += num_audio_chn(&p->side, 0);
    nch += num_audio_chn(&p->back, 0);
    nch += num_audio_chn(&p->coupling, 1);
  }

  if (nch==0) { /* use decoder's speaker-configuration, if necessary */
    nch = channel_num;
  }
  
  if (nch==0) {
    PRINT(SE, "decoderBufferSize: 0 channels... setting to 1 channel!\n");
    nch=1;
  }
  
  *decoderBufferSize = nch * bufferSizePerAudioChn;
  if ( (0 < nch) && (nch <= 48)) {
    return 0;
  }
  else {
    PRINT(SE, "decoderBufferSize: %d channels\n", nch);
    return -1;
  }
}



