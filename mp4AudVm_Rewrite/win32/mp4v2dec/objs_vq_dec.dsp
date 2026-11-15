# Microsoft Developer Studio Project File - Name="objs_vq_dec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_vq_dec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_vq_dec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_vq_dec.mak" CFG="objs_vq_dec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_vq_dec - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_vq_dec - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_vq_dec - Win32 Release"

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
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_vq_dec - Win32 Debug"

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

# Name "objs_vq_dec - Win32 Release"
# Name "objs_vq_dec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_tf\mat_scale_set_shift_para.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\mat_scale_tf_requantize_spectrum.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_BitUnPack.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_check.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_cnst_chk.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_bark_env.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_gain.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_lpc_spectrum_inv.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_pgain.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_pit_seq.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_pitch.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_dec_sq_gain.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_denormalizer_spectrum.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_extend_pitch.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_fwex.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_get_cdbk.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_get_code.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_headerdec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_lsp_decw.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_lsptowt_int.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_movdd.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_mulawinv.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_post_process.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_redec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_scale_dec_bark_env.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_scale_init.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_scale_message.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_scale_tf_proc_spectrum_d.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_scale_vq_decoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_SclBitUnPack.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_set_isp.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_tf_proc_spectrum_d.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_tf_requantize_spectrum.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_TfInit.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_vec_lenp.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_vex_pn.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_vq_decoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src_tf\ntt_zerod.c
# End Source File
# End Group
# End Target
# End Project
