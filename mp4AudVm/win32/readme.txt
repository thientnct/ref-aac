
README FILE FOR THE WIN32 PROJECT FILES FOR THE MPEG-4 AUDIO REFERENCE SOFTWARE



These are the Workspace files for Microsoft Visual C++ 6.0 required for
compilation of the MPEG-4 Audio Reference Software.

.\%ROOT%\mp4AudVm\win32\mp4v2dec\mp4v2dec.dsw    MPEG-4 Version 2 Audio Decoder
.\%ROOT%\mp4AudVm\win32\mp4v2enc\mp4v2enc.dsw    MPEG-4 Version 2 Audio Encoder
.\%ROOT%\mp4AudVm\win32\mp4tools\mp4tools.dsw    Conversion tools

The MPEG-4 Audio Reference Software uses the AFsp-library for reading
and writing audio files. In addition, the MPEG-4 Version 2 Decoder
supports the MPEG-4 File format. For this, the MPEG-4 File Format
reference software library provided by ISO is required. These two
libraries are not part of the MPEG-4 Audio Reference Software and have
to be obtained externally. The Workspace files for Microsoft Visual C++
6.0 assumes their library files and include files in the following
locations:


.\%ROOT%\import\lib                               Library files of AFsp-libary and
                                                  MP4FF library (libtsp.lib and 
                                                  libisomp4.lib)

.\%ROOT%\import\include                           Include files of AFsp-libary and
                                                  MP4FF library (MP4Movies.h, 
                                                  MP4OSMacros.h, libtsp.h, AO.h and
                                                  AObase.h)

.\%ROOT%\import\include\libtsp                    The other include files of AFsp-libary


The library files must be built for 'debug' or 'release' and must use the 
(debug) multi-threaded versions of the run-time libraries.


