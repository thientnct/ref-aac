/*
  "This software module was originally developed by 

  Thomas Schaefer (Fraunhofer Gesellschaft IIS)

  and edited by

  Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)
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

#include <assert.h>
#include "stdio.h"
#include "string.h"
#include "portio.h"
#include "escHandling.h"
#include "stdlib.h"

static unsigned short       longestCWAvailable = 0;
static int                  elementID;
static int                  commonWindow = 0;
static int                  channel = 1;
static unsigned short       escAssignment[MAX_ELEMENTS][NR_OF_ASSIGNMENT_SCHEMES] = {{3, 3, 3},  /* COEF */
                                                                                     {3, 3, 3},  /* COEF_COMPRESS */
                                                                                     {3, 3, 3},  /* COEF_RES */
                                                                                     {9, 0, 0},  /* COMMON_WINDOW */
                                                                                     {3, 3, 3},  /* DIRECTION */
                                                                                     {1, 1, 1},  /* DPCM_NOISE_LAST_POSITION */
                                                                                     {1, 1, 1},  /* DPCM_NOISE_NRG */
                                                                                     {1, 0, 0},  /* ELEMENT_INSTANCE_TAG */
                                                                                     {1, 1, 1},  /* GAIN_CONTROL_DATA_PRESENT */
                                                                                     {1, 1, 1},  /* GLOBAL_GAIN */
                                                                                     {4, 4, 4},  /* HCOD */
                                                                                     {4, 4, 4},  /* HCOD_ESC */
                                                                                     {1, 1, 1},  /* HCOD_SF */
                                                                                     {1, 1, 0},  /* ICS_RESERVED */
                                                                                     {3, 3, 3},  /* LENGTH */
                                                                                     {1, 1, 1},  /* LENGTH_OF_LONGEST_CODEWORD */
                                                                                     {1, 1, 1},  /* LENGTH_OF_REORDERED_SPECTRAL_DATA */
                                                                                     {1, 1, 1},  /* LENGTH_OF_RVLC_ESCAPES */
                                                                                     {1, 1, 1},  /* LENGTH_OF_RVLC_SF */
                                                                                     {1, 1, 1},  /* LTP_COEF */
                                                                                     {1, 1, 1},  /* LTP_DATA_PRESENT */
                                                                                     {1, 1, 1},  /* LTP_LAG */
                                                                                     {1, 1, 1},  /* LTP_LAG_UPDATE */
                                                                                     {1, 1, 1},  /* LTP_LONG_USED */
                                                                                     {1, 1, 1},  /* LTP_SHORT_LAG */
                                                                                     {1, 1, 1},  /* LTP_SHORT_LAG_PRESENT */
                                                                                     {1, 1, 1},  /* LTP_SHORT_USED */
                                                                                     {1, 1, 0},  /* MAX_SFB_CODE */
                                                                                     {9, 0, 0},  /* MS_MASK_PRESENT */
                                                                                     {9, 0, 0},  /* MS_USED */
                                                                                     {3, 3, 3},  /* N_FILT */
                                                                                     {1, 1, 1},  /* NUMBER_PULSE */
                                                                                     {3, 3, 3},  /* ORDER */
                                                                                     {1, 1, 0},  /* PREDICTOR_DATA_PRESENT */
                                                                                     {1, 1, 1},  /* PULSE_AMP */
                                                                                     {1, 1, 1},  /* PULSE_DATA_PRESENT */
                                                                                     {1, 1, 1},  /* PULSE_OFFSET */
                                                                                     {1, 1, 1},  /* PULSE_START_SFB */
                                                                                     {4, 4, 4},  /* REORDERED_SPECTRAL_DATA */
                                                                                     {1, 1, 1},  /* REV_GLOBAL_GAIN */
                                                                                     {2, 2, 2},  /* RVLC_CODE_SF */
                                                                                     {2, 2, 2},  /* RVLC_ESC_SF */
                                                                                     {1, 1, 0},  /* SCALE_FACTOR_GROUPING */
                                                                                     {1, 1, 1},  /* SECT_CB */
                                                                                     {1, 1, 1},  /* SECT_LEN_INCR */
                                                                                     {1, 1, 1},  /* SF_CONCEALMENT */
                                                                                     {1, 1, 1},  /* SF_ESCAPES_PRESENT */
                                                                                     {4, 4, 4},  /* SIGN_BITS */
                                                                                     {1, 1, 1},  /* TNS_DATA_PRESENT */
                                                                                     {1, 1, 0},  /* WINDOW_SEQ */
                                                                                     {1, 1, 0},  /* WINDOW_SHAPE_CODE */
                                                                                     {4, 4, 4}}; /* ALIGN_BITS */

