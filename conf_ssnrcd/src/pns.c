/* 
==============================================================

               (C) copyright Fraunhofer - IIS (2002)
                        All Rights Reserved

   filename: CPnsConformance.c
             
   authors : Martin Weishart martin.weishart@iis.fhg.de
             Ralph Sperschneider
   creation: 2000-10-26
   contents/description: implemation for calculation of
                         PNS conformance criteria

==============================================================
*/

#include "pns.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "libtsp.h"

#include "common.h"
#include "tables.h"
#include "audiofile_io.h"


extern void CompFFT(Float *afftData,int len, int isign) ;


/*****************************************************************************/

struct pns_obj
{
    int                     pns_test;
    FRAME_SIZE              pns_frameSize;
    
    /* temporal Conformance */
    Float                  *m_ConformanceEnergy3_ref;
    Float                  *m_ConformanceEnergy3_est;
    unsigned int            m_Conformance3Counter;
    unsigned int            wrongRatioCount;
    int                     samp_rate;
    const short int        *sfboff;
    unsigned int            sfbsize;
};

/*****************************************************************************/

static int calculateEnergyOfFrame( Float            * buffer, 
                                   Float            * refBuffer,
                                   Float            * est_abs_time_value,
                                   Float            * ref_abs_time_value,
                                   unsigned int     frameSize);

static int accumulateFrameEnergiesInSfb( Float          * buffer, 
                                         Float          * refBuffer,
                                         Float          est_sfb_sum[SFB_1], 
                                         Float          ref_sfb_sum[SFB_1],
                                         unsigned int   frameSize,
                                         pns_ptr        p);

static int calculateSpectralConformance( Float          ** est_sfb_sum, 
                                         Float          ** ref_sfb_sum, 
                                         unsigned int   lastFrame,
                                         unsigned int   *wrongRatioCount,
                                         int            verbose,
                                         unsigned int   sfbsize);

static critical_ratio calculateCriticalRatio( Float         *est_sfb_sum, 
                                               Float        *ref_sfb_sum, 
                                               unsigned int lastFrame,
                                               int          sfbnum,
                                               unsigned int *wrongRatioCount,
                                               int          verbose);

static Float calculateMeanValue( Float          vector[],
                                 unsigned int   maxValue);

static Float calculateStandardDeviation( Float          vector[],
                                         unsigned int   maxValue);

static int calculateSquaredEnergy( float           * buffer, 
                                   float            * refBuffer,
                                   unsigned int     frameSize,
                                   Float            *m_ConformanceEnergy3_ref,
                                   Float            *m_ConformanceEnergy3_est,
                                   unsigned int     *m_Conformance3Counter);

static int calculateTemporalConformance( int                verbose,
                                          Float             *m_ConformanceEnergy3_ref,
                                          Float             *m_ConformanceEnergy3_est,
                                          unsigned int      *m_Conformance3Counter);


/*****************************************************************************/

pns_ptr pnsInit ( void )
{
    pns_ptr obj;
    
    /* allocate memory */
    if(! ( obj = (pns_ptr) calloc(1,sizeof(struct pns_obj))) )
        return NULL;
     
    
    /* default values */
    obj->pns_test           =   -1;
    obj->pns_frameSize      =   FRAME_SIZE_INVALID;
   
    return obj;
}


/*****************************************************************************/

static int initTemporalConformance( unsigned int fileSize, 
                                     Float **m_ConformanceEnergy3_ref,
                                     Float **m_ConformanceEnergy3_est)
{
  if(!(*m_ConformanceEnergy3_ref = (Float*) calloc(fileSize/(BLOCK_LENGTH),sizeof(Float))))
      return -1;
  
  if(!(*m_ConformanceEnergy3_est = (Float*) calloc(fileSize/(BLOCK_LENGTH),sizeof(Float))))
      return -1;
  
  
  return 0;
}

/*****************************************************************************/

void pnsFree (pns_ptr p)
{
    free(p);
}

/*****************************************************************************/

void pnsSetTest(pns_ptr p, int test) {
    
    p->pns_test = test;
}

void pnsSetFrameSize(pns_ptr p, int frameSize) {
    
    p->pns_frameSize = frameSize;
}

int pnsGetTest(pns_ptr p) {

    return p->pns_test;
}

