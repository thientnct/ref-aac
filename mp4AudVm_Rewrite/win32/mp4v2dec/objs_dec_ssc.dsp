# Microsoft Developer Studio Project File - Name="objs_dec_ssc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_dec_ssc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_dec_ssc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_dec_ssc.mak" CFG="objs_dec_ssc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_dec_ssc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_dec_ssc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/MPEG_SW/C/MPEG4 SSC/vm_20030330/refSoft/mp4AudVm_Rewrite/win32/mp4v2dec", ONSAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_dec_ssc - Win32 Release"

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
# ADD CPP /nologo /W4 /GX /Od /Ob2 /I "..\..\src_ssc\import\release" /I "..\..\src_ssc\import" /I "..\..\..\epTool\src" /I "..\..\..\import\include" /I "..\..\src_frame" /I "..\..\src_sls" /D "LOG_DISABLE" /D "_MBCS" /D "_LIB" /D "PLATFORM_WIN32_MSVC" /D "SSC_DEC_LEVEL_MEDIUM" /D "_SSC_LIB_BUILD" /D "SSC_MP4REF_BUILD" /D "CT_SBR" /D "EXT2PAR" /D "SSC_CONF_TOOL" /D "USE_AFSP" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\src_ssc\release\SSCdec.lib"

!ELSEIF  "$(CFG)" == "objs_dec_ssc - Win32 Debug"

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
# ADD CPP /nologo /W4 /Gm /Gi /GX /ZI /Od /I "..\..\src_ssc\import\debug" /I "..\..\src_ssc\import" /I "..\..\..\epTool\src" /I "..\..\..\import\include" /I "..\..\src_frame" /I "..\..\src_sls" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "PLATFORM_WIN32_MSVC" /D "SSC_DEC_LEVEL_MEDIUM" /D "_SSC_LIB_BUILD" /D "SSC_MP4REF_BUILD" /D "CT_SBR" /D "EXT2PAR" /D "SSC_CONF_TOOL" /D "USE_AFSP" /D "WIN32" /D "I2R_LOSSLESS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\src_ssc\debug\SSCdec.lib"

!ENDIF 

# Begin Target

# Name "objs_dec_ssc - Win32 Release"
# Name "objs_dec_ssc - Win32 Debug"
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

!IF  "$(CFG)" == "objs_dec_ssc - Win32 Release"

!ELSEIF  "$(CFG)" == "objs_dec_ssc - Win32 Debug"

# ADD CPP /W4

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\ct_polyphase_ssc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\ct_psdec_ssc.c
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

!IF  "$(CFG)" == "objs_dec_ssc - Win32 Release"

# ADD CPP /FAcs

!ELSEIF  "$(CFG)" == "objs_dec_ssc - Win32 Debug"

!ENDIF 

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
# Begin Source File

SOURCE=..\..\src_ssc\Bits.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\BitstreamParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\BitstreamParserP.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\ct_polyphase_ssc.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\ct_psdec_ssc.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Log.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Merge.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\NoiseSynth.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\NoiseSynthP.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SineSynth.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SscDec.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SscDecP.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\StereoSynth.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\StereoSynthP.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SynthCtrl.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\SynthCtrlP.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\TransSynth.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\Window.h
# End Source File
# End Group
# Begin Group "Imported Files"

# PROP Default_Filter "*.c *.h"
# Begin Group "Source"

# PROP Default_Filter "*.c"
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

SOURCE=..\..\src_ssc\import\SSC_fft.c
# End Source File
# End Group
# Begin Group "Include"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\src_ssc\import\cexcept.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\HuffmanDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\PlatformTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_Alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_BitStream.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_BitStrm_Types.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_Error.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_fft.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_SigProc_Types.h
# End Source File
# Begin Source File

SOURCE=..\..\src_ssc\import\SSC_System.h
# End Source File
# End Group
# End Group
# End Target
# End Project
