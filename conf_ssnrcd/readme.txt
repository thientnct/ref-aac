Audio Conformance Verification according to
 - ISO/IEC 13818-4
 - ISO/IEC 14496-4


Software version history:

  Version nr.   Date             Authors        Description
--------------------------------------------------------------------------------------------
  1.0           26 Jan. 2000     RF             First released version.
  1.1           31 Jan. 2000     RF, TM         Cepstral distortion: only for segments
                                                with signal power in range [-50...-15] dB.
                                                Improved accuracy in calculation of 
                                                cepstral distortion.
  1.2           17 Apr. 2000     RF             MPEG-4 Version 2 Conformance criteria added.
  1.3           11 Oct. 2000     RF             Some compiler warnings removed on UNIX platforms.
  1.4           15 Nov. 2000     RF, RS         Delay compensation included.
                                                Object type based conf. criteria removed,
                                                -t <num> switch added instead. 
                                                k-bit conformance criteria included.
                                                General code clean-up.
  1.6           19 Jan. 2001     HP             Offset in conformance threshold due to
                                                rounding of PCM signals as introduced in
                                                version 1.4 removed again.
  1.8           20 Mar. 2001     RF             -t 4 switch included
                                                General code clean-up.
  1.9           12 Apr. 2006     MM             Corrected LSB criterion checking
                                                -a and -e switches included and
                                                -v switch extended
  2.0           22 Apr. 2008     MS             added PNS conformance check



RF   Ralf Funken (Philips)
TM   Takehiro Moriya (NTT)
RS   Ralph Sperschneider (FhG)
HP   Heiko Purnhagen (Hannover University)
MM   Michael Matejko (FhG)
MS   Manuela Schinn (FhG)


This tool implements the MPEG-2 conformance test for AAC and certain
MPEG-4 conformance tests for some of the MPEG-4 Audio audio
objects. Calculated are 

- RMS value of the difference signal 
- maximum absolute difference and the cepstral distortion 
- segmental SNR (required for MPEG-4 only) 
- average cepstral distortion (required for MPEG-4 only)


There are two different commandline handling modes:

old commandline handling:
===========================
Compile using: 	make
Usage:   		ssnrcd  [options] <reference inputfile> <inputfile under test>

Valid options are:
                -a           Show distribution of bit accuracy over all samples
                -d <num>     Time-offset in samples between reference inputfile and
                             inputfile under test. Positive value means inputfile under test
                             is delayed over <num> samples and the first <num>
                             samples of that input file under test are skipped
                             Negative values are permitted.
                             Default: 0 samples.
                -e [<n>]     Exponential output of values scaled to 2^n. If no n is given,
                             bit resolution of file under test will be used.
                -k <num>     Resolution for calculation of RMS and max. abs.
                             difference sample, expressed in bits.
                             Valid range: [12...24]  Default: 16 bits.
                -l <num>     Analysis segment length in samples.
                             Default: 320 samples.
                -t <num>     Conformance criteria to be applied.
                               0 : No criterion (default)
                               1 : RMS + Max. absolute difference sample
                                   using K bit resolution (see -k option)
                               2 : Segmental SNR + avg. Cepstral Distortion
                               4 : Number of bits accuracy reached for RMS criterion
                             These values may be added. For example -t 7 enables
                             all three criteria.
                -v [<num>]   Verbose level.
                               1 : Progress report (default)
                               2 : Show segments with accuracy below K bit resolution
                                   (see -k option)
                               3 : Samples with accuracy below K bit resolution
                                   (see -k option)
                               4 : Show accuracy of all samples
                -p <num> <frameSize>   PNS conformance test.
                               1 : spectral PNS conformance test for long blocks
                               2 : spectral PNS conformance test for short blocks
                               3 : temporal PNS conformance test

new commandline handling:
==========================

Compile using: 	make mpeg4conf
Usage:   		mpeg4conf  [options] <reference inputfile> <inputfile under test>

Valid options are:
                -a           Show distribution of bit accuracy over all samples
                -d <num>     Time-offset in samples between reference inputfile and
                             inputfile under test. Positive value means inputfile under test
                             is delayed over <num> samples and the first <num>
                             samples of that input file under test are skipped
                             Negative values are permitted.
                             Default: 0 samples.
                -e [<n>]     Exponential output of values scaled to 2^n. If no n is given,
                             bit resolution of file under test will be used.
                -k <num>     Resolution for calculation of RMS and max. abs.
                             difference sample, expressed in bits.
                             Valid range: [12...24]  Default: 16 bits.
                -l <num>     Analysis segment length in samples.
                             Default: 320 samples.
                -t <num>     Conformance criteria to be applied.
                               0 : No criterion (default)
                               1 : RMS + Max. absolute difference sample
                                   using K bit resolution (see -k option)
                               2 : Segmental SNR + avg. Cepstral Distortion
                               4 : Number of bits accuracy reached for RMS criterion
                             These values may be added. For example -t 7 enables
                             all three criteria.
                -v [<num>]   Verbose level.
                               1 : Progress report (default)
                               2 : Show segments with accuracy below K bit resolution
                                   (see -k option)
                               3 : Samples with accuracy below K bit resolution
                                   (see -k option)
                               4 : Show accuracy of all samples
                -p <num>     PNS conformance test.
                               1 : spectral, long blocks,  frame length: 1024
                               2 : spectral, long blocks,  frame length:  960
                               3 : spectral, long blocks,  frame length:  512
                               4 : spectral, short blocks, frame length: 1024                    
                               5 : spectral, short blocks, frame length:  960
                               6 : temporal, frame length: 1024
                               7 : temporal, frame length:  960
 

If the input files have different lengths, only the number of samples in
the shorter file will be used. The additional samples at the end of the
longer file will be ignored.

Segmental SNR and cepstral distortion are calculated only for segments
with reference signal power in the range [-50...-15] dB.  Any partial
segments at the end of the input signal are padded with zeroes when
calculating segmental SNR and cepstral distortion.  The length of the
segments can be set by using the -l option.

Both input files must have the same number of audio channels and the
same sampling rate. Audio files in wav, aiff, aifc and au format are
accepted.  The maximum number of audio channels is 64.