int pnsGetFrameSize(pns_ptr p) {
    
    return p->pns_frameSize;
}

/*****************************************************************************/



/*****************************************************************************/
/*                                                                           */
/* Conformance 1 and 2 : Spectral Criterium                                  */
/*                                                                           */
/*****************************************************************************/

int CalculateSpectralConformance( AFILE            *Pcmfile,
                                  AFILE             *refPcmfile,
                                  audiofile         Pcm_header,
                                  audiofile         refPcm_header,
                                  pns_ptr           p,
                                  int               delay,
                                  int               verbose)
{
  Float ** ref_sfb_sum ;
  Float ** est_sfb_sum ;
  Float * buffer;
  Float * refBuffer;
  long int j = 0, i = 0, k;
  long int fileSize;
  long int numBlocks;
  unsigned int frameSize = p->pns_frameSize;
  
  fileSize = Pcm_header.nrofsamples / Pcm_header.channels;
  fileSize-=(delay>0 ? delay : 0);

  switch(frameSize)
    {
    case FRAME_SIZE_960_LONG:
    case FRAME_SIZE_960_SHORT:
        numBlocks = FRAME_SIZE_960_SHORT;
        break;
    default:
        numBlocks = FRAME_SIZE_1024_SHORT;
        break;
    }
  
  ref_sfb_sum = (Float **)calloc(2*fileSize/numBlocks,sizeof(Float*));
  est_sfb_sum = (Float **)calloc(2*fileSize/numBlocks,sizeof(Float*));
  for ( i = 0; i<2*fileSize/numBlocks; i++){
    ref_sfb_sum[i] = (Float*)calloc(SFB_1,sizeof(Float));
    est_sfb_sum[i] = (Float*)calloc(SFB_1,sizeof(Float));
  }
  buffer = (Float*) calloc(frameSize*2,sizeof(Float));
  refBuffer = (Float*) calloc(frameSize*2,sizeof(Float)) ;

  for ( i=0; i< (int)(fileSize)-((int)frameSize/2) ; i=i+(int)(frameSize/2)){
    /*fprintf(stderr,"%d %d ",i, fileSize);*/
    
   get_audiosample ( (float*) refBuffer,
                     refPcmfile,
                     refPcm_header.channels,
                     i-(delay<0?delay:0),
                     frameSize);

   get_audiosample ( (float*) buffer,
                     Pcmfile,
                     Pcm_header.channels,
                     i+(delay>0?delay:0),
                     frameSize);

    k=-1;
    
    p->samp_rate = Pcm_header.samplefreq;
   
    while ((samp_rate_info[++k].samp_rate != p->samp_rate)) {
      if (samp_rate_info[k].samp_rate == 0) {
        fprintf(stderr,"cannot handle sampling rate %i\n",p->samp_rate);
        return -1 ;
      }
    }
    /* fprintf(stderr,"detected sampling rate: %5i Hz\n",samp_rate_info[k].samp_rate); */
    switch (frameSize)
      {
      case FRAME_SIZE_1024_LONG:
         p->sfboff  = samp_rate_info[k].SFbands1024;
         p->sfbsize = samp_rate_info[k].nsfb1024;
         break;
      case FRAME_SIZE_1024_SHORT:
         p->sfboff  = samp_rate_info[k].SFbands128;
         p->sfbsize = samp_rate_info[k].nsfb128;
    	 break;
      case FRAME_SIZE_960_LONG:
         p->sfboff  = samp_rate_info[k].SFbands960;
         p->sfbsize = samp_rate_info[k].nsfb960;
         break;
      case FRAME_SIZE_960_SHORT:
         p->sfboff  = samp_rate_info[k].SFbands120;
         p->sfbsize = samp_rate_info[k].nsfb120;
         break;
      case FRAME_SIZE_512_LONG:
         p->sfboff = samp_rate_info[k].SFbands512;
         p->sfbsize = samp_rate_info[k].nsfb512;
  	     break;
      default:
         fprintf(stderr,"cannot handle frameSize\n\n");
         exit(1);
      }
    accumulateFrameEnergiesInSfb(buffer, refBuffer, &est_sfb_sum[j][0], &ref_sfb_sum[j][0], frameSize, p);
    j++;
  }
  calculateSpectralConformance( est_sfb_sum, ref_sfb_sum, j, &(p->wrongRatioCount), verbose, p->sfbsize );
  fprintf(stderr,"\n number of sfb with wrong ratio: %d\n", p->wrongRatioCount);
  if ( p->wrongRatioCount == 0 ){
    fprintf(stderr,"\n result ok \n\n");
  }
  else {
    fprintf(stderr,"\n result wrong \n\n");
  }
  for ( i = 0; i<2*fileSize/(256); i++){
    free (ref_sfb_sum[i]);
    free (est_sfb_sum[i]);
  }
  free (ref_sfb_sum);
  free (est_sfb_sum);
  free (buffer);
  free (refBuffer);
  return p->wrongRatioCount;
}

