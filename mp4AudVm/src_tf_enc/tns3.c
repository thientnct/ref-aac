/**********************************************************************
 
This software module was originally developed by Texas Instruments
and edited by         in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard
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
 
Copyright (c) 1997.
**********************************************************************/
/*********************************************************************
 *
 * tns.h  -  AAC temporal noise shaping
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 *
 * Changes:
 * 20-oct-97   CL   Initial revision.
 *
**********************************************************************/

#include <math.h>
#include <stdlib.h>

#include "interface.h"           /* handler, defines, enums */
#include "tf_mainHandle.h"

#include "common_m4a.h"
#include "tns3.h"

#define mymax(a,b) ((a)>(b)?(a):(b))
#define mymin(a,b) ((a)<(b)?(a):(b))
/***********************************************/
/* TNS Profile/Frequency Dependent Parameters  */
/***********************************************/
static long tnsSupportedSamplingRates[13] = 
  {8000,11025,12000,16000,22050,24000,32000,44100,48000,64000,88200,96000,0};

/* Limit bands to > 1.5 kHz */
/*static unsigned short tnsMinBandNumberLong[12] = 
  { 26, 25, 24, 20, 23, 22, 17, 14, 13, 12, 9, 8 };
static unsigned short tnsMinBandNumberShort[12] = 
  { 10, 9, 8, 8, 5, 4, 3, 3, 2, 2, 1, 1 }; */
       
/* Limit bands to > 2.0 kHz */
static unsigned short tnsMinBandNumberLong[12] = 
  { 31, 30, 28, 24, 26, 25, 20, 17, 16, 15, 12, 11 };
static unsigned short tnsMinBandNumberShort[12] = 
  { 12, 10, 10, 8, 6, 6, 4, 3, 3, 2, 2, 2 };
       
/**************************************/
/* Main/Low Profile TNS Parameters    */
/**************************************/
static unsigned short tnsMaxBandsLongMainLow[12] = 
  { 39, 42, 42, 42, 46, 46, 51, 42, 40, 34, 31, 31 };

static unsigned short tnsMaxBandsShortMainLow[12] = 
  { 14, 14, 14, 14, 14, 14, 14, 14, 14, 10, 9, 9 };

static unsigned short tnsMaxOrderLongMain = 20;
static unsigned short tnsMaxOrderLongLow = 12;
static unsigned short tnsMaxOrderShortMainLow = 7;

/**************************************/
/* SSR Profile TNS Parameters         */
/**************************************/
static unsigned short tnsMaxBandsLongSSR[12] = 
  { 19, 23, 23, 23, 29, 29, 26, 26, 26, 27, 28, 28 };

static unsigned short tnsMaxBandsShortSSR[12] = 
  { 7, 8, 8, 8, 7, 7, 6, 6, 6, 7, 7, 7 };

static unsigned short tnsMaxOrderLongSSR = 12;
static unsigned short tnsMaxOrderShortSSR = 7;

#ifdef VERSION2
/**************************************/
/* Low Delay mode TNS Parameters    */
/**************************************/
static unsigned short tnsMaxBandsLowDelay480[12] = 
  { 0, 0, 0, 0, 30, 30, 37, 32, 31, 0, 0, 0 };

static unsigned short tnsMaxBandsLowDelay512[12] = 
  { 0, 0, 0, 0, 31, 31, 37, 32, 31, 0, 0, 0 };
#endif

