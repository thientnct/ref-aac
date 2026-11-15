# Microsoft Developer Studio Project File - Name="objs_ssc_enc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_ssc_enc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_ssc_enc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_ssc_enc.mak" CFG="objs_ssc_enc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_ssc_enc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_ssc_enc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_ssc_enc - Win32 Release"

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
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libsndfile.lib /nologo /subsystem:console /profile /map /machine:I386 /out:"./Release/objs_ssc_enc.exe" /libpath:"..\libsndfile-0.0.27\Win32\LibSndFile\Release"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "./include" /I "..\..\src_ssc\import" /I "..\..\..\import\include" /I "..\..\src_tf" /I "..\..\src_frame" /I "..\..\src_sls" /D "EXT2PAR" /D "_CONSOLE" /D "_MBCS" /D "PLATFORM_WIN32_MSVC" /D "SSC_DEC_LEVEL_MEDIUM" /D "_SSC_STANDALONE" /D "SSC_MP4REF_BUILD" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib

!ELSEIF  "$(CFG)" == "objs_ssc_enc - Win32 Debug"

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
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libsndfile.lib /nologo /subsystem:console /debug /machine:I386 /out:"./Debug/objs_ssc_enc.exe" /pdbtype:sept /libpath:"..\libsndfile-0.0.27\Win32\LibSndFile\Debug"
# SUBTRACT LINK32 /pdb:none
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /GX /ZI /Od /I "./include" /I "..\..\src_ssc\import" /I "..\..\..\import\include" /I "..\..\src_tf" /I "..\..\src_frame" /I "..\..\src_sls" /D "EXT2PAR" /D "_CONSOLE" /D "_MBCS" /D "PLATFORM_WIN32_MSVC" /D "SSC_DEC_LEVEL_MEDIUM" /D "_SSC_STANDALONE" /D "SSC_MP4REF_BUILD" /D "_DEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib

!ENDIF 

# Begin Target

# Name "objs_ssc_enc - Win32 Release"
# Name "objs_ssc_enc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src_ssc\AdpcmTable.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Bits.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\BitstreamParser.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\ct_polyphase_ssc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\ct_psdec_ssc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\huf_s_cont.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\huf_s_nrof_births.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAdpcmGrid.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpBirthRel.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpContAbs.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpContRel_0.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpContRel_1.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpContRel_2.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpContRel_3.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufAmpSinAbs.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufCoarseIid.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufCoarseItd.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufCoarseRho.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufCoarseRhoGlobal.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFineIid.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFineItd.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFineRho.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFineRhoGlobal.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqBirthRel.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqContAbs.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqContRel_0.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqContRel_1.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqContRel_2.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqContRel_3.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufFreqSinAbs.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufNoiRelLag.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufNoiRelLsfE.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\hufNoiRelNdgE.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Log.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Merge.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\NoiseSynth.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SineSynth.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_fft.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SscDec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\StereoSynth.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SynthCtrl.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\TransSynth.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
