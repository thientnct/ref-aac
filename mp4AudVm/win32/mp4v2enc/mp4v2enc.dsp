# Microsoft Developer Studio Project File - Name="Mp4v2enc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Mp4v2enc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mp4v2enc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mp4v2enc.mak" CFG="Mp4v2enc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mp4v2enc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Mp4v2enc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Mp4v2enc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\lib2001\libisomediafile\w32" /I "..\lib2001\libisomediafile\src" /I "..\..\src_mp4" /I "..\..\src_ssc_enc" /I "..\..\src_ssc_enc\include" /I "..\..\src_tf_enc" /I "..\..\src_par" /I "..\..\src_par_enc" /I "..\..\src_frame_enc" /I "..\..\src_lpc" /I "..\..\src_lpc_enc" /I "..\..\src_g723" /I "..\..\src_g723_enc" /I "..\..\src_g729" /I "..\..\src_g729_enc" /I "..\..\..\import\include" /I "..\..\src_tf" /I "..\..\src_frame" /I "..\..\src_sls" /D "PLATFORM_WIN32_MSVC" /D "SSC_DEC_LEVEL_MEDIUM" /D "_CONSOLE" /D "_MBCS" /D "VERSION2" /D "AL_CONFIG" /D "USE_AFSP" /D "MP4FF" /D "CORRIGENDUM1" /D "SRS" /D "PNS" /D "SSC_APP_BUILD" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcmt.lib" /out:"..\bin\release\mp4auenc.exe" /libpath:"..\..\..\import\lib\release"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Mp4v2enc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /ZI /Od /I "..\lib2001\libisomediafile\w32" /I "..\lib2001\libisomediafile\src" /I "..\..\src_mp4" /I "..\..\src_ssc_enc" /I "..\..\src_ssc_enc\include" /I "..\..\src_tf_enc" /I "..\..\src_par" /I "..\..\src_par_enc" /I "..\..\src_frame_enc" /I "..\..\src_lpc" /I "..\..\src_lpc_enc" /I "..\..\src_g723" /I "..\..\src_g723_enc" /I "..\..\src_g729" /I "..\..\src_g729_enc" /I "..\..\..\import\include" /I "..\..\src_tf" /I "..\..\src_frame" /I "..\..\src_sls" /D "PLATFORM_WIN32_MSVC" /D "USE_SSC" /D "SSC_DEC_LEVEL_MEDIUM" /D "_CONSOLE" /D "_MBCS" /D "VERSION2" /D "AL_CONFIG" /D "USE_AFSP" /D "MP4FF" /D "CORRIGENDUM1" /D "SRS" /D "PNS" /D "SSC_APP_BUILD" /D "_DEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FR /FD /c
# SUBTRACT CPP /X /YX
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /nodefaultlib:"libcmtd.lib" /out:"..\bin\debug\mp4auenc.exe" /pdbtype:sept /libpath:"..\..\..\import\lib\debug"

!ENDIF 

# Begin Target

# Name "Mp4v2enc - Win32 Release"
# Name "Mp4v2enc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_mp4\ep_convert.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame_enc\mp4auenc.c
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
# Begin Source File

SOURCE=..\..\..\import\lib\libtsp.lib
# End Source File
# End Target
# End Project
