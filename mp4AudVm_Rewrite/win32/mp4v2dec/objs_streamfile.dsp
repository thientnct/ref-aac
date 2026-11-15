# Microsoft Developer Studio Project File - Name="objs_streamfile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_streamfile - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_streamfile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_streamfile.mak" CFG="objs_streamfile - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_streamfile - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_streamfile - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_streamfile - Win32 Release"

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
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Od /I "..\lib2001\libisomediafile\w32" /I "..\lib2001\libisomediafile\src" /I "..\..\..\import\include" /I "..\..\src_tf" /I "..\..\src_lpc" /I "..\..\..\epTool\src" /I "..\..\src_frame" /I "..\..\src_sls" /D "_WINDOWS" /D "VERSION2" /D "CT_SBR" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "EXT2PAR" /D "USE_AFSP" /D "NDEBUG" /D "EPCONFIG3" /D "WIN32" /D "I2R_LOSSLESS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_streamfile - Win32 Debug"

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
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\lib2001\libisomediafile\w32" /I "..\lib2001\libisomediafile\src" /I "..\..\..\import\include" /I "..\..\src_tf" /I "..\..\src_lpc" /I "..\..\..\epTool\src" /I "..\..\src_frame" /I "..\..\src_sls" /D "_WINDOWS" /D "CT_SBR" /D "NDEBUG" /D "VERSION2" /D "USE_AFSP" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "EXT2PAR" /D "EPCONFIG3" /D "WIN32" /D "I2R_LOSSLESS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "objs_streamfile - Win32 Release"
# Name "objs_streamfile - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src_mp4\ep_convert.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_diagnose.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_fl4.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_helper.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_latm.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_mp4.c
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_raw.c
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src_mp4\ep_convert.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_diagnose.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_fl4.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_helper.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_latm.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_mp4.h
# End Source File
# Begin Source File

SOURCE=..\..\src_mp4\streamfile_raw.h
# End Source File
# End Group
# End Target
# End Project