/*****************************************************/
/* InitTns:                                          */
/*****************************************************/
void TnsInit(long samplingRate,enum AAC_PROFILE profile,TNS_INFO* tnsInfo, QC_MOD_SELECT qc_select, int block_size_samples) 
{
  int fsIndex=0;

  /* Determine if sampling rate is supported */
  while (samplingRate!=tnsSupportedSamplingRates[fsIndex]) {
    if (!tnsSupportedSamplingRates[fsIndex]) {
      CommonExit( 1, "InitTns: Sampling rate of %d not supported in TNS.\nExiting program...\n",samplingRate);
    } 
    fsIndex++;
  }
  
#ifdef VERSION2
  if (qc_select == AAC_LD) {
    switch ( block_size_samples ){
    case 480:
      tnsInfo->tnsMaxBandsLong = tnsMaxBandsLowDelay480[fsIndex];
      break;
    case 512:
      tnsInfo->tnsMaxBandsLong = tnsMaxBandsLowDelay512[fsIndex];
      break;
    default:
      CommonExit( 1, "TnsInit: wrong block size for Low Delay mode.\n");
    }
    tnsInfo->tnsMaxOrderLong = tnsMaxOrderLongLow;
  } else
#endif

  switch( profile ) {
    case MAIN :
      tnsInfo->tnsMaxBandsLong = tnsMaxBandsLongMainLow[fsIndex];
      tnsInfo->tnsMaxBandsShort = tnsMaxBandsShortMainLow[fsIndex];
      tnsInfo->tnsMaxOrderLong = tnsMaxOrderLongMain;
      tnsInfo->tnsMaxOrderShort = tnsMaxOrderShortMainLow;
      break;
    case LOW :
      tnsInfo->tnsMaxBandsLong = tnsMaxBandsLongMainLow[fsIndex];
      tnsInfo->tnsMaxBandsShort = tnsMaxBandsShortMainLow[fsIndex];
      tnsInfo->tnsMaxOrderLong = tnsMaxOrderLongLow;
      tnsInfo->tnsMaxOrderShort = tnsMaxOrderShortMainLow;
      break;
    case SSR :
      tnsInfo->tnsMaxBandsLong = tnsMaxBandsLongSSR[fsIndex];
      tnsInfo->tnsMaxBandsShort = tnsMaxBandsShortSSR[fsIndex];
      tnsInfo->tnsMaxOrderLong = tnsMaxOrderLongSSR;
      tnsInfo->tnsMaxOrderShort = tnsMaxOrderShortSSR;
      break;
    default:
      CommonExit( 1, "InitTns: Profile not supported for TNS.\nExiting program\n");
  }
  tnsInfo->tnsMinBandNumberLong = tnsMinBandNumberLong[fsIndex];
  tnsInfo->tnsMinBandNumberShort = tnsMinBandNumberShort[fsIndex];
}