void SetElementID (int eleID)
{
  elementID = eleID;
}

void SetCommonWindow(int common_window)
{
  commonWindow = common_window;
}

void SetChannel(int chan)
{
  channel = chan;
}

static int GetInstanceOfEsc(unsigned short channel)
{
  int instanceOfEsc = 0;
  switch (elementID) {
  case ID_LFE:
  case ID_SCE:
    instanceOfEsc = 0;
    break;
  case ID_CPE:
    if (channel == 1)
      instanceOfEsc = 0;
    else
      instanceOfEsc = 1;
    break;
  }
  return ( instanceOfEsc );
}

static int GetAssignmentScheme(void)
{
  int assignmentScheme = 0;

  switch(elementID) {
  case ID_LFE:
  case ID_SCE:
    assignmentScheme = 0;
    break;
  case ID_CPE:
    if (commonWindow)
      assignmentScheme = 2;
    else
      assignmentScheme = 1;
    break;
  default:
    myexit("GetAssignmentScheme: This case should not occur");
  }
  return assignmentScheme;
}

void InitEscInstanceBuffers ( ESC_INSTANCE_DATA* escInstanceData ) 
{
  unsigned short esc;
  unsigned short instanceOfEsc;

  for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
    for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        /* let byteAnchorPtr and lenAnchorPtr for each class point to a range in the heap.   */
        escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.bitCount      = 0;
        escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.byteAnchorPtr = (byte *)calloc(MAX_BYTES,sizeof(byte));
        escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.bytePtr       = escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.byteAnchorPtr;
      }
    }
  }
}

void SetEscInstanceBufferToZero( ESC_INSTANCE_DATA* escInstanceData ) 
{
  unsigned short esc;
  unsigned short instanceOfEsc;
  
  for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
    for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        /* free  byteAnchorPtr  */
        escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.bitCount = 0; 
        escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.bytePtr = escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.byteAnchorPtr;
        memset(escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer.byteAnchorPtr, 0, MAX_BYTES);
      }  
    }
  }
}

void SetLongestCWAvailable(unsigned short flag) {
 
  longestCWAvailable = flag;

}

unsigned short GetLongestCWAvailable(void) {

  return(longestCWAvailable);

}

/*********************************************************************************************
* Function : BitFlip()                                                                       *
* Purpose  : flips in 'byteElement' the 'numOfBit' bit                                       *
* Arguments: byteElement -- a byte pointer to the buffer with the bitstream element (MSBL)   *
*            numOfBit    -- the number of bit, that should be flipped                        *
* Returns  : the byte with the flipped bit                                                   *
**********************************************************************************************/

static unsigned char BitFlip(unsigned char byteElement, unsigned short numOfBit) {
 
  unsigned char mask;

  mask = 0x80 >> (numOfBit - 1);
  return ( byteElement ^ mask);

}

