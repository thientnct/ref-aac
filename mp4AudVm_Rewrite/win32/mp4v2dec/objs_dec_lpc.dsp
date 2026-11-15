# Microsoft Developer Studio Project File - Name="objs_dec_lpc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=objs_dec_lpc - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "objs_dec_lpc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "objs_dec_lpc.mak" CFG="objs_dec_lpc - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "objs_dec_lpc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "objs_dec_lpc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "objs_dec_lpc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir "."
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W2 /GX /Z7 /Od /I "..\..\src_tf" /I "..\..\src_par" /I "..\..\src_lpc" /I "..\..\..\import\include" /I "..\..\..\epTool\src" /I "..\..\src_frame" /I "..\..\src_sls" /D "_WINDOWS" /D "VERSION2" /D "SRS" /D "PNS" /D "CORRIGENDUM1" /D "ESC_SUPPORT" /D "EXT2PAR" /D "USE_AFSP" /D "NDEBUG" /D "WIN32" /D "I2R_LOSSLESS" /D "CT_SBR" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x413
# ADD RSC /l 0x413
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "objs_dec_lpc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir "."
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
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

# Name "objs_dec_lpc - Win32 Release"
# Name "objs_dec_lpc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\..\src_lpc\att_abs_postp.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\att_bwx.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\att_firfilt.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\att_iirfilt.c
# End Source File
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

SOURCE=..\..\src_lpc\celp_bitstream_demux.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\celp_decoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\dec_lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nb_celp_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_abs_exc_generation.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_acb_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_exc_generation.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_gain_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_module.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_mp_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_qlsp_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_bws_qrms_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_cng_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_cng_lspvq.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_cng_module.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_enh_gain_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_enh_mp_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_acb_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_gain_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_module.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_mp_config.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_exc_mp_dec.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\nec_qrms_dec.c
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

SOURCE=..\..\src_lpc\phi_fbit.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_gxit.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_lsfr.c
# End Source File
# Begin Source File

SOURCE=..\..\src_lpc\phi_post.c
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
