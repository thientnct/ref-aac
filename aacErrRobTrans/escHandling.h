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
/****************************************************************************************
* Types                                                                                 *
*                                                                                       *
* typedef struct BUFFER_TYPE  --                                                          *
*   type, which points to a range in the heap, storing data elements of an error        *
*   sensitivity category (esc) instance within a frame.                                 *
*                                                                                       *
*---------------------------------------------------------------------------------------*
* Routines                                                                              *
*                                                                                       *
* int WriteElemToBuffer()                                                               *
*   writes the value of a data element corresponding to its esc assignment in the       *
*   esc instance buffer.                                                                *
*                                                                                       *
* void WriteBufferToBuffer()                                                            *
*   Writes the content of 'buf' (byte Vector)  to 'buffer' (esc instance buffer)        *
*                                                                                       *
* void InitEscInstanceBuffers()                                                         *
*   Allocate memory from the heap and set buffers for ESC instances to zero             *
*                                                                                       *
* void SetEscInstanceBufferToZero()                                                     *
*   Sets all bytes of an esc instance buffer to zero.                                   *
*                                                                                       *
*****************************************************************************************/

#include "all.h"
#include "portio.h"

#define MAX_BYTES 1024      /* ... in each esc instance buffer                 */
#define NR_OF_ASSIGNMENT_SCHEMES 3
#define L 0


/* code table for aac data elements */
typedef enum {
  COEF,
  COEF_COMPRESS,
  COEF_RES,
  COMMON_WINDOW,
  DIRECTION,
  DPCM_NOISE_NRG,
  DPCM_NOISE_LAST_POSITION,
  ELEMENT_INSTANCE_TAG,
  GAIN_CONTROL_DATA_PRESENT,
  GLOBAL_GAIN,
  HCOD,
  HCOD_ESC,
  HCOD_SF,
  ICS_RESERVED,
  LENGTH,
  LENGTH_OF_LONGEST_CODEWORD,
  LENGTH_OF_REORDERED_SPECTRAL_DATA,
  LENGTH_OF_RVLC_ESCAPES,
  LENGTH_OF_RVLC_SF,
  LTP_COEF,
  LTP_DATA_PRESENT,
  LTP_LAG,
  LTP_LAG_UPDATE,
  LTP_LONG_USED,
  LTP_SHORT_LAG,
  LTP_SHORT_LAG_PRESENT,
  LTP_SHORT_USED,
  MAX_SFB,
  MS_MASK_PRESENT,
  MS_USED,
  N_FILT,
  NUMBER_PULSE,
  ORDER,
  PREDICTOR_DATA_PRESENT,
  PULSE_AMP,
  PULSE_DATA_PRESENT,
  PULSE_OFFSET,
  PULSE_START_SFB,
  REORDERED_SPECTRAL_DATA,
  REV_GLOBAL_GAIN,
  RVLC_CODE_SF,
  RVLC_ESC_SF,
  SCALE_FACTOR_GROUPING,
  SECT_CB,
  SECT_LEN_INCR,
  SF_CONCEALMENT,
  SF_ESCAPES_PRESENT,
  SIGN_BITS,
  TNS_DATA_PRESENT,
  WINDOW_SEQ,
  WINDOW_SHAPE_CODE,
  ALIGN_BITS,
  MAX_ELEMENTS
} CODE_TABLE;

int            GetID(void);
void           SetChannel(int chan);
void           SetElementID (int eleID);
void           SetCommonWindow(int common_window);
void           InitEscInstanceBuffers ( ESC_INSTANCE_DATA* escInstanceData );
void           LoadEscAssignment(char *fileName, unsigned short nrOfEsc);
void           OpenOutputBitStream(char *fileName);
void           SetEscInstanceBufferToZero( ESC_INSTANCE_DATA* escInstanceData );
unsigned short Take(unsigned short start, unsigned short end, unsigned short value);
void           WriteElemToBuffer(CODE_TABLE codeIndex, unsigned long value, unsigned short length);
void           WriteBufferToBuffer(CODE_TABLE codeIndex, unsigned char *buf, unsigned short lengthBits);
BUFFER_TYPE      *GetClassBuffer(unsigned short esc);
BUFFER_TYPE      *PosNewElement( BUFFER_TYPE *buf, unsigned long int value, unsigned short length);
BUFFER_TYPE      *SeekElement( BUFFER_TYPE *buf, unsigned long int value, unsigned short length, 
                             unsigned short position );
void           WriteBufferToFile( ESC_INSTANCE_DATA* escInstanceData );
void           SetLongestCWAvailable(unsigned short flag);
unsigned short GetLongestCWAvailable(void);
void           UpdateNumBitsInFrame(unsigned short value);
unsigned short GetCurrentPosition( CODE_TABLE codeIndex);
void           SeekElemToBuffer( CODE_TABLE codeIndex, unsigned long value, unsigned short length,
                                unsigned short position);
