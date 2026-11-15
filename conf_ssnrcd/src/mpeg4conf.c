
/*

====================================================================
                        Copyright Header            
====================================================================

This software module was originally developed by Ralf Funken
(Philips, Eindhoven, The Netherlands) and edited by Takehiro Moriya 
(NTT Labs. Tokyo, Japan) in the course of development of the
MPEG-2/MPEG-4 Conformance ISO/IEC 13818-4, 14496-4. This software
module is an implementation of one or more of the Audio Conformance
tools as specified by MPEG-2/MPEG-4 Conformance. ISO/IEC gives users
of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC 13818-3,7, 14496-3) free
license to this software module or modifications thereof for use in
hardware or software products. Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Philips retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party. This copyright notice must be included in 
all copies or derivative works. Copyright 2000.


Source file: MPEG4CONF.C

*/



/*  Standard includes                           */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*  Includes for AFsp Library                   */

#include "libtsp.h"
#include "libtsp/AFpar.h"

/*  Private includes                            */

#include "common.h"
#include "mpeg4conf.h"
#include "audiofile_io.h"
#include "ssnrcd.h"
#include "pns.h"


/***************************************************************************/

struct conf_obj
{
    ssnrcd_ptr              ssnrcdData;
    pns_ptr                 pnsData;
    
    /* chosen tests */
    int                     pns;
    int                     ssnrcd;
    
    unsigned int            apply_criteria;
    int                     segment_length;
    signed long             delay;
    int                     bits_resolution;
    int                     verbose;
    int                     show_accuracy_distribution;
    int                     exponential_output;  
    unsigned long           samples_to_compare;
    unsigned long           samples_lost[2];
    int                     max_resolution;
};

/***************************************************************************/


conf_ptr confDataInit ( void )
{  
    conf_ptr obj;
    
    /* allocate memory */
    if(! ( obj = (conf_ptr) calloc(1,sizeof(struct conf_obj))) )
         return NULL;
          
    obj->apply_criteria                =   DEFAULT_CRITERIA;
    obj->segment_length                =   DEFAULT_SEGMENT_LENGTH;
    obj->delay                         =   DEFAULT_DELAY;
    obj->bits_resolution               =   DEFAULT_RESOLUTION;
    obj->show_accuracy_distribution    =   FALSE;
    obj->max_resolution                =   MAXIMUM_RESOLUTION;
        
    /* chosen test(s) */
    obj->pns                           =   FALSE;
    obj->ssnrcd                        =   TRUE;
    
    obj->ssnrcdData = ssnrcdInit();
    obj->pnsData = pnsInit();

    return obj;
}


/***************************************************************************/

void confDataFree (AFILE* stream1,
                   AFILE* stream2,
                   conf_ptr cD)
{
    close_inputaudio(stream1);
    close_inputaudio(stream2);
    
    if (cD->pns == TRUE)
    {
        pnsFree( cD->pnsData );
    }
    if (cD->ssnrcd == TRUE)
    {
        ssnrcdFree( cD->ssnrcdData );
    }
    
    free(cD);
}


/***************************************************************************/

static void print_banner (void)
{
  fprintf (stderr, "\n");
  fprintf (stderr, "ISO/IEC 14496-4 MPEG-4 Conformance Verification\n");
  fprintf (stderr, "%s  version %s\n", PROGRAM_NAME, SSNRCD_VERSION);
  fprintf (stderr, "\n");
}

/***************************************************************************/