/*****************************************************/
/* TnsEncode:                                        */
/*****************************************************/
void TnsEncode(int numberOfBands,       /* Number of bands per window */
	       int maxSfb,              /* max_sfb */
	       WINDOW_SEQUENCE blockType,   /* block type */
	       int* sfbOffsetTable,     /* Scalefactor band offset table */
	       double* spec,            /* Spectral data array */
	       TNS_INFO* tnsInfo)       /* TNS info */
{
  int numberOfWindows,windowSize;
  int startBand,stopBand,order;    /* Bands over which to apply TNS */
  int lengthInBands;               /* Length to filter, in bands */
  int w;
  int startIndex,length;
  double gain;

  switch( blockType ) {
  case EIGHT_SHORT_SEQUENCE :
    numberOfWindows = NSHORT;
    windowSize = SN2;
    startBand = tnsInfo->tnsMinBandNumberShort;
    stopBand = numberOfBands; 
    lengthInBands = stopBand-startBand;
    order = tnsInfo->tnsMaxOrderShort;
    startBand = mymin(startBand,tnsInfo->tnsMaxBandsShort);
    stopBand = mymin(stopBand,tnsInfo->tnsMaxBandsShort);
    break;

  default: 
    numberOfWindows = 1;
    windowSize = LN2;
    startBand = tnsInfo->tnsMinBandNumberLong;
    stopBand = numberOfBands;
    lengthInBands = stopBand - startBand;
    order = tnsInfo->tnsMaxOrderLong;
    startBand = mymin(startBand,tnsInfo->tnsMaxBandsLong);
    stopBand = mymin(stopBand,tnsInfo->tnsMaxBandsLong);
    break;
  }

  /* Make sure that start and stop bands < maxSfb */
  /* Make sure that start and stop bands >= 0 */
  startBand = mymin(startBand,maxSfb);
  stopBand = mymin(stopBand,maxSfb);
  startBand = mymax(startBand,0);
  stopBand = mymax(stopBand,0);

/*  fprintf(stderr,"(%d)TNS Start %5d : End %5d : lengthInBands %5d\n",(int)blockType,startBand,stopBand,lengthInBands);
  fprintf(stderr,"order %5d\n",order);*/
  
  tnsInfo->tnsDataPresent=0;     /* default TNS not used */
  
  /* Perform analysis and filtering for each window */
  for (w=0;w<numberOfWindows;w++) {

    TNS_WINDOW_DATA* windowData = &tnsInfo->windowData[w];
    TNS_FILTER_DATA* tnsFilter = windowData->tnsFilter;
    double* k = tnsFilter->kCoeffs;    /* reflection coeffs */
    double* a = tnsFilter->aCoeffs;    /* prediction coeffs */

    windowData->numFilters=0;
    windowData->coefResolution = DEF_TNS_COEFF_RES;
    startIndex = w * windowSize + sfbOffsetTable[startBand];
    length = sfbOffsetTable[stopBand] - sfbOffsetTable[startBand];
    gain = LevinsonDurbin(order,length,&spec[startIndex],k);

    if (gain>DEF_TNS_GAIN_THRESH) {  /* Use TNS */
      int truncatedOrder;
      windowData->numFilters++;
      tnsInfo->tnsDataPresent=1;
      tnsFilter->direction = 0;
      tnsFilter->coefCompress = 0;
      tnsFilter->length = lengthInBands;
      QuantizeReflectionCoeffs(order,DEF_TNS_COEFF_RES,k,tnsFilter->index);
      truncatedOrder = TruncateCoeffs(order,DEF_TNS_COEFF_THRESH,k);
      tnsFilter->order = truncatedOrder;
      StepUp(truncatedOrder,k,a);    /* Compute predictor coefficients */
      TnsInvFilter(length,&spec[startIndex],tnsFilter);      /* Filter */      
    }
  }  
}


/*****************************************************/
/* TnsEncode2:                                       */
/* This is a stripped-down version of TnsEncode()    */
/* which performs TNS filtering only (no TNS calc.)  */
/*****************************************************/
void TnsEncode2(int numberOfBands,       /* Number of bands per window */
                int maxSfb,              /* max_sfb */
                WINDOW_SEQUENCE blockType,   /* block type */
                int* sfbOffsetTable,     /* Scalefactor band offset table */
                double* spec,            /* Spectral data array */
                TNS_INFO* tnsInfo,       /* TNS info */
                int decodeFlag)          /* Analysis ot synthesis filter. */
{
  int numberOfWindows,windowSize;
  int startBand,stopBand;                /* Bands over which to apply TNS */
  int w;
  int startIndex,length;

  switch( blockType ) {
  case EIGHT_SHORT_SEQUENCE :
    numberOfWindows = NSHORT;
    windowSize = SN2;
    startBand = tnsInfo->tnsMinBandNumberShort;
    stopBand = numberOfBands; 
    startBand = mymin(startBand,tnsInfo->tnsMaxBandsShort);
    stopBand = mymin(stopBand,tnsInfo->tnsMaxBandsShort);
    break;

  default: 
    numberOfWindows = 1;
    windowSize = LN2;
    startBand = tnsInfo->tnsMinBandNumberLong;
    stopBand = numberOfBands;
    startBand = mymin(startBand,tnsInfo->tnsMaxBandsLong);
    stopBand = mymin(stopBand,tnsInfo->tnsMaxBandsLong);
    break;
  }
     
  /* Make sure that start and stop bands < maxSfb */
  /* Make sure that start and stop bands >= 0 */
  startBand = mymin(startBand,maxSfb);
  stopBand = mymin(stopBand,maxSfb);
  startBand = mymax(startBand,0);
  stopBand = mymax(stopBand,0);

  
  /* Perform filtering for each window */
  for (w=0;w<numberOfWindows;w++) {

    TNS_WINDOW_DATA* windowData = &tnsInfo->windowData[w];
    TNS_FILTER_DATA* tnsFilter = windowData->tnsFilter;

    startIndex = w * windowSize + sfbOffsetTable[startBand];
    length = sfbOffsetTable[stopBand] - sfbOffsetTable[startBand];

    if (tnsInfo->tnsDataPresent  &&  windowData->numFilters) {  /* Use TNS */
      if (decodeFlag)
        TnsFilter(length,&spec[startIndex],tnsFilter);   
      else
        TnsInvFilter(length,&spec[startIndex],tnsFilter);   
    }
  }  
}


