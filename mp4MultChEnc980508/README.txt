AAC Technical Report Encoder Notes
Chuck Lueck, TI <lueck@ti.com>
22-jan-98

Additional notes by
Ralf Geiger, FhG/IIS
07-apr-98
===============================================

The setup to compile and run is very much the same as it is for the
MPEG-4 VM, since the encoder framework was based on the VM.  Most
of the MPEG-4 VM documentation relating to AAC still applies.

Support for three platforms is included: SunOS4, Linux, and Solaris. 
Before compiling, one needs to set the environment variable SYSTEM_NAME to
either SunOS, Linux, or Solaris, such as:

setenv SYSTEM_NAME SunOS

The encoder can then be compiled/linked using make.  The encoder 
compiles and links using g++, and I have recently had success 
compiling using gcc as well.

In order to use raw data files as input, actually the only kind 
I've tested, the AFopenRead function requires that the 
MP4_RAWAUDIOFILE environment variable be set as follows:

setenv MP4_RAWAUDIOFILE "integer16, 0, 48000.0, native, 1, 1.0"

where the 48000.0 is the sampling rate, the 1 means one channel,
and the 1.0 means a gain of 1.0.  Other sampling rates can be
used, but I think that currently the psychoacoustic model only likes
44100 and 48000.  For stereo operation, the MP4_RAWAUDIOFILE
variable should be set as:

setenv MP4_RAWAUDIOFILE "integer16, 0, 48000.0, native, 2, 1.0"

For stereo operation, a channel_pair_element will be encoded in the
bitstream.  For multichannel operation, single_channel_elements and
channel_pair_elements are currently allocated automatically. 

If -lf is switched, the encoder can also process lfe-channel-elements. 
The last channel is then treated as lfe channel.

Most of the general framework command line arguments from MPEG-4 still
apply.  To get help, you can type:

sunos/enc -h

or to encode a raw audio file, audio.dat, at 64 kbps using the Main
profile (by default) you can simply type:

sunos/enc -r 64000 -eo .raw audio.dat

where the -eo .raw specifies a raw data file as input. The executable 
will be built into the sunos, sol, or linux directory depending on your 
platform.  The encoder will attach a .aac extension to the encoded
file.

The other two profiles, SSR and low-complexity, are also supported 
and can be specified using the -c switch followed by the encoder 
parameter string -lc or -ssr, such as:

sunos/enc -c -ssr -r 64000 -eo .raw audio.dat
sunos/enc -c -lc -r 64000 -eo .raw audio.dat