static void show_usage1 (void)
{
  print_banner ();
  fprintf (stderr, "Usage:   %s  [options] <reference inputfile> <inputfile under test>\n", EXECUTABLE_NAME_1);
  fprintf (stderr, "\n");
  fprintf (stderr, "Valid options are:\n");
  
  fprintf (stderr, "                -a           Show distribution of bit accuracy over all samples\n");
  
  fprintf (stderr, "                -d <num>     Time-offset in samples between reference inputfile and\n");
  fprintf (stderr, "                             inputfile under test. Positive value means inputfile under test\n");
  fprintf (stderr, "                             is delayed over <num> samples and the first <num> \n");
  fprintf (stderr, "                             samples of that input file under test are skipped\n");
  fprintf (stderr, "                             Negative values are permitted.\n");
  fprintf (stderr, "                             Default: %d samples.\n", DEFAULT_DELAY);
  
  fprintf (stderr, "                -e [<n>]     Exponential output of values scaled to 2^n. If no n is given,\n");
  fprintf (stderr, "                             bit resolution of file under test will be used.\n");
  
  fprintf (stderr, "                -k <num>     Resolution for calculation of RMS and max. abs.\n");
  fprintf (stderr, "                             difference sample, expressed in bits.\n");
  fprintf (stderr, "                             Valid range: [%d...%d]  Default: %d bits.\n", MINIMUM_RESOLUTION, MAXIMUM_RESOLUTION, DEFAULT_RESOLUTION);
  
  fprintf (stderr, "                -l <num>     Analysis segment length in samples.\n");
  fprintf (stderr, "                             Default: %d samples.\n", DEFAULT_SEGMENT_LENGTH);
  
  fprintf (stderr, "                -t <num>     Conformance criteria to be applied.\n");
  fprintf (stderr, "                               0 : No criterion (default)\n");
  fprintf (stderr, "                               1 : RMS + Max. absolute difference sample\n");
  fprintf (stderr, "                                   using K bit resolution (see -k option)\n");
  fprintf (stderr, "                               2 : Segmental SNR + avg. Cepstral Distortion\n");
  fprintf (stderr, "                               4 : Number of bits accuracy reached for RMS criterion\n");
  fprintf (stderr, "                             These values may be added. For example -t 7 enables\n");
  fprintf (stderr, "                             all three criteria.\n");
    
  fprintf (stderr, "                -v [<num>]   Verbose level.\n");
  fprintf (stderr, "                               1 : Progress report (default)\n");
  fprintf (stderr, "                               2 : Show segments with accuracy below K bit resolution\n");
  fprintf (stderr, "                                   (see -k option)\n");
  fprintf (stderr, "                               3 : Samples with accuracy below K bit resolution\n");
  fprintf (stderr, "                                   (see -k option)\n");
  fprintf (stderr, "                               4 : Show accuracy of all samples\n");
    
  fprintf (stderr, "                -p <num> <frameSize>   PNS conformance test.\n");
  fprintf (stderr, "                               1 : spectral PNS conformance test for long blocks\n");
  fprintf (stderr, "                               2 : spectral PNS conformance test for short blocks\n");
  fprintf (stderr, "                               3 : temporal PNS conformance test\n");
    
  fprintf (stderr, "\n");
  fprintf (stderr, "If the input files have different length, only the number of samples\n");
  fprintf (stderr, "in the shortest file will be used. The additional samples at the end of\n");
  fprintf (stderr, "the longest file will be ignored.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Segmental SNR and cepstral distortion are calculated only for\n");
  fprintf (stderr, "segments with reference signal power in the range [%.0f...%.0f] dB.\n", SSNR_CD_LOWERBOUND, SSNR_CD_UPPERBOUND);
  fprintf (stderr, "Any partial segments at end of input signal are padded with\n");
  fprintf (stderr, "zeroes when calculating segmental SNR and Cepstral Distortion.\n");
  fprintf (stderr, "\n");
}

/***************************************************************************/

