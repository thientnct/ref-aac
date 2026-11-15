/*
  "This software module was originally developed by 

  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 

  and edited by

  Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
  Thomas Schaefer (Fraunhofer Gesellschaft IIS)
  Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

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
  Copyright(c)1996, 1997, 1998, 1999.
*/

#include "all.h"
#include "assert.h"
#include "reorderspec.h"
#include "statistics.h"
#include "resorg.h"
#include "scf_resil.h"
#include "escHandling.h"
#include "rvlcScf.h"

#ifndef MAX    
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define MAX_NR_CODEBOOK11 20

struct Nec_Info
{
  int pulse_data_present;
  int number_pulse;
  int pulse_start_sfb;
  int pulse_position[NUM_NEC_LINES];
  int pulse_offset[NUM_NEC_LINES];
  int pulse_amp[NUM_NEC_LINES];
};
static struct Nec_Info nec_info;

short*	             factors[2];
byte*	             sect;
static unsigned long nBitCount;
static int*          vcb11Anchor;    /* contains the virtual codebook numbers of codebook 11 */

/* -----------------------------*/
/* output of spectral values after deinterleaving, before rescaling */

static void PrintSpectralValues ( int *quant )
{
  unsigned short i;
  for ( i = 0; i < LN2; i++ )
    fprintf ( stderr, "bno %13li line %4i value %13i\n", bno, i, quant[i] );
}

void huffdecinit ( )
{
  factors[0]  = (short *)mal1(MAXBANDS*sizeof(*factors[0]));
  factors[1]  = (short *)mal1(MAXBANDS*sizeof(*factors[0]));
  sect        = (byte  *)mal1(2*(MAXBANDS+1)*sizeof(*sect));
  /* vcb11Anchor has the same size as sect, but has not to store the length of sections.
     but on the other hand each section using codebook 11 will be finished in 'vcb11' with a zero (just a kind of runlength coding)
     the selected size is still larger than the worst case (each second section uses codebook 11) */
  vcb11Anchor = (int   *)mal1(2*(MAXBANDS+1)*sizeof(*vcb11Anchor)); 
} 

/*********************************************************************/
/*
 * NEC noiseless coding
 */
static void get_nec_nc(struct Nec_Info *nec_info)
{
  int i;
  nec_info->number_pulse = getbits(LEN_NEC_NPULSE);
  WriteElemToBuffer(NUMBER_PULSE,nec_info->number_pulse,LEN_NEC_NPULSE);
  nec_info->pulse_start_sfb = getbits(LEN_NEC_ST_SFB);
  WriteElemToBuffer ( PULSE_START_SFB, nec_info->pulse_start_sfb, LEN_NEC_ST_SFB );
  for(i=0; i<nec_info->number_pulse; i++) 
    {
      nec_info->pulse_offset[i] = getbits(LEN_NEC_POFF);
      WriteElemToBuffer(PULSE_OFFSET,nec_info->pulse_offset[i],LEN_NEC_POFF);
      nec_info->pulse_amp[i] = getbits(LEN_NEC_PAMP);
      WriteElemToBuffer(PULSE_AMP,nec_info->pulse_amp[i],LEN_NEC_PAMP);
    }
}

static void nec_nc(int *coef, struct Nec_Info *nec_info)
{
  int i, k;
  
  k = only_long_info.sbk_sfb_top[0][nec_info->pulse_start_sfb];
  
  for(i=0; i<=nec_info->number_pulse; i++) 
    {
      k += nec_info->pulse_offset[i];
      if (coef[k]>0) coef[k] += nec_info->pulse_amp[i];
      else coef[k] -= nec_info->pulse_amp[i];
    }
}

typedef int DINT_DATATYPE ;

static void deinterleave ( DINT_DATATYPE inptr[], 
                           DINT_DATATYPE outptr[], 
                           short ngroups,
                           short nsubgroups[], 
                           int   ncells[], 
                           short cellsize[] )
{
  int i, j, k, l;
  DINT_DATATYPE *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
  short cell_inc, subgroup_inc;
  
  start_subgroup_ptr = outptr;
  
  for ( i = 0; i < ngroups; i++ )
    {
      cell_inc = 0;
      start_inptr = inptr;
      
      /* Compute the increment size for the subgroup pointer */
      subgroup_inc = 0;
      for ( j = 0; j < ncells[i]; j++ ) 
        subgroup_inc += cellsize[j];
      
      /* Perform the deinterleaving across all subgroups in a group */
      for ( j = 0; j < ncells[i]; j++ ) 
	{
	  subgroup_ptr = start_subgroup_ptr;
          for ( k = 0; k < nsubgroups[i]; k++ ) 
	    {
	      outptr = subgroup_ptr + cell_inc;
	      for ( l = 0; l < cellsize[j]; l++ )
                *outptr++ = *inptr++;
              subgroup_ptr += subgroup_inc;
	    }
	  cell_inc += cellsize[j];
	}
      start_subgroup_ptr += (inptr - start_inptr);
    }
}