/*****************************************************************************/

static int calculateEnergyOfFrame( Float        * buffer, 
                                   Float        * refBuffer,
                                   Float        * est_abs_time_value,
                                   Float        * ref_abs_time_value,
                                   unsigned int frameSize )
{

  int   * ref_time_value = (int*) calloc(2*frameSize,sizeof(int));
  Float * ref_weighted_time_value = (Float*) calloc(2*frameSize,sizeof(Float)); 
  int   * est_time_value = (int*) calloc(2*frameSize,sizeof(int)); 
  Float * est_weighted_time_value = (Float*) calloc(2*frameSize,sizeof(Float)); 

  Float * hannwin;
 
  unsigned int i, k;

  /* convert bit buffer to int */
  for ( i = 0; i < frameSize; i++ ){
    ref_time_value[i] =  (int)refBuffer[i];
    ref_time_value[i]++;
    est_time_value[i] =  (int)buffer[i];
    est_time_value[i]++;
  }
  
  switch (frameSize)
    {
    case FRAME_SIZE_1024_LONG:
       hannwin = &hannwin_2048[0];
       break;
    case FRAME_SIZE_1024_SHORT:
       hannwin = &hannwin_256[0];
       break;
    case FRAME_SIZE_960_LONG:
       hannwin = &hannwin_1920[0];
       break;
    case FRAME_SIZE_960_SHORT:
       hannwin = &hannwin_240[0];
       break;
    case FRAME_SIZE_512_LONG:
       hannwin = &hannwin_1024[0];
       break;
    default:
       fprintf(stderr,"cannot handle frameSize\n\n");
       exit(1);
    }
  
  /* multiply each time value with the window shape  */
  for (i = 0; i< frameSize/2; i++){
    ref_weighted_time_value[2*i] = (Float)ref_time_value[i] * hannwin[i]; 
    est_weighted_time_value[2*i] = (Float)est_time_value[i] * hannwin[i];  
  }
  for (i = frameSize/2; i< frameSize; i++){
    ref_weighted_time_value[2*i] = (Float)ref_time_value[i] * hannwin[i];  
    est_weighted_time_value[2*i] = (Float)est_time_value[i] * hannwin[i]; 
  }
  for (i = 0; i< frameSize; i++){
    ref_weighted_time_value[2*i+1] = 0.0f;
    est_weighted_time_value[2*i+1] = 0.0f;
  }
  /* calculate fft of previus calculated values   */
   CompFFT((Float*)ref_weighted_time_value , frameSize, 1);
   CompFFT((Float*)est_weighted_time_value , frameSize, 1);

  /* calculate energy of value  */
  for (i = 0; i< frameSize/2; i++){
    ref_abs_time_value[i] = ref_weighted_time_value[2*i] * ref_weighted_time_value[2*i] + 
      ref_weighted_time_value[2*i+1] * ref_weighted_time_value[2*i+1];
    est_abs_time_value[i] = est_weighted_time_value[2*i] * est_weighted_time_value[2*i] +
      est_weighted_time_value[2*i+1] * est_weighted_time_value[2*i+1];
  }
  free (ref_time_value) ;
  free (ref_weighted_time_value) ;
  free (est_time_value) ;
  free (est_weighted_time_value) ;

  return 0;
}  

/*****************************************************************************/