static void show_usage2 (void)
{
  print_banner ();
  fprintf (stderr, "Usage:   %s  [options] <reference inputfile> <inputfile under test>\n", EXECUTABLE_NAME_2);
  fprintf (stderr, "\n");
  fprintf (stderr, "Valid options are:\n");
  
  fprintf (stderr, "                -a           Show distribution of bit accuracy over all samples\n");
  
  fprintf (stderr, "                -d <num>     Time-offset in samples between reference inputfile and\n");
  fprintf (stderr, "                             inputfile under test. Positive value means inputfile under test\n");
  fprintf (stderr, "                             is delayed over <num> samples and the first <num> \n");
  fprintf (stderr, "                             samples of that input file under test are skipped\n");
  fprintf (stderr, "                             Negative values are permitted.\n");
  fprintf (stderr, "                             Default: %d samples.\n", DEFAULT_DELAY);
  
  fprintf (stderr, "                -e [<n>]     Exponential output of values scaled to 2^n. If no n is given,\n");
  fprintf (stderr, "                             bit resolution of file under test will be used.\n");
  
  fprintf (stderr, "                -k <num>     Resolution for calculation of RMS and max. abs.\n");
  fprintf (stderr, "                             difference sample, expressed in bits.\n");
  fprintf (stderr, "                             Valid range: [%d...%d]  Default: %d bits.\n", MINIMUM_RESOLUTION, MAXIMUM_RESOLUTION, DEFAULT_RESOLUTION);
  
  fprintf (stderr, "                -l <num>     Analysis segment length in samples.\n");
  fprintf (stderr, "                             Default: %d samples.\n", DEFAULT_SEGMENT_LENGTH);
  
  fprintf (stderr, "                -t <num>     Conformance criteria to be applied.\n");
  fprintf (stderr, "                               0 : No criterion (default)\n");
  fprintf (stderr, "                               1 : RMS + Max. absolute difference sample\n");
  fprintf (stderr, "                                   using K bit resolution (see -k option)\n");
  fprintf (stderr, "                               2 : Segmental SNR + avg. Cepstral Distortion\n");
  fprintf (stderr, "                               4 : Number of bits accuracy reached for RMS criterion\n");
  fprintf (stderr, "                             These values may be added. For example -t 7 enables\n");
  fprintf (stderr, "                             all three criteria.\n");
    
  fprintf (stderr, "                -v [<num>]   Verbose level.\n");
  fprintf (stderr, "                               1 : Progress report (default)\n");
  fprintf (stderr, "                               2 : Show segments with accuracy below K bit resolution\n");
  fprintf (stderr, "                                   (see -k option)\n");
  fprintf (stderr, "                               3 : Samples with accuracy below K bit resolution\n");
  fprintf (stderr, "                                   (see -k option)\n");
  fprintf (stderr, "                               4 : Show accuracy of all samples\n");
    
  fprintf (stderr, "                -p <num>     PNS conformance test.\n");
  fprintf (stderr, "                               1 : spectral, long blocks,  frame length: 1024\n");
  fprintf (stderr, "                               2 : spectral, long blocks,  frame length:  960\n");
  fprintf (stderr, "                               3 : spectral, long blocks,  frame length:  512\n");
  fprintf (stderr, "                               4 : spectral, short blocks, frame length: 1024\n");
  fprintf (stderr, "                               5 : spectral, short blocks, frame length:  960\n");
  fprintf (stderr, "                               6 : temporal, frame length: 1024\n");
  fprintf (stderr, "                               7 : temporal, frame length:  960\n");
  
  fprintf (stderr, "\n");
  fprintf (stderr, "If the input files have different length, only the number of samples\n");
  fprintf (stderr, "in the shortest file will be used. The additional samples at the end of\n");
  fprintf (stderr, "the longest file will be ignored.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Segmental SNR and cepstral distortion are calculated only for\n");
  fprintf (stderr, "segments with reference signal power in the range [%.0f...%.0f] dB.\n", SSNR_CD_LOWERBOUND, SSNR_CD_UPPERBOUND);
  fprintf (stderr, "Any partial segments at end of input signal are padded with\n");
  fprintf (stderr, "zeroes when calculating segmental SNR and Cepstral Distortion.\n");
  fprintf (stderr, "\n");
}

/***************************************************************************/

static void parse_commandline1 (string           inputfilename1,
                                string           inputfilename2,
                                conf_ptr         confData,
                                int              argc,
                                char             **argv)