static void CheckBitsInBuffer( ESC_INSTANCE_DATA* escInstanceData ) {

  unsigned short numTotalBits;
  unsigned short numBitsAlign;
  BUFFER_TYPE*     buffer;
  static int     frameNr = -1;
  unsigned short esc;
  unsigned short instanceOfEsc;

  frameNr++;
  escInstanceData = GetEscInstanceData();
  numTotalBits = 0;
  /* computes the actual number of bits in the actual frame */
  for ( instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
      if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        buffer          = &escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer;
        numTotalBits   += buffer->bitCount;
      }
    }
  }
  /* computes the number of align bits */
  numBitsAlign = (8 - (numTotalBits%8)) % 8;
  if (numBitsAlign) {
      WriteElemToBuffer(ALIGN_BITS,0,numBitsAlign); 
    }
  numTotalBits += numBitsAlign;
  /* store the data in the esc instance buffer */
  for ( instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
      if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        buffer          =  &escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer;
        escInstanceData->escInstance[instanceOfEsc][esc].bitCount = buffer->bitCount;  
      }
    }
  }
  escInstanceData->numBitsInFrame = numTotalBits;

}

/*********************************************************************************************
* Function : WriteBufferToFile                                                               *
* Purpose  : Writes all esc instance buffers to file                                         *
* Arguments: nrOfEscInstances                                                                *
* Returns  : void                                                                            *
**********************************************************************************************/

void WriteBufferToFile( ESC_INSTANCE_DATA* escInstanceData ) 
{
  byte*          lastBytePtr;
  byte           addByte;
  byte*          writeBytePtr;
  BUFFER_TYPE*   buffer;        /* current esc instance buffer */
  FILE*          fBitPtr;
  unsigned int   bitsToWrite;
  unsigned short byteCounter;  
  unsigned short numUsedBytes;
  unsigned short bitsFilled;
  unsigned short bitsFree;
  unsigned short numBitsWriteBuffer;
  unsigned short esc;
  unsigned short instanceOfEsc;
  unsigned short finalWriteFlag = 0;

  CheckBitsInBuffer ( escInstanceData );
  fBitPtr            = GetBitFilePtr();
  lastBytePtr        = (byte *)malloc(sizeof(byte));
  writeBytePtr       = (byte *)malloc(sizeof(byte));
  *lastBytePtr       = 0x0;
  numBitsWriteBuffer = 0;
  for ( instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
      if ( escInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        byteCounter     = 1;
        buffer          = &escInstanceData->escInstance[instanceOfEsc][esc].escInstanceBuffer;
        buffer->bytePtr = buffer->byteAnchorPtr;
        bitsToWrite     = buffer->bitCount;
        bitsFilled      = numBitsWriteBuffer % 8; /* significant bits in last byte of write buffer */
        bitsFree        = 8 - bitsFilled;
        numUsedBytes    = (buffer->bitCount + 7) / 8; /* ... in the actual buffer */
        /* while loop writes all data to file */
        while ( byteCounter <= numUsedBytes ) {
          addByte       = *buffer->bytePtr >> bitsFilled;
          *writeBytePtr = *lastBytePtr | addByte;
          if ( byteCounter == numUsedBytes ) {
            if ( bitsToWrite >= bitsFree ) {
              /* write if the last byte is full with significant bits */
              fwrite( writeBytePtr,1,sizeof(byte), fBitPtr );
              *lastBytePtr  = *buffer->bytePtr << bitsFree;
              finalWriteFlag = 0;
            }
            else {
                *lastBytePtr = *writeBytePtr; 
                finalWriteFlag = 1;
            }
          }
          else {
            /* write until reach the last used byte in the buffer */
            fwrite( writeBytePtr,1,sizeof(byte), fBitPtr );
            *lastBytePtr  = *buffer->bytePtr << bitsFree;
            bitsToWrite -=  8;
            buffer->bytePtr++;
          }
          byteCounter++;
        }
        numBitsWriteBuffer += buffer->bitCount;
      }
    }
  }
  if ( finalWriteFlag ) {
    fwrite ( writeBytePtr, 1, sizeof(byte), fBitPtr );
  }

  free(lastBytePtr);
  free(writeBytePtr);
  SetEscInstanceBufferToZero( escInstanceData );
  assert( !(numBitsWriteBuffer%8) );
}

