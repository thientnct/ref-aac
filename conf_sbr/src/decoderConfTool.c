/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

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

#include "config.h"
#include "ct_sbrdecoder.h"

#include "adifdec.h"
#include "diagnose.h"
#include "audioout.h"
#include "bitinput.h"
#include "config.h"
#include "common.h"
#include "ct_sbrdecoderConfTool.h"
#include "ct_sbrConfToolDownSampler.h"
#include "audiofile_io.h"

#ifdef DRC
#include "drc.h" /* to control drc settings */
#endif
#include "gc.h"  /* to contol ssr settings */

#ifdef PARAMETRICSTEREO
int doPsConformance = 0;
#endif

static void             usage(char *s);
static int              parse_cmdline(int argc, char *argv[]);
static void             print_banner(char *cmd);

/* parameters */
extern int delay;
extern int fileIndex;

static char *bitstreaminputfile;

long            bno;
int             debug[256];
int             delay;
char            result[256];
char            resultQmf[256];
char            *audioinput_path;

static void
print_banner(char *cmd)
{
  PRINT (SE, "\n");
  PRINT (SE, "ISO/IEC 14496-4 MPEG-4 Conformance Verification\n");
  PRINT (SE, "%s  version %s\n", cmd, SBRCONF_VERSION);


#ifdef LOW_POWER_SBR
  PRINT (SE, "LOW POWER SBR\n");
#elif PARAMETRICSTEREO
  PRINT (SE, "HIGH QUALITY SBR + PARAMETRIC STEREO\n");
#else
  PRINT (SE, "HIGH QUALITY SBR\n");
#endif
  PRINT (SE, "\n");
}

static void usage(char *s)
{
  PRINT(SE, "usage: %s [options] <bitstream> <inputfile under test>\n", s);
  PRINT(SE, " options are:\n\t");
  PRINT(SE, " -h show this help\n\t");
  PRINT(SE, " -d delay (delay introduced by decoder under test; default: 0)\n\t");
#ifdef CT_SBR
  PRINT(SE, " -l level  (Level of the HE-AAC Profile decoder (2,3,4,5), 5 is default)\n\t");
  PRINT(SE, " -r (Forces the decoder to work in down sampled SBR mode, default is off)\n\t");
#ifdef PARAMETRICSTEREO
  PRINT(SE, " -c (Enables parametric stereo decoding for HE-AAC)\n");
#endif
#ifdef BSAC
  PRINT(SE, " -L (Enables scalable decoding for BSAC Extension)\n");
#endif
#endif
  myexit(0);
}

#ifdef BSAC
extern int core_bandwidth;
int basc_dec_layer;
AFILE *stream_ref = NULL;
audioheader audio_header_ref;
#endif

