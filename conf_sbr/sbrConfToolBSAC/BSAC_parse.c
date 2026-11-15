/*
char *bst : input bitstream pointer for a frame
int val : the bitstream length of the current frame
int *BSAC_Len : the length of BSAC payload 
int *SBR_Len : the length of SBR payload
*/
#ifdef BSAC
#include "streamfile.h"
#include "BSAC_parse.h"

#define MAX_LAYER 100

int core_bandwidth;

short            
sfb_96_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 96, 108, 120, 
  132, 144, 156, 172, 188, 212, 240, 
  276, 320, 384, 448, 512, 576, 640, 
  704, 768, 832, 896, 960, 1024
};   /* 41 scfbands */

short
sfb_96_128[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 128
};   /* 12 scfbands */

short
sfb_64_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 100, 112, 124, 
  140, 156, 172, 192, 216, 240, 268, 
  304, 344, 384, 424, 464, 504, 544, 
  584, 624, 664, 704, 744, 784, 824, 
  864, 904, 944, 984, 1024
};   /* 41 scfbands 47 */ 

short
sfb_64_128[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 128
};   /* 12 scfbands */


short
sfb_48_1024[] =
{
        4,	8,	12,	16,	20,	24,	28,	
        32,	36,	40,	48,	56,	64,	72,	
        80,	88,	96,	108,	120,	132,	144,	
        160,	176,	196,	216,	240,	264,	292,	
        320,	352,	384,	416,	448,	480,	512,	
        544,	576,	608,	640,	672,	704,	736,	
        768,	800,	832,	864,	896,	928,	1024
};

short
sfb_48_128[] =
{
          4,	8,	12,	16,	20,	28,	36,	
          44,	56,	68,	80,	96,	112,	128
};

short
sfb_32_1024[] =
{
        4,	8,	12,	16,	20,	24,	28,	
        32,	36,	40,	48,	56,	64,	72,	
        80,	88,	96,	108,	120,	132,	144,	
        160,	176,	196,	216,	240,	264,	292,	
        320,	352,	384,	416,	448,	480,	512,	
        544,	576,	608,	640,	672,	704,	736,	
        768,	800,	832,	864,	896,	928,	960,
        992,	1024
};

short
sfb_24_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 52, 60, 68, 
  76, 84, 92, 100, 108, 116, 124, 
  136, 148, 160, 172, 188, 204, 220, 
  240, 260, 284, 308, 336, 364, 396, 
  432, 468, 508, 552, 600, 652, 704, 
  768, 832, 896, 960, 1024
};   /* 47 scfbands */

short
sfb_24_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 64, 76, 92, 108, 
  128
};   /* 15 scfbands */

short
sfb_16_1024[] =
{
  8, 16, 24, 32, 40, 48, 56, 
  64, 72, 80, 88, 100, 112, 124, 
  136, 148, 160, 172, 184, 196, 212, 
  228, 244, 260, 280, 300, 320, 344, 
  368, 396, 424, 456, 492, 532, 572, 
  616, 664, 716, 772, 832, 896, 960, 
  1024
};   /* 43 scfbands */

short
sfb_16_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 40, 48, 60, 72, 88, 108, 
  128
};   /* 15 scfbands */

short
sfb_8_1024[] =
{
  12, 24, 36, 48, 60, 72, 84, 
  96, 108, 120, 132, 144, 156, 172, 
  188, 204, 220, 236, 252, 268, 288, 
  308, 328, 348, 372, 396, 420, 448, 
  476, 508, 544, 580, 620, 664, 712, 
  764, 820, 880, 944, 1024
};   /* 40 scfbands */

short
sfb_8_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 60, 72, 88, 108, 
  128
};   /* 15 scfbands */

typedef struct {
    int	    samp_rate;
    int	    nsfb1024;
    short*  SFbands1024;
    int	    nsfb128;
    short*  SFbands128;
} SR_Info_CONF;

