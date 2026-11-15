/*
  "This software module was originally developed by 

  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 

  and edited by

  Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
  Thomas Schaefer (Fraunhofer Gesellschaft IIS)
  Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

  in the course of 
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
  Copyright(c)1996, 1997, 1998, 2000.
*/

#include "portio.h"
#include "statistics.h"
#include "resorg.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "escHandling.h"

#include "aiff_support.h"

#ifdef WIN32
#include <io.h>
#endif

#define PATH_LEN           128
#define COMMENT_LENGTH    1024  /* max nr of comment characters in predefinition file */

static FILE *bin;
static FILE *bit;           /* file pointer to the EP bit file              */
static FILE *inf;           /* pointer to the info file                     */
static char outfd[Chans];
#ifdef PCM_OUTPUT
static char output_path[PATH_LEN];
#endif
static  int               useAIFF         = 0;
static  unsigned short    AIFF_channels   = 1;
static  unsigned short    AIFF_wordsize   = 16;
static  unsigned long     AIFF_frames     = 0;
static  unsigned long     AIFF_sampleRate = 48000;
static  MC_Info*          AIFF_MC_Info    = 0;

static ESC_INSTANCE_DATA  escInstanceData;
static byte               lastWriteByte  = 0x0;  /* stores last byte of the buffer before actual buffer   */
static unsigned int       numWrittenBits = 0;    /* number of written bits to the file                    */

void usage(char *s)
{
#ifdef PCM_OUTPUT
    PRINT(SE, "usage:  %s [options] chan_file trans_file pcm_file\n", s);
#else
    PRINT(SE, "usage:  %s [options] <in_file> <out_file>\n", s);
#endif /*PCM_OUTPUT*/
    PRINT(SE, "options are:\n");
#ifdef PCM_OUTPUT
    PRINT(SE, " -a create AIFF output files\n");
#endif /*PCM_OUTPUT*/
    PRINT(SE, " -i <prog_tag> (ADIF header present)\n");
    PRINT(SE, " -p <profile>  (Main or LC, Main is default)\n");
    PRINT(SE, " -s <sampling_frequency>, Hz (48000 is default)\n");
    PRINT(SE, " -C channel configuration\n");
    PRINT(SE, " -t tns usage flag (0/1, 1 is default)\n");
    PRINT(SE, " -L switch to Low Delay\n");
    PRINT(SE, " -F frameLengthFlag: use frame length 960 (default: 1024)\n");
    PRINT(SE, "                     for Low Delay:   480 (default:  512)\n");
    PRINT(SE, " -l length_of_longest_codeword within bitstream (not standard)\n");
    PRINT(SE, " -r <mode> HCR within spectral data, length_of_longest_codeword and \n");
    PRINT(SE, "                                     length_of_spectral_data within bitstream\n");
    PRINT(SE, "    modi: 0 - raw (not standard)\n");
    PRINT(SE, "          1 - codeword pre-sorting (not standard)\n");
    PRINT(SE, "          2 - consecutive HCR (not standard)\n");
    PRINT(SE, "          3 - codeword pre-sorting and consecutive HCR\n");
    PRINT(SE, " -c virtual codebooks are used to encode codebook 11 within section data\n");
    PRINT(SE, " -f <mode> error resilient coding of scalefactor data, modi:\n");
    PRINT(SE, " \t 0 - pure aac bitstream\n");
    PRINT(SE, " \t 1 - length of sf data in bitstream (not standard)\n");
    PRINT(SE, " \t 2 - 1 + rvlc coding of sf data (not standard)\n");
    PRINT(SE, " \t 3 - 2 + additional bit allowing enhanced concealment\n");
    PRINT(SE, " -D[a-Z] (enable debugging printouts)\n");
    myexit(0);
}

