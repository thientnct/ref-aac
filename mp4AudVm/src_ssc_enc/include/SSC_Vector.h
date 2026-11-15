/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
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

Copyright  2001.

Source file: SSC_vector.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
************************************************************************/

/**************************************************************************
 *
 * VECTOR module implementation
 *
 * SSC_Vector.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __SSC_Vect_h
#define __SSC_Vect_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "SSC_Log.h"
#include "SSC_Vector.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
static const Char SSC_VECT_BEGIN[] = "VEC" ;
static const Char SSC_VECT_END  [] = "TOR" ;

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef enum _SSC_VECT_TYPE
{
    SSC_VECT_FLOAT  = 0,
    SSC_VECT_DOUBLE = 1
}SSC_VECT_TYPE;

/* be sure this number is a multiple of the largest type known by the
 * compiler; at the moment this is 8 bytes */
/* also be sure that all members fit into this number of bytes*/
#define byteLengthInfoStruct (64)
typedef struct vectinfo_tag
{
    Char indentB[3];
    SSCLOG* pLog;

    SSC_VECT_TYPE type;
    SInt length;

    char dummy[byteLengthInfoStruct - sizeof(SSC_VECT_BEGIN)  /* identB*/
                                    - sizeof(SSCLOG*)         /* pLog */
                                    - sizeof(SSC_VECT_TYPE)   /* type */
                                    - sizeof(SInt)            /* length */
                                    - sizeof(SSC_VECT_END)];  /* indentE*/
    Char indentE[3];


}SSC_VECT_INFO, *PSSC_VECT_INFO;



/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void            SSC_VECT_Initialise     ( void );
void            SSC_VECT_Free           ( void );

void* SSC_VECT_CreateInstance ( SSCLOG* const pLog, const SSC_VECT_TYPE type, const SInt length );
void* SSC_VECT_DestroyInstance( void* const pData);

/* operations on all elements of a vector */
Bool SSC_VECT_Clear (void* const pData);
Bool SSC_VECT_Set   (void* const pData, const void* const pValue);
Bool SSC_VECT_Scale (void* const pData, const void* const pScale);

/* operations on 2 complete vectors */
Bool SSC_VECT_Copy  (const void* const pDataSrc, SInt startSrc, void* const pDataDest, SInt startDest, SInt count);
Bool SSC_VECT_Move  (const void* const pDataSrc, SInt startSrc, void* const pDataDest, SInt startDest, SInt count);
Bool SSC_VECT_Acc   (const void* const pDataSrc, void* const pDataDest);
Bool SSC_VECT_Abs   (const void* const pDataSrc, void* const pDataDest);
Bool SSC_VECT_Mpc   (const void* const pDataSrc, void* const pDataDest);
Bool SSC_VECT_FFAbs (const void* const pDataSrc, void* const pDataDest);

/* operations on 1 complete vector */
Bool SSC_VECT_Max   (const void* const pData, void* const pMax   );
Bool SSC_VECT_Sum   (const void* const pData, SInt start, SInt count, void* const pSum   );
Bool SSC_VECT_SumAbs(const void* const pData, SInt start, SInt count, void* const pSumAbs);
Bool SSC_VECT_SumSqr(const void* const pData, SInt start, SInt count, void* const pSumSqr);

/* operations on 2 vectors + storing result in another*/
Bool SSC_VECT_Sub   (const void* const pData1, const void* const pData2, void* const pDataDest);
Bool SSC_VECT_Mpy   (const void* const pData1, SInt startData1, const void* const pData2, SInt startData2, void* const pDataDest, SInt startDest, SInt count);
Bool SSC_VECT_Div   (const void* const pData1, SInt startData1, const void* const pData2, SInt startData2, void* const pDataDest, SInt startDest, SInt count);
Bool SSC_VECT_FFDiv (const void* const pData1, const void* const pData2, void* const pDataDest);
Bool SSC_VECT_MpyC  (const void* const pData1, const void* const pData2, void* const pDataDest);

#ifdef __cplusplus
}
#endif /* __SSC_Vector_h */

#endif



