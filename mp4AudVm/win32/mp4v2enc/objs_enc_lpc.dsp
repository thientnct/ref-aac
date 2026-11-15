# Microsoft Developer Studio Project File - Name="objs_enc_lpc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_enc_lpc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_enc_lpc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_enc_lpc.mak" CFG="objs_enc_lpc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_enc_lpc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_enc_lpc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_enc_lpc - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\src_tf" /I "..\..\src_tf_enc" /I "..\..\src_par" /I "..\..\src_par_enc" /I "..\..\src_frame" /I "..\..\src_frame_enc" /I "..\..\src_lpc" /I "..\..\src_lpc_enc" /I "..\..\src_g723" /I "..\..\src_g723_enc" /I "..\..\src_g729" /I "..\..\src_g729_enc" /I "..\..\..\import\include" /D "NDEBUG" /D "_WINDOWS" /D "VERSION2" /D "AL_CONFIG" /D "USE_AFSP" /D "MP4FF" /D "CORRIGENDUM1" /D "SRS" /D "PNS" /D "WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_enc_lpc - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /Z7 /Od /I "..\..\src_tf" /I "..\..\src_tf_enc" /I "..\..\src_par" /I "..\..\src_par_enc" /I "..\..\src_frame" /I "..\..\src_frame_enc" /I "..\..\src_lpc" /I "..\..\src_lpc_enc" /I "..\..\src_g723" /I "..\..\src_g723_enc" /I "..\..\src_g729" /I "..\..\src_g729_enc" /I "..\..\..\import\include" /D "_DEBUG" /D "_WINDOWS" /D "VERSION2" /D "AL_CONFIG" /D "USE_AFSP" /D "MP4FF" /D "CORRIGENDUM1" /D "SRS" /D "PNS" /D "WIN32" /FR /FD /c
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

# Name "objs_enc_lpc - Win32 Release"
# Name "objs_enc_lpc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src_lpc\att_lsf2pc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\att_pc2lsf.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\att_testbound.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\celp_bitstream_mux.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\celp_encoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\enc_lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nb_celp_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_abs_exc_analysis.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_bws_acb_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_bws_exc_analysis.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_bws_gain_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_bws_lpf.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_module.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_bws_mp_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_bws_qlsp_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_qrms_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_cng.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_cng_lspvq.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_cng_module.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_dtx.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_enh_gain_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_enh_mp_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_exc_acb_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_exc_gain_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_module.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_mp_config.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_exc_mp_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_mode_decision.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\nec_qrms_enc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_vad.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\pan_lsp_intp.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\pan_lspdec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\pan_lspqtz2_dd.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\pan_mv_data.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\pan_sort.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\pan_stab.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\phi_apre.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\phi_axit.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_fbit.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_lsfr.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\phi_nec_lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc_enc\phi_prep.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_priv.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_xits.c
# End Source File
# End Group
# End Target
# End Project