void initio ( int   argc, 
              char* argv[] )
{
    int            i;
    int            j; 
    int*           p;
    char*          cmd; 
    char*          s;
    char           path[PATH_LEN];
#ifdef PCM_OUTPUT
    unsigned char  numOfFiles = 3; /* number of files in the command line */
#else
    unsigned char  numOfFiles = 2;
#endif /*PCM_OUTPUT*/
    unsigned short flag;
    short          tnsUsed = 1;
    short          channelConfiguration = -1;
    unsigned short esc;
    unsigned short instanceOfEsc;

    /* set defaults */
    adif_header_present = 0;
    current_program = -1;
    low_delay = 0;
    frame_length_flag = 0;
    default_config = 1;
    mc_info.profile = Main_Profile;
    mc_info.sampling_rate_idx = Fs_48;
    ResilienceInit();

    /* save cmd 
     * ARGBEGIN is so clever that it throws away argv[0] and 
     * increments the argv pointer
     */
    cmd = argv[0];

    ARGBEGIN {
    default:
        PRINT(SE, "unknown option\n");
        usage(cmd);
    case 'a':
        useAIFF = 1;
        break;
    case 'i':
        adif_header_present = 1;
        s = ARGF();
        current_program = atoi(s);
        if (current_program > ((1<<LEN_NUM_PCE)-1)) {
            PRINT(SE, "Invalid program: %d\n",  current_program);
            current_program = -1;
        }
        break;
    case 'p':
        s = ARGF();
        p = &mc_info.profile;
        if ((!strncmp(s, "Main", 4)) || (!strncmp(s, "main", 4))) {
            *p = Main_Profile; 
            break;
        }
        if ((!strncmp(s, "LC", 2)) || (!strncmp(s, "lc", 2))) {
            *p = LC_Profile; 
            break;
        }
        PRINT(SE, "Unsupported profile %s\n", s);
        myexit("profile");
        break;
    case 's':
        j = atoi(ARGF());
        for ( i = 0; i < (1 << LEN_SAMP_IDX ); i++ ) 
          {
            if ( j == samp_rate_info[i].samp_rate )
              break;
          }
        if (i == (1<<LEN_SAMP_IDX)) {
            PRINT(SE, "Unsupported sampling frequency %d\n", j);
            myexit("samling_rate_idx");
        }
        mc_info.sampling_rate_idx = i;
        AIFF_sampleRate = samp_rate_info[i].samp_rate;
        break;
    case 'C':
      channelConfiguration = (unsigned short) atoi(ARGF());
      break;
    case 'L':
        low_delay = 1;
        break;
    case 'F':
        frame_length_flag = 1;
        break;
    case 'r': {
      flag = (unsigned short) atoi(ARGF());
      SetReorderSpecFlag(flag);
      SetLongestCWAvailable(0); /* set only longest cw available outside reorder routine to false */
      break;
    }
    case 'f': {
      flag = atoi(ARGF());
      SetResilienceScfFlag(flag); /* for possible modes look in SetResilienceScfFlag() */
    }
    break;
    case 'l' : 
      SetLongestCWAvailable(1); /* no reordering, but longest cw in output bitstream available */
      break;
    case 'c' :
      SetTranscodeSectionDataFlag();
      break;
    case 'D' :
      for (s = ARGF(); *s; s++) {
        debug[(int)(*s)] = 1;
        PRINT(SE, "debug option %c enabled\n", *s);
      }
      break;
    case 't' :
       tnsUsed = (unsigned short) atoi(ARGF());
    } 
    ARGEND
      
    if ( argc != numOfFiles ) {
      usage ( cmd );
    }
    
    if ( tnsUsed > 1 ) {
      myexit("only 0 (tns not used) or 1 (tns used) are valid");
    }
    if (channelConfiguration < 0 ) {
      myexit("channel configuration is missing");
    }
    if (channelConfiguration < 1 || channelConfiguration > 2 ) {
      myexit("only mono (1) and stereo (2) are supported");
    }
    switch (channelConfiguration) {
    case 1:
      for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
        for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
          escInstanceData.escInstance[instanceOfEsc][esc].escInstanceUsed = 0;
          if (esc != 0 && instanceOfEsc == 0 ) {
            escInstanceData.escInstance[instanceOfEsc][esc].escInstanceUsed = 1;
          }
        }
      }
      break;
    case 2:
      for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
        for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
          escInstanceData.escInstance[instanceOfEsc][esc].escInstanceUsed = 0;
          if ( ( esc == 0 && instanceOfEsc == 0 ) || ( esc > 0 ) ) {
              escInstanceData.escInstance[instanceOfEsc][esc].escInstanceUsed = 1;
          }
        }
      }
      break;
    default:
      myexit("this case should not occur");
    }
    if ( ! GetResilienceScfFlag ( ) ) {
      for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
        escInstanceData.escInstance[instanceOfEsc][2].escInstanceUsed = 0;
      }
    }
    if ( ! tnsUsed ) {
      for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
        escInstanceData.escInstance[instanceOfEsc][3].escInstanceUsed = 0;
      }
    }
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
        PRINT(SE,"%i ",escInstanceData.escInstance[instanceOfEsc][esc].escInstanceUsed);
      }
      PRINT(SE,"\n");
    }

    bin = fopen(argv[0], "rb");
    if ( bin == 0 ) {
      PRINT(SE, "can't open %s\n", argv[0]);
      myexit("open");
    }
    if ( strlen(argv[1]) + strlen("_??inst.bit" ) + 1 > PATH_LEN) {
      PRINT(SE, "output path too long %s\n", argv[1]);
      myexit ( "create" );
    }
    sprintf (path, "%s", argv[1]);
    bit = OpenEPFile(strcat(path, ".bit"), "wb");
    sprintf (path, "%s", argv[1]);
    inf = OpenEPFile(strcat(path, ".inf"), "w");

