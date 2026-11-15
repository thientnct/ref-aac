
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


Source file: SSNRCD.C

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
#include "cepstrum_analysis.h"
#include "audiofile_io.h"
#include "ssnrcd.h"


/****************************************************************************/

struct ssnrcd_obj
{ 
    unsigned long           no_diff_samples[MAX_PCM_CHANNELS];
    double                  maxabsdif_sample[MAX_PCM_CHANNELS];
    double                  rms_dif[MAX_PCM_CHANNELS];
    double                  ssnr[MAX_PCM_CHANNELS];
    double                  avg_CD[MAX_PCM_CHANNELS];   
    int                     max_rms_accuracy;
    int                     max_lsb_accuracy;
    unsigned long           distribution_list[(MAXIMUM_RESOLUTION + 1) * MAX_PCM_CHANNELS];
};

/***************************************************************************/

ssnrcd_ptr ssnrcdInit ( void )
{
    ssnrcd_ptr obj;
    int ch, n;
    
    /* allocate memory */
    if(! ( obj = (ssnrcd_ptr) calloc(1,sizeof(struct ssnrcd_obj))) )
        return NULL;
    
   /* default values */ 
    for (ch = 0; ch < MAX_PCM_CHANNELS; ch++)
    {    
      obj->rms_dif[ch]   =   INITIAL_RMS_dB;
    }
    
    obj->max_lsb_accuracy    =   MAXIMUM_RESOLUTION;
    obj->max_rms_accuracy    =   MAXIMUM_RESOLUTION;
   
    return obj;
}


/***************************************************************************/

void ssnrcdFree ( ssnrcd_ptr s )
{
    free(s);
}

/***************************************************************************/