void TnsEncodeShortBlock(int numberOfBands,       /* Number of bands per window */
			 int maxSfb,              /* max_sfb */
			 int which_subblock,      /* block number */
			 int* sfbOffsetTable,     /* Scalefactor band offset table */
			 double* spec,            /* Spectral data array */
			 TNS_INFO* tnsInfo)       /* TNS info */
{
  int windowSize;
  int startBand,stopBand;    /* Bands over which to apply TNS */
  int w;
  int startIndex,length;

  windowSize = SN2;
  startBand = tnsInfo->tnsMinBandNumberShort;
  stopBand = numberOfBands; 
  startBand = min(startBand,tnsInfo->tnsMaxBandsShort);
  stopBand = min(stopBand,tnsInfo->tnsMaxBandsShort);
     
  /* Make sure that start and stop bands < maxSfb */
  /* Make sure that start and stop bands >= 0 */
  startBand = min(startBand,maxSfb);
  stopBand = min(stopBand,maxSfb);
  startBand = max(startBand,0);
  stopBand = max(stopBand,0);

  
  /* Perform filtering for each window */
  for (w = which_subblock; w < (which_subblock + 1); w++) {

    TNS_WINDOW_DATA* windowData = &tnsInfo->windowData[w];
    TNS_FILTER_DATA* tnsFilter = windowData->tnsFilter;

    startIndex = sfbOffsetTable[startBand];
    length = sfbOffsetTable[stopBand] - sfbOffsetTable[startBand];

    if (tnsInfo->tnsDataPresent  &&  windowData->numFilters) {  /* Use TNS */
      TnsInvFilter(length,&spec[startIndex],tnsFilter);   
    }
  }  
}

/*****************************************************/
/* TnsFilter:                                        */
/*   Filter the given spec with specified length     */
/*   using the coefficients specified in filter.     */
/*   Not that the order and direction are specified  */
/*   withing the TNS_FILTER_DATA structure.          */
/*****************************************************/
void TnsFilter(int length,double* spec,TNS_FILTER_DATA* filter) {
	int i,j,k=0;
	int order=filter->order;
	double* a=filter->aCoeffs;

	/* Determine loop parameters for given direction */
	if (filter->direction) {

		/* Startup, initial state is zero */
		for (i=length-2;i>(length-1-order);i--) {
			k++;
			for (j=1;j<=k;j++) {
				spec[i]-=spec[i+j]*a[j];
			}
		}
		
		/* Now filter completely inplace */
		for	(i=length-1-order;i>=0;i--) {
			for (j=1;j<=order;j++) {
				spec[i]-=spec[i+j]*a[j];
			}
		}


	} else {

		/* Startup, initial state is zero */
		for (i=1;i<order;i++) {
			for (j=1;j<=i;j++) {
				spec[i]-=spec[i-j]*a[j];
			}
		}
		
		/* Now filter completely inplace */
		for	(i=order;i<length;i++) {
			for (j=1;j<=order;j++) {
				spec[i]-=spec[i-j]*a[j];
			}
		}
	}
}


