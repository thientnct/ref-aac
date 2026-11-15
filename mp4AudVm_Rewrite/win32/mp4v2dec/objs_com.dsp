# Microsoft Developer Studio Project File - Name="objs_com" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_com - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_com.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_com.mak" CFG="objs_com - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_com - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_com - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_com - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_lpc" /I "..\..\..\import\include" /I "..\..\..\epTool\src" /I "..\..\src_frame" /I "..\..\src_sls" /D "_WINDOWS" /D "VERSION2" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "EXT2PAR" /D "USE_AFSP" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /D "CT_SBR" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_com - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_lpc" /I "..\..\..\import\include" /I "..\..\..\epTool\src" /I "..\..\src_frame" /I "..\..\src_sls" /D "_WINDOWS" /D "CT_SBR" /D "NDEBUG" /D "VERSION2" /D "USE_AFSP" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "EXT2PAR" /D "WIN32" /D "I2R_LOSSLESS" /Fr /FD /c
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

# Name "objs_com - Win32 Release"
# Name "objs_com - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_tf\adif.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\audio.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\austream.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\bitstream.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\cmdline.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\common_m4a.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\fir_filt.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\flex_mux.c
# End Source File
# End Group
# End Target
# End Project