static int accumulateFrameEnergiesInSfb( Float          * buffer, 
                                          Float         * refBuffer,
                                          Float         est_sfb_sum[SFB_1], 
                                          Float         ref_sfb_sum[SFB_1],
                                          unsigned int  frameSize,
                                          pns_ptr       p)
{

  Float * est_abs_time_value = (Float*) calloc(frameSize,sizeof(Float));
  Float * ref_abs_time_value = (Float*) calloc(frameSize,sizeof(Float));
  unsigned int i ; 
  int k ;

  calculateEnergyOfFrame( buffer, refBuffer, est_abs_time_value, ref_abs_time_value, frameSize ) ;

  /* for each sfb  */
  /*    accumulate sum of each of the fft values  */

  for (i = 0; i < p->sfbsize; i++){
    ref_sfb_sum[i] = 0.0;
    est_sfb_sum[i] = 0.0;
    for (k = p->sfboff[i]; k < p->sfboff[i+1]; k++){
      ref_sfb_sum[i] += ref_abs_time_value[k];
      est_sfb_sum[i] += est_abs_time_value[k];
/*       printf("sfb %2i: %4i - %4i\n",i,sfboff[i],sfboff[i+1]-1); */
    }
  }
  free (est_abs_time_value) ;
  free (ref_abs_time_value) ;
 
  return 0;
}

/*****************************************************************************/

static int calculateSpectralConformance( Float          ** est_sfb_sum, 
                                          Float          ** ref_sfb_sum, 
                                          unsigned int   lastFrame,
                                          unsigned int   *wrongRatioCount,
                                          int            verbose,
                                          unsigned int   sfbsize)
{

  Float * est_sum = (Float*) calloc(lastFrame,sizeof(Float)), * ref_sum = (Float*) calloc(lastFrame,sizeof(Float)); 
  unsigned int i, k ; 
  critical_ratio cRatio;

  for ( k = 0; k < sfbsize ; k++){
    for ( i = 0; i < lastFrame; i++){
      est_sum[i] = est_sfb_sum[i][k] ;
      ref_sum[i] = ref_sfb_sum[i][k] ;
    }
    cRatio = calculateCriticalRatio( est_sum, ref_sum, lastFrame, k, wrongRatioCount, verbose );
    /*fprintf ( stderr, "\nscale factor band %d has critical ratio:  %f", k, cRatio.meanRatio); */
    /*fprintf ( stderr, "\nscale factor band has standard deviation critical ratio:  %f", cRatio.standardDeviationRatio); */
  }
  free (est_sum) ;
  free (ref_sum) ;
  return 0 ;
}

/*****************************************************************************/

static critical_ratio calculateCriticalRatio( Float             *est_sfb_sum, 
                                               Float            *ref_sfb_sum, 
                                               unsigned int     lastFrame,
                                               int              sfbnum,
                                               unsigned int     *wrongRatioCount,
                                               int              verbose)
{
    
  Float est_sum = 0.0 , ref_sum = 0.0 ;
  unsigned int i ; 
  critical_ratio cRatio = {0.0,0.0} ;

  est_sum = calculateMeanValue(est_sfb_sum, lastFrame);
  ref_sum = calculateMeanValue(ref_sfb_sum, lastFrame);

  if ( ref_sum != 0 ) {
    cRatio.meanRatio = 10.0f*log10(est_sum / ref_sum) ;
  }
  if (verbose || ( cRatio.meanRatio< -PNS_S_C1 || cRatio.meanRatio > PNS_S_C1) ) {
    fprintf (stderr, "\nC1: %5.2fdB", cRatio.meanRatio);
    fprintf(stderr, ", range: [%3.1fdB;%3.1fdB]",-PNS_S_C1,PNS_S_C1);
  }
  if ( cRatio.meanRatio< -PNS_S_C1 || cRatio.meanRatio > PNS_S_C1) {
    (*wrongRatioCount)++ ;
    fprintf ( stderr, ", range exceeded in sfb %2i !",sfbnum);  
  }
  
  est_sum = calculateStandardDeviation(est_sfb_sum, lastFrame );
  ref_sum = calculateStandardDeviation(ref_sfb_sum, lastFrame );
  if ( ref_sum != 0 ) {
    cRatio.standardDeviationRatio =  10.0f*log10(est_sum / ref_sum) ;
  }
  if (verbose || ( cRatio.standardDeviationRatio< -PNS_S_C2 || cRatio.standardDeviationRatio > PNS_S_C2) ) {
    fprintf (stderr, "\nC2: %5.2fdB", cRatio.standardDeviationRatio);
    fprintf(stderr, ", range: [%3.1fdB;%3.1fdB]",-PNS_S_C2,PNS_S_C2);
  }
  if ( cRatio.standardDeviationRatio < -PNS_S_C2 || cRatio.standardDeviationRatio > PNS_S_C2 ) {
    (*wrongRatioCount)++ ;
    fprintf ( stderr, ", range exceeded in sfb %2i !",sfbnum);
    
  }
  return cRatio ;
}

