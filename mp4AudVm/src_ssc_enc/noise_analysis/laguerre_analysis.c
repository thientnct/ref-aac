/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Felip Riera-Palou, Philips Research Eindhoven
* Andy Gerrits, Philips Research Eindhoven
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
*

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright © 2001.

Source file: laguerre_analysis.c

Required libraries: <none>

Authors:

Changes:
************************************************************************/

/******************************************************************************
 *
 *   Module              : Laguerre Analysis implementation
 *
 *   Description         : Implementation of the Laguerre estimation process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_Lag_
 *
 *
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"

#include "cexcept.h"
#include "SSC_Vector.h"
#include "win.h"

#include "laguerre_analysis.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/
static void LOCAL_levin
        (           
           SSC_LAG_STATE* const pms, 
		          Double* const pData
		);
static void LOCAL_laguerre_analysis
		( 
           SSC_LAG_STATE* const pms, 
		   const Double* const pData,
		   const SInt countData
		);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


void SSC_LAG_Initialise ( void )
{
}


void SSC_LAG_Free ( void )
{
}


SSC_LAG_STATE* SSC_LAG_CreateInstance ( const SInt denLen,	const Double lagPole, SSCLOG* pLog )
{
    SSC_LAG_STATE* pms = NULL ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_LAG_STATE), 1, "LAG Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for LAG instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        SSC_LAG_Initialise();

        /* get the log object from the caller for logging */
        pms->pLog    = pLog;

        /* create memory for storing the window that is specified */
        pms->a = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 2);
        if ( pms->a == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->a);
		
        /* create memory for storing the window that is specified */
        pms->b = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 2);
        if ( pms->b == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->b);

        /* create memory for storing the window that is specified */
        pms->states = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, denLen);
        if ( pms->states == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->states);
		
        /* create memory for storing the window that is specified */
        pms->out = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, denLen);
        if ( pms->out == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->out);
		
        /* create memory for storing the window that is specified */
        pms->r = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, denLen);
        if ( pms->r == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->r);
		
        /* create memory for storing the window that is specified */
        pms->p = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, denLen);
        if ( pms->p == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->p);
		
        /* create memory for storing the window that is specified */
        pms->y = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, denLen+1);
        if ( pms->y == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->y);
		
        /* create memory for storing the window that is specified */
        pms->z = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, denLen+1);
        if ( pms->z == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->z);
		
        /* Set default values for other parameters */
		pms->Lp     = denLen;
		pms->lambda = lagPole;
		pms->EZ     = 100;
		pms->b1     = 0.0;
		pms->delta  = 1e-08;
        pms->alpha  = 0.0;
		pms->beta   = 0.0;
		pms->mu     = 0.0;
		pms->inprod = 0.0;
    }
    Catch(Error)
    {
        pms = SSC_LAG_DestroyInstance( pms );
    }

    return pms;
	
}


SSC_LAG_STATE* SSC_LAG_DestroyInstance( SSC_LAG_STATE* const pms )
{
    if ( pms != NULL )
    {
        if ( pms->a != NULL )
        {
            pms->a = SSC_VECT_DestroyInstance(pms->a);
        }
        if ( pms->b != NULL )
        {
            pms->b = SSC_VECT_DestroyInstance(pms->b);
        }
        if ( pms->states != NULL )
        {
            pms->states = SSC_VECT_DestroyInstance(pms->states);
        }
        if ( pms->out != NULL )
        {
            pms->out = SSC_VECT_DestroyInstance(pms->out);
        }
        if ( pms->r != NULL )
        {
            pms->r = SSC_VECT_DestroyInstance(pms->r);
        }
        if ( pms->p != NULL )
        {
            pms->p = SSC_VECT_DestroyInstance(pms->p);
        }
        if ( pms->y != NULL )
        {
            pms->y = SSC_VECT_DestroyInstance(pms->y);
        }
        if ( pms->z != NULL )
        {
            pms->z = SSC_VECT_DestroyInstance(pms->z);
        }

        SSC_LAG_Free();

        SSC_FREE( pms, "LAG Instance" );
    }

    /* return NULL to indicate that memory is freed */
    return NULL;
}

void SSC_LAG_Analysis( SSC_LAG_STATE* const pms, const Double* const pData,
		const SInt countData, Double* const pLAGData )
{
    LOCAL_laguerre_analysis(pms, pData, countData);
	LOCAL_levin(pms, pLAGData);
}