{
    int        i, l, j;
    int        option;
    string     c;
    int        d;
    int        pns_test;
    int        frame_size;
    char       show_usage_flag = FALSE;

    
    /* init inputfilename */ 
    strcpy (inputfilename1, "");
    strcpy (inputfilename2, "");
    
    /*  n = number of command line arguments  */
    if (argc == 1)
      { 
        show_usage_flag = TRUE;
      }
    else
      { 
        for (i = 1; i < argc; i++)
          {
            strcpy (c, argv[i]);

            if (c[0] == '-')
              {
                l = strlen (c);
                for (j = 1; j < l; j++)
                  {
                    option = (char) (c[j]);
                    switch (option)
                      {
                      case 'h': show_usage_flag = TRUE;
                        break;

                      case 'a': confData->show_accuracy_distribution = TRUE;
                        break;
                         
                      case 'd': if (i < (argc - 1))
                        {
                          (confData->delay) = atol (argv[++i]);
                        }
                      break;    

                      case 'e': (confData->exponential_output) = 1;
                        if (i < (argc - 1))
                          {
                            d = atol (argv[i+1]);
                            if (d > 0) 
                              {
                                (confData->exponential_output) = (unsigned int) d;
                                i++;
                              }
                          }
                        break;
                         
                      case 'k': if (i < (argc - 1))
                        {
                          d = atol (argv[++i]);
                          if ((d >= MINIMUM_RESOLUTION) && (d <= MAXIMUM_RESOLUTION))
                            {
                              (confData->bits_resolution) = d;
                            }
                          else
                            {
                              fprintf(stderr,"resolution %d out of range, using default %d\n",d,DEFAULT_RESOLUTION);
                            }
                        }
                      break;

                      case 'l': if (i < (argc - 1))
                        {
                          d = atol (argv[++i]);
                          if (d > 0) 
                            {
                              (confData->segment_length) = d;
                            }
                        }
                      break;    

                      case 't':
                          if (i < (argc - 1))
                          {
                              d = atol (argv[++i]);
                              if (d > 0) 
                              {
                                  confData->apply_criteria = (unsigned int) d;
                              }
                          }
                      break;

                      case 'v': confData->verbose = 1;
                        if (i < (argc - 1))
                          {
                            d = atol (argv[i+1]);
                            if (d > 0) 
                              {
                                confData->verbose = (unsigned int) d;
                                i++;
                              }
                          }
                        break;
                                
                      case 'p': confData->pns = TRUE;
                                confData->ssnrcd = FALSE;
                        if (i < (argc - 1))
                          { 
                            /* PNS-testmode */
                            pns_test = atoi (argv[i+1]);
                            if (pns_test > 0) {
                              pnsSetTest(confData->pnsData, pns_test);
                              i++;
                            } else {
                              fprintf(stderr,"ERROR: too few arguments for option -p\n");
                              show_usage_flag = TRUE;
                            }
                                                   
                            /* frameSize */
                            frame_size = atoi (argv[i+1]);
                            if (frame_size > 0) {
                              if (pns_test == 1) {
                                switch(frame_size) {
                                case 1024:
                                  pnsSetFrameSize(confData->pnsData, FRAME_SIZE_1024_LONG);
                                  break;
                                case 960:
                                  pnsSetFrameSize(confData->pnsData, FRAME_SIZE_960_LONG);
                                  break;
                                case 512:
                                  pnsSetFrameSize(confData->pnsData, FRAME_SIZE_512_LONG);
                                  break;
                                default:
                                  pnsSetFrameSize(confData->pnsData, FRAME_SIZE_INVALID);
                                  fprintf(stderr,"ERROR: cannot handle frameSize: possible lengths for PNS-test 1: 1024, 960, 512\n");
                                  exit(1);
                                }
                              } else 
                                if (pns_test == 2 || pns_test == 3) {
                                  switch(frame_size) {
                                  case 1024:
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_1024_SHORT);
                                    break;
                                  case 960:
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_960_SHORT);
                                    break;
                                  default:
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_INVALID);
                                    fprintf(stderr,"ERROR: cannot handle frameSize: possible lengths for PNS-test 2/3: 1024, 960\n");
                                    exit(1);
                                  }
                                } else {
                                  fprintf(stderr,"ERROR: cannot handle PNS testmode %d\n",pns_test);
                                  show_usage_flag = TRUE;
                                }
                              i++;
                            } else {
                              fprintf(stderr,"ERROR: too few arguments for option -p\n");
                              show_usage_flag = TRUE;
                            }
                          }
                        break;

                      case '?': show_usage_flag = TRUE;
                        break;
                      } /* switch */
                  } /* for */
              }  /*  if - or / found   */
            else
              {
                if (strcmp (inputfilename1, "") == 0)
                  {
                    strcpy (inputfilename1, c);
                  }
                else
                  {
                    strcpy (inputfilename2, c);
                  }
              }
          }
         
        if ((strcmp (inputfilename1, "") == 0) || (strcmp (inputfilename2, "") == 0))
          {
            show_usage_flag = TRUE;
          }
      }

    if (show_usage_flag)
      {
        show_usage1 ();
        exit (1);
      }
}

/***************************************************************************/