/********************************************************/
/* TnsInvFilter:                                        */
/*   Inverse filter the given spec with specified       */
/*   length using the coefficients specified in filter. */
/*   Not that the order and direction are specified     */
/*   withing the TNS_FILTER_DATA structure.             */
/********************************************************/
void TnsInvFilter(int length,double* spec,TNS_FILTER_DATA* filter) {
	int i,j,k=0;
	int order=filter->order;
	double* a=filter->aCoeffs;
	double* temp;

    temp = (double *) malloc (length * sizeof (double));
    if (!temp) {
      CommonExit( 1, "TnsInvFilter: Could not allocate memory for TNS array\nExiting program...\n");
    }

	/* Determine loop parameters for given direction */
	if (filter->direction) {

		/* Startup, initial state is zero */
		temp[length-1]=spec[length-1];
		for (i=length-2;i>(length-1-order);i--) {
			temp[i]=spec[i];
			k++;
			for (j=1;j<=k;j++) {
				spec[i]+=temp[i+j]*a[j];
			}
		}
		
		/* Now filter the rest */
		for	(i=length-1-order;i>=0;i--) {
			temp[i]=spec[i];
			for (j=1;j<=order;j++) {
				spec[i]+=temp[i+j]*a[j];
			}
		}


	} else {

		/* Startup, initial state is zero */
		temp[0]=spec[0];
		for (i=1;i<order;i++) {
			temp[i]=spec[i];
			for (j=1;j<=i;j++) {
				spec[i]+=temp[i-j]*a[j];
			}
		}
		
		/* Now filter the rest */
		for	(i=order;i<length;i++) {
			temp[i]=spec[i];
			for (j=1;j<=order;j++) {
				spec[i]+=temp[i-j]*a[j];
			}
		}
	}
	free(temp);
}





/*****************************************************/
/* TruncateCoeffs:                                   */
/*   Truncate the given reflection coeffs by zeroing */
/*   coefficients in the tail with absolute value    */
/*   less than the specified threshold.  Return the  */
/*   truncated filter order.                         */
/*****************************************************/
int TruncateCoeffs(int fOrder,double threshold,double* kArray) {
  int i;

  for (i=fOrder;i>=0;i--) {
    kArray[i] = (fabs(kArray[i])>threshold) ? kArray[i] : 0.0;
    if (kArray[i]!=0.0) {
      return i;
    }
  }
  return ( 0 );
}

/*****************************************************/
/* QuantizeReflectionCoeffs:                         */
/*   Quantize the given array of reflection coeffs   */
/*   to the specified resolution in bits.            */
/*****************************************************/
void QuantizeReflectionCoeffs(int fOrder,
			      int coeffRes,
			      double* kArray,
			      int* indexArray) {

	double iqfac,iqfac_m;
	int i;

	iqfac = ((1<<(coeffRes-1))-0.5)/(PI/2);
	iqfac_m = ((1<<(coeffRes-1))+0.5)/(PI/2);

	/* Quantize and inverse quantize */
	for (i=1;i<=fOrder;i++) {
		indexArray[i] = (int)(0.5+(asin(kArray[i])*((kArray[i]>=0)?iqfac:iqfac_m)));
		kArray[i] = sin((double)indexArray[i]/((indexArray[i]>=0)?iqfac:iqfac_m));
	}
}

/*****************************************************/
/* Autocorrelation,                                  */
/*   Compute the autocorrelation function            */
/*   estimate for the given data.                    */
/*****************************************************/
void Autocorrelation(int maxOrder,        /* Maximum autocorr order */
		     int dataSize,		  /* Size of the data array */
		     double* data,		  /* Data array */
		     double* rArray) {	  /* Autocorrelation array */

  int order,index;

  for (order=0;order<=maxOrder;order++) {
    rArray[order]=0.0;
    for (index=0;index<dataSize;index++) {
      rArray[order]+=data[index]*data[index+order];
    }
    dataSize--;
  }
}