static void calc_gsfb_table(Info *info, byte *group)
{
  int group_offset;
  int group_idx;
  int offset;
  short * group_offset_p;
  int sfb,len;
  /* first calc the group length*/
  if (info->islong)
    return;
  else 
    {
      group_offset = 0;
      group_idx =0;
      do  
        {
          info->group_len[group_idx]=group[group_idx]-group_offset;
          group_offset=group[group_idx];
          group_idx++;
        } while (group_offset<8);
      info->num_groups=group_idx;
      group_offset_p = info->bk_sfb_top;
      offset=0;
      for (group_idx=0;group_idx<info->num_groups;group_idx++)
        {
          len = info->group_len[group_idx];
          for (sfb=0;sfb<info->sfb_per_sbk[group_idx];sfb++)
            {
              offset += info->sfb_width_128[sfb] * len;
              *group_offset_p++ = offset;
#if 0
              PRINT(SE,"%d %d: %d\n",  group_idx, sfb, offset);
#endif	  
            }
        }
    }
}

static void IncludeVcb11 ( int* sectbits, 
                           int  numOfCodebook11Sections,
                           int* posCodebook11 )
{
  int        bits;
  int        len;
  int        codebook11Counter;
  int        currentPosition;
  int*       vcb11;                       /* pointer to virtual codebooks in the heap */

  bits = sectbits[0];
  len = (1 << bits) - 1;
  /* set pointer with virtual codebooks at the beginning */
  vcb11 = vcb11Anchor;
  /* loop how much sections with codebook 11 */
  for ( codebook11Counter = 0; codebook11Counter < numOfCodebook11Sections; codebook11Counter++ ) { 
    currentPosition = posCodebook11[codebook11Counter]; 
    /* loop over all escapes of the current codebook */
    while ( 1 ) {
      if ( *vcb11 != 0 ) {
        /* Seek at 'currentPosition' the virtual codebook */
        SeekElemToBuffer ( SECT_CB, *vcb11++, LEN_CB_VCB11, currentPosition );
        currentPosition    += LEN_CB_VCB11;                        
      }
      else {
        vcb11++; /* Skip the zero to the next section (if exists) */
        break;
      }
    }
  }
}

/*
 * read the codebook and boundaries
 */
static int huffcb ( byte* sect,                     /* contains sect_cb and sect_len
                                                       (output) */
                    int*  sectbits,                 /* number of bits used to encode
                                                       sect_len_incr */
                    int   tot_sfb, 
                    int   sfb_per_sbk, 
                    byte  max_sfb,
                    int*  positionCodebook11,       /* contains the positions of codebook 11 
                                                       in section data (output) */
                    int*  numOfCodebook11Sections ) /* how often is codebook 11 in all 
                                                       sections used (output) */
{
  int  base;              /* swb counter */
  int  counter;
  int  sectEscVal;        /* escape value for sect_len_incr */
  int  cntSectLenIncr;    /* counts repetitions of sect_len_incr */
  int  lenOfSectLenIncr;  /* length of sect_len_incr in bits */
  int  lenForVcb11;       /* number of bits left free to insert appropriate vcb number later */
  int  nsect;             /* number of sections (return value) */
  int  numOfCodebook[5];  /* section length */

  if (debug['s']) {
    PRINT(SE,"total sfb %d\n", tot_sfb);
    PRINT(SE,"   sect    top   nrOfSfb  cb\n");
  }

  lenOfSectLenIncr  = sectbits[0];
  sectEscVal        = ( 1 << lenOfSectLenIncr ) - 1;
  nsect             = 0;

  for ( base = 0; ( base < tot_sfb ) && ( nsect < tot_sfb ); ) {
    lenForVcb11                   = 0;
    cntSectLenIncr                = 0;
    *sect                         = getbits ( LEN_CB );          /* read codebook from bitstream */
    sect++;
    numOfCodebook[cntSectLenIncr] = getbits ( lenOfSectLenIncr );/* section length */

    while ( numOfCodebook[cntSectLenIncr] == sectEscVal && base < tot_sfb ) {
      base += sectEscVal;
      cntSectLenIncr++;                           /* another value for the num of uses is needed */
      numOfCodebook[cntSectLenIncr] = getbits ( lenOfSectLenIncr ); /* how often is current codebook used */
    }

    /* write section data to buffer */

    if ( GetTranscodeSectionDataFlag() ) {

      /* if codebook 11 is used, reserve for each virtual codebook five bits in the buffer and fill
         it with zeros. Store the position in posCodebook11 for the SeekElemToBuffer function */
      if( *(sect-1) == 11 ) {

        for ( counter = 0; counter <= cntSectLenIncr; counter++ ) {
          lenForVcb11 += numOfCodebook[counter] * LEN_CB_VCB11;
        }

        positionCodebook11[*numOfCodebook11Sections] = GetCurrentPosition ( SECT_CB );
        WriteElemToBuffer( SECT_CB, 0, lenForVcb11 ); /* dummy */
        (*numOfCodebook11Sections)++;
        assert (*numOfCodebook11Sections < MAX_NR_CODEBOOK11 );
      } 

      else {
        /* write other than codebook 11 codebooks as usual in the buffer, but with one bit more */
        WriteElemToBuffer(SECT_CB,*(sect-1),LEN_CB_VCB11);
        /* write the number of using the same codebook in the buffer, but not for codebook = 11 (implicit) */
        for( counter = 0; counter <= cntSectLenIncr; counter++ ) {    
          WriteElemToBuffer(SECT_LEN_INCR, numOfCodebook[counter], lenOfSectLenIncr);
        }
      }
    }
    else {
      WriteElemToBuffer(SECT_CB,*(sect-1),LEN_CB);
      for( counter = 0; counter <= cntSectLenIncr; counter++ ) {    
        WriteElemToBuffer(SECT_LEN_INCR, numOfCodebook[counter], lenOfSectLenIncr);
      }
    }
    base +=  numOfCodebook[cntSectLenIncr];
    *sect = base;
    sect++;
    nsect++;
    if (debug['s'] /* && ! GetTranscodeSectionDataFlag() */) {
      PRINT(SE," %6d %6d %6d %6d\n", 
            nsect, 
            sect[-1], 
            ( nsect == 1 ? sect[-1] : sect[-1] - sect[-3]), 
            sect[-2] );
    }
    if (debug['S']) {
      StatisticsCountCbProbability ( sect[-2],
                                     ( nsect == 1 ? sect[-1] : sect[-1] - sect[-3]) );
    }

    /* insert a zero section for regions above max_sfb for each group */
    if ((sect[-1] % sfb_per_sbk) == max_sfb) {
      base   += (sfb_per_sbk - max_sfb);
      *sect = 0;
      sect++;
      *sect = base;
      sect++;
      nsect++;
      if (debug['s'] /* && ! GetTranscodeSectionDataFlag() */) {
        PRINT(SE,"(%6d %6d %6d %6d)\n", 
              nsect, 
              sect[-1], 
              ( nsect == 1 ? sect[-1] : sect[-1] - sect[-3]), 
              sect[-2] );
      }
      if (debug['S']) {
        StatisticsCountCbProbability ( sect[-2],
                                       ( nsect == 1 ? sect[-1] : sect[-1] - sect[-3]) );
      }
    }
  }
  if ( base != tot_sfb || nsect > tot_sfb ) {
    nsect = 0;
  }
  return ( nsect );
}