/*********************************************************************************************
* Function : PosNewElement                                                                   *
* Purpose  : Include 'value' at the current position in the 'buffer' (esc instance buffer)   *
* Arguments: buf        -- pointer to the current buffer                                     *
*            value      -- value it has to be written to the buffer                          *
*            length     -- number of bits of value                                           *
* Returns  : pointer to the new buffer                                                       *
**********************************************************************************************/

BUFFER_TYPE* PosNewElement( BUFFER_TYPE *buf, unsigned long int value, unsigned short length) {

  unsigned short bitsFilled;
  short          bitPosition;
  unsigned short bitsFree;
  unsigned short lengthAct;
  unsigned short shiftDel; 
  unsigned long  valueAct;
  unsigned long  valueHelp;

  valueAct     = value;  
  lengthAct    = length;
  
  if (length > 0 ) {
 
    bitsFilled = buf->bitCount % 8;       /* filled bits in actual byte of the buffer      */
    bitsFree   = 8 - bitsFilled;     /* free bits in actual byte of the buffer        */
    bitPosition= bitsFree - length;  /* Bit Position in the actual byte of the buffer */  
 
    while ( 1 ) {
      if ( bitPosition >= 0) {
        /* Simple case: enough free bits in actual byte of the buffer                      */
        valueAct <<= bitPosition;  /* shift bits in correct position of value      */
        *buf->bytePtr |= valueAct; /* writes with 'or' in the actual byte          */
        buf->bitCount += lengthAct;     /* update bitCount                              */
        bitsFilled = buf->bitCount % 8; /* filled bits in actual byte of the buffer     */
        if ( bitsFilled == 0 ) {   /* if the actual byte is full, skip to next byte*/
          buf->bytePtr++;
          assert ( buf->bytePtr - buf->byteAnchorPtr < MAX_BYTES );
        }
        break;
      }
      else { 
        /* Difficult case: not enough free bits in actual byte of the buffer               */
        valueHelp = valueAct >> -bitPosition;/* bits, which can placed in the actual byte, */
        /* shiftet in the correct position of a byte  */
        *buf->bytePtr |= valueHelp;       /* writes with 'or' in the actual byte        */
        buf->bitCount += bitsFree;        /* update bitCount                            */
        buf->bytePtr++;                   /* in this case the actual byte is ever full  */
        assert ( buf->bytePtr - buf->byteAnchorPtr < MAX_BYTES );
        shiftDel   = 32 + bitPosition;       /* Computes shift to delete the writing bits  */
        valueAct <<= shiftDel;
        valueAct >>= shiftDel;                /* writing bits now set to zero              */
        lengthAct  = lengthAct - bitsFree ;
        bitsFilled = buf->bitCount % 8;   /* filled bits in actual byte of the buffer   */
        bitsFree   = 8 - bitsFilled;         /* free bits in actual byte of the buffer     */
        bitPosition= bitsFree - lengthAct;    /* Bit Position in actual byte of the buffer */
      } 
    }
  
  }
  return(buf);
}

/*********************************************************************************************
* Function : SeekElement                                                                     *
* Purpose  : Include 'value' at a specified position in the 'buffer' (esc instance buffer)   *
* Arguments: buf        -- pointer to the current buffer                                     *
*            value      -- value it has to be written to the buffer                          *
*            length     -- number of bits of value                                           *
*            position   -- bit pointer of the position, element has to be included           *
* Returns  : pointer to the new buffer                                                       *
**********************************************************************************************/