/*****************************************************/
/* LevinsonDurbin:                                   */
/*   Compute the reflection coefficients for the     */
/*   given data using LevinsonDurbin recursion.      */
/*   Return the prediction gain.                     */
/*****************************************************/
double LevinsonDurbin(int fOrder,          /* Filter order */
					  int dataSize,		   /* Size of the data array */
					  double* data,		   /* Data array */
					  double* kArray) {	   /* Reflection coeff array */

	int order,i;
	double signal;
	double error, kTemp;				/* Prediction error */
	double aArray1[TNS_MAX_ORDER+1];	/* Predictor coeff array */
	double aArray2[TNS_MAX_ORDER+1];	/* Predictor coeff array 2 */
	double rArray[TNS_MAX_ORDER+1];		/* Autocorrelation coeffs */
	double* aPtr = aArray1;				/* Ptr to aArray1 */
	double* aLastPtr = aArray2;			/* Ptr to aArray2 */
	double* aTemp;

	/* Compute autocorrelation coefficients */
	Autocorrelation(fOrder,dataSize,data,rArray);
	signal=rArray[0];	/* signal energy */

	/* Set up pointers to current and last iteration */ 
	/* predictor coefficients.						 */
	aPtr = aArray1;
	aLastPtr = aArray2;
	/* If there is no signal energy, return */
	if (!signal) {
	  kArray[0]=1.0;
	  for (order=1;order<=fOrder;order++) {
	    kArray[0]=0.0;
	  }
	  return 0;
	
	} else {

	  /* Set up first iteration */
	  kArray[0]=1.0;
	  aPtr[0]=1.0;		/* Ptr to predictor coeffs, current iteration*/
	  aLastPtr[0]=1.0;	/* Ptr to predictor coeffs, last iteration */
	  error=rArray[0];
	  
	  /* Now perform recursion */
	  for (order=1;order<=fOrder;order++) {
	    kTemp = aLastPtr[0]*rArray[order-0];
	    for (i=1;i<order;i++) {
	      kTemp += aLastPtr[i]*rArray[order-i];
	    }
	    kTemp = -kTemp/error;
	    kArray[order]=kTemp;
	    aPtr[order]=kTemp;
	    for (i=1;i<order;i++) {
	      aPtr[i] = aLastPtr[i] + kTemp*aLastPtr[order-i];
	    }
	    error = error * (1 - kTemp*kTemp);
	    
	    /* Now make current iteration the last one */
	    aTemp=aLastPtr;
	    aLastPtr=aPtr;		/* Current becomes last */
	    aPtr=aTemp;			/* Last becomes current */
	  }
	  return signal/error;	/* return the gain */
	}
}


/*****************************************************/
/* StepUp:                                           */
/*   Convert reflection coefficients into            */
/*   predictor coefficients.                         */
/*****************************************************/
void StepUp(int fOrder,double* kArray,double* aArray) {

	double aTemp[TNS_MAX_ORDER+2];
	int i,order;

	aArray[0]=1.0;
	aTemp[0]=1.0;
	for (order=1;order<=fOrder;order++) {
		aArray[order]=0.0;
		for (i=1;i<=order;i++) {
			aTemp[i] = aArray[i] + kArray[order]*aArray[order-i];
		}
		for (i=1;i<=order;i++) {
			aArray[i]=aTemp[i];
		}
	}
}


/* Former test code....
main() {
	int i;
	double k[TNS_MAX_ORDER+1];
	double a[TNS_MAX_ORDER+1];
	int ind[TNS_MAX_ORDER+1];
	double e;
	#define DATA_SIZE 1024
	double testData[DATA_SIZE];

	for (i=0;i<DATA_SIZE;i++) {
		testData[i]=sin((double)i/10*2*PI);
		testData[i]+=sin((double)i/5*2*PI);
	}

	e=LevinsonDurbin(TNS_MAX_ORDER,DATA_SIZE,testData,k);
	for (i=0;i<=TNS_MAX_ORDER;i++) {
		fprintf(stdout,"k[%d]=%f\n",i,k[i]);
	}
	fprintf(stdout,"Filter order is:%d\n",TruncateCoeffs(TNS_MAX_ORDER,DEF_TNS_COEFF_THRESH,k));
	for (i=0;i<=TNS_MAX_ORDER;i++) {
		fprintf(stdout,"k[%d]=%f\n",i,k[i]);
	}
	QuantizeReflectionCoeffs(TNS_MAX_ORDER,16,k,ind);
    StepUp(TNS_MAX_ORDER,k,a);

	return 0;
}

*/