static int huffspec ( Info *info, int nsect, byte *sect, short *factors, Float *coef, int *longestCW, unsigned char maxSfb )
{
  Hcb*           hcb;
  Huffman*       hcw;
  int            sectionNr, sfbNr, spectralLineNr, codebook, step, temp, stop, bottom, top;
  short*         bands;
  short*         bandp;
  int            quant[LN2], *qp;	    /* probably could be short */
  int            idx, spectralLineStart;
  int            tmp_spec[LN2];
  unsigned long  word;
  unsigned long  wordext;
  unsigned short dummyLen;
  int            maxCWPerSfb; /* max value within a sfb */
  int            minCWPerSfb; /* min value within a sfb */ 
  int            maxCW = 0;
  int            minCW = 0; 
  int*           vcb11 = NULL;       /* pointer to virtual codebooks in the heap */

  if ( GetTranscodeSectionDataFlag () ) {
    for ( idx = 0; idx < ( 2 * ( MAXBANDS + 1 ) ); idx++ ) {
      vcb11Anchor[idx] = 0;
    }
    vcb11 = vcb11Anchor;
  }
  
  ResetHuffdataTable ( info->islong, maxSfb, info->num_groups );
  
  intclr(quant, LN2);
  
  bands          = info->bk_sfb_top;
  bottom         = 0;
  spectralLineNr = 0;
  bandp          = bands;

  if (debug['c']) {
    fprintf(stderr,"\n   sect  sfbNr  minValue  maxValue  vcb11\n" );
  }
  for ( sectionNr = nsect; sectionNr; sectionNr-- ) {
    codebook = sect[0];
    top   = sect[1];
    sect += 2;
    if( ( codebook == 0 ) ||
        ( codebook == INTENSITY_HCB  ) ||
        ( codebook == INTENSITY_HCB2 ) ) 
      {
        bandp          = bands + top;
        spectralLineNr = bandp[-1];
        bottom         = top;
        continue;
      }
    if ( codebook < BY4BOOKS + 1 ) 
      step = 4;
    else 
      step = 2;
    hcb               = &book[codebook];
    hcw               = hcb->hcw;
    qp                = quant + spectralLineNr;
    spectralLineStart = spectralLineNr;
    for ( sfbNr = bottom; sfbNr < top; sfbNr++ ) { /* for each sfb within the current section */
      unsigned short sfbSpectralLineStart; /* indicates the first line of the current sfb */
      unsigned short window        = 0;     /* current window/subblock */
      unsigned short lineNrPerSwb  = 0;     /* current line number within swb */
      unsigned short lineNrPerUnit = 0;
      unsigned short groupNr ;             /* number of current group */
      unsigned short pastGroup;            /* counter, that starts at 0 and runs till groupNr (current group)  */
      unsigned short firstUnitPerSfb;
      unsigned short unit;
      unsigned short unitsInSpectralDirection;
      unsigned short vcbCnt;


      maxCWPerSfb=0; 
      minCWPerSfb=0;

      if ( info->islong ) {
        unitsInSpectralDirection = LN2 / 4;
      }
      else {
        unitsInSpectralDirection = SN2 / 4;
      }
      sfbSpectralLineStart = spectralLineNr;
      groupNr = sfbNr / info->sfb_per_sbk[0];
      for ( pastGroup = 0; pastGroup < groupNr; pastGroup++) {
        window += info->group_len[pastGroup];
      }
      stop = *bandp++;
      firstUnitPerSfb = ( ( ( unsigned short ) ( ( spectralLineNr + 1 ) / 4 ) ) - unitsInSpectralDirection * window ) 
        / info->group_len[groupNr];
      unit = firstUnitPerSfb;
      while ( spectralLineNr < stop ) {
        unsigned long  count;
        unsigned short linesPerSwb; /* how much lines are within the swb */
        linesPerSwb = ( stop - sfbSpectralLineStart ) / info->group_len[groupNr];
        if ( lineNrPerSwb >= linesPerSwb ) {
          window++; 
          lineNrPerSwb = 0;
          unit = firstUnitPerSfb;
          lineNrPerUnit = 0;
        }
        if ( lineNrPerUnit >= 4 ) {
          lineNrPerUnit = 0;
          unit++;
        }
#if 0
        PRINT(SE,"lineNrPerSwb: %2i, linesPerSwb: %2i, window: %i, unit: %3i\n", 
              lineNrPerSwb, linesPerSwb, window, unit);
#endif
        lineNrPerSwb  += step;
        lineNrPerUnit += step;
        clearbitstreamword();
        count = GetBitCnt();
        temp  = decode_huff_cw ( hcw,1, &dummyLen );
        unpack_idx ( qp, temp, hcb );
	      
        if ( ! hcb->signed_cb ) {
          get_sign_bits ( qp, step );
        }
        if ( codebook == ESCBOOK ) {
          qp[0] = getescape(qp[0]);
          qp[1] = getescape(qp[1]);
          if (debug['S']) {
            StatisticsUsedEscapes ( abs( qp[0] ), abs ( qp[1] ), sfbNr, nsect - sectionNr );
          }
        }
        /* compute the max/min value within a sfb */
        if(qp[0] > maxCWPerSfb)
          maxCWPerSfb = qp[0];
        if(qp[1] > maxCWPerSfb)
          maxCWPerSfb = qp[1];

        if(qp[0] < minCWPerSfb)
          minCWPerSfb = qp[0];
        if(qp[1] < minCWPerSfb)
          minCWPerSfb = qp[1];
        /* max min per frame */
        if(qp[0] > maxCW)
          maxCW = qp[0];
        if(qp[1] > maxCW)
          maxCW = qp[1];
        if(qp[0] < minCW)
          minCW = qp[0];
        if(qp[1] < minCW)
          minCW = qp[1];

        word = getbitstreamword ( &wordext );
        if (debug['S']) {
          StatisticsCountBitsUsedForCodebooks ( codebook,
                                                GetBitCnt() - count,
                                                MAX ( abs ( qp[0] ), abs ( qp[1] ) ) );
        }
        if ( GetReorderSpecFlag () ) {
          AddHuffdataTable ( nsect - sectionNr,            /* section */
                             groupNr,                      /* number of group */
                             info->group_len[groupNr],     /* number of windows within group */
                             sfbNr,                        /* sfb */
                             sfbNr % info->sfb_per_sbk[0], /* sfb in group */
                             window,                       /* current window */
                             unit,                         /* current unit */
                             step,                         /* dimension of codeword */
                             codebook,                     /* huffman code codebook */
                             hcb->maxCWLen,                /* maximum codeword length of current codebook */
                             word,                         /* lower 32 bit of codeword */
                             wordext,                      /* upper 32 bit of codeword */
                             GetBitCnt() - count           /* length of codeword */
                             );
#if 0
          PRINT(SE,"sfb %2i,spectralLineNr: %4i, unit %3i\n",sfbNr,spectralLineNr,unit);
#endif
        }
        qp += step;
        spectralLineNr  += step;
      }
      
      /* NEW section data coding for codebook 11 */
      if ( GetTranscodeSectionDataFlag() && codebook == 11 ) {
        /* search for the current values the best codebook: 11 or 16..31 */
        /* the best codebook ist the first, which is larger than the absolute value */
        *vcb11 = ESCBOOK;
        for ( vcbCnt = FIRSTVCB11BOOK; vcbCnt <= LASTVCB11BOOK; vcbCnt++ ) {
          if( book[vcbCnt].lavInclEsc >= MAX(abs(minCWPerSfb),abs(maxCWPerSfb)) ) {
            *vcb11 = vcbCnt;
            break;
          }
        }

        UpdateVirtualCodebook ( sfbNr,                        /* section */
                                *vcb11,                       /* virtual codebook */
                                (&book[*vcb11])->maxCWLen );  /* maximum codeword length of current virtual codebook */

        if (debug['c']) {
          fprintf(stderr,"   %4d  %4d   %7d   %7d   %4d\n", nsect-sectionNr+1, sfbNr, minCWPerSfb, maxCWPerSfb, *vcb11 );
        }
        vcb11++;
      }

      if(debug['q']) {
        PRINT(SE,"\nsect %d %d\n", codebook, spectralLineStart);
        for ( idx = spectralLineStart; idx < spectralLineNr; idx++ ) {
          PRINT(SE,"%5i",quant[idx]);
        }
        PRINT(SE,"\n");
      }
    } /* end of sfbNr loop */
    bottom = top;
    if( GetTranscodeSectionDataFlag() && codebook == 11 ) {
      *vcb11++ = 0; /* each section with virtual codebooks is finished with zero */
    }
  }

  if (debug['c']) {
    fprintf( stderr, "                ------------------\n");
    fprintf( stderr, "   total         %6d    %6d\n", minCW, maxCW );
  }
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts ("spectral_data_bits");
  
  *longestCW = GetLengthOfLongestCodeword(); 

  /* NEC noisless coding reconstruction */
  if ( (info->islong) && (nec_info.pulse_data_present) )
    nec_nc(quant, &nec_info);
  
  if (!info->islong) 
    {
      deinterleave ( quant,
                     tmp_spec,
                     info->num_groups,   
                     info->group_len,
                     info->sfb_per_sbk,
                     info->sfb_width_128 );
      memcpy(quant,tmp_spec,sizeof(tmp_spec));
    }
  if ( debug['Q'] )
    PrintSpectralValues ( quant );
    
  /* inverse quantization */
  {
    unsigned short i;
    for (i = 0; i < info->bins_per_bk; i++)
      coef[i] = esc_iquant(quant[i]);
  }

  /* rescaling */
  {
    unsigned short i;
    unsigned short k;
    int sbk, nsbk, sfb, nsfb, fac, top;
    Float *fp, scale;
    
    i = 0;
    fp = coef;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) 
      {
	nsfb = info->sfb_per_sbk[sbk];
	k=0;
	for (sfb=0; sfb<nsfb; sfb++) 
	  {
	    top = info->sbk_sfb_top[sbk][sfb];
	    fac = factors[i++]-SF_OFFSET;
	    
	    if (fac >= 0 && fac < TEXP) 
	      scale = exptable[fac];
	    else 
	      if (fac == -SF_OFFSET) 
		scale = 0;
	      else 
		scale = pow( 2.0,  0.25*fac );
	    for ( ; k<top; k++) 
	      *fp++ *= scale;
	  }
      }
  }
  return 1;
}