/*****************************************************************************/

static Float calculateMeanValue( Float          vector[],
                                 unsigned int   maxValue)
{

  Float sum = 0.0 ;
  unsigned int i ;
  for ( i = 0; i < maxValue; i++){
      sum += vector[i] ;
  }
  sum = sum / (maxValue) ;
  return sum ;
}

/*****************************************************************************/

static Float calculateStandardDeviation( Float          vector[],
                                         unsigned int   maxValue)
{
    
  Float mean = 0.0 ;
  unsigned int i ;

  Float standardDeviation = 0.0 ;

  for ( i = 0; i < maxValue; i++){
    mean += vector[i] / (maxValue) ;
  }
  for ( i = 0; i < maxValue; i++){
    standardDeviation += (vector[i] - mean) * (vector[i] - mean) / (maxValue-1) ;
  }
  standardDeviation = sqrt(standardDeviation) ;  
  
  return standardDeviation ;
}


/*****************************************************************************/
/*                                                                           */
/* Conformance 3: Temporal Criterium                                         */
/*                                                                           */
/*****************************************************************************/

int CalculateTemporalConformance( AFILE         *Pcmfile,
                                  AFILE         *refPcmfile,
                                  audiofile     Pcm_header,
                                  audiofile     refPcm_header,
                                  pns_ptr       p,
                                  int           delay,
                                  int           verbose)
{ 

  Float ** ref_sfb_sum ;
  Float ** est_sfb_sum ;
  float * buffer;
  float * refBuffer;
  unsigned int j = 0, i = 0;
  unsigned int  fileSize;
  long int numBlocks;
  unsigned int frameSize = p->pns_frameSize;
  Float *m_ConformanceEnergy3_ref = p->m_ConformanceEnergy3_ref;
  Float *m_ConformanceEnergy3_est = p->m_ConformanceEnergy3_est;
  unsigned int *m_Conformance3Counter = &(p->m_Conformance3Counter);
 
  fileSize = Pcm_header.nrofsamples / Pcm_header.channels;
  fileSize-=(delay>0 ? delay : 0);
  
  switch(frameSize)
    {
    case FRAME_SIZE_960_LONG:
    case FRAME_SIZE_960_SHORT:
        numBlocks = FRAME_SIZE_960_SHORT;
        break;
    default:
        numBlocks = FRAME_SIZE_1024_SHORT;
        break;
    }
  
  ref_sfb_sum = (Float**) calloc(fileSize/(numBlocks),sizeof(Float));
  est_sfb_sum = (Float**) calloc(fileSize/(numBlocks),sizeof(Float));
  for ( i = 0; i< (fileSize/(numBlocks)); i++){
    ref_sfb_sum[i] = (Float*) calloc(SFB_1,sizeof(Float));
    est_sfb_sum[i] = (Float*) calloc(SFB_1,sizeof(Float));
  }
  buffer = (float*) calloc(frameSize*2,sizeof(float));
  refBuffer = (float*) calloc(frameSize*2,sizeof(float));

  initTemporalConformance( fileSize,
                           &m_ConformanceEnergy3_ref,
                           &m_ConformanceEnergy3_est);
  
  for ( i=0; i< (int)(fileSize)-(frameSize) ; i=i+(frameSize)){
      
    get_audiosample ( refBuffer,
                      refPcmfile,
                      refPcm_header.channels,
                      i-(delay<0?delay:0),
                      frameSize);

    get_audiosample ( buffer,
                      Pcmfile,
                      Pcm_header.channels,
                      i+(delay>0?delay:0),
                      frameSize);
         
    calculateSquaredEnergy( buffer, 
                            refBuffer, 
                            frameSize, 
                            m_ConformanceEnergy3_ref, 
                            m_ConformanceEnergy3_est,
                            m_Conformance3Counter);
  }
  
  return calculateTemporalConformance( verbose,
                                       m_ConformanceEnergy3_ref, 
                                       m_ConformanceEnergy3_est,
                                       m_Conformance3Counter);
}

