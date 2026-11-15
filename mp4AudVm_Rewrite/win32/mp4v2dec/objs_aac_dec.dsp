# Microsoft Developer Studio Project File - Name="objs_aac_dec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_aac_dec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_aac_dec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_aac_dec.mak" CFG="objs_aac_dec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_aac_dec - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_aac_dec - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_aac_dec - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_lpc" /I "..\..\..\import\include" /I "..\..\..\epTool\src" /I "..\..\src_frame" /I "..\..\src_sls" /D "_WINDOWS" /D "VERSION2" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "EXT2PAR" /D "CT_SBR" /D "USE_AFSP" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_aac_dec - Win32 Debug"

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

# Name "objs_aac_dec - Win32 Release"
# Name "objs_aac_dec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_tf\bitfct.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\buffers.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\concealment_dummy.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\config.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\coupling.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\decdata.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\decoder_tf.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\escapeScfResil.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\huffdec1.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\huffdec2.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\huffdec3.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\huffinit.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\huffscfresil.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\hufftables.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\imdct.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\intensity.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\intrins.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\monopred.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\nok_lt_prediction.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\pns.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\reorderspec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\rvlcScfConceal_dummy.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\rvlcscfresil.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\statistics_aac_dummy.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\stereo.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\tns.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\tns2.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\transfo.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\util.c
# End Source File
# End Group
# End Target
# End Project