SR_Info_CONF
samp_rate_info_conf[(1<<4/*LEN_SAMP_IDX*/)] = {
    /* sampling_frequency, #long sfb, long sfb, #short sfb, short sfb */
    /* samp_rate, nsfb1024, SFbands1024, nsfb128, SFbands128 */
    {96000, 41, sfb_96_1024, 12, sfb_96_128},	    /* 96000 */
    {88200, 41, sfb_96_1024, 12, sfb_96_128},	    /* 88200 */
    {64000, 47, sfb_64_1024, 12, sfb_64_128},	    /* 64000 */
    {48000, 49, sfb_48_1024, 14, sfb_48_128},	    /* 48000 */
    {44100, 49, sfb_48_1024, 14, sfb_48_128},	    /* 44100 */
    {32000, 51, sfb_32_1024, 14, sfb_48_128},	    /* 32000 */
    {24000, 47, sfb_24_1024, 15, sfb_24_128},	    /* 24000 */
    {22050, 47, sfb_24_1024, 15, sfb_24_128},	    /* 22050 */
    {16000, 43, sfb_16_1024, 15, sfb_16_128},	    /* 16000 */
    {12000, 43, sfb_16_1024, 15, sfb_16_128},	    /* 12000 */
    {11025, 43, sfb_16_1024, 15, sfb_16_128},	    /* 11025 */
    {8000,  40, sfb_8_1024,  15, sfb_8_128 },	    /* 8000  */
    {7350,  40, sfb_8_1024,  15, sfb_8_128 },       /* 7350  */
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0}
};

int sampling_boundarys_conf[(1<<4/*LEN_SAMP_IDX*/)] = {
  92017, 75132, 55426, 46009, 37566, 27713, 23004, 18783,
  13856, 11502, 9391, 0, 0, 0, 0, 0
};

int  getNextSampFreqIndex_conf(int sampRate)
{
  int i = 0;
  if (sampRate < sampling_boundarys_conf[(1<<4/*LEN_SAMP_IDX*/)-1]) return -1;
  while (sampRate < sampling_boundarys_conf[i]) { i++; }
  return i;
}

