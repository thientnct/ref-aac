/*
  "This software module was originally developed by 

  Thomas Schaefer (Fraunhofer Gesellschaft IIS)

  and edited by

  Ralph Sperschneider (Fraunhofer IIS-A)

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
  Copyright(c)1998, 2000.
*/
 
/*****************************************************************************
* Types                                                                      *
*                                                                            *
* typedef struct class_info --                                               *
*   type to store the pre-definition data of a class within a frame          *
*                                                                            *
* typedef struct predDefData --                                              *
*   type to store the pred definition-data of a frame                        *
*                                                                            *
*----------------------------------------------------------------------------*
* Routines                                                                   *
*                                                                            *
* predDefData ReadPredDefFile()                                              *
*   Reads the pre-definition data from a file (*.pred).                      *
*                                                                            *
* predDefData *GetPredDefDeata()                                             *
*   Set a pointer to the pre-definition data in the heap.                    *
*                                                                            *
* unsigned char *GetLastByte                                                 *
*   Set a pointer to the last byte in the writting process                   *
*                                                                            *
* unsignd int *GetNumBits                                                    *
*   Set a pointer to the number of written bits                              *
*                                                                            *
******************************************************************************/

#ifndef _portio_h_
#define _portio_h_

#include <stdio.h>
#include "all.h"

#define MAX_NR_OF_ESC              5
#define MAX_NR_OF_INSTANCE_PER_ESC 2

typedef struct {
  unsigned short bitCount;       /* stores the number of bits in the buffer    */
  byte           *byteAnchorPtr; /* points to the first byte in the heap       */
  byte           *bytePtr;       /* points to the actual byte in the heap      */
} BUFFER_TYPE;

typedef struct {
  unsigned short escInstanceUsed;  /* signals, whether this esc instance is used */
  unsigned short bitCount;         /* number of bits in the class                */
  BUFFER_TYPE    escInstanceBuffer; 
} SINGLE_ESC_INSTANCE_DATA;

typedef struct {
  unsigned short            escForTnsUsed;                                                /* signals, whether the optional esc for tns is used */
  unsigned short            escForRvlcUsed;                                               /* signals, whether the optional esc for rvlc is used */
  unsigned short            numBitsInFrame;                                               /* number of bits in the audio frame */
  SINGLE_ESC_INSTANCE_DATA  escInstance[MAX_NR_OF_INSTANCE_PER_ESC][MAX_NR_OF_ESC];       /* class handler */
} ESC_INSTANCE_DATA;

ESC_INSTANCE_DATA    ReadPreDefFile(char *inputFile);
ESC_INSTANCE_DATA    *GetEscInstanceData(void);
unsigned char *GetLastByte(void);
unsigned int  *GetNumBits(void);
void          UpdateNumBits(unsigned int value);
void          UpdateLastByte(unsigned char lastByte);
void          OpenEPFiles(char *fileName);
void          WriteInfoFile(ESC_INSTANCE_DATA *buffer );
FILE          *OpenEPFile(char *fileName, char *mode);
FILE          *GetBitFilePtr(void);
FILE          *GetInfFilePtr(void);  
int           GetNumberOfClasses(void);

#endif /* _portio_h_ */
