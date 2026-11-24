/**********************************************************************
MPEG-4 Audio VM
MP4 container muxer (encoder side)

Writes encoded access units as samples into an ISO MP4 file using
libisomediafile. Currently supports AAC LC (audioObjectType = 2)
single track.
**********************************************************************/

#ifndef _MP4MUX_H_INCLUDED
#define _MP4MUX_H_INCLUDED

#include "bitstream.h"

/* Open MP4 output file and create audio track.
   samplingFreq   : sampling frequency in Hz
   chanCfg        : MPEG-4 channelConfiguration (1=mono, 2=stereo, ...)
   bitRate        : bit rate in bit/s
   frameNumSample : samples per access unit (for sample duration)
   frameLengthFlag: 1 for 960-sample frames, 0 for 1024
   returns: 0=OK, else ISOErr */
int MP4MuxOpen(const char *fileName,
               unsigned long samplingFreq,
               int chanCfg,
               unsigned long bitRate,
               unsigned long frameNumSample,
               int frameLengthFlag);

/* Add one encoded frame (bit buffer) as an MP4 sample.
   returns: 0=OK, else ISOErr */
int MP4MuxAddFrame(BsBitBuffer *bitBuf);

/* Finalize and write MP4 file to disk.
   returns: 0=OK, else ISOErr */
int MP4MuxClose(void);

#endif /* _MP4MUX_H_INCLUDED */
