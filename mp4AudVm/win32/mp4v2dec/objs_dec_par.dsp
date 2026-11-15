# Microsoft Developer Studio Project File - Name="objs_dec_par" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_dec_par - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_dec_par.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_dec_par.mak" CFG="objs_dec_par - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_dec_par - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_dec_par - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_dec_par - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_frame" /I "..\..\src_lpc" /I "..\..\src_g723" /I "..\..\src_g729" /I "..\..\..\import\include" /D "NDEBUG" /D "_WINDOWS" /D "VERSION2" /D "AL_CONFIG" /D "USE_AFSP" /D "MP4FF" /D "CORRIGENDUM1" /D "SRS" /D "PNS" /D "WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_dec_par - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /Z7 /Od /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_frame" /I "..\..\src_lpc" /I "..\..\src_g723" /I "..\..\src_g729" /I "..\..\..\import\include" /D "_DEBUG" /D "_WINDOWS" /D "VERSION2" /D "AL_CONFIG" /D "USE_AFSP" /D "MP4FF" /D "CORRIGENDUM1" /D "SRS" /D "PNS" /D "WIN32" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "objs_dec_par - Win32 Release"
# Name "objs_dec_par - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_par\dec_par.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxc_lpc_dmy.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcCommon.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcFltDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcItpDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcProtect.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcQAmDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcQLspDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcSynUVDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcSynVDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcUVExtGenDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcVExtGenDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\hvxcVxcDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\indilinecom.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\indilinedec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\indilineqnt.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\indilinesyn.c
# End Source File
# Begin Source File

SOURCE=..\..\src_par\uhd_fft.c
# End Source File
# End Group
# End Target
# End Project