void getgroup(Info *info, byte *group)
{
  int i, j, first_short;
  
  if( debug['g'] ) PRINT(SE,"Grouping: 0");
  first_short=1;
  for (i = 0; i < info->nsbk; i++) 
    {
      if (info->bins_per_sbk[i] > SN2) 
	{
	  /* non-short windows are always their own group */
	  *group++ = i+1;
	}
      else 
	{
	  /* only short-window sequences are grouped! */
	  if (first_short) 
	    {
	      /* first short window is always a new group */
	      first_short=0;
	    }
	  else 
	    {
              j = getbits(1);
              WriteElemToBuffer(SCALE_FACTOR_GROUPING, j, 1);
	      if(j == 0) 
		{
		  *group++ = i;
		}
	      if( debug['g'] ) PRINT(SE,"%1d", j);
	    }
	}
    }
  *group = i;
  if( debug['g'] ) PRINT(SE,"\n");
}

/*
 *  read a synthesis mask
 *  uses EXTENDED_MS_MASK
 *  and grouped mask 
 */
int getmask(Info *info, byte *group, byte max_sfb, byte *mask)
{
  int b, i, mp;
  
  mp = getbits(LEN_MASK_PRES);
  WriteElemToBuffer(MS_MASK_PRESENT, mp, LEN_MASK_PRES);
  if( debug['m'] )
    PRINT(SE,"\nExt. Mask Present: %d\n",mp);
  
  /* special EXTENDED_MS_MASK cases */
  if(mp == 0) 
    { /* no ms at all */
      return 0;
    }
  if(mp == 2) 
    {/* MS for whole spectrum on, mask bits set to 1 */
      for(b = 0; b < info->nsbk; b = *group++)
	for(i = 0; i < info->sfb_per_sbk[b]; i ++)
	  *mask++ = 1;
      return 2;
    }
  
  /* otherwise get mask */
  for(b = 0; b < info->nsbk; b = *group++)
    {
      if( debug['m'] ) PRINT(SE," gr%1i:",b);
      for(i = 0; i < max_sfb; i ++) 
	{
	  *mask = getbits(LEN_MASK);
          WriteElemToBuffer(MS_USED, *mask, LEN_MASK);
	  if( debug['m'] )PRINT(SE,"%1i",*mask);
	  mask++;
	}
      for( ; i < info->sfb_per_sbk[b]; i++)
	{
	  *mask = 0;
	  if( debug['m'] ) PRINT(SE,"%1i",*mask);
	  mask++;
	}
    }
  if( debug['m'] ) PRINT(SE,"\n");
  return 1;
}

void clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int s;
  
  tns_frame_info->n_subblocks = info->nsbk;
  for (s=0; s<tns_frame_info->n_subblocks; s++)
    tns_frame_info->info[s].n_filt = 0;
}

int get_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s, j;
  short                     *sp;
  unsigned short            tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static unsigned short     sgn_mask[] =   { 0x2, 0x4, 0x8 };
  static unsigned short     neg_mask[] =   { 0xfffc, 0xfff8, 0xfff0 };
  unsigned long             count = GetBitCnt();

  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;
  
  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];
    tns_info->n_filt =  getbits( short_flag ? 1 : 2 );
    WriteElemToBuffer(N_FILT, tns_info->n_filt,  short_flag ? 1 : 2);
    if ( !tns_info->n_filt )
      continue;
    j = getbits(1);
    WriteElemToBuffer(COEF_RES, j, 1);
    tns_info -> coef_res = res = j + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  
      {
        tns_filt->stop_band = top;
        j = getbits( short_flag ? 4 : 6);
        WriteElemToBuffer(LENGTH, j,  short_flag ? 4 : 6);
        top = tns_filt->start_band = top - j;
        tns_filt->order = getbits( short_flag ? 3 : 5 );
        WriteElemToBuffer(ORDER, tns_filt->order,  short_flag ? 3 : 5);
        if (tns_filt->order)  
          {
            tns_filt->direction = getbits( 1 );
            WriteElemToBuffer(DIRECTION, tns_filt->direction, 1);
            compress = getbits( 1 );
            WriteElemToBuffer(COEF_COMPRESS, compress, 1);
            res2 = res - compress;
            s_mask = sgn_mask[ res2 - 2 ];
            n_mask = neg_mask[ res2 - 2 ];
            
            sp = tns_filt -> coef;
            for (t=tns_filt->order; t>0; t--)  {
              tmp = getbits( res2 );
              WriteElemToBuffer(COEF, tmp, res2);
              *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
            }
          }
        tns_filt++;
      }
  }   /* subblock loop */
  StatisticsTnsUsed ( GetBitCnt() - count, short_flag );
  return 1;
}

