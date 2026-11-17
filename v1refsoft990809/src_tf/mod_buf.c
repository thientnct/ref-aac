/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by


in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1999.
 *                                                                           *
 ****************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "common_m4a.h"

#include "mod_bufHandle.h"       /* handler, defines, enums */

#include "mod_buf.h"

#define INVALID_HANDLE 0
#define TRUE 1
#define FALSE 0

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) 
#endif
#ifndef NULL 
#define NULL 0
#endif

struct tag_modulo_buffer {
    unsigned int   size;
    unsigned int   current_position;
    unsigned int   readPosition;
    float*         paBuffer;
    int            numVal;
} T_MODULO_BUFFER;

/*****************************************************************************

    functionname: CreateFloatModuloBuffer  
    description:  creates a circular buffer
    returns:      Status info
    input:        number of float values inside circular buffer
    output:       Handle

*****************************************************************************/
HANDLE_MODULO_BUFFER CreateFloatModuloBuffer(unsigned int size)
{
    HANDLE_MODULO_BUFFER pModuloBuffer;

    assert(size > 0);

    pModuloBuffer = (HANDLE_MODULO_BUFFER)calloc(1,sizeof(T_MODULO_BUFFER));

    if(pModuloBuffer == NULL)
        return(INVALID_HANDLE);

    pModuloBuffer->size = size;
    pModuloBuffer->paBuffer = (float*)malloc(sizeof(float)*size);

    if(pModuloBuffer->paBuffer == NULL) {
      DeleteFloatModuloBuffer(pModuloBuffer);
      return INVALID_HANDLE;
    }

    pModuloBuffer->current_position = 0;
    pModuloBuffer->readPosition     = 0;
    pModuloBuffer->numVal     = 0;

    return(pModuloBuffer);
}



/*****************************************************************************

    functionname: DeleteFloatModuloBuffer  
    description:  frees memeory 
    returns:      
    input:        Handle
    output:       

*****************************************************************************/
void DeleteFloatModuloBuffer(HANDLE_MODULO_BUFFER hModuloBuffer)
{
  if (hModuloBuffer) {
    if (hModuloBuffer->paBuffer) free(hModuloBuffer->paBuffer);
    free(hModuloBuffer);
  }
}



/*****************************************************************************

    functionname: AddFloatModuloBufferValues  
    description:  adds values to circular buffer
    returns:      TRUE (size match is verified by assertion)
    input:        Handle, ptr to values to enter, number of values to enter
    output:       

*****************************************************************************/
void copyFLOAT( const float src[], float dest[], int inc_src, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src;
    dest += inc_dest;
    src  += inc_src;
  }
  if (vlen) /* just for bounds-checkers sake */
    *dest = *src;

}

unsigned char AddFloatModuloBufferValues(HANDLE_MODULO_BUFFER hModuloBuffer, const float *values, unsigned int n){
    unsigned int lim1, lim2;

    assert(n <= hModuloBuffer->size);

    lim1 = min(hModuloBuffer->current_position+n, hModuloBuffer->size) - hModuloBuffer->current_position;
    copyFLOAT(values,
	      hModuloBuffer->paBuffer+hModuloBuffer->current_position,
              1,
              1,
	      lim1);

    hModuloBuffer->current_position = (hModuloBuffer->current_position+lim1)%hModuloBuffer->size;
    hModuloBuffer->numVal += n;
    
    if ( (unsigned int) hModuloBuffer->numVal > hModuloBuffer->size){
      CommonExit(1,"modulo buffer overflow");
    }

    lim2 = n - lim1;

    if (lim2 > 0) {
      copyFLOAT(values+lim1,
		hModuloBuffer->paBuffer+hModuloBuffer->current_position,
                1,
                1,
		lim2);
      hModuloBuffer->current_position=(hModuloBuffer->current_position+lim2)%hModuloBuffer->size;
    }
    return(TRUE);
}