/*****************************************************************************/

static int calculateSquaredEnergy( float           *buffer, 
                                    float           *refBuffer,
                                    unsigned int    frameSize,
                                    Float           *m_ConformanceEnergy3_ref,
                                    Float           *m_ConformanceEnergy3_est,
                                    unsigned int    *m_Conformance3Counter) 
{
    
  int * ref_time_value = (int*) calloc(frameSize,sizeof(int));
  int * est_time_value = (int*) calloc(frameSize,sizeof(int));
 
  unsigned int i, k;

  /* convert bit buffers to int */
  for ( i = 0; i <  frameSize; i++ ){
    ref_time_value[i] =  (int)refBuffer[i];   
    ref_time_value[i]++;
    est_time_value[i] =  (int)buffer[i];
    est_time_value[i]++;
  }

  for ( i = 0; i < frameSize/(BLOCK_LENGTH); i++){
    for ( k = 0 ; k < BLOCK_LENGTH ; k++){
      m_ConformanceEnergy3_ref[*m_Conformance3Counter] += ref_time_value[i*BLOCK_LENGTH+k] * ref_time_value[i*BLOCK_LENGTH+k] ;
      m_ConformanceEnergy3_est[*m_Conformance3Counter] += est_time_value[i*BLOCK_LENGTH+k] * est_time_value[i*BLOCK_LENGTH+k] ;
    }
    (*m_Conformance3Counter)++;
  }

  free (ref_time_value) ;
  free (est_time_value) ;

  return 0;
}

/*****************************************************************************/

static int calculateTemporalConformance( int            verbose,
                                          Float          *m_ConformanceEnergy3_ref,
                                          Float          *m_ConformanceEnergy3_est,
                                          unsigned int   *m_Conformance3Counter)
{
  
  Float conf_crit, percent_5 = 0.0, percent_10 = 0.0 ;
  unsigned int i=0, percent_count_5=0, percent_count_10 = 0 ;

  unsigned int percent_count_4 = 0, percent_count_6 = 0 ; 
  Float percent_4 = 0.0, percent_6 = 0.0 ;

  for ( i = 0 ; i < (*m_Conformance3Counter); i++){
    conf_crit = 10.0 * log10(m_ConformanceEnergy3_est[i] / m_ConformanceEnergy3_ref[i]);
    
    if ( fabs(conf_crit ) >  5 )
      percent_count_5++;
    if ( fabs(conf_crit ) >  10 )
      percent_count_10++;

    if ( fabs(conf_crit ) >  6 )
      percent_count_6++;
    if ( fabs(conf_crit ) >  4 )
      percent_count_4++;
  }

  percent_5  = 100.0-((Float)percent_count_5  / (Float)(*m_Conformance3Counter))*100.0 ;
  percent_10 = 100.0-((Float)percent_count_10 / (Float)(*m_Conformance3Counter))*100.0 ;

  if ( (verbose) || ( percent_5 < PNS_T_C1 ) ) {
    fprintf(stderr,"\n C1: %3.1f%%",percent_5);
    fprintf(stderr,", threshold: >=%4.1f%%",PNS_T_C1);
  }
  if ( percent_5 < PNS_T_C1 ) {
    fprintf(stderr,", threshold exceeded !");
  }
  if ( (verbose) || ( percent_10 < PNS_T_C2 ) ) {
    fprintf(stderr," \n C2: %3.1f%%",percent_10); 
    fprintf(stderr,", threshold: >=%4.1f%%",PNS_T_C2);
  }
  if ( percent_10 < PNS_T_C2 ) {
    fprintf(stderr,", threshold exceeded !");
  }

  if ( ( percent_5 >= PNS_T_C1 )  &&  ( percent_10 >= PNS_T_C2 ) )
    fprintf(stderr,"\n\n result ok \n\n");
  else {
    fprintf(stderr,"\n result wrong \n\n");
    return 1;
  }
  return 0;
}

/*****************************************************************************/