void compare_audiodata (AFILE               *stream1,
                        AFILE               *stream2,
                        audiofile           file1,
                        audiofile           file2,
                        unsigned long       samples_to_compare,
                        signed long         delay,
                        int                 segment_length,
                        int                 bits_resolution,
                        int                 exponential_output,
                        int                 max_resolution,
                        int                 verbose,
                        ssnrcd_ptr          ssnrcd)
{
    int                 sample_counter, abs_sample_no, n, ch, sample_accuracy;
    int                 segment_accuracy [MAX_PCM_CHANNELS];
    double              sample_ref, sample_test;
    double              abs_difference, abs_difference_scaled;
    double              maxabsdif_segment [MAX_PCM_CHANNELS];
    double              overall_cum_difference [MAX_PCM_CHANNELS];
    int                 nr_segments_total, segment_counter;
    int                 samples_read1, samples_read2;
    int                 nr_segments_snrcd [MAX_PCM_CHANNELS];
    double              seg_cum_signal1 [MAX_PCM_CHANNELS];
    double              seg_cum_signal2 [MAX_PCM_CHANNELS];
    double              ss_cum [MAX_PCM_CHANNELS];
    float               *pcm_buffer1, *pcm_buffer2, *temp_pcm_buffer;
    double              lpc_coef [N_PR + 1];
    double              cepstrum1 [2*N_PR + 1], cepstrum2 [2*N_PR + 1]; 
    double              cum_sqrt_D [MAX_PCM_CHANNELS];
    double              D;
    double              refsig_power_dB;
    float               *ham_window;
    float               wlag [N_PR + 1];
    int                 skip_length[2];
    const int           scale_up = 1 << (exponential_output - 1);
    
 
   
    /****************************************************************************/
    /*   Memory allocation                                                      */
    /****************************************************************************/
     
    pcm_buffer1     = (float*) malloc (sizeof (float) * file1.channels * segment_length);
    pcm_buffer2     = (float*) malloc (sizeof (float) * file2.channels * segment_length);
    temp_pcm_buffer = (float*) malloc (sizeof (float) * segment_length);
    ham_window      = (float*) malloc (sizeof (float) * segment_length);
   
    if ((pcm_buffer1 == NULL) || 
        (pcm_buffer2 == NULL) || 
        (temp_pcm_buffer == NULL) || 
        (ham_window == NULL))
    {
        printf ("ERROR: Memory allocation failed.\n");
        exit (1);
    }
   

    /****************************************************************************/
    /*   Variable initialization                                                */
    /****************************************************************************/

    for (ch = 0; ch < MAX_PCM_CHANNELS; ch++)
     {
         nr_segments_snrcd [ch]       = 0;
         overall_cum_difference [ch]  = 0.0;
         seg_cum_signal1 [ch]         = 0.0;
         seg_cum_signal2 [ch]         = 0.0;
         ss_cum [ch]                  = 0.0; 
         cum_sqrt_D [ch]              = 0.0;
     }  

    
    nr_segments_total = 1 + ((samples_to_compare - 1) / segment_length);
    abs_sample_no = 0;
    
    for (n = 0; n <= N_PR; n++)
    {
        lpc_coef [n] = 0.0;
    }

    skip_length[0] = -MIN (0, delay);
    skip_length[1] = MAX (0, delay);

    
    /****************************************************************************/
    /*   Calculate windows for cepstral analysis                                */
    /****************************************************************************/

    hamwdw (ham_window, segment_length);
    lagwdw (wlag, N_PR, BW);
   

    /****************************************************************************/
    /*   Verbose output                                                         */
    /****************************************************************************/
    
    if (verbose == 2)
    {
        printf ("\n%s  Segments which didn't reach the LSB criterion:\n", GREP_STRING);
    }
    else if (verbose == 3)
    {
        printf ("\n%s  Samples which didn't reach the LSB criterion :\n", GREP_STRING);
    }
    else if (verbose == 4)
    {
        printf ("\n%s  Difference for all samples                   :\n", GREP_STRING);
    }
    
    
    /****************************************************************************/
    /*   Process audio input files                                              */
    /****************************************************************************/

    for (segment_counter = 0; segment_counter < nr_segments_total; segment_counter++)
    {
        /****************************************************************************/
        /*   Reset variables for segment loop                                       */
        /****************************************************************************/
        
        for (ch = 0; ch < file1.channels; ch++)
        {
            segment_accuracy  [ch] = max_resolution;
            maxabsdif_segment [ch] = 0.0;
        }
        
        /****************************************************************************/
        /*   Read one segment of PCM data from both input files                     */
        /****************************************************************************/

        if (verbose == 1)
        {
            fprintf (stderr, "   %d %% \r", (100 * segment_counter) / nr_segments_total);
        }
        
        samples_read1 = get_audiosample (pcm_buffer1, 
                                         stream1, 
                                         file1.channels, 
                                         segment_counter * segment_length + skip_length[0], 
                                         segment_length);

        samples_read2 = get_audiosample (pcm_buffer2, 
                                         stream2,
                                         file1.channels, 
                                         segment_counter * segment_length + skip_length[1],
                                         segment_length); 
                                         
        apply_zeropadding (pcm_buffer1, 
                           file1.channels, 
                           MIN (samples_read1, samples_read2),
                           segment_length);

        apply_zeropadding (pcm_buffer2, 
                           file1.channels, 
                           MIN (samples_read1, samples_read2),
                           segment_length);
        
        /****************************************************************************/
        /*   Calculate max. abs. difference, SNR, RMS, etc. in current segment      */
        /****************************************************************************/

        for (sample_counter = 0; sample_counter < MIN(samples_read1, samples_read2); sample_counter++)
        {
            abs_sample_no++;
            
            for (ch = 0; ch < file1.channels; ch++)
            {
                /*   read reference and test sample from buffer */

                sample_ref  = pcm_buffer1[(sample_counter * file1.channels) + ch];
                sample_test = pcm_buffer2[(sample_counter * file1.channels) + ch];
                
                /*   count number of different samples and 
                     calculate max. absolute value in difference signal  */
                
                if (sample_ref == sample_test)
                {
                    abs_difference = 0.0;
                    abs_difference_scaled = 0.0;
                }
                else
                {
                    (ssnrcd->no_diff_samples[ch]) ++;
                    abs_difference = fabs (sample_ref - sample_test);
                    abs_difference_scaled = abs_difference / PCM_SCALEFACTOR;
                }
                
                if (abs_difference_scaled > ssnrcd->maxabsdif_sample[ch])
                {
                    ssnrcd->maxabsdif_sample[ch] = abs_difference_scaled;
                }
                
                if (abs_difference_scaled > maxabsdif_segment [ch])
                {
                    maxabsdif_segment [ch] = abs_difference_scaled;
                }

                /*   calculate accuracy of sample from its absolute difference */
                
                if  (abs_difference_scaled == 0.0)
                {
                    sample_accuracy = max_resolution + 2;
                }
                else
                {
                    for (sample_accuracy = max_resolution + 1; (abs_difference_scaled > ABSDIF_THRESHOLD(sample_accuracy)) && (sample_accuracy > 2); sample_accuracy--);
                }
                
                /*   get maximal LSB accuracy */
                
                if (sample_accuracy < ssnrcd->max_lsb_accuracy)
                {
                    ssnrcd->max_lsb_accuracy = sample_accuracy;
                }
                
                if (sample_accuracy < segment_accuracy [ch])
                {
                    segment_accuracy [ch] = sample_accuracy;
                }
            
                /*   count number of samples with their specific absolute difference */
                
                (ssnrcd->distribution_list[((MAXIMUM_RESOLUTION + 1) * ch) + (sample_accuracy - 2)]) ++;
                
                /*  output accuracy for sample (option -v) */
                
                if ( ((verbose==3) && (sample_accuracy < bits_resolution)) || verbose==4)
                {
                    printf ("channel: %2d, segment: %8d,  sample: %8d,  absolute sample: %10d,  ", ch, segment_counter, sample_counter, abs_sample_no);
                    if (exponential_output)
                    {
                        printf ("difference: 2 ^ %6.3e = ", LD(abs_difference_scaled * scale_up));
                        if (LD(abs_difference_scaled * scale_up) < 0)
                        {
                            printf ("%.3e\n", abs_difference_scaled * scale_up);
                        }
                        else
                        {
                            printf ("%.0f\n", abs_difference_scaled * scale_up);
                        }
                    }
                    else
                    {
                        printf ("difference: %.10e\n", abs_difference_scaled);
                    }
                }
                
                /*   collect statistics for overall RMS      */
                
                overall_cum_difference [ch] += (abs_difference_scaled * abs_difference_scaled);
                
                /*   collect statistics for segmental SNR    */
                
                seg_cum_signal1 [ch] += (pcm_buffer1[(sample_counter * file1.channels) + ch] / PCM_SCALEFACTOR) * (pcm_buffer1[(sample_counter * file1.channels) + ch] / PCM_SCALEFACTOR);
                seg_cum_signal2 [ch] += (abs_difference_scaled * abs_difference_scaled);
               
            }   /*  for  ch  */
        }
         
        /****************************************************************************/
        /*   Process completed segment                                              */
        /****************************************************************************/
        
        for (ch = 0; ch < file1.channels; ch++)
        {
            if (seg_cum_signal1 [ch] > 0.0)
            {
                refsig_power_dB = 10 * log10 (seg_cum_signal1 [ch] / segment_length);
                
                if ((refsig_power_dB >= SSNR_CD_LOWERBOUND) && (refsig_power_dB <= SSNR_CD_UPPERBOUND))
                {   
                    nr_segments_snrcd[ch]++;

                    ss_cum [ch] += log10 (1.0 + (seg_cum_signal1 [ch] / (SEGSNR_MIN_NOISE + seg_cum_signal2 [ch])));
                    
                    /****************************************************************************/
                    /*   Cepstral analysis on current segment                                   */
                    /****************************************************************************/
                    
                    copy_sample (temp_pcm_buffer, pcm_buffer1, ch, file1.channels, segment_length);
                    calculate_lpc  (temp_pcm_buffer, segment_length, lpc_coef, ham_window, wlag);
                    lpc2cepstrum (N_PR, lpc_coef, 2 * N_PR, cepstrum1);
                    
                    copy_sample (temp_pcm_buffer, pcm_buffer2, ch, file1.channels, segment_length);
                    calculate_lpc  (temp_pcm_buffer, segment_length, lpc_coef, ham_window, wlag);
                    lpc2cepstrum (N_PR, lpc_coef, 2 * N_PR, cepstrum2);
                    
                    for (n = 1, D = 0.0; n <= 2 * N_PR; n++)
                    {
                        D += ((cepstrum1[n] - cepstrum2[n]) * (cepstrum1[n] - cepstrum2[n]));
                    }
                    
                    cum_sqrt_D [ch] += sqrt (D);
                } 
            }
            
            seg_cum_signal1 [ch] = 0.0;
            seg_cum_signal2 [ch] = 0.0;

        
            /*  output accuracy for segment (option -v) */
            
            if ((verbose==2) && (segment_accuracy [ch] < bits_resolution))
            {
                printf ("channel: %2d, segment: %8d,  ", ch, segment_counter);
                if (exponential_output)
                {
                    printf ("maximum difference: 2 ^ %6.3e = ", LD(maxabsdif_segment [ch] * scale_up));
                    if (LD(maxabsdif_segment [ch] * scale_up) < 0)
                    {
                        printf ("%.3e\n", maxabsdif_segment [ch] * scale_up);
                    }
                    else
                    {
                        printf ("%.0f\n", maxabsdif_segment [ch] * scale_up);
                    }
                }
                else
                {
                    printf ("maximum difference: %.10e\n", maxabsdif_segment [ch]);
                }
            }

        }   /*  for  ch  */
        
    }   /*  for segment_counter   */
    
    /****************************************************************************/
    /*   Calculate overall statistics                                           */
    /****************************************************************************/

    /*   calculate overall RMS value of difference signal (in dB)  */

    for (ch = 0; ch < file1.channels; ch++)
    {
        if (overall_cum_difference [ch] > 0.0)
        {
            ssnrcd->rms_dif[ch] = 20 * log10 (sqrt (overall_cum_difference [ch] / samples_to_compare));
        }
    }

    
    /*   calculate average segmental SNR value of difference signal (in dB)  */

    for (ch = 0; ch < file1.channels; ch++)
    {
        if ((ss_cum [ch] > 0.0) && (nr_segments_snrcd [ch] > 0))
        {
            ssnrcd->ssnr[ch] = 10 * log10 (pow (10.0, (ss_cum [ch] / nr_segments_snrcd [ch])) - 1.0);
        }
        else
        {
            ssnrcd->ssnr[ch] = INITIAL_SNR_dB;
        }
    }


    /*   calculate average cepstral distortion (in dB)   */
    
    for (ch = 0; ch < file1.channels; ch++)
    {
        if (nr_segments_snrcd [ch] > 0)
        {
            ssnrcd->avg_CD[ch] = ((10.0 * sqrt (2.0)) / (log (10) *  nr_segments_snrcd[ch])) * cum_sqrt_D [ch];
        }
        else
        {
            ssnrcd->avg_CD[ch] = 0.0; 
        }
    }


    /****************************************************************************/
    /*   Calculate maximal RMS accuracy                                         */
    /****************************************************************************/
    
    for (ch = 0; ch < file1.channels; ch++)
    { 
        while ((ssnrcd->rms_dif[ch] > RMS_THRESHOLD(ssnrcd->max_rms_accuracy)) && (ssnrcd->max_rms_accuracy > 0))
        {
            (ssnrcd->max_rms_accuracy)--;
        }        
    }
       
    
    /****************************************************************************/
    /*   Verbose output                                                         */
    /****************************************************************************/
    
    if (verbose > 1)
    {
        printf ("%s  end of list.\n\n", GREP_STRING);
    }
    
    /****************************************************************************/
    /*   Free all allocated memory                                              */
    /****************************************************************************/

    free (ham_window);
    free (temp_pcm_buffer);
    free (pcm_buffer2);
    free (pcm_buffer1);
}