/*****************************************************************************

    functionname: ZeroFloatModuloBuffer
    description:  set n elements of circular buffer to zero
    returns:      TRUE
    input:        Handle, number of elements to zero out
    output:       

*****************************************************************************/
unsigned char ZeroFloatModuloBuffer(HANDLE_MODULO_BUFFER hModuloBuffer, unsigned int n){
    unsigned int lim1, lim2;
    float fzero=0.0f;
    assert(n <= hModuloBuffer->size);

    lim1 = min(hModuloBuffer->current_position+n, hModuloBuffer->size) - hModuloBuffer->current_position;
    copyFLOAT(&fzero, &hModuloBuffer->paBuffer[hModuloBuffer->current_position],
              0,
              1,
              lim1);

    hModuloBuffer->current_position=(hModuloBuffer->current_position+lim1)%hModuloBuffer->size;

    lim2 = n - lim1;
    if (lim2 > 0) {
      copyFLOAT(&fzero, &hModuloBuffer->paBuffer[hModuloBuffer->current_position], 
                0,
                1,
                lim2);

      hModuloBuffer->current_position = (hModuloBuffer->current_position+lim2)%hModuloBuffer->size;
    }
    return(TRUE);
}



/*****************************************************************************

    functionname: GetFloatModuloBufferValues  
    description:  checks out circular buffer values
    returns:      TRUE
    input:        Handle, number of values to retrieve, logical position in 
                  circular buffer
    output:       values retrieved

*****************************************************************************/
unsigned char GetFloatModuloBufferValues(HANDLE_MODULO_BUFFER hModuloBuffer, float *values, unsigned int n, unsigned int age){
    unsigned int lim1, lim2;
    unsigned int iReqPosition;

    assert(n <= hModuloBuffer->size);
    assert(age <= hModuloBuffer->size);
    assert(n <= age);

    iReqPosition = (hModuloBuffer->size + hModuloBuffer->current_position - age) % hModuloBuffer->size;

    lim1 = min(iReqPosition + n, hModuloBuffer->size) - iReqPosition;
    copyFLOAT(hModuloBuffer->paBuffer+iReqPosition, values, 1,1,lim1);

    iReqPosition = (iReqPosition + lim1) % hModuloBuffer->size;

    lim2 = n - lim1;

    if (lim2 > 0) 
      copyFLOAT(hModuloBuffer->paBuffer+iReqPosition, values+lim1, 1,1,lim2);
    
    return(TRUE);
}

/*****************************************************************************

    functionname: ReadFloatModuloBufferValues  
    description:  checks out circular buffer values from read pointer
    returns:      TRUE
    input:        Handle, number of values to retrieve
    output:       values retrieved

*****************************************************************************/
unsigned char ReadFloatModuloBufferValues(HANDLE_MODULO_BUFFER hModuloBuffer, float *values, unsigned int n){
  unsigned int lim1, lim2;
  unsigned int iReqPosition;

  assert(n <= hModuloBuffer->size);
  
  iReqPosition = hModuloBuffer->readPosition;

  lim1 = min(iReqPosition + n, hModuloBuffer->size) - iReqPosition;
  copyFLOAT(hModuloBuffer->paBuffer+iReqPosition, values, 1,1,lim1);

  hModuloBuffer->numVal -= n;
  if (hModuloBuffer->numVal < 0 ){
    CommonExit(1,"modulo buffer underrun");
  }

  
  lim2 = n - lim1;
  
  if (lim2 > 0) {
    copyFLOAT(hModuloBuffer->paBuffer,
              values+lim1,
              1,
              1,
              lim2);

    hModuloBuffer->readPosition = lim2;

  } else {   
    hModuloBuffer->readPosition = iReqPosition+lim1;
  }

  return(TRUE);
}

int GetNumVal ( HANDLE_MODULO_BUFFER hModuloBuffer )
{
  return ( hModuloBuffer->numVal );
}