static int
parse_cmdline(int argc, char *argv[])
{
  char *cmd, *s;

  MC_Info *mip = &mc_info;

#ifdef	DRC
  Float param_drcHigh=0;
  Float param_drcLow=0;
  int param_drcTargetRefLevel=-1;
#endif
  int param_selectedProgram = -1;
  int param_ssrDecoderBand = -1;

  /* save cmd 
   * ARGBEGIN is so clever that it throws away argv[0] and 
   * increments the argv pointer
   */
  cmd = argv[0];
    
  print_banner(cmd);

  ARGBEGIN {

  default:
    PRINT(SE, "unknown option\n");
  case 'h':
    usage(cmd);
  case 'd':
    s = ARGF();
    delay = atoi(s);
    break;
#ifdef CT_SBR
  case 'l':
    s = ARGF();
    mip->HE_AAC_level = atoi(s);
    break;
  case 'r':
    mip->bDownSampledSbr = 1;
    break;
#ifdef PARAMETRICSTEREO
  case 'c':
#ifdef LOW_POWER_SBR
    PRINT(SE, "PS not available for LP-SBR\n");
    myexit("sbrEnablePS");
#endif
    mip->sbrEnablePS = 1;
    break;
#endif
#ifdef BSAC
  case 'L':
	s = ARGF();
    basc_dec_layer = atoi(s);
	break;
#endif
#endif
  case 'D':
    for (s = ARGF(); *s; s++) {
      debug[(int)(*s)] = 1;
      PRINT(SE, "debug option %c enabled\n", *s);
    }
  }
  ARGEND
      
    if(argc != 2) {
      usage(cmd);
    }

  printf("%s %s %s\n", GREP_STRING, argv[0], argv[1]);

  /* set decoder-global variables */
  decoder_selectProgram(param_selectedProgram);
  if (param_ssrDecoderBand>0) ssr_decoder_band = param_ssrDecoderBand;
  decoder_setupParameters(mc_info.sampling_rate_idx);
#ifdef DRC
  decoder_setDrc(param_drcHigh, param_drcLow, param_drcTargetRefLevel);
#endif


  bitstreaminputfile = argv[0];

  audioinput_path = argv[1];

#ifdef BSAC
  if(strstr(argv[0],"bs13")){
    fileIndex = 0;
  }
  else if(strstr(argv[0],"bs14")){
    fileIndex = 1;
  }
  else if(strstr(argv[0],"bs15")){
    fileIndex = 2;
  }
  else if(strstr(argv[0],"bs16")){
    fileIndex = 3;
  }
  else if(strstr(argv[0],"bs17")){
    fileIndex = 4;
  }
  else if(strstr(argv[0],"bs12")){
    fileIndex = 5;
  }
#else

  if(strstr(argv[0],"sbr_qmf_")){
    fileIndex = 0;
  }
  else if(strstr(argv[0],"sbr_e_")){
    fileIndex = 1;
  }
  else if(strstr(argv[0],"sbr_gh_")){
    fileIndex = 2;
  }
  else if(strstr(argv[0],"sbr_i_")){
    fileIndex = 3;
  }
  else if(strstr(argv[0],"sbr_s_")){
    fileIndex = 4;
  }
  else if(strstr(argv[0],"sbr_twi_")){
    fileIndex = 5;
  }
#endif

#ifdef PARAMETRICSTEREO
  else if(strstr(argv[0],"sbr_ps_00")){
    fileIndex = 6;
  }
  else if(strstr(argv[0],"sbr_ps_01")){
    fileIndex = 7;
  }
#ifndef NO_PS_CONF_DCOR2
  /* these conformance criteria have been proposed in 14496-4:2004/Amd.11:2006/DCOR 2
   * balloting for this hasn't closed at the time of this writing, so they are not official.
   * Hence they can be removed from the build by defining NO_PS_CONF_DCOR2 in this file and in decoderConfTool.c
   */
  else if(strstr(argv[0],"sbr_ps_02")){
    fileIndex = 8;
  }
  else if(strstr(argv[0],"sbr_ps_03")){
    fileIndex = 9;
  }
  else if(strstr(argv[0],"sbr_ps_04")){
    fileIndex = 10;
  }
  else if(strstr(argv[0],"sbr_ps_05")){
    fileIndex = 11;
  }
  else if(strstr(argv[0],"sbr_ps_06")){
    fileIndex = 12;
  }
#endif
#endif

#ifdef BSAC
  if(fileIndex == 5 || fileIndex == 0) {
	char *ss;
	ss= (char *)calloc(strlen(audioinput_path)+10, sizeof(char));
	strncpy(ss, audioinput_path, strlen(audioinput_path)-4);
	sprintf(ss, "%s.ref.wav", ss);
	open_inputaudio (&stream_ref,
                     ss,
                     &audio_header_ref);
	free(ss);
  }
#endif

  return argc;
}

#ifdef BSAC
#include "streamfile.h"
typedef struct{
    FRAME_DATA    *frameData;
}DEC_DATA;

#define MAX_AU 12
#define MAX_TRACKS 50
#define MAX_BITBUF 256000
#define MAX_TRACKS_PER_LAYER 50

int MP4Audio_ProbeFile (char *inFile)
{
  HANDLE_STREAMFILE file = NULL;
  file = StreamFileOpenRead ( inFile, FILETYPE_AUTO ) ; 
  if (file != NULL) 
  { 
    StreamFileClose (file) ; 
    return 1 ;
  } 
  else 
    return 0 ;
}

/* initialize decData, count and return streams to decode */
int frameDataInit(int  streamCount, /* in: number of ESs */
                  int  maxLayer,
                  DEC_CONF_DESCRIPTOR decConf [MAX_TRACKS],
                  DEC_DATA       *decData
                  )
{
  int layer, track;
  int decoderStreams;
  FRAME_DATA *fD;
  
  /*
    clear decData
  */
  memset(decData,sizeof(DEC_DATA),0);

  /*
    Alloc FrameData
  */
  if(!(decData->frameData = (FRAME_DATA *)calloc(1,sizeof(FRAME_DATA))))
    return -1;
  

  fD = decData->frameData;

  /* save original layer number for ER_BSAC */
  fD->bsacDecLayer=maxLayer;


  /* determine real number of layers and streams */
  if (maxLayer < 0) maxLayer = streamCount-1;
  tracksPerLayer(decConf, &maxLayer, &streamCount, fD->tracksInLayer, NULL /* not interested in ext payload track count */);
  fD->scalOutSelect = maxLayer;

  /* prepare EP decoder */
  for (layer=0,track=0; layer<(signed)fD->scalOutSelect+1; layer++) {
    AUDIO_SPECIFIC_CONFIG *asc = &decConf[track].audioSpecificConfig;
    int convertToEp = asc->epConfig.value;
    if (asc->epConfig.value == 2) convertToEp=0;
    if (asc->epConfig.value == 3) convertToEp=1;
    fD->ep_converter[layer] = EPconvert_create(fD->tracksInLayer[layer],
                                        asc->epConfig.value,
                                        &decConf[track].audioSpecificConfig.epSpecificConfig,
                                        NULL, /* dont write predfile */
                                        0, /* dont remove extension payload classes */
                                        convertToEp,
                                        NULL, NULL /* no ep info needed for output */);
    if (fD->ep_converter[layer]==NULL) return -1; /* TODO: free() */
    track += fD->tracksInLayer[layer];
  }

  /* now allocate internal structures */
  if(!(fD->od = (OBJECT_DESCRIPTOR *)calloc(1,sizeof(OBJECT_DESCRIPTOR)))) return -1;


  initObjDescr(fD->od);
  presetObjDescr(fD->od);

  decoderStreams = track = 0;
  for ( layer = 0; layer < (signed)fD->scalOutSelect+1; layer++ ) {
    int j;
    DEC_CONF_DESCRIPTOR *dC = &decConf[track];
    
    for (j=0; j<EPconvert_expectedOutputClasses(fD->ep_converter[layer]); j++, decoderStreams++) {
      fD->layer[decoderStreams].AULength = (unsigned int *)malloc(MAX_AU*sizeof(unsigned int));
      fD->layer[decoderStreams].AUPaddingBits = (unsigned int *)malloc(MAX_AU*sizeof(unsigned int));
      fD->layer[decoderStreams].bitBuf = BsAllocBuffer(MAX_BITBUF);
      
      initESDescr(&(fD->od->ESDescriptor[decoderStreams]));
      fD->od->ESDescriptor[decoderStreams]->DecConfigDescr = *dC;

      fD->layer[decoderStreams].sampleRate = dC->audioSpecificConfig.samplingFrequency.value ;
      fD->layer[decoderStreams].bitRate = dC->avgBitrate.value;
      /* TODO: this bitrate is wrong for ep1, but noone needs bitRate anyway */
    }
    track += fD->tracksInLayer[layer];
  }

  fD->od->streamCount.value = decoderStreams;

  return decoderStreams;
}