/***************************************************************************/

int  generate_report (int                no_channels,
                       string             input_filename1, 
                       string             input_filename2,
                       unsigned long      samples_to_compare,
                       signed long        delay, 
                       unsigned long      samples_lost[2],
                       int                segment_length,
                       int                bits_resolution,
                       int                show_accuracy_distribution,
                       int                exponential_output,
                       int                max_resolution,
                       unsigned int       apply_criteria,
                       ssnrcd_ptr         ssnrcd)
{

    int                      ch, accuracy;
    char                     full_acc_fail = FALSE;
    char                     lim_acc_fail = FALSE;
    int                      failure = 0;
    const int                scale_up = 1 << (exponential_output - 1);

    /*  print delay and number of skipped samples    */
    
    if (delay != 0)
    {
        if (delay > 0)
        {   
            printf ("    delay of waveform under test                      : %ld samples\n", delay);
        }                                              
        else
        {
            printf ("    delay of reference waveform                       : %ld samples ( %2.1f %% )\n", -delay, (double)-delay / ((double)samples_to_compare + (double)MAX(samples_lost[0], samples_lost[1])) * 100);
        }
    }
    
    if ((samples_lost[0] > 0) || (samples_lost[1] > 0))
    {
        if (samples_lost[0] > 0)
        {
            printf ("    samples lost at the end of reference waveform     : %ld samples ( %2.1f %% )\n", samples_lost[0], (double)samples_lost[0] / ((double)samples_to_compare + (double)samples_lost[0]) * 100);
        }
        else
        {
            printf ("    samples lost at the end of waveform under test    : %ld samples\n", samples_lost[1]);
        }
    }

    /*  print results per channel   */
    
    for (ch = 0; ch < no_channels; ch++)
    {
        printf ("channel: %d \n", ch);

        if (ssnrcd->no_diff_samples[ch] == 0)
        {
            printf ("    input channels match exactly\n");
        }
        else
        {
            printf ("    samples compared                              : %ld\n", samples_to_compare);
            printf ("    number of different samples                   : %lu\n", ssnrcd->no_diff_samples[ch]);
            
            /*  Max. absolute difference sample             */
            
            if (exponential_output)
            {
                printf ("    max. abs. difference sample                   : 2 ^ %.3e = ", LD(ssnrcd->maxabsdif_sample[ch] * scale_up));
                if (LD(ssnrcd->maxabsdif_sample[ch] * scale_up) < 0)
                {
                    printf ("%.3e", ssnrcd->maxabsdif_sample[ch] * scale_up);
                }
                else
                {
                    printf ("%.0f", ssnrcd->maxabsdif_sample[ch] * scale_up);
                }
                printf (" (threshold: 2 ^ %.0f = %.0f, %d bits res.)", LD(ABSDIF_THRESHOLD(bits_resolution) * scale_up), ABSDIF_THRESHOLD(bits_resolution) * scale_up, bits_resolution);
            }
            else
            {
                printf ("    max. abs. difference sample                   : %.2e (threshold: %.2e, %d bits res.)", ssnrcd->maxabsdif_sample[ch], ABSDIF_THRESHOLD(bits_resolution), bits_resolution);
            }

            if ((apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
            {
                if (ssnrcd->maxabsdif_sample[ch] <= ABSDIF_THRESHOLD(bits_resolution))
                {
                    printf (" -> OK");
                }
                else
                {
                    printf (" -> FAIL");
                    full_acc_fail = TRUE;
                }
                
            }
           
            printf ("\n");

            /*  Overall RMS value of difference signal      */

            printf ("    overall RMS value of difference               : ");
            if (ssnrcd->rms_dif[ch] > THRESHOLD_RMS_dB)
            {
                printf ("%.2f dB (threshold: %.2f dB, %d bits res.)", ssnrcd->rms_dif[ch], RMS_THRESHOLD(bits_resolution), bits_resolution);
                
                if ((apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
                {
                    if (ssnrcd->rms_dif[ch] < RMS_THRESHOLD(bits_resolution))
                    {
                        printf (" -> OK");
                    }
                    else
                    {
                        printf (" -> FAIL");
                        full_acc_fail = TRUE;
                    }
                }
            }
            else
            {
                printf ("--");
            }
            printf ("\n");
            
            /*  Average segmental SNR of difference signal  */
            
            printf ("    average segmental SNR value of difference     : ");
            if (ssnrcd->ssnr[ch] > THRESHOLD_SNR_dB)
            {
                printf ("%.2f dB (%d samples per segment)", ssnrcd->ssnr[ch], segment_length);
                
                if ((apply_criteria & SEGSNR_CD_CRITERION) == SEGSNR_CD_CRITERION)
                {
                    if (ssnrcd->ssnr[ch] > SEGMENTAL_SNR_THRESHOLD)
                    {
                        printf (" -> OK");
                    }
                    else
                    {
                        printf (" -> FAIL");
                        lim_acc_fail = TRUE;
                    }
                }
            }
            else
            {
                printf ("--");
            }
            printf ("\n");

            /*  Average cepstral distortion                 */

            printf ("    cepstral distortion                           : ");
            printf ("%.2f dB (%d samples per segment)", ssnrcd->avg_CD[ch], segment_length);
            
            if ((apply_criteria & SEGSNR_CD_CRITERION) == SEGSNR_CD_CRITERION)
            {
                if (ssnrcd->avg_CD[ch] < CEPSTRAL_DISTORTION_THRESHOLD)
                {
                    printf (" -> OK");
                }
                else
                {
                    printf (" -> FAIL");
                    lim_acc_fail = TRUE;
                }
            }    
            printf ("\n");
        }
    }


    /*   print overall result   */

    if ((apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
    {
        printf ("%s  Test on RMS and abs. max. difference sample  : %s\n", GREP_STRING, full_acc_fail ? "FAIL" : "OK");
    }

    if ((apply_criteria & SEGSNR_CD_CRITERION) == SEGSNR_CD_CRITERION)
    {
        printf ("%s  Test on seg. SNR and cepstral distortion     : %s\n", GREP_STRING, lim_acc_fail ? "FAIL" : "OK");
    }

    if ((apply_criteria & NR_BITS_ACCURACY_CRITERION) == NR_BITS_ACCURACY_CRITERION)
    {
        printf ("%s  Reached RMS criterion    for a maximum of    : %i bit\n", GREP_STRING, ssnrcd->max_rms_accuracy);
        printf ("%s  Reached LSB criterion    for a maximum of    : %i bit\n", GREP_STRING, ssnrcd->max_lsb_accuracy);
        printf ("%s  Reached RMS/LSB criteria for a maximum of    : %i bit\n", GREP_STRING, MIN(ssnrcd->max_rms_accuracy, ssnrcd->max_lsb_accuracy));
    }
    
    if (show_accuracy_distribution)
    {
        printf ("%s  Distribution of acuracy over all samples     :\n\n", GREP_STRING); 
        printf (" channel    :       ");
        for (ch = 0; ch < no_channels; ch++)
        {
            printf("%8i", ch); 
        }
        printf ("\n");
        printf ("--------------------");
        for (ch = 0; ch < no_channels; ch++)
        {
            printf("--------"); 
        }
        printf ("------------\n");
        printf (" difference :\n");
        
        for (accuracy = max_resolution + 2; accuracy >= 2; accuracy--)
        {
            if (accuracy < max_resolution + 2)
            {
                if (exponential_output)
                {
                    printf ("  <= 2 ^ %7.0f :  ", LD(ABSDIF_THRESHOLD(accuracy) * scale_up));
                }
                else
                {
                    printf ("  <= %.5e :  ", ABSDIF_THRESHOLD(accuracy));
                }
            }
            else
            {
                /* print no difference samples */
                if (exponential_output)
                {
                    printf ("   = 0           :  ");
                }
                else
                {
                    printf ("   = %.5e :  ", 0.0);
                }
            }
            
            for (ch = 0; ch < no_channels; ch++)
            {
                printf("%8lu", ssnrcd->distribution_list[((MAXIMUM_RESOLUTION + 1) * ch) + (accuracy - 2)]); 
            }
                        
            printf ("  samples\n");
        
            /* print threshold line */
            if (accuracy == bits_resolution)
            {
                printf("***\n");
            }
        }
    
    } /* show_accuracy_distribution */

    printf ("\n\n");

    if ( full_acc_fail ) failure = 2;
    if ( lim_acc_fail )  failure = 3;

    return ( failure );
}

/***************************************************************************/