BUFFER_TYPE* SeekElement( BUFFER_TYPE *buf, unsigned long int value, unsigned short length, 
                          unsigned short position ) {

  unsigned short bitsFilled;
  short          bitPosition;
  unsigned short bitsFree;
  unsigned short lengthAct;
  unsigned short shiftDel; 
  unsigned long  valueAct;
  unsigned long  valueHelp;
  byte           *curBytePtr;
  unsigned short bytePosOffset;

  valueAct     = value;  
  lengthAct    = length;
  
  bytePosOffset = position / 8;
  curBytePtr    = buf->byteAnchorPtr+bytePosOffset;
  
  if (length > 0 ) {
 
    bitsFilled = position% 8;        /* filled bits in actual byte of the buffer      */
    bitsFree   = 8 - bitsFilled;     /* free bits in actual byte of the buffer        */
    bitPosition= bitsFree - length;  /* Bit Position in the actual byte of the buffer */  
 
    while ( 1 ) {
      if ( bitPosition >= 0) {
        /* Simple case: enough free bits in actual byte of the buffer              */
        valueAct <<= bitPosition;  /* shift bits in correct position of value      */      
        *curBytePtr |= valueAct;   /* writes with 'or' in the actual byte          */
        position += lengthAct;     /* update bitCount                              */
        bitsFilled = position % 8; /* filled bits in actual byte of the buffer     */
        if ( bitsFilled == 0 ) {   /* if the actual byte is full, skip to next byte*/
          curBytePtr++;
        }
        break;
      }
      else { 
        /* Difficult case: not enough free bits in actual byte of the buffer               */
        valueHelp = valueAct >> -bitPosition;/* bits, which can placed in the actual byte, */
        /* shiftet in the correct position of a byte  */
        *curBytePtr |= valueHelp;            /* writes with 'or' in the actual byte        */
        position += bitsFree;                /* update bitCount                            */
        curBytePtr++;                        /* in this case the actual byte is ever full  */
        shiftDel   = 32 + bitPosition;       /* Computes shift to delete the writing bits  */
        valueAct <<= shiftDel;
        valueAct >>= shiftDel;               /* writing bits now set to zero               */
        lengthAct  = lengthAct - bitsFree ;
        bitsFilled = position % 8;           /* filled bits in actual byte of the buffer   */
        bitsFree   = 8 - bitsFilled;         /* free bits in actual byte of the buffer     */
        bitPosition= bitsFree - lengthAct;   /* Bit Position in actual byte of the buffer  */
      } 
    }
  
  }
  
  return(buf);
}


unsigned short Take(unsigned short start, unsigned short end, unsigned short value) {

  unsigned short returnValue = 0x0;
 
  start = start + 8; 
  end = end + 8;
  returnValue = value << ( start - 1 );
  returnValue = returnValue >> ( 15 - end + start );
  return( returnValue );

}

/*********************************************************************************************
* Function : WriteElemToBuffer                                                               *
* Purpose  : Writes 'value' to 'buffer' (esc instance buffer)                                *
* Arguments: value      -- value it has to be written to the buffer                          *
*            length     -- number of bits of value                                           *
*            codeIndex  -- code number for the error protection class of the input buffer    *
* Returns  : void                                                                            *
**********************************************************************************************/

void WriteElemToBuffer( CODE_TABLE codeIndex, unsigned long value, unsigned short length) {
  
  BUFFER_TYPE*   buffer;
  unsigned short esc           = escAssignment[(int)codeIndex][GetAssignmentScheme()];
  unsigned short instanceOfEsc = GetInstanceOfEsc(channel);

  /* esc instance availability check */
  if ( !GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
    PRINT(SE,"the following error sensitivity category instance is out of order:\n");
    PRINT(SE," --> error sensitivity category: %i, instance %i\n",esc, instanceOfEsc);
    myexit ("WriteElemToBuffer");
  }
  buffer = &(GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceBuffer); /* Searches the correct buffer from the heap */ 
  buffer = PosNewElement(buffer, value, length);

  if(debug['v']) 
    PRINT(SE,"wrote elem %2d ( %2d bits, total: %4d ) to instance %d of esc %d\n", 
          (int) codeIndex, 
          length,
          buffer->bitCount, 
          instanceOfEsc,
          esc );
}