static int frameDataAddAccessUnit(DEC_DATA *decData,
                                  HANDLE_STREAM_AU au,
                                  int track)
{
  unsigned long unitSize, unitPad;
  HANDLE_BSBITBUFFER bb;
  HANDLE_BSBITSTREAM bs;
  HANDLE_BSBITBUFFER AUBuffer  = NULL;
  int idx = track;

  unitSize = (au->numBits+7)/8;
  unitPad = (unitSize*8) - au->numBits;

  bb = BsAllocPlainDirtyBuffer(au->data, /*au->numBits*/ unitSize<<3);
  bs = BsOpenBufferRead(bb);

  AUBuffer = decData->frameData->layer[idx].bitBuf ;
  if (AUBuffer!=0 ) {
    if ((BsBufferFreeBits(AUBuffer)) > (long)unitSize*8 ) {
      int auNumber = decData->frameData->layer[idx].NoAUInBuffer;
      BsGetBufferAppend(bs, AUBuffer, 1, unitSize*8);
      decData->frameData->layer[idx].AULength[auNumber]= unitSize*8;
      decData->frameData->layer[idx].AUPaddingBits[auNumber]= unitPad;
      decData->frameData->layer[idx].NoAUInBuffer++;/* each decoder must decrease this by the number of decoded AU */       
            /* each decoder must remove the first element in the list  by shifting the elements down */
      DebugPrintf(6,"AUbuffer %2i: %i AUs, last size %i, lastPad %i\n",idx, decData->frameData->layer[idx].NoAUInBuffer, decData->frameData->layer[idx].AULength[auNumber], decData->frameData->layer[idx].AUPaddingBits[auNumber]);
    } else {
      BsGetSkip(bs,unitSize*8);
      CommonWarning (" input buffer overflow for layer %d ; skipping next AU",idx);
    }
  } else {
    BsGetSkip(bs,unitSize*8);
  }
  BsCloseRemove (bs, 0);
  free(bb);
  return 0;
}

int GetBSACFrameLen(HANDLE_BSBITSTREAM stream)
{
	int len;
	unsigned long data_tmp;

	BsGetBit(stream, &len, 11);
	BsGetBit(stream, &data_tmp, 5);

	return len-2;
}

int GetBSACFrameLen_ext(HANDLE_BSBITSTREAM stream, int *nch)
{
	int len;
	int channel_idx[6] = {1, 2, 1, 2, 1, 2};
	unsigned long data_tmp;

	BsGetBit(stream, &len, 11);
	BsGetBit(stream, &data_tmp, 5);
	data_tmp = data_tmp>>1;
	if(data_tmp > 6) {
		printf("Error: wrong extension channel idx\n");
		return 0;
	}
	*nch = channel_idx[data_tmp];

	return len-2;
}
#endif

