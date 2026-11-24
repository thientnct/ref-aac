/**********************************************************************
MPEG-4 Audio VM
MP4 container muxer (encoder side)

Writes encoded access units as samples into an ISO MP4 file using
libisomediafile. Currently supports AAC LC (audioObjectType = 2)
single track.

Source file: mp4mux.c
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ISOMovies.h"

#include "common_m4a.h"
#include "bitstream.h"
#include "mp4mux.h"
#include "bitstreamHandle.h"
#include "bitstreamStruct.h"

/* ---- module state ---- */

static ISOMovie  moov         = NULL;
static ISOMedia  media        = NULL;
static ISOTrack  track        = NULL;
static ISOHandle sampleEntryH = NULL;
static u32       sampleDuration = 0;
static int       haveSampleEntry = 0;
static char      outFileName[1024];

/* ---- helpers ---- */

/* Build AudioSpecificConfig for AAC LC (audioObjectType = 2).
   Returns number of ASC bytes written to out[]. */
static int buildASC(unsigned char *out,
                    unsigned long samplingFreq,
                    int chanCfg,
                    int frameLengthFlag)
{
  static const unsigned long freqTable[13] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
  };
  int freqIdx = -1;
  int i;
  unsigned long long acc = 0;
  int nbits = 0;
  int nbytes;

  for (i = 0; i < 13; i++)
    if (freqTable[i] == samplingFreq) { freqIdx = i; break; }

#define PUTBITS(val, n) \
  do { acc = (acc << (n)) | ((unsigned long long)(val) & ((1ULL << (n)) - 1)); nbits += (n); } while (0)

  PUTBITS(2, 5);                 /* audioObjectType = AAC LC */
  if (freqIdx >= 0) {
    PUTBITS(freqIdx, 4);         /* samplingFrequencyIndex */
  } else {
    PUTBITS(0xf, 4);             /* escape -> explicit frequency */
    PUTBITS(samplingFreq, 24);   /* samplingFrequency */
  }
  PUTBITS(chanCfg, 4);           /* channelConfiguration */
  PUTBITS(frameLengthFlag, 1);   /* GASpecificConfig: frameLengthFlag */
  PUTBITS(0, 1);                 /* dependsOnCoreCoderFlag */
  PUTBITS(0, 1);                 /* extensionFlag */

#undef PUTBITS

  nbytes = (nbits + 7) / 8;
  acc <<= (nbytes * 8 - nbits);  /* pad last byte with zero bits */
  for (i = 0; i < nbytes; i++)
    out[i] = (unsigned char)(acc >> (8 * (nbytes - 1 - i)));

  return nbytes;
}

/* ---- public functions ---- */

int MP4MuxOpen(const char *fileName,
               unsigned long samplingFreq,
               int chanCfg,
               unsigned long bitRate,
               unsigned long frameNumSample,
               int frameLengthFlag)
{
  ISOErr err = ISONoErr;
  unsigned char desc[16];
  int ascLen, descLen;
  ISOHandle ascH = NULL;

  strncpy(outFileName, fileName, sizeof(outFileName) - 1);
  outFileName[sizeof(outFileName) - 1] = '\0';

  /* DecoderSpecificInfo descriptor: tag 0x05 + length + ASC payload.
     ASC is at most 5 bytes, so length always fits in one byte. */
  ascLen = buildASC(desc + 2, samplingFreq, chanCfg, frameLengthFlag);
  desc[0] = 0x05;
  desc[1] = (unsigned char)ascLen;
  descLen = 2 + ascLen;

  err = MP4NewMovie(&moov, 1, 0xff, 0xff, 0xfe, 0xff, 0xff); if (err) return err;
  err = MP4NewMovieTrack(moov, ISONewTrackIsAudio, &track); if (err) return err;
  err = MP4AddTrackToMovieIOD(track); if (err) return err;
  err = MP4SetTrackEnabled(track, 1); if (err) return err;
  err = MP4NewTrackMedia(track, &media, ISOAudioHandlerType,
                         samplingFreq, NULL); if (err) return err;
  err = MP4BeginMediaEdits(media); if (err) return err;
  err = MP4SetMediaLanguage(media, "und"); if (err) return err;

  err = ISONewHandle(descLen, &ascH); if (err) return err;
  memcpy(*ascH, desc, descLen);

  err = ISONewHandle(0, &sampleEntryH); if (err) { ISODisposeHandle(ascH); return err; }
  err = MP4NewSampleDescription(track, sampleEntryH, 1,
                                0x40,      /* objectTypeIndication (AAC) */
                                0x05,      /* streamType = audio */
                                0,         /* decoderBufferSizeDB */
                                bitRate, bitRate,
                                ascH);
  ISODisposeHandle(ascH);
  if (err) return err;

  sampleDuration = frameNumSample;
  haveSampleEntry = 1;
  return 0;
}

int MP4MuxAddFrame(BsBitBuffer *bitBuf)
{
  ISOErr err = ISONoErr;
  long numBit = BsBufferNumBit(bitBuf);
  unsigned char *data = bitBuf->data;
  u32 length = (u32)((numBit + 7) >> 3);
  u32 pad = (u32)((8 - (numBit & 7)) & 7);
  ISOHandle dataH = NULL, sizeH = NULL, durH = NULL, padH = NULL;

  err = ISONewHandle(length, &dataH); if (err) return err;
  memcpy(*dataH, data, length);

  err = ISONewHandle(sizeof(u32), &sizeH); if (err) goto bail;
  err = ISONewHandle(sizeof(u32), &durH);  if (err) goto bail;
  err = ISONewHandle(sizeof(u8),  &padH);  if (err) goto bail;

  *(u32*)*sizeH = length;
  *(u32*)*durH  = sampleDuration;
  *(u8*)*padH   = (u8)pad;

  err = ISOAddMediaSamplesPad(media, dataH, 1, durH, sizeH,
                              haveSampleEntry ? sampleEntryH : NULL,
                              NULL, NULL, padH);

  if (haveSampleEntry) {
    ISODisposeHandle(sampleEntryH);
    sampleEntryH = NULL;
    haveSampleEntry = 0;
  }

bail:
  if (dataH) ISODisposeHandle(dataH);
  if (sizeH) ISODisposeHandle(sizeH);
  if (durH)  ISODisposeHandle(durH);
  if (padH)  ISODisposeHandle(padH);
  return err;
}

int MP4MuxClose(void)
{
  ISOErr err = ISONoErr;
  u64 duration;

  err = ISOEndMediaEdits(media); if (err) return err;
  err = ISOGetMediaDuration(media, &duration); if (err) return err;
  err = ISOInsertMediaIntoTrack(track, 0, 0, duration, 1); if (err) return err;
  err = ISOWriteMovieToFile(moov, outFileName); if (err) return err;

  ISODisposeMovie(moov);
  moov = NULL; media = NULL; track = NULL;
  return 0;
}