static void parse_commandline2 (string           inputfilename1,
                                string           inputfilename2,
                                conf_ptr         confData,
                                int              argc,
                                char             **argv)

{
    int        i, l, j;
    int        option;
    string     c;
    int        d;
    int        pns_test;
    int        frame_size;
    char       show_usage_flag = FALSE;


    /* init inputfilename */ 
    strcpy (inputfilename1, "");
    strcpy (inputfilename2, "");

    /*  n = number of command line arguments  */
    if (argc == 1)
    { 
        show_usage_flag = TRUE;
    }
    else
    { 
        for (i = 1; i < argc; i++)
        {
            strcpy (c, argv[i]);

            if (c[0] == '-')
            {
                l = strlen (c);
                for (j = 1; j < l; j++)
                {
                    option = (char) (c[j]);
                    switch (option)
                    {
                    case 'h': 
                        show_usage_flag = TRUE;
                        break;

                    case 'a': 
                        confData->show_accuracy_distribution = TRUE;
                        break;
 
                    case 'd': 
                        if (i < (argc - 1))
                        {
                            (confData->delay) = atol (argv[++i]);
                        }
                        break;    

                    case 'e': 
                        (confData->exponential_output) = 1;
                        if (i < (argc - 1))
                        {
                            d = atol (argv[i+1]);
                            if (d > 0) 
                            {
                                (confData->exponential_output) = (unsigned int) d;
                                i++;
                            }
                        }
                        break;
 
                    case 'k': 
                        if (i < (argc - 1))
                        {
                            d = atol (argv[++i]);
                            if ((d >= MINIMUM_RESOLUTION) && (d <= MAXIMUM_RESOLUTION))
                            {
                                (confData->bits_resolution) = d;
                            }
                            else
                            {
                                fprintf(stderr,"resolution %d out of range, using default %d\n",d,DEFAULT_RESOLUTION);
                            }
                        }
                        break;

                    case 'l': 
                        if (i < (argc - 1))
                        {
                            d = atol (argv[++i]);
                            if (d > 0) 
                            {
                                (confData->segment_length) = d;
                            }
                        }
                        break;    

                    case 't':
                        if (i < (argc - 1))
                        {
                            d = atol (argv[++i]);
                            if (d > 0) 
                            {
                                confData->apply_criteria = (unsigned int) d;
                            }
                        }
                        break;

                    case 'v': 
                        confData->verbose = 1;
                        if (i < (argc - 1))
                        {
                            d = atol (argv[i+1]);
                            if (d > 0) 
                            {
                                confData->verbose = (unsigned int) d;
                                i++;
                            }
                        }
                        break;
        
                    case 'p': 
                        confData->pns = TRUE;
                        confData->ssnrcd = FALSE;
                        
                        if (i < (argc - 1))
                        { 
                            /* PNS-testmode */
                            pns_test = atoi (argv[i+1]);
                            
                            if (pns_test > 0) {
                                
                                switch (pns_test) {
                                case 1: /* spectral long blocks, 1024 */
                                    pnsSetTest(confData->pnsData, 1);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_1024_LONG);
                                    break;
                                case 2: /* spectral long blocks, 960 */
                                    pnsSetTest(confData->pnsData, 1);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_960_LONG);
                                    break;
                                case 3: /* spectral long blocks, 512 */
                                    pnsSetTest(confData->pnsData, 1);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_512_LONG);
                                    break;
                                case 4: /* spectral short blocks, 1024 */
                                    pnsSetTest(confData->pnsData, 2);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_1024_SHORT);
                                    break;
                                case 5: /* spectral short blocks, 960 */
                                    pnsSetTest(confData->pnsData, 2);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_960_SHORT);
                                    break;
                                case 6: /* temporal, 1024 */
                                    pnsSetTest(confData->pnsData, 3);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_1024_SHORT);
                                    break;
                                case 7: /* temporal, 960 */
                                    pnsSetTest(confData->pnsData, 3);
                                    pnsSetFrameSize(confData->pnsData, FRAME_SIZE_960_SHORT);
                                    break;
                                }
                                i++;
                                
                            } else {
                                fprintf(stderr,"ERROR: too few arguments for option -p\n");
                                show_usage_flag = TRUE;
                            }
                       }
                       break;
                           
                  case '?': 
                        show_usage_flag = TRUE;
                        break;
                  } /* switch */
                } /* for */
            }  /*  if - or / found   */
            else
            {
                if (strcmp (inputfilename1, "") == 0)
                {
                    strcpy (inputfilename1, c);
                }
                else
                {
                    strcpy (inputfilename2, c);
                }
            }
            }

        if ((strcmp (inputfilename1, "") == 0) || (strcmp (inputfilename2, "") == 0))
        {
            show_usage_flag = TRUE;
        }
    }

    if (show_usage_flag)
    {
        show_usage2 ();
        exit (1);
    }
    
}
        
        
/***************************************************************************/


