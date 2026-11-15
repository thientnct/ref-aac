# Microsoft Developer Studio Project File - Name="objs_sls" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_sls - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_sls.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_sls.mak" CFG="objs_sls - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_sls - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_sls - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_sls - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\src_tf_enc" /I "..\..\src_tf" /I "..\..\src_frame" /I "..\..\src_sls" /D "NDEBUG" /D "I2R_LOSSLESS" /D "WIN32" /D "_MBCS" /D "_LIB" /D "PNS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_sls - Win32 Debug"

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
# ADD CPP /nologo /W3 /GX /Zi /Od /I "..\..\src_tf_enc" /I "..\..\src_tf" /I "..\..\src_frame" /I "..\..\src_sls" /D "_MBCS" /D "_LIB" /D "PNS" /D "_DEBUG" /D "WIN32" /D "I2R_LOSSLESS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "objs_sls - Win32 Release"
# Name "objs_sls - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src_sls\acod.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\cbac.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\common.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_compact_tables.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_mdct.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_tns.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_win_dctiv.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\inv_int_mdct.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\lit_ll_en.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\lit_ms.c
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\mc_enc.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src_sls\aaz_enc_dec.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\acod.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\block.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\cbac.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\chandefs.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\common.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_compact_tables.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_kbd_win.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_mdct.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_mdct_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\int_win_dctiv.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\inv_int_mdct.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\lit_ll_en.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\lit_ms.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\mc_enc.h
# End Source File
# Begin Source File

SOURCE=..\..\src_sls\stdinc.h
# End Source File
# End Group
# End Target
# End Project