/* global, so that it can be closed in myexit() */
static HANDLE_BITINPUT bitstreaminput;
int
main(int argc, char *argv[])
{
  long avgBitrate = 0;
  long maxBitrate = 0;
  int bufferFullness = -1;
  int channelConfig;
  int remainingArgs;
  BUFFER_SIMULATION sim;
  int i;

  SBRDECODER sbrDec = {0};
  AFILE  *stream1;
  audioheader audio_header1;
  int samples_read1;
  int sample_counter = 0;
#ifdef BSAC
  int sample_counter_ref = 0;
#endif
  int firstHeader = 1;
  int storeIndex = 0;
  int numSamplesStored = 0;

  Float *storedData[Chans];
  float pcm_buffer1[Chans*2048];
  Float   *tmpDataReadFromFileBuffer[Chans];
  Float   *tmpDataRef[Chans];
  Float   *tmpData[Chans];
  Float   *tmpData2[Chans];
  Float   *tempDataDS[Chans];

#ifdef BSAC
  int nch;
  int BSACFrameLen;
  int BSACLen;
  int BSACSBRLen;
  int decodeLen;
  int BSACExtensionType;
  int first, ext_flag, chIdx;
  char buffer_empty;
  HANDLE_STREAMFILE stream = NULL;
  HANDLE_STREAMPROG prog = NULL;
  int  suitableTracks = 0;
  DEC_DATA           *decData = NULL;
  FRAME_DATA           *fd = NULL;
  HANDLE_BSBITSTREAM  bsac_bitStream, layer_stream;
  HANDLE_BSBITBUFFER  bsac_bitBuf;
  unsigned int i_ES;
  int AUno;
  SBRBITSTREAM ct_sbrBitStr_bsac;
  SBRBITSTREAM* sbrBitStr;
  sbrBitStr = &ct_sbrBitStr_bsac;
#endif

  for (i=0;i<Chans;i++) {
    tmpData[i] = (Float *)malloc(LN*sizeof(Float));
    tmpData2[i] = (Float *)malloc(LN*sizeof(Float));
    tempDataDS[i] = (Float *)malloc(LN*sizeof(Float));
    tmpDataRef[i] = (Float *)malloc(LN*sizeof(Float));
    tmpDataReadFromFileBuffer[i] = (Float *)malloc(2*LN*sizeof(Float));
#ifdef BSAC
	memset(tmpDataReadFromFileBuffer[i], 0, 2*LN*sizeof(Float));
#endif
    /* kaehleof 20050217: yuck... who has had this idea? */
    storedData[i] = (Float *)malloc(3000000*sizeof(*storedData[0]));
  }

#ifdef BSAC
  /* initilize bsac scalable decoding */
  basc_dec_layer = 48;
#endif

  decoder_init();
  remainingArgs = parse_cmdline(argc, argv);	    /* parse command line */

#ifndef BSAC
  bitstreaminput = bitinput_openFile(bitstreaminputfile,
                                     &prog_config,
                                     &mc_info,
                                     &channelConfig,
                                     &maxBitrate,
                                     &avgBitrate,
                                     &bufferFullness,
                                     (remainingArgs==1));

  decoder_setSBR_conformance(1);
  decoder_setupParameters(mc_info.sampling_rate_idx);
  if (bitstreaminput==NULL) myexit("error opening bitstream");
  if (remainingArgs==1) myexit(NULL);

  /* prepare buffer simulation */
  sim = create_bitBuffer_sim();
  {
    long bufferSize;
    double avgFrameLen = .0;

    getDecoderBufferSize(&prog_config, channelConfig, &bufferSize);
    if (NULL != bitinput_getCurrentADTSHeader(bitstreaminput)) avgBitrate = 0; /* get more accurate estimate from ADTS-Headers */

    if (avgBitrate>0) avgFrameLen = BITRATE2FRAMELEN(avgBitrate);
    else if (avgBitrate==0) avgFrameLen = 0; /* unknown, calculate on the fly */
    else if (avgBitrate<0) avgFrameLen = -1; /* variable bitrate */

    if (bufferFullness == 0x7fffffff) { /* also indicates variable bitrate */
      bufferFullness = 0;
      avgFrameLen = -1;
    }

    bitBuffer_sim_setFrameLen(sim, avgFrameLen, bufferSize);
    if (maxBitrate>0) bitBuffer_sim_testPeakBitrate(sim, maxBitrate);
  }
#else
  if(MP4Audio_ProbeFile(bitstreaminputfile)) {
	  stream = StreamFileOpenRead( bitstreaminputfile, FILETYPE_AUTO );
	  decData = (DEC_DATA *)calloc(1,sizeof(DEC_DATA));
	  prog = StreamFileGetProgram( stream, /*programNr*/0 );
	  suitableTracks = prog->trackCount;
	  suitableTracks=frameDataInit(suitableTracks,
                               -1,/*maxLayer,*/
                               prog->decoderConfig,
                               decData);
  }
  else{
	CommonWarning("error decoding audio file %s",bitstreaminputfile);
  }

  /* initialize mc_info */
  nch = decData->frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
  for(i = 0; i < nch; i ++)
	mc_info.ch_info[i].present = 1;
  mc_info.sampling_rate = decData->frameData->layer[0].sampleRate;
#endif



  /* main loop */
  for(bno = 0;; bno++) {
#ifndef BSAC
    SBRBITSTREAM *sbrBitStr = NULL;
#else
	int tracksForDecoder;
#endif
    Float **output_data;
    int output_blockLen;
    int err = bitinput_getNextBlock(bitstreaminput);
#ifdef BSAC
	err = 0;
#endif
    if (err == -2) break; /* eof */
    if (err) myexit("error reading next block");

#ifndef BSAC
    dec(&output_data, &output_blockLen);

    /* look for SBR data */
    if (mc_info.sbrPresentFlag != 0) {
      if ((sbrBitStr = (SBRBITSTREAM*)decoder_getSBRbitstream())!=NULL)
        if (sbrBitStr->NrElements == 0) sbrBitStr=NULL;
    }
#else
	mc_info.upsamplingFactor = 2.f;

	/* allocate memory: output_data */
	output_data = (Float **)malloc(Chans*sizeof(Float*));
	for(i =0 ; i < Chans; i ++)
		output_data[i] = (Float *)malloc(LN2*sizeof(Float));

	{
      HANDLE_STREAM_AU inputAUs[MAX_TRACKS_PER_LAYER];
      HANDLE_STREAM_AU decoderAUs[MAX_TRACKS_PER_LAYER];
      int layer, track;
      int firstTrackInLayer, resultTracksInLayer;
      int err = 0;
      for (track=0; track<MAX_TRACKS_PER_LAYER; track++) {
        inputAUs[track]=StreamFileAllocateAU(0);
        decoderAUs[track]=StreamFileAllocateAU(0);
      }
      firstTrackInLayer = tracksForDecoder = 0;
      /* go through each decoded layer */
      for (layer=0; layer<(signed)decData->frameData->scalOutSelect+1; layer++) {
        /* go through all frames in the superframe */
        resultTracksInLayer = EPconvert_expectedOutputClasses(decData->frameData->ep_converter[layer]);
        while ((signed)decData->frameData->layer[tracksForDecoder].NoAUInBuffer<StreamAUsPerFrame(prog, firstTrackInLayer)) {
          int epconv_input_tracks;
          if (EPconvert_numFramesBuffered(decData->frameData->ep_converter[layer])) {
            epconv_input_tracks=0;
          } else {
            epconv_input_tracks=decData->frameData->tracksInLayer[layer];
          }

          /* go through all tracks in this layer */
          for (track=0; track<epconv_input_tracks; track++) {
            err = StreamGetAccessUnit( prog, firstTrackInLayer+track, inputAUs[track] );
            if ( err ) break;
          }
          if ( err ) break;

          /* decode with epTool if necessary */
          err = EPconvert_processAUs(decData->frameData->ep_converter[layer],
                                     inputAUs, epconv_input_tracks,
                                     decoderAUs, MAX_TRACKS_PER_LAYER);
          if (resultTracksInLayer!=err) {
            CommonWarning("Expected %i AUs after ep-conversion, but got %i AUs", resultTracksInLayer, err);
            /*resultTracksInLayer=err;*/
          }
          err=0;

          /* save the tracks */
          for (track=0; track<resultTracksInLayer; track++) {
            frameDataAddAccessUnit(decData, decoderAUs[track], tracksForDecoder+track);
          }
		  
        }
        if (err) break;
        firstTrackInLayer += decData->frameData->tracksInLayer[layer];
        tracksForDecoder += resultTracksInLayer;
      }
      for (track=0; track<MAX_TRACKS_PER_LAYER; track++) {
        if (inputAUs[track]) StreamFileFreeAU(inputAUs[track]);
        if (decoderAUs[track]) StreamFileFreeAU(decoderAUs[track]);
      }
      buffer_empty=1;
      for (track=0; track<(signed)decData->frameData->od->streamCount.value; track++) {
        if (decData->frameData->layer[track].NoAUInBuffer>0) buffer_empty=0;
      }
      if (buffer_empty) break;
    }

	/* combine all ES data into one bitstream */
	bsac_bitBuf = BsAllocBuffer(256000);
	fd = decData->frameData;
	for(i_ES=0; i_ES < fd->od->streamCount.value; i_ES++) {

	 AUno =  fd->layer[i_ES].NoAUInBuffer;
	 /* open the first layer bitbuffer  to read */
	 if (AUno<=0) {
		CommonWarning("bsacAUDecode : %d-th ES has no Access unit; skiping frame", i_ES);
		return;
	 }

	 layer_stream= BsOpenBufferRead(fd->layer[i_ES].bitBuf);
	 BsManipulateSetNumByte( (BsBufferNumBit(fd->layer[i_ES].bitBuf)+7)/8, layer_stream);
	 if (fd->layer[i_ES].AULength[0] > 0) 
		BsGetBufferAppend(layer_stream, bsac_bitBuf, 1, fd->layer[i_ES].AULength[0]);
	 BsCloseRemove(layer_stream,1);
	 fd->layer[i_ES].NoAUInBuffer--;
	}

	bsac_bitStream = BsOpenBufferRead(bsac_bitBuf); 
	BsManipulateSetNumByte((BsBufferNumBit(bsac_bitBuf)+7)/8, bsac_bitStream);

	/* read BSAC frame length */
	decodeLen = BSACFrameLen = GetBSACFrameLen(bsac_bitStream);
	first = ext_flag = 0;
	sbrBitStr->NrElementsCore = 0;
	while(decodeLen) {
		unsigned long data_tmp;
		/* read BSAC extended block length */
		if(first) {
			BsGetBit(bsac_bitStream, &data_tmp, 4);
			BSACExtensionType = (int)data_tmp;
			BSACFrameLen = GetBSACFrameLen_ext(bsac_bitStream, &nch);
			decodeLen -= 2;
			GetBSACExtension(bsac_bitStream, BSACFrameLen, &BSACLen, &BSACSBRLen, &BSACExtensionType);
			if(nch == 1)
			  sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].ElementID = SBR_ID_SCE;
			else if(nch == 2)
			  sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].ElementID = SBR_ID_CPE;
			sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].chIndx = chIdx;
			mc_info.ch_info[chIdx].present = 1;
			if(nch == 2)
				mc_info.ch_info[chIdx+1].present = 1;
			chIdx += nch;
			ext_flag = 1;
			decodeLen -= (BSACLen+1+BSACSBRLen);
		}
		else {
			/* get SBR position and length */
			GetBSACPosInfo(bsac_bitStream, decodeLen, &BSACLen, &BSACSBRLen, &BSACExtensionType, nch, mc_info.sampling_rate);
			/* set channel type */
			if(nch == 1)
			  sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].ElementID = SBR_ID_SCE;
			else if(nch == 2)
			  sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].ElementID = SBR_ID_CPE;
			sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].chIndx = 0;
			chIdx = nch;
			first = 1;
			decodeLen -= (BSACLen+BSACSBRLen);
		}

		/* look for SBR data */
		/* copy SBR data from BSAC bitstream */
		if (/*mc_info.sbrPresentFlag || */BSACSBRLen != 0) {
			if(BSACExtensionType==0 || BSACExtensionType==14)
				sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].ExtensionType = SBR_EXTENSION;
			else if(BSACExtensionType==1 || BSACExtensionType==13)
				sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].ExtensionType = SBR_EXTENSION_CRC;
			if(BSACSBRLen >= 15)
				BSACSBRLen -= 1;
			sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].Payload = BSACSBRLen;
			BsGetBit( bsac_bitStream, &sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].Data[0], 4);
			for(i = 1; i < BSACSBRLen; i ++)
				BsGetBit( bsac_bitStream, &sbrBitStr->sbrElement[sbrBitStr->NrElementsCore].Data[i], 8);
			sbrBitStr->NrElementsCore ++;
		}
		else
			sbrBitStr=NULL;

		/* byte alignment */
		if(ext_flag)
			BsGetBit(bsac_bitStream, &data_tmp, 4);
	}

	sbrBitStr->NrElements = sbrBitStr->NrElementsCore;