static void LOCAL_levin
        (           
           SSC_LAG_STATE* const pms, 
		          Double* const pData
		)
{			
    if ( pms != NULL )
    {

        Double *RVect = pms->r;		/* correlation coeffs */
	    Double *p     = pms->p;		/* arbitrary p vector */
	    SInt NSize    = pms->Lp;	
	    Double * result_coef	= pData; /* Out: resulting coeffs   */
				
        Double *y = pms->y;
        Double *z = pms->z;
		Double alpha = pms->alpha;
		Double beta = pms->beta;
		Double mu = pms->mu;
		Double inprod = pms->inprod;

        SInt k,c1;

		for (c1=0;c1<NSize+1;c1++)
		{
			y[c1]=0.0;				
			z[c1]=0.0;
		}


		y[0]=-RVect[1]/RVect[0];			
		result_coef[0]=p[0]/RVect[0];
		alpha=-RVect[1]/RVect[0];
		beta=RVect[0];

		for (k=1;k<NSize;k++)
		{

			beta=(1-alpha*alpha)*beta;
			inprod=0.0;
			for (c1=1;c1<=k;c1++)
			{
				inprod=inprod+RVect[c1]*result_coef[k-c1];
			}
    			mu=(p[k]-inprod)/beta;

			for (c1=0;c1<k;c1++)
			{
				z[c1]=result_coef[c1]+mu*y[k-1-c1];
			}       
			for (c1=0;c1<k;c1++)
			{
				result_coef[c1]=z[c1];
			}
  			result_coef[k]=mu;

			if (k<NSize-1)
			{
				inprod=0.0;
				for (c1=1;c1<=k;c1++)
				{
					inprod=inprod+RVect[c1]*y[k-c1];
				}

				alpha=-(RVect[k+1]+inprod)/beta;
				for (c1=0;c1<k;c1++)
				{
					z[c1]=y[c1]+alpha*y[k-1-c1];
				}
    				for(c1=0;c1<k;c1++)
				{
					y[c1]=z[c1];
				}
				y[k]=alpha;
			}
		}
	} 
}


static void LOCAL_laguerre_analysis
		( 
           SSC_LAG_STATE* const pms, 
		   const Double* const pData,
		   const SInt countData
		)

{
    if ( pms != NULL )
    {
		const Double * const x = pData;		
		SInt   N  = countData;			
		SInt   Lp = pms->Lp;		
		Double lambda = pms->lambda;		
		Double *r = pms->r;		
		Double *p = pms->p;		

		Double *a = pms->a;
		Double *b = pms->b;
		Double b1 = pms->b1;
		Double *states = pms->states;
		Double *out = pms->out;
		Double delta= pms->delta;

		SInt i,k;

		for (i=0;i<Lp;i++)
		{
			states[i]=0.0;
			out[i]=0.0;
			r[i]=0.0;
			p[i]=0.0;
		}

		a[0]=1.0;
		a[1]=-1.0*lambda;

		b[0]=-1.0*lambda;
		b[1]=1.0;

		b1=sqrt(1-pow(fabs(lambda),2.0));

		for (i=1; i<N; i++)					/* Real data */
		{
			out[0]=b1*x[i-1]+states[0];
   			states[0]=-1.0*a[1]*out[0];
        		r[0]=r[0]+out[0]*out[0];
			p[0]=p[0]+out[0]*x[i];

			for (k=1; k<Lp; k++)
			{
				out[k]=b[0]*out[k-1]+states[k];
				states[k]=b[1]*out[k-1]-a[1]*out[k];
				r[k]=r[k]+out[k]*out[0];
				p[k]=p[k]+out[k]*x[i];
			}
		}


		out[0]=b1*x[N-1]+states[0];				/* Sample N-1 (last)*/
		states[0]=-1.0*a[1]*out[0];
		r[0]=r[0]+out[0]*out[0];

		for (k=1; k<Lp; k++)
		{
			out[k]=b[0]*out[k-1]+states[k];
			states[k]=b[1]*out[k-1]-a[1]*out[k];
			r[k]=r[k]+out[k]*out[0];
		}


		for (i=0; i<pms->EZ; i++)		       /* Padded zeros data*/
		{
			out[0]=states[0];
			states[0]=-1.0*a[1]*out[0];
			r[0]=r[0]+out[0]*out[0];

			for (k=1; k<Lp; k++)
			{
				out[k]=b[0]*out[k-1]+states[k];
				states[k]=b[1]*out[k-1]-a[1]*out[k];
				r[k]=r[k]+out[k]*out[0];
			}
		}

		r[0]=r[0]+delta*r[0]+delta;		/* Perturbation to avoid */
									    /* problems when r=0	 */
		
		
    }			
}	
	