void Get_CoreBW(int target, int windowSequence, int num_window_groups, int *window_group_length, int maxSfb, int base_band, int sampling_rate)
{
	int		i, g, reg, cband, layer, slayer_size;
	int		sampling_rate_idx;
	int		last_freq[MAX_LAYER];
	int		init_max_freq[8];
	int		end_freq[MAX_LAYER];
	int		end_cband[MAX_LAYER];
	int     layer_reg[MAX_LAYER];
	int     layer_max_freq[MAX_LAYER], layer_max_cband[MAX_LAYER];
	short	swb_offset[8][64];

	sampling_rate_idx = getNextSampFreqIndex_conf(sampling_rate);

	if(windowSequence == 2) {
		for(g = 0; g < num_window_groups; g++) {
			swb_offset[g][0] = 0;
			for(i = 0; i < samp_rate_info_conf[sampling_rate_idx].nsfb128; i ++)
				swb_offset[g][i+1] = samp_rate_info_conf[sampling_rate_idx].SFbands128[i] * window_group_length[g];
		}
		slayer_size = 0;
		for(g = 0; g < num_window_groups; g++) {

			init_max_freq[g] = base_band * window_group_length[g] * 4;
			last_freq[g] = swb_offset[g][maxSfb];

			switch(sampling_rate) {
				case 48000:
				case 44100:
					if ((init_max_freq[g]%32) >= 16)
						init_max_freq[g] = (init_max_freq[g] / 32) * 32 + 20;
					else if ((init_max_freq[g]%32) >= 4)
						init_max_freq[g] = (init_max_freq[g] / 32) * 32 + 8;
					break;
				case 32000:
				case 24000:
				case 22050:
					init_max_freq[g] = (init_max_freq[g] / 16) * 16;
					break;
				case 16000:
				case 12000:
				case 11025:
					init_max_freq[g] = (init_max_freq[g] / 32) * 32;
					break;
				case  8000:
					init_max_freq[g] = (init_max_freq[g] / 64) * 64;
					break;
			}

			if (init_max_freq[g] > last_freq[g])
				init_max_freq[g] = last_freq[g]; 
			end_freq[g] = init_max_freq[g];
			end_cband[g] = (end_freq[g] + 31) / 32;
			slayer_size += (init_max_freq[g]+31)/32;
		}
    

		layer = 0;
		for(g = 0; g < num_window_groups; g++)
		for (cband = 1; cband <= end_cband[g]; layer++, cband++) {

			layer_reg[layer] = g;
			layer_max_freq[layer] = cband * 32;
			if (layer_max_freq[layer] > init_max_freq[g])
				layer_max_freq[layer] = init_max_freq[g];

			layer_max_cband[layer] = cband;
		}

		layer = slayer_size;
		for(g = 0; g < num_window_groups; g++) {
			for (i=0; i<window_group_length[g]; i++) { 
				layer_reg[layer] = g;
				layer++;
			}
		}

		for (layer=slayer_size+8; layer<MAX_LAYER; layer++) 
			layer_reg[layer] = layer_reg[layer-8];

		for (layer = slayer_size; layer <MAX_LAYER; layer++) {
			reg = layer_reg[layer];
			switch(sampling_rate) {
				case 48000:
				case 44100:
					end_freq[reg] += 8;
					if ((end_freq[reg]%32) != 0)
						end_freq[reg] += 4;
					break;
				case 32000:
				case 24000:
				case 22050:
					end_freq[reg] += 16;
					break;
				case 16000:
				case 12000:
				case 11025:
					end_freq[reg] += 32;
				break;
				default:
					end_freq[reg] += 64;
					break;
			}
			if (end_freq[reg] > last_freq[reg] )
				end_freq[reg] = last_freq[reg];

			layer_max_freq[layer] = end_freq[reg];
		}
	} else {
		swb_offset[0][0] = 0;
		for(i = 0; i < samp_rate_info_conf[sampling_rate_idx].nsfb1024; i ++)
			swb_offset[0][i+1] = samp_rate_info_conf[sampling_rate_idx].SFbands1024[i];
		g = 0;

		for(layer = 0; layer < MAX_LAYER; layer++)
			layer_reg[layer] = 0;

		last_freq[g] = swb_offset[g][maxSfb];

		/************************************************/
		/* calculate layer_max_cband and layer_max_freq */
		/************************************************/
		init_max_freq[g] = base_band * 32;

		if (init_max_freq[g] > last_freq[g])
			init_max_freq[g] = last_freq[g];
		slayer_size = (init_max_freq[g]+31)/32;

		cband = 1;
		for (layer = 0; layer < slayer_size; layer++, cband++) {

			layer_max_freq[layer] = cband * 32;
			if (layer_max_freq[layer] > init_max_freq[g])
				layer_max_freq[layer] = init_max_freq[g];

			layer_max_cband[layer] = cband;
		}

		for (layer = slayer_size; layer < MAX_LAYER; layer++) {
			switch(sampling_rate) {
				case 48000:
				case 44100:
					layer_max_freq[layer] = layer_max_freq[layer-1] + 8;

					if ((layer_max_freq[layer]%32) != 0)
						layer_max_freq[layer] += 4;
					break;
				case 32000:
				case 24000:
				case 22050:
					layer_max_freq[layer] = layer_max_freq[layer-1] + 16;
					break;
				case 16000:
				case 12000:
				case 11025:
					layer_max_freq[layer] = layer_max_freq[layer-1] + 32;
				break;
				default:
					layer_max_freq[layer] = layer_max_freq[layer-1] + 64;
					break;
			}
			if (layer_max_freq[layer] > last_freq[g])
				layer_max_freq[layer] = last_freq[g];
		}
	}

	if(windowSequence == 2)
	{
		g = layer_reg[target+slayer_size-1];
		core_bandwidth = layer_max_freq[target+slayer_size-1]/window_group_length[g]*sampling_rate/2./128.;
	}
	else
	{
		core_bandwidth = layer_max_freq[target+slayer_size-1]*sampling_rate/2./1024.;
	}
}

extern int basc_dec_layer;