#endif

    /* do SBR conformance */
    if (sbrBitStr!=NULL) {
      if (bno == 0) {

        Twiddle twiddle, *ptrTwiddle=NULL;
        FILE *twiddlefile;
#if !defined LOW_POWER_SBR
        if( (twiddlefile = fopen("twiddle.dat", "rb")) ) {
          double fi, beta;
          unsigned int count;
          count  = fread(&fi,   1, sizeof(double), twiddlefile);
          count += fread(&beta, 1, sizeof(double), twiddlefile);
          if(mc_info.bDownSampledSbr) {
            count += fread(&fi,   1, sizeof(double), twiddlefile);
            count += fread(&beta, 1, sizeof(double), twiddlefile);
          }
          if(count == 2 * sizeof(double) * (mc_info.bDownSampledSbr + 1)) {
            ptrTwiddle = &twiddle;
            ptrTwiddle->fi   = fi;
            ptrTwiddle->beta = beta;
            PRINT(SE, "using fi = %f, beta = %f\n", fi, beta);
          }
          else {
            if (fileIndex != 5) {
              PRINT(SE, "warning: ignoring short twiddle-file\n");
            }
          }
          fclose(twiddlefile);
        }
#endif

        sbrDec = openSBR(mc_info.sampling_rate, mc_info.bDownSampledSbr, ptrTwiddle);

        if (sbrDec==NULL) {
          myexit("openSBR failed!\n");
        }

#if !defined LOW_POWER_SBR
        if (fileIndex == 5) {
          FILE *twiddlefile = NULL;
          int headerAvailable = 0;
          SBR_ERROR err = SBRDEC_OK ;

          open_inputaudio (&stream1,
                           audioinput_path,
                           &audio_header1);

          if (audio_header1.samplefreq != mc_info.sampling_rate * (2 - mc_info.bDownSampledSbr)) {
            myexit("sampling rates of input files differ!\n");
          }

          samples_read1 = get_audiosample ( (float*) storedData[0],
                                            stream1,
                                            audio_header1.channels,
                                            0,
                                            16384 * (2 - mc_info.bDownSampledSbr));

          close_inputaudio (stream1);

          err = readAndDecodeSBRConfTool(sbrDec,
                                         sbrBitStr,
                                         mc_info.bDownSampledSbr,
                                         mc_info.ch_info,
                                         &headerAvailable);

          twiddle = identifyQmfBank( (float*) storedData[0],
                                     audio_header1.channels,
                                     mc_info.bDownSampledSbr,
#ifndef BSAC
                                     getSBRstartBand (),
#else
									 30,
#endif
                                     getSBRstopBand  () );

          closeSBR (sbrDec);

          sbrDec = openSBR(mc_info.sampling_rate,
                           mc_info.bDownSampledSbr,
                           &twiddle);

          if (sbrDec == NULL) {
            myexit("openSBR failed!\n");
          }

          if ((twiddlefile = fopen("twiddle.dat", "rb+")) || (twiddlefile = fopen("twiddle.dat", "wb+"))) {
            if (mc_info.bDownSampledSbr) {
              fseek(twiddlefile, 2 * sizeof(double), SEEK_SET);
            }
            fwrite(&twiddle.fi,   1, sizeof(double), twiddlefile);
            fwrite(&twiddle.beta, 1, sizeof(double), twiddlefile);
            fclose(twiddlefile);
          } else {
            PRINT(SE, "couldn't open or create twiddle.dat. Maybe permission denied?");
          }
        }
#endif

        open_inputaudio (&stream1,
                         audioinput_path,
                         &audio_header1);
        sample_counter = delay;
#ifdef BSAC
		sample_counter_ref = delay;
#endif

        if (audio_header1.samplefreq != mc_info.sampling_rate * (2 - mc_info.bDownSampledSbr)) {
          myexit("sampling rates of input files differ!\n");
        }

#ifdef PARAMETRICSTEREO
        /* we should compare both channels if
           - the stream has only one channel (sce)
           - the inputfile has two channels
           - the -c switch was given
        */
        {
          int n_channels = 0;
          for (i=0; i < Chans; i++) {
            if (mc_info.ch_info[i].present && mc_info.ch_info[i].cpe == 0) {
              n_channels++;
            }
          }
          if (n_channels == 1 && audio_header1.channels == 2 && mc_info.sbrEnablePS) {
            doPsConformance = 1;
          }
        }
#endif
      }

      samples_read1 = get_audiosample (pcm_buffer1,
                                       stream1,
                                       audio_header1.channels,
                                       sample_counter,
                                       (1024*(2 - mc_info.bDownSampledSbr)));
      if (samples_read1==0) {
        /* end of input file -> wait delay more samples */
        if (delay > 0) {
          int i;
          delay -= 1024*(2 - mc_info.bDownSampledSbr);
          for (i = 0; i < Chans * 2048; i++)
            pcm_buffer1[i] = 0;
        }
        else {
          myexit("Warning! Short inputfile\n");
        }
      }
      sample_counter += (1024*(2 - mc_info.bDownSampledSbr));

      /* Copy to tmpData*/
      {
        int channel = 0;
        for (i=0; i<Chans; i++) {
          int kk;
          if (!(mc_info.ch_info[i].present))
            continue;

          for(kk = 0; kk < (1024*(2 - mc_info.bDownSampledSbr)); kk++){
            tmpDataReadFromFileBuffer [i][kk] = tmpDataReadFromFileBuffer [i][kk+(1024*(2 - mc_info.bDownSampledSbr))];
            tmpData[i][kk] = pcm_buffer1 [(kk * audio_header1.channels) + channel];
            tmpDataReadFromFileBuffer [i][kk+(1024*(2 - mc_info.bDownSampledSbr))] = tmpData[i][kk];
          }
          channel++;
#ifdef PARAMETRICSTEREO
          if (doPsConformance) {
            if (i >= Chans - 1) myexit("Oh no! Ps was in the last channel! That won't work! Giving up\n");
            for(kk = 0; kk < (1024*(2 - mc_info.bDownSampledSbr)); kk++){
              tmpDataReadFromFileBuffer [i + 1][kk] = tmpDataReadFromFileBuffer [i + 1][kk+(1024*(2 - mc_info.bDownSampledSbr))];
              tmpData[i + 1][kk] = pcm_buffer1 [(kk * audio_header1.channels) + channel];
              tmpDataReadFromFileBuffer [i + 1][kk+(1024*(2 - mc_info.bDownSampledSbr))] = tmpData[i + 1][kk];
            }
            /* no more channels -> go on */
            break;
          }
#endif
        }
#ifdef BSAC
	  if(stream_ref) {
		  samples_read1 = get_audiosample ( pcm_buffer1,
                                    stream_ref,
                                    1,
                                    sample_counter_ref,
                                    1024);
		  if (samples_read1==0) {
			/* end of input file -> wait delay more samples */
			if (delay > 0) {
			  int i;
			  delay -= 1024;
			  for (i = 0; i < Chans * 1024; i++)
				pcm_buffer1[i] = 0;
			}
			else {
			  myexit("Warning! Short reference inputfile\n"); 
			}
		  }
		  for(i = 0; i < 1; i ++) {
			int k;
			for(k = 0; k < 1024; k ++)
			  output_data[i][k] = (double) pcm_buffer1[i*1024+k];
		  }
		  sample_counter_ref += 1024;
	  }
#endif
      }

      for (i=0; i<Chans; i++) {
        int kk;
        if (!(mc_info.ch_info[i].present))
          continue;
        for (kk=0; kk<LN2; kk++)
          tempDataDS[i][kk] = output_data[i][kk];
      }

      if (fileIndex == 0 || fileIndex == 5) {
        int frameSize = (mc_info.bDownSampledSbr ? 1024:2048);
        /* qmf-test is different */
#ifndef BSAC
        if (applySBR(sbrDec,
                     sbrBitStr,
                     output_data,
                     tmpDataRef,
                     mc_info.bDownSampledSbr,
#ifdef PARAMETRICSTEREO
                     mc_info.sbrEnablePS,
#endif
                     mc_info.ch_info) != SBRDEC_OK)
#else
		if (applySBR_BSAC(sbrDec,
                     sbrBitStr,
                     output_data,
                     tmpDataRef,
                     mc_info.bDownSampledSbr,
#ifdef PARAMETRICSTEREO
                     mc_info.sbrEnablePS,
#endif
                     mc_info.ch_info, core_bandwidth) != SBRDEC_OK)
#endif
          myexit("invalid sbr bitstream");
        calculateDiffandRms(tmpData, tmpDataRef, mc_info.ch_info,frameSize);
      } else {
        int headerAvailable = 0;
        SBR_ERROR err = SBRDEC_OK ;

        err = readAndDecodeSBRConfTool(sbrDec,
                                       sbrBitStr,
                                       mc_info.bDownSampledSbr,
                                       mc_info.ch_info,
                                       &headerAvailable);

        /* if header available use stored data*/
        if(headerAvailable){
          if(firstHeader){
            firstHeader = 0;
            numSamplesStored = storeIndex;
            storeIndex = 0;
          }

          err = applySBRConfTool(sbrDec,
                                 sbrBitStr,
                                 storedData,
                                 tmpDataRef,
                                 mc_info.bDownSampledSbr,
                                 mc_info.ch_info,
#ifdef PARAMETRICSTEREO
                                 mc_info.sbrEnablePS,
#endif
                                 storeIndex);

          if(storeIndex > 2*1024 && storeIndex < numSamplesStored - 2*1024){
            int frameSize = (mc_info.bDownSampledSbr ? 1024:2048);
            calculateDiffandRms(tmpData, tmpDataRef, mc_info.ch_info, frameSize);
          }

          storeIndex += 1024;

        } else {
          /* Down sample signal*/

          {
            int i;

            for (i=0; i<Chans; i++) {
              if (!(mc_info.ch_info[i].present))
                continue;
              downSampleSbr(tmpData, (mc_info.upsamplingFactor == 2.0f), i);
              memcpy(storedData[i] + storeIndex,tmpData[i],1024*sizeof(double));
            }

            mc_info.upsamplingFactor = 1;
          }

          /* Let's upsample the signal again, to see how much it differs, from the one we just had.*/
          if(storeIndex > 2*1024){
            int frameSize = (mc_info.bDownSampledSbr ? 1024:2048);

            err = applySBRConfTool(sbrDec,
                                   sbrBitStr,
                                   storedData,
                                   tmpData2,
                                   mc_info.bDownSampledSbr,
                                   mc_info.ch_info,
#ifdef PARAMETRICSTEREO
                                   mc_info.sbrEnablePS,
#endif
                                   (storeIndex-1024));

            calculateDiffandRmsQmf(tmpData2, tmpDataReadFromFileBuffer, mc_info.ch_info,frameSize);
          }

          storeIndex += 1024;
         }
      }
    }

#ifndef BSAC
    /* do buffer simulation */
    {
      const ADTS_Header *tmp_head = bitinput_getCurrentADTSHeader(bitstreaminput);
      update_bitBuffer_sim(sim, bitinput_resetCounter(bitstreaminput)<<3, tmp_head);
    }
    /* initial buffer fullness */
    if ((bno==0)&&(bufferFullness!=-1)) {
      bitBuffer_sim_adjustFullness(sim, bufferFullness);
    }
    check_bitBuffer_sim(sim, "", debug['b'], debug['B'], debug['R']);
#else
	for(i=0; i<Chans; i++)
	  free(output_data[i]);
	free(output_data);
#endif
  }

#ifdef BSAC
  if (stream_ref)
	AFclose(stream_ref);
#endif

  myexit(NULL);
  return (EXIT_SUCCESS);
}


void
myexit(char *s)
{
  bitinput_close(bitstreaminput);
  decoder_free();
  printf("%s\n", resultQmf);
  printf("%s\n", result);
  if (s) PRINT(SE,"\n%s\n", s);
  exit(s ? 1 : 0);
}