#ifdef PCM_OUTPUT
    if ( strlen(argv[2]) + strlen(".pcm" ) + 1 > PATH_LEN) {
      PRINT(SE, "output path too long %s\n", argv[2]);
      myexit ( "create" );
    }
    strcpy ( output_path, argv[2] );
#endif    
}

#ifdef PCM_OUTPUT
static void write_fext(Ch_Info *cip, char c, int n)
{
        cip->fext = (char *)mal1(5);
        sprintf(cip->fext, "_%c%02d", c, n);
}

static int open_output_files(MC_Info *mip)
{
    int i, j;
    char file[PATH_LEN];

    if (useAIFF)
        AIFF_MC_Info = mip;

    j=0;
    /* if FCenter == 1 (center speaker present)
     *  position    index   extension
     *  left        1       01
     *  center      0       00
     *  right       2       02
     * if FCenter == 0 (center speaker absent)
     *  position    index   extension
     *  left        0       01
     *  right       1       02
      */
    for (i=((FCenter==1) ? 0 : 1); i<FChans; i++) {
        write_fext(&mip->ch_info[j], 'f', i);
        j++;
    }
    for (i=0; i<SChans; i++) {
        write_fext(&mip->ch_info[j], 's', i);
        j++;
    }
    /* if BFCenter == 1 (center speaker present)
     *  position    index   extension
     *  left        3       00
     *  center      5       02
     *  right       4       01
     * if BCenter == 0 (center speaker absent)
     *  position    index   extension
     *  left        3       00
     *  right       4       01
      */
    for (i=0; i<BChans; i++) {
        write_fext(&mip->ch_info[j], 'b', i);
        j++;
    }
    for (i=0; i<LChans; i++) {
        write_fext(&mip->ch_info[j], 'l', i);
        j++;
    }
    
    for (i=0; i<Chans; i++) {
        if (!(mip->ch_info[i].present))
            continue;
       
        strcpy(file, output_path);
        strcat(file, mip->ch_info[i].fext);
        if (!useAIFF)
            strcat(file, ".pcm");
        else
            strcat(file, ".aif");

        if (debug['o'])
            PRINT(SE, "opening file %s at position %d\n", file, i);
            
#ifdef WIN32
        outfd[i] = _open (file, _O_CREAT | _O_TRUNC | _O_BINARY | _O_WRONLY, _S_IWRITE);
#else
        outfd[i] = creat(file, 0666);
#endif

        if(outfd[i] < 0) {
            PRINT(SE, "cant create %s\n", file);
            myexit("create");
        }
        if ( useAIFF ) {
            /* write dummy header and position for audio data */
            if ( WriteAIFFHeader(outfd[i], AIFF_channels, AIFF_frames, AIFF_wordsize, AIFF_sampleRate) != 0 ) {
                PRINT( SE, "cannot write AIFF header for %s\n", file );
                myexit( "aiff-header" );
            }
        }
    }
    return(1);
}
#endif /*PCM_OUTPUT*/

void fmtchan(char *p, float *data, int stride)
{
    int i, c;
    float s;

    for(i=0; i<LN2; i++) {
        s = *data++;
        if(s < 0) {
            s -= .5;
            if(s < -0x7fff)
                s = (float) -0x7fff;
        } else {
            s += .5;
            if(s > 0x7fff)
                s = (float) 0x7fff;
        }
        c = (int) s;

    p[0] = (c >> 8) & 0xff;
    p[1] = c & 0xff;

        p += stride;
    }
}

#ifdef PCM_OUTPUT
void writeout(Float **data, MC_Info *mip)
{
    int i, stride, size;
    char obuf[LN2*2];
    static int opened = 0;

    if (!opened) {
        open_output_files(mip);
        opened=1;
    }

    stride=2;
    size = sizeof(obuf);
    for (i=0; i<Chans; i++) {
        if (!(mip->ch_info[i].present))
            continue;

        fmtchan(obuf, data[i], stride);
        if(write(outfd[i], obuf, size) != size) {
            PRINT(SE, "write error\n");
            myexit("write");
        }
        if ( useAIFF )
           AIFF_frames += LN2;
    }
}
#endif /*PCM_OUTPUT*/