static int compare_audioheaderdata (audiofile file1,
                                    audiofile file2)
{
    int status = 0;

    if (file1.channels != file2.channels)
    {
        status = 1;
    }

    if (file1.samplefreq != file2.samplefreq)
    {
        status = 1;
    }

    return (status);
}


/***************************************************************************/

static void compare_filelength (audiofile       audio_file1,
                                audiofile        audio_file2,
                                signed long      delay,
                                unsigned long    *samples_lost,
                                unsigned long    *samples_to_compare)
{
    unsigned long   start_compare_pos, end_compare_pos;
    
    start_compare_pos  = (unsigned long) MIN (audio_file1.nrofsamples, MAX (-delay, 0));
    end_compare_pos    = (unsigned long) MAX (MIN (audio_file1.nrofsamples, (audio_file2.nrofsamples - delay)), (long) start_compare_pos);
    *samples_to_compare = end_compare_pos - start_compare_pos;

    samples_lost[0] = (unsigned long) MAX ((audio_file1.nrofsamples - end_compare_pos), 0);
    samples_lost[1] = (unsigned long) MAX ((audio_file2.nrofsamples - delay - (signed long) end_compare_pos), 0);

    if (samples_to_compare == 0)
    {  
        printf ("ERROR: Nothing to compare. Either one of the input files has zero length or delay is out of range.\n");
        exit (1);
    }

    if (audio_file1.nrofsamples != audio_file2.nrofsamples)
    {
        printf ("WARNING: Input files have unequal length.\n");
    }  
}


/***************************************************************************/
/***************************************************************************/