int getics(Info *info, int common_window, byte *win, byte *wshape, 
	   byte *group, byte max_sfb, int *lpflag, int *prstflag, 
	   int *pns, byte *sect, Float *coef, unsigned short *global_gain, 
	   short *factors, TNS_frame_info *tns)
{
  int nsect, i, tot_sfb, ret;
  unsigned short tns_data_present;
  unsigned short gain_control_data_present;
  unsigned short posLongestCW = 0;
  int            longestCW;
  int            numOfCodebook11Sections = 0;
  int            positionCodebook11[MAX_NR_CODEBOOK11];
  unsigned short scfFlag;
 
  /*
   * global gain
   */
  *global_gain = getbits(LEN_SCL_PCM);  
  WriteElemToBuffer(GLOBAL_GAIN,*global_gain,LEN_SCL_PCM);
  if (debug['f'])
    PRINT(SE,"global gain: %3d\n", *global_gain);
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts("startup_info_bits");
  if (!common_window)
    get_ics_info(win, wshape, group, &max_sfb, lpflag, prstflag, common_window);
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts("ics_info_bits");
  memcpy(info, winmap[*win], sizeof(Info));
  
  /* calculate total number of sfb for this grouping */
  if ( max_sfb == 0 ) 
    tot_sfb = 0;
  else 
    {
      i = 0;
      if (debug['f'])
	{
	  fprintf(stderr,"nsbk = %i (subblocks per block)\n", info->nsbk);
	  fprintf(stderr,"sfb_per_sbk = %i (scalefactor bands per subblock)\n", info->sfb_per_sbk[0]);
	}
      tot_sfb = info->sfb_per_sbk[0];
      if (debug['f'])
	PRINT(SE,"group[%d] lasts till subblock %i, total sfb = %d\n", i, group[i]-1, tot_sfb);
      while ( group[i++] < info->nsbk )
	{
	  tot_sfb += info->sfb_per_sbk[0];
	  if (debug['f'])
	    PRINT(SE,"group[%d] lasts till subblock %i, total sfb = %d\n", i, group[i]-1, tot_sfb);
	}
    }
  
  /* 
   * section data
   */
  nsect = huffcb(sect, 
                 info->sectbits, 
                 tot_sfb, 
                 info->sfb_per_sbk[0], 
                 max_sfb, 
                 positionCodebook11,
                 &numOfCodebook11Sections);
  if (debug['S']) {
    if ( *win == EIGHT_SHORT_WINDOW ) {
      StatisticsCountNumberOfSectionsShort ( nsect );
    }
    else {
      StatisticsCountNumberOfSectionsLong  ( nsect );
    }
  }
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts ("section_data_bits");
  *pns = nsect;
  if ( nsect == 0 && max_sfb > 0 )
    return 0;
  
  /* calculate band offsets
   * (because of grouping and interleaving this cannot be
   * a constant: store it in info.bk_sfb_top)
   */
  calc_gsfb_table(info, group);
  
  /*
   * scale factor data
   */
  BeginScfRead(); /* for sreeni debuging only */

  scfFlag = GetResilienceScfFlag ( );
  switch ( scfFlag ) {
  case 0:
  case 1:
    if( !hufffac( info, group, nsect, sect, *global_gain, factors ) ) {
      return 0;
    }
    break;
  case 2:
  case 3:
    if( !RVLCScfDecoding( info, group, nsect, sect, *global_gain, factors ) ) {
      return 0;
    }
    break;
  default:
    myexit ("impossible scf resilience mode!");
    break;
  }

  if (debug['S'])
    StatisticsCountBitsForBitstreamParts ("scale_factor_data_bits");
  EndScfRead(); /* for sreeni debuging only */
#if 0
  scf_trnsc ( GetScfBits(),                  /* number of bits used for scf encoding in the original bitstream */
              GetBeginFlagForScfResilience() /* begin flag */ 
              samp_rate_info[mc_info.sampling_rate_idx].samp_rate      /* sampling rate */
              info->islong,                  /* window type */
              info->num_groups,              /* number of groups */
              info->sfb_per_sbk[0],          /* number of scalefactors per group */
              factors;                       /* scalefactors itself */
              );
#endif
#if 0
  {
    unsigned short i, j;
    PRINT(SE,"number of bits used for scalefactors in the original bitstream: %lu\n", GetScfBits());
    PRINT(SE,"begin flag: %hu\n", GetBeginFlagForScfResilience() );
    PRINT(SE,"sampling rate: %i\n", samp_rate_info[mc_info.sampling_rate_idx].samp_rate );
    PRINT(SE,"window type (info->islong): %i\n", info->islong);
    PRINT(SE,"number of groups (info->num_groups): %i\n", info->num_groups);
    PRINT(SE,"number of scalefactors per group (info->sfb_per_sbk[0]): %i\n", info->sfb_per_sbk[0]);
    for ( j = 0; j < info->num_groups; j++ ) {
      for ( i = 0; i < info->sfb_per_sbk[0]; i++ ) {
        PRINT(SE,"scf[%hu][%2hu]=%4hi\n", j, i, factors[i]);
      }
    }
  }
#endif
  /*
   * NEC noiseless coding
   */
  nec_info.pulse_data_present = getbits(LEN_PULSE_PRES);
  WriteElemToBuffer(PULSE_DATA_PRESENT, nec_info.pulse_data_present, LEN_PULSE_PRES);
  if (nec_info.pulse_data_present) 
    {
      if (info->islong) 
	{
	  get_nec_nc(&nec_info);
	}
      else 
	{
	  PRINT(SE, "Pulse data not allowed for short blocks\n");
	  myexit("nec_nc");
	}
    }
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts ("pulse_data_bits");
  
  /*
   * tns data
   */
  tns_data_present = getbits(LEN_TNS_PRES);
  WriteElemToBuffer(TNS_DATA_PRESENT, tns_data_present, LEN_TNS_PRES);
  if (tns_data_present) 
    {
      get_tns(info, tns);
    }
  else 
    {
      clr_tns(info, tns);
    }
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts ("tns_data_bits");
  
  /*
   * Sony gain control
   */
  gain_control_data_present = getbits(LEN_GAIN_PRES);
  WriteElemToBuffer(GAIN_CONTROL_DATA_PRESENT, gain_control_data_present, LEN_GAIN_PRES);
  if (gain_control_data_present) {
    PRINT(SE, "Gain control not implmented\n");
    myexit("gain_ctl");
  }
  if (debug['S'])
    StatisticsCountBitsForBitstreamParts ("gain_control_data_bits");


  /*
   * if command line switch -l is on, write zero to bitstream, because longest CW is not yet
   * available
   */

  if( GetLongestCWAvailable() == 1) {
    WriteElemToBuffer(LENGTH_OF_LONGEST_CODEWORD, 0, LEN_LONGEST_CW);
    posLongestCW = GetCurrentPosition(LENGTH_OF_LONGEST_CODEWORD) - LEN_LONGEST_CW;
  }

  /*
   * Huffman decoding of spectral values
   */

  ret = huffspec ( info, nsect, sect, factors, coef, &longestCW, max_sfb);

  /* Longest CW is now available */

  if( GetLongestCWAvailable() == 1) {
    SeekElemToBuffer(LENGTH_OF_LONGEST_CODEWORD, longestCW, LEN_LONGEST_CW, posLongestCW);
  }

  
  /*
   * reordering algorithm for Huffman codewords within spectral data
   */
  
  if ( GetReorderSpecFlag () ) {
    CompletionHuffdataTable ( ); /* window allocation for codewords */
  }
  
  if ( debug['h'] )
    PrintHuffdataTable ( );
  WindowInterleaveHuffdataTable ( ); 
  if ( debug['i'] )
    {
      if ( *win == EIGHT_SHORT_WINDOW )
        fprintf(stderr,"short block interleaving:\n\n");
      else
        fprintf(stderr,"long block interleaving:\n\n");
      PrintHuffdataTable ( );
    }
  if ( GetReorderSpecFlag() )
    ReorderSpec();

  /* write new section data */
  if ( GetTranscodeSectionDataFlag () ) {
    IncludeVcb11 ( info->sectbits, numOfCodebook11Sections, positionCodebook11 );
  }
  return ( ret );
}

