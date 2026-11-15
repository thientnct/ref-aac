# Microsoft Developer Studio Project File - Name="Mp4v2dec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Mp4v2dec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mp4v2dec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mp4v2dec.mak" CFG="Mp4v2dec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mp4v2dec - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Mp4v2dec - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Mp4v2dec - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\..\src_sls" /I "..\..\..\epTool\src" /I "..\..\src_mp4" /I "..\..\win32" /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_lpc" /I "..\..\src_ssc" /I "..\..\src_ssc\import" /I "..\..\..\import\include" /I "..\..\src_frame" /D "PLATFORM_WIN32_MSVC" /D "_CONSOLE" /D "_MBCS" /D "CT_SBR" /D "ISOMP4" /D "MP4FF" /D "VERSION2" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "SSC_MP4REF_BUILD" /D "USE_AFSP" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libisomedia.lib libtsp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcmt.lib" /out:"..\bin\release\mp4audec.exe" /libpath:"..\..\..\import\lib"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Mp4v2dec - Win32 Debug"

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
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\..\src_sls" /I "..\..\..\epTool\src" /I "..\..\src_mp4" /I "..\..\win32" /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_lpc" /I "..\..\src_ssc" /I "..\..\src_ssc\import" /I "..\..\..\import\include" /I "..\..\src_frame" /D "PLATFORM_WIN32_MSVC" /D "_CONSOLE" /D "_MBCS" /D "CT_SBR" /D "ISOMP4" /D "MP4FF" /D "NDEBUG" /D "VERSION2" /D "USE_AFSP" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "SSC_MP4REF_BUILD" /D "WIN32" /D "I2R_LOSSLESS" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libisomedia.lib libtsp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"libcmt.lib" /out:"..\bin\debug\mp4audec.exe" /libpath:"..\..\..\import\lib"
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "Mp4v2dec - Win32 Release"
# Name "Mp4v2dec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_frame\decifc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\decifc.h
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\mp4audec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_frame\mp4ifc.c
# End Source File
# End Group
# End Target
# End Project