int getbyte(void)
{
    int c;
    static unsigned short errorCounter = 0;

    c = getc(bin);
    if(c < 0) {
      errorCounter++;
      if ( errorCounter > 3 ) { /* this guarantees, hat the transoder works till the end of the aac bitstream */
        myexit((debug['n']) ? 
               "getbyte: End of Bitstream" : 0);
      }
      c = 0;
    }
    return c;
}

void myexit ( const char* s )
{
  if ( useAIFF )
    closefiles( AIFF_MC_Info );
  if (debug['v'])
    PRINT(SE,"%lx %d\n", cword, nbits);
  if (s)
    PRINT(SE,"\nmyexit: %s\n", s);
  if ( !s && debug['S'])
    StatisticsPrint ();
  exit(s ? 1 : 0);
}

void closefiles( MC_Info *mip )
{
   int i;
   if ( mip && useAIFF )
   {
      for ( i = 0; i < Chans; i++ )
      {
         if ( !(mip->ch_info[i].present) )
            continue;
         WriteAIFFHeader( outfd[i], AIFF_channels, AIFF_frames, AIFF_wordsize, AIFF_sampleRate );
         close( outfd[i] );
      }
   }
}

/********************************************************************
* Function : GetEscInstanceData                                     *
* Purpose  : Reads the adress of the stored esc instance data       *
* Arguments: void                                                   *
* Returns  : A Pointer, which points to the stored pre-def. data    *
*********************************************************************/

ESC_INSTANCE_DATA *GetEscInstanceData ( )
{
   return(&escInstanceData);
}

void UpdateNumBitsInFrame(unsigned short value) {
  escInstanceData.numBitsInFrame = value;
}

FILE *GetInfFilePtr(void) 
{
  return ( inf );
}

FILE *GetBitFilePtr(void) 
{
  return ( bit );
}

/********************************************************************
* Function : GetLastByte                                            *
* Purpose  : Set a pointer to the last byte in the writting process *
* Arguments: void                                                   *
* Returns  : A byte pointer                                         *
*********************************************************************/

byte *GetLastByte(void) 
{
  return(&lastWriteByte);
}

/********************************************************************
* Function : GetNumBits                                             *
* Purpose  : Set a pointer to the number of written bits            *
* Arguments: void                                                   *
* Returns  : An unsigned int pointer                                *
*********************************************************************/

unsigned int *GetNumBits(void) 
{
  return(&numWrittenBits);
}

/********************************************************************
* Function : UpdateNumBits                                          *
* Purpose  : Updates the number of written bits                     *
* Arguments: Value of actual number of written bits                 *
* Returns  : void                                                   *
*********************************************************************/

void UpdateNumBits(unsigned int value) 
{
  numWrittenBits = value;
}

/********************************************************************
* Function : UpdateLastByte                                         *
* Purpose  : Updates the last byte value in the writting process    *
* Arguments: Value of the last byte                                 *
* Returns  : void                                                   *
*********************************************************************/

void UpdateLastByte(byte lastByte) 
{
  lastWriteByte = lastByte;
}

FILE *OpenEPFile(char *fileName, char *mode) 
{
  FILE *filePtr;
  filePtr = fopen ( fileName, mode);
  if ( filePtr == 0 ) {
      PRINT(stderr,"cant open %s\n", fileName);
      exit(8);
  }
  return(filePtr);
}

/********************************************************************
* Function : WriteInfoFile                                          *
* Purpose  : Writes the info for the EP tool to a file (*.inf)      *
* Arguments: actual number of predifined set                        *
* Returns  : void                                                   *
*********************************************************************/

void WriteInfoFile(ESC_INSTANCE_DATA *escInstanceData) 
{
  unsigned short esc;
  unsigned short instanceOfEsc;
  
  if ( inf != NULL ) {
    fprintf(inf, "%hu\n", escInstanceData->numBitsInFrame );
    for ( instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
        if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
          fprintf(inf, "%hu ", 0);
        }
      }
    }
    fprintf(inf,"\n");
    for ( instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
        if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
          fprintf(inf,"%hu\n", escInstanceData->escInstance[instanceOfEsc][esc].bitCount);
        }
      }
    }
  }
}