int main (int argc, char *argv[])
{
    audiofile               audio_file1;
    audiofile               audio_file2;
    int                     pns_result;
    int                     pns_frameSize;
    int                     pns_test;
    int                     failure         =  0;
    conf_ptr                confData        =  NULL;
  
    
    /****************************************************************************/
    /*  Init confData                                                           */
    /****************************************************************************/
      
    confData = confDataInit ( );
    
     
    /****************************************************************************/
    /*   Get command line parameters                                            */
    /****************************************************************************/
    
    if (strstr(argv[0], "ssnrcd")) 
    {  
        parse_commandline1 (audio_file1.input_filename,
                            audio_file2.input_filename,
                            confData,
                            argc,
                            argv);
        
    } 
    else if (strstr(argv[0], "mpeg4conf"))     /* new command line handling */
    {
        parse_commandline2 (audio_file1.input_filename,
                            audio_file2.input_filename,
                            confData,
                            argc,
                            argv);
    } 
    else 
    {
        printf("Cannot handle program name.\n");
    }  
    
    /****************************************************************************/
    /*   Program banner on screen                                               */
    /****************************************************************************/

    print_banner ();
    
    printf ("%s  %s and %s:  \n", GREP_STRING, audio_file1.input_filename, audio_file2.input_filename);


    /****************************************************************************/
    /*   Open both input files                                                  */
    /****************************************************************************/

    if (open_inputaudio (&audio_file1.stream, audio_file1.input_filename, &audio_file1))
    {
        printf ("ERROR: Cannot open reference waveform.\n");
        exit (1);
    }
 
    if (open_inputaudio (&audio_file2.stream, audio_file2.input_filename, &audio_file2))
    {
        printf ("ERROR: Cannot open waveform under test.\n");
        exit (1);
    }
        
    /****************************************************************************/
    /*   Check audio header data                                                */
    /****************************************************************************/

    if (compare_audioheaderdata (audio_file1, audio_file2))
    {
        printf ("ERROR: Headers of input files are different.\n");
        exit (1);
    }

    if (check_audioheader (audio_file1))
    {
        printf ("ERROR: Header of reference waveform is invalid.\n");
        exit (1);
    }

    if (check_audioheader (audio_file2))
    {
        printf ("ERROR: Header of waveform under test is invalid.\n");
        exit (1);
    }
    
    if (audio_file1.bits_per_sample != audio_file2.bits_per_sample)
    {    
        printf ("WARNING: Input files have different resolution.\n");
    }

    if ((confData->apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
    {
        if (confData->bits_resolution >= audio_file1.bits_per_sample)
        {    
            printf ("WARNING: Resolution of reference waveform too low for requested %d bit conformance testing.\n", confData->bits_resolution);
        }

        if (confData->bits_resolution >= audio_file2.bits_per_sample)
        {    
            printf ("WARNING: Resolution of waveform under test too low for requested %d bit conformance testing.\n", confData->bits_resolution);
        }
    }
    
    if (confData->exponential_output)
    {
        if (confData->exponential_output == 1)
        {
            confData->exponential_output = audio_file2.bits_per_sample;
        }
        
        if (confData->bits_resolution > confData->exponential_output)
        {
            printf ("ERROR: Resolution for calculation of RMS and max. abs. (%d bit) is greather than output resolution (%d bit).\n", confData->bits_resolution, confData->exponential_output);
            exit (1);
        }
        
        confData->max_resolution = confData->exponential_output;
    }
    
    /****************************************************************************/
    /*   Compare file length                                                    */
    /****************************************************************************/

    compare_filelength (audio_file1,
                        audio_file2,
                        confData->delay,
                        confData->samples_lost,
                        &(confData->samples_to_compare));

    
    /****************************************************************************/
    /*   SSNRCD                                                                 */
    /****************************************************************************/

    if (confData->ssnrcd == TRUE)
    {     
        compare_audiodata (audio_file1.stream,
                           audio_file2.stream,
                           audio_file1,
                           audio_file2,
                           confData->samples_to_compare,  
                           confData->delay,
                           confData->segment_length,
                           confData->bits_resolution,
                           confData->exponential_output,
                           confData->max_resolution,
                           confData->verbose,
                           confData->ssnrcdData);

        failure = generate_report (audio_file1.channels,
                                   audio_file1.input_filename,
                                   audio_file2.input_filename,
                                   confData->samples_to_compare,
                                   confData->delay,
                                   confData->samples_lost,
                                   confData->segment_length,
                                   confData->bits_resolution,
                                   confData->show_accuracy_distribution,
                                   confData->exponential_output,
                                   confData->max_resolution,
                                   confData->apply_criteria,
                                   confData->ssnrcdData);
    }

    /****************************************************************************/
    /*   PNS                                                                    */
    /****************************************************************************/
    
    if (confData->pns == TRUE)
    {
        pns_frameSize = pnsGetFrameSize(confData->pnsData);
        pns_test = pnsGetTest(confData->pnsData);        
        
        switch (pns_test) 
        {
        case 1:
        case 2: 
            /* Spectral Conformance */
            pns_result = CalculateSpectralConformance( audio_file2.stream, 
                                                       audio_file1.stream, 
                                                       audio_file2, 
                                                       audio_file1, 
                                                       confData->pnsData,
                                                       confData->delay,
                                                       confData->verbose);
            
            printf("%s  spectral PNS conformance with analysis frame size %d  : %s\n\n", GREP_STRING, pns_frameSize, pns_result ? "FAIL" : "OK");
            break;
        case 3:
            /* Temporal Conformance */      
            pns_result = CalculateTemporalConformance( audio_file2.stream,
                                                       audio_file1.stream,
                                                       audio_file2,
                                                       audio_file1,
                                                       confData->pnsData,
                                                       confData->delay,
                                                       confData->verbose);
            
            printf("%s  temporal PNS conformance with analysis frame size %d  : %s\n\n", GREP_STRING, pns_frameSize, pns_result ? "FAIL" : "OK");
            break;
        default:
            printf ("ERROR: cannot handle PNS testmode: %d.\n", pns_test);
            break;
        }
    }
    
    /****************************************************************************/
    /*   Free Data                                                              */
    /****************************************************************************/

    confDataFree (audio_file1.stream, audio_file2.stream, confData);

    return (failure);
}

/***************************************************************************/