/* 
 * get scale factors
 */
int hufffac(Info *info, byte *group, int nsect, byte *sect,
	    unsigned short global_gain, short *factors)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
  unsigned short scfFlag;
  /* unsigned short posLenSfHuffCode; */   /* position of len_sf_huff_code in the buffer of the class */
  unsigned short lenSfHuffCode = 0;  /* the current length of the sf data */
  unsigned short numDecodedBits;   /* length of the current scale factor in bits */
  unsigned short elemenLen;          /* depends on block type */

  static unsigned long totalLength = 0;

  /* read the scf flag: if equal to one write the length of sf data before sf data */
  scfFlag = GetResilienceScfFlag();
  
  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);

  for ( i = 0; i < MAXBANDS; i++ )
    factors[i] = 0;
  
  sfb = 0;
  fac_trans = factor_transmitted;
  for ( i = 0; i < nsect; i++ )
    {
      top = sect[1];		/* top of section in sfb */
      t = sect[0];		/* codebook for this section */
      sect += 2;
      for ( ; sfb < top; sfb++ ) 
        fac_trans[sfb] = t;
    }
  
  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
  
  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    PRINT(SE,"\nscale factors\n");

  /* write zeros at the position of the length of sf data */
  if( scfFlag == 1 ) {
    elemenLen = ( info->islong ? LEN_LONG_LENGTH_OF_SF_DATA : LEN_SHORT_LENGTH_OF_SF_DATA );
    /* WriteElemToBuffer(LENGTH_OF_SF_DATA, 0, elemenLen); */
    /* posLenSfHuffCode = GetCurrentPosition(LENGTH_OF_SF_DATA) - elemenLen;*/
  }

  for(b = 0; b < info->nsbk; )
    {
      n = info->sfb_per_sbk[b];
      b = *group++;
      for(i = 0; i < n; i++)
	{
	  if(fac_trans[i])
	    {
	      /* decode intensity positions */
	      if ( (info->nsbk==1) &&	    /* No short blocks yet! */
		   ( (fac_trans[i] == INTENSITY_HCB) || 
		     (fac_trans[i] == INTENSITY_HCB2) ) ) 
		{
                  /* decode intensity position */
		  t = decode_huff_cw(hcw,0,&numDecodedBits);
                  lenSfHuffCode += numDecodedBits;
		  is_pos += t - MIDFAC;
		  if (debug['f'])
		    PRINT(SE,"%3d %3d (is_pos)\n", i, is_pos);
		  factors[i] = is_pos;
		  continue;
		}
	      /* decode scale factor */
	      t = decode_huff_cw(hcw,0,&numDecodedBits);
              lenSfHuffCode += numDecodedBits;
	      fac += t - MIDFAC;    /* 1.5 dB */
	      
	      if (debug['f'])
		PRINT(SE,"%3d", fac-global_gain); /* (-global_gain) included by RS */
	      
	      if( fac >= 2 * maxfac || fac < 0 )
		return 0;
	      
	      factors[i] = fac;
	    }
	}
      if (debug['f'])
	PRINT(SE,"\n");
      
      /* expand short block grouping */
      if ( ! ( info->islong ) ) 
        for ( bb++; bb < b; bb++ ) 
          {
            for ( i = 0; i < n; i++ ) 
              factors[i + n] = factors[i];
            factors += n;
          }
      fac_trans += n;
      factors += n;
    }
  if( scfFlag == 1 ) {
  /*SeekElemToBuffer(LENGTH_OF_SF_DATA, lenSfHuffCode, elemenLen, posLenSfHuffCode); */
    totalLength += lenSfHuffCode;
  }

  return 1;
}

