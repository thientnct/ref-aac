MPEG-4 Audio VM

Readme file: README_AFsp

$Id: README_AFsp,v 1.1.1.2 1998-05-07 17:19:29+03 suonio Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
14-feb-97   HP    born
**********************************************************************


The AFsp Audio File I/O Package
-------------------------------

To provide support for a variety of audio file formats, the VM
software uses the AFsp package for all audio file i/o. The AFsp
package is written by Peter Kabal <kabal@TSP.EE.McGill.CA> and is
available via ftp:

current version:	AFsp-V2R2.tar.Z
original site:		ftp://ftp.TSP.EE.McGill.CA/pub/AFsp
mirror site:		ftp://ftp.tnt.uni-hannover.de/pub/audio/AFsp

To make the libtsp library required by the VM software, unpack
AFsp-V2R2.tar.Z, goto AFsp-V2R2/libtsp and start make. Finally the
following three files have to be copied to the vm directory tree:

vm/include/libtsp.h
vm/include/libtsp/AFpar.h
vm/<platform>/lib/libtsp.a


Excerpt from the AFsp Copying file
----------------------------------

Copyright (C) 1996 Peter Kabal
 
The AFsp audio file routines are provided free of charge without warranty of
any kind.  All components are copyrighted.  They may be distributed free of
charge provided that the recipients also acquire the right to distribute them.
In no case can this software be incorporated into a work which is distributed
for a fee.


Excerpt from the AFsp README file
---------------------------------

Oct. 29, 1996
 
Audio File I/O routines

The AFsp package is a library of routines for reading and writing audio files.
The emphasis is on providing support for the type of audio file used by the
speech processing research community.  The routines have been designed to be
easy to use, yet provide transparent support the reading of several audio file
formats.  A secondary purpose for distributing these routines is to encourage
the use of a standard audio file format for the header information in the
output files.

...

The AFsp routines form part of a signal processing library.  The full library
is available for anonymous ftp as libtsp-VxRx.tar.Z.

The AFsp routines are covered by copyright, see the file "Copying" for details
of the distribution conditions.

AFsp-V2R2.tar.Z
http://www.TSP.EE.McGill.CA/software.html
or ftp://ftp.TSP.EE.McGill.CA/pub/AFsp

Peter Kabal
Department of Electrical Engineering    McGill University
+1 514 398-7130   +1 514 398-4470 Fax
kabal@TSP.EE.McGill.CA
 

end of README_AFsp