void	GetBSACPosInfo(HANDLE_BSBITSTREAM stream,int val,int *BSAC_Len,int *SBR_Len, int *BSACExtensionType, int nch, int sampling_rate)
{
	unsigned int	CHK_ZC;
	unsigned char	CHK_ext;
	int				bs_buf_Byte_idx = 0;
	int				loop = 1;
	unsigned long	bst[4];
	int				i, ch, bit_used;
	int				top_layer, maxSfb, base_band;
	int				windowSequence, groupInfo[7], num_window_groups, window_group_length[8];

	BsGetBit(stream, &top_layer, 6);
	bit_used = 6;

	BsGetBit(stream, &i, 2+nch*8);  /* base_snf_thr, max_scalefactor */
	BsGetBit(stream, &base_band, 5);
	bit_used += (2+nch*8+5);
	for(ch = 0; ch < nch; ch++) {
		BsGetBit(stream, &i, 5+3+3+4);   /* cband_si_type, scf_model0, scf_model1, max_sfb_si_len */
		bit_used += (5+3+3+4);
	}
	BsGetBit(stream, &i, 1);   /* ics_reserved */
	bit_used += 1;
	
	/*  window sequence */
	BsGetBit(stream, &windowSequence, 2);
	
	/*  window shape */
	BsGetBit(stream, &i, 1); 
	bit_used += 3;
	
	if (windowSequence == 2)
	{
		BsGetBit(stream, &maxSfb, 4);		
		for (i = 0; i < 7; i++)
			BsGetBit(stream, &groupInfo[i], 1);
		bit_used += 11;
	}
	else 
	{
		BsGetBit(stream, &maxSfb, 6);
		bit_used += 6;
	}

	if(bit_used&0x7) {
		BsGetBit(stream, &i, 8-(bit_used&0x7));
		bit_used += 8-(bit_used&0x7);
	}
	bs_buf_Byte_idx += (bit_used>>3);
	
	num_window_groups = 1;
	window_group_length[0] = 1;
	if (windowSequence == 2) {
		for (i=0; i<7; i++) {
			if (groupInfo[i]==0) {
				window_group_length[num_window_groups] = 1;
				num_window_groups++;
			}
			else {
				window_group_length[num_window_groups-1]++;
			}
		}
	}

	if(basc_dec_layer < top_layer)
		top_layer = basc_dec_layer;
	Get_CoreBW(top_layer, windowSequence, num_window_groups, window_group_length, maxSfb, base_band, sampling_rate);

	/* bits before window_Sequence: 11+4+1+6+2+8*nch+5+(5+3+3+4)*nch+1
	   framelength is read by GetBSACFrameLen, 11+5 bits */
	for(i = 0; i < 4; i++)
		BsGetBit(stream, &bst[i], 8);

	CHK_ZC = bst[bs_buf_Byte_idx]<<24 | bst[bs_buf_Byte_idx+1]<<16 | bst[bs_buf_Byte_idx+2]<<8 | bst[bs_buf_Byte_idx+3];
	bs_buf_Byte_idx+=4;

	while(CHK_ZC!=0 && bs_buf_Byte_idx<val)
	{
		BsGetBit(stream, &bst[0], 8);
		CHK_ZC = (CHK_ZC<<8) | bst[0];
		bs_buf_Byte_idx++;
	}
	if(bs_buf_Byte_idx >= val) {
		*BSACExtensionType = -1;
		return;
	}
	BsGetBit(stream, &bst[0], 8);
	CHK_ext = bst[0];
	bs_buf_Byte_idx++;
	while(loop && bs_buf_Byte_idx<val) {
		loop = 0;
		if (CHK_ext==0xf0 || CHK_ext==0xfe)
		{
			*BSAC_Len = bs_buf_Byte_idx;
			BsGetBit(stream, SBR_Len, 4);
			if(*SBR_Len == 15) {
				BsGetBit(stream, &bst[0], 8);
				*SBR_Len += bst[0]-1;
			}
			if(CHK_ext==0xfe) {
				*BSACExtensionType = 14;
			}
			else {
				*BSACExtensionType = 0;
			}
		}
		else if (CHK_ext==0xf1 || CHK_ext==0xfd)
		{
			*BSAC_Len = bs_buf_Byte_idx;
			BsGetBit(stream, SBR_Len, 4);
			if(*SBR_Len == 15) {
				BsGetBit(stream, &bst[0], 8);
				*SBR_Len += bst[0]-1;
			}
			if(CHK_ext==0xfd) {
				*BSACExtensionType = 13;
			}
			else {
				*BSACExtensionType = 1;
			}
		}
		else if (CHK_ext==0xff) {
			*BSAC_Len = bs_buf_Byte_idx;
			*SBR_Len = 0;
			*BSACExtensionType = 15;
		}
		else
		{
			CHK_ZC = (CHK_ZC<<8) | CHK_ext;
			BsGetBit(stream, &CHK_ext, 8);
			bs_buf_Byte_idx++;
			while(!(CHK_ZC==0 && (CHK_ext&0xf0) == 0xf0))
			{
				CHK_ZC = (CHK_ZC<<8) | CHK_ext;
				BsGetBit(stream, &CHK_ext, 8);
				bs_buf_Byte_idx++;
			}
			loop = 1;
		}
	}
}

