# Microsoft Developer Studio Project File - Name="libisoaac" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libisoaac - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "libisoaac.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "libisoaac.mak" CFG="libisoaac - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "libisoaac - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "libisoaac - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libisoaac - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\import\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "NEW_ADTS" /D "WRITE_NEW_ADTS" /D "DRC" /D "BIGDECODER" /D "USELIB_AFsp" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libisoaac - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\import\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "NEW_ADTS" /D "WRITE_NEW_ADTS" /D "DRC" /D "BIGDECODER" /D "USELIB_AFsp" /YX /FD /GZ /c
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

# Name "libisoaac - Win32 Release"
# Name "libisoaac - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\adifdec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bitinput.c
# End Source File
# Begin Source File

SOURCE=..\..\src\block.c
# End Source File
# Begin Source File

SOURCE=..\..\src\config.c
# End Source File
# Begin Source File

SOURCE=..\..\src\coupling.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_envcalc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_envdec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_envextr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_freqsca.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_hfgen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_polyphase.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_psdec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrbitb.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrcrc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrdec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrdecoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src\diagnose.c
# End Source File
# Begin Source File

SOURCE=..\..\src\dolby_adapt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\drc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_common.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_compensate.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_imdct.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_ipqf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_mdct_common.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_pqf_common.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_unpac.c
# End Source File
# Begin Source File

SOURCE=..\..\src\huffdec1.c
# End Source File
# Begin Source File

SOURCE=..\..\src\huffdec2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\huffdec3.c
# End Source File
# Begin Source File

SOURCE=..\..\src\huffinit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hufftables.c
# End Source File
# Begin Source File

SOURCE=..\..\src\intensity.c
# End Source File
# Begin Source File

SOURCE=..\..\src\intrins.c
# End Source File
# Begin Source File

SOURCE=..\..\src\monopred.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nok_lt_prediction.c
# End Source File
# Begin Source File

SOURCE=..\..\src\pns.c
# End Source File
# Begin Source File

SOURCE=..\..\src\stereo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\tns.c
# End Source File
# Begin Source File

SOURCE=..\..\src\transfo.c
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\adifdec.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bitinput.h
# End Source File
# Begin Source File

SOURCE=..\..\src\block.h
# End Source File
# Begin Source File

SOURCE=..\..\src\chandefs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\coupling.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_envcalc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_envdec.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_envextr.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_freqsca.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_hfgen.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_polyphase.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_psdec.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrbitb.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrconst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrcrc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrdec.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ct_sbrdecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\src\diagnose.h
# End Source File
# Begin Source File

SOURCE=..\..\src\dolby_adapt.h
# End Source File
# Begin Source File

SOURCE=..\..\src\dolby_win.h
# End Source File
# Begin Source File

SOURCE=..\..\src\dolby_win_ssr.h
# End Source File
# Begin Source File

SOURCE=..\..\src\drc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\gc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\gc_unpac.h
# End Source File
# Begin Source File

SOURCE=..\..\src\huffdec.h
# End Source File
# Begin Source File

SOURCE=..\..\src\huffdec_intern.h
# End Source File
# Begin Source File

SOURCE=..\..\src\huffinit.h
# End Source File
# Begin Source File

SOURCE=..\..\src\hufftables.h
# End Source File
# Begin Source File

SOURCE=..\..\src\intensity.h
# End Source File
# Begin Source File

SOURCE=..\..\src\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\src\intrins.h
# End Source File
# Begin Source File

SOURCE=..\..\src\monopred.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nok_lt_prediction.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nok_ltp_common.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nok_ltp_common_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\pns.h
# End Source File
# Begin Source File

SOURCE=..\..\src\stereo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\tns.h
# End Source File
# Begin Source File

SOURCE=..\..\src\transfo.h
# End Source File
# End Group
# End Target
# End Project