/*********************************************************************************************
* Function : SeekElemToBuffer                                                                *
* Purpose  : Include 'value' at a specified position in the 'buffer' (esc instance buffer)   *
* Arguments: codeIndex  -- code number for the error protection class of the input buffer    *
*            value      -- value it has to be written to the buffer                          *
*            length     -- number of bits of the value                                       *
*            position   -- specified position (bit pointer)                                  *
* Returns  : void                                                                            *
**********************************************************************************************/

void SeekElemToBuffer( CODE_TABLE codeIndex, unsigned long value, unsigned short length,
                      unsigned short position) {
  
  BUFFER_TYPE*   buffer;
  unsigned short esc           = escAssignment[(int)codeIndex][GetAssignmentScheme()];
  unsigned short instanceOfEsc = GetInstanceOfEsc(channel);

  /* esc instance availability check */
  if ( !GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
    PRINT(SE,"the following error sensitivity category instance is out of order:\n");
    PRINT(SE," --> error sensitivity category: %i, instance %i\n",esc, instanceOfEsc);
    myexit ("SeekElemToBuffer");
  }
  buffer = &(GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceBuffer); /* Searches the correct buffer from the heap */  
  buffer = SeekElement(buffer, value, length, position);
}

unsigned short GetCurrentPosition( CODE_TABLE codeIndex) {

  BUFFER_TYPE*   buffer;
  unsigned short esc           = escAssignment[(int)codeIndex][GetAssignmentScheme()];
  unsigned short instanceOfEsc = GetInstanceOfEsc(channel);

  /* esc instance availability check */
  if ( !GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
    PRINT(SE,"the following error sensitivity category instance is out of order:\n");
    PRINT(SE," --> error sensitivity category: %i, instance %i\n",esc, instanceOfEsc);
    myexit ("GetCurrentPosition");
  }
  buffer = &(GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceBuffer);
  return buffer->bitCount;

}

/*********************************************************************************************
* Function : WriteBufferToBuffer                                                             *
* Purpose  : Writes the contents of 'buf' (byte Vector)  to 'buffer' (esc instance buffer)   *
* Arguments: *buf       -- pointer, who points to the first byte of the input buffer         *
*            lengthBits -- number of bits of the input buffer                                *
*            codeIndex  -- code number for the error protection class of the input buffer    *
* Returns  : void                                                                            *
**********************************************************************************************/

void WriteBufferToBuffer( CODE_TABLE codeIndex, unsigned char *buf, unsigned short lengthBits) 
{
  BUFFER_TYPE*   buffer;           /* this is the esc instance buffer (output) */
  unsigned short numUsedBytes;     /* number of bytes of the input buffer      */
  unsigned short byteCounter;      /* number of actual written bytes           */
  unsigned short numBitsLastByte;  /* number of bits in the last byte of buf   */
  unsigned short esc           = escAssignment[(int)codeIndex][GetAssignmentScheme()];
  unsigned short instanceOfEsc = GetInstanceOfEsc(channel);

  /* esc instance availability check */
  if ( !GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
    PRINT(SE,"the following error sensitivity category instance is out of order:\n");
    PRINT(SE," --> error sensitivity category: %i, instance %i\n",esc, instanceOfEsc);
    myexit ("WriteBufferToBuffer");
  }
  buffer       = &(GetEscInstanceData()->escInstance[instanceOfEsc][esc].escInstanceBuffer);
  numUsedBytes = lengthBits / 8;
  byteCounter  = 0;
  while( byteCounter < numUsedBytes ) {
    buffer = PosNewElement(buffer, *buf, 8);
    byteCounter++;
    buf++;
  }
  numBitsLastByte = lengthBits % 8;
  buffer = PosNewElement(buffer, *buf >> ( 8 - numBitsLastByte ), numBitsLastByte);

  if(debug['v']) 
    PRINT(SE,"wrote elem %2d ( %2d bits, total: %4d ) to instance %d of esc %d\n", 
          (int) codeIndex, 
          lengthBits,
          buffer->bitCount, 
          instanceOfEsc,
          esc );
}