void	GetBSACExtension(HANDLE_BSBITSTREAM stream,int val,int *BSAC_Len,int *SBR_Len, int *BSACExtensionType)
{
	unsigned long	tmp;
	int				i;
	
	if (*BSACExtensionType==0x0 || *BSACExtensionType==0xe)
	{
		*BSAC_Len = val;
		
		if(*BSACExtensionType==0xe) { /* MC with SBR */
			*BSACExtensionType = 14;
			BsGetBit(stream, &tmp, 4);
			for(i = 0; i < val-1; i ++)
				BsGetBit(stream, &tmp, 8);
			BsGetBit(stream, &tmp, 4);
		}
		else {
			*BSACExtensionType = 0;
		}

		/* get SBR length */
		BsGetBit(stream, SBR_Len, 4);
		if(*SBR_Len == 15) {
			BsGetBit(stream, &tmp, 8);
			*SBR_Len += tmp-1;
		}
	}
	else if (*BSACExtensionType==0x1 || *BSACExtensionType==0xd)
	{
		*BSAC_Len = val;
		
		if(*BSACExtensionType==0xd) { /* MC with SBR */
			*BSACExtensionType = 13;
			BsGetBit(stream, &tmp, 4);
			for(i = 0; i < val-1; i ++)
				BsGetBit(stream, &tmp, 8);
			BsGetBit(stream, &tmp, 4);
		}
		else {
			*BSACExtensionType = 1;
		}

		/* get SBR length */
		BsGetBit(stream, SBR_Len, 4);
		if(*SBR_Len == 15) {
			BsGetBit(stream, &tmp, 8);
			*SBR_Len += tmp-1;
		}
	}
	else if (*BSACExtensionType==0xff) {  /* MC */
		*BSAC_Len = val;
		*SBR_Len = 0;
		*BSACExtensionType = 15;
	}
	else
	{
		printf("Error: not supported BSAC extension type\n");
	}
}

typedef struct
{
  SBR_SYNC_STATE syncState ;
  SBR_FRAME_DATA frameData;
} SBR_CHANNEL;

typedef struct sbrDecoderInstance
{
  SBR_CHANNEL SbrChannel[MAX_NUM_CHANNELS];
#ifdef PARAMETRICSTEREO
  HANDLE_PS_DEC hParametricStereoDec;
#endif

} sbrDecoderInstance ;

extern float drcFactorVector[Chans][64][64];