/* rm2 inverse quantization
 * escape books need ftn call
 * other books done via macro
 */
#define iquant( q ) ( q >= 0 ) ? (Float)iq_exp_tbl[ q ] : (Float)(-iq_exp_tbl[ - q ])

Float esc_iquant(int q)
{
  if (q > 0) 
    {
      if (q < MAX_IQ_TBL) 
	{
	  return((Float)iq_exp_tbl[q]);
	}
      else 
	{
	  return(pow(q, 4./3.));
	}
    }
  else 
    {
      q = -q;
      if (q < MAX_IQ_TBL) 
	{
	  return((Float)(-iq_exp_tbl[q]));
	}
      else 
	{
	  return(-pow(q, 4./3.));
	}
    }
}




int byte_align ( )
{
  int i = 0;
  int j;
  
  if (debug['a'])
    PRINT(SE,"align: %lx %d\n", cword, nbits);
  while (nbits & ((1<<3)-1)) {
    j = getbits(1);
    /* Due to new data elements, the number of bits used for byte alignment 
       within the intput bitstream may differ from that in the output bitstream.
       Therefore, these bits are not directly put into any class but are newly derived
       by the function WriteBufferToFile() and stored at the end of the last class */
    i++;
  }
  return(i);
}

unsigned long GetBitCnt ( )
{
  return( nBitCount );
}

void ResetBitCount ( )
{
  nBitCount = 0;
}

static unsigned long bitstreamword;
static unsigned long bitstreamwordext;

unsigned long getbitstreamword( unsigned long *wordext )
{
  *wordext = bitstreamwordext;
  return( bitstreamword );
}

void clearbitstreamword( )
{
  bitstreamword = 0;
  bitstreamwordext = 0;
}

long getbits(int n)
{
  long value;
  /*   int  debug = 0; */
  
  if ( nbits <= 16 ) {
    cword = ( cword<<16 ) | getshort();
    nbits += 16;
  }
  
  nbits -= n;
  nBitCount += n; /* nbitcout counts the amount of bits read from the bitstream */
  value = (cword>>nbits) & ((1<<n)-1);
  /*   if( debug ) fprintf(stderr,"xx  n %2d   word %08lx",n, bitstreamword); */
  bitstreamwordext = bitstreamwordext << n;
  /*   if( debug ) fprintf(stderr,"  ext1 %08lx", bitstreamwordext); */
  if( n > 0 )
    bitstreamwordext = bitstreamwordext | (bitstreamword >> (32-n));
  /*   if( debug ) fprintf(stderr,"  ext2 %08lx", bitstreamwordext); */
  bitstreamword = bitstreamword << n;
  /*   if( debug ) fprintf(stderr,"  shiftword %08lx", bitstreamword); */
  bitstreamword = bitstreamword | value;
  /*   if( debug ) fprintf(stderr,"  orword %08lx\n", bitstreamword); */
  return value;
}

int getescape(int q)
{
  int i, off, neg, j;
  
  if(q < 0)
    {
      if(q != -16)
	return q;
      neg = 1;
    } else
      {
	if(q != +16)
	  return q;
	neg = 0;
      }
  
  for(i=4;; i++)
    {
      j = getbits(1);
      if ( ! GetReorderSpecFlag() ) {
        WriteElemToBuffer(HCOD_ESC, j, 1);
      }
      if(j == 0)
	break;
    }
  
  if(i > 16)
    {
      j = getbits(i-16);
      if ( ! GetReorderSpecFlag() ) {
        WriteElemToBuffer(HCOD_ESC, j, i-16);
      }
      off = j << 16;
      j = getbits(16);
      if ( ! GetReorderSpecFlag() ) {
        WriteElemToBuffer(HCOD_ESC, j, 16);
      }
      off |= j;
    } 
  else {
    j = getbits(i);
    if ( ! GetReorderSpecFlag() ) {
      WriteElemToBuffer(HCOD_ESC, j, i);
    }
    off = j;
  }
  
  i = off + (1<<i);
  if(neg)
    i = -i;
  return i;
}