SBR_ERROR
applySBR_BSAC (SBRDECODER self,
          SBRBITSTREAM * stream, 
          double* timeData[Chans],
          double* timeDataOut[Chans],
#ifdef SBR_SCALABLE
          int maxSfbFreqLine,
#endif
          int bDownSampledSbr,
		  Ch_Info ch_info[10],
					 /* 20060107 */
		  int	core_bandwidth
		  /*,int bUsedBSAC */
		)
{
  SBR_ERROR err = SBRDEC_OK ;

  int element, eleChannels=0;
  int  sbrChannelIndex = 0;
  int channelIndex;
  int processedChannels[/*MAX_TIME_CHANNELS*/Chans];
  int i;

  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0] ;
    
  memset(&processedChannels,0,/*MAX_TIME_CHANNELS*/Chans*sizeof(int));

  /* We have already done a check (outside this function) to make sure that SBR data exists. 
     This is since this implementation of the implicit signalling does not assume that SBR 
     exists, but rather checks for its existence, and adjusts the output sample rate if 
     SBR data is found.

     However, if we had assumed that SBR data did exist, we would have to check now if it does
     i.e. stream->NrElements != 0, and if it doesn't we would have to use the SBR-Tool for upsampling,
     since we've already set up the output device at twice the sampling rate.
     */
  if (stream->NrElements)
  {
	  /* 20060107 */
	  /* if (bUsedBSAC==1) */
			err = readSBR (self, stream) ;
	  /* else
			err = readSBR (self, stream, numChannels) ; */
    if (err != SBRDEC_OK) return err ;
  }


  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements ; element++)
  {
    if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE)
    {
      eleChannels = (stream->sbrElement [element].ElementID == SBR_ID_CPE) ? 2 : 1 ;
/* #if 0 */
      channelIndex = stream->sbrElement [element].chIndx;
/*#else
      channelIndex = sbrChannelIndex;
#endif */

      for(i=0; i<eleChannels; i++){
        decodeSbrData(SbrChannel[sbrChannelIndex + i].frameData.iEnvelope,
                      SbrChannel[sbrChannelIndex + i].frameData.sbrNoiseFloorLevel,
                      SbrChannel[sbrChannelIndex + i].frameData.domain_vec1,
                      SbrChannel[sbrChannelIndex + i].frameData.domain_vec2,
                      SbrChannel[sbrChannelIndex + i].frameData.nSfb,
                      SbrChannel[sbrChannelIndex + i].frameData.frameInfo,
                      SbrChannel[sbrChannelIndex + i].frameData.nNfb,
                      SbrChannel[sbrChannelIndex + i].frameData.ampRes,
                      SbrChannel[sbrChannelIndex + i].frameData.nScaleFactors,
                      SbrChannel[sbrChannelIndex + i].frameData.nNoiseFactors,
                      SbrChannel[sbrChannelIndex + i].frameData.offset,
                      SbrChannel[sbrChannelIndex + i].frameData.coupling,
                      sbrChannelIndex + i);
      }

      if(eleChannels == 2 && SbrChannel[sbrChannelIndex].frameData.coupling){
          sbr_envelope_unmapping (SbrChannel[sbrChannelIndex].frameData.iEnvelope,
                                  SbrChannel[sbrChannelIndex+1].frameData.iEnvelope,
                                  SbrChannel[sbrChannelIndex].frameData.sbrNoiseFloorLevel,
                                  SbrChannel[sbrChannelIndex+1].frameData.sbrNoiseFloorLevel,
                                  SbrChannel[sbrChannelIndex].frameData.nScaleFactors,
                                  SbrChannel[sbrChannelIndex].frameData.nNoiseFactors,
                                  SbrChannel[sbrChannelIndex+1].frameData.ampRes);
              
      }

#ifdef SBR_SCALABLE
      eleChannels = numChannels;
#endif

      for(i=0; i<eleChannels; i++){
        sbr_dec_bsac (timeData[channelIndex + i],
                 timeDataOut[channelIndex + i],
                 &(SbrChannel[sbrChannelIndex + i].frameData),
                 (SbrChannel[sbrChannelIndex + i].syncState==SBR_ACTIVE),
                 sbrChannelIndex + i,
#ifdef SBR_SCALABLE
                 maxSfbFreqLine,
#endif
                 bDownSampledSbr
				 ,drcFactorVector[channelIndex]
				/* 20060107 */
				,core_bandwidth
				/*,bUsedBSAC */
				 ) ;

        processedChannels[channelIndex + i] = 1;
      }

      sbrChannelIndex += eleChannels;
    }
  }



  /* 
  Do the LFE channel, if available, or channels were syncState != ACTIVE for which we do up-sampling only.
  */
  for (channelIndex = 0 ; channelIndex < /*numChannels*/Chans ; channelIndex++)
  {
    if(ch_info[channelIndex].present && !processedChannels[channelIndex]){
      sbr_dec_bsac (timeData[channelIndex],
               timeDataOut[channelIndex],
               NULL,  /* no SBR...*/
               0,
               sbrChannelIndex,
#ifdef SBR_SCALABLE
               maxSfbFreqLine,
#endif
               bDownSampledSbr
			   ,drcFactorVector[channelIndex]
				/* 20060107 */
				,core_bandwidth
				/*,bUsedBSAC*/
			   );

      sbrChannelIndex++;
      processedChannels[channelIndex] = 1;
    }
  }

  for(i= 0; i < Chans; i++){
    int j;
    for(j=0; j < 32; j++){
      memcpy(drcFactorVector[i][j], drcFactorVector[i][j+32],64*sizeof(float));
    }
  }

  return err ;
}
#endif